
CREATE TYPE usability AS ENUM ('map', 'flag', 'icon');
ALTER TYPE imagetype ADD VALUE 'icon';      -- It has to be deleted

ALTER TABLE images ADD COLUMN usabilityes usability[] NOT NULL DEFAULT '{}'::usability[];

-- ------------------------------------------------------------------------------------------------
SELECT set_db_version(1, 19);

-- Bugfix 2019.01.08

-- ALTER TYPE image_type DROP VALUE 'JPEG', 'icon';
ALTER TABLE images ALTER COLUMN image_type SET DATA TYPE text USING image_type::text;
ALTER TABLE images ALTER COLUMN image_type DROP DEFAULT;
DROP TYPE imagetype;
CREATE TYPE imagetype AS ENUM ('BMP', 'GIF', 'JPG', 'PNG', 'PBM', 'PGM', 'PPM', 'XBM', 'XPM', 'BIN');
ALTER TABLE images ALTER COLUMN image_type SET DATA TYPE imagetype USING
    CASE image_type
	WHEN 'BMP'  THEN 'BMP'::imagetype
	WHEN 'GIF'  THEN 'GIF'::imagetype
	WHEN 'JPG'  THEN 'JPG'::imagetype
	WHEN 'JPEG' THEN 'JPG'::imagetype
	WHEN 'PNG'  THEN 'PNG'::imagetype
	WHEN 'PBM'  THEN 'PBM'::imagetype
	WHEN 'PGM'  THEN 'PGM'::imagetype
	WHEN 'PPM'  THEN 'PPM'::imagetype
	WHEN 'XBM'  THEN 'XBM'::imagetype
	WHEN 'XPM'  THEN 'XPM'::imagetype
	ELSE             'BIN'::imagetype
    END;

-- ALTER TYPE usability DROP VALUE 'icon';
ALTER TABLE images ALTER COLUMN usabilityes SET DATA TYPE text USING usabilityes::text[];
ALTER TABLE images ALTER COLUMN usabilityes DROP DEFAULT;
DROP TYPE usability;
CREATE TYPE usability AS ENUM ('map', 'flag');
UPDATE images SET usabilityes = array_remove(usabilityes::text[], 'icon'::text);
ALTER TABLE images ALTER COLUMN usabilityes SET DATA TYPE usability[] USING usabilityes::usability[];
ALTER TABLE images ALTER COLUMN usabilityes SET DEFAULT '{}'::usability[];

