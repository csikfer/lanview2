
-- Az node2snmpdevice(bigint nid) előkészítése:
-- Ha ideiglenesen töröljük a nodes rekordot, akkor megmaradjanak a tulajdonában lévő rekordok.
DROP TRIGGER IF EXISTS node_param_value_check_reference_node_id ON public.node_params;
DROP TRIGGER IF EXISTS nports_check_reference_node_id           ON public.nports;
DROP TRIGGER IF EXISTS interfaces_check_reference_node_id       ON public.interfaces;

-- Töröljük a felesleges paramétereket, és ellenörzéseket IS.
CREATE OR REPLACE FUNCTION public.check_reference_node_id() RETURNS trigger AS
$BODY$
    ($inTable) = @{$_TD->{args}};
    if ($_TD->{new}{node_id} ==   $_TD->{old}{node_id}) { return; } # No change, ok
    if ($_TD->{new}{node_id} == - $_TD->{old}{node_id}) { return; } # Temporary deletion of owner, no check
    $rv = spi_exec_query('SELECT COUNT(*) FROM ' . $inTable .' WHERE node_id = ' . $_TD->{new}{node_id});
    $nn  = $rv->{rows}[0]->{count};
    if ($nn == 1) { return; }
    if ($nn == 0) {
        spi_exec_query("SELECT error('InvRef', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
    }
    else {
        spi_exec_query("SELECT error('DataError', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
    }
    return "SKIP";
$BODY$
  LANGUAGE plperl VOLATILE
  COST 100;
ALTER FUNCTION public.check_reference_node_id()
  OWNER TO lanview2;
COMMENT ON FUNCTION public.check_reference_node_id() IS
'Ellenőrzi, hogy a node_id mező valóban egy node rekordra mutat-e.
Ha a node_id érték nem változik, akkor nincs ellenörzés.
Ha a node_id előjelet vált, akkor sincs ellenörzés, a node_id mindíg pozitív szám, a negatív értékekkel az a speciális eset van jelezve,
 amikor a nodes rekordot snmpdevices rekordra vagy fordítva konvertáljuk, ebben az esetben a rekord ideiglenesen törölve lesz.
A paraméter a tábla neve, amelyikben és amelyik leszármazottai között szerepelnie kell a node rekordnak';

CREATE TRIGGER interfaces_check_reference_node_id       BEFORE INSERT OR UPDATE ON public.interfaces  FOR EACH ROW EXECUTE PROCEDURE public.check_reference_node_id('nodes');
CREATE TRIGGER node_param_value_check_reference_node_id BEFORE INSERT OR UPDATE ON public.node_params FOR EACH ROW EXECUTE PROCEDURE public.check_reference_node_id('patchs');
CREATE TRIGGER nports_check_reference_node_id           BEFORE INSERT OR UPDATE ON public.nports      FOR EACH ROW EXECUTE PROCEDURE public.check_reference_node_id('nodes');

CREATE OR REPLACE FUNCTION check_interface() RETURNS TRIGGER AS $$
DECLARE
    n integer;
BEGIN
    IF NEW.hwaddress IS NULL THEN
        RETURN NEW;
    END IF;
    IF TG_OP = 'UPDATE' AND OLD.node_id = - NEW.node_id THEN
        RETURN NEW; -- No check
    END IF;
    SELECT COUNT(*) INTO n FROM interfaces WHERE node_id <> NEW.node_id AND hwaddress = NEW.hwaddress;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.port_id, NEW.hwaddress::text, 'check_interface()', TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION check_interface() IS 'Különböző node_id esetén nem lehet két azonos MAC.
Ha a node_id előjelet vált, akkor nincs ellenörzés (node rekord ideiglenes törlése).';

CREATE OR REPLACE FUNCTION check_host_services() RETURNS TRIGGER AS $$
DECLARE
    id      bigint;
    msk     text;
    sn      text;
    cset    boolean   := FALSE;
    nulltd  timestamp := '2000-01-01 00:00';
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF OLD.node_id = - NEW.node_id THEN -- Temporary deletion of owner, no check
            RETURN NEW;
        END IF;
        IF OLD.node_id = NEW.node_id AND
          (OLD.superior_host_service_id IS NOT NULL AND NEW.superior_host_service_id IS NULL) THEN
            -- Update a superior akármi törlése miatt, több rekord törlésénél előfordulhat,
            -- hogy már nincs meg a node vagy port amire a node_id ill. a prort_id mutat.
            -- késöbb ez a rekord törölve lessz, de ha hibát dobunk, akkor semmilyen törlés nem lessz.
            cset := TRUE;
        END IF;
    END IF;
    IF cset = FALSE AND NEW.port_id IS NOT NULL THEN
        -- Ha van port, az nem mutathat egy másik host portjára!
        SELECT node_id INTO id FROM nports WHERE port_id = NEW.port_id;
        IF NOT FOUND THEN
            PERFORM error('InvRef', NEW.port_id, 'port_id', 'check_host_services()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF id <> NEW.node_id THEN
            PERFORM error('InvalidOp', NEW.port_id, 'port_id', 'check_host_services()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
    ELSIF cset = FALSE AND 1 <> COUNT(*) FROM nodes WHERE node_id = NEW.node_id THEN
        PERFORM error('InvRef', NEW.node_id, 'node_id', 'check_host_services()', TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    IF TG_OP = 'UPDATE' THEN
        IF NEW.host_service_id <> OLD.host_service_id THEN
            PERFORM error('Constant', OLD.host_service_id, 'host_service_id', 'check_host_services()', TG_TABLE_NAME, TG_OP);
        END IF;
        -- change noalarm ?
        IF  NEW.noalarm_flag <> OLD.noalarm_flag
         OR COALESCE(NEW.noalarm_from, nulltd) <> COALESCE(OLD.noalarm_from, nulltd)
         OR COALESCE(NEW.noalarm_to,   nulltd) <> COALESCE(OLD.noalarm_to,   nulltd) THEN
            IF NEW.noalarm_flag = 'off' OR NEW.noalarm_flag = 'on' OR NEW.noalarm_flag = 'to' THEN
                NEW.noalarm_from = NULL;
            END IF;
            IF NEW.noalarm_flag = 'off' OR NEW.noalarm_flag = 'on' OR NEW.noalarm_flag = 'from' THEN
                NEW.noalarm_to   = NULL;
            END IF;
            IF (NEW.noalarm_flag = 'to' OR NEW.noalarm_flag = 'from_to') AND NEW.noalarm_to IS NULL THEN
                PERFORM error('NotNull', OLD.host_service_id, 'noalarm_to', 'check_host_services()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
            IF (NEW.noalarm_flag = 'from' OR NEW.noalarm_flag = 'from_to') AND NEW.noalarm_from IS NULL THEN
                PERFORM error('NotNull', OLD.host_service_id, 'noalarm_from', 'check_host_services()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
            IF ( NEW.noalarm_flag = 'from_to'                               AND NEW.noalarm_from  > NEW.noalarm_to) OR
               ((NEW.noalarm_flag = 'from_to' OR  NEW.noalarm_flag = 'to')  AND CURRENT_TIMESTAMP > NEW.noalarm_to) THEN
                PERFORM error('OutOfRange', OLD.host_service_id, 'noalarm_to', 'check_host_services()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
            IF  NEW.noalarm_flag <> OLD.noalarm_flag            -- Ha mégis csak azonos (modosítottunk időadatokat)
             OR COALESCE(NEW.noalarm_from, nulltd) <> COALESCE(OLD.noalarm_from, nulltd)
             OR COALESCE(NEW.noalarm_to,   nulltd) <> COALESCE(OLD.noalarm_to,   nulltd) THEN
                INSERT INTO host_service_noalarms
                    (host_service_id,     noalarm_flag,     noalarm_from,     noalarm_to,     noalarm_flag_old, noalarm_from_old, noalarm_to_old, user_id,                                     msg) VALUES
                    (NEW.host_service_id, NEW.noalarm_flag, NEW.noalarm_from, NEW.noalarm_to, OLD.noalarm_flag, OLD.noalarm_from, OLD.noalarm_to, current_setting('lanview2.user_id')::bigint, NEW.last_noalarm_msg);
            END IF;
        END IF;
    END IF;
    IF cset = FALSE THEN 
        IF TG_OP = 'INSERT' AND NEW.superior_host_service_id IS NULL THEN
            NEW := find_superior(NEW);
        ELSIF NEW.superior_host_service_id IS NOT NULL AND (TG_OP = 'INSERT' OR NEW.superior_host_service_id <> OLD.superior_host_service_id) THEN
            SELECT superior_service_mask INTO msk FROM services WHERE service_id = NEW.service_id;
            IF msk IS NOT NULL THEN
                SELECT  service_name INTO sn
                    FROM host_services JOIN services USING(service_id)
                    WHERE host_service_id = NEW.superior_host_service_id;
                IF sn !~ msk THEN
                    PERFORM error('Params', NEW.host_service_id, 'superior_host_service_id : "' || sn || '" !~ "' || msk || '"' , 'check_host_services()', TG_TABLE_NAME, TG_OP);
                    RETURN NULL;
                END IF; -- 1
            END IF; -- 2
        END IF; -- 3
    END IF; -- 4
    RETURN NEW;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION check_host_services() IS '
A host_services rekord ellenörző trigger függvény:
A rekord ID-t nem engedi modosítani.
Ellenörzi, hogy a node_id valóban egy nodes vagy snmpdevices rekordot azonosít-e,
kivéve UPDATE esetén, ha a node_id mező előjelet vált (a node ideiglenes törlését jelzi).
Ha port_id nem NULL, ellenörzi, hogy a node_id álltal azonosított objektum portja-e.
Ellenörzi a noalarm_flag, noalarm_from és noalarm_to mezők konzisztenciáját. Ha a két
időadat közöl valamelyik, felesleges, akkor törli azt, ha hiányos akkor kizárást generál.
Ha az idöadatok alapján a noalarm_flag már lejárt, akkor a noalarm_flag "off" lessz, és törli
mindkét időadatot.
Insert esetén, ha a superior_host_service_host_name értéke NULL, akkor egy find_superior() hívással
megkísérli kitölteni azt.
Ha a superior_host_service_host_name értéke nem NULL, és rekord beszúrás történt, vagy superior_host_service_host_name
megváltozott, akkor ellenörzi, hogy megfelel-e a services.superior_service_mask -mintának a hivatkozott szervíz neve.
';


CREATE OR REPLACE FUNCTION node2snmpdevice(nid bigint) RETURNS snmpdevices AS $$
DECLARE
    node  nodes;
    ntype nodetype[];
    snmpdev snmpdevices;
BEGIN
    SELECT * INTO node FROM ONLY nodes WHERE node_id = nid;
    IF NOT FOUND THEN
        PERFORM error('DataWarn', nid, 'node_id', 'node2snmpdevice()', 'nodes');
    END IF;
    UPDATE node_params   SET node_id = -nid WHERE node_id = nid;
    UPDATE nports        SET node_id = -nid WHERE node_id = nid;
    UPDATE host_services SET node_id = -nid WHERE node_id = nid;
    DELETE FROM nodes WHERE node_id = nid;
    ntype := array_append(node.node_type, 'snmp'::nodetype);
    ntype := array_replace(ntype, 'node'::nodetype, 'host'::nodetype);
    INSERT INTO
        snmpdevices(node_id, node_name, node_note, node_type, place_id, features, deleted,
            inventory_number, serial_number, model_number, model_name, location,
            node_stat, os_name, os_version)
        VALUES(nid, node.node_name, node.node_note, ntype, node.place_id, node.features, node.deleted,
            node.inventory_number, node.serial_number, node.model_number, node.model_name, node.location,
            node.node_stat, node.os_name, node.os_version)
        RETURNING * INTO snmpdev;
    UPDATE node_params   SET node_id = nid WHERE node_id = -nid;
    UPDATE nports        SET node_id = nid WHERE node_id = -nid;
    UPDATE host_services SET node_id = nid WHERE node_id = -nid;
    RETURN snmpdev;
END
$$ LANGUAGE plpgsql;
ALTER FUNCTION node2snmpdevice(bigint) OWNER TO lanview2;
COMMENT ON FUNCTION node2snmpdevice(bigint) IS
'Egy létező nodes rekord konvertálása/mozgatása az snmpdefices táblába, a tulajdonában lévő objektumok megtartásával.';

-- ----------------------------

DROP TABLE IF EXISTS alarm_service_vars;
CREATE TABLE alarm_service_vars (
    alarm_service_var_id    bigserial   PRIMARY KEY,
    alarm_id                bigint      NOT NULL
            REFERENCES alarms(alarm_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    service_var_id          bigint      NOT NULL
            REFERENCES service_vars(service_var_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    service_var_value       text,
    var_state               notifswitch,
    state_msg               text,
    raw_value               text,
    UNIQUE(alarm_id, service_var_id)
);
ALTER TABLE alarm_service_vars OWNER TO lanview2;

CREATE OR REPLACE FUNCTION alarm_notice() RETURNS TRIGGER AS $$
DECLARE
    sup_alarm_pending boolean := false;
BEGIN
    IF NEW.host_service_id <> 0                 -- is not ticket alarm
    AND NEW.superior_alarm_id IS NOT NULL THEN  -- sup. alarm is exists
        IF (SELECT end_time FROM alarms WHERE alarm_id = NEW.superior_alarm_id) IS NULL THEN
            sup_alarm_pending := true;  -- no events
        END IF;
    END IF; 
    IF NOT sup_alarm_pending THEN
        -- on-line events
        UPDATE user_events SET event_state = 'dropped'
            WHERE alarm_id IN ( SELECT alarm_id FROM alarms WHERE host_service_id = NEW.host_service_id)
                AND event_state = 'necessary';

        WITH uids AS (
            SELECT DISTINCT user_id
                FROM group_users
                WHERE group_id = ANY (
                    SELECT unnest(COALESCE(hs.online_group_ids, s.online_group_ids))
                        FROM host_services AS hs JOIN services AS s USING(service_id)
                        WHERE host_service_id = NEW.host_service_id)
        ) INSERT INTO user_events(user_id, alarm_id, event_type) SELECT user_id, NEW.alarm_id, 'notice'::usereventtype FROM uids;
        -- off-line events
        WITH uids AS (
            SELECT DISTINCT user_id
                FROM group_users
                WHERE group_id = ANY (
                    SELECT unnest(COALESCE(hs.offline_group_ids, s.offline_group_ids))
                        FROM host_services AS hs JOIN services AS s USING(service_id)
                        WHERE host_service_id = NEW.host_service_id)
        ) INSERT INTO user_events(user_id, alarm_id, event_type) SELECT user_id, NEW.alarm_id, 'sendmail'::usereventtype FROM uids;
    END IF;
    -- save actual value for connecting service variables 
    INSERT INTO alarm_service_vars(alarm_id, service_var_id, service_var_value, var_state, state_msg, raw_value)
        SELECT NEW.alarm_id, service_var_id, service_var_value, var_state, state_msg, raw_value FROM service_vars WHERE host_service_id = NEW.host_service_id;
    -- notify
    IF NOT sup_alarm_pending THEN
        NOTIFY alarm;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Hibajavítás: Nem zárta le az alarm-okat rendesen
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
    IF state < 'warning'::notifswitch THEN        -- ok
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
                -- DELETE FROM alarms WHERE alarm_id = alid;    Hülyeség! alid = NULL
            END IF;
        ELSE                    -- The alarm remains
            IF na = 'on'::isnoalarm OR s.disabled OR hs.disabled THEN -- Alarm is disabled, or services is disabled
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
$$ LANGUAGE plpgsql;

-- ----------------------------

SELECT set_db_version(1, 21);
