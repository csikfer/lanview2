
-- //// alarm.nodes

CREATE TABLE alarms (
    alarm_id        serial      PRIMARY KEY,
    host_service_id integer
            REFERENCES host_services(host_service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    daemon_id       integer     DEFAULT NULL
            REFERENCES host_services(host_service_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE CASCADE,
    first_status    notifswitch NOT NULL,
    max_status      notifswitch NOT NULL,
    last_status     notifswitch NOT NULL,
    begin_time      timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    event_descr     varchar(255),
    superior_alarm  integer     DEFAULT NULL
            REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    noalarm         boolean     NOT NULL,
    end_time        timestamp   DEFAULT NULL,
    msg_time        timestamp   DEFAULT NULL,
    alarm_time      timestamp   DEFAULT NULL,
    notice_time     timestamp   DEFAULT NULL,
    notice_user     integer     DEFAULT NULL,
    ack_time        timestamp   DEFAULT NULL,
    ack_user        integer     DEFAULT NULL,
    ack_msg         varchar(255)
);
CREATE INDEX alarms_begin_time_index ON alarms (begin_time);
CREATE INDEX alarms_end_time_index   ON alarms (end_time);
ALTER  TABLE alarms OWNER TO lanview2;

COMMENT ON COLUMN alarms.host_service_id IS 'A riasztást kiváltó szervíz azonosítója';
COMMENT ON COLUMN alarms.first_status IS 'A riasztás keletkezésekori státusz';
COMMENT ON COLUMN alarms.max_status IS 'A riasztás ideje alatti legsújosabb státusz.';
COMMENT ON COLUMN alarms.last_status IS 'Az utolsó statusz a riastás vége elött';
COMMENT ON COLUMN alarms.begin_time IS 'A riasztás kezdetánek az időpontja';
COMMENT ON COLUMN alarms.event_descr IS 'A risztást létrehozó üzenete, ha van';
COMMENT ON COLUMN alarms.superior_alarm IS 'A egy függő szervíz, vagy hoszt is riastásban van, akkor értéke true';
COMMENT ON COLUMN alarms.noalarm    IS 'Ha az értesítés riasztásról le van tíltva, akkor értéke true.';
COMMENT ON COLUMN alarms.end_time   IS 'A riasztás végének az időpontja';
COMMENT ON COLUMN alarms.msg_time   IS 'Az off-line üzenet(ek) elküldésének az időpontja';
COMMENT ON COLUMN alarms.alarm_time IS 'Az első on line értesítés időpontja';
COMMENT ON COLUMN alarms.notice_time IS 'On line felhasználó észrevette/lekérte a riasztás adatait';
COMMENT ON COLUMN alarms.notice_user IS 'Az első on-line user, aki észrevette a riasztást';
COMMENT ON COLUMN alarms.ack_time   IS 'A riasztás on-line nyugtázásának az időpontja';
COMMENT ON COLUMN alarms.ack_user   IS 'Az user, aki a risztást nyugtázta';
COMMENT ON COLUMN alarms.ack_msg    IS 'A nyugtázáshoz fűzött üzenet';

ALTER TABLE host_services ADD FOREIGN KEY (act_alarm_log_id)
    REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
ALTER TABLE host_services ADD FOREIGN KEY (last_alarm_log_id)
    REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;

CREATE OR REPLACE FUNCTION alarm_id2name(integer) RETURNS TEXT AS $$
DECLARE
    rname TEXT;
BEGIN
    SELECT host_service_id2name(host_service_id) || ':' || max_status INTO rname
        FROM alarms
        JOIN host_services USING(host_service_id)
        WHERE alarm_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'alarm_id', 'alarm_id2name()', 'alarms');
    END IF;
    RETURN rname;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION alarm_id2name(integer) IS '
Álltatános is2name függvény.
Az alarms.alarm_id érték alapján egy megjeleníthető stringgel tér vissza.
Ha nem létezik az azonosító szerinti rekord, akkor hibát dob.
';

-- //////////////////////////////////////////////////
-- HELPER FÜGGVÉNYEk
-- //////////////////////////////////////////////////

CREATE OR REPLACE FUNCTION set_host_status(hs host_services) RETURNS VOID AS $$
DECLARE
    st notifswitch;
BEGIN
    SELECT host_service_state INTO st FROM host_services
        WHERE node_id = hs.node_id AND delegate_host_state
        ORDER BY host_service_state DESC LIMIT 1;
    IF NOT FOUND THEN
        st = 'on';
    END IF;
    UPDATE nodes SET node_stat = st WHERE node_id = hs.node_id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION check_superior_service(_hs host_services) RETURNS integer AS $$
DECLARE
    hs host_services;
    n  integer;
BEGIN
    hs := _hs;
    n  := 0;
    LOOP
        IF hs.superior_host_service_id IS NULL THEN
            RETURN NULL;
        END IF;
        IF n > 20 THEN
            PERFORM error('Loop', _hs.superior_host_service_id, 'superior_host_service_id', 'chack_superior_service()');
        END IF;
        SELECT * INTO hs FROM host_services WHERE host_service_id = hs.superior_host_service_id;
        IF hs.host_service_state > 'warning' AND hs.host_service_state <> 'unknown' THEN
            IF hs.act_alarm_log_id IS NULL THEN
                PERFORM error('DataWarn', hs.host_service_id, 'act_alarm_log_id', 'check_superior_service()', 'host_services');
            END IF;
            RETURN hs.act_alarm_log_id;
        END IF;
        n := n + 1;
    END LOOP;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION chk_flapping(
    hsid        integer,
    s           services)
RETURNS boolean AS $$
BEGIN
    RETURN s.flapping_max_change <  COUNT(*) FROM host_service_logs
            WHERE host_service_id = hsid
              AND date_of > (CURRENT_TIMESTAMP - s.flapping_interval);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION touch_host_service(
    hsid        integer                 -- A host_services rekord id-je
)
RETURNS host_services AS $$
DECLARE
    hs          host_services;
BEGIN
    UPDATE host_services SET last_touched = CURRENT_TIMESTAMP WHERE host_service_id = hsid
        RETURNING * INTO hs;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hsid, 'host_service_id', 'touch_host_service()', 'host_services');
    END IF;
    RETURN hs;