-- Bugfix 2019.01.10.
-- drop noalarm from alarms table
-- 2019.01.19. commented RAISE INFO ...

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
    supaid      bigint;         -- Superior host_service act alarm_id
    tflapp      interval;       -- Flapping detection time window
    iflapp      integer;        -- Changes in the state within the time window
    alid        bigint;         -- Actual alarm record ID
    mca         integer;        -- max_check_attempts
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
    -- set new record states
    IF state < 'warning'::notifswitch THEN   -- ok:_ 'on' or 'recovered'
        state := 'on'::notifswitch;  -- 'on' or 'recovered' -> 'on'
        IF old_hs.hard_state >= 'warning'::notifswitch THEN
            hs.host_service_state := 'recovered';
        ELSE
            hs.host_service_state := 'on'::notifswitch;
        END IF;
        hs.hard_state := 'on'::notifswitch;
        hs.soft_state := 'on'::notifswitch;
        hs.check_attempts := 0;
    ELSE                        -- not ok
        hs.soft_state := state;
        IF old_hs.hard_state >= 'warning'::notifswitch THEN   -- So far it was bad
            hs.hard_state := state;
            hs.host_service_state := state;
            hs.check_attempts := 0;
        ELSE
            IF old_hs.soft_state = 'on'::notifswitch THEN    -- Is this the first mistake?
                hs.check_attempts := 1;     -- Yes. ￼Let's start counting
            ELSE
                hs.check_attempts := hs.check_attempts + 1; -- No. We continue to count
            END IF;
            mca := COALESCE(hs.max_check_attempts, s.max_check_attempts, 1);
            IF forced OR hs.check_attempts >= mca THEN  -- It is definitely a problem
                hs.hard_state := state;
                hs.host_service_state := state;
            END IF;
        END IF;
    END IF;
    -- flapping
    tflapp := COALESCE(hs.flapping_interval, s.flapping_interval);
    iflapp := COALESCE(hs.flapping_max_change, s.flapping_max_change);
    IF chk_flapping(hsid, tflapp, iflapp) THEN
        hs.host_service_state := 'flapping'::notifswitch;
        state := 'flapping'::notifswitch;
    END IF;
    -- delegate status to node
    PERFORM set_host_status(hs);
    supaid := check_superior_service(hs);
    -- Disable alarms status?
    na := is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to);
    IF na = 'expired'::isnoalarm THEN  -- If it has expired, it will be deleted
        hs.noalarm_flag := 'off'::noalarmtype;
        hs.noalarm_from := NULL;
        hs.noalarm_to   := NULL;
        na := 'off'::isnoalarm;
    END IF;
    -- last changed time
    IF hs.last_changed IS NULL OR old_hs.host_service_state <> hs.host_service_state THEN
        hs.last_changed = CURRENT_TIMESTAMP;
    END IF;
    -- Alarm ...
    alid := old_hs.act_alarm_log_id;
    -- RAISE INFO 'act_alarm_log_id = %', alid;
    IF state = 'on'::notifswitch THEN        -- ok
        IF alid IS NOT NULL THEN   -- close act alarm
            -- RAISE INFO 'Close % alarms record for % service.', alid, hs.host_service_id;
            UPDATE alarms SET end_time = CURRENT_TIMESTAMP WHERE alarm_id = alid;
            hs.act_alarm_log_id := NULL;
            aldo := 'close'::reasons;
        ELSE
            IF hs.host_service_state = old_hs.host_service_state THEN
                aldo := 'unchange'::reasons;
            ELSE
                aldo := 'modify'::reasons;
            END IF;
        END IF;
    ELSE                        -- not ok
        IF alid IS NULL THEN    -- create new alarm
            -- RAISE INFO 'New (%) alarms record for % service.', na, hs.host_service_id;
            IF na = 'off'::isnoalarm AND NOT s.disabled AND NOT hs.disabled THEN -- Alarm is not disabled, and services is not disabled
                INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id)
                    VALUES(hsid, dmid, state, state, state, note, supaid )
                    RETURNING alarm_id INTO hs.act_alarm_log_id;
                aldo := 'new'::reasons;
                alid := hs.act_alarm_log_id;
                hs.last_alarm_log_id := alid;
                -- RAISE INFO 'New alarm_id = %', alid;
            ELSE
                -- RAISE INFO 'New alarm discard';
                aldo := 'discard'::reasons;
                DELETE FROM alarms WHERE alarm_id = alid;
            END IF;
        ELSE                    -- The alarm remains
            IF na = 'off'::isnoalarm AND NOT s.disabled AND NOT hs.disabled THEN -- Alarm is not disabled, and services is not disabled
                aldo := 'remove'::reasons;
                DELETE FROM alarms WHERE alarm_id = alid;
                -- RAISE INFO 'Disable alarm, remove (alarm_id = %)', alid;
                alid := NULL;
                hs.last_alarm_log_id := NULL;
            ELSIF old_hs.host_service_state < state THEN
                UPDATE alarms SET
                        max_status  = greatest(hs.host_service_state, max_status),
                        last_status = hs.host_service_state,
                        superior_alarm_id = supaid
                    WHERE alarm_id = alid;
                aldo := 'modify'::reasons;
                -- RAISE INFO 'Update alarm (alarm_id = %', alid;
            ELSIF old_hs.host_service_state > state THEN
                UPDATE alarms SET
                        last_status = hs.host_service_state,
                        superior_alarm_id = supaid
                    WHERE alarm_id = alid;
                aldo := 'modify'::reasons;
                -- RAISE INFO 'Update alarm (2) (alarm_id = %', alid;
            ELSE
                aldo := 'unchange'::reasons;
                -- RAISE INFO 'Unchange alarm (alarm_id = %', alid;
            END IF;
        END IF;
    END IF;
    -- save record
    UPDATE host_services SET
            max_check_attempts = hs.max_check_attempts,
            host_service_state = hs.host_service_state,
            hard_state         = hs.hard_state,
            soft_state         = hs.soft_state,
            state_msg          = note,
            check_attempts     = hs.check_attempts,
            last_changed       = hs.last_changed,
            act_alarm_log_id   = alid,
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
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, supaid, na = 'on',
                           alid, aldo);
    END IF;
    RETURN hs;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE VIEW public.online_alarms_noack AS 
 WITH a AS (
         SELECT a_1.alarm_id,
            a_1.host_service_id,
            a_1.daemon_id,
            a_1.first_status,
            a_1.max_status,
            a_1.last_status,
            a_1.begin_time,
            a_1.event_note,
            a_1.superior_alarm_id,
            a_1.end_time,
            ARRAY( SELECT user_events.user_id
                   FROM user_events
                  WHERE user_events.alarm_id = a_1.alarm_id AND user_events.event_type = 'notice'::usereventtype AND user_events.event_state <> 'dropped'::usereventstate) AS online_user_ids
           FROM alarms a_1
          WHERE 0 = (( SELECT count(*) AS count
                   FROM user_events
                  WHERE user_events.alarm_id = a_1.alarm_id AND user_events.event_type = 'acknowledge'::usereventtype)) AND COALESCE((a_1.end_time + ((( SELECT sys_params.param_value
                   FROM sys_params
                  WHERE sys_params.sys_param_name = 'user_notice_timeout'::text))::interval)) > now(), true)
        )
 SELECT a.alarm_id AS online_alarm_id,
    a.host_service_id,
    host_service_id2name(a.host_service_id) AS host_service_name,
    n.node_name,
    p.place_name,
    n.place_id,
    a.superior_alarm_id,
    a.begin_time,
    a.end_time,
    a.first_status,
    a.max_status,
    a.last_status,
    alarm_message(a.host_service_id, a.max_status) AS msg,
    a.online_user_ids,
    ( SELECT array_agg(user_events.user_id) AS array_agg
           FROM user_events
          WHERE user_events.alarm_id = a.alarm_id AND user_events.event_type = 'notice'::usereventtype AND user_events.event_state = 'happened'::usereventstate) AS notice_user_ids,
    ( SELECT array_agg(user_events.user_id) AS array_agg
           FROM user_events
          WHERE user_events.alarm_id = a.alarm_id AND user_events.event_type = 'view'::usereventtype) AS view_user_ids
   FROM a
     JOIN host_services h USING (host_service_id)
     JOIN services s USING (service_id)
     JOIN nodes n USING (node_id)
     JOIN places p USING (place_id)
  WHERE 0 < array_length(a.online_user_ids, 1);


