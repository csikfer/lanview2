BEGIN;  -- Version 1.12 ==> 1.13

CREATE OR REPLACE VIEW named_host_services AS
  SELECT 
    hs.host_service_id,
    hs.node_id,
    n.node_name,
    hs.service_id,
    s.service_name,
    hs.port_id,
    hs.host_service_note,
    hs.prime_service_id,
    pri.service_name AS pri_service_name,
    hs.proto_service_id,
    pro.service_name AS pro_service_name,
    hs.delegate_host_state,
    COALESCE(hs.check_cmd, s.check_cmd, pri.check_cmd, pro.check_cmd) AS check_cmd,
    hs.features,
    s.features AS s_features,
    pri.features AS pri_features,
    pro.features AS pro_features,
    hs.disabled,
    s.disabled AS s_disabled,
    hs.superior_host_service_id,
    host_service_id2name(hs.superior_host_service_id) AS superior_host_service_name,
    COALESCE(hs.max_check_attempts, s.max_check_attempts, pri.max_check_attempts, pro.max_check_attempts) AS max_check_attempts,
    COALESCE(hs.normal_check_interval, s.normal_check_interval, pri.normal_check_interval, pro.normal_check_interval) AS normal_check_interval,
    COALESCE(hs.retry_check_interval, s.retry_check_interval, pri.retry_check_interval, pro.retry_check_interval) AS retry_check_interval,
    COALESCE(hs.timeperiod_id, s.timeperiod_id, pri.timeperiod_id, pro.timeperiod_id) AS timeperiod_id,
    COALESCE(hs.flapping_interval, s.flapping_interval, pri.flapping_interval, pro.flapping_interval) as flapping_interval,
    COALESCE(hs.flapping_max_change, s.flapping_max_change, pri.flapping_max_change, pro.flapping_max_change) AS flapping_max_change,
    hs.noalarm_flag, 
    hs.noalarm_from,
    hs.noalarm_to,
    COALESCE(hs.offline_group_ids, s.offline_group_ids, pri.offline_group_ids, pro.offline_group_ids) AS offline_group_ids,
    COALESCE(hs.online_group_ids, s.online_group_ids, pri.online_group_ids, pro.online_group_ids) AS online_group_ids,
    hs.host_service_state,
    hs.soft_state,
    hs.hard_state,
    hs.state_msg,
    hs.check_attempts,
    hs.last_changed,
    hs.last_touched,
    hs.act_alarm_log_id,
    hs.last_alarm_log_id,
    hs.deleted,
    hs.last_noalarm_msg,
    COALESCE(hs.heartbeat_time, s.heartbeat_time, pri.heartbeat_time, pro.heartbeat_time) AS heartbeat_time
  FROM host_services AS hs
  JOIN nodes    AS n   USING(node_id)
  JOIN services AS s   USING(service_id) 
  JOIN services AS pri ON pri.service_id = hs.prime_service_id
  JOIN services AS pro ON pro.service_id = hs.proto_service_id;
  
  
CREATE OR REPLACE VIEW vlan_list_by_host AS
  SELECT DISTINCT   node_id, vlan_id, node_name, vlan_name, vlan_note, vlan_stat
  FROM port_vlans AS pv
  JOIN nports     AS p  USING(port_id)
  JOIN vlans      AS v  USING(vlan_id)
  JOIN nodes      AS n  USING(node_id);
  
  
SELECT set_db_version(1, 13);
END;