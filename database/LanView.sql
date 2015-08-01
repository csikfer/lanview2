-- //////////////////// 0 - MAIN DATABASE /////////////////////
-- PostgreSQL Config:
-- APPEND: /etc/postgresql/.../main/postgresql.conf
/*
# custom_variable_classes = 'lanview2'
# Session actual user id (0 = nobody)
lanview2.user_id = 0
# Actual user rights
lanview2.sesion_rights = none
# Last error code (errors.error_id)
lanview2.last_error_code = -1
*/
-- CREATE Languages
/*
DROP PROCEDURAL LANGUAGE IF EXISTS plperl  CASCADE;
DROP PROCEDURAL LANGUAGE IF EXISTS plpgsql CASCADE;
CREATE LANGUAGE plpgsql;
CREATE LANGUAGE plperl;
*/
-- //// CREATE LANVIEW2 USER
/*
CREATE ROLE lanview2
	LOGIN PASSWORD 'edcrfv+11'
	SUPERUSER CREATEDB CREATEROLE
	VALID UNTIL 'infinity';
*/
-- CONTRIB:
/*
psql -d lanview2 -f /usr/share/postgresql/9.1/extension/pgcrypto.sql
psql -d lanview2 -f /usr/share/postgresql/9.1/extension/dblink.sql
*/
-- //// CREATE DB LINUX
/*
DROP DATABASE IF EXISTS lanview2;

CREATE DATABASE lanview2
	WITH OWNER = lanview2
	ENCODING = 'UTF8'
	LC_COLLATE = 'hu_HU.utf8'
	LC_CTYPE = 'hu_HU.utf8'
	CONNECTION LIMIT = -1;
*/
-- //// CREATE DB WINDOWS

-- CREATE DATABASE lanview2
-- 	WITH OWNER = lanview2
-- 	ENCODING = 'UTF8'
-- 	LC_COLLATE = 'Hungarian, Hungary'
-- 	LC_CTYPE = 'Hungarian, Hungary'
-- 	CONNECTION LIMIT = -1;

-- *****************************************************************************************************
-- Megjegyzések:
-- *****************************************************************************************************
-- Minden név kisbetüs, tagolás ha kell az '_' karakterrel.
-- Tipus neveknél nincs tagolás
-- Tábla nevek többesszám, mező nevek egyes szám (mezőnév csak indokolt esetben többesszám, ha több dolgot jelöl, de ez kerülendő)
-- Ha egy mező másik tábla id-re utal, akkor neve a tábla neve egyes számban + "_id"
-- A mező tulajdonságai ha lehet a mező megadásánál egy sorban, nem utólag külön, mert igy szerintem átláthatóbb, rövidebb
-- Ha az egymást követő sorok táblázatként is felfoghatóak, akkor az oszlopok egymás alatt legyenek, ha ez lehetséges.
-- A tabulátor karakterek használatát kerüljük, mert nem minden editor ugyanúgy kezeli, szétesik a formázás
-- *****************************************************************************************************

SET client_encoding   = 'UTF8';
SET default_with_oids = 'FALSE';

CREATE OR REPLACE LANGUAGE plpgsql;
CREATE OR REPLACE LANGUAGE plperl;

CREATE EXTENSION IF NOT EXISTS dblink;
CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- //////////////////// 1 - PUBLIC SCHEMA ////////////////////

BEGIN;

CREATE OR REPLACE FUNCTION table_is_exists(text) RETURNS boolean AS $$
BEGIN
    RETURN 1 = COUNT(*) FROM information_schema.tables WHERE 'BASE TABLE' = table_type AND $1 = table_name;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION table_is_exists(text) IS 'Ha a paraméterként megadott bevű tábla létezik, akkor igaz értékkel, egyébként hamis értékkel tér vissza';

CREATE OR REPLACE FUNCTION view_is_exists(text) RETURNS boolean AS $$
BEGIN
    RETURN 1 = COUNT(*) FROM information_schema.tables WHERE 'VIEW' = table_type AND $1 = table_name;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION view_is_exists(text) IS 'Ha a paraméterként megadott bevű nézet tábla létezik, akkor igaz értékkel, egyébként hamis értékkel tér vissza';

