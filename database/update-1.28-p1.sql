-- 2021.03.16. 

-- nem kell
DROP VIEW IF EXISTS node_tools;

-- Ez a mező felesleges, öröklések miatt kezelhetetlen, nehezen ellenörizhető
ALTER TABLE tools DROP COLUMN IF EXISTS object_name;
-- Helyette lista
ALTER TABLE tools ADD COLUMN IF NOT EXISTS object_names text[] NOT NULL DEFAULT '{"nodes","snmpdevices"}';
-- A parancs hívások nem platform függetlenek, azonosítani kell öket, hol használhatóak
-- Ha null, akkor platform független
ALTER TABLE tools ADD COLUMN IF NOT EXISTS platforms text[] DEFAULT NULL;
-- Egy függvény az ellenözzéséhez
CREATE OR REPLACE FUNCTION check_platform(tid bigint, kern text, prod text) RETURNS boolean
    LANGUAGE 'plpgsql'
AS $BODY$
DECLARE
    ps text[];
BEGIN
    SELECT platforms INTO ps FROM tools WHERE tool_id = tid;
    IF ps is NULL THEN
        RETURN true;
    ELSIF array_length(ps, 1) IS NULL THEN
        RETURN true;
    ELSIF kern = ANY (ps) THEN
        RETURN true;
    END IF;
    RETURN prod = ANY (ps);
END;
$BODY$;

ALTER FUNCTION check_platform(bigint, text)
    OWNER TO lanview2;

-- Ez lemaradt, enélkül egyetlen tools rekordot sem lehet elmenteni! (lásd.: text_id)
ALTER TYPE tablefortext ADD VALUE IF NOT EXISTS 'tools';

-- Hibajavítás
CREATE OR REPLACE FUNCTION check_reference_tool_object() RETURNS trigger
    LANGUAGE 'plpgsql'
AS $BODY$
DECLARE
    id_name text;
    stmt    text;
    objs    text[];
