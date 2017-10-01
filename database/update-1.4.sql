-- Version 1.3 ==> 1.4
-- BEGIN;

CREATE TYPE fieldattr AS ENUM ('rewrite_protected');
ALTER TYPE fieldattr OWNER TO lanview2;

CREATE TABLE field_attrs (
    field_attr_id   bigserial   PRIMARY KEY,
    table_name      text        NOT NULL,
    field_name      text        NOT NULL,
    attrs           fieldattr[] NOT NULL,
    UNIQUE(table_name, field_name)
);
ALTER TABLE field_attrs OWNER TO lanview2;

INSERT INTO field_attrs(    table_name,     field_name,         attrs   )
     VALUES            (    'patchs',       'place_id',         '{rewrite_protected}' ),
                       (    'nodes',        'inventory_number', '{rewrite_protected}' ),
                       (    'nodes',        'serial_number',    '{rewrite_protected}' ),
                       (    'nodes',        'place_id',         '{rewrite_protected}' ),
                       (    'snmpdevices',  'inventory_number', '{rewrite_protected}' ),
                       (    'snmpdevices',  'serial_number',    '{rewrite_protected}' ),
                       (    'snmpdevices',  'place_id',         '{rewrite_protected}' );

SELECT set_db_version(1, 4);

ALTER TABLE patchs ADD COLUMN model_number text DEFAULT NULL;
ALTER TABLE patchs ADD COLUMN model_name   text DEFAULT NULL;


CREATE OR REPLACE FUNCTION post_insert_phs_links() RETURNS trigger AS $$
DECLARE
    modif       boolean := FALSE;   -- UPDATE esetén, ha érdemi változtatás történt
    lid         bigint;            -- phs_link_id
    pid         bigint;            -- port_id a lánc elején
    lrec_r      phs_links;          -- phs_links rekord jobra menet / végpont felöli menet
    lrec_l      phs_links;          -- phs_links rekord balra menet
    psh         portshare;          -- Eredő share
    psh_r       portshare;          -- Mentett eredő share a jobramenet végén
    dir         linkdirection;      -- Végig járási (kezdő) irány
    path_r      bigint[];          -- Linkek (id) lánca, jobra menet, vagy végpontrol menet
    path_l      bigint[];          -- Linkek (id) lánca, barla menet
    shares      portshare[];
    shix        bigint;            -- Index a fenti tömbön
    lt          linktype;
    logl    log_links_table;
