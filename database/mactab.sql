-- //// OUI

-- Rendszer paraméter a insert_or_update_mactab(pid bigint, mac macaddr, typ settype, mst mactabstate[]) függvényhez:
INSERT INTO sys_params
    (sys_param_name,                    param_type_id,                 param_value,    sys_param_note) VALUES
    ('arps_expire_interval',            param_type_name2id('interval'),'21 days',      'Az arps táblában ennyi idő mulva törlődik egy rekord, ha nem frissül a last_time mező'),
    ('mactab_move_check_interval',      param_type_name2id('interval'),'01:00:00',     'Hibás topológia miatt mozgó MAC lokációk észlelési időintervalluma'),
    ('mactab_move_check_count',         param_type_name2id('bigint'),  '6',            'Hibás topológia miatt mozgó MAC lokációk észlelési határértéke (látszólagos mozgások száma).'),
    ('mactab_check_stat_interval',      param_type_name2id('interval'),'00:30:00',     'Ennél régebben frissítet mactab rekordoknál újra számolandó a mactab_state mező.'),
    ('mactab_reliable_expire_interval', param_type_name2id('interval'),'60 days',      'Az mactab táblában ennyi idő mulva törlődik egy rekord, ha nem frissül a last_time mező, és vannak az adat megbízhatóságára utaló fleg-ek'),
    ('mactab_expire_interval',          param_type_name2id('interval'),'14 days',      'Az mactab táblában ennyi idő mulva törlődik egy rekord, ha nem frissül a last_time mező, és nincsenek az adat megbízhatóságára utaló fleg-ek'),
    ('mactab_suspect_expire_interval',  param_type_name2id('interval'),'1 day',        'Az mactab táblában ennyi idő mulva törlődik egy rekord, ha nem frissül a last_time mező, és csak a suspect flag van beállítva');
-- Port paraméter a insert_or_update_mactab(pid bigint, mac macaddr, typ settype, mst mactabstate[]) függvényhez:
INSERT INTO param_types
    (param_type_name,    param_type_type, param_type_note)    VALUES
    ('suspected_uplink', 'boolean',      'Port paraméter: Feltételezhetően egy uplink, a portnak a mactab táblába való felvétele tiltott.');


CREATE TABLE ouis (
    oui		macaddr PRIMARY KEY,
    oui_name	varchar(32)	NOT NULL,
    oui_note	varchar(255)	DEFAULT NULL
);
ALTER TABLE ouis OWNER TO lanview2;
COMMENT ON TABLE ouis IS 'Organizational Unique Identifier';

CREATE OR REPLACE FUNCTION is_content_oui(mac macaddr) RETURNS boolean AS $$
BEGIN
    RETURN 0 <> COUNT(*) FROM ouis WHERE oui = trunc(mac);
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION is_content_oui(mac macaddr) IS
'Megvizsgálja, hogy a paraméterként megadott MAC első 3 byte-ja által azonosítptt OUI rekord létezik-e.
Ha igen true-val, ellenkező esetben false-val tér vissza.';

-- //// ARP

CREATE TABLE arps (
    ipaddress       inet        PRIMARY KEY,
    hwaddress       macaddr     NOT NULL,
    first_time      timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP, -- First time discovered
    last_time       timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP  -- Last time discovered
);
CREATE INDEX arps_hwaddress_index   ON arps(hwaddress);
CREATE INDEX arps_first_time_index  ON arps(first_time);
CREATE INDEX arps_last_time_index   ON arps(last_time);
ALTER TABLE arps OWNER TO lanview2;

