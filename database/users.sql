-- //// TPOWS

CREATE TYPE dayofweek AS ENUM ('sunday','monday','tuesday','wednesday','thursday','friday','saturday');
ALTER TYPE dayofweek OWNER TO lanview2;
COMMENT ON TYPE dayofweek IS 'Date of week enumeration.';

-- Idő intervallum definíciók egy megadott napon egy héten bellül
CREATE TABLE tpows (
    tpow_id     serial          PRIMARY KEY,
    tpow_name   varchar(32)     NOT NULL UNIQUE,
    tpow_descr  varchar(255)    DEFAULT NULL,
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
    timeperiod_id       serial          PRIMARY KEY,
    timeperiod_name     varchar(32)     NOT NULL UNIQUE,
    timeperiod_descr    varchar(255)    DEFAULT NULL
);
ALTER TABLE timeperiods OWNER TO lanview2;
COMMENT ON TABLE  timeperiods                  IS 'Time periods';
COMMENT ON COLUMN timeperiods.timeperiod_id    IS 'Unique ID';
COMMENT ON COLUMN timeperiods.timeperiod_name  IS 'Unique name for time periods';

-- //// TIMEPERIODS_TPOWS

CREATE TABLE timeperiod_tpows (
    timeperiod_tpow_id serial  PRIMARY KEY,
    tpow_id         integer     REFERENCES tpows(tpow_id)  MATCH FULL ON DELETE RESTRICT ON UPDATE CASCADE,
    timeperiod_id   integer     REFERENCES timeperiods(timeperiod_id)  MATCH FULL ON DELETE RESTRICT ON UPDATE CASCADE,
    UNIQUE(tpow_id,timeperiod_id)
);
ALTER TABLE timeperiod_tpows OWNER TO lanview2;
COMMENT ON TABLE  timeperiod_tpows                 IS 'Time periods and tpows kapcsoló tábla';
COMMENT ON COLUMN timeperiod_tpows.tpow_id         IS 'Unique ID';
COMMENT ON COLUMN timeperiod_tpows.timeperiod_id   IS 'Unique ID';

-- Hét napjainak a nevét numerikus kóddá alakítja
CREATE OR REPLACE FUNCTION dow2int(dayofweek) RETURNS integer AS $$
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
CREATE OR REPLACE FUNCTION int2dow(integer) RETURNS dayofweek AS $$
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
CREATE OR REPLACE FUNCTION time_in_timeperiods(integer, timestamp DEFAULT CURRENT_TIMESTAMP) RETURNS boolean AS $$
BEGIN
    RETURN 0 < COUNT(*) FROM tpows JOIN timeperiods_tpows USING (tpow_id)
        WHERE $1 = timeperiod_id AND int2dow(EXTRACT(DOW FROM $2)) = dow AND $2::time >= begin_time AND $2::time < end_time;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION tpow_name2id(varchar(32)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT tpow_id INTO id FROM tpows WHERE tpow_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'tpow_name2id(varchar(32))', 'tpows');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION timeperiod_name2id(varchar(32)) RETURNS integer AS $$
BEGIN
    RETURN timeperiod_id FROM timeperiods WHERE timeperiod_name = $1;
END
$$ LANGUAGE plpgsql;

INSERT INTO tpows(tpow_name, tpow_descr, dow) VALUES
    ('all_sun', 'On Sundays all day',    'sunday'   ),
    ('all_mon', 'On Mondays all day',    'monday'   ),
    ('all_tue', 'On Tuesdays all day',   'tuesday'  ),
    ('all_wed', 'On Wednesdays all day', 'wednesday'),
    ('all_thu', 'On Thursdays all day',  'thursday' ),
    ('all_fri', 'On Fridays all day',    'friday'   ),
    ('all_sat', 'On Saturdays all day',  'saturday' );

INSERT INTO timeperiods(timeperiod_id, timeperiod_name, timeperiod_descr) VALUES
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
    user_id             serial          PRIMARY KEY,
    user_name           varchar(32)     NOT NULL UNIQUE,
    user_descr          varchar(255)    DEFAULT NULL,
    passwd              varchar(64),
    enabled             boolean         DEFAULT TRUE,
    host_notif_period   integer         DEFAULT 0 REFERENCES timeperiods(timeperiod_id) MATCH FULL
                                            ON DELETE RESTRICT ON UPDATE RESTRICT,
    serv_notif_period   integer         DEFAULT 0 REFERENCES timeperiods(timeperiod_id) MATCH FULL
                                            ON DELETE RESTRICT ON UPDATE RESTRICT,
    -- ez valójában egy set akar lenni, de csak enum van, ezért tömb.
    host_notif_switchs  notifswitch[] NOT NULL DEFAULT '{"unreachable","down","recovered","unknown","critical"}',
    -- ez szintén egy set, mint feljebb
    serv_notif_switchs  notifswitch[] NOT NULL DEFAULT '{"unreachable","down","recovered","unknown","critical"}',
    host_notif_cmd      varchar(255)    DEFAULT NULL,
    serv_notif_cmd      varchar(255)    DEFAULT NULL,
    tel                 varchar(20)     DEFAULT NULL,
    addresses           varchar(128)[]  DEFAULT NULL,
    place_id            integer         DEFAULT NULL REFERENCES places(place_id) MATCH SIMPLE
                                            ON DELETE RESTRICT ON UPDATE RESTRICT
);
ALTER TABLE users OWNER TO lanview2;
COMMENT ON TABLE  users                 IS 'Users and contact table';


