-- VIEW

CREATE OR REPLACE VIEW view_alarms AS
 SELECT a.alarm_id, a.host_service_id, a.first_status, a.max_status, a.last_status, a.begin_time,
        a.event_descr, a.superior_alarm, a.noalarm, a.end_time, a.msg_time, a.alarm_time, a.notice_time,
        a.notice_user, a.ack_time, a.ack_user, a.ack_msg, a.daemon_id,
        hs.host_service_descr, hs.host_service_alarm_msg,
        s.service_id, s.service_name, s.service_descr, s.service_alarm_msg,
        n.node_id, n.node_name, n.node_descr, n.node_alarm_msg,
        p.place_id, p.place_name, p.place_descr, p.image,
        (select pa.image from places pa where pa.place_id = p.parent_id) AS parent_image,
        p.frame, p.tel, p.parent_id, p.place_alarm_msg
   FROM alarms a
   JOIN host_services hs USING (host_service_id)
   JOIN services s USING (service_id)
   JOIN nodes n USING (node_id)
   JOIN places p USING (place_id)
  WHERE a.superior_alarm IS NULL AND a.noalarm IS FALSE AND a.ack_time IS NULL;

ALTER TABLE view_alarms OWNER TO lanview2;

CREATE OR REPLACE VIEW view_archive_alarms AS
 SELECT a.alarm_id, a.host_service_id, a.first_status, a.max_status, a.last_status, a.begin_time,
        a.event_descr, a.superior_alarm, a.noalarm, a.end_time, a.msg_time, a.alarm_time, a.notice_time,
        (SELECT CASE WHEN (au.first_name || ' ' || au.last_name) <> ' ' THEN (au.first_name || ' ' || au.last_name) ELSE au.username END FROM auth_user au WHERE au.id = a.notice_user) AS notice_user,
        a.ack_time,
        (SELECT CASE WHEN (au.first_name || ' ' || au.last_name) <> ' ' THEN (au.first_name || ' ' || au.last_name) ELSE au.username END FROM auth_user au WHERE au.id = a.ack_user) AS ack_user,
        a.ack_msg, a.daemon_id,
        hs.host_service_descr, hs.host_service_alarm_msg,
        s.service_id, s.service_name, s.service_descr, s.service_alarm_msg,
        n.node_id, n.node_name, n.node_descr, n.node_alarm_msg,
        p.place_id, p.place_name, p.place_descr, p.image,
        (select pa.image from places pa where pa.place_id = p.parent_id) AS parent_image,
        p.frame, p.tel, p.parent_id, p.place_alarm_msg
   FROM alarms a
   JOIN host_services hs USING (host_service_id)
   JOIN services s USING (service_id)
   JOIN nodes n USING (node_id)
   JOIN places p USING (place_id)
  WHERE a.ack_time IS NOT NULL;

ALTER TABLE view_archive_alarms OWNER TO lanview2;