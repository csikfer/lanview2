
ALTER TABLE public.interfaces
    ADD COLUMN IF NOT EXISTS delegate_oper_stat  boolean DEFAULT FALSE,
    ADD COLUMN IF NOT EXISTS delegate_admin_stat boolean DEFAULT TRUE;
COMMENT ON COLUMN public.interfaces.delegate_oper_stat  IS 'Delegate port op. state to portvars service';
COMMENT ON COLUMN public.interfaces.delegate_admin_stat IS 'Delegate port admin state to portvars service';

TRUNCATE public.unusual_fkeys;
INSERT INTO public.unusual_fkeys VALUES (1, 'public', 'nports', 'node_id', 'owner', 'public', 'nodes', 'node_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (2, 'public', 'port_params', 'port_id', 'owner', 'public', 'nports', 'port_id', '{nports,pports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (3, 'public', 'node_params', 'node_id', 'owner', 'public', 'patchs', 'node_id', '{patchs,nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (4, 'public', 'interfaces', 'node_id', 'owner', 'public', 'nodes', 'node_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (5, 'public', 'host_services', 'node_id', 'property', 'public', 'nodes', 'node_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (6, 'public', 'host_services', 'port_id', 'property', 'public', 'nports', 'port_id', '{nports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (7, 'public', 'phs_links_table', 'port_id1', 'owner', 'public', 'nports', 'port_id', '{nports,pports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (8, 'public', 'phs_links_table', 'port_id2', 'property', 'public', 'nports', 'port_id', '{nports,pports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (9, 'public', 'log_links_table', 'port_id1', 'owner', 'public', 'nports', 'port_id', '{nports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (10, 'public', 'log_links_table', 'port_id2', 'property', 'public', 'nports', 'port_id', '{nports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (11, 'public', 'table_shapes', 'right_shape_ids', 'self', 'public', 'table_shapes', 'table_shape_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (12, 'public', 'phs_links_shape', 'node_id1', 'property', 'public', 'patchs', 'port_id', '{patchs,nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (13, 'public', 'log_links_shape', 'node_id1', 'property', 'public', 'nports', 'port_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (14, 'public', 'lldp_links_shape', 'node_id1', 'property', 'public', 'nports', 'port_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (15, 'public', 'phs_named_link', 'port_id1', 'owner', 'public', 'nports', 'port_id', '{nports,pports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (16, 'public', 'app_errs', 'node_id', 'property', 'public', 'nodes', 'node_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (17, 'public', 'app_memos', 'node_id', 'property', 'public', 'nodes', 'node_id', '{nodes,snmpdevices}');
INSERT INTO public.unusual_fkeys VALUES (18, 'public', 'services', 'online_group_ids', 'property', 'public', 'groups', 'group_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (19, 'public', 'services', 'offline_group_ids', 'property', 'public', 'groups', 'group_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (20, 'public', 'host_services', 'online_group_ids', 'property', 'public', 'groups', 'group_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (21, 'public', 'host_services', 'offline_group_ids', 'property', 'public', 'groups', 'group_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (55, 'public', 'arps_shape', 'host_service_id', 'property', 'public', 'host_services', 'host_service_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (47, 'public', 'online_alarm_acks', 'online_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (48, 'public', 'online_alarm_acks', 'notice_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (49, 'public', 'online_alarm_acks', 'view_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (50, 'public', 'online_alarm_acks', 'ack_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (51, 'public', 'online_alarm_acks', 'superior_alarm_id', 'property', 'public', 'host_services', 'host_service_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (43, 'public', 'online_alarm_unacks', 'online_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (44, 'public', 'online_alarm_unacks', 'notice_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (45, 'public', 'online_alarm_unacks', 'view_user_ids', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (46, 'public', 'online_alarm_unacks', 'superior_alarm_id', 'property', 'public', 'host_services', 'host_service_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (52, 'public', 'portvars', 'port_id', 'owner', 'public', 'nports', 'port_id', '{nports,interfaces}');
INSERT INTO public.unusual_fkeys VALUES (53, 'public', 'portvars', 'service_var_type_id', 'property', 'public', 'service_var_types', 'service_var_type_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (54, 'public', 'portvars', 'host_service_id', 'property', 'public', 'host_services', 'host_service_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (60, 'public', 'port_vlans', 'port_id', 'owner', 'public', 'nports', 'port_id', '{interfaces}');
INSERT INTO public.unusual_fkeys VALUES (61, 'public', 'phs_links_shape', 'create_user_id', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (62, 'public', 'phs_links_shape', 'modify_user_id', 'property', 'public', 'users', 'user_id', NULL);
INSERT INTO public.unusual_fkeys VALUES (63, 'public', 'alarm_service_vars', 'service_var_id', 'property', 'public', 'service_vars', 'service_var_id', '{service_vars,service_rrd_vars}');
SELECT pg_catalog.setval('public.unusual_fkeys_unusual_fkey_id_seq', 63, true);



SELECT set_db_version(1, 26);
