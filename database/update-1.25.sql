-- DROP TABLE rrd_files

DROP VIEW IF EXISTS portvars;
CREATE VIEW portvars AS 
 SELECT sv.service_var_id AS portvar_id,
    sv.service_var_name,
    sv.service_var_note,
    sv.service_var_type_id,
    sv.host_service_id,
    hs.port_id,
    sv.delegate_port_state,
    sv.service_var_value,
    sv.var_state,
    sv.last_time,
    sv.features,
    sv.raw_value,
    sv.delegate_service_state,
    sv.state_msg,
    sv.disabled
   FROM service_vars sv
   JOIN host_services hs USING (host_service_id)
  WHERE NOT sv.deleted AND NOT hs.deleted AND hs.port_id IS NOT NULL;
ALTER TABLE service_vars DROP COLUMN IF EXISTS rrd_file_id;
DROP TABLE IF EXISTS rrd_files;

-- service_vars for RRD

CREATE TABLE IF NOT EXISTS service_rrd_vars (
    rrdhelper_id bigint DEFAULT NULL
        REFERENCES host_services (host_service_id) MATCH SIMPLE
            ON UPDATE RESTRICT ON DELETE RESTRICT,
    rrd_beat_id bigint DEFAULT NULL
        REFERENCES rrd_beats (rrd_beat_id) MATCH SIMPLE
            ON UPDATE RESTRICT ON DELETE RESTRICT,
    rrd_disabled boolean NOT NULL DEFAULT false,
    PRIMARY KEY (service_var_id),
    UNIQUE (service_var_name),
    CONSTRAINT service_vars_host_service_id_fkey FOREIGN KEY (host_service_id)
      REFERENCES host_services (host_service_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE CASCADE,
    CONSTRAINT service_vars_service_var_type_id_fkey FOREIGN KEY (service_var_type_id)
      REFERENCES service_var_types (service_var_type_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE RESTRICT,
    CHECK ((rrdhelper_id IS NULL OR rrd_beat_id IS NULL) AND NOT rrd_disabled)
  ) INHERITS (service_vars);
ALTER TABLE service_rrd_vars OWNER TO lanview2;

-- Remote key management for inheritance (service_vars -> service_rrd_vars)

ALTER TABLE alarm_service_vars DROP CONSTRAINT IF EXISTS alarm_service_vars_service_var_id_fkey;
CREATE OR REPLACE FUNCTION check_alarm_service_vars() RETURNS TRIGGER AS $$
BEGIN
    IF TG_OP = 'UPDATE' AND NEW.service_var_id < 0 THEN -- convert: service_vars -> service_rrd_vars, intermediate state
        RETURN NEW;
    END IF;
    IF 0 = COUNT(*) FROM service_vars WHERE service_var_id = NEW.service_var_id THEN
        PERFORM error('IdNotFound', NEW.service_var_id, 'service_var_id', 'check_alarm_service_vars()', TG_TABLE_NAME, TG_OP);
        RETURN NULL;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER alarm_service_vars_check_value BEFORE INSERT OR UPDATE ON alarm_service_vars FOR EACH ROW EXECUTE PROCEDURE check_alarm_service_vars();

CREATE OR REPLACE FUNCTION service_var2service_rrd_var(vid bigint) RETURNS service_rrd_vars AS $$
DECLARE
    sv  service_vars;
    srv service_rrd_vars;
BEGIN
    SELECT * INTO sv FROM ONLY service_vars WHERE service_var_id = vid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', vid, 'service_var_id', 'service_var2service_rrd_var(bigint)', 'service_vars');
        RETURN NULL;
    END IF;
    UPDATE alarm_service_vars SET service_var_id = -vid WHERE service_var_id = vid;
    DELETE FROM service_vars WHERE service_var_id = vid;
    INSERT INTO service_rrd_vars(
            service_var_id, service_var_name, service_var_note, service_var_type_id, 
            host_service_id, service_var_value, var_state, last_time, features, 
            deleted, raw_value, delegate_service_state, state_msg, delegate_port_state, 
            disabled, flag, rarefaction, rrdhelper_id, rrd_beat_id, rrd_disabled)
        VALUES (
            vid, sv.service_var_name, sv.service_var_note, sv.service_var_type_id, 
            sv.host_service_id, sv.service_var_value, sv.var_state, sv.last_time, sv.features, 
            sv.deleted, sv.raw_value, sv.delegate_service_state, sv.state_msg, sv.delegate_port_state, 
            sv.disabled, sv.flag, sv.rarefaction, NULL, NULL, true)
        RETURNING * INTO srv;
    UPDATE alarm_service_vars SET service_var_id = vid WHERE service_var_id = -vid;
    RETURN srv;
END;
$$ LANGUAGE plpgsql;

-- Drop 'COMPUTE' value from servicevartype

ALTER TABLE service_var_types ALTER COLUMN service_var_type SET DATA TYPE text;
DROP TYPE IF EXISTS servicevartype;
UPDATE service_var_types SET service_var_type = 'GAUGE' WHERE service_var_type = 'COMPUTE';
CREATE TYPE servicevartype AS ENUM ('GAUGE', 'COUNTER', 'DCOUNTER', 'DERIVE', 'DDERIVE', 'ABSOLUTE');
ALTER TYPE servicevartype OWNER TO lanview2;
ALTER TABLE service_var_types ALTER COLUMN service_var_type SET DATA TYPE servicevartype USING service_var_type::servicevartype;
ALTER TABLE service_var_types ADD COLUMN IF NOT EXISTS raw_to_rrd boolean DEFAULT NULL;

CREATE OR REPLACE FUNCTION check_before_service_value() RETURNS trigger AS $$
DECLARE
    tids record;
    pt   paramtype;
BEGIN
    IF TG_OP = 'INSERT' THEN
        IF 0 < COUNT(*) FROM service_vars WHERE service_var_id = NEW.service_var_id THEN
            PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_id', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF 0 < COUNT(*) FROM service_vars WHERE service_var_name = NEW.service_var_id AND host_service_id = NEW.host_service_id THEN
            PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_name', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF TG_TABLE_NAME = 'service_rrd_vars' THEN
            IF (SELECT raw_to_rrd FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id) IS NULL THEN
                PERFORM error('NotNull', NEW.service_var_type_id, 'service_var_types.raw_to_rrd', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
        END IF;
    ELSIF TG_OP = 'UPDATE' THEN
        IF OLD.service_var_id <> NEW.service_var_id THEN
            PERFORM error('Constant', OLD.service_var_id, 'service_var_id', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF OLD.service_var_name <> NEW.service_var_name THEN
            IF COUNT(*) FROM service_vars WHERE host_service_id = NEW.host_service_id AND service_var_name = NEW.service_var_name THEN
                PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_name', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
        END IF;
    ELSIF TG_OP = 'DELETE' THEN
        DELETE FROM alarm_service_vars WHERE service_var_id = OLD.service_var_id;
        RETURN OLD;
    END IF;
    SELECT param_type_id, raw_param_type_id INTO tids FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id;
    IF NEW.service_var_value IS NOT NULL THEN
        SELECT param_type_type INTO pt FROM param_types WHERE param_type_id = tids.param_type_id;
        NEW.service_var_value := check_paramtype(NEW.service_var_value, pt);
    END IF;
    IF NEW.raw_value IS NOT NULL THEN
        SELECT param_type_type INTO pt FROM param_types WHERE param_type_id = tids.raw_param_type_id;
        NEW.raw_value         := check_paramtype(NEW.raw_value, pt);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
ALTER FUNCTION check_before_service_value() OWNER TO lanview2;

CREATE OR REPLACE FUNCTION service_rrd_value_after() RETURNS trigger AS $$
DECLARE
    val text;
    pt paramtype;
BEGIN
    IF NOT NEW.rrd_disabled AND NEW.raw_value IS NOT NULL AND NEW.last_time IS NOT NULL THEN -- Next value?
        IF COALESCE((NEW.last_time > OLD.last_time), true) THEN
            IF raw_to_rrd FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id THEN
                SELECT param_type_type INTO pt
                    FROM param_types       AS pt
                    JOIN service_var_types AS vt ON vt.raw_param_type_id = pt.param_type_id
                    WHERE NEW.service_var_type_id = vt.service_var_type_id;
                val := NEW.raw_value;
            ELSE
                SELECT param_type_type INTO pt
                    FROM param_types       AS pt
                    JOIN service_var_types AS vt USING(param_type_id)
                    WHERE NEW.service_var_type_id = vt.service_var_type_id;
                val := NEW.service_var_value;
            END IF;
            IF pt = 'integer' OR pt = 'real' OR pt = 'interval' THEN -- Numeric
                IF pt = 'interval' THEN
                    val := extract(epoch FROM val::interval)::text;
                END IF;
                PERFORM pg_notify('rrdhelper', 'rrd ' || NEW.rrdhelper_id::text || ' '  || val || ' ' || NEW.service_var_id::text);
            END IF;
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
ALTER FUNCTION service_rrd_value_after() OWNER TO lanview2;

DROP TRIGGER service_vars_check_value ON public.service_vars;
CREATE TRIGGER service_vars_check_value     BEFORE INSERT OR UPDATE OR DELETE ON service_rrd_vars FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();
CREATE TRIGGER service_rrd_vars_check_value BEFORE INSERT OR UPDATE OR DELETE ON service_rrd_vars FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();
CREATE TRIGGER service_rrd_vars_service_rrd_value_after AFTER UPDATE ON service_rrd_vars FOR EACH ROW EXECUTE PROCEDURE service_rrd_value_after();

-- ------------------------------------------------------------------------------------------------
-- bugfix
CREATE OR REPLACE FUNCTION node2snmpdevice(nid bigint) RETURNS snmpdevices AS $$
DECLARE
    node  nodes;
    ntype nodetype[];
    snmpdev snmpdevices;
BEGIN
    SELECT * INTO node FROM ONLY nodes WHERE node_id = nid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', nid, 'node_id', 'node2snmpdevice(bigint)', 'nodes');
        RETURN NULL;
    END IF;
    UPDATE node_params   SET node_id = -nid WHERE node_id = nid;
    UPDATE nports        SET node_id = -nid WHERE node_id = nid;
    UPDATE host_services SET node_id = -nid WHERE node_id = nid;
    DELETE FROM nodes WHERE node_id = nid;
    ntype := array_append(node.node_type, 'snmp'::nodetype);
    ntype := array_replace(ntype, 'node'::nodetype, 'host'::nodetype);
    INSERT INTO
        snmpdevices(node_id, node_name, node_note, node_type, place_id, features, deleted,
            inventory_number, serial_number, model_number, model_name, location,
            node_stat, os_name, os_version)
        VALUES(nid, node.node_name, node.node_note, ntype, node.place_id, node.features, node.deleted,
            node.inventory_number, node.serial_number, node.model_number, node.model_name, node.location,
            node.node_stat, node.os_name, node.os_version)
        RETURNING * INTO snmpdev;
    UPDATE node_params   SET node_id = nid WHERE node_id = -nid;
    UPDATE nports        SET node_id = nid WHERE node_id = -nid;
    UPDATE host_services SET node_id = nid WHERE node_id = -nid;
    RETURN snmpdev;
END
$$ LANGUAGE plpgsql;

ALTER TABLE app_errs RENAME COLUMN sql_err_num TO sql_err_code;
ALTER TABLE app_errs ALTER  COLUMN sql_err_code SET DATA TYPE text USING sql_err_code::text;

SELECT set_db_version(1, 25);
