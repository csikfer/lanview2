-- //// LAN.IPPROTOCOLLS

CREATE TABLE ipprotocols (
    protocol_id     bigint     PRIMARY KEY,    -- == protocol number
    protocol_name   text NOT NULL UNIQUE
);
ALTER TABLE ipprotocols OWNER TO lanview2;

INSERT INTO ipprotocols (protocol_id, protocol_name) VALUES
    ( -1, 'nil' ),  --No network
    (  0, 'ip'  ),
    (  1, 'icmp'),
    (  6, 'tcp' ),
    ( 17, 'udp' );

    
CREATE TABLE service_types (
    service_type_id     bigserial       PRIMARY KEY,
    service_type_name   text     UNIQUE,
    service_type_note   text     DEFAULT NULL
);
ALTER TABLE service_types OWNER TO lanview2;
COMMENT ON TABLE  service_types IS 'A service objektumok csoportosítását teszi lehetővé, egy rekord csak egy csoportba tartozhat. Alarm message.';
COMMENT ON COLUMN service_types.service_type_id   IS 'service csoport ill. típus azonosító.';
COMMENT ON COLUMN service_types.service_type_name IS 'service csoport ill. típus név.';
COMMENT ON COLUMN service_types.service_type_note IS 'Megjegyzés.';

INSERT INTO service_types (service_type_id, service_type_name) VALUES 
    ( -1,        'unmarked'),
    (  0,        'ticket');
    


