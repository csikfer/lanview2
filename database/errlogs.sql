
-- ---------------------------------------------------------------------------
-- Apolication errors
CREATE TABLE app_errs (
    applog_id   bigserial          PRIMARY KEY,
    date_of     timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    app_name    varchar(32)     DEFAULT NULL,
    node_id     bigint         DEFAULT NULL,	-- Ezt nem tölti ki senki !!!
    pid         bigint         DEFAULT NULL,
    app_ver     varchar(32)     DEFAULT NULL,
    lib_ver     varchar(32)     DEFAULT NULL,
    thread_name varchar(32)     DEFAULT NULL,
    err_code    bigint         DEFAULT NULL,
    err_name    varchar(32)     DEFAULT NULL,
    err_subcode bigint         DEFAULT NULL,
    err_msg     text            DEFAULT NULL,
    errno       bigint         DEFAULT NULL,
    func_name   varchar(255)    DEFAULT NULL,
    func_src    varchar(255)    DEFAULT NULL,
    src_line    bigint         DEFAULT NULL
);
CREATE INDEX app_errs_date_of_index ON app_errs (date_of);
ALTER TABLE app_errs OWNER TO lanview2;
COMMENT ON TABLE  app_errs IS
'Aplication errors log table
Program hiba esetén jön létre (C++ Qt alkalmazás/szerver) egy cError*
típusu kizárás generálásakor, ha a program képes a rekordot létrehozni.
A rekord a cError objektum alapján kerül kitöltésre.'
;
COMMENT ON COLUMN app_errs.applog_id   IS 'Unique ID for app_errs';
COMMENT ON COLUMN app_errs.date_of     IS 'Record timestamp';
COMMENT ON COLUMN app_errs.app_name    IS 'Aplication name';
COMMENT ON COLUMN app_errs.app_ver     IS 'Aplication version identifier';
COMMENT ON COLUMN app_errs.lib_ver     IS 'liblv2 version identifier';
COMMENT ON COLUMN app_errs.node_id     IS 'Host azonosító, vagy NULL ha nem ismert,';
COMMENT ON COLUMN app_errs.pid         IS 'Aplication process PID number';
COMMENT ON COLUMN app_errs.thread_name IS 'Aplication thread identifier (optional)';
COMMENT ON COLUMN app_errs.err_code    IS 'Error code';
COMMENT ON COLUMN app_errs.err_name    IS 'Error symbolic code';
COMMENT ON COLUMN app_errs.err_subcode IS 'Error sub code, or integer parameter';
COMMENT ON COLUMN app_errs.err_msg     IS 'Error message or string parameter';
COMMENT ON COLUMN app_errs.errno       IS 'Sytem errno (errno actual value, nam feltétlenül kapcsolódik a hibához)';
COMMENT ON COLUMN app_errs.func_name   IS 'Function full name.';
COMMENT ON COLUMN app_errs.func_src    IS 'Code source name.';
COMMENT ON COLUMN app_errs.src_line    IS 'Code source line number.';

CREATE OR REPLACE FUNCTION app_err_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    name TEXT;
BEGIN
    SELECT app_name || ':' || err_name INTO name
        FROM app_errs
        WHERE applog_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'applog_id', 'app_err_id2name()', 'app_errs');
    END IF;
    RETURN name;
END;
$$ LANGUAGE plpgsql;


CREATE TYPE errtype AS ENUM ('Fatal', 'Error', 'Warning', 'Ok');
ALTER TYPE errtype OWNER TO lanview2;
COMMENT ON TYPE errtype IS
'Hiba sújossága
Fatal   Fatális hiba
Error   Sújos hiba
Warning Figyelmeztetés, nincs kizárás
Ok      Nem hiba, ''Info''
';