COMMENT ON TABLE arps IS
'Az ARP tábla (vagy DHCP konfig) lekérdezések eredményét tartalmazó tábla.
Az adatmanipulációs műveleteket nem közvetlenül, hanem a kezelő függvényeken keresztül kell elvégezni,
hogy létrejöjjenek a log rekordok is: insert_or_update_arp(), arp_remove() és refresh_arps().';
COMMENT ON COLUMN arps.ipaddress IS 'A MAC-hoz detektált ip cím, egyedi kulcs.';
COMMENT ON COLUMN arps.hwaddress IS 'Az ethernet cím, nem egyedi kulcs.';
COMMENT ON COLUMN arps.first_time IS 'A rekord létrehozásának az ideje, ill. az első detektálás ideje.';
COMMENT ON COLUMN arps.last_time IS 'Az legutóbbi detektálás ideje.';

CREATE TABLE arp_logs (
    arp_log_id      bigserial   PRIMARY KEY,
    reason          reasons     NOT NULL,
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    ipaddress       inet        DEFAULT NULL,
    hwaddress_new   macaddr     DEFAULT NULL,
    hwaddress_old   macaddr     DEFAULT NULL,
    first_time_old  timestamp   DEFAULT NULL,
    last_time_old   timestamp   DEFAULT NULL
);
CREATE INDEX arp_logs_date_of_index   ON arp_logs(date_of);
ALTER TABLE arp_logs OWNER TO lanview2;
COMMENT ON TABLE arp_logs IS 'Az arps tábla változásainag a napló táblája.';

CREATE OR REPLACE FUNCTION insert_or_update_arp(inet, macaddr) RETURNS reasons AS $$
DECLARE
    arp arps;
BEGIN
    SELECT * INTO arp FROM arps WHERE ipaddress = $1;
    IF NOT FOUND THEN
        INSERT INTO arps(ipaddress, hwaddress) VALUES ($1, $2);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = $2 THEN
            UPDATE arps SET last_time = CURRENT_TIMESTAMP WHERE ipaddress = arp.ipaddress;
            RETURN 'found';
        ELSE
            UPDATE arps
                SET hwaddress = $2,  first_time = CURRENT_TIMESTAMP, last_time = CURRENT_TIMESTAMP
                WHERE ipaddress = arp.ipaddress;
            INSERT INTO
                arp_logs(reason, ipaddress, hwaddress_new, hwaddress_old, first_time_old, last_time_old)
                VALUES( 'move',  $1,        $2,            arp.hwaddress, arp.first_time, arp.last_time);
            RETURN 'update';
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION insert_or_update_arp(inet, macaddr) IS
'A detektált MAC - IP cím pár alapján modosítja az arps táblát, és kezeli a napló táblát is
Paraméterek:
    $1      IP cím
    $2      MAC
Visszatérési érték:
    Ha létrejött egy új rekord, akkor "insert".
    Ha nincs változás (csak a last_time frissül), akkor "found".
    Ha az IP cím egy másik MAC-hez lett rendelve, akkor "update".';

CREATE OR REPLACE FUNCTION arp_remove(
    a  arps,
    re reasons DEFAULT 'remove'
) RETURNS void AS $$
BEGIN
    INSERT INTO
        arp_logs(reason, ipaddress,  hwaddress_old, first_time_old, last_time_old)
        VALUES(  re,     a.ipaddress,a.hwaddress,   a.first_time,   a.last_time);
    DELETE FROM arps WHERE ipaddress = a.ipaddress;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION arp_remove(a  arps,re reasons) IS
'Törli a megadott rekordot az arps táblából.
Paraméterek
    a   A törlendő rekord.
    re  A log rekordba irandó ok, alapértelmezetten ez a "remove".
Nincs visszatérési érték.';


CREATE OR REPLACE FUNCTION refresh_arps() RETURNS integer AS $$
DECLARE
    a  arps;
    ret integer := 0;
BEGIN
    FOR a IN SELECT * FROM arps
        WHERE  last_time < (CURRENT_TIMESTAMP - get_interval_sys_param('arps_expire_interval'))
    LOOP
        PERFORM arp_remove(a, 'expired');
        ret := ret +1;
    END LOOP;
    RETURN ret;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION refresh_arps() IS
