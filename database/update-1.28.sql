-- Hibajavítás
-- !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Figyelmeztetés !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
-- A következő két utasítás mellékhatása, hogy a features mező indexe a nodes, és az snmpdevices táblában nem lessz azonos.
-- A LanView2 API ezt nem tolerálja, ki kel menteni a teljes adatbázist, és visszatölteni, ezután a mező sorrendek helyreállnak.
ALTER TABLE patchs DROP COLUMN IF EXISTS features CASCADE;
ALTER TABLE nodes ADD COLUMN IF NOT EXISTS features text DEFAULT NULL;


ALTER TABLE service_var_types ALTER COLUMN service_var_type SET DEFAULT 'GAUGE';
UPDATE service_var_types SET service_var_type = DEFAULT WHERE service_var_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN service_var_type SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN raw_to_rrd SET DEFAULT false;
UPDATE service_var_types SET raw_to_rrd = DEFAULT WHERE raw_to_rrd IS NULL;
ALTER TABLE service_var_types ALTER COLUMN raw_to_rrd SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN plausibility_type SET DEFAULT 'no';
UPDATE service_var_types SET plausibility_type = DEFAULT WHERE plausibility_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN plausibility_type SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN warning_type SET DEFAULT 'no';
UPDATE service_var_types SET warning_type = DEFAULT WHERE warning_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN warning_type SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN critical_type SET DEFAULT 'no';
UPDATE service_var_types SET critical_type = DEFAULT WHERE critical_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN critical_type SET NOT NULL;

DROP TRIGGER IF EXISTS service_vars_check_value     ON public.service_rrd_vars;
DROP TRIGGER IF EXISTS service_rrd_vars_check_value ON public.service_rrd_vars;
DROP TRIGGER IF EXISTS service_vars_check_value     ON public.service_vars;

CREATE OR REPLACE FUNCTION check_before_service_value()
  RETURNS trigger AS
$BODY$
DECLARE
    tids record;
    pt   paramtype;
