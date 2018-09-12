-- Hibajavítás

-- Nem lehet törölni sunet-et
CREATE OR REPLACE FUNCTION subnet_delete_before() RETURNS TRIGGER AS $$
BEGIN
    UPDATE ip_addresses SET subnet_id = NULL, address = NULL WHERE subnet_id = OLD.subnet_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER subnet_delete_beforeí_trigger BEFORE DELETE ON subnets FOR EACH ROW EXECUTE PROCEDURE subnet_delete_before();

-- A VLAN-ok szintén törölhetetlenek
CREATE OR REPLACE FUNCTION vlan_delete_before() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM subnets WHERE vlan_id = OLD.vlan_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER vlan_delete_before_trigger BEFORE DELETE ON vlans FOR EACH ROW EXECUTE PROCEDURE vlan_delete_before();

-- A is_dyn_addr() hibát dobott, ha volt exclude tartomány
CREATE OR REPLACE FUNCTION is_dyn_addr(inet) RETURNS bigint AS $$
DECLARE
    id  bigint;
BEGIN
    SELECT dyn_addr_range_id INTO id FROM dyn_addr_ranges WHERE exclude = false AND begin_address <= $1 AND $1 <= end_address;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    IF 0 < COUNT(*) FROM dyn_addr_ranges WHERE exclude = true  AND begin_address <= $1 AND $1 <= end_address THEN
        RETURN NULL;
    END IF;
    RETURN id;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION is_dyn_addr(inet) IS 'Ellenőrzi, hogy a paraméterként megadott IP cím része-e egy dinamikus IP tartománynak';

-- Az lldp_link rekord törlése, ha egy azonos log_link rekordot törlünk.
CREATE OR REPLACE FUNCTION delete_lldp_link() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM lldp_links_table
        WHERE (port_id1 = OLD.port_id1 AND port_id2 = OLD.port_id2)
           OR (port_id1 = OLD.port_id2 AND port_id2 = OLD.port_id1);
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;
CREATE TRIGGER delete_log_links_table BEFORE DELETE ON log_links_table FOR EACH ROW EXECUTE PROCEDURE delete_lldp_link();

-- Egy VIEW a mactab_logs megjelenítéséhez
CREATE OR REPLACE FUNCTION mac2node_name(mac macaddr) RETURNS text AS $$
DECLARE
    nn text;
BEGIN
    SELECT node_name INTO nn 
        FROM interfaces
        JOIN nodes USING(node_Id)
        WHERE hwaddress = mac
        LIMIT 1;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    RETURN nn;
END;
$$ LANGUAGE plpgsql;