CREATE TABLE errors (
    error_id    bigserial      PRIMARY KEY,
    error_name  varchar(32) UNIQUE,
    error_note varchar(255),
    error_type  errtype     NOT NULL
);
ALTER TABLE errors OWNER TO lanview2;
COMMENT ON TABLE  errors            IS 'Adatbázis műveletek közben keletkező hiba típusok táblája.';
COMMENT ON COLUMN errors.error_id   IS 'Hiba típus ID.';
COMMENT ON COLUMN errors.error_name IS 'Hiba típus név.';
COMMENT ON COLUMN errors.error_note IS 'Hiba típus leírása, hiba üzenet szövege.';
COMMENT ON COLUMN errors.error_type IS 'Hiba sújossága.';

INSERT INTO errors
    ( error_id, error_name, error_type, error_note) VALUES
    (  0,   'Ok',           'Ok',       'O.K.');
INSERT INTO errors
    ( error_name,     error_type, error_note) VALUES
    ( 'Start',        'Ok',       'Start program or service '),
    ( 'ReStart',      'Ok',       'Restart program or service '),
    ( 'Info',         'Ok',       'Info '),
    ( 'WParams',      'Warning',  'Parameter(s) warning '),
    ( 'WNotFound',    'Warning',  'Warning, data not found '),
    ( 'WDisabled',    'Warning',  'Warning, tiltott művelet '),
    ( 'DataWarn',     'Warning',  'Inconsisten data warning '),
    ( 'DropData',     'Warning',  'Az adat eldobásra került '),
    ( 'RunTime',      'Warning',  'Futás idejű hiba '),
    ( 'SysDataErr',   'Fatal',    'System data Error in % table.'),
    ( 'UnknErrorId',  'Fatal',    'Unknown error id '),
    ( 'IdNotUni',     'Error',    'Record ID is not unique '),
    ( 'NameNotUni',   'Error',    'Record name is not unique '),
    ( 'InvRef',       'Error',    'Invalid foreign key value '),
    ( 'DataError',    'Error',    'Data or program error '),
    ( 'Constant',     'Error',    'Modify constant data '),
    ( 'Params',       'Error',    'Parameter(s) error '),
    ( 'UserName',     'Error',    'Invalid User Name '),
    ( 'NameNotFound', 'Error',    'Name is not found '),
    ( 'IdNotFound',   'Error',    'Id is not found '),
    ( 'InvalidOp',    'Error',    'Invalid operation '),
    ( 'InvalidNAddr', 'Error',    'Invalid sub net address '),
    ( 'NotFound',     'Error',    'A keresett adat nem létezik '),
    ( 'Ambiguous',    'Error',    'Nem egyértelmű adat '),
    ( 'Collision',    'Error',    'Ötközés '),
    ( 'Loop',         'Error',    'Túl sok iteráció vagy körbe hivatkozás '),
    ( 'Disabled',     'Error',    'Tiltott művelet '),
    ( 'NotNull',      'Error',    'Az adat nem lehet NULL '),
    ( 'OutOfRange',   'Error',    'Data Out off range ');

