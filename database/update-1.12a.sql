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