BEGIN
    IF NEW.object_name IS NULL THEN 
        PERFORM error('InvRef', NEW.tool_id, NEW.object_name || ' is NULL ', 'check_reference_tool_object()',  TG_TABLE_NAME, TG_OP);
    END IF;
    SELECT object_names INTO objs FROM tools WHERE tool_id = NEW.tool_id;
    IF NOT FOUND THEN 
        PERFORM error('InvRef', NEW.tool_id,'tools record not found', 'check_reference_tool_object()',  TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    IF NOT NEW.object_name = ANY (objs) THEN
        PERFORM error('Params', NEW.tool_id, NEW.object_name, 'check_reference_tool_object()',  TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    SELECT column_name INTO id_name FROM information_schema.columns         -- find table id name
        WHERE table_schema = 'public' AND table_name = NEW.object_name          -- table name
          AND ordinal_position = 1 AND column_name LIKE '%_id';                 -- first field and id?
    IF NOT FOUND THEN 
        PERFORM error('InvRef', NEW.tool_id, NEW.object_name || ' table id name is not found', 'check_reference_tool_object()',  TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    stmt := format('SELECT COUNT(*) FROM %s WHERE %s = $1', NEW.object_name, id_name);
    EXECUTE stmt USING NEW.object_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', NEW.object_id, basename, 'check_reference_tool_object()',  TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    RETURN NEW;
END;
$BODY$;

ALTER FUNCTION check_reference_tool_object()
    OWNER TO lanview2;

-- 2021.04.13.
-- phs_link_table javítása: értelmetlen a modosítást végző azonosítása, mivel mindíg replace van, ami töröl, majd beiloleszt.
-- A függőségek miatt több törlés/újradefiniálás kell :(

DROP FUNCTION IF EXISTS next_phs_link(phs_links,portshare,linkdirection);
DROP FUNCTION IF EXISTS next_patch(bigint,phslinktype,portshare);
DROP VIEW IF EXISTS phs_named_links;
DROP VIEW IF EXISTS log_named_links;
DROP VIEW IF EXISTS lldp_named_links;
DROP VIEW IF EXISTS phs_links_shape;
DROP VIEW IF EXISTS phs_links;
ALTER TABLE phs_links_table DROP COLUMN IF EXISTS modify_time;
ALTER TABLE phs_links_table DROP COLUMN IF EXISTS modify_user_id;
-- a VIEW a törölt mezők nélkül
CREATE OR REPLACE VIEW public.phs_links AS
 SELECT phs_links_table.phs_link_id,
    phs_links_table.port_id1,
    phs_links_table.port_id2,
    phs_links_table.phs_link_type1,
    phs_links_table.phs_link_type2,
    phs_links_table.phs_link_note,
    phs_links_table.port_shared,
    phs_links_table.link_type,
    phs_links_table.create_time,
    phs_links_table.create_user_id,
    true AS forward
   FROM phs_links_table
UNION
 SELECT phs_links_table.phs_link_id,
    phs_links_table.port_id2 AS port_id1,
    phs_links_table.port_id1 AS port_id2,
    phs_links_table.phs_link_type2 AS phs_link_type1,
    phs_links_table.phs_link_type1 AS phs_link_type2,
    phs_links_table.phs_link_note,
    phs_links_table.port_shared,
    phs_links_table.link_type,
    phs_links_table.create_time,
    phs_links_table.create_user_id,
    false AS forward
   FROM phs_links_table;
ALTER TABLE public.phs_links
    OWNER TO lanview2;
COMMENT ON VIEW public.phs_links
    IS 'Symmetric View Table for physical links';

CREATE OR REPLACE VIEW public.phs_links_shape AS
 SELECT phs_links.phs_link_id,
    phs_links.port_id1,
    n1.node_id AS node_id1,
    n1.node_name AS node_name1,
        CASE
            WHEN phs_links.phs_link_type1 = 'Front'::phslinktype THEN p1.port_name || shared_cable(phs_links.port_id1, ' / '::text)
            ELSE p1.port_name
        END AS port_name1,
    p1.port_index AS port_index1,
    p1.port_tag AS port_tag1,
    (n1.node_name || ':'::text) || p1.port_name AS port_full_name1,
    phs_links.phs_link_type1,
        CASE
            WHEN phs_links.phs_link_type1 = 'Front'::phslinktype THEN phs_links.port_shared::text
            WHEN phs_links.phs_link_type1 = 'Back'::phslinktype THEN shared_cable_back(phs_links.port_id1)
            ELSE ''::text
        END AS port_shared1,
    phs_links.port_id2,
    n2.node_id AS node_id2,
    n2.node_name AS node_name2,
        CASE
            WHEN phs_links.phs_link_type2 = 'Front'::phslinktype THEN p2.port_name || shared_cable(phs_links.port_id2, ' / '::text)
            ELSE p2.port_name
        END AS port_name2,
    p2.port_index AS port_index2,
    p2.port_tag AS port_tag2,
    (n2.node_name || ':'::text) || p2.port_name AS port_full_name2,
    phs_links.phs_link_type2,
        CASE
            WHEN phs_links.phs_link_type2 = 'Front'::phslinktype THEN phs_links.port_shared::text
            WHEN phs_links.phs_link_type2 = 'Back'::phslinktype THEN shared_cable_back(phs_links.port_id2)
            ELSE ''::text
        END AS port_shared2,
    phs_links.phs_link_note,
    phs_links.link_type,
    phs_links.create_time,
    phs_links.create_user_id,
    phs_links.forward
   FROM phs_links
     JOIN (nports p1
     JOIN patchs n1 USING (node_id)) ON p1.port_id = phs_links.port_id1
     JOIN (nports p2
     JOIN patchs n2 USING (node_id)) ON p2.port_id = phs_links.port_id2;
ALTER TABLE public.phs_links_shape
    OWNER TO lanview2;

CREATE OR REPLACE FUNCTION public.next_patch(pid bigint, link_type phslinktype, sh portshare)
    RETURNS phs_links LANGUAGE 'plpgsql' AS $BODY$
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
    -- RAISE INFO 'port = %:%/%/%', node_id2name(port.node_id), port.port_name, link_type, sh;
    IF link_type = 'Front' THEN        -- If Front, then go back
        -- RAISE INFO 'Is Front, go back';
        IF port.shared_cable <> '' THEN             -- Share ?
            osh := min_shared(port.shared_cable, sh);
            IF osh = 'NC' THEN  -- There is no connection
                -- RAISE INFO 'RETURN: osh = NC';
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
        -- RAISE INFO 'next_patch() RETURN osh = "%"; orec : id = % %/% -> %/%',
        --    orec.port_shared, orec.phs_link_id, orec.phs_link_type1, orec.port_id1, orec.phs_link_type2, orec.port_id2;
        RETURN orec;
    ELSIF link_type = 'Back' THEN          -- If back, then go front
        -- RAISE INFO 'Is Back, go Front; %/%', port, sh;
        pid := COALESCE(port.shared_port_id, pid);
        RAISE INFO 'Base port_id : %', pid;
        f   := false;  -- Found
        orec := NULL;
        FOR trec IN SELECT * FROM phs_links WHERE port_id1 IN (SELECT port_id FROM pports WHERE shared_port_id = pid OR port_id = pid) AND phs_link_type1 = 'Front' ORDER BY port_shared ASC LOOP
            SELECT shared_cable INTO tsh FROM pports WHERE port_id = trec.port_id1;
            -- RAISE INFO 'for : phs_link_id = %, port_shared = %,%', trec.phs_link_id, trec.port_shared, tsh;
            tsh := min_shared(tsh, osh);
            tsh := min_shared(tsh, trec.port_shared);
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
        -- RAISE INFO 'RETURN sh = "%"/[%]; rrec id = % % -> %',
        --    orec.port_shared, orec.phs_link_note, orec.phs_link_id, orec.port_id1, orec.port_id2;
        RETURN orec;
    ELSE
        -- Is not patch. Incorrect parameter.
        PERFORM error('Params', -1, 'phs_link_type2', 'next_phs_link', 'phs_links');
    END IF;
END;
$BODY$;
ALTER FUNCTION public.next_patch(bigint, phslinktype, portshare)
    OWNER TO lanview2;
COMMENT ON FUNCTION public.next_patch(bigint, phslinktype, portshare)
    IS '
Find the next link. It assumes that the first port_id parameter identifies a patch port.
The direction of the search link is determined by the second parameter, which belongs to the current link, that is
''front'' in the direction of the back, in the direction of ''back'' in the front panel. Returned value is the recorded phs_links record,
or NULL. In the return record, the port_shared field is modified, the value is set as the parameter, and the scanned
the result of sharing. If there are multiple possible link records, the lowest share returns the function
(if there is one A and B, then A), the rest of the records can be obtained by modifying the second parameter. List of additional shares found in the note field.';

CREATE OR REPLACE FUNCTION public.next_phs_link(rec phs_links, sh portshare, dir linkdirection)
    RETURNS phs_links LANGUAGE 'plpgsql' AS $BODY$
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
$BODY$;
ALTER FUNCTION public.next_phs_link(phs_links, portshare, linkdirection)
    OWNER TO lanview2;
COMMENT ON FUNCTION public.next_phs_link(phs_links, portshare, linkdirection)
    IS '
A következő link keresése. A második paraméter az eddigi eredő megosztás. A harmadik paraméter határozza meg
milyen irányba kell menni: ''Right'' esetén a második porton kell tovább lépni, egyébként az elsőn.
A visszaadott érték a talált phs_links rekord,
vagy NULL. A visszaadott rekordban a port_shared mező modosítva lessz, értéke a paraméterként megadott, és a beolvasott
megosztás eredője. Ha több lehetséges link rekord van, akkor a legalacsonyabb megosztás értéküt adja vissza a függvény
(ha van egy A és B, akkor az A-t),a többi rekordot a második paraméter modosításával kaphatjuk vissza.
A hívás feltételezi, hogy az irányba eső port egy patch port.
';

-- A triggerből is eltávolitandó a törölt mező (debug infók kikommentezve)
-- A triggerből is eltávolitandó a törölt mező (debug infók kikommentezve)
CREATE OR REPLACE FUNCTION public.post_insert_phs_links()
    RETURNS trigger LANGUAGE 'plpgsql' AS $BODY$
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
 -- end to end link is logical link
    IF NEW.phs_link_type1 = 'Term' AND NEW.phs_link_type2 = 'Term' THEN
        -- RAISE INFO 'Insert logical link record 1-1 ...';
        path_r := ARRAY[NEW.phs_link_id];
        INSERT INTO log_links_table (port_id1,     port_id2,     link_type,     phs_link_chain)
                             VALUES (NEW.port_id1, NEW.port_id2, NEW.link_type, path_r)
                             RETURNING * INTO logl;
        -- RAISE INFO 'Log links record id = %', logl.log_link_id;
        RETURN NEW;
 -- end to patch : One end of the chain
    ELSIF NEW.phs_link_type1 = 'Term' OR NEW.phs_link_type2 = 'Term' THEN
        -- Direction?
        IF NEW.phs_link_type1 = 'Term' THEN
            dir := 'Right';
            -- RAISE INFO 'Step from end chain: Right';
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
            -- RAISE INFO '******* loop : psh = "%" lrec_r: id = %, % -> %',psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
            lrec_r := next_phs_link(lrec_r, psh, dir);
            -- RAISE INFO 'After call next, psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
            psh := COALESCE(lrec_r.port_shared, 'NC');  -- is result share
            dir := 'Right';     -- The next step always 'Right'
            IF psh = 'NC' THEN     -- no next
                -- RAISE INFO 'Not found logical link.';
                RETURN NEW;
            END IF;
            -- RAISE INFO 'Path : [%] <@ [%]', lrec_r.phs_link_id, array_to_string(path_r, ',');
            IF  ARRAY[lrec_r.phs_link_id] <@ path_r  -- Loop ?
             OR array_length(path_r, 1) > 10 THEN    -- Too long chain ?
                PERFORM error('Loop', lrec_r.phs_link_id, 'phs_link_id', 'post_insert_phs_links()', TG_TABLE_NAME, TG_OP);
            END IF;
            path_r := array_append(path_r, lrec_r.phs_link_id);
            IF lrec_r.phs_link_type2 = 'Term' THEN    -- Bingo
                lt := link_type12(lt, lrec_r.link_type);
                -- RAISE INFO 'Insert logical link record (*)%(%) - %(%) [%]...',
                --        port_id2full_name(pid), pid, port_id2full_name(lrec_r.port_id2), lrec_r.port_id2, array_to_string(path_r, ',');
                INSERT INTO log_links_table (port_id1, port_id2,   phs_link_chain, share_result, link_type)
                                     VALUES (pid, lrec_r.port_id2, path_r,         psh,          lt);
                RETURN NEW;
            END IF;
        END LOOP;
 -- patch to patch
    ELSE
        -- RAISE INFO 'Walk to the right, then to the left, per share.';
        shares := ARRAY['','A','B','AA','AB','BA','BB','C','D'];    -- probe all shares
        WHILE array_length(shares, 1) > 0 LOOP
            path_r := ARRAY[NEW.phs_link_id];           -- The path array for go right, and start point
            path_l := ARRAY[]::bigint[];                -- The path array for go left
            psh    := shares[1];                        -- Pop first share
            shares := shares[2:array_upper(shares,1)];
            lrec_r := NEW;                              -- Start point for right
            lrec_l := NEW;                              -- Start point for left
            dir    := 'Right';                          -- Go right
            -- RAISE INFO '******** Loop ..., Right chanin share : "%"', psh;
            -- Go right
            LOOP
                -- RAISE INFO '>>>>>> Loop , psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                lrec_r := next_phs_link(lrec_r, psh, dir);
                -- RAISE INFO 'NEXT Right : psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
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
                -- RAISE INFO 'Broken chain to the right';
                CONTINUE;   -- No results, maybe the next
            ELSE
                -- RAISE INFO 'Right endpoint';
            END IF;
            -- Go left
            dir := 'Left';  -- First step only
            LOOP
                -- RAISE INFO '<<<<<< Loop , psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                lrec_l := next_phs_link(lrec_l, psh, dir);
                -- RAISE INFO 'NEXT Left : psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
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
                -- RAISE INFO 'Insert logical link record %(%) - %(%) [%] ...',
                --        port_id2full_name(lrec_l.port_id2), lrec_l.port_id2, port_id2full_name(lrec_r.port_id2), lrec_r.port_id2, array_to_string(path_r, ',');
                INSERT INTO log_links_table (port_id1,        port_id2,        phs_link_chain, share_result, link_type)
                                     VALUES (lrec_l.port_id2, lrec_r.port_id2, path_r,         psh,          lt);
                -- RAISE INFO 'Call shares_filt(%, %)', shares, psh;
                shares := shares_filt(shares, psh);  -- Drop collisions shares
                -- RAISE INFO 'Result shares_filt() : %', shares;
            ELSE
                -- RAISE INFO 'Broken chain to the left';
                -- No results, maybe the next
            END IF;
        END LOOP;
    END IF;
    RETURN NEW;
END;
$BODY$;

-- Hibajavítás 2021.05.13.
-- A leltári szám és szériaszám egyediségének az ellenörzése hibás volt

CREATE OR REPLACE FUNCTION node_check_before_insert() RETURNS trigger
    LANGUAGE 'plpgsql' COST 100 VOLATILE NOT LEAKPROOF
AS $BODY$
DECLARE
    n bigint;
BEGIN
--  RAISE INFO 'Insert NODE, new id = %', NEW.node_id;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_id = NEW.node_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_name = NEW.node_name;
    IF n > 0 THEN
        PERFORM error('NameNotUni', -1, 'node_name = ' || NEW.node_name, 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF NEW.serial_number IS NOT NULL THEN
        IF btrim(NEW.serial_number) = '' THEN
            NEW.serial_number := NULL;
        ELSIF COALESCE(NEW.serial_number <> OLD.serial_number, true) THEN
            SELECT COUNT(*) INTO n FROM patchs WHERE serial_number = NEW.serial_number;
            IF n > 0 THEN
                PERFORM error('NameNotUni', NEW.node_id, 'serial_number = ' || NEW.serial_number, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
            END IF;
        END IF;
    END IF;
    IF NEW.inventory_number IS NOT NULL THEN
        IF btrim(NEW.inventory_number) = '' THEN
            NEW.inventory_number := NULL;
        ELSIF COALESCE(NEW.inventory_number <> OLD.inventory_number, true) THEN
            SELECT COUNT(*) INTO n FROM patchs WHERE inventory_number = NEW.inventory_number;
            IF n > 0 THEN
                PERFORM error('NameNotUni', NEW.node_id, 'inventory_number = ' || NEW.inventory_number, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
            END IF;
        END IF;
    END IF;
    CASE TG_TABLE_NAME
        WHEN 'patchs' THEN
            NEW.node_type = '{patch}';  -- constant field
        WHEN 'nodes' THEN
            IF NEW.node_type IS NULL THEN
                NEW.node_type = '{node}';
            END IF;
            IF 'patch'::nodetype = ANY (NEW.node_type) THEN
                PERFORM error('DataError', NEW.node_id, 'node_type', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
            END IF;
        WHEN 'snmpdevices' THEN
            IF NEW.node_type IS NULL THEN
                NEW.node_type = '{host,snmp}';
            END IF;
            IF 'patch'::nodetype = ANY (NEW.node_type) THEN
                PERFORM error('DataError', NEW.node_id, 'node_type', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
            END IF;
        ELSE
            PERFORM error('DataError', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END CASE;
    RETURN NEW;
END;
$BODY$;

CREATE OR REPLACE FUNCTION node_check_before_update() RETURNS trigger
    LANGUAGE 'plpgsql' COST 100 VOLATILE NOT LEAKPROOF
AS $BODY$
DECLARE
    n bigint;
BEGIN
    IF NEW.node_name <> OLD.node_name THEN
        SELECT COUNT(*) INTO n FROM patchs WHERE node_name = NEW.node_name;
        IF n > 0 THEN
            PERFORM error('NameNotUni', NEW.node_id, 'node_name = ' || NEW.node_name, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    IF NEW.serial_number IS NOT NULL THEN
        IF btrim(NEW.serial_number) = '' THEN
            NEW.serial_number := NULL;
        ELSIF COALESCE(NEW.serial_number <> OLD.serial_number, true) THEN
            SELECT COUNT(*) INTO n FROM patchs WHERE serial_number = NEW.serial_number;
            IF n > 0 THEN
                PERFORM error('NameNotUni', NEW.node_id, 'serial_number = ' || NEW.serial_number, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
            END IF;
        END IF;
    END IF;
    IF NEW.inventory_number IS NOT NULL THEN
        IF btrim(NEW.inventory_number) = '' THEN
            NEW.inventory_number := NULL;
        ELSIF COALESCE(NEW.inventory_number <> OLD.inventory_number, true) THEN
            SELECT COUNT(*) INTO n FROM patchs WHERE inventory_number = NEW.inventory_number;
            IF n > 0 THEN
                PERFORM error('NameNotUni', NEW.node_id, 'inventory_number = ' || NEW.inventory_number, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
            END IF;
        END IF;
    END IF;
    RETURN NEW;
END;
$BODY$;

-- clear empty or spaces values
UPDATE patchs SET serial_number = NULL WHERE btrim(serial_number) = '';
UPDATE patchs SET inventory_number = NULL WHERE btrim(inventory_number) = '';

/* Check duplicate serial number :

WITH ns AS (
    SELECT serial_number, COUNT(node_id) AS n
    FROM patchs
    WHERE serial_number IS NOT NULL
    GROUP BY serial_number)
SELECT * FROM ns WHERE n > 1;

    Check duplicate inventory number
    
WITH ns AS (
    SELECT inventory_number, COUNT(node_id) AS n
    FROM patchs
    WHERE inventory_number IS NOT NULL
    GROUP BY inventory_number)
SELECT * FROM ns WHERE n > 1;
*/
-- 2021.06.10. Modify reasons

ALTER TYPE reasons ADD VALUE IF NOT EXISTS 'in_progress' AFTER 'update';