CREATE OR REPLACE FUNCTION table_or_view_is_exists(text) RETURNS boolean AS $$
BEGIN
    RETURN 1 = COUNT(*) FROM information_schema.tables WHERE $1 = table_name;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION table_or_view_is_exists(text) IS 'Ha a paraméterként megadott bevű tábla vagy nézet tábla létezik, akkor igaz értékkel, egyébként hamis értékkel tér vissza';

CREATE TYPE paramtype AS ENUM ( 'boolean', 'bigint', 'double precision', 'text', 'interval', 'date', 'time', 'timestamp', 'inet', 'bytea' );

CREATE TABLE param_types (
    param_type_id       bigserial       NOT NULL PRIMARY KEY,
    param_type_name     varchar(32)     NOT NULL UNIQUE,
    param_type_note     text    DEFAULT NULL,
    param_type_type     paramtype       NOT NULL,
    param_type_dim      varchar(32)     DEFAULT NULL
);
ALTER TABLE param_types OWNER TO lanview2;
COMMENT ON TABLE param_types IS 'Paraméterek deklarálása (név, típus, dimenzió)';
COMMENT ON COLUMN param_types.param_type_id   IS 'A paraméter típus leíró egyedi azonosítója.';
COMMENT ON COLUMN param_types.param_type_name IS 'A paraméter típus neve.';
COMMENT ON COLUMN param_types.param_type_note IS 'A paraméterhez egy magyarázó szöveg';
COMMENT ON COLUMN param_types.param_type_type IS 'Típus azonosító';
COMMENT ON COLUMN param_types.param_type_dim  IS 'Egy opcionális dimenzió';

INSERT INTO param_types
    (param_type_name,   param_type_type)    VALUES
    ('boolean',         'boolean'),
    ('bigint',          'bigint'),
    ('double precision','double precision'),
    ('text',            'text'),
    ('interval',        'interval'),
    ('date',            'date'),
    ('time',            'time'),
    ('timestamp',       'timestamp'),
    ('inet',            'inet'),
    ('bytea',           'bytea');

CREATE OR REPLACE FUNCTION param_type_name2id(varchar(32)) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT param_type_id INTO id FROM param_types WHERE param_type_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'param_type_name2id()', 'param_types');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE TYPE reasons AS ENUM ('new', 'insert', 'remove', 'expired', 'move', 'restore', 'modify', 'update', 'unchange', 'found', 'notfound', 'discard', 'caveat', 'ambiguous', 'error');
ALTER TYPE reasons OWNER TO lanview2;
COMMENT ON TYPE reasons IS
'Okok ill. műveletek eredményei
';

