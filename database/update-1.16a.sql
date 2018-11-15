-- 2018.10.17. Bugfix.

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
        RAISE INFO 'Is Back, go Front; %/%', port, sh;
        pid := COALESCE(port.shared_port_id, pid);
        RAISE INFO 'Base port_id : %', pid;
        f   := false;  -- Found
        orec := NULL;
        FOR trec IN SELECT * FROM phs_links WHERE port_id1 IN (SELECT port_id FROM pports WHERE shared_port_id = pid OR port_id = pid) AND phs_link_type1 = 'Front' ORDER BY port_shared ASC LOOP
            SELECT shared_cable INTO tsh FROM pports WHERE port_id = trec.port_id1;
            RAISE INFO 'for : phs_link_id = %, port_shared = %,%', trec.phs_link_id, trec.port_shared, tsh;
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
        RAISE INFO 'RETURN sh = "%"/[%]; rrec id = % % -> %',
            orec.port_shared, orec.phs_link_note, orec.phs_link_id, orec.port_id1, orec.port_id2;
        RETURN orec;
    ELSE
        -- Is not patch. Incorrect parameter.
        PERFORM error('Params', -1, 'phs_link_type2', 'next_phs_link', 'phs_links');
    END IF;
END;
$$ LANGUAGE plpgsql;

-- 2018.10.19 Bugfix

CREATE OR REPLACE FUNCTION shared_cable(pid bigint, pref text DEFAULT '') RETURNS text AS $$
DECLARE
    port   pports;
BEGIN
    SELECT * INTO port FROM pports WHERE port_id = pid;
    RETURN CASE 
        WHEN port.shared_port_id IS NULL AND port.shared_cable = ''::portshare THEN ''
        WHEN port.shared_port_id IS NULL THEN pref || port.shared_cable::text
        ELSE pref || port.shared_cable::text || '(' || port_id2name(port.shared_port_id) || ')'
    END;
END;
$$ LANGUAGE plpgsql;


DROP VIEW IF EXISTS phs_links_shape;
CREATE VIEW phs_links_shape AS
    SELECT phs_link_id, 
           port_id1,
            n1.node_id AS node_id1,
            n1.node_name AS node_name1,
            CASE
                WHEN phs_link_type1 = 'Front'::phslinktype THEN
                    p1.port_name  || shared_cable(port_id1, ' / ')
                ELSE
                    p1.port_name
            END AS port_name1,
            p1.port_index AS port_index1,
            p1.port_tag AS port_tag1,
            n1.node_name || ':' || p1.port_name AS port_full_name1,
            phs_link_type1,
            CASE
                WHEN phs_link_type1 = 'Front'::phslinktype THEN
                    port_shared::text
                WHEN phs_link_type1 =  'Back'::phslinktype THEN
                    shared_cable(port_id1)
                ELSE
                    ''
            END AS port_shared1,
           port_id2,
            n2.node_id AS node_id2,
            n2.node_name AS node_name2,
            CASE
                WHEN phs_link_type2 = 'Front'::phslinktype THEN
                    p2.port_name  || shared_cable(port_id2, ' / ')
                ELSE
                    p2.port_name
            END AS port_name2,
            p2.port_index AS port_index2,
            p2.port_tag AS port_tag2,
            n2.node_name || ':' || p2.port_name AS port_full_name2,
            phs_link_type2,
            CASE WHEN phs_link_type2 = 'Front'::phslinktype THEN
                    port_shared::text
                WHEN phs_link_type2 =  'Back'::phslinktype THEN
                    shared_cable(port_id2)
                ELSE
                    ''
            END AS port_shared2,
           phs_link_note,
           link_type,
           create_time,
           create_user_id,
           modify_time,
           modify_user_id,
           forward
    FROM phs_links JOIN ( nports AS p1 JOIN patchs AS n1 USING(node_id)) ON p1.port_id = port_id1
                   JOIN ( nports AS p2 JOIN patchs AS n2 USING(node_id)) ON p2.port_id = port_id2;

// Az azonos nevű függvények megkülönböztetése nem nagyon működik
DROP FUNCTION IF EXISTS set_language(integer);
CREATE OR REPLACE FUNCTION set_language_id(id integer) RETURNS integer AS $$
BEGIN
    PERFORM set_config('lanview2.language_id', id::text, false);
    RETURN id;
END;
$$ LANGUAGE plpgsql;

-- 2018.10.25. +Info

CREATE OR REPLACE FUNCTION shared_cable_back(pid bigint) RETURNS text AS $$
DECLARE
    port   pports;
    ta     text[];
BEGIN
    SELECT * INTO port FROM pports WHERE port_id = pid;
    CASE 
        WHEN port.shared_port_id IS NULL AND port.shared_cable = ''::portshare THEN
            RETURN '';
        WHEN port.shared_port_id IS NULL THEN
            SELECT array_agg(shared_cable || ':' || port_name) INTO ta FROM pports WHERE shared_port_id = port.port_id;
            IF array_length(ta, 1) > 0 THEN
                RETURN port.shared_cable::text || ' / ' || array_to_string(ta, '; ');
            ELSE
                RETURN port.shared_cable::text;
            END IF;
        ELSE
            RETURN port.shared_cable::text || '!!(' || port_id2name(port.shared_port_id) || ')';
    END CASE;
