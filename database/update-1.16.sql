
-- In the original function, the first parameter is superfluous
-- 2018.10.07. Bugfix. management of branches
DROP FUNCTION IF EXISTS next_phs_link(bigint, bigint, phslinktype, portshare);

CREATE OR REPLACE FUNCTION next_patch(
    pid         bigint,	    -- port_id
    link_type   phslinktype,-- link type
    sh          portshare   -- Share
)   RETURNS phs_links AS    -- Találat, modosított
$$
DECLARE
    port        pports;         -- patch port rekord
    osh         portshare;      -- Result sharing
    tsh         portshare;      -- temp.
    orec        phs_links;      -- Returned value. Modified phs_links record
    trec        phs_links;      -- temp.
    f           boolean;
BEGIN
    osh := sh;
    BEGIN   -- Get patch port record
        SELECT * INTO STRICT port FROM pports WHERE port_id = pid;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN     -- Not found. Incorrect parameter.
                PERFORM error('NotFound', pid, 'port_id', 'next_phs_link()', 'pports');
            WHEN TOO_MANY_ROWS THEN     -- Incredible, Database data error
                PERFORM error('Ambiguous',pid, 'port_id', 'next_phs_link()', 'pports');
    END;
    RAISE INFO 'port = %:%/%/%', node_id2name(port.node_id), port.port_name, link_type, sh;
    IF link_type = 'Front' THEN        -- If Front, then go back
        RAISE INFO 'Is Front, go back';
        IF port.shared_cable <> '' THEN             -- Share ?
            osh := min_shared(port.shared_cable, sh);
            IF osh = 'NC' THEN  -- There is no connection
                RAISE INFO 'RETURN: osh = NC';
                RETURN NULL;         -- NULL, No next
            END IF;
            -- Fetch, real port, if shared cable
            IF port.shared_port_id IS NOT NULL THEN
                SELECT * INTO port FROM pports WHERE port_id = port.shared_port_id;
            END IF;
        END IF;
        BEGIN -- Back link. Only one can be
            SELECT * INTO STRICT orec FROM phs_links
                WHERE port_id1 = port.port_id AND phs_link_type1 = 'Back';
            EXCEPTION
                WHEN NO_DATA_FOUND THEN -- No next
                    RAISE INFO 'RETURN: NULL (break)';
                    RETURN NULL;
                WHEN TOO_MANY_ROWS THEN -- Incredible. Database data error
                    PERFORM error('Ambiguous', -1, 'port_id1', 'next_phs_link()', 'phs_links');
        END;
        orec.port_shared   := min_shared(orec.port_shared, osh);    -- Result share
        orec.phs_link_note := NULL;                                 -- There are no branches
        RAISE INFO 'next_patch() RETURN osh = "%"; orec : id = % %/% -> %/%',
            orec.port_shared, orec.phs_link_id, orec.phs_link_type1, orec.port_id1, orec.phs_link_type2, orec.port_id2;
        RETURN orec;
    ELSIF link_type = 'Back' THEN          -- If back, then go front
        RAISE INFO 'Is Back, go Front';
        -- Is back share? Get real port?
        IF port.shared_cable <> '' AND min_shared(port.shared_cable, sh) = 'NC' THEN
            RAISE INFO 'Shared cable, find real port...';
            FOR port IN SELECT * FROM pports WHERE port_id = port.shared_port_id LOOP
                EXIT WHEN min_shared(port.shared_cable, sh) <> 'NC';
            END LOOP;
            osh := min_shared(port.shared_cable, sh);
            IF osh = 'NC' THEN  -- Not found, no next
                RAISE INFO 'RETURN: osh IS NULL (shared cable, shared port not found)';
                RETURN NULL;
            END IF;
        END IF;
        -- Due to cable divisions, you need to look for the real link. More can be. Order by port_shared.
        f    := false;  -- Found
        orec := NULL;
        FOR trec IN SELECT * FROM phs_links WHERE port_id1 = port.port_id AND phs_link_type1 = 'Front' ORDER BY port_shared ASC LOOP
            RAISE INFO 'for : phs_link_id = %, port_shared = %', trec.phs_link_id, trec.port_shared;
            tsh := min_shared(osh, trec.port_shared);
            IF tsh <> 'NC' THEN
                IF f THEN   -- More results, shared value(s) -> note
                    IF orec.phs_link_note IS NULL THEN
                        orec.phs_link_note := tsh::text;
                    ELSE
                        orec.phs_link_note := orec.phs_link_note || ',' || tsh::text;
                    end if;
                ELSE    -- The first result is the result
                    orec := trec;
                    orec.port_shared   := tsh;
                    orec.phs_link_note := NULL;
                    f := true;
                END IF;
            END IF;
        END LOOP;
        RAISE INFO 'RETURN sh = "%"/[%]; rrec id = % % -> %',
            orec.port_shared, orec.phs_link_note, orec.phs_link_id, orec.port_id1, orec.port_id2;
        RETURN orec;
    ELSE
        -- Is not patch. Incorrect parameter.
        PERFORM error('Params', -1, 'phs_link_type2', 'next_phs_link', 'phs_links');
    END IF;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION next_patch(bigint, phslinktype, portshare) IS $$
