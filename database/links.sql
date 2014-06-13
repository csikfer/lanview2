-- //// LAN.LLINKS_TABLE

CREATE TABLE log_links_table (
    log_link_id     bigserial          PRIMARY KEY,
    port_id1        bigint         NOT NULL UNIQUE,
    --  REFERENCES interfaces(port_id) ON DELETE CASCADE ON UPDATE RESTRICT,
    port_id2        bigint         NOT NULL UNIQUE,
    --  REFERENCES interfaces(port_id) ON DELETE CASCADE ON UPDATE RESTRICT,
    log_link_note  varchar(255)     DEFAULT NULL,                   -- Description
    link_type       linktype        NOT NULL,
--    first_time    timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP, -- First time discovered the logical link
--    last_time     timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP, -- Last time discovered the logical link
--    source_id       bigint         NOT NULL
--        REFERENCES users(user_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    phs_link_chain  bigint[]       NOT NULL,   -- phs-link-id chains
    share_result    portshare       NOT NULL DEFAULT ''-- Ha az útvonalban megsoztás van, megadja az összeköttetés milyenségét
);
ALTER TABLE log_links_table OWNER TO lanview2;
COMMENT ON TABLE  log_links_table                   IS 'Logical Links Table';
COMMENT ON COLUMN log_links_table.log_link_id       IS 'Unique ID for logical links';
COMMENT ON COLUMN log_links_table.port_id1          IS 'Interface''s ID which connects to logical links';
COMMENT ON COLUMN log_links_table.port_id2          IS 'Interface''s ID which connects to logical links';
COMMENT ON COLUMN log_links_table.log_link_note    IS 'Description';
-- COMMENT ON COLUMN log_links_table.first_time        IS 'First time discovered the logical link';
-- COMMENT ON COLUMN log_links_table.last_time         IS 'Last time discovered the logical link';

-- //// LAN.LLINKS VIEW

CREATE VIEW log_links AS
    SELECT log_link_id,             port_id1,             port_id2, log_link_note, link_type, /*first_time, last_time, source_id,*/ phs_link_chain, share_result FROM log_links_table
    UNION
    SELECT log_link_id, port_id2 AS port_id1, port_id1 AS port_id2, log_link_note, link_type, /*first_time, last_time, source_id,*/ phs_link_chain, share_result FROM log_links_table;
ALTER TABLE log_links OWNER TO lanview2;
COMMENT ON VIEW log_links IS 'Symmetric View Table for logical links';

CREATE TABLE lldp_links_table (
    lldp_link_id bigserial         PRIMARY KEY,
    port_id1    bigint REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    port_id2    bigint REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    first_time  timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP, -- First time discovered the logical link
    last_time   timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP  -- Last time discovered the logical link
);
ALTER TABLE lldp_links_table OWNER TO lanview2;

CREATE OR REPLACE VIEW lldp_links AS
    SELECT lldp_link_id,             port_id1,             port_id2, first_time, last_time FROM lldp_links_table
    UNION
    SELECT lldp_link_id, port_id2 AS port_id1, port_id1 AS port_id2, first_time, last_time FROM lldp_links_table;
ALTER TABLE lldp_links OWNER TO lanview2;
COMMENT ON VIEW lldp_links IS 'Symmetric View Table for LLDP links';

-- //// LAN.PLINKS_TABLE
CREATE TYPE  phslinktype AS ENUM (
    'Front',    -- Patch panel front
    'Back',     -- Patch panel back
    'Term'      -- terminal (node)
);
ALTER TYPE phslinktype OWNER TO lanview2;
CREATE TABLE phs_links_table (
    phs_link_id     bigserial          PRIMARY KEY,
    port_id1        bigint, -- REFERENCES nports(port_id) ON DELETE CASCADE ON UPDATE RESTRICT,
    port_id2        bigint, -- REFERENCES nports(port_id) ON DELETE CASCADE ON UPDATE RESTRICT,
    phs_link_type1  phslinktype     NOT NULL,
    phs_link_type2  phslinktype     NOT NULL,
    phs_link_note  varchar(255)    DEFAULT NULL,
    port_shared     portshare       NOT NULL DEFAULT '',
    link_type       linktype        NOT NULL,
    create_time     timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    create_user_id  bigint         DEFAULT NULL,
    modify_time     timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    modify_user_id  bigint         DEFAULT NULL,
    forward         boolean         DEFAULT true
);
ALTER TABLE phs_links_table OWNER TO lanview2;
COMMENT ON TABLE  phs_links_table                   IS 'Phisical Links Table';
COMMENT ON COLUMN phs_links_table.phs_link_id       IS 'Unique ID for phisical links';
COMMENT ON COLUMN phs_links_table.port_id1          IS 'Port''s ID(1) which connects to physical link';
COMMENT ON COLUMN phs_links_table.phs_link_type1    IS $$Link típusa(1), végpont 'Term', patch panel előlap 'Front', vagy hátlap 'Back'$$;
COMMENT ON COLUMN phs_links_table.port_id2          IS 'Port''s ID(2) which connects to physical link';
COMMENT ON COLUMN phs_links_table.phs_link_type2    IS $$Link típusa(2), végpont 'Term', patch panel előlap 'Front', vagy hátlap 'Back'$$;
COMMENT ON COLUMN phs_links_table.port_shared       IS $$Mindíg a 'Front' patch portra vonatkozik, ha mindkettő 'Front' patch port, akkor csak '' lehet, vagyis a megosztás tiltott$$;
COMMENT ON COLUMN phs_links_table.phs_link_note    IS 'Description';
COMMENT ON COLUMN phs_links_table.create_time       IS 'Time setting up the physical link';
COMMENT ON COLUMN phs_links_table.create_user_id    IS 'User ID for who set this physical link';
COMMENT ON COLUMN phs_links_table.modify_time       IS 'Time modified the physical link';
COMMENT ON COLUMN phs_links_table.modify_user_id    IS 'User ID for who modified the physical link';
COMMENT ON COLUMN phs_links_table.forward           IS 'értéke mindíg true, a phs_link VIEW táblában van jelentősége, ott a plussz (reverse) soroknál false.';

-- ////

CREATE OR REPLACE VIEW phs_links AS
    SELECT phs_link_id,             port_id1,            port_id2,                   phs_link_type1,                    phs_link_type2,
           phs_link_note, port_shared, link_type, create_time, create_user_id, modify_time, modify_user_id, forward
       FROM phs_links_table
    UNION
    SELECT phs_link_id, port_id2 AS port_id1, port_id1 AS port_id2, phs_link_type2 AS phs_link_type1, phs_link_type1 AS phs_link_type2,
           phs_link_note, port_shared, link_type, create_time, create_user_id, modify_time, modify_user_id, 'f'
       FROM phs_links_table
;
ALTER TABLE phs_links OWNER TO lanview2;
COMMENT ON VIEW phs_links IS 'Symmetric View Table for physical links';

-- // Egyébb VIEW táblák

CREATE OR REPLACE VIEW log_named_links AS
    SELECT log_link_id, port_id1, n1.node_name AS node_name1, p1.port_name AS port_name1,
                        port_id2, n2.node_name AS node_name2, p2.port_name AS port_name2,
                        log_link_note, link_type, phs_link_chain, share_result
    FROM log_links JOIN ( nports AS p1 JOIN patchs AS n1 USING(node_id)) ON p1.port_id = port_id1
                   JOIN ( nports AS p2 JOIN patchs AS n2 USING(node_id)) ON p2.port_id = port_id2;
COMMENT ON VIEW log_named_links IS 'Symmetric View Table for logical links with name fields';


CREATE OR REPLACE VIEW phs_named_links AS
    SELECT phs_link_id, port_id1, n1.node_name AS node_name1, p1.port_name AS port_name1, phs_link_type1,
                        port_id2, n2.node_name AS node_name2, p2.port_name AS port_name2, phs_link_type2,
                        phs_link_note, port_shared, link_type,
                        create_time, create_user_id, modify_time, modify_user_id, forward
    FROM phs_links JOIN ( nports AS p1 JOIN patchs AS n1 USING(node_id)) ON p1.port_id = port_id1
                   JOIN ( nports AS p2 JOIN patchs AS n2 USING(node_id)) ON p2.port_id = port_id2;
COMMENT ON VIEW phs_named_links IS 'Symmetric View Table for physical links with name fields';

CREATE OR REPLACE VIEW lldp_named_links AS
 SELECT lldp_links.lldp_link_id,
        lldp_links.port_id1, n1.node_name AS node_name1, p1.port_name AS port_name1,
        lldp_links.port_id2, n2.node_name AS node_name2, p2.port_name AS port_name2,
        lldp_links.first_time, lldp_links.last_time
   FROM lldp_links
   JOIN (nports p1
   JOIN patchs n1 USING (node_id)) ON p1.port_id = lldp_links.port_id1
   JOIN (nports p2
   JOIN patchs n2 USING (node_id)) ON p2.port_id = lldp_links.port_id2;

ALTER TABLE lldp_named_links OWNER TO lanview2;
COMMENT ON VIEW lldp_named_links IS 'Symmetric View Table for lldp links with name fields';
-- -------------------------------
-- ----- Functions, TRIGGERs -----
-- -------------------------------

-- phs_links ellenörzések
-- Ellenörzi a link típusát, ill. ha null és ki tudja találni, akkor azt adja vissza
-- Mivel kell az iftypes rekord, ezért mellékesen ellenörzi a port_id-t is.
-- Busz típusu interface-k nél sem engedünk meg töbszörös csatlakozást, mert nehezen követhető,
--  valós, vagy fiktív "elosztó dobozokat" kell betenni, a korlátozás megkerüléséhez.
CREATE OR REPLACE FUNCTION phs_link_type(bigint, phslinktype) RETURNS phslinktype AS
-- $1 port_id
-- $2 phs_link_type
$$
DECLARE
    itlt    linktype;
BEGIN
    BEGIN
        -- Link típus lekérése, egyben ellenőrizzük van-e ilyen port egyáltalán
        SELECT iftype_link_type INTO STRICT itlt FROM nports JOIN iftypes USING (iftype_id) WHERE port_id = $1;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN     -- nem találtunk
                PERFORM error('NotFound', -1, 'port_id', 'phs_link_type()', 'phs_links_table', 'INSERT');
            WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű, és nagyon gáz
                PERFORM error('DataError',-1, 'port_id', 'phs_link_type()', 'phs_links_table', 'INSERT');
    END;
    IF $2 IS NULL THEN  -- Nincs megadva link típus
        IF itlt = 'ptp' OR itlt = 'bus' THEN    -- ebben az esetben kitalálható
            RETURN 'Term';                      -- kitaláltuk, és bizonyára jó is, tehát kész
        ELSIF itlt = 'patch' THEN               -- nem egyértelmű
            PERFORM error('Ambiguous', -1, 'phs_link_type', 'phs_link_type()', 'phs_links_table');
        ELSE                                    -- Nem is lehet linkje
            PERFORM error('Params', -1, 'phs_link_type', 'phs_link_type()', 'phs_links_table');
        END IF;
    END IF;
    IF $2 = 'Term' AND ( itlt = 'ptp' OR itlt = 'bus' ) THEN
        RETURN $2;  -- OK
    ELSIF ( $2 = 'Front' OR $2 = 'Back' ) AND itlt = 'patch' THEN
        RETURN $2;  -- OK
    END IF;
    PERFORM error('Params', -1, 'iftype_link_type = ' || itlt || ', phs_link_type = ' || $2, 'phs_link_type()', 'phs_links_table', 'INSERT');
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION check_phs_shared(bigint, portshare, phslinktype, bigint)
-- $1 port_id
-- $2 port_shared
-- $3 phs_link_type
-- $4 OLD.phs_link_id OR -1
    RETURNS void AS $$
DECLARE
    n bigint;
    ps portshare;
    id bigint := $4;
BEGIN
    IF $2 = '' OR $3 <> 'Front' THEN     -- Nincs SHARE
        IF $3 = 'Term' OR $3 = 'Back' THEN
            RETURN ;
        END IF;
        SELECT COUNT(*) INTO n FROM phs_links WHERE port_id1 = $1 AND phs_link_type1 = $3 AND phs_link_id <> id;
        IF n > 0 THEN
            PERFORM error('Collision', n, 'port_shared', 'check_phs_shared', 'phs_links_table');
        END IF;
        RETURN;
    ELSE
        -- Megnézzük, hogy az erre a portra vonatkozó sherelt link nem-e ütközik.
        FOR ps IN SELECT port_shared FROM phs_links WHERE port_id1 = $1 AND phs_link_type1 = $3 AND phs_link_id <> id LOOP
            IF FALSE = check_shared(ps, $2) THEN
                PERFORM error('Collision', n, 'port_shared', 'check_phs_shared', 'phs_links_table');
            END IF;
        END LOOP;
        -- Nem megengedett a megosztott link, ha valamelyik port a hátlapon meg van osztva
        SELECT shared_cable INTO ps FROM pports WHERE port_id = $1;
        IF ps <> '' THEN
            PERFORM error('Collision', n, 'port_shared and shared_cable', 'check_phs_shared', 'phs_links_table');
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION link_type12(linktype, linktype) RETURNS linktype AS
$$
BEGIN
    IF $1 <> $2 THEN
        PERFORM error('Collision', -1, $1 || ' <> ' || $2, 'link_type', 'phs_links_table');
    END IF;
    RETURN $1;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION link_type(bigint, bigint, linktype) RETURNS linktype AS
-- $1 port_id1
-- $2 port_id2
-- $3 link_type
$$
DECLARE
    plt1   linktype;
    plt2   linktype;
BEGIN
    IF $3 IS NOT NULL THEN
        RETURN $3;
    END IF;
    -- Port link típus lekérése, Ellenőrizni már nem kell
    plt1 := iftype_link_type FROM nports JOIN iftypes USING (iftype_id) WHERE port_id = $1;
    plt2 := iftype_link_type FROM nports JOIN iftypes USING (iftype_id) WHERE port_id = $2;
    CASE
        WHEN plt1 = 'patch' AND plt2 = 'patch' THEN
            RETURN 'ptp';
        WHEN plt1 = 'patch' THEN
            RETURN plt2;
        WHEN plt2 = 'patch' THEN
            RETURN plt1;
        ELSE
            RETURN link_type12(plt1, plt2);
    END CASE;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION XOR( boolean, boolean) RETURNS boolean AS $$
BEGIN
    RETURN ( $1 and not $2) or ( not $1 and $2);
END
$$ LANGUAGE plpgsql;

-- Új fizikai link felvitele elötti ellenörzések, esetleges kiegészítések
CREATE OR REPLACE FUNCTION check_insert_phs_links() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
    old_id bigint := -1; -- ilyen ID nincs, de kezelhetőbb min a NULL
    uid text;
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.phs_link_id <> OLD.phs_link_id THEN -- Nem szeretjük, ha az id megváltozik
            PERFORM error('Constant', -1, 'phs_link_id', 'check_insert_phs_links()', TG_TABLE_NAME, TG_OP);
        END IF;
        old_id := OLD.phs_link_id;
    END IF;
    -- Link típusa: ha nincs megpróbálja kitölteni, ha nem jó pampog (port_id -t is ellenörzi.)
    NEW.phs_link_type1 := phs_link_type(NEW.port_id1, NEW.phs_link_type1);
    NEW.phs_link_type2 := phs_link_type(NEW.port_id2, NEW.phs_link_type2);
    NEW.link_type := link_type(NEW.port_id1, NEW.port_id2, NEW.link_type);
    -- Egyediség
    IF TG_OP = 'INSERT' THEN
        SELECT COUNT(*) INTO n FROM phs_links
            WHERE port_id1 = NEW.port_id1 AND port_shared = NEW.port_shared AND phs_link_type1 = NEW.phs_link_type1
               OR port_id1 = NEW.port_id2 AND port_shared = NEW.port_shared AND phs_link_type1 = NEW.phs_link_type2;
    ELSE    --  UPDATE
        SELECT COUNT(*) INTO n FROM phs_links
            WHERE (port_id1 = NEW.port_id1 AND port_shared = NEW.port_shared AND phs_link_type1 = NEW.phs_link_type1
                OR port_id1 = NEW.port_id2 AND port_shared = NEW.port_shared AND phs_link_type1 = NEW.phs_link_type2)
              AND phs_link_id <> OLD.phs_link_id;
    END IF;
    IF n > 0 THEN
        PERFORM error('IdNotUni', n, 'port_id1 or port_id2', 'check_insert_phs_links()', TG_TABLE_NAME, TG_OP);
    END IF;
    -- Megosztás csak akkor, ha az egyik, és csak az egyik oldal 'Front'
    IF NEW.port_shared <> '' AND NOT XOR('Front' = NEW.phs_link_type1, 'Front' = NEW.phs_link_type2) THEN
        PERFORM error('Params', -1, 'port_shared', 'check_insert_phs_links()', TG_TABLE_NAME, TG_OP);
    END IF;
    -- Share ütküzések
    PERFORM check_phs_shared(NEW.port_id1, NEW.port_shared, NEW.phs_link_type1, old_id);
    PERFORM check_phs_shared(NEW.port_id2, NEW.port_shared, NEW.phs_link_type2, old_id);
    -- Ha nincs kitöltve az user_id
    IF NEW.create_user_id IS NULL THEN
        SELECT current_setting('lanview2.user_id') INTO uid;
        IF uid <> 'NULL' THEN
            NEW.create_user_id := CAST(uid AS bigint);
        END IF;
    END IF;
    NEW.forward := CAST('f' AS boolean);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TYPE linkdirection AS ENUM ('Left', 'Right');
ALTER TYPE linkdirection OWNER TO lanview2;
-- Előkapjuk a következő linket a megadott irányban
-- Vigyázz! A beolvasott link rekorddal ha tovább megyünk, az legyen mindíg Right, vagy visszafordulunk!
CREATE OR REPLACE FUNCTION next_phs_link(
    rec   phs_links,      -- Az aktuális fizikai link rekord, majd a következő
    sh    portshare,      -- SHARE, ha NULL, akkor nincs következő rekord
    dir   linkdirection   -- Irány Right: 1-2->, Left <-1-2
)   RETURNS phs_links AS  -- A visszaadott rekordban van tárolva az eredő megosztás, tehát ez a mező nem a rekordbeli értéket tartalmazza.
$$
DECLARE
    lid         bigint;        -- phs_link_id
    pid         bigint;        -- port_id a megadott irányban
    link_type   phslinktype;    -- link/csatlakozás típusa a megadott irányban
    port        pports;         -- port rekord a megadott irányban
    osh         portshare;      -- eredő share
    orec        phs_links;      -- Visszaadott értésk
BEGIN
    IF dir = 'Right' THEN
        pid         := rec.port_id2;
        link_type   := rec.phs_link_type2;
    ELSE
        pid         := rec.port_id1;
        link_type   := rec.phs_link_type1;
    END IF;
    lid := rec.phs_link_id;
    osh := sh;
    BEGIN   -- Előkapjuk a pports rekordot, pontosan egynek kell lennie
        SELECT * INTO STRICT port FROM pports WHERE port_id = pid;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN     -- nem találtunk, ciki, mert többször ellenörzött
                PERFORM error('NotFound', pid, 'port_id', 'next_phs_link()', 'pports');
            WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű, ez is nagyon gáz
                PERFORM error('Ambiguous',pid, 'port_id', 'next_phs_link()', 'pports');
    END;
    IF link_type = 'Front' THEN        -- Ha Front, akkor hátlapon megyünk tovább
        -- RAISE INFO 'next_phs_link() Front #% DIR:%  port: %:%(%)/%', lid, dir, port.node_id, port.port_name, port.port_id, link_type;
        IF port.shared_cable <> '' THEN             -- Ha van hátlapi kábel megosztás
            osh := min_shared(port.shared_cable, sh);-- A két share minimuma, van átjárás ?
            IF osh IS NULL THEN  -- A hátlapi megosztás miatt ez rossz irány, nincs átjárás
                -- RAISE INFO 'RETURN: osh IS NULL';
                RETURN NULL;         -- NULL, nincs következő link
            END IF;
            -- A hátlapi link csak az egyik megosztásra van megadva, beolvassuk azt a portot
            IF port.shared_port_id IS NOT NULL THEN -- Az "igazi" port id-je, vagy NULL ha ez az "igazi"
                SELECT * INTO port FROM pports WHERE port_id = port.shared_port_id;
            END IF;
        END IF;
        BEGIN -- Hátlapi összeköttetés csak egy lehet, a share máshogy van lerendezve
            SELECT * INTO STRICT orec FROM phs_links
                WHERE port_id1 = port.port_id AND phs_link_type1 = 'Back' AND phs_link_id <> lid;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN -- Nincs következő link
                    -- RAISE INFO 'RETURN: sh IS NULL';
                    RETURN NULL;
                WHEN TOO_MANY_ROWS THEN     -- Ez viszont hiba
                    PERFORM error('Ambiguous', -1, 'port_id1', 'next_phs_link()', 'phs_links');
        END;
        -- RAISE INFO 'RETURN osh = "%"; orec : id = % % -> %', osh, orec.phs_link_id, orec.port_id1, orec.port_id2;
        orec.port_shared := min_shared(orec.port_shared, osh);
        RETURN orec;    -- Megvan, és kész (az eredmény (Back) 2-es portján nem foglalkoztunk a share-val)
    ELSIF link_type = 'Back' THEN          -- Ha hátlap, akkor az előlap felé megyünk
        -- RAISE INFO 'next_phs_link() Back #% DIR:%  port: %:%(%)/%', lid, dir, port.node_id, port.port_name, port.port_id, link_type;
        -- hátlapi megosztás esetán, lehet, hogy nem ez az előlapi port
        IF port.shared_cable <> '' AND min_shared(port.shared_cable, sh) IS NULL THEN
            -- tényleg nem ez az, keressük meg
            FOR port IN SELECT * FROM pports WHERE port_id = port.shared_port_id LOOP
                EXIT WHEN min_shared(port.shared_cable, sh) IS NOT NULL;
            END LOOP;
            osh := min_shared(port.shared_cable, sh);
            IF osh IS NULL THEN  -- Ha nem találtuk meg, akkor nincs link.
                -- RAISE INFO 'RETURN: osh IS NULL';
                RETURN NULL;
            END IF;
        END IF; -- Megvan az előlapi port
        -- A kábelmegosztások miatt megint keresni kell az igazi linket.
        FOR orec IN SELECT * FROM phs_links WHERE port_id1 = port.port_id AND phs_link_type1 = 'Front' AND phs_link_id <> lid LOOP
            -- RAISE INFO 'for : orec.phs_link_id = %, orec.port_shared = %', orec.phs_link_id, orec.port_shared;
            EXIT WHEN min_shared(osh, orec.port_shared) IS NOT NULL;
        END LOOP;
        osh := min_shared(sh, orec.port_shared);
        IF orec IS NULL OR osh IS NULL THEN
            -- RAISE INFO 'RETURN: NULL';
            RETURN NULL;
        END IF;
        -- RAISE INFO 'RETURN sh = "%"; rrec id = % % -> %', sh, orec.phs_link_id, orec.port_id1, orec.port_id2;
        orec.port_shared := osh;
        RETURN orec; -- Ha van találat sh nem NULL.
    ELSE
        -- Ha nem is patch, akkor nem kéne hívni ezt a függvényt.
        PERFORM error('Params', -1, 'phs_link_type2', 'next_phs_link', 'phs_links');
    END IF;
END;
$$ LANGUAGE plpgsql;

-- Fizikai link beinzertálása után, kitaláljuk a logikai linket.

CREATE OR REPLACE FUNCTION post_insert_phs_links()
  RETURNS trigger AS
$BODY$
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
        modif = !(OLD.port_id1       = NEW.port_id1       AND OLD.port_id2       = NEW.port_id2      AND
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
                RAISE INFO 'Insert logical link record...';
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
                RAISE INFO 'Insert logical link record...';
                path_r := path_l || path_r;
                INSERT INTO log_links_table (port_id1,        port_id2,        phs_link_chain, share_result, link_type)
                                     VALUES (lrec_l.port_id2, lrec_r.port_id2, path_r,         psh,          lt);
                -- RAISE INFO 'Call shares_filt(%, %)', shares, psh;
                shares = shares_filt(shares, psh);  -- A találattal ütköző share-ket kiszűrjük.
                -- RAISE INFO 'Result shares_filt() : %', shares;
            END IF;
        END LOOP;
    END IF;
    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION post_insert_phs_links() OWNER TO lanview2;

CREATE TRIGGER check_insert_phs_links BEFORE INSERT OR UPDATE           ON phs_links_table FOR EACH ROW EXECUTE PROCEDURE check_insert_phs_links();
CREATE TRIGGER post_insert_phs_links  AFTER  INSERT OR UPDATE OR DELETE ON phs_links_table FOR EACH ROW EXECUTE PROCEDURE post_insert_phs_links();

-- Hasonló mint a check_reference_port_id(), de itt a két idegen kulcs neve port_id1 és port_id2,
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
        RETURN new;
    END IF;
    IF 0 < COUNT(*) FROM log_links WHERE port_id1 = NEW.port_id1 THEN
        PERFORM error('IdNotUni', NEW.port_id1, 'port_id1', 'check_log_links()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF 0 < COUNT(*) FROM log_links WHERE port_id1 = NEW.port_id2 THEN
        PERFORM error('IdNotUni', NEW.port_id2, 'port_id2', 'check_log_links()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER log_links_table_check_log_links BEFORE UPDATE OR INSERT ON log_links_table FOR EACH ROW EXECUTE PROCEDURE check_log_links();
