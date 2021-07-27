-- Hibajavítás
-- ha nem volt állapot változás, akkor nem mentette a state_msg mezőt

CREATE OR REPLACE FUNCTION set_service_stat(
    hsid   bigint,                      -- host_service_id
    state  notifswitch,                 -- state
    note   text DEFAULT ''::text,       -- note, state message
    dmid   bigint DEFAULT NULL::bigint, -- daemon host_service_id
    forced boolean DEFAULT false)
  RETURNS host_services AS
$BODY$
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
    hs := touch_host_service(hsid);     -- update last_touched field, and fetch record
    SELECT * INTO s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'set_service_stat()', 'services');
    END IF;
    IF state = hs.host_service_state AND state = hs.hard_state AND state = hs.soft_state THEN
        IF hs.state_msg <> note THEN
    UPDATE host_services SET state_msg = note WHERE host_service_id  = hsid;
        END IF;
        RETURN hs;  -- No change status
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
            IF old_hs.soft_state = 'on'::notifswitch THEN   -- Is this the first mistake?
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
    tflapp := COALESCE(hs.flapping_interval,   s.flapping_interval);
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
    IF hs.hard_state < 'warning'::notifswitch THEN        -- new state is good
        IF alid IS NOT NULL THEN   -- close act alarm
            -- RAISE INFO 'Close % alarms record for % service.', alid, hs.host_service_id;
            UPDATE alarms SET end_time = CURRENT_TIMESTAMP WHERE alarm_id = alid;
            hs.last_alarm_log_id := alid;
            hs.act_alarm_log_id := NULL;
            aldo := 'close'::reasons;
        ELSE
            IF hs.host_service_state = old_hs.host_service_state THEN
                aldo := 'unchange'::reasons;
            ELSE
                aldo := 'modify'::reasons;
            END IF;
        END IF;
    ELSE                        -- new status is not good
        IF alid IS NULL THEN    -- create new alarm
            -- RAISE INFO 'New (%) alarms record for % service.', na, hs.host_service_id;
            IF na = 'off'::isnoalarm THEN -- If alarm is not disabled, then create
                INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id)
                    VALUES(hsid, dmid, state, state, state, note, supaid )
                    RETURNING alarm_id INTO alid;
                aldo := 'new'::reasons;
                hs.act_alarm_log_id := alid;
                -- RAISE INFO 'New alarm_id = %', alid;
            ELSE
                -- RAISE INFO 'New alarm discard';
                aldo := 'discard'::reasons;
            END IF;
        ELSE                    -- new state is not good and there was an alarm
            IF na = 'on'::isnoalarm THEN -- Alarm is disabled
                aldo := 'remove'::reasons;
                DELETE FROM alarms WHERE alarm_id = alid;
                -- RAISE INFO 'Disable alarm, remove (alarm_id = %)', alid;
                hs.act_alarm_log_id := NULL;
                IF alid = hs.last_alarm_log_id THEN
                    hs.last_alarm_log_id := NULL;
                END If;
                alid := NULL;
            ELSIF old_hs.host_service_state <> state THEN
                UPDATE alarms SET
                        max_status  = greatest(hs.host_service_state, max_status),
                        last_status = hs.host_service_state,
                        superior_alarm_id = supaid
                    WHERE alarm_id = alid;
                aldo := 'modify'::reasons;
                -- RAISE INFO 'Update alarm (alarm_id = %', alid;
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
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, supaid, na = 'on',
                           alid, aldo);
    END IF;
    RETURN hs;
END
$BODY$
  LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION set_service_stat(bigint, notifswitch, text, bigint, boolean) OWNER TO lanview2;

ALTER TYPE fieldflag ADD VALUE IF NOT EXISTS 'html_text' AFTER 'huge';
COMMENT ON TYPE public.fieldflag
    IS 'Field flags:
table_hide      The field is hidden in the table.
dialog_hide     The field is hidden in the dialog.
read_only       The field is read only.
passwd          The field is password.
huge            Long text.
html_text       Long HTML text
batch_edit      The field modify Batch.
bg_color        Set background color by enum_vals record.
fg_color        Set character color by enum_vals record.
font            Set font by enum_vals record.
tool_tip        Show Tool tip text by enum_vals record.
HTML            This field is visible in the HTML report.
raw             Show raw value
image           Display an image or icon
notext          Hide text, but only if the image flag is exists.';