Find the next link. It assumes that the first port_id parameter identifies a patch port.
The direction of the search link is determined by the second parameter, which belongs to the current link, that is
'front' in the direction of the back, in the direction of 'back' in the front panel. Returned value is the recorded phs_links record,
or NULL. In the return record, the port_shared field is modified, the value is set as the parameter, and the scanned
the result of sharing. If there are multiple possible link records, the lowest share returns the function
(if there is one A and B, then A), the rest of the records can be obtained by modifying the second parameter. List of additional shares found in the note field.$$;

-- Előkapjuk a következő linket a megadott irányban
-- Vigyázz! A beolvasott link rekorddal ha tovább megyünk, az legyen mindíg Right, vagy visszafordulunk!
CREATE OR REPLACE FUNCTION next_phs_link(
    rec   phs_links,      -- Az aktuális fizikai link rekord, majd a következő
    sh    portshare,      -- SHARE, ha 'NC', akkor nincs következő rekord
    dir   linkdirection   -- Irány Right: 1-2->, Left <-1-2
)   RETURNS phs_links AS  -- A visszaadott rekordban van tárolva az eredő megosztás, tehát ez a mező nem a rekordbeli értéket tartalmazza.
$$
DECLARE
    pid         bigint;        -- port_id a megadott irányban
    link_type   phslinktype;    -- link/csatlakozás típusa a megadott irányban
BEGIN
    IF dir = 'Right' THEN
        pid         := rec.port_id2;
        link_type   := rec.phs_link_type2;
    ELSE
        pid         := rec.port_id1;
        link_type   := rec.phs_link_type1;
    END IF;
    RETURN next_patch(pid, link_type, sh);
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION next_phs_link(phs_links, portshare, linkdirection) IS $$
A következő link keresése. A második paraméter az eddigi eredő megosztás. A harmadik paraméter határozza meg
milyen irányba kell menni: 'Right' esetén a második porton kell tovább lépni, egyébként az elsőn.
A visszaadott érték a talált phs_links rekord,
vagy NULL. A visszaadott rekordban a port_shared mező modosítva lessz, értéke a paraméterként megadott, és a beolvasott
megosztás eredője. Ha több lehetséges link rekord van, akkor a legalacsonyabb megosztás értéküt adja vissza a függvény
(ha van egy A és B, akkor az A-t),a többi rekordot a második paraméter modosításával kaphatjuk vissza.
A hívás feltételezi, hogy az irányba eső port egy patch port.
$$;

CREATE OR REPLACE FUNCTION post_insert_phs_links() RETURNS trigger AS $$
DECLARE
    lid         bigint;             -- phs_link_id
    pid         bigint;             -- port_id a lánc elején
    lrec_r      phs_links;          -- phs_links rekord jobra menet / végpont felöli menet
    lrec_l      phs_links;          -- phs_links rekord balra menet
    psh         portshare;          -- Eredő share
    psh_r       portshare;          -- Mentett eredő share a jobramenet végén
    dir         linkdirection;      -- Végig járási (kezdő) irány
    path_r      bigint[];           -- Linkek (id) lánca, jobra menet, vagy végpontrol menet
    path_l      bigint[];           -- Linkek (id) lánca, barla menet
    shares      portshare[];
    shix        bigint;             -- Index a fenti tömbön
    lt          linktype;
    logl        log_links_table;
