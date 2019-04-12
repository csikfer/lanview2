DELETE FROM user_events;
ALTER SEQUENCE user_events_user_event_id_seq RESTART WITH 1;
DELETE FROM alarms;
ALTER SEQUENCE alarms_alarm_id_seq RESTART WITH 1;
DELETE FROM app_errs;
ALTER SEQUENCE app_errs_applog_id_seq RESTART WITH 1;
DELETE FROM app_memos;
ALTER SEQUENCE app_memos_app_memo_id_seq RESTART WITH 1;
DELETE FROM arp_logs;
ALTER SEQUENCE arp_logs_arp_log_id_seq RESTART WITH 1;
DELETE FROM db_errs;
ALTER SEQUENCE db_errs_dblog_id_seq RESTART WITH 1;
DELETE FROM host_service_logs;
ALTER SEQUENCE host_service_logs_host_service_log_id_seq RESTART WITH 1;
DELETE FROM host_service_noalarms;
ALTER SEQUENCE host_service_noalarms_host_service_noalarm_id_seq RESTART WITH 1;
-- DELETE FROM mactab;
DELETE FROM mactab_logs;
ALTER SEQUENCE mactab_logs_mactab_log_id_seq RESTART WITH 1;
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
