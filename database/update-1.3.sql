BEGIN;  -- Version 1.2 ==> 1.3

-- nodes.sql

-- A tábla neve (vagy a mezők elnevezése) nem követte a szabályokat, és a program nem minden mezőt ismert fel
-- Az 'ip_address' bázis név -> 'ip_addresses' táblanév miatt a szabályokat is büvíteni kell:
-- nem egy 's' , hanem 'es' utótagot kapott 
ALTER TABLE ipaddresses RENAME TO ip_addresses;

CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
BEGIN
 -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím
    IF NEW.ip_address_type IS NULL THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) THEN
            NEW.ip_address_type := 'dynamic';
        ELSE
            NEW.ip_address_type := 'fixip';
        END IF;
    END IF;
    IF NEW.address IS NOT NULL THEN
        -- Nincs subnet (id), keresünk egyet
        IF NEW.subnet_id IS NULL AND NEW.ip_address_type <> 'external' THEN
            BEGIN
                SELECT subnet_id INTO STRICT NEW.subnet_id FROM subnets WHERE netaddr >> NEW.address;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN     -- nem találtunk
                        PERFORM error('NotFound', -1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                        PERFORM error('Ambiguous',-1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END;
            -- RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        -- Ha megadtuk a subnet id-t is, akkor a címnek benne kell lennie
        ELSIF NEW.ip_address_type <> 'external' THEN
            SELECT netaddr INTO n FROM subnets WHERE subnet_id = NEW.subnet_id;
            IF NOT FOUND THEN
                PERFORM error('InvRef', NEW.subnet_id, 'subnet_id', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
            IF NOT n >> NEW.address THEN
                PERFORM error('InvalidNAddr', NEW.subnet_id, CAST(n AS TEXT) || '>>' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        ELSE
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha van már ilyen fix vagy pseudo ip cím, az baj, de privat címneknél nincs ütközés
        IF NEW.address IS NOT NULL AND NEW.ip_address_type <> 'private' THEN
            -- Töröljük az azonos dinamikus címmeketm ha van
            IF NEW.ip_address_type = 'dynamic' THEN
                UPDATE ip_addresses SET address = NULL WHERE address = NEW.address AND ip_address_type = 'dynamic' AND ip_address_id <> NEW.ip_address_id;
            -- minden egyébb ütközés hiba
            ELSIF 0 < COUNT(*) FROM ip_addresses WHERE
                         ( ip_address_type = 'fixip' OR ip_address_type = 'pseudo') AND
                         address = NEW.address AND ip_address_id <> NEW.ip_address_id THEN
                PERFORM error('IdNotUni', n, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        END IF;
        -- Ha a preferred nincs megadva, akkor az elsőnek megadott cím a preferált
        IF NEW.preferred IS NULL THEN
            SELECT 1 + COUNT(*) INTO NEW.preferred FROM interfaces JOIN ip_addresses USING(port_id) WHERE port_id = NEW.port_id AND preferred IS NOT NULL AND address IS NOT NULL;
        END IF;
    ELSE
        -- Cím ként a NULL csak a dynamic típusnál megengedett
        IF NEW.ip_address_type <> 'dynamic' THEN
            PERFORM error('DataError', 0, 'NULL ip as non dynamic type', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
        END IF;
        -- RAISE INFO 'IP address is NULL';
    END IF;
    -- RAISE INFO 'Return, NEW = %', NEW;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;



-- LanView.sql

UPDATE fkey_types SET table_name = 'ip_addresses' WHERE table_name = 'ipaddresses';

-- mactab.sql

CREATE OR REPLACE VIEW arps_shape AS
    SELECT
        ipaddress,
        hwaddress,
        set_type,
        host_service_id,
        first_time,
        last_time,
        arp_note,
        port_id2full_name(ip.port_id) AS port_by_ipa,
        first_node_id2name(array_agg(ih.node_id)) AS node_by_hwa,
        array_agg(port_id2name(ih.port_id)) AS ports_by_hwa
    FROM arps AS a
    LEFT OUTER JOIN interfaces  AS ih USING(hwaddress) -- many
    LEFT OUTER JOIN ip_addresses AS ip ON ip.address = a.ipaddress
    GROUP BY ipaddress, hwaddress, set_type, host_service_id, first_time, last_time, arp_note, ip.port_id
;

CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    aid     bigint;
    oip     ip_addresses;
    net     cidr;
    sni     bigint;
BEGIN
    BEGIN       -- MAC -> IP check
        SELECT ip_address_id INTO STRICT aid FROM ip_addresses JOIN interfaces USING(port_id) WHERE
             ip_address_type = 'dynamic' AND hwaddress = hwa AND (address <> ipa OR address IS NULL);
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                aid := NULL;
            WHEN TOO_MANY_ROWS THEN
                PERFORM error('DataWarn', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses JOIN interfaces');
                aid := NULL;
    END;
    IF aid IS NOT NULL THEN
        SELECT * INTO oip FROM ip_addresses WHERE ip_address_id = aid;     -- régi IP
        SELECT netaddr INTO net FROM subnets WHERE subnet_id = oip.subnet_id;
        IF net >> ipa THEN      -- Ha nem változott a subnet
            sni := oip.subnet_id;
        ELSE
            sni := NULL;
        END IF;
        UPDATE ip_addresses SET address = ipa, subnet_id = sni WHERE ip_address_id = aid;   -- új ip
        INSERT INTO dyn_ipaddress_logs(ipaddress_new, ipaddress_old, set_type, port_id)
                                VALUES (ipa, oip.address, stp, oip.port_id);
    END IF;
    SELECT * INTO arp FROM arps WHERE ipaddress = ipa;
    IF NOT FOUND THEN
        INSERT INTO arps(ipaddress, hwaddress,set_type, host_service_id) VALUES (ipa, hwa, stp, hsi);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = hwa THEN
	    IF arp.set_type < stp THEN
	        UPDATE arps SET set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP WHERE ipaddress = arp.ipaddress;
		RETURN 'update';
	    ELSE
	        UPDATE arps SET last_time = CURRENT_TIMESTAMP WHERE ipaddress = arp.ipaddress;
		RETURN 'found';
	    END IF;
        ELSE
            UPDATE arps
                SET hwaddress = hwa,  first_time = CURRENT_TIMESTAMP, set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP
                WHERE ipaddress = arp.ipaddress;
            INSERT INTO
                arp_logs(reason, ipaddress, hwaddress_new, hwaddress_old, set_type_old, host_service_id_old, first_time_old, last_time_old)
                VALUES( 'move',  ipa,       hwa,           arp.hwaddress, arp.set_type, arp.host_service_id, arp.first_time, arp.last_time);
            RETURN 'modify';
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE VIEW mactab_shape AS
    SELECT
        n.node_id               AS node_id,
        n.node_name             AS node_name,
        t.port_id               AS port_id,
        p.port_name             AS port_name,
        t.hwaddress             AS hwaddress,
        t.mactab_state          AS mactab_state,
        t.first_time            AS first_time,
        t.last_time             AS last_time,
        t.state_updated_time    AS state_updated_time,
        t.set_type              AS set_type,
        r.node_name             AS r_node_name,
        port_id2name(i.port_id) AS r_port_name,
        -- A rendszer nem kezeli a cím tömboket
        ARRAY(SELECT ipaddress::text FROM arps         WHERE arps.hwaddress = t.hwaddress)    AS ipaddrs_by_arp,
        ARRAY(SELECT address::text   FROM ip_addresses WHERE ip_addresses.port_id = i.port_id) AS ipaddrs_by_rif
    FROM mactab AS t
      JOIN nports AS p USING (port_id)
      JOIN nodes  AS n USING (node_id)
      LEFT OUTER JOIN interfaces AS i USING(hwaddress)
      LEFT OUTER JOIN nodes      AS r ON r.node_id = i.node_id
      ;
      
CREATE OR REPLACE VIEW arps_shape AS
    SELECT
        ipaddress,
        hwaddress,
        set_type,
        host_service_id,
        first_time,
        last_time,
        arp_note,
        port_id2full_name(ip.port_id) AS port_by_ipa,
        first_node_id2name(array_agg(ih.node_id)) AS node_by_hwa,
        array_agg(port_id2name(ih.port_id)) AS ports_by_hwa
    FROM arps AS a
    LEFT OUTER JOIN interfaces   AS ih USING(hwaddress) -- many
    LEFT OUTER JOIN ip_addresses AS ip ON ip.address = a.ipaddress
    GROUP BY ipaddress, hwaddress, set_type, host_service_id, first_time, last_time, arp_note, ip.port_id
;

-- SET database version: 1.3
SELECT set_db_version(1, 3);
END;