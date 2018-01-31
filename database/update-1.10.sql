-- ************************************************ BUGFIX ******************************************************************* 
CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    noipa   boolean := false;
    joint   boolean := false;
    oip     ip_addresses;
    net     cidr;
    sni     bigint;
    n       integer;
    t       text;
BEGIN
    RAISE INFO 'ARP : % - %; %', ipa, hwa, stp;
    -- check / update ip_addresses table
    BEGIN       -- IP -> Get old ip_addresses record
        SELECT * INTO STRICT oip FROM ip_addresses WHERE address = ipa;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                noipa := true;
            WHEN TOO_MANY_ROWS THEN
                IF 0 < COUNT(*) FROM ip_addresses WHERE address = ipa AND ip_address_type <> 'joint'::addresstype THEN
                    PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
                END IF;
                joint := true;
    END;
    IF stp = 'config' THEN  -- We assume this is fixip
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
                    PERFORM ticket_alarm('critical', t, hsi);
                END;
            END IF;
        ELSIF joint THEN    -- Check ip_addresses record if joint
            IF 0 = COUNT(*) FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa AND hwaddress = hwa THEN
                t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                  || 'The existing IP address record ports names : '
                  || ARRAY(SELECT port_id2full_name(port_id) FROM ip_addresses WHERE address = ipa)::text;
                PERFORM ticket_alarm('critical', t, hsi);
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
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        END IF;
    ELSE    -- stp <> 'config' ( = 'query')
        IF joint THEN    -- Check ip_addresses record if joint
            IF 0 = COUNT(*) FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa AND hwaddress = hwa THEN
                t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                  || 'The existing IP address record ports names : ' 
                  || ARRAY(SELECT port_id2full_name(port_id) FROM ip_addresses WHERE address = ipa)::text;
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        ELSE
            IF NOT noipa THEN
                RAISE INFO 'Address record (%) : % - %', stp, ipa, port_id2full_name(oip.port_id);
                IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
                    IF oip.ip_address_type = 'dynamic' THEN     -- Clear old dynamic IP
                        RAISE INFO 'Delete address (%) : % - %', stp, port_id2full_name(oip.port_id), ipa;
                        UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
                        noipa := true;
                    ELSE
                        t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is query. '
                        || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
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
                        t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is query. '
                        || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id)
                        || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
                        PERFORM ticket_alarm('critical', t, hsi);
                    END;
                END IF;
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
COMMENT ON FUNCTION replace_arp(inet, macaddr, settype, bigint) IS
'A detektált MAC - IP cím pár alapján modosítja az arps táblát, és kezeli a napló táblát is
Ellenörzi, és frissíti az ip_addresses táblát is (ha stp = "config", akkor feltételezi, hogy ez egy fixip típusú cím)
Ha ütközést észlel, akkor létrehoz egy alarms rekordot (szolgáltatás : ticket, host_service_id = 0).
Paraméterek:
    ipa     IP cím
    hwa     MAC
    stp     adat forrás, default "query"
    hsi     opcionális host_service_id
Visszatérési érték:
    Ha létrejött egy új rekord, akkor "insert".
    Ha nincs változás (csak a last_time frissül), akkor "found".
    Ha a cím összerendelés nem változott, de a set_type igen, akkor "update".
    Ha az IP cím egy másik MAC-hez lett rendelve, akkor "modfy".';

-- Ellenőrzi, hogy a address (ip cím) mező rendben van-e
-- Nem ütközik más címmel
CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
    ipa ip_addresses;
    nip boolean;
