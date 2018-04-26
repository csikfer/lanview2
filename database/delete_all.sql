-- Delete all user data

DELETE FROM alarms;
DELETE FROM app_errs;
DELETE FROM app_memos;
DELETE FROM arp_logs;
DELETE FROM dyn_ipaddress_logs;
DELETE FROM host_service_logs;
DELETE FROM host_service_noalarms;
DELETE FROM imports;
DELETE FROM mactab_logs;
DELETE FROM patchs;
DELETE FROM phs_links_table;
DELETE FROM subnets;
DELETE FROM vlans;


DELETE FROM arps;
DELETE FROM dyn_addr_ranges;
DELETE FROM host_services;
DELETE FROM images;
DELETE FROM import_templates;
DELETE FROM mactab;
DELETE FROM lldp_links_table;
DELETE FROM user_events;
DELETE FROM db_errs;

DELETE FROM group_users;
DELETE FROM groups;
DELETE FROM users;

DELETE FROM places WHERE place_type = 'real';
DELETE FROM place_groups WHERE place_group_name NOT IN ('all', 'none', 'community', 'guestroom', 'none', 'office', 'schoolroom', 'student', 'technical_room');

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