CREATE TABLE services (
    service_id              bigserial   PRIMARY KEY,
    service_name            text        NOT NULL UNIQUE,
    service_note            text        DEFAULT NULL,
    service_type_id         bigint      DEFAULT -1  -- unmarked
        REFERENCES service_types(service_type_id) MATCH FULL ON DELETE SET DEFAULT ON UPDATE RESTRICT,
    protocol_id             bigint      DEFAULT -1  -- nil
        REFERENCES ipprotocols(protocol_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    port                    integer     DEFAULT NULL,
    superior_service_mask   text        DEFAULT NULL,
    check_cmd               text        DEFAULT NULL,
    features                text        DEFAULT ':',
    disabled                boolean     NOT NULL DEFAULT FALSE,
    max_check_attempts      integer     DEFAULT NULL,
    normal_check_interval   interval    DEFAULT NULL,
    retry_check_interval    interval    DEFAULT NULL,
    timeperiod_id           bigint      NOT NULL DEFAULT 0  -- DEFAULT 'always'
        REFERENCES timeperiods(timeperiod_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    flapping_interval       interval    NOT NULL DEFAULT '30 minutes',
    flapping_max_change     integer     NOT NULL DEFAULT 15,
    deleted                 boolean     NOT NULL DEFAULT false
);
ALTER TABLE services OWNER TO lanview2;
COMMENT ON TABLE  services                  IS 'Services table';
COMMENT ON COLUMN services.service_id       IS 'ID egyedi azonosító';
COMMENT ON COLUMN services.service_name     IS 'Szervice name (egyedi)';
COMMENT ON COLUMN services.service_note     IS 'Megjegyzés';
COMMENT ON COLUMN services.protocol_id      IS 'Ip protocol id (-1 : nil, if no ip protocol)';
COMMENT ON COLUMN services.port             IS 'Default (TCP, UDP, ...) port number. or NULL';
COMMENT ON COLUMN services.check_cmd        IS 'Default check command';
COMMENT ON COLUMN services.features         IS
'Default paraméter lista (szeparátor a kettőspont, paraméter szeparátor az ''='', első és utolsó karakter a szeparátor):\n
timing
    custom      Belső időzítés (alapértelmezett)
    timed       Időzített
    thread      Saját szál
    timed,thread Időzített saját szállal
    passive     Valamilyen lekérdezés (superior) járulékos eredményeként van állpota, vagy csak az alárendelteket indítja, önálló tevékenység nélkül.
    polling     Egyszeri futás
process      Az szolgáltatás ellenörzése egy program, paraméterek:
    respawn     A programot újra kell indítani, ha kilép. A kilépés nem hiba.\n
    continue    A program normál körülmények között nem lép ki, csak ha leállítják, vagy hiba van. (default)
    polling     A programot egyszer kell inditani, elvégzi a dolgát és kilép. Nincs időzítés vagy újraindítás
    timed       A programot időzitve kell indítani (periódikusan)
    carried     A program önállóan állítja a statusát, a fenzi opciókkal együtt adható meg.
superior    Alárendelteket ellenörző eljárásokat hív
    <üres>      Alárendelt viszony,autómatikus
method
    custom      saját/ismeretlen (alapértelmezett)
    nagios      Egy Nagios plugin
    munin       Egy Munin plugin
    qparser     query parser
    parser      Parser szülö, ha a parser önálló szálban fut.
    carried     Önálló (csak akkor kell adminisztrállni, ha kiakadt)
ifType      A szolgáltatás hierarhia mely port típus linkjével azonos (paraméter: interface típus neve)
disabled    service_name = icontsrv , csak a host_services rekordban, a szolgáltatás (riasztás) tiltva.
reversed    service_name = icontsrv , csak a host_services rekordban, a port fordított bekötését jelzi.
serial      Serial port paraméterei pl.: "serial=19200 7E1 No"
logrot      Log az stderr-re, log fájlt a superior kezeli. Log fájl rotálása (par1: max méret, par2: arhiv [db])
lognull     A superior az stderr és stdout-ot a null-ba irányítja. A szolgáltatás önállóan loggol.
tcp         Alternatív TCP port megadása, paraméter a port száma
udp         Alternatív UDP port megadása, paraméter a port száma
';
COMMENT ON COLUMN services.max_check_attempts    IS 'Hibás eredmények maximális száma, a riasztás kiadása elött. Alapértelmezett érték.';
COMMENT ON COLUMN services.normal_check_interval IS 'Ellenörzések ütemezése, ha nincs hiba. Alapértelmezett érték.';
COMMENT ON COLUMN services.retry_check_interval  IS 'Ellenörzések ütemezése, hiba esetén a riasztás kiadásáig. Alapértelmezett érték.';


ALTER TABLE services ADD COLUMN offline_group_ids bigint[] DEFAULT NULL;
COMMENT ON COLUMN services.offline_group_ids IS 'off-line riasztás küldése, az user_froup_id-k alapján a tagoknak, alapértelmezés';
ALTER TABLE services ADD COLUMN online_group_ids  bigint[] DEFAULT NULL;
COMMENT ON COLUMN services.online_group_ids  IS 'on-line riasztás küldése, az user_froup_id-k alapján a tagoknak, alapértelmezés';
ALTER TABLE services ADD COLUMN heartbeat_time interval;

INSERT INTO services (service_id, service_name, service_note)
     VALUES          ( -1,        'nil',        'A NULL-t reprezentálja, de összehasonlítható');
INSERT INTO services (service_id, service_name, service_note, disabled)
     VALUES          (  0,        'ticket',     'Hiba jegy',  true );

CREATE OR REPLACE FUNCTION service_name2id(text) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT service_id INTO id FROM services WHERE service_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'service_name2id()', 'services');
    END IF;
    RETURN id;

END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION service_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    name TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT service_name INTO name FROM services WHERE service_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'service_id', 'service_id2name()', 'services');
    END IF;
    RETURN name;

END;
$$ LANGUAGE plpgsql;
-- //// LAN.HOSTSERVICES

CREATE TYPE noalarmtype AS ENUM ('off',     -- Az alarm-ok nincsenek letiltva
                                 'on',      -- Az alarmok le vannak tiltva
                                 'to',      -- Az alarmok egy megadott időpontig le vannak/voltak tiltva
                                 'from',    -- Az alarmok egy időpontól le lesznek/le vannak tiltva
                                 'from_to');-- Az alarmok egy tőintervallumban le lesznek/vannak/voltak tiltva
