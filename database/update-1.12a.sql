-- Hibajavítások...

DROP FUNCTION IF EXISTS get_str_port_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_bool_port_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_int_port_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_interval_port_param(pid bigint, tname text);

CREATE OR REPLACE FUNCTION get_bool_sys_param(pname text) RETURNS boolean AS $$
BEGIN
    RETURN cast_to_boolean(get_text_sys_param(pname));
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_int_sys_param(pname text) RETURNS bigint AS $$
BEGIN
    RETURN cast_to_integer(get_text_sys_param(pname));
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_interval_sys_param(pname text) RETURNS interval AS $$
BEGIN
    RETURN cast_to_interval(get_text_sys_param(pname));
END
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION refresh_mactab() RETURNS integer AS $$
DECLARE
    mt  mactab;
    ret integer := 0;
    t1 timestamp;
    t2 timestamp;
    t3 timestamp;
BEGIN
    t1 := now() - get_interval_sys_param('mactab_check_stat_interval');
    FOR mt IN SELECT * FROM mactab
        WHERE  state_updated_time < t1
    LOOP
        IF 'update' = mactab_changestat(mt, current_mactab_stat(mt.port_id, mt.hwaddress, mt.mactab_state)) THEN
            ret := ret + 1;
        END IF;
    END LOOP;
    
    t1 := now() - get_interval_sys_param('mactab_suspect_expire_interval');
    t2 := now() - get_interval_sys_param('mactab_expire_interval');
    t3 := now() - get_interval_sys_param('mactab_reliable_expire_interval');
    FOR mt IN SELECT * FROM mactab
        WHERE ( last_time < t1  AND     mactab_state && ARRAY['suspect']     ::mactabstate[] )
           OR ( last_time < t2  AND NOT mactab_state && ARRAY['arp','likely']::mactabstate[] )
           OR ( last_time < t3)
    LOOP
	RAISE INFO 'Remove (expired) : %', mt;
        PERFORM mactab_remove(mt, 'expired');
        ret := ret +1;
    END LOOP;
    
    FOR mt IN SELECT DISTINCT(mactab.*)
	FROM mactab
	JOIN port_params USING(port_id)
	JOIN param_types USING(param_type_id)
	WHERE (param_type_name = 'query_mac_tab'    AND NOT cast_to_boolean(param_value, true)) 
	   OR (param_type_name = 'suspected_uplink' AND     cast_to_boolean(param_value, false)) 
    LOOP
	RAISE INFO 'Remove (discard) : %', mt;
        PERFORM mactab_remove(mt, 'discard');
        ret := ret +1;
    END LOOP;

    RETURN ret;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION refresh_arps() RETURNS integer AS $$
DECLARE
    a  arps;
    ret integer := 0;
    t timestamp;
BEGIN
    t := now() - get_interval_sys_param('arps_expire_interval');
    FOR a IN SELECT * FROM arps
        WHERE  set_type < 'config'::settype AND last_time < t
    LOOP
        PERFORM arp_remove(a, 'expired');
        ret := ret +1;
    END LOOP;
    RETURN ret;
END;
$$ LANGUAGE plpgsql;

DROP FUNCTION IF EXISTS expired_onine_alarm(did bigint);
CREATE OR REPLACE FUNCTION expired_online_alarm() RETURNS VOID AS $$
DECLARE
    t timestamp;
BEGIN
    t := now() - get_interval_sys_param('user_notice_timeout');
    IF NOT NULL t THEN
        UPDATE user_events SET event_state = 'dropped', happened = now()
            WHERE event_type = 'notice' AND event_state = 'necessary' AND created < t;
    END IF;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION expired_offline_alarm() RETURNS VOID AS $$
DECLARE
    t interval;
BEGIN
    t := now() - get_interval_sys_param('user_message_timeout');
    IF NOT NULL t THEN
        UPDATE user_events SET event_state = 'dropped', happened = now()
            WHERE event_type = 'sendmail' AND event_state = 'necessary' AND created < t;
    END IF;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION service_cron(did bigint) RETURNS VOID AS $$
