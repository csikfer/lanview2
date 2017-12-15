
CREATE TABLE disabled_alarms (
  CONSTRAINT disabled_alarms_pkey PRIMARY KEY (alarm_id),
  CONSTRAINT disabled_alarms_daemon_id_fkey FOREIGN KEY (daemon_id)
      REFERENCES public.host_services (host_service_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL,
  CONSTRAINT disabled_alarms_host_service_id_fkey FOREIGN KEY (host_service_id)
      REFERENCES public.host_services (host_service_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE CASCADE,
  CONSTRAINT disabled_alarms_superior_alarm_id_fkey FOREIGN KEY (superior_alarm_id)
      REFERENCES public.alarms (alarm_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL,
  CHECK (noalarm) NO INHERIT
) INHERITS (alarms);

CREATE INDEX host_services_node_id               ON host_services     USING btree (node_id);
CREATE INDEX host_services_service_id            ON host_services     USING btree (service_id);
CREATE INDEX host_services_last_touched          ON host_services     USING btree (last_touched);
CREATE INDEX host_services_act_alarm_log_id      ON host_services     USING btree (act_alarm_log_id);
CREATE INDEX host_services_last_alarm_log_id     ON host_services     USING btree (last_alarm_log_id);
CREATE INDEX host_service_logs_alarm_id          ON host_service_logs USING btree (alarm_id);
CREATE INDEX host_service_logs_superior_alarm_id ON host_service_logs USING btree (superior_alarm_id);

DELETE FROM alarms WHERE noalarm;
ALTER TABLE ONLY alarms ADD CONSTRAINT noalarm_is_off CHECK(NOT noalarm) NO INHERIT;

CREATE INDEX disabled_alarms_begin_time_index ON disabled_alarms USING btree (begin_time);
CREATE INDEX disabled_alarms_end_time_index   ON disabled_alarms USING btree (end_time);
CREATE INDEX disabled_alarms_host_service_id  ON disabled_alarms USING btree (host_service_id);

-- Drop fkeys : --> alarms ==> (unsupported fkey) alarms + disabled_alarms
ALTER TABLE host_services     DROP CONSTRAINT host_services_act_alarm_log_id_fkey;
ALTER TABLE host_services     DROP CONSTRAINT host_services_last_alarm_log_id_fkey;
ALTER TABLE host_service_logs DROP CONSTRAINT host_service_logs_superior_alarm_id_fkey;
ALTER TABLE host_service_logs DROP CONSTRAINT host_service_logs_alarm_id_fkey;

INSERT INTO unusual_fkeys
  ( table_name,      column_name,     unusual_fkeys_type, f_table_name, f_column_name, f_inherited_tables) VALUES
  ( 'host_services', 'act_alarm_log_id',  'property',     'alarms',     'alarm_id',    '{alarms, disabled_alarms}'),
  ( 'host_services', 'last_alarm_log_id', 'property',     'alarms',     'alarm_id',    '{alarms, disabled_alarms}'),
  ( 'host_service_logs','superior_alarm_id','property',   'alarms',     'alarm_id',    '{alarms, disabled_alarms}'),
  ( 'host_service_logs', 'alarm_id',      'property',     'alarms',     'alarm_id',    '{alarms, disabled_alarms}');

CREATE OR REPLACE FUNCTION check_unique_alarm_id() RETURNS trigger AS $$
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.alarm_id <> OLD.alarm_id THEN
            PERFORM error('Constant', -1, 'alarm_id', 'check_unique_alarm_id()', TG_TABLE_NAME, TG_OP);
        END IF;
    ELSE
        IF 0 < COUNT(*) FROM alarms WHERE alarm_id = NEW.alarm_id THEN
            PERFORM error('IdNotUni', NEW.alarm_id, 'alarm_id', 'check_unique_alarm_id()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION check_alarm_id_on_host_services() RETURNS trigger AS $$
BEGIN
    IF NEW.act_alarm_log_id IS NOT NULL AND 0 = COUNT(*) FROM alarms WHERE NEW.act_alarm_log_id = alarm_id THEN
        PERFORM error('InvRef', NEW.act_alarm_log_id, 'act_alarm_log_id', 'check_alarm_id_on_host_services()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF NEW.last_alarm_log_id IS NOT NULL AND 0 = COUNT(*) FROM alarms WHERE NEW.last_alarm_log_id = alarm_id THEN
        PERFORM error('InvRef', NEW.last_alarm_log_id, 'last_alarm_log_id', 'check_alarm_id_on_host_services()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION check_alarm_id_on_host_service_logs() RETURNS trigger AS $$
BEGIN
    IF NEW.alarm_id IS NOT NULL AND 0 = COUNT(*) FROM alarms WHERE NEW.alarm_id = alarm_id THEN
        PERFORM error('InvRef', NEW.alarm_id, 'alarm_id', 'check_alarm_id_on_host_service_logs()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF NEW.superior_alarm_id IS NOT NULL AND 0 = COUNT(*) FROM alarms WHERE NEW.superior_alarm_id = alarm_id THEN
        PERFORM error('InvRef', NEW.superior_alarm_id, 'superior_alarm_id', 'check_alarm_id_on_host_service_logs()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_alarms() RETURNS TRIGGER AS $$
BEGIN
    UPDATE host_services     SET act_alarm_log_id  = NULL WHERE act_alarm_log_id  = OLD.alarm_id;
    UPDATE host_services     SET last_alarm_log_id = NULL WHERE last_alarm_log_id = OLD.alarm_id;
    UPDATE host_service_logs SET alarm_id          = NULL WHERE alarm_id          = OLD.alarm_id;
    UPDATE host_service_logs SET superior_alarm_id = NULL WHERE superior_alarm_id = OLD.alarm_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION truncate_disabled_alarms() RETURNS TRIGGER AS $$
BEGIN
    UPDATE host_services     SET act_alarm_log_id  = NULL WHERE act_alarm_log_id  IN (SELECT alarm_id FROM disabled_alarms);
    UPDATE host_services     SET last_alarm_log_id = NULL WHERE last_alarm_log_id IN (SELECT alarm_id FROM disabled_alarms);
    UPDATE host_service_logs SET alarm_id          = NULL WHERE alarm_id          IN (SELECT alarm_id FROM disabled_alarms);
    UPDATE host_service_logs SET superior_alarm_id = NULL WHERE superior_alarm_id IN (SELECT alarm_id FROM disabled_alarms);
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER alarm_check_unique_alarm_id           BEFORE INSERT OR UPDATE ON alarms          FOR EACH ROW EXECUTE PROCEDURE check_unique_alarm_id();
CREATE TRIGGER disabled_alarm_check_unique_alarm_id  BEFORE INSERT OR UPDATE ON disabled_alarms FOR EACH ROW EXECUTE PROCEDURE check_unique_alarm_id();
CREATE TRIGGER alarm_delete_alarms                   AFTER DELETE ON alarms                     FOR EACH ROW EXECUTE PROCEDURE delete_alarms();
CREATE TRIGGER disabled_alarm_delete_alarms          AFTER DELETE ON disabled_alarms            FOR EACH ROW EXECUTE PROCEDURE delete_alarms();
CREATE TRIGGER disabled_alarm_truncate_alarms        BEFORE TRUNCATE ON disabled_alarms         FOR STATEMENT EXECUTE PROCEDURE truncate_disabled_alarms();
CREATE TRIGGER hs_check_alarm_id_on_host_services    BEFORE INSERT OR UPDATE ON host_services   FOR EACH ROW EXECUTE PROCEDURE check_alarm_id_on_host_services();
CREATE TRIGGER hs_check_alarm_id_on_host_service_logs BEFORE INSERT OR UPDATE ON host_service_logs FOR EACH ROW EXECUTE PROCEDURE check_alarm_id_on_host_service_logs();

CREATE OR REPLACE FUNCTION set_service_stat(
    hsid        bigint,             -- A host_services rekord id-je
    state       notifswitch,        -- Az új státusz
    note        text DEFAULT '',    -- Az eseményhez tartozó üzenet (opcionális)
    dmid        bigint DEFAULT NULL,-- Daemon host_service_id
    forced      boolean DEFAULT false)  -- Hiba esetén azonnali statusz állítás (nem számolgat)
RETURNS host_services AS $$
DECLARE
    hs          host_services;  -- Az új host_services rekord
    old_hs      host_services;  -- A régi
    s           services;       -- A hozzá tartozó services rekord
    na          isnoalarm;      -- Alarm tiltás állpota
    sup         bigint;
    tflapp      interval;       -- Flapping figyelés időablakának a hossza
    iflapp      integer;        -- Az isőablakon bellüli státuszváltozások száma
    alid        bigint;
    aldo        reasons := 'unknown';
BEGIN
    hs := touch_host_service(hsid);
    SELECT * INTO  s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'set_service_stat()', 'services');
    END IF;
    IF state = hs.host_service_state AND state = hs.hard_state AND state = hs.soft_state THEN   -- Ha nincs változás
        RETURN hs;
    END IF;
    old_hs := hs;
    alid := old_hs.act_alarm_log_id;
    CASE state
        WHEN 'on','recovered' THEN
            hs.host_service_state := state;
            hs.hard_state := 'on';
            hs.soft_state := 'on';
            hs.check_attempts := 0;
        WHEN 'warning', 'unreachable','down','unknown','critical' THEN
            IF hs.hard_state <> 'on' THEN   -- nem most rommlott el
                hs.hard_state := state;
                hs.soft_state := state;
                hs.host_service_state := state;
                hs.check_attempts := 0;
            ELSE                            -- most vagy nem rég romlott el, hihető?
                IF hs.soft_state = 'on' THEN
                    hs.check_attempts := 1; -- pont most lett rossz, kezdünk számolni
                ELSE
                    hs.check_attempts := hs.check_attempts + 1; -- tovább számolunk
                END IF;
                hs.soft_state := state;
                IF hs.max_check_attempts IS NULL THEN
                    IF s.max_check_attempts IS NULL THEN
                        PERFORM error('WNotFound', hs.host_service_id, 'max_check_attempts', 'set_service_stat()');
                        hs.max_check_attempts := 1;
                    ELSE
                        hs.max_check_attempts := s.max_check_attempts;
                    END IF;
                END IF;
                IF forced OR hs.check_attempts >= hs.max_check_attempts THEN  -- Elhisszük, hogy baj van
                    hs.hard_state := state;
                    hs.host_service_state := state;
                    hs.check_attempts := 0;
                END IF;
            END IF;
        ELSE
            PERFORM error('Params', state, 'notifswitch', 'set_service_stat()');    -- kilép!
    END CASE;
    tflapp := COALESCE(hs.flapping_interval, s.flapping_interval);
    iflapp := COALESCE(hs.flapping_max_change, s.flapping_max_change);
    IF chk_flapping(hsid, tflapp, iflapp) THEN
        hs.host_service_state := 'flapping';
    END IF;
    PERFORM set_host_status(hs);
    sup := check_superior_service(hs);
    na := is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to);
    IF na = 'expired' THEN  -- Lejárt, töröljük a noalarm attributumokat, ne zavarjon
        hs.noalarm_flag := 'off';
        hs.noalarm_from := NULL;
        hs.noalarm_to   := NULL;
        na := 'off';
    END IF;
    IF hs.last_changed IS NULL OR old_hs.host_service_state <> hs.host_service_state THEN
        hs.last_changed = CURRENT_TIMESTAMP;
    END IF;
    -- Alarm
    CASE
        -- Riasztás lezárása
        WHEN hs.host_service_state = 'recovered' THEN
            IF hs.act_alarm_log_id IS NULL THEN
                -- PERFORM error('DataWarn', hsid, 'act_alarm_log_id', 'set_service_stat()', 'host_services');
                RAISE INFO 'Status is recovered and alarm record not set.';
            ELSE
                RAISE INFO 'Close alarms record';
                UPDATE alarms SET
                        end_time = CURRENT_TIMESTAMP
                    WHERE alarm_id = hs.act_alarm_log_id AND end_time IS NULL;
                IF NOT FOUND THEN
                    PERFORM error('DataWarn', hs.act_alarm_log_id, 'end_time', 'set_service_stat()', 'alarms');
                ELSE
                    hs.last_alarm_log_id := hs.last_alarm_log_id;
                END IF;
                hs.act_alarm_log_id := NULL;
            END IF;
            aldo := 'close';
        -- Új riasztás,
        WHEN (old_hs.host_service_state < 'warning' OR old_hs.host_service_state = 'unknown')
         AND hs.host_service_state >= 'warning' THEN
            RAISE INFO 'New alarms record';
            IF na = 'on' THEN 
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
        -- A helyzet fokozódik
        WHEN old_hs.host_service_state < hs.host_service_state OR old_hs.host_service_state = 'unknown' THEN
        RAISE INFO 'Update alarms record Up';
            UPDATE alarms SET
                    max_status  = hs.host_service_state,
                    last_status = hs.host_service_state,
                    superior_alarm_id = sup
                WHERE alarm_id = hs.act_alarm_log_id;
            aldo := 'modify';
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.host_service_state < hs.host_service_state THEN
            RAISE INFO 'Update alarms record Down';
            UPDATE alarms SET
                    last_status = hs.host_service_state,
                    superior_alarm_id = sup
                WHERE alarm_id = hs.act_alarm_log_id;
            aldo := 'modify';
        ELSE
            RAISE INFO 'No mod alarms, old_hs = %, hs = %' ,old_hs, hs;
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