-- CREATE SEQUENCE db_errs_dblog_id_seq;
CREATE TABLE db_errs (
    dblog_id    bigserial          PRIMARY KEY,
    date_of     timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    error_id    bigint         NOT NULL
        REFERENCES errors(error_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    user_id     bigint         NOT NULL DEFAULT 0, -- REFERENCES users(user_id) még nincs definiálva, DEFAULT = nobody
    table_name   varchar(64)     DEFAULT NULL,
    trigger_op  varchar(8)      DEFAULT NULL,
    err_subcode bigint         DEFAULT NULL,
    err_msg  varchar(255)    DEFAULT NULL,
    func_name    varchar(255)    DEFAULT NULL,
    reapeat     bigint         DEFAULT 0,
    date_of_last timestamp 	NOT NULL DEFAULT CURRENT_TIMESTAMP,
    acknowledged boolean 	DEFAULT false);
CREATE INDEX db_errs_date_of_index      ON db_errs (date_of);
CREATE INDEX db_errs_date_of_last_index ON db_errs (date_of_last);
ALTER TABLE db_errs OWNER TO lanview2;
COMMENT ON TABLE db_errs IS
'Adatbázis műveletek közben keletkező hiba események táblája.
A rekord az erroer(...) függvénnyel hozható létre, függetlenül az aktuális tranzakciótól';
COMMENT ON COLUMN db_errs.date_of IS 'Az esemény bekövetkeztének az időpontja';
COMMENT ON COLUMN db_errs.error_id IS 'A hiba azonosító';
COMMENT ON COLUMN db_errs.user_id IS 'A hiba bekövetkezésekor az aktuális kapcsolathoz tartozó felhasználó azonosítója';
COMMENT ON COLUMN db_errs.table_name IS 'A hib a eseményhez kapcsolódó tábla neve.';
COMMENT ON COLUMN db_errs.trigger_op IS 'Ha a hiba egy TRIGGER függvényben keletkezett, akko a triggert kiváltó esemény neve.';
COMMENT ON COLUMN db_errs.err_subcode IS 'Másodlagos hiba kód, vagy numerikus hiba paraméter.';
COMMENT ON COLUMN db_errs.err_msg IS 'Másodlagos hiba üzenet, vagy szöveges hiba paraméter.';
COMMENT ON COLUMN db_errs.func_name IS 'Az aktuális függvény neve, ahol a hiba történt.';
COMMENT ON COLUMN db_errs.reapeat IS 'Ha kétszer nyugtázatlan azonos rekord keletkezne, akkor csak ez a számláló inkrementálódik.';
COMMENT ON COLUMN db_errs.date_of_last IS 'Ha reapeat értéke 0, akkor azonos date_of-al, reapeat inkrementálásakor az aktuális idő kerül a mezőbe.';

CREATE OR REPLACE FUNCTION db_err_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    name TEXT;
BEGIN
    SELECT func_name || ':' || err_name INTO name
        FROM db_errs JOIN errors USING(error_id)
        WHERE dblog_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, 'dblog_id', 'db_err_id2name()', 'db_errs');
    END IF;
    RETURN name;
END;
$$ LANGUAGE plpgsql;

-- -------------------------------
-- ----- Functions, TRIGGERs -----
-- -------------------------------

CREATE OR REPLACE FUNCTION db_error_chk_reapeat() RETURNS TRIGGER AS $$
DECLARE
    err db_errs; -- Időben az előző hiba rekord
BEGIN
    IF NEW.acknowledged THEN
	RETURN NEW;
    END IF;
    SELECT * INTO err FROM db_errs
	WHERE NOT acknowledged
	 AND error_id    = NEW.error_id
	 AND user_id     = NEW.user_id
	 AND table_name   = NEW.table_name
	 AND trigger_op  = NEW.trigger_op
	 AND err_subcode = NEW.err_subcode
	 AND err_msg  = NEW.err_msg
	 AND func_name    = NEW.func_name
	ORDER BY date_of_last DESC LIMIT 1;
    IF FOUND THEN
        UPDATE db_errs SET reapeat = err.reapeat +1, date_of_last = NOW() WHERE dblog_id = err.dblog_id;
        RETURN NULL;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION db_error_chk_reapeat() IS 'Trigger függvény az db_ers táblához, hogy ne legyenek ismételt rekordok.';
CREATE TRIGGER db_errors_before_insert BEFORE INSERT ON db_errs FOR EACH ROW EXECUTE PROCEDURE db_error_chk_reapeat();
COMMENT ON TRIGGER db_errors_before_insert ON db_errs IS
'Ha két azonos nyugtázatlan rekordot kéne rögzíteni (date_of és reapeat mezők kivételével),
akkor a megelőző rekordnak csak a reapeat mezője lessz inkrementálva, és a date_of_last aktualizálva,
az új rekord viszont nem kerül rögzítésre.
Ha az új rekordban az acknowledged mező igaz, akkor mindenképpen rógzítve lessz az új rekord';

