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
  
CREATE OR REPLACE VIEW node_port_vlans AS
  SELECT node_id, port_id, port_name, port_index, vlan_id, vlan_name, vlan_note, vlan_type
  FROM port_vlans
  JOIN nports     USING(port_id)
  JOIN vlans      USING(vlan_id);

CREATE OR REPLACE VIEW vlan_list_by_host AS
  SELECT DISTINCT   node_id, vlan_id, node_name, vlan_name, vlan_note, vlan_stat
  FROM port_vlans 
  JOIN nports     USING(port_id)
  JOIN vlans      USING(vlan_id)
  JOIN nodes      USING(node_id);
  
  
CREATE OR REPLACE FUNCTION host_service_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    name TEXT;
    proto TEXT;
    prime TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT
            n.node_name || CASE WHEN p.port_name IS NULL THEN '' ELSE ':' || p.port_name END || '.' || s.service_name,
            sprime.service_name,
            sproto.service_name
          INTO name, prime, proto
        FROM host_services hs
        JOIN nodes n USING(node_id)
        JOIN services s USING(service_id)
        LEFT JOIN nports p ON hs.port_id = p.port_id			-- ez lehet NULL
        JOIN services sproto ON hs.proto_service_id = sproto.service_id	-- a 'nil' nevű services a NULL
        JOIN services sprime ON hs.prime_service_id = sprime.service_id	-- a 'nil' nevű services a NULL
        WHERE host_service_id = $1;
    IF NOT FOUND THEN
        -- PERFORM error('IdNotFound', $1, 'host_service_id', 'host_service_id2name()', 'host_services');
        RETURN NULL;
    END IF;
    IF proto = 'nil' AND prime = 'nil' THEN
        RETURN name;
    ELSIF proto = 'nil' THEN
        RETURN name || '(:' || prime || ')';
    ELSIF prime = 'nil' THEN
        RETURN name || '(' || proto || ':)';
    ELSE
        RETURN name || '(' || proto || ':' || prime || ')';
    END IF;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION host_service_id2name(bigint) IS 'A host_service_id -ből a hivatkozott rekord alapján előállít egy egyedi nevet.
    Ha nincs ilyen rekord, vagy a paraméter NULL, akkor NULL-lal tér vissza.';


CREATE OR REPLACE FUNCTION alarm_message(sid bigint, st notifswitch) RETURNS text AS $$
DECLARE
    msg         text DEFAULT NULL;
    hs          host_services;
    s           services;
    n           nodes;
    pl          places;
    lid		integer;
    tid         bigint DEFAULT NULL;         -- text id
BEGIN
    SELECT * INTO hs FROM host_services WHERE host_service_id = sid;
    IF NOT FOUND THEN
        RETURN '#' || sid::text || ' deleted or invalid service is ' || st::text;
    END IF;
    
    SELECT * INTO s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        s.service_name := 'unknown';
    ELSE
        SELECT text_id INTO tid FROM alarm_messages WHERE service_type_id = s.service_type_id AND status = st;
    END IF;
    -- RAISE INFO 'text_id = %', tid;
    IF tid IS NOT NULL THEN
	lid = get_language_id();
	-- RAISE INFO 'Get text, lid = %', lid;
        SELECT texts[1] INTO msg FROM localizations WHERE text_id = tid AND table_for_text = 'alarm_messages' AND language_id = lid;
        IF NOT FOUND THEN
            -- RAISE INFO 'localization (%) text (%) not found', lid, tid;
            msg := COALESCE(
                (SELECT texts[1] FROM localizations WHERE text_id = tid AND table_for_text = 'alarm_messages' AND language_id = get_int_sys_param('default_language')),
                (SELECT texts[1] FROM localizations WHERE text_id = tid AND table_for_text = 'alarm_messages' AND language_id = get_int_sys_param('failower_language')));
        END IF;
    END IF;
    IF msg IS NULL THEN
	-- RAISE INFO 'localization text not found, set default';
        -- msg := '$hs.name is $st';
        RETURN host_service_id2name(sid) || ' is ' || st;
    END IF;

    msg := replace(msg, '$st', st::text);
    IF msg LIKE '%$hs.%' THEN 
        IF msg LIKE '%$hs.name%' THEN
            msg := replace(msg, '$hs.name',  host_service_id2name(sid));
        END IF;
        msg := replace(msg, '$hs.note',  COALESCE(hs.host_service_note, ''));
    END IF;

    IF msg LIKE '%$s.%' THEN 
        msg := replace(msg, '$s.name',   s.service_name);
        msg := replace(msg, '$s.note',   COALESCE(s.service_note, ''));
    END IF;

    IF msg LIKE '%$n.%' OR msg LIKE '%$pl.%' THEN
        SELECT * INTO n FROM nodes WHERE node_id = hs.node_id;
        IF NOT FOUND THEN
            msg := replace(msg, '$n.name',     'unknown');
            msg := replace(msg, '$n.note',     '');
            msg := replace(msg, '$pl.name',    'unknown');
            msg := replace(msg, '$pl.note',    '');
            RETURN msg;
        END IF;
        IF msg LIKE '%$n.%' THEN
            msg := replace(msg, '$n.name',     n.node_name);
            msg := replace(msg, '$n.note',     COALESCE(n.node_note, ''));
        END IF;
        IF msg LIKE '%$pl.%' THEN
            SELECT * INTO pl FROM placess WHERE place_id = n.place_id;
            IF NOT FOUND THEN
                msg := replace(msg, '$pl.name','unknown');
                msg := replace(msg, '$pl.note','');
            ELSE
                msg := replace(msg, '$pl.name',pl.place_name);
                msg := replace(msg, '$pl.note',COALESCE(pl.place_note, ''));
            END IF;
        END IF;
    END IF;
    RETURN msg;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION alarm_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    rname TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT alarm_message(host_service_id, max_status) INTO rname FROM alarms WHERE alarm_id = $1;
    IF NOT FOUND THEN
        rname := 'not found'; 
    END IF;
    RETURN rname || ' / #' || $1;
END;
$$ LANGUAGE plpgsql;

ALTER TYPE fieldflag ADD VALUE 'raw';
ALTER TABLE table_shape_fields DROP CONSTRAINT table_shape_fields_table_shape_id_table_shape_field_name_key;
CREATE INDEX table_shape_fields_table_shape_id_index ON table_shape_fields(table_shape_id);

  
SELECT set_db_version(1, 13);
END;