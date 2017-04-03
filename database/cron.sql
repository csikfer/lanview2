
CREATE OR REPLACE FUNCTION services_heartbeat(did bigint) RETURNS VOID AS $$
DECLARE
    hsr host_services;
    msg text;
BEGIN
    FOR hsr IN SELECT hs.*
         FROM host_services AS hs
         JOIN services      AS s  USING(service_id)
         WHERE
           host_service_state <> 'unknown' AND
           (NOW() - last_touched) > COALESCE(hs.heartbeat_time, s.heartbeat_time)
    LOOP
	msg := 'Expired "' || hsr.host_service_state::text || '" state : ' || (NOW() - hsr.last_touched)::text;
	-- RAISE NOTICE '%1 : %2', host_service_id2name(hsr.host_service_id), msg;
	PERFORM set_service_stat(hsr.host_service_id, 'unknown'::notifswitch, msg, did, true);
    END LOOP;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION expired_online_alarm(did bigint) RETURNS VOID AS $$
DECLARE
    expi interval;
BEGIN
    SELECT param_value::interval INTO expi FROM sys_params WHERE sys_param_name = 'user_notice_timeout';
    UPDATE user_events SET event_state = 'dropped'
        WHERE event_state = 'necessary' AND event_type = 'notice' AND (created + expi) < NOW();
END
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION service_cron(did bigint) RETURNS VOID AS $$
BEGIN
    PERFORM services_heartbeat(did);
    PERFORM expired_online_alarm(did);
    PERFORM refresh_mactab();
    PERFORM refresh_arps();
END
$$ LANGUAGE plpgsql;