-- Az adatbázis hiba rekordban nem jelenik meg az user neve, csak az ID, mert hiányzik az fkey def.
ALTER TABLE db_errs ALTER COLUMN user_id DROP NOT NULL;
ALTER TABLE db_errs ADD CONSTRAINT db_errs_user_id_fkey FOREIGN KEY (user_id)
      REFERENCES public.users (user_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL;

-- Melyik címtáblában van a port

CREATE OR REPLACE VIEW port_in_mactab AS
    SELECT
        i.port_id AS port_id,
        n.node_id AS node_id,
        i.port_name AS port_name,
        n.node_name || ':' || i.port_name AS port_full_name,
        m.port_id AS mactab_port_id,
        port_id2full_name(m.port_id) AS mactab_port_full_name,
        m.mactab_state,
        m.first_time,
        m.last_time,
        m.set_type
    FROM mactab     AS m
    JOIN interfaces AS i USING(hwaddress)
    JOIN nodes      AS n USING(node_id);
    
    
-- Hibajavítás (2018.08.09.)

CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
    ipa ip_addresses;
    nip boolean;    -- IP address type IS NULL
    snid bigint;
    cnt integer;
BEGIN
    RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím ?
    nip := NEW.ip_address_type IS NULL;
    IF nip THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) IS NOT NULL THEN
            NEW.ip_address_type := 'dynamic';
        ELSE
            NEW.ip_address_type := 'fixip';
        END IF;
    END IF;
    IF NEW.address IS NOT NULL THEN
        -- Check subnet_id
        IF NEW.subnet_id IS NOT NULL AND NOT (SELECT NEW.address << netaddr FROM subnets WHERE subnet_id = NEW.subnet_id) THEN
            RAISE INFO 'Clear subnet id : %', NEW.subnet_id;
            NEW.subnet_id := NULL;  -- Clear invalid subnet_id
        END IF;
        -- Nincs subnet (id), keresünk egyet
        IF NEW.subnet_id IS NULL AND NEW.ip_address_type <> 'external' THEN
            BEGIN
                SELECT subnet_id INTO STRICT NEW.subnet_id FROM subnets WHERE netaddr >> NEW.address;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN     -- nem találtunk
                        PERFORM error('NotFound', -1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                        PERFORM error('Ambiguous',-1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END;
            RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        ELSIF NEW.ip_address_type = 'external'  THEN
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha nem private, akkor vizsgáljuk az ütközéseket
        IF NEW.ip_address_type <> 'private' THEN
            -- Azonos IP címek?
            FOR ipa IN SELECT * FROM ip_addresses WHERE NEW.address = address LOOP
                IF ipa.ip_address_id <> NEW.ip_address_id THEN 
                    IF ipa.ip_address_type = 'dynamic' THEN
                    -- Ütköző dinamikust töröljük.
                        UPDATE ip_addresses SET address = NULL WHERE ip_address_id = ipa.ip_address_id;
                    ELSIF ipa.ip_address_type = 'joint' AND (nip OR NEW.ip_address_type = 'joint') THEN
                    -- Ha közös címként van megadva a másik, ...
                        NEW.ip_address_type := 'joint';
                    ELSIF ipa.ip_address_type <> 'private' THEN
                    -- Minden más esetben ha nem privattal ütközik az hiba
                        PERFORM error('IdNotUni', 0, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    END IF;
                END IF;
            END LOOP;
        END IF;
        -- Ha a preferred nincs megadva, akkor az elsőnek megadott cím a preferált
        IF NEW.preferred IS NULL THEN
            SELECT 1 + COUNT(*) INTO NEW.preferred FROM interfaces JOIN ip_addresses USING(port_id) WHERE port_id = NEW.port_id AND preferred IS NOT NULL AND address IS NOT NULL;
        END IF;
    ELSE
        -- Cím ként a NULL csak a dynamic típusnál megengedett
        IF NEW.ip_address_type <> 'dynamic' THEN
            PERFORM error('DataError', 0, 'NULL ip as non dynamic type', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
        END IF;
        -- RAISE INFO 'IP address is NULL';
    END IF;
    -- RAISE INFO 'Return, NEW = %', NEW;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Hibajavítás (2018.08.17.)

CREATE OR REPLACE FUNCTION set_host_status(hs host_services) RETURNS VOID AS $$
DECLARE
    st notifswitch;
    ost notifswitch;
BEGIN
    IF hs.delegate_host_state THEN
        st := hs.hard_state;
        SELECT hard_state INTO ost FROM host_services
                WHERE node_id = hs.node_id AND delegate_host_state AND host_service_id <> hs.host_service_id
                ORDER BY hard_state DESC limit 1;
        IF NOT FOUND THEN
            ost := 'on';
        END IF;
        IF st >= ost THEN
            UPDATE nodes SET node_stat = st WHERE node_id = hs.node_id;
        END IF;
    END IF;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION set_host_status(host_services) IS
'Beállítja a node állapotás a szervíz állapot (hard_state) alapján, amennyiben a szervíz rekordban a delegate_host_state igaz.
és nincs ojan szervíz példány, aminél delegate_host_state szintén igaz, és az állapota nem rosszabb (nagyobb), vagy azonos.';

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
            RAISE INFO 'Close % alarms record.', alid;
            UPDATE alarms SET end_time = CURRENT_TIMESTAMP WHERE alarm_id = alid;
            hs.act_alarm_log_id := NULL;
            aldo := 'close';
        WHEN state <> 'on' AND alid IS NULL THEN
            RAISE INFO 'New (%) alarms record.', na;
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
        WHEN old_hs.hard_state < state THEN
            RAISE INFO 'The situation is worse. Update % alarms record.', alid;
            IF alid IS NULL THEN
                PERFORM error('WNotFound', hsid, 'host_service_id', 'set_service_stat()', 'host_services');
                aldo := 'caveat';
            ELSE
                UPDATE alarms SET
                        max_status  = hs.host_service_state,
                        last_status = hs.host_service_state,
                        superior_alarm_id = sup
                    WHERE alarm_id = alid;
                aldo := 'modify';
            END IF;
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.hard_state > state THEN
            RAISE INFO 'The situation is better. Update % alarms record', alid;
            IF alid IS NULL THEN
                PERFORM error('WNotFound', hsid, 'host_service_id', 'set_service_stat()', 'host_services');
                aldo := 'caveat';
            ELSE
                UPDATE alarms SET
                        last_status = hs.host_service_state,
                        superior_alarm_id = sup
                    WHERE alarm_id = alid;
                aldo := 'modify';
            END IF;
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