'Törli azokat a rekordokat az arps táblából, melyeknél a last_time értéke túl régi.
A lejárati időintervallumot a "arps_expire_interval" rendszerváltozó tartalmazza.
Visszatérési érték a törölt rekordok száma. A törlés oka "expired"lessz.';

CREATE OR REPLACE FUNCTION is_content_arp(mac macaddr) RETURNS boolean AS $$
BEGIN
    RETURN 0 <> COUNT(*) FROM arps WHERE hwaddress = mac;
END;
$$ LANGUAGE plpgsql;


-- //// mactab

CREATE TYPE mactabstate AS ENUM ('likely', 'arp', 'oid', 'port', 'suspect');
ALTER TYPE mactabstate OWNER TO lanview2;
COMMENT ON TYPE mactabstate IS
'A cím információ minösítése:
likely	Hihető, van ilyen című bejegyzett eszköz.
arp	Szerepel az arps táblában
oid	Azonosítható a cím alapján a gyártó (OUI)
suspect	Gyanús, az észleléskor hibákat jelzett a port
';

CREATE TABLE mactab (
    hwaddress       macaddr PRIMARY KEY,
    port_id         bigint NOT NULL REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state    mactabstate[]   DEFAULT '{}',
    first_time      timestamp       DEFAULT CURRENT_TIMESTAMP,
    last_time       timestamp       DEFAULT CURRENT_TIMESTAMP,
    state_updated_time timestamp    DEFAULT CURRENT_TIMESTAMP,
    set_type        settype NOT NULL DEFAULT 'manual'
);
CREATE INDEX mactab_first_time_index            ON mactab(first_time);
CREATE INDEX mactab_last_time_index             ON mactab(last_time);
CREATE INDEX mactab_state_updated_time_index    ON mactab(state_updated_time);
ALTER TABLE mactab OWNER TO lanview2;
COMMENT ON TABLE mactab IS 'Port címtábla lekérdezések eredményét tartalmazó tábla. Az adatmanipulációs műveletek a függvényeket keresztűl lehet elvégezni : insert_or_update_mactab()';

