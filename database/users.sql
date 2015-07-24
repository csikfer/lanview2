-- //// TPOWS

CREATE TYPE dayofweek AS ENUM ('sunday','monday','tuesday','wednesday','thursday','friday','saturday');
ALTER TYPE dayofweek OWNER TO lanview2;
COMMENT ON TYPE dayofweek IS 'Date of week enumeration.';

-- Idő intervallum definíciók egy megadott napon egy héten bellül
CREATE TABLE tpows (
    tpow_id     bigserial       PRIMARY KEY,
    tpow_name   varchar(32)     NOT NULL UNIQUE,
    tpow_note   varchar(255)    DEFAULT NULL,
    dow         dayofweek       NOT NULL,
    begin_time  time            NOT NULL DEFAULT  '0:00',
    end_time    time            NOT NULL DEFAULT '24:00'
);
ALTER TABLE tpows OWNER TO lanview2;
COMMENT ON TABLE  tpows             IS 'One time period of week';
COMMENT ON COLUMN tpows.tpow_id     IS 'Unique ID';
COMMENT ON COLUMN tpows.tpow_name   IS 'Unique name for time intervall of week';
COMMENT ON COLUMN tpows.dow         IS 'Day of week';
COMMENT ON COLUMN tpows.begin_time  IS 'Begin time';
COMMENT ON COLUMN tpows.end_time    IS 'End time';

-- //// TIMEPERIODS

CREATE TABLE timeperiods (
    timeperiod_id       bigserial       PRIMARY KEY,
    timeperiod_name     varchar(32)     NOT NULL UNIQUE,
    timeperiod_note     varchar(255)    DEFAULT NULL
);
ALTER TABLE timeperiods OWNER TO lanview2;
COMMENT ON TABLE  timeperiods                  IS 'Time periods';
COMMENT ON COLUMN timeperiods.timeperiod_id    IS 'Unique ID';
COMMENT ON COLUMN timeperiods.timeperiod_name  IS 'Unique name for time periods';

-- //// TIMEPERIODS_TPOWS

CREATE TABLE timeperiod_tpows (
    timeperiod_tpow_id bigserial  PRIMARY KEY,
    tpow_id         bigint     REFERENCES tpows(tpow_id)  MATCH FULL ON DELETE RESTRICT ON UPDATE CASCADE,
    timeperiod_id   bigint     REFERENCES timeperiods(timeperiod_id)  MATCH FULL ON DELETE RESTRICT ON UPDATE CASCADE,
    UNIQUE(tpow_id,timeperiod_id)
);
ALTER TABLE timeperiod_tpows OWNER TO lanview2;
COMMENT ON TABLE  timeperiod_tpows                 IS 'Time periods and tpows kapcsoló tábla';
COMMENT ON COLUMN timeperiod_tpows.tpow_id         IS 'Unique ID';
COMMENT ON COLUMN timeperiod_tpows.timeperiod_id   IS 'Unique ID';

-- Hét napjainak a nevét numerikus kóddá alakítja
CREATE OR REPLACE FUNCTION dow2int(dayofweek) RETURNS bigint AS $$
BEGIN
    CASE $1
        WHEN 'sunday'    THEN  RETURN 0;
        WHEN 'monday'    THEN  RETURN 1;
        WHEN 'tuesday'   THEN  RETURN 2;
        WHEN 'wednesday' THEN  RETURN 3;
        WHEN 'thursday'  THEN  RETURN 4;
        WHEN 'friday'    THEN  RETURN 5;
        WHEN 'saturday'  THEN  RETURN 6;
    END CASE;
END;
$$ LANGUAGE plpgsql;

-- Hét napjainak a nevét numerikus kóddá alakítja
CREATE OR REPLACE FUNCTION int2dow(bigint) RETURNS dayofweek AS $$
BEGIN
    CASE $1
        WHEN 0  THEN  RETURN 'sunday';
        WHEN 1  THEN  RETURN 'monday';
        WHEN 2  THEN  RETURN 'tuesday';
        WHEN 3  THEN  RETURN 'wednesday';
        WHEN 4  THEN  RETURN 'thursday';
        WHEN 5  THEN  RETURN 'friday';
        WHEN 6  THEN  RETURN 'saturday';
    END CASE;
END;
$$ LANGUAGE plpgsql;



-- // A megadott időpont benne van-e a megadott időintervallumban
CREATE OR REPLACE FUNCTION time_in_timeperiods(bigint, timestamp DEFAULT CURRENT_TIMESTAMP) RETURNS boolean AS $$
BEGIN
    RETURN 0 < COUNT(*) FROM tpows JOIN timeperiods_tpows USING (tpow_id)
        WHERE $1 = timeperiod_id AND int2dow(EXTRACT(DOW FROM $2)) = dow AND $2::time >= begin_time AND $2::time < end_time;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION tpow_name2id(varchar(32)) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT tpow_id INTO id FROM tpows WHERE tpow_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'tpow_name2id(varchar(32))', 'tpows');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION timeperiod_name2id(varchar(32)) RETURNS bigint AS $$
BEGIN
    RETURN timeperiod_id FROM timeperiods WHERE timeperiod_name = $1;