BEGIN
    IF TG_OP = 'DELETE' THEN
        -- Delete log_links if exists.
        DELETE FROM log_links_table WHERE OLD.phs_link_id = ANY (phs_link_chain);
        RETURN OLD;
    END IF;
    NEW.forward := false; -- This is always false
    IF TG_OP = 'UPDATE' AND 
                    (OLD.port_id1       = NEW.port_id1       AND OLD.port_id2       = NEW.port_id2      AND
                     OLD.port_shared    = NEW.port_shared    AND
                     OLD.phs_link_type1 = NEW.phs_link_type1 AND OLD.phs_link_type2 = NEW.phs_link_type2)
    THEN    -- There is no significant change
        RETURN NEW;
    END IF;
    NEW.modify_time = NOW();
 -- end to end link is logical link
    IF NEW.phs_link_type1 = 'Term' AND NEW.phs_link_type2 = 'Term' THEN
        RAISE INFO 'Insert logical link record 1-1 ...';
        path_r := ARRAY[NEW.phs_link_id];
        INSERT INTO log_links_table (port_id1,     port_id2,     link_type,     phs_link_chain)
                             VALUES (NEW.port_id1, NEW.port_id2, NEW.link_type, path_r)
                             RETURNING * INTO logl;
        RAISE INFO 'Log links record id = %', logl.log_link_id;
        RETURN NEW;
 -- end to patch : One end of the chain
    ELSIF NEW.phs_link_type1 = 'Term' OR NEW.phs_link_type2 = 'Term' THEN
        -- Direction?
        IF NEW.phs_link_type1 = 'Term' THEN
            dir := 'Right';
            RAISE INFO 'Step from end chain: Right';
            pid := NEW.port_id1;    -- Az innenső végponti port id
        ELSE
            dir := 'Left';
            -- RAISE INFO 'Step from end chain: Left';
            pid := NEW.port_id2;    -- Az innenső végponti port id
        END IF;
        lt  := NEW.link_type;
        psh := NEW.port_shared;
        path_r := ARRAY[NEW.phs_link_id]; -- First link
        lrec_r := NEW;
        LOOP
            RAISE INFO '******* loop : psh = "%" lrec_r: id = %, % -> %',psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
            lrec_r := next_phs_link(lrec_r, psh, dir);
            RAISE INFO 'After call next, psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
            psh := COALESCE(lrec_r.port_shared, 'NC');  -- is result share
            dir := 'Right';     -- The next step always 'Right'
            IF psh = 'NC' THEN     -- no next
                RAISE INFO 'Not found logical link.';
                RETURN NEW;
            END IF;
            RAISE INFO 'Path : [%] <@ [%]', lrec_r.phs_link_id, array_to_string(path_r, ',');
            IF  ARRAY[lrec_r.phs_link_id] <@ path_r  -- Loop ?
             OR array_length(path_r, 1) > 10 THEN    -- Too long chain ?
                PERFORM error('Loop', lrec_r.phs_link_id, 'phs_link_id', 'post_insert_phs_links()', TG_TABLE_NAME, TG_OP);
            END IF;
            path_r := array_append(path_r, lrec_r.phs_link_id);
            IF lrec_r.phs_link_type2 = 'Term' THEN    -- Bingo
                lt := link_type12(lt, lrec_r.link_type);
                RAISE INFO 'Insert logical link record (*)%(%) - %(%) [%]...',
                        port_id2full_name(pid), pid, port_id2full_name(lrec_r.port_id2), lrec_r.port_id2, array_to_string(path_r, ',');
                INSERT INTO log_links_table (port_id1, port_id2,   phs_link_chain, share_result, link_type)
                                     VALUES (pid, lrec_r.port_id2, path_r,         psh,          lt);
                RETURN NEW;
            END IF;
        END LOOP;
 -- patch to patch
    ELSE
        RAISE INFO 'Walk to the right, then to the left, per share.';
        shares := ARRAY['','A','B','AA','AB','BA','BB','C','D'];    -- probe all shares
        WHILE array_length(shares, 1) > 0 LOOP
            path_r := ARRAY[NEW.phs_link_id];           -- The path array for go right, and start point
            path_l := ARRAY[]::bigint[];                -- The path array for go left
            psh    := shares[1];                        -- Pop first share
            shares := shares[2:array_upper(shares,1)];
            lrec_r := NEW;                              -- Start point for right
            lrec_l := NEW;                              -- Start point for left
            dir    := 'Right';                          -- Go right
            RAISE INFO '******** Loop ..., Right chanin share : "%"', psh;
            -- Go right
            LOOP
                RAISE INFO '>>>>>> Loop , psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                lrec_r := next_phs_link(lrec_r, psh, dir);
                RAISE INFO 'NEXT Right : psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                psh    := COALESCE(lrec_r.port_shared, 'NC');
                EXIT WHEN psh = 'NC';
                IF  lrec_r.phs_link_id = ANY (path_r)  -- Loop ?
                OR array_length(path_r, 1) > 10 THEN   -- Too long chain ?
                    PERFORM error('Loop', lrec_r.phs_link_id, 'phs_link_id', 'post_insert_phs_links()', TG_TABLE_NAME, TG_OP);
                END IF;
                path_r := array_append(path_r, lrec_r.phs_link_id);
                EXIT WHEN lrec_r.phs_link_type2 = 'Term';    -- Bingo, right
            END LOOP;
            IF psh = 'NC' THEN
                RAISE INFO 'Broken chain to the right';
                CONTINUE;   -- No results, maybe the next
            ELSE
                RAISE INFO 'Right endpoint';
            END IF;
            -- Go left
            dir := 'Left';  -- First step only
            LOOP
                RAISE INFO '<<<<<< Loop , psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                lrec_l := next_phs_link(lrec_l, psh, dir);
                RAISE INFO 'NEXT Left : psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                psh := COALESCE(lrec_l.port_shared, 'NC');
                EXIT WHEN psh = 'NC';     -- Broken
                dir := 'Right';
                IF  lrec_l.phs_link_id = ANY (path_l)  -- Loop?
                OR array_length(path_l, 1) > 10 THEN   -- Too long chain ?
                    PERFORM error('Loop', lrec_l.phs_link_id, 'phs_link_id', 'post_insert_phs_links()', TG_TABLE_NAME, TG_OP);
                END IF;
                path_l := array_prepend(lrec_l.phs_link_id, path_l);
                EXIT WHEN lrec_l.phs_link_type2 = 'Term';    -- Bingo, right also.
            END LOOP;
            IF psh <> 'NC' THEN           -- bingo
                lt := link_type12(lrec_r.link_type, lrec_l.link_type);
                path_r := path_l || path_r;
                RAISE INFO 'Insert logical link record %(%) - %(%) [%] ...',
                        port_id2full_name(lrec_l.port_id2), lrec_l.port_id2, port_id2full_name(lrec_r.port_id2), lrec_r.port_id2, array_to_string(path_r, ',');
                INSERT INTO log_links_table (port_id1,        port_id2,        phs_link_chain, share_result, link_type)
                                     VALUES (lrec_l.port_id2, lrec_r.port_id2, path_r,         psh,          lt);
                RAISE INFO 'Call shares_filt(%, %)', shares, psh;
                shares = shares_filt(shares, psh);  -- Drop collisions shares
                RAISE INFO 'Result shares_filt() : %', shares;
            ELSE
                RAISE INFO 'Broken chain to the left';
                -- No results, maybe the next
            END IF;
        END LOOP;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION error (
    text,                   -- $1 Error name (errors.err_name)
    bigint  DEFAULT -1,     -- $2 Error Sub code
    text    DEFAULT 'nil',  -- $3 Error Sub Message
    text    DEFAULT 'nil',  -- $4 Source name (function name, ...)
    text    DEFAULT 'nil',  -- $5 Actual table name
    text    DEFAULT 'no'    -- $6 Actual TRIGGER type
) RETURNS boolean AS $$
DECLARE
    er errors%ROWTYPE;
    ui text;
    cmd text;
    con CONSTANT text := 'errlog';
    subc bigint := $2;
    subm text    := $3;
    srcn text    := $4;
    tbln text    := $5;
    trgn text    := $6;