CREATE TABLE mactab_logs (
    mactab_log_id   bigserial   PRIMARY KEY,
    hwaddress       macaddr     NOT NULL,
    reason          reasons     NOT NULL,
    be_void         boolean     NOT NULL DEFAULT false,
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    port_id_old     bigint DEFAULT NULL REFERENCES interfaces(port_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state_old mactabstate[] DEFAULT NULL,
    first_time_old  timestamp   NOT NULL,
    last_time_old   timestamp   NOT NULL,
    set_type_old    settype     NOT NULL,
    port_id_new     bigint DEFAULT NULL REFERENCES interfaces(port_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state_new mactabstate[] NOT NULL,
    set_type_new    settype     NOT NULL
);
CREATE INDEX mactab_logs_hwaddress_index ON mactab_logs(hwaddress);
CREATE INDEX mactab_logs_date_of_index   ON mactab_logs(date_of);
ALTER TABLE mactab_logs OWNER TO lanview2;
COMMENT ON TABLE  mactab_logs IS 'Port cím tábla rekordok változása';
COMMENT ON COLUMN mactab_logs.hwaddress IS 'A változáshoz tartozó MAC';
COMMENT ON COLUMN mactab_logs.reason IS
'A változás típusa:
    move        A MAC egy másik port címtáblájában jelent meg
    modify      Állpotváltozás
    remove      A bejegyzés törölve lett.
    expired     A MAC nem jelent meg egyik címtáblában sem egy megadott ideje, ezért törölve lett.';
COMMENT ON COLUMN mactab_logs.be_void IS 'Nem releváns log rekord, nem valós mozgás eredménye.';
COMMENT ON COLUMN mactab_logs.date_of IS 'A bejegyzés időpontja';
COMMENT ON COLUMN mactab_logs.port_id_old IS 'Az eredeti portmac bejegyzéshez tartozó port_id érték, vagy NULL';

CREATE OR REPLACE FUNCTION is_linked(
    pid bigint,
    mac macaddr
) RETURNS boolean AS $$
DECLARE
    n integer;
BEGIN
    SELECT COUNT(*) INTO n FROM log_links JOIN interfaces ON log_links.port_id2 = interfaces.port_id WHERE log_links.port_id1 = pid AND interfaces.hwaddress = mac;
    CASE n
    WHEN 1 THEN
        RETURN true;
    WHEN 0 THEN
        RETURN false;
    ELSE
        PERFORM error('DataError', n, 'MAC is not unique', 'is_linked(bigint, macaddr)', 'interfaces, log_links');
    END CASE;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION is_linked(pid bigint, mac macaddr) IS '
Megvizsgálja, hogy a port címtábla lekérdezés eredményeként kapott MAC címnek megfelelő log_links táblabejegyzés is létezik-e.
Paraméterek:
    pid A port id, melynek a címtáblájából a MAC származik.
    mac A címtáblából kiolvasott MAC cím.
Visszaadott érték:
    Ha megtalálta a port_id és MAC párossal egyenértékű rekordot, akkor true, ha nem akkor false.
    Ha több rekord is megfeleltethető, ami elvileg lehetetlen, akkor dob egy kizárást.
';

CREATE OR REPLACE FUNCTION mactab_move(
    mt  mactab,
    pid bigint,
    mac macaddr,        
    typ settype,
    mst mactabstate[]
) RETURNS void AS $$
BEGIN
    INSERT INTO mactab_logs(hwaddress, reason, port_id_old, mactab_state_old, first_time_old, last_time_old, set_type_old, port_id_new, mactab_state_new, set_type_new)
           VALUES         (mac,       'move', mt.port_id,  mt.mactab_state,  mt.first_time,  mt.last_time,  mt.set_type,  pid,         mst,              typ);
    UPDATE mactab SET port_id = pid, mactab_state = mst, first_time = CURRENT_TIMESTAMP, last_time = CURRENT_TIMESTAMP, state_updated_time = CURRENT_TIMESTAMP
            WHERE hwaddress = mac;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION mactab_changestat(
    mt  mactab,
    mst mactabstate[],
    typ settype DEFAULT NULL,
    rt boolean DEFAULT false
) RETURNS reasons AS $$
BEGIN
    IF typ IS NULL THEN
        typ := mt.set_type;
    END IF;
    IF mst <@ mt.mactab_state AND mst @> mt.mactab_state AND typ = mt.set_type THEN
        IF rt THEN
            UPDATE mactab SET last_time = CURRENT_TIMESTAMP, state_updated_time = CURRENT_TIMESTAMP WHERE hwaddress = mt.hwaddress;
        ELSE
            UPDATE mactab SET state_updated_time = CURRENT_TIMESTAMP WHERE hwaddress = mt.hwaddress;
        END IF;
        RETURN 'unchange';
    END IF;
    INSERT INTO mactab_logs(hwaddress, reason, port_id_old, mactab_state_old, first_time_old, last_time_old, set_type_old, port_id_new, mactab_state_new, set_type_new)
           VALUES       (mt.hwaddress,'modify',mt.port_id,  mt.mactab_state,  mt.first_time,  mt.last_time,  mt.set_type,  mt.port_id,  mst,              typ);
    IF rt THEN
        UPDATE mactab SET mactab_state = mst, last_time = CURRENT_TIMESTAMP, state_updated_time = CURRENT_TIMESTAMP  WHERE hwaddress = mt.hwaddress;
    ELSE
        UPDATE mactab SET mactab_state = mst, state_updated_time = CURRENT_TIMESTAMP WHERE hwaddress = mt.hwaddress;
    END IF;
    RETURN 'update';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION mactab_remove(
    mt  mactab,
    re reasons DEFAULT 'remove'
) RETURNS void AS $$
BEGIN
    INSERT INTO mactab_logs(hwaddress, reason, port_id_old, mactab_state_old, first_time_old, last_time_old, set_type_old)
           VALUES       (mt.hwaddress, re,     mt.port_id,  mt.mactab_state,  mt.first_time,  mt.last_time,  mt.set_type);
    DELETE FROM mactab WHERE hwaddress = mt.hwaddress;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION current_mactab_stat(
    pid bigint,
    mac macaddr,        
    mst mactabstate[] DEFAULT '{}'
) RETURNS mactabstate[] AS $$
DECLARE
    ret mactabstate[] := '{}';
BEGIN
    IF mst && ARRAY['suspect'::mactabstate] THEN
        ret := ARRAY['suspect'::mactabstate];
    END IF;
    IF is_content_oui(mac) THEN
        ret = append_array(ret, 'oui::mactabstate');
    END IF;
    IF is_linked(pid, mac) THEN
        ret := append_array(ret, 'likely::mactabstate');
    END IF;
    IF is_content_arp(mac) THEN
        ret := append_array(ret, 'arp'::mactabstate);
    END IF;
    RETURN ret;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION insert_or_update_mactab(
    pid bigint,
    mac macaddr,        
    typ settype DEFAULT 'query',
    mst mactabstate[] DEFAULT '{}'
) RETURNS reasons AS $$
DECLARE
    mt       mactab;
    pname_di CONSTANT varchar(32) := 'suspected_uplink';
    maxct    CONSTANT bigint      := get_int_sys_param('mactab_move_check_count');
    mv       CONSTANT reasons     := 'move';
    btm      CONSTANT timestamp   := CURRENT_TIMESTAMP - get_interval_sys_param('mactab_move_check_interval');
BEGIN
    IF get_bool_port_param(pid, pname_di) THEN
        RETURN 'discard';
    END IF;
    mst := current_mactab_stat(mst, pid, mac);
    SELECT * INTO mt FROM mactab WHERE hwaddress = mac;
    IF NOT FOUND THEN
        INSERT INTO mactab(hwaddress, port_id, mactab_state,set_type) VALUES (mac, pid, mst, typ);
        RETURN 'insert';
    ELSE
        IF mt.port_id = pid THEN
            RETURN mactab_changestat(mt, mst, typ, true);
        ELSE
            IF maxct < COUNT(*) FROM mactab_logs WHERE date_of > btm AND hwaddress = mac AND reason = mv THEN
                -- Csiki-csuki van, az egyik port valójában uplink?
                IF get_bool_port_param(mt.port_id, pname_di)
                 OR ((SELECT COUNT(*) FROM mactab_logs WHERE date_of > btm AND port_id = pid        AND reason = mv)
                   < (SELECT COUNT(*) FROM mactab_logs WHERE date_of > btm AND port_id = mt.port_id AND reason = mv))
                THEN    -- Nem az új pid a gyanús
                    PERFORM mactab_move(mt, pid, mac, typ, mst);
                    PERFORM set_bool_port_param(mt.port_id, true, pname);
                    UPDATE mactab_logs SET be_void = true WHERE date_of > btm AND port_id = mt.port_id AND reason = mv AND hwaddress = mac;
                    RETURN 'restore';
                ELSE
                    PERFORM set_bool_port_param(pid, true, pname);
                    UPDATE mactab_logs SET be_void = true WHERE date_of > btm AND port_id = pid        AND reason = mv AND hwaddress = mac;
                    RETURN 'modify';
                END IF;
            ELSE
                -- Egy másik porton jelent meg a MAC
                PERFORM mactab_move(mt, pid, mac, typ, mst);
                RETURN mv;
            END IF;
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION insert_or_update_mactab(pid bigint, mac macaddr, typ settype, mst mactabstate[]) IS '
Egy (switch) port és mac (cím tábla) összerendelés létrehozása, vagy módosítása.
Paraméterek:
  pid   A (switch) port ID.
  mac   A hozzárandelendő MAC cím (a port címtáblálábol kiolvasott érték).
  typ   Az összerendelés típusa, alapértelmezett a "query".
  mst   Az összerendelés jellemzői (megbízhatóság) alapértelmezetten egy üres tömb. Csak egy elem adható meg, a suspect, a többi jelző beállítása automatikus.
Visszaadott érték:
  Ha a pid-del azonosított portnak van egy "suspected_uplink" típusú paramétere, és ennek értéke true, akkor a függvény nem csinál semmit, és a "discard" értékkel tér vissza.
  Ha az összerendelés létezik, akkor csak a last_time mező értékét aktualizálja, és "unchange" értékkel tér vissza, ill. ha az állapot változott, akkor az "update" értékkel.
  Ha az összerendlés még nem létezik, akkor beszúrja a mactab rekordot, és az "insert" értékkel tér vissza.
  Ha a MAC egy másik porthoz volt hozzárandelve, akkor megvizsgálja, hogy a megadott MAC cím hányszor jelent meg más-más porton egy megadott időn bellül.
  A váltások maximális megengedett számát a "mactab_move_check_count" nevű és egész típusú, a vizsgállt időintervallumot pedig a "mactab_move_check_interval" intervallum
  típusú rendszer paraméter tartalmazza. Ha a váltások száma nem nagyobb mint a megengedett, akkor módosítja a mactab rekordot, és a "move" értékkel tér vissza.
  Ellenkező esetben megvizsgálja, hogy a régi vagy az új MAC-hoz rendelt porton volt-e több változás a viszgállt időintervallumban.
  Amelyiken több változás volt, ahhoz a porthoz hozzárendeli a "suspected_uplink" nevű és true értékű port paraméteret. Ha ez az új port, akkor "modify" értékkel tér vissza.
  Ha a régi, akkor módosítj a mactab rekordot az új port ID-vel, és "restire" értékkel tér vissza. A viszgállt időintervallumban a feltételezhetően tévesen keletkezett
  mactab_logs rekordokban true-ra állítja a be_void mezőt.
  Ha a régebbi porton be van állítva a "suspected_uplink" nevű paraméter és értéke true, az egyenértékkű azzal az esettel, mintha azon lett volna több változás.
';

CREATE OR REPLACE FUNCTION refresh_mactab() RETURNS integer AS $$
DECLARE
    mt  mactab;
    ret integer := 0;
BEGIN
    FOR mt IN SELECT * FROM mactab
        WHERE  state_updated_time (CURRENT_TIMESTAMP - get_interval_sys_param('mactab_check_stat_interval'))
    LOOP
        IF 'update' = mactab_changestat(mt, current_mactab_stat(mt.port_id, hwaddress, mt.mactab_state)) THEN
            ret := ret + 1;
        END IF;
    END LOOP;
    FOR mt IN SELECT * FROM mactab
        WHERE ( last_time < (CURRENT_TIMESTAMP - get_interval_sys_param('mactab_suspect_expire_interval'))  AND mactab_state && ARRAY['suspect']::mactabstate[] )
           OR ( last_time < (CURRENT_TIMESTAMP - get_interval_sys_param('mactab_reliable_expire_interval')) AND mactab_state && ARRAY['likely', 'arp', 'oid']::mactabstate[] )
           OR ( last_time < (CURRENT_TIMESTAMP - get_interval_sys_param('mactab_expire_interval'))          AND mactab_state =  ARRAY[]::mactabstate[] )
    LOOP
        PERFORM mactab_remove(mt, 'expired');
        ret := ret +1;
    END LOOP;
    RETURN ret;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION refresh_mactab() IS
'Frissíti a mactab tábla rekordokban a mactab_state mezőket.
Törli azokat a rekordokat a mactab táblából, melyeknál a last_time értéke túl régi.
A lejárati időintervallumokat a "mactab_suspect_expire_interval", "mactab_reliable_expire_interval" és "mactab_expire_interval" rendszerváltozó tartalmazza.
Azt, hogy melyik lejárati idő érvényes, azt a rekord mactab_state mező értéke határozza meg.
Visszatérési érték a törölt rekordok száma. A törlés oka "expired"lessz.';
