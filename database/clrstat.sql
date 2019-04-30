TRUNCATE user_events RESTART IDENTITY;
DELETE FROM alarms;
ALTER SEQUENCE alarms_alarm_id_seq RESTART WITH 1;
DELETE FROM app_errs;
ALTER SEQUENCE app_errs_applog_id_seq RESTART WITH 1;
TRUNCATE app_memos RESTART IDENTITY;
TRUNCATE arp_logs RESTART IDENTITY;
TRUNCATE db_errs RESTART IDENTITY;
TRUNCATE host_service_logs RESTART IDENTITY;
TRUNCATE host_service_noalarms RESTART IDENTITY;
-- DELETE FROM mactab;
-- DELETE FROM mactab_logs;
UPDATE host_services SET
	host_service_state = 'unknown',
	soft_state = 'unknown',
	hard_state = 'unknown',
	state_msg = NULL,
	check_attempts = 0,
	last_changed = NULL,
	last_touched = NULL,
	act_alarm_log_id = NULL,
	last_alarm_log_id = NULL,
	last_noalarm_msg = NULL;
UPDATE service_vars SET
	service_var_value = NULL,
	raw_value = NULL,
	var_state = 'unknown',
	last_time = NULL;
UPDATE nodes SET
	node_stat = 'unknown';
UPDATE interfaces SET
	port_ostat = 'unknown',
	port_astat = 'unknown',
	port_stat = 'unknown';
TRUNCATE mactab_logs RESTART IDENTITY;
TRUNCATE ip_address_logs RESTART IDENTITY;
TRUNCATE dyn_ipaddress_logs RESTART IDENTITY;