-- ERROR functions
-- Get Error recod by name
CREATE OR REPLACE FUNCTION error_by_name(varchar(32)) RETURNS errors AS $$
DECLARE
    err errors%ROWTYPE;
BEGIN
    SELECT * INTO err FROM errors WHERE error_name = $1;
    IF NOT FOUND THEN
        SELECT * INTO err FROM errors WHERE error_name = 'UnknErrorId';
        IF NOT FOUND THEN
            RAISE EXCEPTION 'Sytem data error in errors table, original error code is %', $1;
        END IF;
        err.error_note := err.error_note || ' #' || $1;
    END IF;
    RETURN err;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION error_by_name(varchar(32)) IS 'Egy hiba típus rekord beolvasása a hiba típus név alapján';

-- Get Error recod by ID
CREATE OR REPLACE FUNCTION error_by_id(bigint) RETURNS errors AS $$
DECLARE
    err errors%ROWTYPE;
BEGIN
    SELECT * INTO err FROM errors WHERE error_id = $1;
    IF NOT FOUND THEN
        SELECT * INTO err FROM errors WHERE error_name = 'UnknErrorId';
        IF NOT FOUND THEN
            RAISE EXCEPTION 'Sytem data error in errors table, original error code is %', $1;
        END IF;
    END IF;
    RETURN err;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION error_by_id(bigint) IS 'Egy hiba tipus rekord beolvasása a hiba tipus azonosító alapján';

-- Get Error ID by name
CREATE OR REPLACE FUNCTION error_name2id(varchar(32)) RETURNS bigint AS $$
DECLARE
    err errors%ROWTYPE;
BEGIN
    err := error_by_name($1);
    return err.error_id;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION error_name2id(varchar(32)) IS 'Hiba tipus azonosító a név alapján';
-- Get Error description by name
CREATE OR REPLACE FUNCTION error_name2descr(varchar(32)) RETURNS varchar(255) AS $$
DECLARE
    err errors%ROWTYPE;
BEGIN
    err := error_by_name($1);
    return err.error_note;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION error_name2descr(varchar(32)) IS 'Hiba tipus leírás a név alapján';

CREATE OR REPLACE FUNCTION error (
    text,                   -- $1 Error name (errors.err_name)
    bigint DEFAULT -1,     -- $2 Error Sub code
    text    DEFAULT 'nil',  -- $3 Error Sub Message
    text    DEFAULT 'nil',  -- $4 Source name (function name, ...)
    text    DEFAULT 'nil',  -- $5 Actual table name
    text    DEFAULT 'no'    -- $6 Actual TRIGGER type
) RETURNS boolean AS $$
DECLARE
    er errors%ROWTYPE;
    ui text;
    cmd text;
    con CONSTANT text := 'errlog';
    subc bigint := $2;
    subm text    := $3;
    srcn text    := $4;
    tbln text    := $5;
    trgn text    := $6;
BEGIN
    IF subc IS NULL THEN subc := -1;    END IF;
    IF subm IS NULL THEN subm := 'nil'; END IF;
    IF srcn IS NULL THEN srcn := 'nil'; END IF;
    IF tbln IS NULL THEN tbln := 'nil'; END IF;
    IF trgn IS NULL THEN trgn := 'no';  END IF;
    -- RAISE NOTICE 'called: error(%,%,%,%,%,%) ...', $1,subc,subm,srcn,tbln,trgn;
    er := error_by_name($1);
    PERFORM set_config('lanview2.last_error_code', CAST(er.error_id AS text), false);
    SELECT current_setting('lanview2.user_id') INTO ui;
    IF ui = '-1' THEN
        ui = '0';	-- nobody
    END IF;
    -- Tranzakción kívülröl kel (dblink-el) kiírni a log rekordot, mert visszagörgetheti
    PERFORM dblink_connect(con, 'dbname=lanview2');
    cmd := 'INSERT INTO db_errs'
     || '(error_id, user_id, table_name, trigger_op, err_subcode, err_msg, func_name) VALUES ('
     || er.error_id   || ',' || ui || ',' || quote_nullable(tbln) || ','
     || quote_nullable(trgn) || ',' || subc || ',' || quote_nullable(subm) || ',' || quote_nullable(srcn)
     || ')';
    RAISE NOTICE 'Error log :  "%"', cmd;
    PERFORM dblink_exec(con, cmd, false);
    PERFORM dblink_disconnect(con);
    -- dblink vége
    CASE er.error_type
        WHEN 'Ok' THEN
            RAISE INFO 'Info %, #% %',       $1, subc, (er.error_note || subm);
        WHEN 'Warning', 'Ok' THEN
            RAISE WARNING 'WARNING %, #% %', $1, subc, (er.error_note || subm);
     -- WHEN 'Fatal', 'Error' THEN
        ELSE
            RAISE EXCEPTION 'ERROR %, #% %', $1, subc, (er.error_note || subm);
    END CASE;
    RETURN true;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION error (text,bigint,text,text,text,text) IS