BEGIN
    PERFORM services_heartbeat(did);
    PERFORM expired_online_alarm();
    PERFORM expired_offline_alarm();
    PERFORM refresh_mactab();
    PERFORM refresh_arps();
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION replace_mactab(
    pid bigint,
    mac macaddr,        
    typ settype DEFAULT 'query',
    mst mactabstate[] DEFAULT '{}'
) RETURNS reasons AS $$
DECLARE
    mt       mactab;
    pdiid    bigint;
    maxct    CONSTANT bigint    := get_int_sys_param('mactab_move_check_count');
    mv       CONSTANT reasons   := 'move';
    btm      CONSTANT timestamp := CURRENT_TIMESTAMP - get_interval_sys_param('mactab_move_check_interval');
BEGIN
    SELECT param_type_id INTO pdiid FROM param_types WHERE param_type_name = 'suspected_uplink';
    IF NOT FOUND THEN
         PERFORM error('NameNotFound', -1, 'suspected_uplink', 'replace_mactab(bigint,macaddr,settype,mactabstate[])', 'param_types');
    END IF;
    IF  cast_to_boolean(param_value, false) FROM port_params WHERE port_id = pid AND param_type_id = pdiid THEN
        RETURN 'discard';
    END IF;
    mst := current_mactab_stat(pid, mac, mst);
    SELECT * INTO mt FROM mactab WHERE hwaddress = mac;
    IF NOT FOUND THEN
        INSERT INTO mactab(hwaddress, port_id, mactab_state,set_type) VALUES (mac, pid, mst, typ);
        RETURN 'insert';
    ELSE
        IF mt.port_id = pid THEN
            -- No changed, refresh state
            RETURN mactab_changestat(mt, mst, typ, true);
        ELSE
            -- The old port not suspect and to many change
            IF NOT cast_to_boolean(param_value, false) FROM port_params WHERE port_id = mt.port_id AND param_type_id = pdiid
             AND maxct < (SELECT COUNT(*) FROM mactab_logs WHERE date_of > btm AND hwaddress = mac AND reason = mv) THEN
                IF (SELECT COUNT(*) FROM mactab_logs WHERE date_of > btm AND port_id_old = pid        AND reason = mv)
                 < (SELECT COUNT(*) FROM mactab_logs WHERE date_of > btm AND port_id_old = mt.port_id AND reason = mv)
                THEN    -- Nem az új pid a gyanús
                    PERFORM mactab_move(mt, pid, mac, typ, mst);
                    INSERT INTO port_params(mt.port_id, param_type_id, param_value) VALUES (pid, pdiid, 't');
                    UPDATE mactab_logs SET be_void = true WHERE date_of > btm AND port_id_old = mt.port_id AND reason = mv AND hwaddress = mac;
                    RETURN 'modify';
                ELSE
                    INSERT INTO port_params(port_id, param_type_id, param_value) VALUES (pid, pdiid, 't');
                    UPDATE mactab_logs SET be_void = true WHERE date_of > btm AND port_id_old = pid        AND reason = mv AND hwaddress = mac;
                    RETURN 'restore';
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

CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
    ipa ip_addresses;
    nip boolean;    -- IP address type IS NULL
    snid bigint;
    cnt integer;
BEGIN
    RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím ?
    nip := NEW.ip_address_type IS NULL;
    IF nip THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) IS NOT NULL THEN
            NEW.ip_address_type := 'dynamic';
        ELSE
            NEW.ip_address_type := 'fixip';
        END IF;
    END IF;
    IF NEW.address IS NOT NULL THEN
        -- Check subnet_id
        IF NEW.subnet_id IS NOT NULL AND (SELECT NEW.address << netaddr FROM subnets WHERE subnet_id = NEW.subnet_id) THEN
            RAISE INFO 'Clear subnet id : %', NEW.subnet_id;
            NEW.subnet_id := NULL;  -- Clear invalid subnet_id
        END IF;
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
            RAISE INFO 'Set subnet id : %', NEW.subnet_id;
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


