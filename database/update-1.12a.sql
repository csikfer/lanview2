-- Hibajavítások...

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

DROP FUNCTION IF EXISTS get_str_port_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_bool_port_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_int_port_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_interval_port_param(pid bigint, tname text);
