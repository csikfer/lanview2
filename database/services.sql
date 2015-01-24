-- //// LAN.IPPROTOCOLLS

CREATE TABLE ipprotocols (
    protocol_id     bigint     PRIMARY KEY,    -- == protocol number
    protocol_name   varchar(32) NOT NULL UNIQUE
);
ALTER TABLE ipprotocols OWNER TO lanview2;

INSERT INTO ipprotocols (protocol_id, protocol_name) VALUES
    ( -1, 'nil' ),  --No network
    (  0, 'ip'  ),
    (  1, 'icmp'),
    (  6, 'tcp' ),
    ( 17, 'udp' );

-- //// LAN.SERVICES

CREATE TABLE services (
    service_id              bigserial      PRIMARY KEY,
    service_name            varchar(32)    NOT NULL,
    service_note            varchar(255)   DEFAULT NULL,
    service_alarm_msg       varchar(255)   DEFAULT NULL,
    protocol_id             bigint         DEFAULT -1  -- nil
        REFERENCES ipprotocols(protocol_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    port                    integer        DEFAULT NULL,
    superior_service_mask   varchar(64)    DEFAULT NULL,
    check_cmd               varchar(255)   DEFAULT NULL,
    properties              varchar(255)   DEFAULT ':',
    disabled                boolean        NOT NULL DEFAULT FALSE,
    max_check_attempts      integer        DEFAULT NULL,
    normal_check_interval   interval       DEFAULT NULL,
    retry_check_interval    interval       DEFAULT NULL,
    timeperiod_id           bigint         NOT NULL DEFAULT 0,  -- DEFAULT 'always'
    flapping_interval       interval       NOT NULL DEFAULT '30 minutes',
    flapping_max_change     integer        NOT NULL DEFAULT 15,
    UNIQUE (service_name, protocol_id)
);
ALTER TABLE services OWNER TO lanview2;
COMMENT ON TABLE  services                  IS 'Services table';
COMMENT ON COLUMN services.service_id       IS 'Egyedi azonosító';
COMMENT ON COLUMN services.service_name     IS 'Szervice name';
COMMENT ON COLUMN services.service_note     IS 'Megjegyzés';
COMMENT ON COLUMN services.protocol_id      IS 'Ip protocol id (-1 : nil, if no ip protocol)';
COMMENT ON COLUMN services.port             IS 'Default (TCO, UDP, ...) port number. or NULL';
COMMENT ON COLUMN services.check_cmd        IS 'Default check command';
COMMENT ON COLUMN services.properties       IS
'Default paraméter lista (szeparátor a kettőspont, paraméter szeparátor az ''='', első és utolsó karakter a szeparátor):\n
daemon      Az szolgáltatás ellenörzése egy daemon program, paraméterek:\n
    respawn     daemon paraméter: a programot újra kell indítani, ha kilép. A kilépés nem hiba.\n
    continue    daemon paraméter: a program normál körülmények között nem lép ki, csak ha leállítják, vagy hiba van. (default)
    polling     daemon paraméter: a programot időzítve kell hívni. He elvégezte az ellenörzést, akkor kilép.
inspector
    timed       Időzített
    thread      Időzített, saját szállal (?!)
    continue    Saját szál belső ütemezés ill. folyamatos (?!)
    passive     Valamilyen lekérdezés (superior) járulékos eredményeként van állpota
superior    Alárendelteket ellenörző eljárásokat hív, szolgál ki (passive)
    <üres>      Alárendelt viszony,autómatikus
    custom      egyedileg kezelt (a cInspector objektum nem ovassa be az alárendelteket, azok egyedileg kezelendőek)
protocol    Ez egy protokolt (is)
mode        Ez egy módszer, sablon, ...
system
    lanview2    Lanview2 modul
    nagios      Egy Nagios plugin
    munin       Egy Munin plugin
ifType      A szolgáltatás hierarhia mely port típus linkjével azonos (paraméter: interface típus neve)
disabled    service_name = icontsrv , csak a host_services rekordban, a szolgáltatás (riasztás) tiltva.
reversed    service_name = icontsrv , csak a host_services rekordban, a port fordított bekötését jelzi.
serial      Serial port paraméterei pl.: "serial=19200 7E1 No"
logrot      Log az stderr-re, log fájlt a superior kezeli. Log fájl rotálása (par1: max méret, par2: arhiv [db])
lognull     A superior az stderr és stdout-ot a null-ba irányítja. A szolgáltatás önállóan loggol.
tcp         Alternatív TCP port megadása, paraméter a port száma
udp         Alternatív UDP port megadása, paraméter a port száma
';
COMMENT ON COLUMN services.max_check_attempts IS 'Hibás eredmények maximális száma, a riasztás kiadása elött. Alapértelmezett érték.';
COMMENT ON COLUMN services.normal_check_interval IS 'Ellenörzések ütemezése, ha nincs hiba. Alapértelmezett érték.';
COMMENT ON COLUMN services.retry_check_interval IS 'Ellenörzések ütemezése, hiba esetén a riasztás kiadásáig. Alapértelmezett érték.';


INSERT INTO services (service_id, service_name, service_note) VALUES
    ( -1, 'nil', 'A NULL-t reprezentálja, de összehasonlítható');

CREATE OR REPLACE FUNCTION service_name2id(varchar(32)) RETURNS bigint AS $$
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
"off"     Az alarm-ok nincsenek letiltva
"on"      Az alarmok le vannak tiltva
"to"      Az alarmok egy megadott időpontig le vannak/voltak tiltva
"from"    Az alarmok egy időpontól le lesznek/le vannak tiltva
"from_to" Az alarmok egy tőintervallumban le lesznek/vannak/voltak tiltva';

CREATE TABLE host_services (
    host_service_id         bigserial      PRIMARY KEY,
    node_id                 bigint         NOT NULL,
    --  REFERENCES nodess(node_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    service_id              bigint         NOT NULL
        REFERENCES services(service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    port_id                 bigint         DEFAULT NULL,
    host_service_note       varchar(255)   DEFAULT NULL,
    host_service_alarm_msg  varchar(255) DEFAULT NULL,
    prime_service_id        bigint         DEFAULT -1       -- nil
        REFERENCES services(service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE RESTRICT,
    proto_service_id        bigint         DEFAULT -1       -- nil
        REFERENCES services(service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE RESTRICT,
    --  REFERENCES interfaces(port_id) MATCH SIMPLE
    delegate_host_state     boolean        NOT NULL DEFAULT FALSE,
    check_cmd               varchar(255)   DEFAULT NULL,
    properties              varchar(255)   DEFAULT NULL,
    disabled                boolean        NOT NULL DEFAULT FALSE,
    superior_host_service_id bigint        DEFAULT NULL
        REFERENCES host_services(host_service_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    max_check_attempts      integer        DEFAULT NULL,
    normal_check_interval   interval       DEFAULT NULL,
    retry_check_interval    interval       DEFAULT NULL,
    timeperiod_id           bigint         DEFAULT NULL   -- DEFAULT 'always'
        REFERENCES timeperiods(timeperiod_id) MATCH SIMPLE ON DELETE RESTRICT ON UPDATE RESTRICT,
    flapping_interval       interval       DEFAULT NULL,
    flapping_max_change     integer        DEFAULT NULL,
    noalarm_flag            noalarmtype    NOT NULL DEFAULT 'off',
    noalarm_from            timestamp      DEFAULT NULL,
    noalarm_to              timestamp      DEFAULT NULL,
    offline_group_id        bigint[]       DEFAULT NULL,
    online_group_id         bigint[]       DEFAULT NULL,
-- Állapot
    host_service_state      notifswitch    NOT NULL DEFAULT 'unknown',
    soft_state              notifswitch    NOT NULL DEFAULT 'unknown',
    hard_state              notifswitch    NOT NULL DEFAULT 'unknown',
    state_msg               varchar(255)   DEFAULT NULL,
    check_attempts          integer        NOT NULL DEFAULT 0,
    last_changed            TIMESTAMP      DEFAULT NULL,
    last_touched            TIMESTAMP      DEFAULT NULL,
    act_alarm_log_id        bigint         DEFAULT NULL,   -- REFERENCES alarms(alarm_id)
    last_alarm_log_id       bigint         DEFAULT NULL,   -- REFERENCES alarms(alarm_id)
-- Állapot vége
    deleted                 boolean        NOT NULL DEFAULT FALSE
);
ALTER TABLE host_services OWNER TO lanview2;

CREATE UNIQUE INDEX host_services_port_subservices_key ON host_services (node_id, service_id, COALESCE(port_id, -1), prime_service_id, proto_service_id);
-- CREATE UNIQUE INDEX host_services_node_id_service_id ON host_services(node_id, service_id) WHERE port_id IS NULL;

COMMENT ON TABLE  host_services IS 'A szolgáltatás-node összerendelések, ill. a konkrét szolgáltatások vagy ellenörzés utasítások táblája, és azok állpota.';
COMMENT ON COLUMN host_services.host_service_id IS 'Egyedi azonosító';
COMMENT ON COLUMN host_services.host_service_note IS 'Megjegyzés / leírás.';
COMMENT ON COLUMN host_services.host_service_alarm_msg IS 'Riasztás esetén egy megjelnítendő üzenet ill. magyarázó szöveg.';
COMMENT ON COLUMN host_services.node_id IS 'A node ill. host azonosítója, amin a szolgáltatás, vagy az ellenörzés fut.';
COMMENT ON COLUMN host_services.service_id IS 'A szolgáltatást, vagy az ellenörzés típusát azonosító ID';
COMMENT ON COLUMN host_services.prime_service_id IS 'Az ellenőrzés elsődleges módszerét azonosító, szervíz típus ID';
COMMENT ON COLUMN host_services.proto_service_id IS 'Az ellenőrzés módszerének opcionális további azonosítása (protokol), szervíz típus ID';
COMMENT ON COLUMN host_services.port_id IS 'Opcionális port azonosító, ha a szolgáltatás ill. ellenörzés egy porthoz rendelt.';
COMMENT ON COLUMN host_services.delegate_host_state IS 'Értéke igaz, ha a szolgáltatás állapotát örökli a node is.';
COMMENT ON COLUMN host_services.check_cmd IS 'Egy opcionális parancs.';
COMMENT ON COLUMN host_services.properties IS 'További paraméterek. Ld.: services.properties . Ha értéke nem NULL, akkor fellülbírálja a service_id -hez tartozó services.properties értékét';
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
            n.node_name || ':' || CASE WHEN p.port_name IS NULL THEN '' ELSE ':' || p.port_name END || s.service_name,
            sprime.service_name,
            sproto.service_name
          INTO name, prime, proto
        FROM host_services hs
        JOIN nodes n USING(node_id)
        JOIN services s USING(service_id)
        LEFT JOIN nports p ON hs.port_id = p.port_id
        JOIN services sproto ON hs.proto_service_id = sproto.service_id
        JOIN services sprime ON hs.prime_service_id = sprime.service_id
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
        REFERENCES host_services(host_service_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    date_of             timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    old_state           notifswitch     NOT NULL,
    old_soft_state      notifswitch     NOT NULL,
    old_hard_state      notifswitch     NOT NULL,
    new_state           notifswitch     NOT NULL,
    new_soft_state      notifswitch     NOT NULL,
    new_hard_state      notifswitch     NOT NULL,
    event_note          varchar(255)    DEFAULT NULL,
    superior_alarm      bigint          DEFAULT NULL,
    noalarm             boolean         NOT NULL
);
CREATE INDEX host_service_logs_date_of_index ON host_service_logs (date_of);
ALTER TABLE host_service_logs OWNER TO lanview2;
COMMENT ON TABLE host_service_logs IS 'Hoszt szervízek státusz változásainak a log táblája';

CREATE TABLE host_service_noalarms (
    host_service_noalarm_id bigserial   PRIMARY KEY,
    date_of                 timestamp   DEFAULT CURRENT_TIMESTAMP,
    host_service_id         bigint      REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    noalarm_flag            noalarmtype NOT NULL,
    noalarm_from            timestamp   DEFAULT NULL,
    noalarm_to              timestamp   DEFAULT NULL,
    user_id                 bigint      DEFAULT NULL REFERENCES users(user_id) MATCH SIMPLE
);
CREATE INDEX host_service_noalarms_date_of_index ON host_service_noalarms (date_of);
ALTER TABLE host_service_noalarms OWNER TO lanview2;
COMMENT ON TABLE host_service_noalarms IS 'Hoszt szervízek riasztásainak a letiltását loggoló tábla';
COMMENT ON COLUMN host_service_noalarms.date_of IS 'A tiltás megadásának az időpontja';
COMMENT ON COLUMN host_service_noalarms.host_service_id IS 'A tiltott szolgáltatás azonosító';
COMMENT ON COLUMN host_service_noalarms.noalarm_flag IS 'A tiltás típusa';
COMMENT ON COLUMN host_service_noalarms.noalarm_from IS 'Ha a tíltás időhöz kötött, akkor a tiltás kezdete, egyébkét NULL';
COMMENT ON COLUMN host_service_noalarms.noalarm_to IS 'Ha a tíltás időhöz kötött, akkor a tiltás vége, egyébkét NULL';
COMMENT ON COLUMN host_service_noalarms.user_id IS 'A tíltást kiadó felhasználó azonosítója.';

CREATE TABLE host_service_charts (
    host_service_chart_id bigserial    PRIMARY KEY,
    host_service_id     bigint         REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    rrd_file_name       varchar(255)   DEFAULT NULL,
    graph_order         bigint[]       DEFAULT NULL,
    graph_args          varchar(255)   DEFAULT NULL,
    graph_vlabel        varchar(255)   DEFAULT NULL,
    graph_scale         boolean        DEFAULT NULL,   -- ??
    graph_info          varchar(255)   DEFAULT NULL,   -- ??
    graph_category      varchar(255)   DEFAULT NULL,   -- ??
    graph_period        varchar(255)   DEFAULT NULL,   -- ??
    graph_height        bigint         DEFAULT 300,
    graph_width         bigint         DEFAULT 600,
    deleted             boolean        NOT NULL DEFAULT FALSE
);
ALTER TABLE host_service_charts OWNER TO lanview2;


CREATE TYPE servicevartype AS ENUM ('DERIVE','COUNTER', 'GAUGE', 'ABSOLUTE', 'COMPUTE');
ALTER TYPE servicevartype OWNER TO lanview2;

CREATE TYPE drawtype AS ENUM ('LINE', 'AREA', 'STACK');
ALTER TYPE drawtype OWNER TO lanview2;

CREATE TABLE host_service_vars (
    service_var_id      bigserial       PRIMARY KEY,
    service_var_name    varchar(32)     NOT NULL,
    service_var_note    varchar(255)    DEFAULT NULL,
    host_service_id     bigint          NOT NULL
        REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    color               bigint          DEFAULT 0,
    service_var_type    servicevartype  DEFAULT 'GAUGE',
    draw_type           drawtype        DEFAULT 'LINE',
    cdef                varchar(255)    DEFAULT NULL,
    negative            boolean         DEFAULT FALSE,
    dim                 varchar(32)     DEFAULT NULL,
    min_value           real            DEFAULT NULL,
    max_value           real            DEFAULT NULL,
    warning_max         real            DEFAULT NULL,
    warning_min         real            DEFAULT NULL,
    critical_max        real            DEFAULT NULL,
    critical_min        real            DEFAULT NULL,
    deleted             boolean         NOT NULL DEFAULT FALSE,
    UNIQUE (host_service_id, service_var_name)
);
ALTER TABLE host_service_vars OWNER TO lanview2;

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
    cset    boolean := FALSE;
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
        IF NEW.noalarm_flag <> OLD.noalarm_flag OR NEW.noalarm_from <> OLD.noalarm_from OR NEW.noalarm_to <> OLD.noalarm_to THEN
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
            IF (NEW.noalarm_flag = 'from_to' AND NEW.noalarm_from > NEW.noalarm_to) OR
               ((NEW.noalarm_flag = 'from_to' OR NEW.noalarm_flag = 'to')  AND  CURRENT_TIMESTAMP > NEW.noalarm_to) THEN
                PERFORM error('OutOfRange', OLD.host_service_id, 'noalarm_to', 'check_host_services()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
            IF NEW.noalarm_flag <> 'off' THEN
                INSERT INTO host_service_noalarms (host_service_id, noalarm_flag, noalarm_from, noalarm_to, user_id)
                    VALUES(NEW.host_service_id, NEW.noalarm_flag, NEW.noalarm_from, NEW.noalarm_to, current_setting('lanview2.user_id')::bigint);
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
                    PERFORM error('Params', OLD.host_service_id, 'superior_host_service_id', 'check_host_services()', TG_TABLE_NAME, TG_OP);
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
    _hsnm varchar(32) DEFAULT NULL, -- Superior service_name
    _ptyp varchar(64) DEFAULT NULL  -- A host port típusa ami linkel a superior_host -hoz
) RETURNS host_services AS $$
DECLARE
    r    record;
    hsnm varchar(32) := _hsnm;
    ptyp varchar(64) := _ptyp;
    serv varchar(32);
    rres bigint;
    nid  bigint;
BEGIN
    IF hsnm IS NULL THEN
        hsnm := superior_service_mask FROM services WHERE service_id = hsrv.service_id;
    END IF;
    IF ptyp IS NULL THEN
        ptyp := substring(
            (SELECT properties FROM services WHERE service_id = hsrv.service_id)
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
    RAISE INFO '(symple) ROW_COUNT is %, FOUND is %', rres, FOUND;
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
    hsnm varchar(32) DEFAULT NULL, -- Superior service_name
    ptyp varchar(64) DEFAULT NULL  -- A host port típusa ami linkel a superior_host -hoz
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


-- ///////////////////////////
-- Views
-- ///////////////////////////

CREATE OR REPLACE VIEW view_host_services AS
    SELECT
        hs.host_service_id          AS host_service_id,
        hs.host_service_note        AS host_service_note,
        hs.host_service_alarm_msg   AS host_service_alarm_msg,
        hs.node_id                  AS node_id,
        n.node_name                 AS node_name,
        hs.service_id               AS service_id,
        s.service_name              AS service_name,
        hs.prime_service_id         AS prime_service_id,
        prime_s.service_name        AS prime_service_name,
        hs.port_id                  AS port_id,
        p.port_name                 AS port_name,
        hs.delegate_host_state      AS delegate_host_state,
        COALESCE(hs.check_cmd,             s.check_cmd)              AS check_cmd,
        s.properties                AS default_properties,
        hs.properties               AS properties,
        hs.superior_host_service_id AS superior_host_service_id,
        superior_hs_h.node_name     AS superior_host_service_host_name,
        superior_hs_s.service_name  AS superior_host_service_service_name,
        COALESCE(hs.max_check_attempts,    s.max_check_attempts)     AS max_check_attempts,
        COALESCE(hs.normal_check_interval, s.normal_check_interval)  AS normal_check_interval,
        COALESCE(hs.retry_check_interval,  s.retry_check_interval)   AS retry_check_interval,
        COALESCE(hs.flapping_interval,     s.flapping_interval)      AS flapping_interval,
        COALESCE(hs.flapping_max_change,   s.flapping_max_change)    AS flapping_max_change,
        hs.timeperiod_id            AS timeperiod_id,
        tp.timeperiod_name          AS timeperiod_name,
        hs.noalarm_flag             AS noalarm_flag,
        hs.noalarm_from             AS noalarm_from,
        hs.noalarm_to               AS noalarm_to,
        hs.offline_group_id         AS offline_group_id,
        hs.online_group_id          AS online_group_id,
        hs.host_service_state       AS host_service_state,
        hs.soft_state               AS soft_state,
        hs.hard_state               AS hard_state,
        hs.check_attempts           AS check_attempts,
        hs.last_changed             AS last_changed,
        hs.last_touched             AS last_touched,
        hs.act_alarm_log_id         AS act_alarm_log_id,
        hs.last_alarm_log_id        AS last_alarm_log_id,
        hs.deleted                  AS deleted
    FROM     host_services      AS  hs
        LEFT JOIN services      AS  s               ON hs.service_id            = s.service_id
        LEFT JOIN nodes         AS  n               ON hs.node_id               = n.node_id
        LEFT JOIN nports        AS  p               ON hs.port_id               = p.port_id
        LEFT JOIN timeperiods   AS  tp              ON hs.timeperiod_id         = tp.timeperiod_id
        LEFT JOIN services      AS  prime_s         ON hs.prime_service_id      = prime_s.service_id
        LEFT JOIN host_services AS  superior_hs     ON hs.superior_host_service_id = superior_hs.host_service_id
        LEFT JOIN nodes         AS  superior_hs_h   ON superior_hs.node_id      = superior_hs_h.node_id
        LEFT JOIN services      AS  superior_hs_s   ON superior_hs.service_id   = superior_hs_s.service_id;

