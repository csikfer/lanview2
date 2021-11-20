-- Delete all user data

-- \i clrstat.sql

TRUNCATE imports RESTART IDENTITY;
-- TRUNCATE import_templates RESTART IDENTITY;
TRUNCATE mactab_logs RESTART IDENTITY;
TRUNCATE arps RESTART IDENTITY;
TRUNCATE dyn_addr_ranges RESTART IDENTITY;
TRUNCATE dyn_ipaddress_logs RESTART IDENTITY;
TRUNCATE group_users RESTART IDENTITY;
TRUNCATE mactab RESTART IDENTITY;
TRUNCATE phs_links_table RESTART IDENTITY;
TRUNCATE log_links_table RESTART IDENTITY;
TRUNCATE lldp_links_table RESTART IDENTITY;
TRUNCATE alarm_service_vars RESTART IDENTITY;
TRUNCATE app_memos  RESTART IDENTITY;
TRUNCATE db_errs  RESTART IDENTITY;
TRUNCATE host_service_noalarms  RESTART IDENTITY;
TRUNCATE ip_address_logs RESTART IDENTITY;

DELETE FROM app_errs;

DELETE FROM service_vars;
DELETE FROM host_services;
ALTER SEQUENCE host_services_host_service_id_seq RESTART WITH 1;
TRUNCATE tool_objects;

DELETE FROM nports;
ALTER SEQUENCE nports_port_id_seq RESTART WITH 1;
DELETE FROM patchs;
ALTER SEQUENCE patchs_node_id_seq RESTART WITH 1;
DELETE FROM subnets;
ALTER SEQUENCE subnets_subnet_id_seq RESTART WITH 1;
DELETE FROM vlans;
ALTER SEQUENCE images_image_id_seq RESTART WITH 1;
DELETE FROM groups;
ALTER SEQUENCE graphs_graph_id_seq RESTART WITH 1;
DELETE FROM users;
ALTER SEQUENCE users_user_id_seq RESTART WITH 1;
DELETE FROM images WHERE 'map' = ANY (usabilityes);

TRUNCATE place_params;
TRUNCATE place_group_params;
DELETE FROM places WHERE place_type = 'real';
DELETE FROM place_groups WHERE place_group_name NOT IN ('all', 'none', 'community', 'guestroom', 'office', 'schoolroom', 'student', 'technical_room');

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

INSERT INTO group_users ( group_id, user_id ) VALUES
    ( group_name2id('system'),   user_name2id('system') ),
    ( group_name2id('admin'),    user_name2id('admin') ),
    ( group_name2id('operator'), user_name2id('operator') ),
    ( group_name2id('operator'), user_name2id('admin') ),
    ( group_name2id('viewer'),   user_name2id('viewer') );