BEGIN
    IF TG_OP = 'UPDATE' THEN
        modif = NOT (OLD.port_id1       = NEW.port_id1       AND OLD.port_id2       = NEW.port_id2      AND
                     OLD.port_shared    = NEW.port_shared    AND
                     OLD.phs_link_type1 = NEW.phs_link_type1 AND OLD.phs_link_type2 = NEW.phs_link_type2);
        NEW.modify_time = NOW();
    END IF;
    IF modif OR TG_OP = 'DELETE' THEN
        -- Töröljük a log_links_table rekordot, ha van hozzá
        DELETE FROM log_links_table WHERE OLD.phs_link_id = ANY (phs_link_chain);
        IF TG_OP = 'DELETE' THEN    -- Delete esetén nincs több teendő
            RETURN OLD;
        END IF;
    ELSIF TG_OP = 'UPDATE' THEN     -- Nem változik lényeges dolog, igy kész
        RETURN NEW;
    END IF;
    -- Ha végpontok közötti link, akkor ez egy logikai link is egyben
    IF NEW.phs_link_type1 = 'Term' AND NEW.phs_link_type2 = 'Term' THEN
    RAISE INFO 'Insert logical link record 1-1 ...';
        path_r[1] := NEW.phs_link_id;
        INSERT INTO log_links_table (port_id1,     port_id2,     link_type,     phs_link_chain)
                             VALUES (NEW.port_id1, NEW.port_id2, NEW.link_type, path_r)
                             RETURNING * INTO logl;
        RAISE INFO 'Log links record id = %', logl.log_link_id;
        RETURN NEW;
    -- Ha az egyik fele végpont
    ELSIF NEW.phs_link_type1 = 'Term' OR NEW.phs_link_type2 = 'Term' THEN
        -- De merre is indulunk
        IF NEW.phs_link_type1 = 'Term' THEN
            dir := 'Right';
            -- RAISE INFO 'Végigjárás végponttól: Right';
            pid := NEW.port_id1;    -- Az innenső végponti port id
        ELSE
            dir := 'Left';
            -- RAISE INFO 'Végigjárás végponttól: Left';
            pid := NEW.port_id2;    -- Az innenső végponti port id
        END IF;
        lt  := NEW.link_type;
        psh := NEW.port_shared;
        -- NODE => PATCH ->?
        path_r[1] := NEW.phs_link_id;
        lrec_r := NEW;
        LOOP
            -- RAISE INFO '******* loop : psh = "%" lrec_r: id = %, % -> %',psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
            lrec_r := next_phs_link(lrec_r, psh, dir);
            psh := lrec_r.port_shared;
            -- RAISE INFO 'After call, psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
            dir := 'Right';     -- Ha tovább megyünk, az miníg jobbara lessz!
            IF psh IS NULL THEN     -- Nincs új logikai link.
                -- RAISE INFO 'Not found logical link.';
                RETURN NEW;
            END IF;
            -- RAISE INFO 'Path : [%] <@ [%]', lrec_r.phs_link_id, array_to_string(path_r, ',');
            IF  ARRAY[lrec_r.phs_link_id] <@ path_r   -- körhivatkozás ?
             OR array_length(path_r, 1) > 10 THEN   -- Legfeljebb ekkora lehet a lánc
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
    ELSE  -- Ha min a két fele patch
        -- RAISE INFO 'Végigjárás jobra, majd balra, megosztásonként.';
        shares := ARRAY['','A','B','AA','AB','BA','BB','C','D'];    -- A lehetséges, végig próbálandó share-ek
        WHILE array_length(shares, 1) > 0 LOOP
            path_r := ARRAY[NEW.phs_link_id];           -- A path tömb a jobra bejáráshoz, és a kiíndulópont
            path_l := ARRAY[]::bigint[];               -- A path tömb a balra bejáráshoz
            psh    := shares[1];                        -- az első elemmel dolgozunk
            shares := shares[2:array_upper(shares,1)];  -- és ki is kapjuk az első elemet a tömbből
            lrec_r := NEW;                              -- Kiindulási pont az új rekord, elöszőr jobra
            lrec_l := NEW;                              -- majd ugyaninnen balra is
            dir    := 'Right';
            -- RAISE INFO '******** Loop ..., Right/Left chanin share : "%"', psh;
            -- Elindulunk 'jobra'
            LOOP
                -- RAISE INFO '>>>>>> Loop , psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                lrec_r := next_phs_link(lrec_r, psh, dir);
                psh    := lrec_r.port_shared;
                -- RAISE INFO 'NEXT Right : psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                EXIT WHEN psh IS NULL;     -- Erre nem találtunk végpontot
                IF  lrec_r.phs_link_id = ANY (path_r)   -- körhivatkozás
                OR array_length(path_r, 1) > 10 THEN   -- Legfeljebb ekkora lehet a lánc
                    PERFORM error('Loop', lrec_r.phs_link_id, 'phs_link_id', 'post_insert_phs_links()', TG_TABLE_NAME, TG_OP);
                END IF;
                path_r := array_append(path_r, lrec_r.phs_link_id);
                EXIT WHEN lrec_r.phs_link_type2 = 'Term';    -- Bingo, persze csak jobbra
            END LOOP;
            CONTINUE WHEN psh IS NULL;      -- Nincs találat
            -- Elindulunk 'balra'
            dir := 'Left';  -- de ez csak az első ugrásra vonatkozik!
            LOOP
                -- RAISE INFO '<<<<<< Loop , psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                lrec_l := next_phs_link(lrec_l, psh, dir);
                psh := lrec_l.port_shared;
                -- RAISE INFO 'NEXT Left : psh = "%" lrec_r: id = %, % -> %', psh, lrec_r.phs_link_id, lrec_r.port_id1, lrec_r.port_id2;
                dir := 'Right';
                EXIT WHEN psh IS NULL;     -- Erre nem találtunk végpontot
                IF  lrec_l.phs_link_id = ANY (path_l)   -- körhivatkozás
                OR array_length(path_l, 1) > 10 THEN   -- Legfeljebb ekkora lehet a lánc
                    PERFORM error('Loop', lrec_l.phs_link_id, 'phs_link_id', 'post_insert_phs_links()', TG_TABLE_NAME, TG_OP);
                END IF;
                path_l := array_prepend(lrec_l.phs_link_id, path_l);
                EXIT WHEN lrec_l.phs_link_type2 = 'Term';    -- Bingo mind két irány.
            END LOOP;
            IF psh IS NOT NULL THEN           -- bingo
                lt := link_type12(lrec_r.link_type, lrec_l.link_type);
                path_r := path_l || path_r;
                RAISE INFO 'Insert logical link record %(%) - %(%) [%] ...',
                        port_id2full_name(lrec_l.port_id2), lrec_l.port_id2, port_id2full_name(lrec_r.port_id2), lrec_r.port_id2, array_to_string(path_r, ',');
                INSERT INTO log_links_table (port_id1,        port_id2,        phs_link_chain, share_result, link_type)
                                     VALUES (lrec_l.port_id2, lrec_r.port_id2, path_r,         psh,          lt);
                -- RAISE INFO 'Call shares_filt(%, %)', shares, psh;
                shares = shares_filt(shares, psh);  -- A találattal ütköző share-ket kiszűrjük.
                -- RAISE INFO 'Result shares_filt() : %', shares;
                RETURN NEW;
            END IF;
        END LOOP;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
  
-- A trigger függvény a log_links_table ellenörzésére szolgál
CREATE OR REPLACE FUNCTION check_log_links() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
BEGIN
    IF TG_OP = 'UPDATE' THEN
        -- Csak a log_link_note mező módisítása megengedett
        IF (NEW.port_id1  <> OLD.port_id1 OR NEW.port_id2 <> OLD.port_id2 OR
            NEW.link_type <> OLD.link_type OR
            NEW.phs_link_chain <> OLD.phs_link_chain OR
            NEW.share_result <> OLD.share_result) THEN
            PERFORM error('Constant', -1, '', 'check_log_links()', TG_TABLE_NAME, TG_OP);
        END IF;
        RETURN NEW;
    END IF;
    -- RAISE INFO 'INSERT log.link %(%) -- %(%) [%]', port_id2full_name(NEW.port_id1), NEW.port_id1, port_id2full_name(NEW.port_id2), NEW.port_id2, NEW.phs_link_chain;
    n := COUNT(*) FROM log_links WHERE port_id1 = NEW.port_id1;
    IF 0 < n THEN
        -- RAISE WARNING 'port_id1 = %(%), found % record(s).', port_id2full_name(NEW.port_id1), NEW.port_id1, n;
        PERFORM error('IdNotUni', NEW.port_id1, 'port_id1', 'check_log_links()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- END;