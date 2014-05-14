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

CREATE OR REPLACE FUNCTION view_is_exists(text) RETURNS boolean AS $$
BEGIN
    RETURN 1 = COUNT(*) FROM information_schema.tables WHERE 'VIEW' = table_type AND $1 = table_name;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION table_or_view_is_exists(text) RETURNS boolean AS $$
BEGIN
    RETURN 1 = COUNT(*) FROM information_schema.tables WHERE $1 = table_name;
END
$$ LANGUAGE plpgsql;

CREATE TYPE paramtype AS ENUM ( 'any', 'boolean', 'int', 'real', 'char', 'string', 'interval', 'ipaddress');

CREATE TABLE param_types (
    param_type_id       serial          NOT NULL PRIMARY KEY,
    param_type_name     varchar(32)     NOT NULL UNIQUE,
    param_type_note     varchar(255)    DEFAULT NULL,
    param_type_type     paramtype       NOT NULL DEFAULT 'any',
    param_type_dim      varchar(32)     DEFAULT NULL
);
ALTER TABLE param_types OWNER TO lanview2;
COMMENT ON TABLE param_types IS 'Paraméterek deklarálása (név, típus, dimenzió)';
COMMENT ON COLUMN param_types.param_type_id   IS 'A paraméter leíró egyedi azonosítója.';
COMMENT ON COLUMN param_types.param_type_name IS 'A paraméter neve.';
COMMENT ON COLUMN param_types.param_type_note IS 'A paraméterhez egy magyarázó szöveg';
COMMENT ON COLUMN param_types.param_type_type IS 'Egy opcionális típus azonosító';
COMMENT ON COLUMN param_types.param_type_dim  IS 'Egy opcionális dimenzió';

INSERT INTO param_types(param_type_name, param_type_type) VALUES ('URL', 'URL');

CREATE OR REPLACE FUNCTION param_type_name2id(varchar(32)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT param_type_id INTO id FROM param_types WHERE param_type_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'param_type_name2id()', 'param_types');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE TABLE sys_params (
    sys_param_id        serial          PRIMARY KEY,
    sys_param_name      varchar(32)     NOT NULL UNIQUE,
    param_type_id       integer         NOT NULL
            REFERENCES param_types(param_type_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    sys_param_value     text            DEFAULT NULL
);

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
\i indalarm.sql

CREATE TYPE unusualfkeytype AS ENUM ('property', 'self', 'owner');
COMMENT ON TYPE unusualfkeytype IS
'Az adatbázisban nem természetes módon definiált távoli kulcsok/hivatkozások típusa:
property    Normál, egy tulajdonségra mutató kulcs
self        Nem igazi távoli kulcs, saját (esetleg a leszármazott) táblára mutat
owner       Szülő objektumra mutató kulcs
';

CREATE TABLE unusual_fkeys (
    unusual_fkeys_id    serial          NOT NULL PRIMARY KEY,
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

COMMENT ON TABLE unusual_fkeys IS 'Az öröklödéshez kapcsolódó távoli kulcsok definíciói';
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
  ( 'nports',           'node_id',      'owner',            'nodes',        'node_id',      '{nodes,snmpdevices}'),
  ( 'port_param_values','port_id',      'owner',            'nports',       'port_id',      '{nports,pports,interfaces}'),
  ( 'interfaces',       'node_id',      'owner',            'nodes',        'node_id',      '{nodes,snmpdevices}'),
  ( 'host_services',    'node_id',      'property',         'nodes',        'node_id',      '{nodes,snmpdevices}'),
  ( 'phs_links_table',  'port_id1',     'property',         'nports',       'port_id',      '{nports, pports,interfaces}'),
  ( 'phs_links_table',  'port_id2',     'property',         'nports',       'port_id',      '{nports, pports,interfaces}'),
  ( 'log_links_table',  'port_id1',     'property',         'nports',       'port_id',      '{nports, interfaces}'),
  ( 'log_links_table',  'port_id2',     'property',         'nports',       'port_id',      '{nports, interfaces}');

CREATE TABLE fkey_types (
    fkeys_types_id      serial          NOT NULL PRIMARY KEY,
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

/*

CREATE TYPE logreasons AS ENUM ('deleted', 'modifyed', 'timeout');


CREATE TYPE eventtype AS ENUM ('deleted', 'updated', 'moved', 'stale');
ALTER TYPE eventtype OWNER TO lanview2;

CREATE TABLE log_vlans (
    time_of     timestamp       DEFAULT CURRENT_TIMESTAMP,
    event_type  eventtype   NOT NULL,
    vlan_id     integer         NOT NULL,
    vlan_name   varchar(32)     NOT NULL,
    vlan_descr  varchar(255)    DEFAULT NULL,
    vlan_stat   boolean         NOT NULL
);
CREATE INDEX vlans_time_of_index ON log_vlans (time_of);
ALTER TABLE log_vlans OWNER TO lanview2;

CREATE TABLE log_port_vlans (
    time_of     timestamp       DEFAULT CURRENT_TIMESTAMP,
    event_type  eventtype   NOT NULL,
    port_id     integer         NOT NULL,
    vlan_id     integer         NOT NULL,
    first_time  timestamp       NOT NULL,
    last_time   timestamp       NOT NULL,
    vlan_type   vlantype   NOT NULL DEFAULT 'Untagged'
);
CREATE INDEX port_vlans_time_of_index ON log_port_vlans (time_of);
ALTER TABLE log_port_vlans OWNER TO lanview2;
*/

-- //////////////////// Initial Data ////////////////////

END;

BEGIN;
SELECT error('Ok', 0, 'First log message / Init lanview2 database ...');
END;

