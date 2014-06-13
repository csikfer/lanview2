-- //// mactab

CREATE TABLE ouis (
    oui		macaddr PRIMARY KEY,
    oui_name	varchar(32)	NOT NULL,
    oui_note	varchar(255)	DEFAULT NULL
);
ALTER TABLE ouis OWNER TO lanview2;

CREATE TYPE mactabstate AS ENUM ('likely', 'noarp', 'nooid', 'suspect');
ALTER TYPE mactabstate OWNER TO lanview2;
COMMENT ON TYPE mactabstate IS
'A cím információ minösítése:
likely	Hihető, van ilyen című bejegyzett eszköz.
noarp	Nem szerepel az arps táblában
nooid	Nem azonosítható a cím alapján a gyártó (OUI)
suspect	Gyanús, az észleléskor hibákat jelzett a port
';

CREATE TABLE mactab (
    hwaddress       macaddr PRIMARY KEY,
    port_id         bigint NOT NULL REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state    mactabstate[] DEFAULT '{}',
    first_time      timestamp   DEFAULT CURRENT_TIMESTAMP,
    last_time       timestamp   DEFAULT CURRENT_TIMESTAMP,
    set_type        settype NOT NULL DEFAULT 'manual'
);
ALTER TABLE mactab OWNER TO lanview2;

CREATE TABLE mactab_logs (
    mactab_log_id   bigserial      PRIMARY KEY,
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
CREATE INDEX hwaddress_index ON mactab_logs(hwaddress);
CREATE INDEX date_of_index   ON mactab_logs(date_of);
ALTER TABLE mactab_logs OWNER TO lanview2;

INSERT INTO sys_params
    (sys_param_name,                param_type_id,                 param_value,    sys_param_note) VALUES
    ('mac_tab_move_check_interval', param_type_name2id('interval'),'01:00:00',     'Hibás topológia miatt mozgó MAC lokációk észlelési időintervalluma'),
    ('mac_tab_move_check_count',    param_type_name2id('bigint'),  '6',            'Hibás topológia miatt mozgó MAC lokációk észlelési határértéke (látszólagos mozgások száma).');
INSERT INTO param_types
    (param_type_name,    param_type_type, param_type_note)    VALUES
    ('suspected_uplink', 'boolean',      'Feltételezhetően egy uplink, a portnak a mactab táblába való felvétele tiltott.');

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
    UPDATE mactab SET port_id = pid, mactab_state = mst, first_time = CURRENT_TIMESTAMP, last_time = CURRENT_TIMESTAMP
            WHERE hwaddress = mac;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION insert_or_update_mactab(
    pid bigint,
    mac macaddr,        
    typ settype DEFAULT 'query',
    mst mactabstate[] DEFAULT '{}'
) RETURNS reasons AS $$
DECLARE
    mt          mactab;
    pname_di CONSTANT varchar(32) := 'suspected_uplink';
    maxct    CONSTANT bigint     := get_int_sys_param('mac_tab_move_check_count');
    mv       CONSTANT reasons     := 'move';
    btm      CONSTANT timestamp   := CURRENT_TIMESTAMP - get_interval_sys_param('mac_tab_move_check_interval');
BEGIN
    IF get_bool_port_param(pid, pname_di) THEN
        RETURN 'discard';
    END IF;
    SELECT * INTO mt FROM mactab WHERE hwaddress = mac;
    IF NOT FOUND THEN
        INSERT INTO mactab(hwaddress, port_id, mactab_state,set_type) VALUES (mac, pid, mst, typ);
        RETURN 'insert';
    ELSE
        IF mt.port_id = pid THEN
            UPDATE mactab SET last_time = CURRENT_TIMESTAMP WHERE hwaddress = mac;
            RETURN 'unchange';
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
                    RETURN 'update';
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
  mst   Az összerendelés jellemzői (megbízhatóság) alapértelmezetten egy üres tömb.
Visszaadott érték:
  Ha a pid-del azonosított portnak van egy "suspected_uplink" típusú paramétere, és ennek értéke true, akkor a fögvény nem csinál semmit, és a "discard" értékkel tér vissza.
  Ha az összerendelés létezik, akkor csak a last_time mező értékét aktualizálja, és "unchanged" értékkel tér vissza.
  Ha az összerendlés még nem létezik, akkor beszőrja a mactab rekordot, és az "insert" értékkel tér vissza.
  Ha a MAC egy másik porthoz volt hozzárandelve, akkor megvizsgálja, hogy a megadott MAC cím hányszor jelent meg más-más porton egy megadott időn bellül.
  A váltások maximális megengedett számát a "mac_tab_move_check_count" nevű és egész típusú, a vizsgállt időintervallumot pedig a "mac_tab_move_check_interval" intervallum
  típusú rendszer paraméter tartalmazza. Ha a váltások száma nem nagyobb mint a megengedett, akkor módosítja a mactab rekordot, és a "move" értékkel tér vissza.
  Ellenkező esetben megvizsgálja, hogy a régi vagy az új MAC-hoz rendelt porton volt-e több változás a viszgállt időintervallumban.
  Amelyiken több változás volt, ahhoz a porthoz hozzárendeli a "suspected_uplink" nevű és true értékű port paraméteret. Ha ez az új port, akkor "modify" értékkel tér vissza.
  Ha a régi, akkor módosítj a mactab rekordot az új port ID-vel, és "update" értékkel tér vissza. A viszgállt időintervallumban a feltételezhetően tévesen keletkezett
  mactab_logs rekordokban true-ra állítja a be_void mezőt.
  Ha a régebbi porton be van állítva a "suspected_uplink" nevű paraméter és értéke true, az egyenértékkű azzal az esettel, mintha azon lett volna több változás.
';

CREATE TABLE arps (
    ipaddress       inet        PRIMARY KEY,
    hwaddress       macaddr     NOT NULL,
    first_time      timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP, -- First time discovered
    last_time       timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP  -- Last time discovered
);
ALTER TABLE arps OWNER TO lanview2;

CREATE TABLE arp_logs (
    arp_log_id      bigserial      PRIMARY KEY,
    reason          reasons     NOT NULL,
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    ipaddress_new   inet        DEFAULT NULL,
    hwaddress_new   macaddr     DEFAULT NULL,
    ipaddress_old   inet        DEFAULT NULL,
    hwaddress_old   macaddr     DEFAULT NULL,
    first_time_old  timestamp   DEFAULT NULL,
    last_time_old   timestamp   DEFAULT NULL
);
ALTER TABLE arp_logs OWNER TO lanview2;

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
                arp_logs(reason, ipaddress_new, hwaddress_new, ipaddress_old, hwaddress_old, first_time_old, last_time_old)
                VALUES( 'move',  $1,            $2,            arp.ipaddress, arp.hwaddress, arp.first_time, arp.last_time);
            RETURN 'update';
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;