BEGIN
    IF TG_OP = 'INSERT' THEN
        IF 0 < COUNT(*) FROM service_vars WHERE service_var_id = NEW.service_var_id THEN
            PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_id', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF 0 < COUNT(*) FROM service_vars WHERE service_var_name = NEW.service_var_name AND host_service_id = NEW.host_service_id THEN
            PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_name', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
    ELSIF TG_OP = 'UPDATE' THEN
        IF OLD.service_var_id <> NEW.service_var_id THEN
            PERFORM error('Constant', OLD.service_var_id, 'service_var_id', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF OLD.service_var_name <> NEW.service_var_name THEN
            IF 0 < COUNT(*) FROM service_vars WHERE service_var_name = NEW.service_var_name AND host_service_id = NEW.host_service_id THEN
                PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_name', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
        END IF;
    ELSIF TG_OP = 'DELETE' THEN
        DELETE FROM alarm_service_vars WHERE service_var_id = OLD.service_var_id;
        RETURN OLD;
    END IF;
    SELECT param_type_id, raw_param_type_id INTO tids FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id;
    IF NEW.service_var_value IS NOT NULL THEN
        SELECT param_type_type INTO pt FROM param_types WHERE param_type_id = tids.param_type_id;
        NEW.service_var_value := check_paramtype(NEW.service_var_value, pt);
    END IF;
    IF NEW.raw_value IS NOT NULL THEN
        SELECT param_type_type INTO pt FROM param_types WHERE param_type_id = tids.raw_param_type_id;
        NEW.raw_value         := check_paramtype(NEW.raw_value, pt);
    END IF;
    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION check_before_service_value() OWNER TO lanview2;


CREATE TRIGGER service_vars_check_value     BEFORE INSERT OR UPDATE OR DELETE ON service_vars
    FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();
CREATE TRIGGER service_rrd_vars_check_value BEFORE INSERT OR UPDATE OR DELETE ON service_rrd_vars
    FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();

CREATE OR REPLACE FUNCTION service_rrd_value_after() RETURNS trigger AS
$BODY$
DECLARE
    val text;
    pt paramtype;
    payload text;
BEGIN
    IF NOT NEW.rrd_disabled AND NEW.raw_value IS NOT NULL AND NEW.last_time IS NOT NULL THEN -- Next value?
        IF OLD.last_time IS NULL OR (NEW.last_time - OLD.last_time) > '1 second'::interval THEN
            IF raw_to_rrd FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id THEN
                SELECT param_type_type INTO pt
                    FROM param_types       AS pt
                    JOIN service_var_types AS vt ON vt.raw_param_type_id = pt.param_type_id
                    WHERE NEW.service_var_type_id = vt.service_var_type_id;
                val := NEW.raw_value;
            ELSE
                SELECT param_type_type INTO pt
                    FROM param_types       AS pt
                    JOIN service_var_types AS vt USING(param_type_id)
                    WHERE NEW.service_var_type_id = vt.service_var_type_id;
                val := NEW.service_var_value;
            END IF;
            IF pt = 'integer' OR pt = 'real' OR pt = 'interval' THEN -- Numeric
                IF pt = 'interval' THEN
                    val := extract(EPOCH FROM val::interval)::text;
                END IF;
                payload := 'rrd '
                        || NEW.rrdhelper_id::text || ' '
                        || extract(EPOCH FROM NEW.last_time  AT TIME ZONE 'CETDST' AT TIME ZONE 'UTC')::bigint::text || ' '
                        || val || ' ' 
                        || NEW.service_var_id::text;
                PERFORM pg_notify('rrdhelper', payload);
            END IF;
        END IF;
    END IF;
    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION service_rrd_value_after() OWNER TO lanview2;

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

-- csak a felesleges INFO-k lettek kikommentezve.
CREATE OR REPLACE FUNCTION replace_dyn_addr_range(baddr inet, eaddr inet, hsid bigint DEFAULT NULL, excl boolean DEFAULT false, snid bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    brec dyn_addr_ranges;
    erec dyn_addr_ranges;
    bcol  boolean;
    ecol  boolean;
    r     reasons;
    snid2 bigint;
BEGIN
    IF snid IS NULL THEN 
        BEGIN       -- kezdő cím subnet-je
            SELECT subnet_id INTO STRICT snid FROM subnets WHERE netaddr >> baddr;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                    RETURN 'notfound';
                WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                    RETURN 'ambiguous';
        END;
        BEGIN       -- vég cím subnet-je
            SELECT subnet_id INTO STRICT snid2 FROM subnets WHERE netaddr >> eaddr;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                    RETURN 'notfound';
                WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                    RETURN 'ambiguous';
        END;
        IF snid <> snid2 THEN
            RETURN 'caveat';
        END IF;
    END IF;
    
    SELECT * INTO brec FROM dyn_addr_ranges WHERE begin_address <= baddr AND baddr <= end_address AND exclude = excl;
    bcol := FOUND;
    SELECT * INTO erec FROM dyn_addr_ranges WHERE begin_address <= eaddr AND eaddr <= end_address AND exclude = excl;
    ecol := FOUND;
    IF bcol AND ecol AND brec.dyn_addr_range_id = erec.dyn_addr_range_id AND baddr = brec.begin_address AND brec.end_address = eaddr THEN
        -- RAISE INFO '1: % < % ; % = %', baddr, eaddr, brec, erec; 
        UPDATE dyn_addr_ranges
            SET
                last_time = CURRENT_TIMESTAMP,
                host_service_id = hsid,
                flag = false
            WHERE dyn_addr_range_id = brec.dyn_addr_range_id;
        RETURN 'update';
    END IF;
    IF bcol AND ecol THEN
        -- RAISE INFO '2: % < % ; % <> %', baddr, eaddr, brec, erec;
        if brec.dyn_addr_range_id <> erec.dyn_addr_range_id THEN
            DELETE FROM dyn_addr_ranges WHERE dyn_addr_range_id = erec.dyn_addr_range_id;
            r := 'remove';
        ELSE
            r := 'modify';
        END IF;
        UPDATE dyn_addr_ranges
            SET
                begin_address = baddr,
                end_address = eaddr,
                subnet_id = snid,
                last_time = CURRENT_TIMESTAMP,
                host_service_id = hsid,
                flag = false
            WHERE dyn_addr_range_id = brec.dyn_addr_range_id;
        RETURN r;
    END IF;
    IF bcol OR ecol THEN
        if ecol THEN
            -- RAISE INFO '3: % < % ; ecol %', baddr, eaddr, erec;
            brec := erec;
        -- ELSE
            -- RAISE INFO '3: % < % ; bcol %', baddr, eaddr, brec;
        END IF;
        UPDATE dyn_addr_ranges
            SET
                begin_address = baddr,
                end_address = eaddr,
                subnet_id = snid,
                last_time = CURRENT_TIMESTAMP,
                host_service_id = hsid,
                flag = false
            WHERE dyn_addr_range_id = brec.dyn_addr_range_id;
        RETURN 'modify';
    END IF;
    -- RAISE INFO '4: % < % ', baddr, eaddr;
    INSERT INTO dyn_addr_ranges (begin_address, end_address, exclude, subnet_id, host_service_id) VALUES(baddr, eaddr, excl, snid, hsid);
    return 'insert';
END;
$$ LANGUAGE plpgsql;

-- Hibajavítás: 2020.06.08. Egy hibásan megadott fkey (öröklés miatt nem valódi fkey), a target_id-pedig kötelező.
ALTER TABLE imports DROP CONSTRAINT IF EXISTS imports_node_id_fkey;
CREATE TRIGGER imports_check_reference_node_id
  BEFORE INSERT OR UPDATE
  ON public.imports
  FOR EACH ROW
  EXECUTE PROCEDURE public.check_reference_node_id('nodes');
  
-- A paraméter rekordok mint esemény, egy idő adat hozzáadása 2020.08.29.
ALTER TABLE node_params ADD COLUMN IF NOT EXISTS date_of TIMESTAMP DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE port_params ADD COLUMN IF NOT EXISTS date_of TIMESTAMP DEFAULT CURRENT_TIMESTAMP;

-- !!!!!!!!!!!!!!!!!!!!!!!!!!! Nem biztos, hogy végleges !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
-- Grafikonok
-- régebbi probálkozás törlése
DROP TABLE IF EXISTS graphs, graph_vars CASCADE;

-- Új (probálkozás)

DROP TABLE IF EXISTS srv_diagram_types, srv_diagram_type_vars, service_diagrams;
DROP TYPE IF EXISTS srvdiagramtypetext, srvdiagramtypevartext;

ALTER TYPE tablefortext ADD VALUE IF NOT EXISTS 'service_diagram_types';
ALTER TYPE tablefortext ADD VALUE IF NOT EXISTS 'srv_diagram_type_vars';
CREATE TYPE srvdiagramtypetext AS ENUM ('title', 'vertical_label');
CREATE TYPE srvdiagramtypevartext AS ENUM ();

CREATE TABLE srv_diagram_types (
    srv_diagram_type_id         bigserial   PRIMARY KEY,
    srv_diagram_type_name       text        NOT NULL,
    srv_diagram_type_note       text        DEFAULT NULL,
    service_id                  bigint      NOT NULL
                REFERENCES services(service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    text_id                     bigint      NOT NULL DEFAULT nextval('text_id_sequ'),
    height                      integer     NOT NULL CHECK height >  50 AND height <= 2160,
    width                       integer     NOT NULL CHECK width  > 100 AND width  <= 2840,
    hrule                       text        NOT NULL DEFAULT '',
    features                    text        DEFAULT NULL,
    UNIQUE (srv_diagram_type_name, service_id)
);

CREATE TABLE srv_diagram_type_vars (
    srv_diagram_type_var_id     bigserial   PRIMARY KEY,
    srv_diagram_type_var_name   text        NOT NULL,
    srv_diagram_type_var_note   text        DEFAULT NULL,
    srv_diagram_type_id         bigint      NOT NULL
                REFERENCES srv_diagram_types(srv_diagram_type_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    service_var_name            text        NOT NULL,
    rpn                         text        DEFAULT NULL,
    features                    text        DEFAULT NULL,
    UNIQUE (srv_diagram_type_id, srv_diagram_type_var_name),
    UNIQUE (srv_diagram_type_id, service_var_name)
};

CREATE TABLE service_diagrams (
    service_diagram_id          bigserial   NOT NULL PRIMARY KEY,
    host_service_id             bigint      NOT NULL
                REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    srv_diagram_type_id         bigint      NOT NULL
                REFERENCES srv_diagram_types(srv_diagram_type_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    features                    text        DEFAULT NULL,
    UNIQUE (host_service_id, srv_diagram_type_id)
);
ALTER TABLE service_diagrams OWNER TO lanview2;
COMMENT ON TBALE service_diagrams IS 'Szolgáltatás lekérdezéshez rendelt diagram leírója, ill kapcsoló rekord';



-- ******************************
-- SELECT set_db_version(1, 28);
