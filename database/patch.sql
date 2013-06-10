-- 2011.10.02.

CREATE OR REPLACE RULE tpows_ins AS ON INSERT TO tpows DO INSTEAD
    INSERT INTO tpows_table( tpow_name,     tpow_descr,             dow,      begin_time,                      end_time)
                VALUES ( NEW.tpow_name, NEW.tpow_descr, dow2int(NEW.dow), NEW.begin_time, NEW.end_time)
        RETURNING tpow_id, tpow_name, tpow_descr, int2dow(dow) AS dow, begin_time, end_time;

CREATE OR REPLACE RULE tpows_upd AS ON UPDATE TO tpows DO INSTEAD
    UPDATE tpows_table SET
        tpow_id     = NEW.tpow_id,
        tpow_name   = NEW.tpow_name,
        tpow_descr  = NEW.tpow_descr,
        dow         = dow2int(NEW.dow),
        begin_time  = NEW.begin_time,
        end_time    = NEW.end_time
    WHERE tpow_id = OLD.tpow_id
    RETURNING tpow_id, tpow_name, tpow_descr, int2dow(dow) AS dow, begin_time, end_time;

-- 2011,10.03

ALTER TABLE alarms DROP CONSTRAINT alarms_host_service_id_fkey;
ALTER TABLE alarms
  ADD CONSTRAINT alarms_host_service_id_fkey FOREIGN KEY (host_service_id)
      REFERENCES host_services (host_service_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE CASCADE;

ALTER TABLE host_services DROP CONSTRAINT host_services_service_id_fkey;
ALTER TABLE host_services
  ADD CONSTRAINT host_services_service_id_fkey FOREIGN KEY (service_id)
      REFERENCES services (service_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE CASCADE;
ALTER TABLE host_services DROP CONSTRAINT host_services_timeperiod_id_fkey;
ALTER TABLE host_services
  ADD CONSTRAINT host_services_timeperiod_id_fkey FOREIGN KEY (timeperiod_id)
      REFERENCES timeperiods (timeperiod_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE SET DEFAULT;

ALTER TABLE host_service_logs DROP CONSTRAINT host_service_logs_host_service_id_fkey;
ALTER TABLE host_service_logs
  ADD CONSTRAINT host_service_logs_host_service_id_fkey FOREIGN KEY (host_service_id)
      REFERENCES host_services (host_service_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL;
ALTER TABLE host_service_logs
    ADD COLUMN service_name   varchar(32) NOT NULL DEFAULT '',
    ADD COLUMN node_name      varchar(32) NOT NULL DEFAULT '';

CREATE OR REPLACE FUNCTION set_service_stat(
    hsid        integer,                -- A host_services rekord id-je
    state       notifswitch,            -- Az új státusz
    note        varchar(255) DEFAULT '')-- Az eseményhez tartozó özenet (opcionális)
RETURNS host_services AS $$
DECLARE
    hs          host_services;
    old_hs      host_services;
    s           services;
    na          integer;
    sup         integer;
BEGIN
    SELECT * INTO hs FROM host_services WHERE host_service_id = hsid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hsid, 'host_service_id', 'set_service_stat()', 'host_services');
    END IF;
    SELECT * INTO  s FROM services      WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'set_service_stat()', 'services');
    END IF;
    IF state = hs.host_service_state AND state = hs.hard_state AND state = hs.soft_state THEN   -- Ha nincs változás
        RETURN hs;
    END IF;
    old_hs := hs;
    CASE state
        WHEN 'on' THEN
            hs.hard_state = 'on';
            hs.soft_state = 'on';
            hs.check_attempts := 0;
            IF hs.hard_state = 'on' THEN
                hs.host_service_state = 'on';
            ELSE
                hs.host_service_state = 'recovered';
            END IF;
        WHEN 'warning', 'unreachable','down','recovered','unknown','critical' THEN
            IF hs.hard_state <> 'on' THEN
                hs.hard_state = state;
                hs.soft_state = state;
                hs.host_service_state = state;
                hs.check_attempts := 0;
            ELSE
                IF hs.soft_state = 'on' THEN
                    hs.check_attempts := 1;
                ELSE
                    hs.check_attempts := hs.check_attempts + 1;
                END IF;
                hs.soft_state = state;
                IF hs.max_check_attempts IS NULL THEN
                    IF s.max_check_attempts IS NULL THEN
                        PERFORM error('DataError', hs.host_service_id, 'max_check_attempts', 'set_service_stat()');
                    END IF;
                    hs.max_check_attempts := hs.max_check_attempts;
                END IF;
                IF hs.check_attempts >= hs.max_check_attempts THEN
                    hs.hard_state := state;
                    hs.host_service_state := state;
                END IF;
            END IF;
        ELSE
            PERFORM error('Params', state, 'notifswitch', 'set_service_stat()');
    END CASE;
    IF chk_flapping(hsid, s) THEN
        hs.host_service_state := 'flapping';
    END IF;
    PERFORM set_host_status(hs);
    sup := check_superior_service(hs);
    IF sup IS NULL THEN
        sup := check_superior_host(hs);
    END IF;
    na := is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to);
    -- Update, and insert log record
    IF hs.last_changed IS NULL OR old_hs.host_service_state <> hs.host_service_state THEN
        hs.last_changed = CURRENT_TIMESTAMP;
    END IF;
    IF hs.host_service_state <> old_hs.host_service_state OR
       hs.hard_state         <> old_hs.hard_state OR
       hs.soft_state         <> old_hs.soft_state THEN
        INSERT INTO host_service_logs(host_service_id, old_state, old_soft_state, old_hard_state,
                           new_state, new_soft_state, new_hard_state, event_descr, superior_alarm, noalarm,
                           service_name, node_name)
            VALUES  (hsid, old_hs.host_service_state, old_hs.soft_state, old_hs.hard_state,
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, sup, na > 0,
                           s.service_name, (SELECT node_name FROM nodes WHERE node_id = hs.node_id));
    END IF;
    -- Alarm
    IF na = -1 THEN  -- Lejárt, töröljük a noalarm attributumokat, ne zavarjon
        hs.noalarm_flag := 'off';
        hs.noalarm_from := NULL;
        hs.noalarm_to   := NULL;
    END IF;
    CASE
        -- Riasztás lezárása
        WHEN hs.host_service_state = 'recovered' THEN
        RAISE INFO 'Close alarms record';
            IF hs.act_alarm_log_id IS NULL THEN
                PERFORM error('DataWarn', hsid, 'act_alarm_log_id', 'set_service_stat()', 'host_services');
            ELSE
                UPDATE alarms SET
                        end_tim = CURRENT_TIMESTAMP
                    WHERE alarm_id = hs.act_alarm_log_id AND end_time IS NULL;
                IF NOT FOUND THEN
                    PERFORM error('DataWarn', alarm_id, 'end_time', 'set_service_stat()', 'alarms');
                ELSE
                    hs.last_alarm_log_id := hs.last_alarm_log_id;
                END IF;
                hs.last_alarm_log_id := NULL;
            END IF;
        -- Új riasztás,
        WHEN (old_hs.host_service_state < 'warning' OR old_hs.host_service_state = 'unknown')
         AND hs.host_service_state >= 'warning' THEN
            RAISE INFO 'New alarms record';
            INSERT INTO alarms (host_service_id, first_status, max_status, last_status, event_descr, superior_alarm, noalarm)
                VALUES(hsid, hs.host_service_state, hs.host_service_state, hs.host_service_state, note, sup, na > 0 )
                RETURNING alarm_id INTO hs.act_alarm_log_id;
        -- A helyzet fokozódik
        WHEN old_hs.host_service_state < hs.host_service_state OR old_hs.host_service_state = 'unknown' THEN
        RAISE INFO 'Update alarms record Up';
            UPDATE alarms SET
                    max_status  = hs.host_service_state,
                    last_status = hs.host_service_state,
                    noalarm     = na > 0,
                    superior_alarm = sup
                WHERE alarm_id = hs.act_alarm_log_id;
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.host_service_state < hs.host_service_state THEN
            RAISE INFO 'Update alarms record Down';
            UPDATE alarms SET
                    last_status = hs.host_service_state,
                    noalarm     = na > 0,
                    superior_alarm = sup
                WHERE alarm_id = hs.act_alarm_log_id;
        ELSE
             RAISE INFO 'No mod alarms, old_hs = %, hs = %' ,old_hs, hs;
    END CASE;
    UPDATE host_services SET
            max_check_attempts = hs.max_check_attempts,
            host_service_state = hs.host_service_state,
            hard_state         = hs.hard_state,
            soft_state         = hs.soft_state,
            check_attempts     = hs.check_attempts,
            last_changed       = hs.last_changed,
            act_alarm_log_id   = hs.act_alarm_log_id,
            last_alarm_log_id  = hs.last_alarm_log_id,
            noalarm_flag       = hs.noalarm_flag,
            noalarm_from       = hs.noalarm_from,
            noalarm_to         = hs.noalarm_to
        WHERE host_service_id  = hsid;
    RAISE INFO '/ set_service_stat() = %', hs;
    RETURN hs;
END
$$ LANGUAGE plpgsql;