END
$$ LANGUAGE plpgsql;

INSERT INTO tpows(tpow_name, tpow_note, dow) VALUES
    ('all_sun', 'On Sundays all day',    'sunday'   ),
    ('all_mon', 'On Mondays all day',    'monday'   ),
    ('all_tue', 'On Tuesdays all day',   'tuesday'  ),
    ('all_wed', 'On Wednesdays all day', 'wednesday'),
    ('all_thu', 'On Thursdays all day',  'thursday' ),
    ('all_fri', 'On Fridays all day',    'friday'   ),
    ('all_sat', 'On Saturdays all day',  'saturday' );

INSERT INTO timeperiods(timeperiod_id, timeperiod_name, timeperiod_note) VALUES
    ( -1, 'never',  'At no time'   ),
    ( 0,  'always', 'All the time' );

INSERT INTO timeperiod_tpows (tpow_id, timeperiod_id) VALUES
    ( tpow_name2id('all_sun'), 0 ),
    ( tpow_name2id('all_mon'), 0 ),
    ( tpow_name2id('all_tue'), 0 ),
    ( tpow_name2id('all_wed'), 0 ),
    ( tpow_name2id('all_thu'), 0 ),
    ( tpow_name2id('all_fri'), 0 ),
    ( tpow_name2id('all_sat'), 0 );


-- //// USR.USERS

CREATE TYPE notifswitch AS ENUM ('on','recovered','warning','critical','unreachable','down','flapping','unknown');
ALTER TYPE notifswitch OWNER TO lanview2;
COMMENT ON TYPE notifswitch IS 'Notification switch, or host or service status';

CREATE TABLE users (    -- contacts
    user_id             bigserial       PRIMARY KEY,
    user_name           varchar(32)     NOT NULL UNIQUE,
    user_note           varchar(255)    DEFAULT NULL,
    passwd              varchar(64),
    enabled             boolean         DEFAULT TRUE,
    host_notif_period   bigint          DEFAULT 0 REFERENCES timeperiods(timeperiod_id) MATCH FULL
                                            ON DELETE RESTRICT ON UPDATE RESTRICT,
    serv_notif_period   bigint          DEFAULT 0 REFERENCES timeperiods(timeperiod_id) MATCH FULL
                                            ON DELETE RESTRICT ON UPDATE RESTRICT,
    -- ez valójában egy set akar lenni, de csak enum van, ezért tömb.
    host_notif_switchs  notifswitch[]   NOT NULL DEFAULT '{"unreachable","down","recovered","unknown","critical"}',
    -- ez szintén egy set, mint feljebb
    serv_notif_switchs  notifswitch[]   NOT NULL DEFAULT '{"unreachable","down","recovered","unknown","critical"}',
    host_notif_cmd      varchar(255)    DEFAULT NULL,
    serv_notif_cmd      varchar(255)    DEFAULT NULL,
    tels                varchar(20)[]   DEFAULT NULL,
    addresses           varchar(128)[]  DEFAULT NULL,
    place_id            bigint          DEFAULT NULL REFERENCES places(place_id) MATCH SIMPLE
                                            ON DELETE RESTRICT ON UPDATE RESTRICT
);
ALTER TABLE users OWNER TO lanview2;
COMMENT ON TABLE  users                 IS 'Users and contact table';


CREATE TYPE rights AS ENUM ( 'none', 'viewer', 'indalarm', 'operator', 'admin', 'system' );
ALTER TYPE  rights OWNER TO lanview2;

CREATE TABLE groups (
    group_id        bigserial           PRIMARY KEY,
    group_name      varchar(32)         NOT NULL UNIQUE,
    group_note      varchar(255)        DEFAULT NULL,
    group_rights    rights              DEFAULT 'viewer',
    place_group_id  bigint              DEFAULT NULL REFERENCES place_groups(place_group_id) MATCH SIMPLE
                                            ON DELETE RESTRICT ON UPDATE RESTRICT
);
ALTER TABLE groups OWNER TO lanview2;
COMMENT ON TABLE  groups                IS 'User groups';
COMMENT ON COLUMN groups.group_id       IS 'Group ID';
COMMENT ON COLUMN groups.group_name     IS 'Group name';
COMMENT ON COLUMN groups.group_note     IS '';
COMMENT ON COLUMN groups.group_rights   IS 'User rights';
COMMENT ON COLUMN groups.place_group_id IS 'Zone';

CREATE OR REPLACE FUNCTION group_name2id(varchar(32)) RETURNS bigint AS $$
BEGIN
    RETURN group_id FROM groups WHERE group_name = $1;
END
$$ LANGUAGE plpgsql;