CREATE TABLE sys_params (
    sys_param_id        bigserial       PRIMARY KEY,
    sys_param_name      varchar(32)     NOT NULL UNIQUE,
    sys_param_note      text    DEFAULT NULL,
    param_type_id       bigint          NOT NULL
            REFERENCES param_types(param_type_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    param_value         text            DEFAULT NULL
);

CREATE OR REPLACE FUNCTION set_text_sys_param(pname varchar(32), txtval text, tname varchar(32) DEFAULT 'text') RETURNS reasons AS $$
DECLARE
    type_id bigint;
    rec sys_params;
BEGIN
    SELECT param_type_id INTO type_id FROM param_types WHERE param_type_name = tname;
    IF NOT FOUND THEN
        RETURN 'notfound';
    END IF;
    SELECT * INTO rec FROM sys_params WHERE sys_param_name = pname;
    IF NOT FOUND THEN
        INSERT INTO sys_params(sys_param_name, param_type_id, param_value) VALUES (pname, type_id, txtval);
        RETURN 'insert';
    END IF;
    IF type_id = rec.param_type_id THEN
        IF txtval = rec.param_value THEN
            RETURN 'found';
        END IF;
        UPDATE sys_params SET param_value = txtval WHERE sys_param_name = pname;
        RETURN 'update';
    END IF;
    UPDATE sys_params SET param_value = txtval, param_type_id = type_id WHERE sys_param_name = pname;
    RETURN 'modify';
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION set_text_sys_param(pname varchar(32), txtval text, tname varchar(32)) IS '
Egy rendszer paraméter (sys_params tábla egy rekordja) értékének, és adat típusának a megadása.
A függvény paraméterei
  pname   A paraméter neve
  txtval  A paraméter értéke
  tname   A paraméter típusleíró rekord neve. Opcionálís, ha nem adjuk meg, akkor "text".
Visszatérési érték:
  Ha a paraméter már létezik, és a megadott értékű és típusú, akkor "found".
  Ha még nincs ilyen paraméter, akkor "insert"
  Ha magadtuk a típus nevet, és nincs ilyen típus rekord, akkor "notfound".
  Ha volt ilyen nevű paraméter, és csak az érték változott, akkor "update"
  Ha volt ilyen paraméter, és a típus változott, akkor "modify".
';

CREATE OR REPLACE FUNCTION set_bool_sys_param(pname varchar(32), boolval boolean DEFAULT true, tname varchar(32) DEFAULT 'boolean') RETURNS reasons AS $$
BEGIN
    RETURN set_text_sys_param(pname, boolval::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_int_sys_param(pname varchar(32), intval bigint, tname varchar(32) DEFAULT 'bigint') RETURNS reasons AS $$
BEGIN
    RETURN set_text_sys_param(pname, intval::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_interval_sys_param(pname varchar(32), ival interval, tname varchar(32) DEFAULT 'interval') RETURNS reasons AS $$
BEGIN
    RETURN set_text_sys_param(pname, ival::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_text_sys_param(pname varchar(32)) RETURNS text AS $$
DECLARE
    res text;
BEGIN
    SELECT param_value INTO res FROM sys_params WHERE sys_param_name = pname;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    RETURN res;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_bool_sys_param(pname varchar(32)) RETURNS boolean AS $$
BEGIN
    IF get_text_sys_param(pname)::boolean THEN
        RETURN true;
    ELSE
        RETURN false;
    END IF;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_int_sys_param(pname varchar(32)) RETURNS bigint AS $$
BEGIN
    RETURN get_text_sys_param(pname)::bigint;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_interval_sys_param(pname varchar(32)) RETURNS interval AS $$
BEGIN
    RETURN get_text_sys_param(pname)::interval;
END
$$ LANGUAGE plpgsql;

-- //////////////////// ERROR LOGOK ////////////////////
\i errlogs.sql
-- //////////////////// Helyek/helyiségek ////////////////////
\i locations.sql
-- //////////////////// Felhasználók  ////////////////////
\i users.sql

\i nodes.sql
\i services.sql
\i mactab.sql
\i links.sql
\i imports.sql
-- //////////////////// ALARM  ////////////////////
\i alarm.sql
-- \i indalarm.sql

CREATE TYPE unusualfkeytype AS ENUM ('property', 'self', 'owner');
COMMENT ON TYPE unusualfkeytype IS
'Az adatbázisban nem természetes módon definiált távoli kulcsok/hivatkozások típusa:
property    Normál, egy tulajdonségra mutató kulcs
self        Nem igazi távoli kulcs, saját (esetleg a leszármazott) táblára mutat
owner       Szülő objektumra mutató kulcs
';

CREATE TABLE unusual_fkeys (
    unusual_fkeys_id    bigserial       NOT NULL PRIMARY KEY,
    table_schema        varchar(32)     NOT NULL DEFAULT 'public',
    table_name          varchar(32)     NOT NULL,
    column_name         varchar(32)     NOT NULL,
    unusual_fkeys_type  unusualfkeytype NOT NULL DEFAULT 'property',
    f_table_schema      varchar(32)     NOT NULL DEFAULT 'public',
    f_table_name        varchar(32)     NOT NULL,
    f_column_name       varchar(32)     NOT NULL,
    f_inherited_tables  varchar(32)[]   DEFAULT NULL,
    UNIQUE (table_schema, table_name, column_name)
);

COMMENT ON TABLE  unusual_fkeys IS 'Az öröklödéshez kapcsolódó távoli kulcsok definíciói';
COMMENT ON COLUMN unusual_fkeys.table_schema IS 'A tábla séma neve, melyben a hivatkozó mezőt definiáljuk';
COMMENT ON COLUMN unusual_fkeys.table_name  IS 'A tábla neve, melyben a hivatkozó mezőt definiáljuk';
COMMENT ON COLUMN unusual_fkeys.column_name IS 'A hivatkozó/mutató mező neve';
COMMENT ON COLUMN unusual_fkeys.unusual_fkeys_type IS 'A hivatkozás típusa';
COMMENT ON COLUMN unusual_fkeys.f_table_schema IS 'A hivatkozott tábla séma neve';
COMMENT ON COLUMN unusual_fkeys.f_table_name IS 'A hivatkozott tábla neve';
COMMENT ON COLUMN unusual_fkeys.f_table_schema IS 'A hivatkozott tábla elsődleges kulcsmezóje, ill. a hivatkozott mező';
COMMENT ON COLUMN unusual_fkeys.f_inherited_tables IS 'Azon leszármazott táblák nevei, melyekre még vonatkozhat a hivatkozás (a séma név azonos)';

INSERT INTO unusual_fkeys
  ( table_name,         column_name,    unusual_fkeys_type, f_table_name,   f_column_name,  f_inherited_tables) VALUES
  ( 'nports',           'node_id',      'owner',            'nodes',        'node_id',      '{nodes, snmpdevices}'),
  ( 'port_param_values','port_id',      'owner',            'nports',       'port_id',      '{nports, pports, interfaces}'),
  ( 'node_param_values','node_id',      'owner',            'patchs',       'node_id',      '{patchs, nodes, snmpdevices}'),
  ( 'interfaces',       'node_id',      'owner',            'nodes',        'node_id',      '{nodes, snmpdevices}'),
  ( 'host_services',    'node_id',      'property',         'nodes',        'node_id',      '{nodes, snmpdevices}'),
  ( 'host_services',    'port_id',      'property',         'nports',       'port_id',      '{nports, interfaces}'),
  ( 'phs_links_table',  'port_id1',     'property',         'nports',       'port_id',      '{nports, pports, interfaces}'),
  ( 'phs_links_table',  'port_id2',     'property',         'nports',       'port_id',      '{nports, pports, interfaces}'),
  ( 'log_links_table',  'port_id1',     'property',         'nports',       'port_id',      '{nports, interfaces}'),
  ( 'log_links_table',  'port_id2',     'property',         'nports',       'port_id',      '{nports, interfaces}');

CREATE TABLE fkey_types (
    fkeys_types_id      bigserial       NOT NULL PRIMARY KEY,
    table_schema        varchar(32)     NOT NULL DEFAULT 'public',
    table_name          varchar(32)     NOT NULL,
    column_name         varchar(32)     NOT NULL,
    unusual_fkeys_type  unusualfkeytype NOT NULL DEFAULT 'owner',
    UNIQUE (table_schema, table_name, column_name)
);

COMMENT ON TABLE fkey_types IS 'A távoli kulcsok típusát definiáló tábla (a nem property tíousoknál)';
COMMENT ON COLUMN fkey_types.table_schema IS 'A tábla séma neve, melyben a hivatkozó mezőt definiáljuk';
COMMENT ON COLUMN fkey_types.table_name  IS 'A tábla neve, melyben a hivatkozó mezőt definiáljuk';
COMMENT ON COLUMN fkey_types.column_name IS 'A hivatkozó/mutató mező neve';
COMMENT ON COLUMN fkey_types.unusual_fkeys_type IS 'A hivatkozás/távoli kulcs típusa';

INSERT INTO fkey_types
  ( table_name,             column_name,            unusual_fkeys_type) VALUES
  ( 'pports',               'node_id',              'owner'         ),
  ( 'ipaddresses',          'port_id',              'owner'         ),
  ( 'table_shape_fields',   'table_shape_id',       'owner'         ),
  ( 'table_shape_filters',  'table_shape_field_id', 'owner'         );

\i gui.sql

-- //////////////////// Initial Data ////////////////////

END;

BEGIN;
SELECT error('Ok', 0, 'First log message / Init lanview2 database ...');
END;