END;
$$ LANGUAGE plpgsql;


DROP VIEW IF EXISTS phs_links_shape;
CREATE VIEW phs_links_shape AS
    SELECT phs_link_id, 
           port_id1,
            n1.node_id AS node_id1,
            n1.node_name AS node_name1,
            CASE
                WHEN phs_link_type1 = 'Front'::phslinktype THEN
                    p1.port_name  || shared_cable(port_id1, ' / ')
                ELSE
                    p1.port_name
            END AS port_name1,
            p1.port_index AS port_index1,
            p1.port_tag AS port_tag1,
            n1.node_name || ':' || p1.port_name AS port_full_name1,
            phs_link_type1,
            CASE
                WHEN phs_link_type1 = 'Front'::phslinktype THEN
                    port_shared::text
                WHEN phs_link_type1 =  'Back'::phslinktype THEN
                    shared_cable_back(port_id1)
                ELSE
                    ''
            END AS port_shared1,
           port_id2,
            n2.node_id AS node_id2,
            n2.node_name AS node_name2,
            CASE
                WHEN phs_link_type2 = 'Front'::phslinktype THEN
                    p2.port_name  || shared_cable(port_id2, ' / ')
                ELSE
                    p2.port_name
            END AS port_name2,
            p2.port_index AS port_index2,
            p2.port_tag AS port_tag2,
            n2.node_name || ':' || p2.port_name AS port_full_name2,
            phs_link_type2,
            CASE WHEN phs_link_type2 = 'Front'::phslinktype THEN
                    port_shared::text
                WHEN phs_link_type2 =  'Back'::phslinktype THEN
                    shared_cable_back(port_id2)
                ELSE
                    ''
            END AS port_shared2,
           phs_link_note,
           link_type,
           create_time,
           create_user_id,
           modify_time,
           modify_user_id,
           forward
    FROM phs_links JOIN ( nports AS p1 JOIN patchs AS n1 USING(node_id)) ON p1.port_id = port_id1
                   JOIN ( nports AS p2 JOIN patchs AS n2 USING(node_id)) ON p2.port_id = port_id2;

-- 2018.10.31.  Bugfix: missing indexes

ALTER TABLE phs_links_table ADD CONSTRAINT phs_links_table_unique1 UNIQUE (port_id1, phs_link_type1, port_shared);
ALTER TABLE phs_links_table ADD CONSTRAINT phs_links_table_unique2 UNIQUE (port_id2, phs_link_type2, port_shared);
CREATE INDEX IF NOT EXISTS phs_links_table_port_id1_index ON phs_links_table(port_id1);
CREATE INDEX IF NOT EXISTS phs_links_table_port_id2_index ON phs_links_table(port_id2);

CREATE OR REPLACE FUNCTION min_shared(portshare, portshare) RETURNS portshare AS $$
BEGIN
    RETURN CASE
        WHEN $1 IS NULL OR $2 IS NULL THEN
            'NC'
        WHEN $1 = 'NC'  OR $2 = 'NC'  THEN
            'NC'
        WHEN $1 = '' OR $1 = $2 THEN
            $2
        WHEN $2 = '' THEN
            $1
        WHEN $1 < $2
         AND (   ( $1 = 'A' AND ( $2 = 'AA' OR $2 = 'AB' ) )
	      OR ( $1 = 'B' AND ( $2 = 'BA' OR $2 = 'BB' ) )) THEN
            $2
        WHEN $1 > $2
         AND (   ( $2 = 'A' AND ( $1 = 'AA' OR $1 = 'AB' ) )
	      OR ( $2 = 'B' AND ( $1 = 'BA' OR $1 = 'BB' ) )) THEN
            $1
        ELSE
            'NC'
     END;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

-- 2018.11.15.  Bugfix:
DROP INDEX IF EXISTS public.table_shape_fields_table_shape_id_index;    -- duplicate

ALTER TABLE enum_vals DROP CONSTRAINT IF EXISTS enum_vals_enum_type_name_enum_val_name_key; -- Not good, one field can be NULL
ALTER TABLE enum_vals ALTER COLUMN enum_val_name DROP NOT NULL; --There is an empty value that can be mixed with the empty value for the type.
UPDATE enum_vals SET enum_val_name = NULL WHERE enum_val_name = '';
CREATE UNIQUE INDEX enum_vals_enum_type_name_enum_val_name_key ON
	enum_vals (enum_type_name, enum_val_name) WHERE enum_val_name IS NOT NULL;
CREATE UNIQUE INDEX enum_vals_enum_type_name_key ON
	enum_vals (enum_type_name) WHERE enum_val_name IS NULL;