CREATE TABLE group_users (
    group_user_id   bigserial  PRIMARY KEY,
    group_id        bigint REFERENCES groups(group_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    user_id         bigint REFERENCES users(user_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    UNIQUE(group_id, user_id)
);
ALTER TABLE group_users OWNER TO lanview2;
COMMENT ON TABLE  group_users           IS 'Group members table';
COMMENT ON COLUMN group_users.group_id  IS 'Group ID';
COMMENT ON COLUMN group_users.user_id   IS 'User ID';

INSERT INTO groups(group_id, group_name, group_note, group_rights) VALUES
    ( 0, 'system',  'system',           'system'  ),
    ( 1, 'admin',   'Administrators',   'admin'   ),
    ( 2, 'operator','Operators',        'operator'),
    ( 3, 'indalarm','IndAlarm users',   'indalarm'),
    ( 4, 'viewer',  'Viewers',          'viewer'  );

SELECT setval('groups_group_id_seq', 5);

INSERT INTO users(user_id, user_name, user_note, passwd) VALUES
    ( 0, 'nobody',  'Unknown user', NULL),
    ( 1, 'system',  'system',       NULL),
    ( 2, 'admin',   'Administrator','admin'),
    ( 3, 'operator','Operator',     'operator'),
    ( 4, 'viewer',  'Viewer',       NULL);

SELECT setval('users_user_id_seq', 5);

CREATE OR REPLACE FUNCTION user_name2id(varchar(32)) RETURNS bigint AS $$
BEGIN
    RETURN user_id FROM users WHERE user_name = $1;
END
$$ LANGUAGE plpgsql;

INSERT INTO group_users ( group_id, user_id ) VALUES
    ( group_name2id('system'),   user_name2id('system') ),
    ( group_name2id('admin'),    user_name2id('admin') ),
    ( group_name2id('operator'), user_name2id('operator') ),
    ( group_name2id('viewer'),   user_name2id('viewer') );



-- Az aktuális felhasználói név beállítása.
-- Ha nincs ilyen felhasználó, akkor dob egy kizárást, és létrehoz egy db_errs rekordot.
-- Ha nincs felhasználó, akkor a következő értékeket állítja be (alapértelmezett értékek):
-- lanview2.user_name   = 'unknown'
-- lanview2.user_id     = '0'
-- lanview2.user_rights = 'none'
-- Ha a paraméterkén megadott felhasználónév valós, akkor annak a paramétereit állítja be a fenti
-- konfigurációs változókba.
CREATE OR REPLACE FUNCTION set_user_name(varchar(32)) RETURNS users AS $$
DECLARE
    ur users%ROWTYPE;
    ri rights;
BEGIN
    SELECT * INTO ur FROM users WHERE user_name = $1;
    IF NOT FOUND THEN
        PERFORM set_config('lanview2.user_name',   'unknown', false);
        PERFORM set_config('lanview2.user_id',     '0',       false);
        PERFORM set_config('lanview2.user_rights', 'none',    false);
        PERFORM error('UserName', -1, $1, 'set_user_name()', 'users');
    ELSE
        SELECT MAX(group_rights) INTO ri FROM group_users JOIN groups USING(group_id) WHERE user_id = ur.user_id;
        PERFORM set_config('lanview2.user_name',   CAST(ur.user_name AS text), false);
        PERFORM set_config('lanview2.user_id',     CAST(ur.user_id   AS text), false);
        PERFORM set_config('lanview2.user_rights', CAST(ri           AS text), false);
    END IF;
    RETURN ur;
END;
$$ LANGUAGE plpgsql;

-- Az aktuális felhasználói id beállítása.
CREATE OR REPLACE FUNCTION set_user_id(bigint DEFAULT NULL) RETURNS users AS $$
DECLARE
    ur users%ROWTYPE;
    ri rights;
BEGIN
    SELECT * INTO ur FROM users WHERE user_id = $1;
    IF NOT FOUND THEN
        PERFORM set_config('lanview2.user_name',   'unknown', false);
        PERFORM set_config('lanview2.user_id',     '0',       false);
        PERFORM set_config('lanview2.user_rights', 'none',    false);
        PERFORM error('UserName', $1, '', 'set_user_id()', 'users');
    ELSE
        SELECT MAX(group_rights) INTO ri FROM group_users JOIN groups USING(group_id) WHERE user_id = ur.user_id;
        PERFORM set_config('lanview2.user_name',   CAST(ur.user_name AS text), false);
        PERFORM set_config('lanview2.user_id',     CAST(ur.user_id   AS text), false);
        PERFORM set_config('lanview2.user_rights', CAST(ri           AS text), false);
    END IF;
    RETURN ur;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION crypt_user_password() RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.passwd IS NULL THEN
            NEW.passwd := OLD.passwd;
            RETURN NEW;
        ELSIF NEW.passwd = OLD.passwd THEN
            RETURN NEW;
        END IF;
    ELSE -- 'INSERT'
        IF NEW.passwd IS NULL THEN
            RETURN NEW;
        END IF;
    END IF;
    NEW.passwd := crypt(NEW.passwd, gen_salt('md5'));
    RETURN NEW;
END
$$ LANGUAGE plpgsql;
ALTER FUNCTION crypt_user_password()  OWNER TO lanview2;
COMMENT ON FUNCTION crypt_user_password() IS 'Trigger függvény az users táblához. Titkosítja a passwd mwzőt, ha meg van adva, vagy változott.';

CREATE TRIGGER crypt_password BEFORE UPDATE OR INSERT ON users FOR EACH ROW EXECUTE PROCEDURE crypt_user_password();