
CREATE TABLE port_vlan_logs (
    port_vlan_log_id    bigserial       PRIMARY KEY,
    date_of             timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    reason              reasons         NOT NULL,
    port_id             bigint          NOT NULL,
    vlan_id             bigint          NOT NULL
         REFERENCES vlans(vlan_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    old_type            vlantype        NOT NULL,
    first_time_old      timestamp without time zone,
    last_time_old       timestamp without time zone,
    new_type            vlantype        DEFAULT NULL,
    acknowledged        boolean         DEFAULT false
);
CREATE INDEX port_vlan_logs_date_of_index ON port_vlan_logs (date_of);
CREATE INDEX port_vlan_logs_port_id       ON port_vlan_logs (port_id);
CREATE INDEX port_vlan_logs_vlan_id       ON port_vlan_logs (vlan_id);
ALTER TABLE port_vlan_logs OWNER TO lanview2;

CREATE OR REPLACE VIEW named_port_vlans AS
    SELECT port_vlan_id, node_id, node_name, port_id, port_name, vlan_id, vlan_name, vlan_type 
    FROM port_vlans JOIN vlans USING(vlan_id) JOIN nports USING(port_id) JOIN nodes USING(node_id);

CREATE OR REPLACE FUNCTION delete_port_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM port_params       WHERE port_id = OLD.port_id;
    DELETE FROM host_services     WHERE port_id = OLD.port_id;
    DELETE FROM port_vlans        WHERE port_id = OLD.port_id;
    DELETE FROM port_vlan_logs    WHERE port_id = OLD.port_id;
    DELETE FROM mactab            WHERE port_id = OLD.port_id;
    DELETE FROM phs_links_table   WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    DELETE FROM lldp_links_table  WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION update_port_vlan(pid bigint, vid bigint, vt vlantype, st settype DEFAULT 'query'::settype) RETURNS reasons AS $$
DECLARE
    rec port_vlans;
    r reasons;
BEGIN
    SELECT * INTO rec FROM port_vlans WHERE port_id = pid AND vlan_id = vid;
    IF NOT FOUND THEN
        IF 0 = COUNT(*) FROM vlans WHERE vlan_id = vid THEN
            INSERT INTO vlans(vlan_id, vlan_name) VALUES(vid, 'AUTO_INSERTED_VLAN' || vid::text);
            r := 'new';
        ELSE
            r := 'insert';
        END IF;
        INSERT INTO port_vlans (port_id, vlan_id, vlan_type, set_type, flag) VALUES (pid, vid, vt, st, true);
        RETURN r;
    END IF;
    IF rec.vlan_type = vt THEN
        UPDATE port_vlans SET last_time = now(), flag = true WHERE port_id = pid AND vlan_id = vid;
        RETURN 'unchange';
    END IF;
    UPDATE port_vlans SET vlan_type = vt, set_type = st, first_time = now(), last_time = now(), flag = true WHERE port_id = pid AND vlan_id = vid;
    INSERT INTO port_vlan_logs(reason, port_id, vlan_id,   old_type, first_time_old, last_time_old, new_type)
                        VALUES('modify', pid,    vid, rec.vlan_type, rec.first_time, rec.last_time, vt);
    RETURN 'modify';
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION rm_unmarked_port_vlan(nid bigint) RETURNS integer AS $$
DECLARE
    rec port_vlans;
    n integer;
BEGIN
    n := 0;
    FOR rec IN SELECT port_vlans.* FROM port_vlans JOIN nports USING(port_id) WHERE port_vlans.flag = false AND node_id = nid LOOP
        n := n + 1;
        INSERT INTO port_vlan_logs(reason,       port_id,     vlan_id,      old_type, first_time_old, last_time_old)
                            VALUES('remove', rec.port_id, rec.vlan_id, rec.vlan_type, rec.first_time, rec.last_time);
        DELETE FROM port_vlans WHERE port_vlan_id = rec.port_vlan_id;
        
    END LOOP;
    RETURN n;
END;
$$ LANGUAGE plpgsql;

INSERT INTO services(service_name, superior_service_mask, check_cmd, features,
                     max_check_attempts, normal_check_interval, flapping_interval, flapping_max_change)
            VALUES  ('portvlan', 'lv2d', 'portvlan $S -R $host_service_id', ':process=continue:timing=passive:superior:logrot=500M,8:method=inspector:',
                     1, '00:05:00', '00:30:00' , 5);
INSERT INTO services(service_name, superior_service_mask, features,
                     max_check_attempts, normal_check_interval, retry_check_interval, flapping_interval, flapping_max_change)
            VALUES  ('pvlan', 'portvlan', ':timing=timed:delay=1000:', 3, '00:30:00', '00:05:00', '03:00:00', 4 );
            
ALTER TABLE patchs ADD COLUMN location point DEFAULT NULL;

-- ************************************************* BUGFIX! *********************************************************************

INSERT INTO unusual_fkeys
  ( table_name,   column_name, unusual_fkeys_type, f_table_name, f_column_name, f_inherited_tables) VALUES
  ( 'port_vlans', 'port_id',   'owner',            'nports',     'port_id',     '{interfaces}');
  
-- *********************************************** END BUGFIX! *******************************************************************
  
  
SELECT set_db_version(1, 12);