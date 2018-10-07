
-- Az eredeti függvényben az első paraméter felesleges
DROP FUNCTION IF EXISTS next_phs_link(bigint, bigint, phslinktype, portshare);
CREATE OR REPLACE FUNCTION next_patch(
    pid         bigint,	    -- port_id
    link_type   phslinktype,-- link/csatlakozás típusa a megadott irányban
    sh          portshare   -- SHARE, ha 'NC', akkor nincs következő rekord
)   RETURNS phs_links AS    -- A visszaadott rekordban van tárolva az eredő megosztás, tehát ez a mező nem a rekordbeli értéket tartalmazza.
$$
DECLARE
    port        pports;         -- port rekord a megadott irányban
    osh         portshare;      -- eredő share
    orec        phs_links;      -- Visszaadott értésk
BEGIN
    osh := sh;
    BEGIN   -- Előkapjuk a pports rekordot, pontosan egynek kell lennie
        SELECT * INTO STRICT port FROM pports WHERE port_id = pid;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN     -- nem találtunk, ciki, mert többször ellenörzött
                PERFORM error('NotFound', pid, 'port_id', 'next_phs_link()', 'pports');
            WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű, ez is nagyon gáz
                PERFORM error('Ambiguous',pid, 'port_id', 'next_phs_link()', 'pports');
    END;
    -- RAISE INFO 'port = %:%', node_id2name(port.node_id), port.port_name;
    IF link_type = 'Front' THEN        -- Ha Front, akkor hátlapon megyünk tovább
        -- RAISE INFO 'next_phs_link() Front port: %:%(%)/%', port.node_id, port.port_name, port.port_id, link_type;
        IF port.shared_cable <> '' THEN             -- Ha van hátlapi kábel megosztás
            osh := min_shared(port.shared_cable, sh);-- A két share minimuma, van átjárás ?
            IF osh = 'NC' THEN  -- A hátlapi megosztás miatt ez rossz irány, nincs átjárás
                -- RAISE INFO 'RETURN: osh = NC';
                RETURN NULL;         -- NULL, nincs következő link
            END IF;
            -- A hátlapi link csak az egyik megosztásra van megadva, beolvassuk azt a portot
            IF port.shared_port_id IS NOT NULL THEN -- Az "igazi" port id-je, vagy NULL ha ez az "igazi"
                SELECT * INTO port FROM pports WHERE port_id = port.shared_port_id;
            END IF;
        END IF;
        BEGIN -- Hátlapi összeköttetés csak egy lehet, a share máshogy van lerendezve
            SELECT * INTO STRICT orec FROM phs_links
                WHERE port_id1 = port.port_id AND phs_link_type1 = 'Back';
            EXCEPTION
                WHEN NO_DATA_FOUND THEN -- Nincs következő link
                    -- RAISE INFO 'RETURN: NULL (break)';
                    RETURN NULL;
                WHEN TOO_MANY_ROWS THEN     -- Ez viszont hiba
                    PERFORM error('Ambiguous', -1, 'port_id1', 'next_phs_link()', 'phs_links');
        END;
        -- RAISE INFO 'RETURN osh = "%"; orec : id = % % -> %', osh, orec.phs_link_id, orec.port_id1, orec.port_id2;
        orec.port_shared := min_shared(orec.port_shared, osh);
        RETURN orec;    -- Megvan, és kész (az eredmény (Back) 2-es portján nem foglalkoztunk a share-val)
    ELSIF link_type = 'Back' THEN          -- Ha hátlap, akkor az előlap felé megyünk
        -- RAISE INFO 'next_phs_link() Back port: %:%(%)/%', port.node_id, port.port_name, port.port_id, link_type;
        -- hátlapi megosztás esetán, lehet, hogy nem ez az előlapi port
        IF port.shared_cable <> '' AND min_shared(port.shared_cable, sh) = 'NC' THEN
            -- RAISE INFO 'Shared cable, find port...';
            -- tényleg nem ez az, keressük meg
            FOR port IN SELECT * FROM pports WHERE port_id = port.shared_port_id LOOP
                EXIT WHEN min_shared(port.shared_cable, sh) <> 'NC';
            END LOOP;
            osh := min_shared(port.shared_cable, sh);
            IF osh = 'NC' THEN  -- Ha nem találtuk meg, akkor nincs link.
                -- RAISE INFO 'RETURN: osh IS NULL (shared cable, shared port not found)';
                RETURN NULL;
            END IF;
        END IF; -- Megvan az előlapi port
        -- A kábelmegosztások miatt megint keresni kell az igazi linket. Több is lehet az 'alacsonyabb' megosztás lessz az eredmény.
        FOR orec IN SELECT * FROM phs_links WHERE port_id1 = port.port_id AND phs_link_type1 = 'Front' ORDER BY port_shared ASC LOOP
            -- RAISE INFO 'for : orec.phs_link_id = %, orec.port_shared = %', orec.phs_link_id, orec.port_shared;
            EXIT WHEN min_shared(osh, orec.port_shared) <> 'NC';
        END LOOP;
        osh := min_shared(sh, orec.port_shared);
        IF orec IS NULL OR osh = 'NC' THEN
            -- RAISE INFO 'RETURN: NULL (break)';
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

COMMENT ON FUNCTION next_patch(bigint, phslinktype, portshare) IS $$
A következő link keresése. Feltételezi, hogy az első port_id paraméter egy patch portot azonosít.
A keresett link irányát a második paraméter határozza meg, ami az aktuális linkhez tartozik, vagyis
'front' esetén a hátlap irányába, 'back' esetén az előlap irányában. A visszaadott érték a talált phs_links rekord,
vagy NULL. A visszaadott rekordban a port_shared mező modosítva lessz, értéke a paraméterként megadott, és a beolvasott
megosztás eredője. Ha több lehetséges link rekord van, akkor a legalacsonyabb megosztás értéküt adja vissza a függvény
(ha van egy A és B, akkor az A-t),a többi rekordot a második paraméter modosításával kaphatjuk vissza.
$$;

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


SELECT set_db_version(1, 15);