COMMENT ON TYPE noalarmtype IS '
Riasztás tiltási állapotok:
"off"     Az alarm-ok nincsenek letiltva;
"on"      Az alarmok le vannak tiltva;
"to"      Az alarmok egy megadott időpontig le vannak/voltak tiltva;
"from"    Az alarmok egy időpontól le lesznek/le vannak tiltva;
"from_to" Az alarmok egy tőintervallumban le lesznek/vannak/voltak tiltva';

CREATE TABLE host_services (
    host_service_id         bigserial      PRIMARY KEY,
    node_id                 bigint         NOT NULL,
    --  REFERENCES nodess(node_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    service_id              bigint         NOT NULL
        REFERENCES services(service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    port_id                 bigint         DEFAULT NULL,
    host_service_note       text           DEFAULT NULL,
    prime_service_id        bigint         NOT NULL DEFAULT -1       -- nil
        REFERENCES services(service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE RESTRICT,
    proto_service_id        bigint         NOT NULL DEFAULT -1       -- nil
        REFERENCES services(service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE RESTRICT,
    delegate_host_state     boolean        NOT NULL DEFAULT FALSE,
    check_cmd               text           DEFAULT NULL,
    features                text           DEFAULT NULL,
    disabled                boolean        NOT NULL DEFAULT FALSE,
    superior_host_service_id bigint        DEFAULT NULL
        REFERENCES host_services(host_service_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    max_check_attempts      integer        DEFAULT NULL,
    normal_check_interval   interval       DEFAULT NULL,
    retry_check_interval    interval       DEFAULT NULL,
    timeperiod_id           bigint         DEFAULT NULL
        REFERENCES timeperiods(timeperiod_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    flapping_interval       interval       DEFAULT NULL,
    flapping_max_change     integer        DEFAULT NULL,
    noalarm_flag            noalarmtype    NOT NULL DEFAULT 'off',
    noalarm_from            timestamp      DEFAULT NULL,
    noalarm_to              timestamp      DEFAULT NULL,
    offline_group_ids       bigint[]       DEFAULT NULL,
    online_group_ids        bigint[]       DEFAULT NULL,
-- Állapot
    host_service_state      notifswitch    NOT NULL DEFAULT 'unknown',
    soft_state              notifswitch    NOT NULL DEFAULT 'unknown',
    hard_state              notifswitch    NOT NULL DEFAULT 'unknown',
    state_msg               text           DEFAULT NULL,
    check_attempts          integer        NOT NULL DEFAULT 0,
    last_changed            TIMESTAMP      DEFAULT NULL,
    last_touched            TIMESTAMP      DEFAULT NULL,
    act_alarm_log_id        bigint         DEFAULT NULL,   -- REFERENCES alarms(alarm_id) lsd.: alarm.sql
    last_alarm_log_id       bigint         DEFAULT NULL,   -- REFERENCES alarms(alarm_id) lsd.: alarm.sql
-- Állapot vége
    deleted                 boolean        NOT NULL DEFAULT FALSE,
    last_noalarm_msg        text           DEFAULT NULL
);
ALTER TABLE host_services OWNER TO lanview2;
ALTER TABLE host_services ADD CONSTRAINT no_self_superior CHECK(host_service_id <> superior_host_service_id);

CREATE UNIQUE INDEX host_services_port_subservices_key ON host_services (node_id, service_id, COALESCE(port_id, -1), prime_service_id, proto_service_id);
-- CREATE UNIQUE INDEX host_services_node_id_service_id ON host_services(node_id, service_id) WHERE port_id IS NULL;
ALTER TABLE dyn_addr_ranges ADD FOREIGN KEY (host_service_id)  REFERENCES host_services(host_service_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT;

COMMENT ON TABLE  host_services IS 'A szolgáltatás-node összerendelések, ill. a konkrét szolgáltatások vagy ellenörzés utasítások táblája, és azok állpota.';
COMMENT ON COLUMN host_services.host_service_id IS 'Egyedi azonosító';
COMMENT ON COLUMN host_services.host_service_note IS 'Megjegyzés / leírás.';
COMMENT ON COLUMN host_services.node_id IS 'A node ill. host azonosítója, amin a szolgáltatás, vagy az ellenörzés fut.';
COMMENT ON COLUMN host_services.service_id IS 'A szolgáltatást, vagy az ellenörzés típusát azonosító ID';
COMMENT ON COLUMN host_services.prime_service_id IS 'Az ellenőrzés elsődleges módszerét azonosító, szervíz típus ID';
COMMENT ON COLUMN host_services.proto_service_id IS 'Az ellenőrzés módszerének opcionális további azonosítása (protokol), szervíz típus ID';
COMMENT ON COLUMN host_services.port_id IS 'Opcionális port azonosító, ha a szolgáltatás ill. ellenörzés egy porthoz rendelt.';
COMMENT ON COLUMN host_services.delegate_host_state IS 'Értéke igaz, ha a szolgáltatás állapotát örökli a node is.';
COMMENT ON COLUMN host_services.check_cmd IS 'Egy opcionális parancs.';
COMMENT ON COLUMN host_services.features IS 'További paraméterek. Ld.: services.features . Ha értéke nem NULL, akkor fellülbírálja a service_id -hez tartozó services.features értékét';
COMMENT ON COLUMN host_services.superior_host_service_id IS 'Szülő szolgáltatás. Az ellenörzés a szülőn keresztül hajtódik végre, ill. az végzi.';
COMMENT ON COLUMN host_services.max_check_attempts IS 'Hibás eredmények maximális száma, a riasztás kiadása elött.';
COMMENT ON COLUMN host_services.normal_check_interval IS 'Ellenörzések ütemezése másodpercben, ha nincs hiba.';
COMMENT ON COLUMN host_services.retry_check_interval IS 'Ellenörzések ütemezése, hiba esetén a riasztás kiadásáig.';
COMMENT ON COLUMN host_services.timeperiod_id IS 'Az ellenörzések időtartománya.';
COMMENT ON COLUMN host_services.noalarm_flag IS 'A riasztás tiltásának az állapota';
COMMENT ON COLUMN host_services.noalarm_from IS 'Ha a tiltáshoz kezdő időpont tartozik, akkor a tiltás kezdete.';
COMMENT ON COLUMN host_services.noalarm_to IS 'Ha a tiltáshoz záró időpont tartozik, akkor a tiltás vége.';
COMMENT ON COLUMN host_services.host_service_state IS 'A szolgáltatás állapota, álltalában azonos a hard_state-val, kiegészítve a recovered, és flapping állapottal.';
COMMENT ON COLUMN host_services.hard_state IS 'A szolgálltatás elfogadott állapota.';
COMMENT ON COLUMN host_services.soft_state IS 'A szolgálltatás utolsó ellenörzés szerinti állapota.';
COMMENT ON COLUMN host_services.state_msg IS 'Az aktuális állapothoz tartozó opcionális állapot üzenet';
COMMENT ON COLUMN host_services.check_attempts IS 'Hiba számláló.';
COMMENT ON COLUMN host_services.last_changed IS 'Utolsó állapot változás időpontja.';
COMMENT ON COLUMN host_services.last_touched IS 'Utolsó ellenözzés időpontja';
COMMENT ON COLUMN host_services.act_alarm_log_id IS 'Riasztási állapot esetén az aktuális riasztás log rekord ID-je';
COMMENT ON COLUMN host_services.last_alarm_log_id IS 'Az utolsó riasztás log rekord ID-je';

ALTER TABLE host_services ADD COLUMN heartbeat_time interval;

INSERT INTO host_services (host_service_id, node_id, service_id, host_service_note)
     VALUES               ( 0,              -1,      0,          'Hiba jegy.');


CREATE OR REPLACE FUNCTION host_service_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    name TEXT;
    proto TEXT;
    prime TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT
            n.node_name || CASE WHEN p.port_name IS NULL THEN '' ELSE ':' || p.port_name END || '.' || s.service_name,
            sprime.service_name,
            sproto.service_name
          INTO name, prime, proto
        FROM host_services hs
        JOIN nodes n USING(node_id)
        JOIN services s USING(service_id)
        LEFT JOIN nports p ON hs.port_id = p.port_id			-- ez lehet NULL
        JOIN services sproto ON hs.proto_service_id = sproto.service_id	-- a 'nil' nevű services a NULL
        JOIN services sprime ON hs.prime_service_id = sprime.service_id	-- a 'nil' nevű services a NULL
        WHERE host_service_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'host_service_id', 'host_service_id2name()', 'host_services');
    END IF;
    IF proto = 'nil' AND prime = 'nil' THEN
        RETURN name;
    ELSIF proto = 'nil' THEN
        RETURN name || '(:' || prime || ')';
    ELSIF prime = 'nil' THEN
        RETURN name || '(' || proto || ':)';
    ELSE
        RETURN name || '(' || proto || ':' || prime || ')';
    END IF;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION host_service_id2name(bigint) IS 'A host_service_id -ből a hivatkozott rekord alapján előállít egy egyedi nevet.';

CREATE TABLE host_service_logs (
    host_service_log_id bigserial      PRIMARY KEY,
    host_service_id     bigint
        REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    date_of             timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    old_state           notifswitch     NOT NULL,
    old_soft_state      notifswitch     NOT NULL,
    old_hard_state      notifswitch     NOT NULL,
    new_state           notifswitch     NOT NULL,
    new_soft_state      notifswitch     NOT NULL,
    new_hard_state      notifswitch     NOT NULL,
    event_note          text            DEFAULT NULL,
    superior_alarm_id   bigint          DEFAULT NULL,
    noalarm             boolean         NOT NULL
);
CREATE INDEX host_service_logs_date_of_index ON host_service_logs (date_of);
ALTER TABLE host_service_logs OWNER TO lanview2;
COMMENT ON TABLE host_service_logs IS 'Host szervízek státusz változásainak a log táblája';

CREATE TABLE host_service_noalarms (
    host_service_noalarm_id bigserial   PRIMARY KEY,
    date_of                 timestamp   DEFAULT CURRENT_TIMESTAMP,
    host_service_id         bigint      REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    noalarm_flag            noalarmtype NOT NULL,
    noalarm_from            timestamp   DEFAULT NULL,
    noalarm_to              timestamp   DEFAULT NULL,
    noalarm_flag_old        noalarmtype NOT NULL,
    noalarm_from_old        timestamp   DEFAULT NULL,
    noalarm_to_old          timestamp   DEFAULT NULL,
    user_id                 bigint      DEFAULT NULL REFERENCES users(user_id) MATCH SIMPLE,
    msg                     text        DEFAULT NULL
);
CREATE INDEX host_service_noalarms_date_of_index ON host_service_noalarms (date_of);
ALTER TABLE host_service_noalarms OWNER TO lanview2;
COMMENT ON TABLE host_service_noalarms IS 'Host szervízek riasztásainak a letiltását loggoló tábla';
COMMENT ON COLUMN host_service_noalarms.date_of IS 'A tiltás megadásának az időpontja';
COMMENT ON COLUMN host_service_noalarms.host_service_id IS 'A tiltott szolgáltatás azonosító';
COMMENT ON COLUMN host_service_noalarms.noalarm_flag IS 'A tiltás típusa';
COMMENT ON COLUMN host_service_noalarms.noalarm_from IS 'Ha a tíltás időhöz kötött, akkor a tiltás kezdete, egyébkét NULL';
COMMENT ON COLUMN host_service_noalarms.noalarm_to IS 'Ha a tíltás időhöz kötött, akkor a tiltás vége, egyébkét NULL';
COMMENT ON COLUMN host_service_noalarms.user_id IS 'A tíltást kiadó felhasználó azonosítója.';

-- --------------------------------------------------------------------------------------------------

CREATE TABLE app_memos (
    app_memo_id         bigserial       PRIMARY KEY,
    date_of             timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    app_name            text            DEFAULT NULL,
    pid                 bigint          DEFAULT NULL,
    thread_name         text            DEFAULT NULL,
    app_ver             text            DEFAULT NULL,
    lib_ver             text            DEFAULT NULL,
    func_name           text            DEFAULT NULL,
    src_name            text            DEFAULT NULL,
    src_line            integer         DEFAULT NULL,
    node_id             bigint          DEFAULT NULL,
    host_service_id     bigint          DEFAULT NULL
        REFERENCES host_services(host_service_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    user_id             bigint          DEFAULT NULL
        REFERENCES users(user_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    importance          notifswitch     DEFAULT 'unknown',
    memo                text            NOT NULL,
    acknowledged        boolean         DEFAULT false
);
CREATE INDEX app_memos_date_of_index ON app_memos (date_of);
ALTER TABLE app_memos OWNER TO lanview2;
COMMENT ON TABLE  app_memos IS '';


CREATE TYPE isnoalarm AS ENUM ('on', 'expired', 'off');
ALTER TYPE isnoalarm OWNER TO lanview2;

CREATE OR REPLACE FUNCTION is_noalarm(flg noalarmtype, tm_from timestamp, tm_to timestamp, tm timestamp DEFAULT CURRENT_TIMESTAMP) RETURNS isnoalarm AS $$
BEGIN
    CASE
        WHEN  flg = 'off'                                       THEN RETURN  'off';      -- Nincs tiltás
        WHEN  flg = 'on'                                        THEN RETURN  'on';      -- Időkorlát nélküli tiltás
        WHEN (flg = 'to' OR flg = 'from_to') AND tm >  tm_to    THEN RETURN  'expired'; -- Már lejárt
        WHEN  flg = 'to'                     AND tm <= tm_to    THEN RETURN  'on';      -- Tiltás
        WHEN                                     tm >= tm_from  THEN RETURN  'on';      -- Tiltás
        WHEN                                     tm <  tm_from  THEN RETURN  'off';     -- Még nem lépett életbe
    END CASE;
END
$$ LANGUAGE plpgsql;

-- host_services rekord ellenörzése
CREATE OR REPLACE FUNCTION check_host_services() RETURNS TRIGGER AS $$
DECLARE
    id      bigint;
    msk     text;
    sn      text;
    cset    boolean   := FALSE;
    nulltd  timestamp := '2000-01-01 00:00';
BEGIN
    IF TG_OP = 'UPDATE' THEN
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
Ellenörzi, hogy a node_id valóban egy nodes vagy snmpdevices rekordot azonosít-e.
Ha port_id nem NULL, ellenörzi, hogy a node_id álltal azonosított objektum portja-e.
Ellenörzi a noalarm_flag, noalarm_from és noalarm_to mezők konzisztenciáját. Ha a két
időadat közöl valamelyik, felesleges, akkor törli azt, ha hiányos akkor kizárást generál.
Ha az idöadatok alapján a noalarm_flag már lejárt, akkor a noalarm_flag "off" lessz, és törli
mindkét időadatot.
Insert esetén, ha a superior_host_service_host_name értéke NULL, akkor egy find_superior() hívással
megkísérli kitölteni azt.
Ha ha a superior_host_service_host_name értéke nem NULL, és rekord beszúrás történt, vagy superior_host_service_host_name
megváltozott, akkor ellenörzi, hogy megfelel-e a services.superior_service_mask -mintának a hivatkozott szervíz neve.
';


CREATE TRIGGER host_services_check_reference_node_id BEFORE UPDATE OR INSERT ON host_services FOR EACH ROW EXECUTE PROCEDURE check_host_services();

-- Beállítja a superior id-ket
CREATE OR REPLACE FUNCTION find_superior(
    hsrv  host_services,            -- A frissítendő host_services rekord
    _hsnm text DEFAULT NULL, -- Superior service_name
    _ptyp text DEFAULT NULL  -- A host port típusa ami linkel a superior_host -hoz
) RETURNS host_services AS $$
DECLARE
    r    record;
    hsnm text := _hsnm;
    ptyp text := _ptyp;
    serv text;
    rres bigint;
    nid  bigint;
BEGIN
    IF hsnm IS NULL THEN
        hsnm := superior_service_mask FROM services WHERE service_id = hsrv.service_id;
    END IF;
    IF ptyp IS NULL THEN
        ptyp := substring(
            (SELECT features FROM services WHERE service_id = hsrv.service_id)
            FROM E'\\:iftype\\=(.*)\\:');
    END IF;
    IF hsnm IS NULL OR ptyp IS NULL THEN
--        RAISE INFO 'find_superior(%,%)', hsnm, ptyp;
        RETURN hsrv;
    END IF;
    nid = hsrv.node_id;
    RAISE INFO 'Simple search. node_id = %, ptype = %, hsnm = %', nid, ptyp, hsnm;
    SELECT suphstsrv.host_service_id, suphstsrv.node_id INTO r
        FROM nports         slport
        JOIN iftypes        slprtyp   ON slprtyp.iftype_id = slport.iftype_id
        JOIN log_links      loglink   ON loglink.port_id1  = slport.port_id
        JOIN nports         maport    ON maport.port_id    = loglink.port_id2
        JOIN host_services  suphstsrv ON suphstsrv.node_id = maport.node_id
        JOIN services       supsrv    ON supsrv.service_id = suphstsrv.service_id
      WHERE slport.node_id = nid AND slprtyp.iftype_name = ptyp AND supsrv.service_name ~ hsnm;
    GET DIAGNOSTICS rres = ROW_COUNT;
    RAISE INFO '(simple) ROW_COUNT is %, FOUND is %', rres, FOUND;
    IF rres = 0 THEN
        RAISE INFO 'Search acros one hub';
        SELECT shs.host_service_id, shs.node_id INTO r
            FROM nports         ap
            JOIN iftypes        apt ON ap.iftype_id = apt.iftype_id
            JOIN log_links      ll  ON ll.port_id1 = ap.port_id
            JOIN nports         hp  ON ll.port_id2 = hp.port_id
            JOIN nports         shp ON shp.node_id = hp.node_id
            JOIN log_links      sll ON shp.port_id = sll.port_id1
            JOIN nports         sp  ON sp.port_id  = sll.port_id2
            JOIN host_services  shs ON shs.node_id = sp.node_id
            JOIN services       ss  ON shs.service_id = ss.service_id
            WHERE ap.node_id = nid AND apt.iftype_name = ptyp AND ss.service_name ~ hsnm;
        GET DIAGNOSTICS rres = ROW_COUNT;
        RAISE INFO '(complex) ROW_COUNT is %', rres;
    END IF;
    IF rres > 1 THEN
       PERFORM error('Ambiguous', hsrv.host_service_id, ptyp || ',' || hsnm, 'set_superior');
    ELSIF rres = 0 THEN
       PERFORM error('WNotFound', hsrv.host_service_id, ptyp || ',' || hsnm, 'set_superior');
       RETURN hsrv;
    END IF;
    hsrv.superior_host_service_id := r.host_service_id;
    RETURN hsrv;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_superior(
    hsid  bigint,            -- Az frissítendő host_services rekord
    hsnm text DEFAULT NULL, -- Superior service_name
    ptyp text DEFAULT NULL  -- A host port típusa ami linkel a superior_host -hoz
) RETURNS boolean AS $$
DECLARE
    hsrv  host_services;            -- Az frissítendő host_services rekord
BEGIN
    SELECT * INTO hsrv FROM host_services WHERE host_service_id = hsid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hsid, 'host_service_id', 'set_superior()', 'host_services');
    END IF;
    IF hsrv.superior_host_service_id IS NULL THEN
        hsrv := find_superior(hsrv, hsnm, ptyp);
    ELSE
        RETURN FALSE;
    END IF;
    IF hsrv.superior_host_service_id IS NULL THEN
        RETURN FALSE;
    END IF;
    UPDATE host_services SET
            superior_host_service_id = hsrv.host_service_id
        WHERE host_service_id = hsid;
    RETURN TRUE;
END
$$ LANGUAGE plpgsql;




