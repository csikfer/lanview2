DELETE FROM alarms;
DELETE FROM app_errs;
DELETE FROM app_memos;
DELETE FROM arp_logs;
DELETE FROM db_errs;
DELETE FROM host_service_logs;
DELETE FROM host_service_noalarms;

UPDATE host_services
   SET noalarm_flag='off', host_service_state='unknown', soft_state='unknown', hard_state='unknown', 
       state_msg=NULL, check_attempts=0, last_changed=NULL, last_touched=NULL, 
       act_alarm_log_id=NULL, last_alarm_log_id=NULL;

DELETE FROM mactab;
DELETE FROM mactab_logs;