CREATE TYPE rights AS ENUM ( 'none', 'viewer', 'indalarm', 'operator', 'admin', 'system' );
ALTER TYPE  rights OWNER TO lanview2;

CREATE TABLE groups (
    group_id        serial          PRIMARY KEY,
    group_name      varchar(32)     NOT NULL UNIQUE,
    group_descr     varchar(255)    DEFAULT NULL,
    group_rights    rights          DEFAULT 'viewer',
    place_id        integer         DEFAULT NULL REFERENCES places(place_id) MATCH SIMPLE
                                            ON DELETE RESTRICT ON UPDATE RESTRICT
);
ALTER TABLE groups OWNER TO lanview2;
COMMENT ON TABLE  groups            IS 'User groups';
COMMENT ON COLUMN groups.group_id   IS 'Group ID';
COMMENT ON COLUMN groups.group_name IS 'Group name';
COMMENT ON COLUMN groups.group_descr IS '';
COMMENT ON COLUMN groups.group_rights IS 'User rights';
COMMENT ON COLUMN groups.place_id   IS 'Place type group, (parent)place id';

CREATE OR REPLACE FUNCTION group_name2id(varchar(32)) RETURNS integer AS $$
BEGIN
    RETURN group_id FROM groups WHERE group_name = $1;
END
$$ LANGUAGE plpgsql;

CREATE TABLE group_users (
    group_user_id   serial  PRIMARY KEY,
    group_id        integer REFERENCES groups(group_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    user_id         integer REFERENCES users(user_id) MATCH FULL ON DELETE CASCADE ON UPDATE CASCADE,
    UNIQUE(group_id, user_id)
);
ALTER TABLE group_users OWNER TO lanview2;
COMMENT ON TABLE  group_users           IS 'Group members table';
COMMENT ON COLUMN group_users.group_id  IS 'Group ID';
COMMENT ON COLUMN group_users.user_id   IS 'User ID';

INSERT INTO groups(group_id, group_name, group_descr, group_rights) VALUES
    ( 0, 'system',  'system',           'system'  ),
    ( 1, 'admin',   'Administrators',   'admin'   ),
    ( 2, 'operator','Operators',        'operator'),
    ( 3, 'indalarm','IndAlarm users',   'indalarm'),
    ( 4, 'viewer',  'Viewers',          'viewer'  );

SELECT setval('groups_group_id_seq', 5);

INSERT INTO users(user_id, user_name, user_descr, passwd) VALUES
    ( 0, 'nobody',  'Unknown user', NULL),
    ( 1, 'system',  'system',       NULL),
    ( 2, 'admin',   'Administrator',crypt('admin',gen_salt('md5'))),
    ( 3, 'operator','Operator',     crypt('operator',gen_salt('md5'))),
    ( 4, 'viewer',  'Viewer',       NULL);

SELECT setval('users_user_id_seq', 5);

CREATE OR REPLACE FUNCTION user_name2id(varchar(32)) RETURNS integer AS $$
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
CREATE OR REPLACE FUNCTION set_user_id(integer DEFAULT NULL) RETURNS users AS $$
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

CREATE OR REPLACE FUNCTION is_member_userr(uid integer, gids integer[], pid integer DEFAULT NULL) RETURNS boolean AS $$
DECLARE
    n   integer;
    apid integer;       -- place_id
    pgid integer;       -- place_id -hez kapcsolódó group_id, vagy NULL
BEGIN
    SELECT COUNT(*) INTO n FROM group_users WHERE user_id = uid AND group_id = ANY (gids);
    IF n > 0 THEN
        RETURN TRUE;
    END IF;
    apid := pid;
    LOOP
        IF apid IS NULL THEN
            RETURN FALSE;
        END IF;
        SELECT COUNT(*) INTO n FROM group_users INNER JOIN groups USING(group_id) WHERE apid = place_id;
        IF n > n THEN
            RETURN TRUE;
        END IF;
        BEGIN
            SELECT place_id INTO STRICT apid FROM places WHERE parent_id = apid;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                   RETURN FALSE;
        END;
    END LOOP;
END;
$$ LANGUAGE plpgsql;
