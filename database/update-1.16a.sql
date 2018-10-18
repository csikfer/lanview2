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