CREATE OR REPLACE FUNCTION alarm_notice() RETURNS TRIGGER AS $$
DECLARE
    gids bigint[];
BEGIN
    IF NEW.superior_alarm_id IS NULL THEN
        IF TG_OP = 'INSERT' THEN
            UPDATE user_events SET event_state = 'dropped'
                WHERE alarm_id IN ( SELECT alarm_id FROM alarms WHERE host_service_id = NEW.host_service_id)
                  AND event_state = 'necessary';
            SELECT COALESCE(online_group_ids, (SELECT online_group_ids FROM services WHERE service_id = host_services.service_id))
                    INTO gids FROM host_services WHERE host_service_id = NEW.host_service_id;
            IF gids IS NOT NULL THEN
                INSERT INTO user_events(user_id, alarm_id, event_type)
                    SELECT DISTINCT user_id, NEW.alarm_id, 'notice'::usereventtype   FROM group_users WHERE group_id = ANY (gids);
            END IF;
            SELECT COALESCE(offline_group_ids, (SELECT offline_group_ids FROM services WHERE service_id = host_services.service_id))
                    INTO gids FROM host_services WHERE host_service_id = NEW.host_service_id;
            IF gids IS NOT NULL THEN
                INSERT INTO user_events(user_id, alarm_id, event_type)
                    SELECT DISTINCT user_id, NEW.alarm_id, 'sendmail'::usereventtype FROM group_users WHERE group_id = ANY (gids);
            END IF;
        END IF;
        NOTIFY alarm;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION ticket_alarm(
    lst notifswitch,        -- stat (last_state)
    msg text,               -- stat message
    did bigint DEFAULT NULL,-- daemon ID
    aid bigint DEFAULT NULL,-- superior alarm ID
    fst notifswitch DEFAULT NULL,-- first stat
    mst notifswitch DEFAULT NULL -- max stat
) RETURNS alarms AS $$
DECLARE
    ar alarms;
    hs host_services;
    repi interval;
    sid bigint := 0;
