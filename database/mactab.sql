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

CREATE TABLE arps (
    ipaddress       inet        PRIMARY KEY,
    hwaddress       macaddr     NOT NULL,
    first_time      timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP, -- First time discovered
    last_time       timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP  -- Last time discovered
);
ALTER TABLE arps OWNER TO lanview2;

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
            RETURN 2;
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;
