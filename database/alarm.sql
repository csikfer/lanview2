
-- //// alarm.nodes

ALTER TABLE host_services     DROP CONSTRAINT IF EXISTS host_services_act_alarm_log_id_fkey;
ALTER TABLE host_services     DROP CONSTRAINT IF EXISTS host_services_last_alarm_log_id_fkey;
ALTER TABLE host_service_logs DROP CONSTRAINT IF EXISTS host_service_logs_superior_alarm_id_fkey;
DROP  TABLE                                   IF EXISTS alarms;

CREATE TABLE alarms (
    alarm_id        bigserial      PRIMARY KEY,
    host_service_id bigint
            REFERENCES host_services(host_service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    daemon_id       bigint     DEFAULT NULL
            REFERENCES host_services(host_service_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    first_status    notifswitch NOT NULL,
    max_status      notifswitch NOT NULL,
    last_status     notifswitch NOT NULL,
    begin_time      timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    event_note      text,
    superior_alarm_id  bigint   DEFAULT NULL
            REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    noalarm         boolean     NOT NULL,
    end_time        timestamp   DEFAULT NULL,
    msg_time        timestamp   DEFAULT NULL,
    alarm_time      timestamp   DEFAULT NULL,
    notice_time     timestamp   DEFAULT NULL,
    notice_user_id  bigint     DEFAULT NULL
        REFERENCES users(user_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    ack_time        timestamp   DEFAULT NULL,
    ack_user_id     bigint      DEFAULT NULL
        REFERENCES users(user_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    ack_msg         text
);
CREATE INDEX alarms_begin_time_index ON alarms (begin_time);
CREATE INDEX alarms_end_time_index   ON alarms (end_time);
ALTER  TABLE alarms OWNER TO lanview2;

COMMENT ON TABLE  alarms                 IS 'Riasztási események táblája';
COMMENT ON COLUMN alarms.host_service_id IS 'A riasztást kiváltó szervíz azonosítója';
COMMENT ON COLUMN alarms.first_status    IS 'A riasztás keletkezésekori állapot';
COMMENT ON COLUMN alarms.max_status      IS 'A riasztás ideje alatti legsújosabb állapot.';
COMMENT ON COLUMN alarms.last_status     IS 'Az utolsó statusz a riastás vége elött';
COMMENT ON COLUMN alarms.begin_time      IS 'A riasztás kezdetánek az időpontja';
COMMENT ON COLUMN alarms.event_note      IS 'A risztást létrehozó üzenete, ha van';
COMMENT ON COLUMN alarms.superior_alarm_id  IS
    'Ha egy függő szervíz váltotta ki a riasztást, és a felsőbb szintű szolgáltatás is roasztási állpotban van, akkor annak a riasztásnak az azonosítója.';
COMMENT ON COLUMN alarms.noalarm         IS 'Ha az értesítés riasztásról le van tíltva, akkor értéke true.';
COMMENT ON COLUMN alarms.end_time        IS 'A riasztás végének az időpontja';
COMMENT ON COLUMN alarms.msg_time        IS 'Az off-line üzenet(ek) elküldésének az időpontja, ha volt ilyen';
COMMENT ON COLUMN alarms.alarm_time      IS 'Az első on line értesítés időpontja, ill. mikor jelent meg a riasztás egy futó kliens applikáción';
COMMENT ON COLUMN alarms.notice_time     IS 'On line felhasználó észrevette/lekérte a riasztás adatait';
COMMENT ON COLUMN alarms.notice_user_id  IS 'Az első on-line user, aki észrevette a riasztást';
COMMENT ON COLUMN alarms.ack_time        IS 'A riasztás on-line nyugtázásának az időpontja';
COMMENT ON COLUMN alarms.ack_user_id     IS 'Az user, aki a risztást nyugtázta';
COMMENT ON COLUMN alarms.ack_msg         IS 'A nyugtázáshoz fűzött üzenet';

ALTER TABLE host_services ADD FOREIGN KEY (act_alarm_log_id)
    REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
ALTER TABLE host_services ADD FOREIGN KEY (last_alarm_log_id)
    REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
ALTER TABLE host_service_logs ADD FOREIGN KEY (superior_alarm_id)
    REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
    
CREATE OR REPLACE FUNCTION alarm_notice() RETURNS TRIGGER AS $$
BEGIN
    IF NOT NEW.noalarm OR NEW.alarms.superior_alarm_id IS NULL THEN
--        IF  (TG_OP = 'UPDATE' AND NEW.max_status > OLD.max_status) OR TG_OP = 'INSERT' THEN
            NOTIFY alarm;
--        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- CREATE TRIGGER alarms_before_insert_or_update  BEFORE UPDATE OR INSERT ON alarms  FOR EACH ROW EXECUTE PROCEDURE alarm_notice();

CREATE OR REPLACE FUNCTION alarm_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    rname TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT host_service_id2name(host_service_id) || '/' || max_status INTO rname
        FROM alarms
        WHERE alarm_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'alarm_id', 'alarm_id2name()', 'alarms');
    END IF;
    RETURN rname;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION alarm_id2name(bigint) IS '
Álltatános id2name függvény.
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
    IF hs.delegate_host_state THEN
        UPDATE nodes SET node_stat = hs.hard_state WHERE node_id = hs.node_id;
    END IF;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION set_host_status(host_services) IS
'Beállítja a node állapotás a szervíz állapot (hard_state) alapján, amennyiben a szervíz rekordban a delegate_host_stat igaz.';

CREATE OR REPLACE FUNCTION check_superior_service(_hs host_services) RETURNS bigint AS $$
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
COMMENT ON FUNCTION check_superior_service(host_services) IS
'Ha egy függő szervízről van szó, akkor a felsőbb szintű szolgáltatások között keres egy ismert és
 warning-nál magsabszintű riasztási állapotút, és annak az azonosítójával tér vissza.
 Egyébként ill. ha nincs találat, akkor NULL-al tér vissza.';

CREATE OR REPLACE FUNCTION chk_flapping(
    hsid        bigint,
    tflapp      interval,
    iflapp      integer)
RETURNS boolean AS $$
BEGIN
    RETURN iflapp <  COUNT(*) FROM host_service_logs
            WHERE host_service_id = hsid
              AND date_of > (CURRENT_TIMESTAMP - tflapp);
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION chk_flapping(bigint, interval, integer) IS
'Ellenörzi, hogy az állpotváltozások billegésnek (flapping) nubősülnek-e.
 Vagyis a megadott időintervallumban (tflapp) a hsid azonosítójü szolgáltatás állpota
 töbszőr változott mint iflapp. Ha igen igazzal, egyébként hamis értékkel tér vissza.';

CREATE OR REPLACE FUNCTION touch_host_service(
    hsid        bigint                 -- A host_services rekord id-je
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
COMMENT ON FUNCTION touch_host_service(bigint) IS
'A megadott azonosítójú host_services rekordban a last_touched mezőt az aktuális időpontra változtatja,
 és a módosított rekord tartalomával tér vissza';


CREATE OR REPLACE FUNCTION set_service_stat(
    hsid        bigint,             -- A host_services rekord id-je
    state       notifswitch,        -- Az új státusz
    note        text DEFAULT '',    -- Az eseményhez tartozó üzenet (opcionális)
    dmid        bigint DEFAULT NULL)-- Daemon host_service_id
RETURNS host_services AS $$
DECLARE
    hs          host_services;  -- Az új host_services rekord
    old_hs      host_services;  -- A régi
    s           services;       -- A hozzá tartozó services rekord
    na          isnoalarm;      -- Alarm tiltás állpota
    sup         bigint;
    tflapp      interval;       -- Flapping figyelés időablakának a hossza
    iflapp      integer;        -- Az isőablakon bellüli státuszváltozások száma
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
    CASE state
        WHEN 'on','recovered' THEN
            IF hs.hard_state = 'on' THEN
                hs.host_service_state := 'on';
            ELSE
                hs.host_service_state := 'recovered';
            END IF;
            hs.hard_state := 'on';
            hs.soft_state := 'on';
            hs.check_attempts := 0;
        WHEN 'warning', 'unreachable','down','unknown','critical' THEN
            IF hs.hard_state <> 'on' THEN   -- nem most rommlott el
                hs.hard_state := state;
                hs.soft_state := state;
                hs.host_service_state := state;
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
    -- Update, and insert log record
    IF hs.last_changed IS NULL OR old_hs.host_service_state <> hs.host_service_state THEN
        hs.last_changed = CURRENT_TIMESTAMP;
    END IF;
    IF hs.host_service_state <> old_hs.host_service_state OR
       hs.hard_state         <> old_hs.hard_state OR
       hs.soft_state         <> old_hs.soft_state THEN
        INSERT INTO host_service_logs(host_service_id, old_state, old_soft_state, old_hard_state,
                           new_state, new_soft_state, new_hard_state, event_note, superior_alarm_id, noalarm)
            VALUES  (hsid, old_hs.host_service_state, old_hs.soft_state, old_hs.hard_state,
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, sup, na = 'on');
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
        -- Új riasztás,
        WHEN (old_hs.host_service_state < 'warning' OR old_hs.host_service_state = 'unknown')
         AND hs.host_service_state >= 'warning' THEN
            RAISE INFO 'New alarms record';
            INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id, noalarm)
                VALUES(hsid, dmid, hs.host_service_state, hs.host_service_state, hs.host_service_state, note, sup, na = 'on' )
                RETURNING alarm_id INTO hs.act_alarm_log_id;
        -- A helyzet fokozódik
        WHEN old_hs.host_service_state < hs.host_service_state OR old_hs.host_service_state = 'unknown' THEN
        RAISE INFO 'Update alarms record Up';
            UPDATE alarms SET
                    max_status  = hs.host_service_state,
                    last_status = hs.host_service_state,
                    noalarm     = (na = 'on'),
                    superior_alarm_id = sup
                WHERE alarm_id = hs.act_alarm_log_id;
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.host_service_state < hs.host_service_state THEN
            RAISE INFO 'Update alarms record Down';
            UPDATE alarms SET
                    last_status = hs.host_service_state,
                    noalarm     = (na = 'on'),
                    superior_alarm_id = sup
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
COMMENT ON FUNCTION set_service_stat(hsid bigint, state notifswitch, note text, dmid bigint) IS
'Adminisztrálja a megadott szolgáltatás megállpított új állapotát.';

CREATE TABLE alarm_messages (
    service_type_id     bigint          NOT NULL
        REFERENCES service_types(service_type_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    status              notifswitch     NOT NULL,
    short_msg           text     NOT NULL,
    message             text            NOT NULL,
    PRIMARY KEY (service_type_id, status)
);
ALTER  TABLE alarm_messages OWNER TO lanview2;
COMMENT ON TABLE alarm_messages IS 'Riasztási üzenetek táblája.';
COMMENT ON COLUMN alarm_messages.service_type_id IS 'Melyik szervíz típushoz/csoporthoz tartozik a riasztási üzenet.';
COMMENT ON COLUMN alarm_messages.status IS 'Milyen állapothoz tartozik a riasztási üzenet.';
COMMENT ON COLUMN alarm_messages.short_msg IS 'Rövide üzenet';
COMMENT ON COLUMN alarm_messages.message IS 'Részletes üzenet minta.';

CREATE OR REPLACE FUNCTION alarm_message(sid bigint, st notifswitch) RETURNS text AS $$
DECLARE
    msg         text; 
    hs          host_services;
    stid        bigint;
    s           services;
    n           nodes;
    pl          places;
BEGIN
    SELECT * INTO hs FROM host_services WHERE host_service_id = sid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', sid, 'host_service_id', 'alarm_message()', 'host_services');
    END IF;
    
    SELECT * INTO s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'alarm_message()', 'services');
    END IF;
    
    SELECT message INTO msg FROM alarm_messages WHERE service_type_id = s.service_type_id AND status = st;
    IF NOT FOUND THEN
        SELECT message INTO msg FROM alarm_messages WHERE service_type_id = -1 AND status = st; -- service_type = unmarked
        IF NOT FOUND THEN
            msg := '$hs.name is $st';
        END IF;
    END IF;

    msg := replace(msg, '$st',       st::text);
    IF msg LIKE '%$hs.%' THEN 
        IF msg LIKE '%$hs.name%' THEN
            msg := replace(msg, '$hs.name',  host_service_id2name(hs.host_service_id));
        END IF;
        msg := replace(msg, '$hs.note',  COALESCE(hs.host_service_note, ''));
    END IF;

    IF msg LIKE '%$s.%' THEN 
        msg := replace(msg, '$s.name',   s.service_name);
        msg := replace(msg, '$s.note',   COALESCE(s.service_note, ''));
    END IF;

    IF msg LIKE '%$n.%' OR msg LIKE '%$pl.%' THEN
        SELECT * INTO n FROM nodes WHERE node_id = hs.node_id;
        IF NOT FOUND THEN
            PERFORM error('IdNotFound', hs.node_id, 'node_id', 'alarm_message()', 'nodes');
        END IF;
        IF msg LIKE '%$n.%' THEN
            msg := replace(msg, '$n.name',     n.node_name);
            msg := replace(msg, '$n.note',     COALESCE(n.node_note, ''));
        END IF;
        IF msg LIKE '%$pl.%' THEN
            SELECT * INTO pl FROM placess WHERE place_id = n.place_id;
            IF NOT FOUND THEN
                PERFORM error('IdNotFound', n.place_id, 'plaxe_id', 'alarm_message()', 'places');
            END IF;
            msg := replace(msg, '$pl.name',     pl.place_name);
            msg := replace(msg, '$pl.note',     COALESCE(pl.place_note, ''));
        END IF;
    END IF;
    RETURN msg;
END
$$ LANGUAGE plpgsql;

DROP VIEW IF EXISTS view_alarms;
CREATE VIEW view_alarms AS
    SELECT
    a.alarm_id,
    a.host_service_id,
    hs.node_id,
    n.place_id,
    a.superior_alarm_id,
    a.begin_time,
    a.end_time,
    COALESCE(n.node_note, n.node_name) AS node_name,
    a.max_status,
    COALESCE(pl.place_note, pl.place_name) AS place_name,
    alarm_message(host_service_id, max_status) AS msg,
    au.user_name AS ack_user_name
    FROM alarms        AS a
    JOIN host_services AS hs USING(host_service_id)
    JOIN nodes         AS n  USING(node_id)
    JOIN places        AS pl USING(place_id)
    LEFT OUTER JOIN users AS au ON a.ack_user_id = au.user_id
    ORDER BY begin_time DESC;
    