BEGIN
    SELECT * INTO hs FROM host_services WHERE host_service_id = sid;
    IF NOT FOUND THEN
        IF 0 = COUNT(*) FROM nodes WHERE node_id = sid THEN
            INSERT INTO nodes(node_id, node_name, node_note,    node_type )
               VALUES        (sid,     'nil',   'Independent', '{node, virtual}');
        END IF;
        IF 0 = COUNT(*) FROM services WHERE service_id = sid THEN
            INSERT INTO services (service_id, service_name, service_note, disabled, service_type_id)
                 VALUES          (  sid,      'ticket',     'Hiba jegy',  true,     sid );
        END IF;
        INSERT INTO host_services (host_service_id, node_id, service_id,  host_service_note, disabled)
             VALUES               (  sid,           sid,     sid,         'Hiba jegy',       true)
             RETURNING * INTO hs;
    END IF;
    IF 'on' = is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to) THEN
        repi := COALESCE(get_interval_sys_param('ticet_reapeat_time'), '14 days'::interval);
        SELECT * INTO ar FROM alarms
                    WHERE host_service_id = sid
                    AND (begin_time + repi) > NOW()
                    AND end_time IS NULL
                    AND lst = last_status
                    AND COALESCE(aid, -1) = COALESCE(superior_alarm_id, -1)
                    AND COALESCE(did, -1) = COALESCE(daemon_id, -1)
                    AND msg = event_note
                    LIMIT 1;
        IF NOT FOUND THEN
            INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id)
                        VALUES (sid,             did, COALESCE(fst, lst), COALESCE(mst, lst), lst, msg,        aid)
                    RETURNING * INTO ar;
        END IF;
    END IF;
    RETURN ar;
END;
$$ LANGUAGE plpgsql;


ALTER TABLE alarms DROP COLUMN noalarm;

-- Modify table_shape_fields !!!!!!

ALTER TABLE table_shape_fields ADD COLUMN icon text DEFAULT NULL;
COMMENT ON COLUMN table_shape_fields.icon IS 'Column header icon name';
ALTER TABLE table_shape_fields DROP COLUMN  expression; -- unused (instead: 'view.func' or 'view.expr' in feature)

-- Raring the query of service variables
ALTER TABLE service_vars ADD COLUMN rarefaction integer DEFAULT 1;

-- Mod. 2019.01.19.

ALTER TABLE nodes ADD COLUMN os_name text;
COMMENT ON COLUMN nodes.os_name IS 'Operating system or firmware name';
ALTER TABLE nodes ADD COLUMN os_version text;
COMMENT ON COLUMN nodes.os_version IS 'Operating system or firmware version';