BEGIN
    IF subc IS NULL THEN subc := -1;    END IF;
    IF subm IS NULL THEN subm := 'nil'; END IF;
    IF srcn IS NULL THEN srcn := 'nil'; END IF;
    IF tbln IS NULL THEN tbln := 'nil'; END IF;
    IF trgn IS NULL THEN trgn := 'no';  END IF;
    -- RAISE NOTICE 'called: error(%,%,%,%,%,%) ...', $1,subc,subm,srcn,tbln,trgn;
    er := error_by_name($1);
    PERFORM set_config('lanview2.last_error_code', CAST(er.error_id AS text), false);
    SELECT current_setting('lanview2.user_id') INTO ui;
    IF ui = '-1' THEN
        ui = '0';	-- nobody
    END IF;
    -- Tranzakción kívülröl kel (dblink-el) kiírni a log rekordot, mert visszagörgetheti
    SELECT 'dbname=lanview2 port=' || setting INTO cmd FROM pg_show_all_settings() WHERE name = 'port';
    PERFORM dblink_connect(con, cmd);
    cmd := 'INSERT INTO db_errs'
     || '(error_id, user_id, table_name, trigger_op, err_subcode, err_msg, func_name) VALUES ('
     || er.error_id   || ',' || ui || ',' || quote_nullable(tbln) || ','
     || quote_nullable(trgn) || ',' || subc || ',' || quote_nullable(subm) || ',' || quote_nullable(srcn)
     || ')';
    RAISE NOTICE 'Error log :  "%"', cmd;
    PERFORM dblink_exec(con, cmd, false);
    PERFORM dblink_disconnect(con);
    -- dblink vége
    CASE er.error_type
        WHEN 'Ok' THEN
            RAISE INFO 'Info %, #% %',       $1, subc, (er.error_note || subm);
        WHEN 'Warning', 'Ok' THEN
            RAISE WARNING 'WARNING %, #% %', $1, subc, (er.error_note || subm);
     -- WHEN 'Fatal', 'Error' THEN
        ELSE
            RAISE EXCEPTION 'ERROR %, #% %', $1, subc, (er.error_note || subm);
    END CASE;
    RETURN true;
END;
$$ LANGUAGE plpgsql;


SELECT set_db_version(1, 16);