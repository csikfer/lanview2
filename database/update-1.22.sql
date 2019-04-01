ALTER TABLE node_params RENAME TO node_params_old;

CREATE TABLE node_params (
  node_param_id bigserial NOT NULL PRIMARY KEY ,
  node_param_name text NOT NULL,
  node_param_note text DEFAULT NULL,
  node_id bigint NOT NULL,
  param_type_id bigint NOT NULL
      REFERENCES param_types (param_type_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE RESTRICT,
  param_value text,
  flag boolean DEFAULT false,
  UNIQUE (node_param_name, node_id)
);
ALTER TABLE node_params OWNER TO lanview2;
COMMENT ON TABLE  node_params IS 'Node extra paraméter értékek.';
COMMENT ON COLUMN node_params.node_param_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN node_params.node_param_name IS 'Paraméter neve.';
COMMENT ON COLUMN node_params.node_param_note IS 'Megjegyzés.';
COMMENT ON COLUMN node_params.node_id IS 'A tulajdonos node rekordjának az azonosítója.';
COMMENT ON COLUMN node_params.param_type_id IS 'A paraméter adat típusát definiáló param_types rekord azonosítója.';
COMMENT ON COLUMN node_params.param_value IS 'A parméter érték.';

WITH npo AS (
    SELECT node_param_id,
           param_type_name AS node_param_name,
           NULL AS node_param_note,
           node_id,
           param_type_id,
           param_value,
           flag
    FROM node_params_old
    JOIN param_types USING(param_type_id) )
INSERT INTO node_params SELECT * FROM npo;

DROP TABLE node_params_old;

CREATE TRIGGER node_param_check_value
  BEFORE INSERT OR UPDATE ON node_params
  FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();

CREATE TRIGGER node_param_value_check_reference_node_id
  BEFORE INSERT OR UPDATE ON node_params
  FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('patchs');

-- - - - - - - - - - - - - - -

DROP FUNCTION IF EXISTS set_str_node_param(nid bigint, txtval text, tname text);
DROP FUNCTION IF EXISTS set_bool_node_param(pid bigint, boolval boolean, tname text);
DROP FUNCTION IF EXISTS set_int_node_param(pid bigint, intval bigint, tname text);
DROP FUNCTION IF EXISTS set_interval_node_param(pid bigint, ival interval, tname text);
DROP FUNCTION IF EXISTS get_str_node_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_bool_node_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_int_node_param(pid bigint, tname text);
DROP FUNCTION IF EXISTS get_interval_node_param(pid bigint, tname text);

-- ----------------------------

ALTER TABLE port_params RENAME TO port_params_old;

CREATE TABLE port_params (
  port_param_id bigserial NOT NULL PRIMARY KEY ,
  port_param_name text NOT NULL,
  port_param_note text DEFAULT NULL,
  port_id bigint NOT NULL,
  param_type_id bigint NOT NULL
      REFERENCES param_types (param_type_id) MATCH FULL
      ON UPDATE RESTRICT ON DELETE RESTRICT,
  param_value text,
  flag boolean DEFAULT false,
  UNIQUE (port_param_name, port_id)
);
ALTER TABLE port_params OWNER TO lanview2;
COMMENT ON TABLE  port_params IS 'port extra paraméter értékek.';
COMMENT ON COLUMN port_params.port_param_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN port_params.port_param_name IS 'Paraméter neve.';
COMMENT ON COLUMN port_params.port_param_note IS 'Megjegyzés.';
COMMENT ON COLUMN port_params.port_id IS 'A tulajdonos port rekordjának az azonosítója.';
COMMENT ON COLUMN port_params.param_type_id IS 'A paraméter adat típusát definiáló param_types rekord azonosítója.';
COMMENT ON COLUMN port_params.param_value IS 'A parméter érték.';

WITH ppo AS (
    SELECT port_param_id,
           param_type_name AS port_param_name,
           NULL AS port_param_note,
           port_id,
           param_type_id,
           param_value,
           flag
    FROM port_params_old
    JOIN param_types USING(param_type_id) )
INSERT INTO port_params SELECT * FROM ppo;

DROP TABLE port_params_old;

CREATE TRIGGER port_param_check_value BEFORE INSERT OR UPDATE ON port_params
  FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();

CREATE TRIGGER port_params_check_reference_port_id BEFORE INSERT OR UPDATE ON port_params
  FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id();

-- - - - - - - - - - - - - - -

DROP FUNCTION IF EXISTS set_str_port_param(bigint, text, text);
DROP FUNCTION IF EXISTS set_bool_port_param(bigint, boolean, text);
DROP FUNCTION IF EXISTS set_int_port_param(bigint, bigint, text);
DROP FUNCTION IF EXISTS set_interval_port_param(bigint, interval, text);
DROP FUNCTION IF EXISTS get_str_port_param(bigint, text);
DROP FUNCTION IF EXISTS get_bool_port_param(bigint, text);
DROP FUNCTION IF EXISTS get_int_port_param(bigint, text);
DROP FUNCTION IF EXISTS get_interval_port_param(bigint, text);

-- For replace_mactab()
CREATE FUNCTION get_bool_port_param(pid bigint, name text) RETURNS boolean AS $$
BEGIN
    RETURN cast_to_boolean(param_value) FROM port_params WHERE port_id = pid AND port_param_name = name;
END;
$$ LANGUAGE plpgsql;

CREATE FUNCTION set_bool_port_param(pid bigint, val boolean, name text) RETURNS VOID AS $$
BEGIN
    INSERT INTO port_params(port_param_name, port_id, param_type_id, param_value)
        VALUES (name, pid, param_type_name2id('boolean'), val::text)
    ON CONFLICT ON CONSTRAINT port_params_port_param_name_port_id_key DO UPDATE
        SET param_type_id = EXCLUDED.param_type_id, param_value = EXCLUDED.param_value;
END;
$$ LANGUAGE plpgsql;

-- - - - - - - - - - - - - - -

UPDATE port_params SET param_type_id = param_type_name2id('boolean')
    WHERE param_type_id = param_type_name2id('query_mac_tab') OR param_type_id = param_type_name2id('link_is_invisible_for_LLDP')
       OR param_type_id = param_type_name2id('suspected_uplink');

UPDATE node_params SET param_type_id = param_type_name2id('date')
    WHERE param_type_id = param_type_name2id('battery_changed');

DELETE FROM param_types
    WHERE param_type_name = 'search_domain'
       OR param_type_name = 'query_mac_tab' OR param_type_name = 'link_is_invisible_for_LLDP'
       OR param_type_name = 'suspected_uplink'
       OR param_type_name = 'battery_changed';

-- ----------------------------

SELECT set_db_version(1, 22);