BEGIN
 -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím
    nip := NEW.ip_address_type IS NULL;
    IF nip THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) IS NOT NULL THEN
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
        -- Ha nem private, akkor vizsgáljuk az ütközéseket
        IF NEW.address IS NOT NULL AND NEW.ip_address_type <> 'private' THEN
            -- Azonos IP címek?
            FOR ipa IN SELECT * FROM ip_addresses WHERE NEW.address = address LOOP
                IF ipa.ip_address_id <> NEW.ip_address_id THEN 
                    IF ipa.ip_address_type = 'dynamic' THEN
                    -- Ütköző dinamikust töröljük.
                        UPDATE ip_addresses SET address = NULL WHERE ip_address_id = ipa.ip_address_id;
                    ELSIF ipa.ip_address_type = 'joint' AND (nip OR NEW.ip_address_type = 'joint') THEN
                    -- Ha közös címként van megadva a másik, ...
                        NEW.ip_address_type := 'joint';
                    ELSIF ipa.ip_address_type <> 'private' THEN
                    -- Minden más esetben ha nem privattal ütközik az hiba
                        PERFORM error('IdNotUni', 0, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    END IF;
                END IF;
            END LOOP;
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

-- Ellenőrzi, hogy a paraméterként megadott IP cím része-e egy dinamikus IP tartománynak,
CREATE OR REPLACE FUNCTION is_dyn_addr(inet) RETURNS bigint AS $$
DECLARE
    id  bigint;
BEGIN
    SELECT dyn_addr_range_id INTO id FROM dyn_addr_ranges WHERE exclude = false AND begin_address <= $1 AND $1 <= end_address;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    SELECT dyn_addr_range_id         FROM dyn_addr_ranges WHERE exclude = true  AND begin_address <= $1 AND $1 <= end_address;
    IF FOUND THEN
        RETURN NULL;
    END IF;
    RETURN id;
END;
$$ LANGUAGE plpgsql;

UPDATE param_types SET param_type_name = 'bytes_per_sec', param_type_type = 'real' WHERE param_type_name = 'bypes_per_sec';

-- END BUGFIX

ALTER TABLE service_var_types ADD COLUMN raw_param_type_id bigint;
UPDATE service_var_types SET raw_param_type_id = param_type_id;
ALTER TABLE service_var_types ALTER COLUMN raw_param_type_id SET NOT NULL;
ALTER TABLE service_var_types ADD FOREIGN KEY (raw_param_type_id)
    REFERENCES param_types(param_type_id) MATCH FULL ON UPDATE RESTRICT ON DELETE RESTRICT;
COMMENT ON COLUMN service_var_types.raw_param_type_id IS 'A host_service_vars.raw_value adat típusa, ami nem feltétlenül azonos a változó típusával.';
INSERT INTO param_types (param_type_name, param_type_note, param_type_type, param_type_dim) VALUES ('packets_per_sec', 'packets/sec', 'real', 'pkt/s');
UPDATE service_var_types SET param_type_id = param_type_name2id('bytes_per_sec'),   raw_param_type_id = param_type_name2id('bytes')   WHERE service_var_type_name = 'ifbytes';
UPDATE service_var_types SET param_type_id = param_type_name2id('packets_per_sec'), raw_param_type_id = param_type_name2id('packets') WHERE service_var_type_name = 'ifpacks';
INSERT INTO table_shape_fields (table_shape_field_name, table_shape_id, field_sequence_number) VALUES ('raw_param_type_id', table_shape_name2id('service_var_types'), 45);

CREATE OR REPLACE FUNCTION check_before_service_value() RETURNS TRIGGER AS $$
DECLARE
    tid bigint;
    pt  paramtype;
BEGIN
    IF NEW.service_var_value IS NOT NULL THEN
        SELECT param_type_id     INTO tid FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id;
        SELECT param_type_type   INTO pt  FROM param_types       WHERE param_type_id = tid;
        NEW.service_var_value = check_paramtype(NEW.service_var_value, pt);
    END IF;
    IF NEW.raw_value         IS NOT NULL THEN
        SELECT raw_param_type_id INTO tid FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id;
        SELECT param_type_type   INTO pt  FROM param_types       WHERE param_type_id = tid;
        NEW.raw_value         = check_paramtype(NEW.raw_value, pt);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql IMMUTABLE;


SELECT set_db_version(1, 10);
