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
    port_id         integer NOT NULL REFERENCES interfaces(port_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state    mactabstate[] DEFAULT '{}',
    first_time      timestamp   DEFAULT CURRENT_TIMESTAMP,
    last_time       timestamp   DEFAULT CURRENT_TIMESTAMP,
    set_type        settype NOT NULL DEFAULT 'manual'
);
ALTER TABLE mactab OWNER TO lanview2;

CREATE TABLE mactab_logs (
    mactab_log_id   serial      PRIMARY KEY,
    hwaddress       macaddr     NOT NULL,
    reason          reasons     NOT NULL,
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    port_id_old     integer DEFAULT NULL REFERENCES interfaces(port_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state_old mactabstate[] DEFAULT NULL,
    first_time_old  timestamp   DEFAULT NULL,
    last_time_old   timestamp   DEFAULT NULL,
    set_type_old    settype NOT NULL DEFAULT 'manual',
    port_id_new     integer DEFAULT NULL REFERENCES interfaces(port_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    mactab_state_new mactabstate[] DEFAULT NULL,
    set_type_new    settype NOT NULL DEFAULT 'manual'
);
CREATE INDEX hwaddress_index ON mactab_logs(hwaddress);
ALTER TABLE mactab_log OWNER TO lanview2;

CREATE OR REPLACE FUNCTION insert_or_update_mactab(
    pid integer,
    mac macaddr,        
    typ set_type DEFAULT 'query',
    mst mactabstate[] DEFAULT '{}'
) RETURNS reasons AS $$
DECLARE
    mt          mactab;
    changed     integer;
BEGIN
    SELECT * INTO mt FROM mactab WHERE hwaddress = mac;
    IS NOT FOUND THEN
        INSERT INTO mactab(hwaddress, port_id, mactab_state,set_type) VALUES (mac, pid, mst, typ);
        RETURN 'new';
    ELSE
        IF mactab.port_id = pid THEN
            RETURN 'unchanged';
        ELSE
            SELECT COUNT(*) INTO changed FROM mactab_logs WHERE hwaddress = mac AND ;
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE TABLE arps (
    ipaddress       inet        PRIMARY KEY,
    hwaddress       macaddr     NOT NULL,
    first_time      timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP, -- First time discovered
    last_time       timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP  -- Last time discovered
);
ALTER TABLE arps OWNER TO lanview2;

CREATE TABLE arp_logs (
    arp_log_id      serial      PRIMARY KEY,
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

CREATE OR REPLACE FUNCTION insert_or_update_arp(inet, macaddr) RETURNS integer AS $$
DECLARE
    arp arps;
BEGIN
    SELECT * INTO arp FROM arps WHERE ipaddress = $1;
    IF NOT FOUND THEN
        INSERT INTO arps(ipaddress, hwaddress) VALUES ($1, $2);
        RETURN 1;
    ELSE
        IF arp.hwaddress = $2 THEN
            UPDATE arps SET last_time = CURRENT_TIMESTAMP WHERE ipaddress = arp.ipaddress;
            RETURN 0;
        ELSE
            UPDATE arps
                SET hwaddress = $2,  first_time = CURRENT_TIMESTAMP, last_time = CURRENT_TIMESTAMP
                WHERE ipaddress = arp.ipaddress;
            INSERT arp_logs
                INTO arp_logs(reason, ipaddress_new, hwaddress_new, ipaddress_old, hwaddress_old, first_time_old, last_time_old)
                VALUES(      'moved', $1,            $2,            arp.ipaddress, arp.hwaddress, arp.first_time, arp.last_time);
            RETURN 2;
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;