END
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION set_service_stat(
    hsid        integer,                -- A host_services rekord id-je
    state       notifswitch,            -- Az új státusz
    note        varchar(255) DEFAULT '',-- Az eseményhez tartozó üzenet (opcionális)
    dmid        integer DEFAULT NULL)   -- Daemon host_service_id
RETURNS host_services AS $$
DECLARE
    hs          host_services;
    old_hs      host_services;
    s           services;
    na          isnoalarm;
    sup         integer;
BEGIN
    hs := touch_host_service(hsid);
    SELECT * INTO  s FROM services      WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'set_service_stat()', 'services');
    END IF;
    IF state = hs.host_service_state AND state = hs.hard_state AND state = hs.soft_state THEN   -- Ha nincs változás
        PERFORM set_host_status(hs);
        RETURN hs;
    END IF;
    old_hs := hs;
    CASE state
        WHEN 'on','recovered' THEN
            IF hs.hard_state = 'on' THEN
                hs.host_service_state = state;
            ELSE
                hs.host_service_state = 'recovered';
            END IF;
            hs.hard_state := 'on';
            hs.soft_state := 'on';
            hs.check_attempts := 0;
        WHEN 'warning', 'unreachable','down','unknown','critical' THEN
            IF hs.hard_state <> 'on' THEN   -- nem most rommlott el
                hs.hard_state := state;
                hs.soft_state := state;
                hs.host_service_state = state;
                hs.check_attempts := 0;
            ELSE                            -- mostm vagy nem rég romlott el, hihető?
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
                IF hs.check_attempts >= hs.max_check_attempts THEN  -- Elhisszük, hogy baj van
                    hs.hard_state := state;
                    hs.host_service_state := state;
                END IF;
            END IF;
        ELSE
            PERFORM error('Params', state, 'notifswitch', 'set_service_stat()');    -- kilép!
    END CASE;
    IF chk_flapping(hsid, s) THEN
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
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, sup, na = 'on',
                           s.service_name, (SELECT node_name FROM nodes WHERE node_id = hs.node_id));
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
                hs.last_alarm_log_id := NULL;
            END IF;
        -- Új riasztás,
        WHEN (old_hs.host_service_state < 'warning' OR old_hs.host_service_state = 'unknown')
         AND hs.host_service_state >= 'warning' THEN
            RAISE INFO 'New alarms record';
            INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_descr, superior_alarm, noalarm)
                VALUES(hsid, dmid, hs.host_service_state, hs.host_service_state, hs.host_service_state, note, sup, na = 'on' )
                RETURNING alarm_id INTO hs.act_alarm_log_id;
        -- A helyzet fokozódik
        WHEN old_hs.host_service_state < hs.host_service_state OR old_hs.host_service_state = 'unknown' THEN
        RAISE INFO 'Update alarms record Up';
            UPDATE alarms SET
                    max_status  = hs.host_service_state,
                    last_status = hs.host_service_state,
                    noalarm     = (na = 'on'),
                    superior_alarm = sup
                WHERE alarm_id = hs.act_alarm_log_id;
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.host_service_state < hs.host_service_state THEN
            RAISE INFO 'Update alarms record Down';
            UPDATE alarms SET
                    last_status = hs.host_service_state,
                    noalarm     = (na = 'on'),
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




