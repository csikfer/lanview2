-- Hibajavítások...

-- It was a serious mistake. It did not stop the alarms.
CREATE OR REPLACE FUNCTION set_service_stat(
    hsid        bigint,             -- host_service_id
    state       notifswitch,        -- new state
    note        text DEFAULT '',    -- message (optional)
    dmid        bigint DEFAULT NULL,-- Daemon host_service_id
    forced      boolean DEFAULT false)  -- Immediate change of status, if true
RETURNS host_services AS $$
DECLARE
    hs          host_services;  -- New host services record
    old_hs      host_services;  -- Old host_services record
    s           services;       -- Services rekord
    na          isnoalarm;      -- Alarm barring status
    sup         bigint;         -- Superior host_service_id
    tflapp      interval;       -- Flapping detection time window
    iflapp      integer;        -- Changes in the state within the time window
    alid        bigint;         -- Actual alarm record ID
    aldo        reasons := 'unknown';
BEGIN
    hs := touch_host_service(hsid);
    SELECT * INTO  s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'set_service_stat()', 'services');
    END IF;
    IF state = hs.host_service_state AND state = hs.hard_state AND state = hs.soft_state THEN
        RETURN hs;  -- No change in status
    END IF;
    old_hs := hs;
    CASE state
        WHEN 'on','recovered' THEN
            state := 'on';
            IF old_hs.host_service_state > 'recovered' THEN
                hs.host_service_state := 'recovered';
            ELSE
                hs.host_service_state := state;
            END IF;
            hs.hard_state := state;
            hs.soft_state := state;
            hs.check_attempts := 0;
        WHEN 'warning', 'unreachable','down','unknown','critical' THEN
            IF hs.hard_state <> 'on' THEN   -- So far it was bad
                hs.hard_state := state;
                hs.soft_state := state;
                hs.host_service_state := state;
                hs.check_attempts := 0;
            ELSE
                IF old_hs.soft_state = 'on' THEN    -- Is this the first mistake?
                    hs.check_attempts := 1;     -- Yes. ￼Let's start counting
                ELSE
                    hs.check_attempts := hs.check_attempts + 1; -- No. We continue to count
                END IF;
                hs.soft_state := state;
                IF hs.max_check_attempts IS NULL THEN
                    IF s.max_check_attempts IS NULL THEN    -- Missing default value
                        PERFORM error('WNotFound', hs.host_service_id, 'max_check_attempts', 'set_service_stat()');
                        hs.max_check_attempts := 1;         -- Then be 1
                    ELSE
                        hs.max_check_attempts := s.max_check_attempts;
                    END IF;
                END IF;
                IF forced OR hs.check_attempts >= hs.max_check_attempts THEN  -- It is definitely a problem
                    hs.hard_state := state;
                    hs.host_service_state := state;
                    hs.check_attempts := 0;
                END IF;
            END IF;
        ELSE
            PERFORM error('Params', state, 'notifswitch', 'set_service_stat()'); -- exception!
    END CASE;
    tflapp := COALESCE(hs.flapping_interval, s.flapping_interval);
    iflapp := COALESCE(hs.flapping_max_change, s.flapping_max_change);
    IF chk_flapping(hsid, tflapp, iflapp) THEN
        hs.host_service_state := 'flapping';
    END IF;
    -- delegate status
    PERFORM set_host_status(hs);
    sup := check_superior_service(hs);
    -- Disable alarms status?
    na := is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to);
    IF na = 'expired' THEN  -- If it has expired, it will be deleted
        hs.noalarm_flag := 'off';
        hs.noalarm_from := NULL;
        hs.noalarm_to   := NULL;
        na := 'off';
    END IF;
    IF hs.last_changed IS NULL OR old_hs.host_service_state <> hs.host_service_state THEN
        hs.last_changed = CURRENT_TIMESTAMP;
    END IF;
    -- Alarm
    alid := old_hs.act_alarm_log_id;
    CASE
        WHEN state = 'on' AND alid IS NOT NULL THEN
            RAISE INFO 'Close %1 alarms record.', alid;
            UPDATE alarms SET end_time = CURRENT_TIMESTAMP WHERE alarm_id = alid;
            hs.act_alarm_log_id := NULL;
            aldo := 'close';
        WHEN (old_hs.host_service_state < 'warning' OR old_hs.host_service_state = 'unknown') -- old: ok or unknown
         AND hs.host_service_state >= 'warning' THEN                                          -- new: not ok
            RAISE INFO 'New alarms record.';
            IF na = 'on' THEN -- Alarm is disabled?
                INSERT INTO disabled_alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id, noalarm)
                    VALUES(hsid, dmid, hs.host_service_state, hs.host_service_state, hs.host_service_state, note, sup, true )
                    RETURNING alarm_id INTO hs.act_alarm_log_id;
            ELSE 
                INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id, noalarm)
                    VALUES(hsid, dmid, hs.host_service_state, hs.host_service_state, hs.host_service_state, note, sup, false )
                    RETURNING alarm_id INTO hs.act_alarm_log_id;
            END IF;
            aldo := 'new';
            alid := hs.act_alarm_log_id;
            hs.last_alarm_log_id := alid;
        WHEN old_hs.host_service_state < hs.host_service_state OR old_hs.host_service_state = 'unknown' THEN
        RAISE INFO 'The situation is worse. Update %1 alarms record.', alid;
            UPDATE alarms SET
                    max_status  = hs.host_service_state,
                    last_status = hs.host_service_state,
                    superior_alarm_id = sup
                WHERE alarm_id = alid;
            aldo := 'modify';
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.host_service_state < hs.host_service_state THEN
            RAISE INFO 'The situation is better. Update % alarms record', alid;
            UPDATE alarms SET
                    last_status = hs.host_service_state,
                    superior_alarm_id = sup
                WHERE alarm_id = alid;
            aldo := 'modify';
        ELSE
            RAISE INFO 'No mod % alarm.' , alid;
            aldo := 'unchange';
    END CASE;
    UPDATE host_services SET
            max_check_attempts = hs.max_check_attempts,
            host_service_state = hs.host_service_state,
            hard_state         = hs.hard_state,
            soft_state         = hs.soft_state,
            state_msg          = note,
            check_attempts     = hs.check_attempts,
            last_changed       = hs.last_changed,
            act_alarm_log_id   = hs.act_alarm_log_id,
            last_alarm_log_id  = hs.last_alarm_log_id,
            noalarm_flag       = hs.noalarm_flag,
            noalarm_from       = hs.noalarm_from,
            noalarm_to         = hs.noalarm_to
        WHERE host_service_id  = hsid;
    -- RAISE INFO '/ set_service_stat() = %', hs;
    -- Create log
    IF hs.host_service_state <> old_hs.host_service_state OR
       hs.hard_state         <> old_hs.hard_state OR
       hs.soft_state         <> old_hs.soft_state THEN
        INSERT INTO host_service_logs(host_service_id, old_state, old_soft_state, old_hard_state,
                           new_state, new_soft_state, new_hard_state, event_note, superior_alarm_id, noalarm,
                           alarm_id, alarm_do)
            VALUES  (hsid, old_hs.host_service_state, old_hs.soft_state, old_hs.hard_state,
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, sup, na = 'on',
                           alid, aldo);
    END IF;
    RETURN hs;
END
$$ LANGUAGE plpgsql;