'Adatbázis műveleti hiba rekord generálása.
Létrehoz egy db_errs rekordot a megadott paraméterek, és lanview2 konfigurációs változók aktuális értéke alapján.
Ha a megadott hiba név típusa nem  ''Ok'', vagy ''Warning'', akkor dob egy kizárást. egyébként true-val visszatér.
    text,                   Hiba típus neve
    bigint DEFAULT -1,     Másodlagos hiba kód, vagy numerikus paraméter
    text    DEFAULT ''nil'',  Másodlagos hiba üzenet, vagy szöveges paraméter
    text    DEFAULT ''nil'',  A forrás azonosítója pl. függvény neve
    text    DEFAULT ''nil'',  Az aktuális tábla neve
    text    DEFAULT ''no''    TRIGGER típusa vagy ''no''';

CREATE OR REPLACE VIEW db_errors AS
    SELECT date_of, reapeat, error_name, error_type, error_note,
           user_id, table_name, trigger_op, err_subcode, err_msg, func_name
    FROM db_errs JOIN errors USING(error_id);


CREATE OR REPLACE FUNCTION insert_error (
    text,                   -- $1 Error name (errors.err_name)
    bigint DEFAULT -1,     -- $2 Error Sub code
    text    DEFAULT 'nil',  -- $3 Error Sub Message
    text    DEFAULT 'nil',  -- $4 Source name (function name, ...)
    text    DEFAULT 'nil',  -- $5 Actual table name
    text    DEFAULT 'ext'   -- $6 Actual TRIGGER type
) RETURNS db_errs AS $$
DECLARE
    er errors%ROWTYPE;
    ui   bigint;
    subc bigint := $2;
    subm text    := $3;
    srcn text    := $4;
    tbln text    := $5;
    trgn text    := $6;
    re  db_errs%ROWTYPE;
BEGIN
    IF subc IS NULL THEN subc := -1;    END IF;
    IF subm IS NULL THEN subm := 'nil'; END IF;
    IF srcn IS NULL THEN srcn := 'nil'; END IF;
    IF tbln IS NULL THEN tbln := 'nil'; END IF;
    IF trgn IS NULL THEN trgn := 'ext'; END IF;
    er := error_by_name($1);
    SELECT current_setting('lanview2.user_id') INTO ui;
    IF ui = '-1' THEN
        ui := 0;	-- nobody
    END IF;
    INSERT INTO db_errs
	      (error_id, user_id, table_name, trigger_op, err_subcode, err_msg, func_name)
	   VALUES
	      (er.error_id, ui, tbln, trgn, subc, subm, srcn)
	   RETURNING * INTO re;
    RETURN re;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION error (text,bigint,text,text,text,text) IS
'Egy hiba rekord rögzítése. Egy adatbázis tartalmi hiba rögzítése, ha az egy aplikációban derült ki, ill. azt az app a függvény hívással jelzi.';
