ALTER TABLE node_params RENAME TO node_params_old;

CREATE TABLE node_params (
  node_param_id bigserial NOT NULL PRIMARY KEY ,
  node_param_name text NOT NULL,
  node_param_note text DEFAULT NULL,
  node_id bigint NOT NULL,
  param_type_id bigint NOT NULL
      REFERENCES param_types (param_type_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE RESTRICT,
  param_value text,
  flag boolean DEFAULT false,
  UNIQUE (node_param_name, node_id)
);
ALTER TABLE node_params OWNER TO lanview2;
COMMENT ON TABLE  node_params IS 'Node extra paraméter értékek.';
COMMENT ON COLUMN node_params.node_param_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN node_params.node_param_name IS 'Paraméter neve.';
COMMENT ON COLUMN node_params.node_param_note IS 'Megjegyzés.';
COMMENT ON COLUMN node_params.node_id IS 'A tulajdonos node rekordjának az azonosítója.';
COMMENT ON COLUMN node_params.param_type_id IS 'A paraméter adat típusát definiáló param_types rekord azonosítója.';
COMMENT ON COLUMN node_params.param_value IS 'A parméter érték.';

WITH npo AS (
    SELECT node_param_id,
           param_type_name AS node_param_name,
           NULL AS node_param_note,
           node_id,
           param_type_id,
           param_value,
           flag
    FROM node_params_old
    JOIN param_types USING(param_type_id) )
INSERT INTO node_params SELECT * FROM npo;

DROP TABLE node_params_old;

CREATE TRIGGER node_param_check_value
  BEFORE INSERT OR UPDATE ON node_params
  FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();

CREATE TRIGGER node_param_value_check_reference_node_id
  BEFORE INSERT OR UPDATE ON node_params
  FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('patchs');

-- - - - - - - - - - - - - - -

DROP FUNCTION IF EXISTS set_str_node_param(nid bigint, txtval text, tname text);
DROP FUNCTION IF EXISTS set_bool_node_param(pid bigint, boolval boolean, tname text);
DROP FUNCTION IF EXISTS set_int_node_param(pid bigint, intval bigint, tname text);
DROP FUNCTION IF EXISTS set_interval_node_param(pid bigint, ival interval, tname text);
DROP FUNCTION IF EXISTS get_str_node_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_bool_node_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_int_node_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_interval_node_param(pid bigint, tname text);

-- ----------------------------

ALTER TABLE port_params RENAME TO port_params_old;

CREATE TABLE port_params (
  port_param_id bigserial NOT NULL PRIMARY KEY ,
  port_param_name text NOT NULL,
  port_param_note text DEFAULT NULL,
  port_id bigint NOT NULL,
  param_type_id bigint NOT NULL
      REFERENCES param_types (param_type_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE RESTRICT,
  param_value text,
  flag boolean DEFAULT false,
  UNIQUE (port_param_name, port_id)
);
ALTER TABLE port_params OWNER TO lanview2;
COMMENT ON TABLE  port_params IS 'port extra paraméter értékek.';
COMMENT ON COLUMN port_params.port_param_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN port_params.port_param_name IS 'Paraméter neve.';
COMMENT ON COLUMN port_params.port_param_note IS 'Megjegyzés.';
COMMENT ON COLUMN port_params.port_id IS 'A tulajdonos port rekordjának az azonosítója.';
COMMENT ON COLUMN port_params.param_type_id IS 'A paraméter adat típusát definiáló param_types rekord azonosítója.';
COMMENT ON COLUMN port_params.param_value IS 'A parméter érték.';

WITH ppo AS (
    SELECT port_param_id,
           param_type_name AS port_param_name,
           NULL AS port_param_note,
           port_id,
           param_type_id,
           param_value,
           flag
    FROM port_params_old
    JOIN param_types USING(param_type_id) )
INSERT INTO port_params SELECT * FROM ppo;

DROP TABLE port_params_old;

CREATE TRIGGER port_param_check_value BEFORE INSERT OR UPDATE ON port_params
  FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();

CREATE TRIGGER port_params_check_reference_port_id BEFORE INSERT OR UPDATE ON port_params
  FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id();

-- - - - - - - - - - - - - - -

DROP FUNCTION IF EXISTS set_str_port_param(bigint, text, text);
DROP FUNCTION IF EXISTS set_bool_port_param(bigint, boolean, text);
DROP FUNCTION IF EXISTS set_int_port_param(bigint, bigint, text);
DROP FUNCTION IF EXISTS set_interval_port_param(bigint, interval, text);
DROP FUNCTION IF EXISTS get_str_port_param(bigint, text);
DROP FUNCTION IF EXISTS get_bool_port_param(bigint, text);
DROP FUNCTION IF EXISTS get_int_port_param(bigint, text);
DROP FUNCTION IF EXISTS get_interval_port_param(bigint, text);

-- For replace_mactab()
CREATE FUNCTION get_bool_port_param(pid bigint, name text) RETURNS boolean AS $$
BEGIN
    RETURN cast_to_boolean(param_value) FROM port_params WHERE port_id = pid AND port_param_name = name;
END;
$$ LANGUAGE plpgsql;

CREATE FUNCTION set_bool_port_param(pid bigint, val boolean, name text) RETURNS VOID AS $$
BEGIN
    INSERT INTO port_params(port_param_name, port_id, param_type_id, param_value)
        VALUES (name, pid, param_type_name2id('boolean'), val::text)
    ON CONFLICT ON CONSTRAINT port_params_port_param_name_port_id_key DO UPDATE
        SET param_type_id = EXCLUDED.param_type_id, param_value = EXCLUDED.param_value;
END;
$$ LANGUAGE plpgsql;

-- - - - - - - - - - - - - - -

UPDATE port_params SET param_type_id = param_type_name2id('boolean')
    WHERE param_type_id = param_type_name2id('query_mac_tab') OR param_type_id = param_type_name2id('link_is_invisible_for_LLDP')
       OR param_type_id = param_type_name2id('suspected_uplink');

UPDATE node_params SET param_type_id = param_type_name2id('date')
    WHERE param_type_id = param_type_name2id('battery_changed');

DELETE FROM param_types
    WHERE param_type_name = 'search_domain'
       OR param_type_name = 'query_mac_tab' OR param_type_name = 'link_is_invisible_for_LLDP'
       OR param_type_name = 'suspected_uplink'
       OR param_type_name = 'battery_changed';

-- ----------------------------

SELECT set_db_version(1, 22);

-- ----------------------------
-- Hibajavítás : 2019.04.02.

ALTER TABLE query_parsers ALTER COLUMN regular_expression SET NOT NULL;
ALTER TABLE query_parsers ALTER COLUMN item_sequence_number SET NOT NULL;

-- Javítások : 2019.04.04.

CREATE OR REPLACE FUNCTION mac2full_port_name(mac macaddr) RETURNS text AS $$
DECLARE
    nn text[];
BEGIN
    nn = ARRAY(
        SELECT node_name || ':' || port_name
            FROM interfaces
            JOIN nodes USING(node_Id)
            WHERE hwaddress = mac
            ORDER BY node_name ASC, port_name ASC
        );
    IF array_length(nn, 1) = 0 THEN
        RETURN NULL;
    END IF;
    RETURN array_to_string(nn, ', ');
END;
$$ LANGUAGE plpgsql IMMUTABLE;
ALTER FUNCTION mac2full_port_name(macaddr) OWNER TO lanview2;

CREATE OR REPLACE FUNCTION ip2full_port_name(ip inet) RETURNS text AS $$
DECLARE
    nn text[];
BEGIN
    nn = ARRAY(
        SELECT node_name || ':' || port_name
            FROM interfaces
            JOIN ip_addresses USING(port_id)
            JOIN nodes USING(node_Id)
            WHERE address = ip
            ORDER BY node_name ASC, port_name ASC
        );
    IF array_length(nn, 1) = 0 THEN
        RETURN NULL;
    END IF;
    RETURN array_to_string(nn, ', ');
END;
$$ LANGUAGE plpgsql IMMUTABLE;
ALTER FUNCTION ip2full_port_name(inet) OWNER TO lanview2;

DROP VIEW IF EXISTS arps_shape;

CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    noipa   boolean := false;
    joint   boolean := false;
    oip     ip_addresses;
    net     cidr;
    n       integer;
    t       text;
BEGIN
    RAISE INFO 'ARP : % - %; %', ipa, hwa, stp;
    -- check / update ip_addresses table
    BEGIN       -- IP -> Get old ip_addresses record
        SELECT * INTO STRICT oip FROM ip_addresses WHERE address = ipa AND ip_address_type <> 'private'::addresstype;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                noipa := true;
                -- RAISE INFO 'No ip_addresses record.';
            WHEN TOO_MANY_ROWS THEN
		-- RAISE INFO 'More ip_addresses record.';
                IF 0 < COUNT(*) FROM ip_addresses WHERE address = ipa AND NOT(ip_address_type = 'joint'::addresstype OR ip_address_type = 'private'::addresstype) THEN
                    PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
                END IF;
                RAISE INFO 'ip_addresses records is all join.';
                joint := true;
    END;
    IF joint THEN    -- Check ip_addresses record if joint
        IF 0 = COUNT(*) FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa AND hwaddress = hwa THEN
	    -- not found, fake MAC ?
	    IF 0 = COUNT(*) FROM ip_addresses JOIN port_params USING(port_id) JOIN param_types USING(param_type_id) WHERE param_type_name = 'override_mac' AND address = ipa AND cast_to_mac(param_value) = hwa THEN
		t := 'IP address (join) missing : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
		 || 'The existing IP address record ports names : '
		 || ARRAY(SELECT ' ' || port_id2full_name(port_id) || ' / ' || hwaddress || ' '  FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa)::text;
		RAISE WARNING 'Ticket : %', t; 
		PERFORM ticket_alarm('critical', t, hsi);
	    END IF;
        END IF;
    ELSIF stp = 'config' THEN  -- We assume this is fixip
        IF noipa THEN   -- not found ip_addresses record
            RAISE INFO 'Nothing address record (config) : %', ipa;
            SELECT ip_addresses.* INTO oip FROM ip_addresses JOIN interfaces USING(port_id) WHERE ip_address_type = 'dynamic' AND hwaddress = hwa;
            GET DIAGNOSTICS n = ROW_COUNT;
            IF n = 1 THEN   -- One record ?
		t := 'Modify by config : ' || oip.ip_address_type  || ' -> fixip ; ' || oip.address || ' -> ' || ipa;
		PERFORM error('DataWarn', oip.ip_address_id, t, 'replace_arp', 'ip_addresses');
                DECLARE
                    msg text;
                    det text;
                    hnt text;
                BEGIN 
                    UPDATE ip_addresses SET address = ipa, ip_address_type = 'fixip' WHERE ip_address_id = oip.ip_address_id;
                EXCEPTION WHEN OTHERS THEN
                    GET STACKED DIAGNOSTICS
                        msg = MESSAGE_TEXT,
                        det = PG_EXCEPTION_DETAIL,
                        hnt = PG_EXCEPTION_HINT;
                    t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id)
                      || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
                    RAISE WARNING 'Ticket : %', t; 
                    PERFORM ticket_alarm('critical', t, hsi);
                END;
            END IF;
        ELSE                -- Check ip_addresses record if not joint
            RAISE INFO 'Address record (config) : % - %', ipa, port_id2full_name(oip.port_id);
            IF hwa = hwaddress FROM interfaces WHERE port_id = oip.port_id THEN -- Check MAC
                IF oip.ip_address_type = 'dynamic' THEN
                    UPDATE ip_addresses SET ip_address_type = 'fixip' WHERE ip_address_id = oip.ip_address_id;
                END IF;
            ELSE    -- ip address collision
                t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                  || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
                RAISE WARNING 'Ticket : %', t; 
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        END IF;
    ELSE    -- stp <> 'config' ( = 'query')
        IF NOT noipa THEN
	    RAISE INFO 'Address record (%) : % - %', stp, ipa, port_id2full_name(oip.port_id);
	    IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
		IF oip.ip_address_type = 'dynamic' THEN     -- Clear old dynamic IP
		    RAISE INFO 'Delete address (%) : % - %', stp, port_id2full_name(oip.port_id), ipa;
		    UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
		    noipa := true;
		ELSE
		    t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
		      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
		    RAISE WARNING 'Ticket : %', t; 
		    PERFORM ticket_alarm('critical', t, hsi);
		END IF;
	    END IF;
	END IF;
	IF noipa THEN
	    RAISE INFO 'Nothing address record (%) : %', stp, ipa;
	    SELECT ip_addresses.* INTO oip FROM ip_addresses JOIN interfaces USING(port_id) WHERE ip_address_type = 'dynamic' AND hwaddress = hwa;
	    GET DIAGNOSTICS n = ROW_COUNT;
	    RAISE INFO '% record by hwaddress', n;
	    IF n = 1 THEN   -- One record ?
		DECLARE
		    msg text;
		    det text;
		    hnt text;
		BEGIN
		    RAISE INFO 'Update ip_address : % -> %', ipa, port_id2full_name(oip.port_id);
		    UPDATE ip_addresses SET address = ipa WHERE ip_address_id = oip.ip_address_id;
		EXCEPTION WHEN OTHERS THEN
		    GET STACKED DIAGNOSTICS
			msg = MESSAGE_TEXT,
			det = PG_EXCEPTION_DETAIL,
			hnt = PG_EXCEPTION_HINT;
		    t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
		      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id)
		      || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
		    RAISE WARNING 'Ticket : %', t; 
		    PERFORM ticket_alarm('critical', t, hsi);
		END;
	    END IF;
	END IF;
    END IF;
    -- update arps table
    RAISE INFO 'Get arps record : %', ipa;
    SELECT * INTO arp FROM arps WHERE ipaddress = ipa;
    IF NOT FOUND THEN
        RAISE INFO 'Insert arps: % - %', ipa, hwa;
        INSERT INTO arps(ipaddress, hwaddress,set_type, host_service_id, arp_note) VALUES (ipa, hwa, stp, hsi, t);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = hwa THEN
	    IF arp.set_type < stp THEN
                RAISE INFO 'Update arps: % - %', ipa, hwa;
	        UPDATE arps SET set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'update';
	    ELSE
                RAISE INFO 'Touch arps: % - %', ipa, hwa;
	        UPDATE arps SET last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'found';
	    END IF;
        ELSE
            RAISE INFO 'Move arps: % - % -> %', ipa, arp.hwaddress, hwa;
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
$$ LANGUAGE plpgsql;

-- Javítás !!!! 2019.04.11. !!!!
ALTER TYPE errtype ADD VALUE 'Info';
COMMENT ON TYPE errtype IS
'Hiba sújossága
Fatal   Fatal error
Error   Error
Warning Warning
Ok      Ok (Info)
Info    Info only (Not used by errors table)
';

