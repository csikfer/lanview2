-- 2023.01.17.
-- Hely leltár kódok 
ALTER TABLE places ADD COLUMN IF NOT EXISTS inventory_location text DEFAULT NULL;
CREATE UNIQUE INDEX places_inventory_location_index ON places (inventory_location) WHERE inventory_location IS NOT NULL;

-- Hibajavítás (null címek felszaporodása)

CREATE OR REPLACE FUNCTION public.replace_arp(
	ipa inet,
	hwa macaddr,
	stp settype DEFAULT 'query'::settype,
	hsi bigint DEFAULT NULL::bigint)
    RETURNS reasons
    LANGUAGE 'plpgsql'
    COST 100
    VOLATILE PARALLEL UNSAFE
AS $BODY$
DECLARE
    arp     arps;               -- arps record
    noipa   boolean := false;   -- Not found ip_address record
    joint   boolean := false;   -- Is joint type ip address
    oip     ip_addresses;       -- Found IP address record
    n       integer;            -- record number
    t       text;               -- Text for arps record (arp_note)
    msg     text;               -- Text for ip_address record (ip_address_note), or other message
    col     boolean := false;   -- Address collision
    adt addresstype := 'fixip';
BEGIN
    -- RAISE INFO 'ARP : % - %; %', ipa, hwa, stp;
    -- check ip_addresses table (set: noipa and joint)
    BEGIN       -- IP -> Get old () ip_addresses record
        SELECT * INTO STRICT oip FROM ip_addresses WHERE address = ipa AND ip_address_type <> 'private'::addresstype;
        joint := 'joint'::addresstype = oip.ip_address_type;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                noipa := true;
                -- RAISE INFO 'No ip_addresses record.';
            WHEN TOO_MANY_ROWS THEN
		-- RAISE INFO 'More ip_addresses record.';
                IF 0 < COUNT(*) FROM ip_addresses WHERE address = ipa AND NOT(ip_address_type = 'joint'::addresstype OR ip_address_type = 'private'::addresstype) THEN
                    PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
                END IF;
                -- RAISE INFO 'ip_addresses records is all join.';
                joint := true;
    END;
    IF NOT noipa THEN -- Van ilyen IP cím
        IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
            IF oip.ip_address_type = 'dynamic' THEN
            	-- Delete other null ip record(s)
            	DELETE FROM ip_addresses WHERE address IS NULL AND subnet_id = oip.subnet_id;
                -- Delete outdated address
                UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
                INSERT INTO ip_address_logs(reason, daemon_id, ip_address_id, ip_address_type_new, port_id, address_old, ip_address_type_old)
                    VALUES('discard', hsi, oip.ip_address_id, 'dynamic', oip.port_id, oip.address, 'dynamic');
                noipa := true;
            ELSE
                col := true;
            END IF;
        END IF;
    END IF;
    IF noipa THEN   -- ip_addresses record not found by IP address (ipa)
        -- ip address record(s) by MAC
        SELECT ip_addresses.* INTO oip FROM ip_addresses JOIN interfaces USING(port_id)
                WHERE hwaddress = hwa AND (ip_address_type = 'fixip'::addresstype OR ip_address_type = 'dynamic'::addresstype);
        GET DIAGNOSTICS n = ROW_COUNT;
        IF n = 1 AND (stp = 'config' OR oip.ip_address_type = 'dynamic') THEN
            DECLARE
                det text;
                hnt text;
                snid bigint := oip.subnet_id;
            BEGIN 
                IF stp = 'query' AND is_dyn_addr(ipa) IS NOT NULL THEN
                    adt := 'dynamic';
                END IF;
                IF NOT ipa << netaddr FROM subnets WHERE subnet_id = snid THEN
                    snid := NULL;
                END IF;
                t := 'Modify by config : ' || oip.ip_address_type  || ' -> ' || adt || '; ' || oip.address || ' -> ' || ipa;
                msg := 'Modify by replace_arp(), service : ' || COALESCE(host_service_id2name(hsi), 'NULL') || ' ' || NOW()::text;
                UPDATE ip_addresses SET ip_address_note = msg, address = ipa, ip_address_type = adt, subnet_id = snid WHERE ip_address_id = oip.ip_address_id;
                INSERT INTO ip_address_logs(reason, message, daemon_id, ip_address_id, address_new, ip_address_type_new, port_id, address_old, ip_address_type_old)
                    VALUES('modify', msg, hsi, oip.ip_address_id, ipa, adt, oip.port_id, oip.address, oip.ip_address_type);
            EXCEPTION WHEN OTHERS THEN
                GET STACKED DIAGNOSTICS
                    msg = MESSAGE_TEXT,
                    det = PG_EXCEPTION_DETAIL,
                    hnt = PG_EXCEPTION_HINT;
                t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp
                    || ' The existing IP address record port name : ' || port_id2full_name(oip.port_id)
                    || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
                -- RAISE WARNING 'Ticket : %', t; 
                PERFORM ticket_alarm('critical', t, hsi);
            END;
        ELSE
            DECLARE
                pid bigint;
            BEGIN
                SELECT port_id INTO pid FROM interfaces WHERE hwaddress = hwa;
                GET DIAGNOSTICS n = ROW_COUNT;
                IF n = 1 THEN
                    IF stp = 'query' AND is_dyn_addr(ipa) IS NOT NULL THEN
                        adt := 'dynamic';
                    END IF;
                    t := 'Inser IP address (' || adt || ') record : port : ' || port_id2full_name(pid) || ' .';
                    msg := 'Insert by replace_arp(), service : ' || COALESCE(host_service_id2name(hsi), 'NULL') || ' ' || NOW()::text;
                    INSERT INTO ip_addresses(port_id, ip_address_note, address, ip_address_type) VALUES(pid, msg, ipa, adt);
                END IF;
            END;
        END IF;
    ELSIF col THEN  -- ip_addresses found by IP address (ipa) AND colision
        t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
          || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
        -- RAISE WARNING 'Ticket : %', t; 
        PERFORM ticket_alarm('critical', t, hsi);
    ELSE            -- ip_addresses found by IP address (ipa) AND NOT colision
        IF stp = 'config' AND oip.ip_address_type = 'dynamic' THEN
            DECLARE
                sid bigint := NULL;
            BEGIN
                UPDATE ip_addresses SET ip_address_type = 'fixip' WHERE ip_address_id = oip.ip_address_id;
                INSERT INTO ip_address_logs(reason, message, daemon_id, ip_address_id, address_new, ip_address_type_new, port_id, address_old, ip_address_type_old)
                    VALUES('update', msg, hsi, oip.ip_address_id, ipa, 'fixip', oip.port_id, ipa, 'dynamic');
            END;
        END IF;
    END IF;
    -- update arps table
    -- RAISE INFO 'Get arps record : %', ipa;
    SELECT * INTO arp FROM arps WHERE ipaddress = ipa;
    IF NOT FOUND THEN
        -- RAISE INFO 'Insert arps: % - %', ipa, hwa;
        INSERT INTO arps(ipaddress, hwaddress,set_type, host_service_id, arp_note) VALUES (ipa, hwa, stp, hsi, t);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = hwa THEN
	    IF arp.set_type < stp THEN
                -- RAISE INFO 'Update arps: % - %', ipa, hwa;
	        UPDATE arps SET set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'update';
	    ELSE
                -- RAISE INFO 'Touch arps: % - %', ipa, hwa;
	        UPDATE arps SET last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'found';
	    END IF;
        ELSE
            -- RAISE INFO 'Move arps: % - % -> %', ipa, arp.hwaddress, hwa;
            UPDATE arps
                SET hwaddress = hwa,  first_time = CURRENT_TIMESTAMP, set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP, arp_note = t
                WHERE ipaddress = arp.ipaddress;
            INSERT INTO
                arp_logs(reason, ipaddress, hwaddress_new, hwaddress_old, set_type_old, host_service_id_old, first_time_old, last_time_old)
                VALUES( 'move',  ipa,       hwa,           arp.hwaddress, arp.set_type, arp.host_service_id, arp.first_time, arp.last_time);
            RETURN 'modify';
        END IF;
    END IF;
END;
$BODY$;

ALTER FUNCTION public.replace_arp(inet, macaddr, settype, bigint)
    OWNER TO lanview2;

-- Az öszes NULL címet törli, az egyedieket is!!
DELETE FROM ip_addresses WHERE address IS NULL;
	
SELECT set_db_version(1, 32);

-- 2023.01.19.

CREATE OR REPLACE FUNCTION get_inventory_location(il text, ppid bigint) RETURNS text LANGUAGE 'plpgsql'
    STABLE LEAKPROOF PARALLEL SAFE
AS $$
DECLARE
	rec	record;
	r 	text   := il;
	pid bigint := ppid;
BEGIN
	LOOP
		EXIT WHEN r IS NOT NULL OR pid IS NULL;
		SELECT inventory_location, parent_id INTO rec FROM places WHERE place_id = pid;
		r   := rec.inventory_location;
		pid := rec.parent_id;
	END LOOP;
	RETURN r;
END;
$$;

ALTER FUNCTION get_inventory_location(text, bigint)
    OWNER TO lanview2;
