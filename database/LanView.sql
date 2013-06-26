-- //////////////////// 0 - MAIN DATABASE /////////////////////
-- PostgreSQL Config:
-- APPEND: /etc/postgresql/8.4/main/postgresql.conf
/*
custom_variable_classes = 'lanview2'
# Session actual user name (lanview2 user name)
lanview2.user_name = nobody
# Session actual user id (0 = unknown)
lanview2.user_id = 0
# Actual user rights
lanview2.sesion_rights = none
# Last error code (errors.error_id)
lanview2.last_error_code = 0
# Last error log record id (db_errs.dblog_id)
lanview2.last_error_id = -1
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

-- //////////////////// ERROR LOGOK ////////////////////
\i errlogs.sql
-- //////////////////// Helyek/helyiségek ////////////////////
\i locations.sql
-- //////////////////// Felhasználók  ////////////////////
\i users.sql

-- ********************************************************
-- ****                     LAN                        ****
-- ********************************************************
\i nodes.sql
\i services.sql
\i links.sql
\i imports.sql
-- //////////////////// ALARM  ////////////////////
\i alarm.sql
\i indalarm.sql

CREATE TYPE unusualfkeytype AS ENUM ('property', 'self', 'owner');
COMMENT ON TYPE unusualfkeytype IS
'Az adatbázisban nem természetes módon definiált távoli kulcsok típusa:
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

INSERT INTO unusual_fkeys
  ( table_name,     column_name,    unusual_fkeys_type, f_table_name,   f_column_name,  f_inherited_tables) VALUES
  ( 'nports',       'node_id',      'owner',            'patchs',       'node_id',      '{hubs,nodes,hosts,snmpdevices}'),
  ( 'port_param_values','port_id',  'owner',            'nports',       'port_id',      '{nports,interfaces,iface_addrs,nodes,hosts,snmpdevices}'),
  ( 'interfaces',   'node_id',      'owner',            'nodes',        'node_id',      '{nodes,hosts,snmpdevices}'),
  ( 'interfaces',   'port_staple_id','self',            'interfaces',   'port_id',      '{interfaces,iface_addrs,hosts,snmpdevices}'),
  ( 'port_vlns',    'port_id',      'owner',            'interfaces',   'port_id',      '{interfaces,iface_addrs,hosts,snmpdevices}'),
  ( 'ipaddresses',  'port_id',      'owner',            'iface_addrs',  'port_id',      '{iface_addrs,hosts,snmpdevices}'),
  ( 'iface_addrs',  'node_id',      'owner',            'hosts',        'node_id',      '{hosts,snmpdevices}'),
  ( 'host_services','node_id',      'property',         'nodes',        'node_id',      '{nodes,hosts,snmpdevices}'),
  ( 'phs_links_table','port_id1',   'property',         'nports',       'port_id',      '{nports, pports,interfaces,iface_addrs,nodes,hosts,snmpdevices}'),
  ( 'phs_links_table','port_id2',   'property',         'nports',       'port_id',      '{nports, pports,interfaces,iface_addrs,nodes,hosts,snmpdevices}'),
  ( 'log_links_table','port_id1',   'property',         'nports',       'port_id',      '{nports, pports,interfaces,iface_addrs,nodes,hosts,snmpdevices}'),
  ( 'log_links_table','port_id2',   'property',         'nports',       'port_id',      '{nports, pports,interfaces,iface_addrs,nodes,hosts,snmpdevices}'),
  ( 'lldp_links_table','port_id1',  'property',         'nports',       'port_id',      '{nports, pports,interfaces,iface_addrs,nodes,hosts,snmpdevices}'),
  ( 'lldp_links_table','port_id2',  'property',         'nports',       'port_id',      '{nports, pports,interfaces,iface_addrs,nodes,hosts,snmpdevices}');

CREATE TABLE fkey_types (
    fkeys_types_id      serial          NOT NULL PRIMARY KEY,
    table_schema        varchar(32)     NOT NULL DEFAULT 'public',
    table_name          varchar(32)     NOT NULL,
    column_name         varchar(32)     NOT NULL,
    unusual_fkeys_type  unusualfkeytype NOT NULL DEFAULT 'owner',
    UNIQUE (table_schema, table_name, column_name)
);

INSERT INTO fkey_types
  ( table_name,             column_name,            unusual_fkeys_type) VALUES
  ( 'pports',               'node_id',              'owner'         ),
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

/*
-- **** TEST ****
INSERT INTO patchs (node_name) VALUES ('patch1'), ('patch2');
INSERT INTO nodes  (node_name, port_name) VALUES ('node1', 'N1:0'), ('node2', 'N2:0');
INSERT INTO pports (port_name, port_index, node_id) VALUES
    ('P1:1', 1, node_name2id('patch1')),
    ('P1:2', 2, node_name2id('patch1')),
    ('P1:3', 3, node_name2id('patch1')),
    ('P2:1', 1, node_name2id('patch2')),
    ('P2:2', 2, node_name2id('patch2')),
    ('P2:3', 3, node_name2id('patch2'));
INSERT INTO nports (port_name, node_id) VALUES
    ('N1:1', node_name2id('node1')),
    ('N1:2', node_name2id('node1')),
    ('N1:3', node_name2id('node1')),
    ('N1:4', node_name2id('node1')),
    ('N1:5', node_name2id('node1')),
    ('N2:1', node_name2id('node2')),
    ('N2:2', node_name2id('node2')),
    ('N2:3', node_name2id('node2')),
    ('N2:4', node_name2id('node2')),
    ('N2:5', node_name2id('node2'));

INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('N1:1', 'node1'),  NULL,   port_nn2id('N2:1', 'node2'),  NULL, '');    -- N1:1 <-> N2:1
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P1:1', 'patch1'), 'Back', port_nn2id('P2:1', 'patch2'),'Back', '');   -- P1:1 [-] P2:1
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P1:2', 'patch1'), 'Back', port_nn2id('P2:2', 'patch2'),'Back', '');   -- P1:2 [-] P2:2
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P1:1', 'patch1'), 'Front',port_nn2id('N1:2', 'node1'),  NULL, '');    -- P1:1 ]-> N1:2
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P1:2', 'patch1'), 'Front',port_nn2id('N1:3', 'node1'),  NULL, 'A');   -- P1:2A]-> N1:3
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P1:2', 'patch1'), 'Front',port_nn2id('N1:4', 'node1'),  NULL, 'B');   -- P1:2B]-> N1:4
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P2:1', 'patch2'), 'Front',port_nn2id('N2:2', 'node2'),  NULL, '');    -- P2:1 ]-> N2:2 !
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P2:2', 'patch2'), 'Front',port_nn2id('N2:4', 'node2'),  NULL, 'B');
INSERT INTO phs_links_table(port_id1, phs_link_type1, port_id2, phs_link_type2, port_shared) VALUES ( port_nn2id('P2:2', 'patch2'), 'Front',port_nn2id('N2:3', 'node2'),  NULL, 'A');

END; */
/*
-- Ennek nem kéne mennie (ha kész lessznek a triggerek)
INSERT INTO nports (port_name, iftype_id, node_id) VALUES
    ('P2:4', 1, node_name2id('patch2')),
    ('P2:5', 1, node_name2id('patch2')),
    ('P1:4', 1, node_name2id('patch1'));

INSERT INTO patchs (node_id, node_name) VALUES (5,'patch3'), (6,'patch4');
INSERT INTO nodes  (node_id, node_name, port_name) VALUES (7, 'node3', 'N3:0'), (8, 'node4', 'N4:0');

INSERT INTO patchs (node_name) VALUES ('patch5'), ('patch6');
INSERT INTO nodes  (node_name, port_name) VALUES ('node5', 'N5:0'), ('node6', 'N6:0');

INSERT INTO host_services
 (node_id, service_id, port_id) VALUES
 (node_name2id('node1'), 1, port_name2id('N1:1'));     -- jó
INSERT INTO host_services
 (node_id, service_id, port_id) VALUES
 (node_name2id('node2'), 1, port_name2id('P1:1'));     -- nem jó
*/
-- /////// TEST DATA ....
/*
INSERT INTO vlans ( vlan_id, vlan_name, vlan_descr, vlan_stat ) VALUES
    (    1, 'admdev',   'Network devices',              'on'),
    ( 3001, 'DEVICES',  'KKFK Server and Devices',      'on'),
    ( 3016, 'HALLGDEV', 'KKFKEDU Server and Devices',   'on');

INSERT INTO subnets ( subnet_id, subnet_name, subnet_descr, netaddr, vlan_id, is_primary) VALUES
    (    1, 'admdev',   'Network devices',              '172.20.128.0/24',     1, 'on'),
    (    2, 'DEVICES',  'KKFK Server and Devices',      '172.20.1.0/24',    3001, 'on'),
    (    3, 'HALLGDEV', 'KKFKEDU Server and Devices',   '172.20.16.0/24',   3016, 'on');

INSERT INTO snmpdevices
 ( node_id, node_name, node_stat, port_name, port_index, iftype_id, hwaddress, address, subnet_id, sysname) VALUES
 ( 1,       'moxa1',   'On',    'Moxa Ethernet',  1,     6,  '0090e80a10f8','172.20.128.46', 1,  'WLANCGW' );

INSERT INTO interfaces
 (port_name,   port_index, iftype_id, node_id) VALUES
 ('Moxa serial port 01',2, 1,      1 );
*/


-- SELECT * FROM db_errs JOIN errors USING (error_id);
