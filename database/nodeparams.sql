CREATE TABLE node_params (
    node_param_id       bigserial       PRIMARY KEY,
    param_type_id       bigint          NOT NULL
            REFERENCES param_types(param_type_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    node_id             bigint          NOT NULL,   -- REFERENCES nodes(node_id)
    param_value         text            DEFAULT NULL,
    flag                boolean         DEFAULT false,
    UNIQUE (param_type_id, node_id)
);
ALTER TABLE node_params OWNER TO lanview2;
COMMENT ON TABLE  node_params IS 'Nod extra paraméter értékek.';
COMMENT ON COLUMN node_params.node_param_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN node_params.param_type_id IS 'A paraméter tulajdonságait definiáló param_types rekord azonosítója.';
COMMENT ON COLUMN node_params.node_id IS 'A tulajdonos node rekordjának az azonosítója.';
COMMENT ON COLUMN node_params.param_value IS 'A parméter érték.';

CREATE OR REPLACE FUNCTION set_str_node_param(pid bigint, txtval text, tname text DEFAULT 'text') RETURNS reasons AS $$
DECLARE
    type_id bigint;
BEGIN
    SELECT param_type_id INTO type_id FROM param_types WHERE param_type_name = tname;
    IF NOT FOUND THEN
        RETURN 'notfound';
    END IF;
    IF 0 < COUNT(*) FROM node_params WHERE node_id = pid AND param_type_id = type_id AND param_value = txtval THEN
        RETURN 'found';
    END IF;
    UPDATE node_params SET param_value = txtval WHERE node_id = pid AND param_type_id = type_id;
    IF NOT FOUND THEN
        INSERT INTO node_params(node_id, param_type_id, param_value) VALUES (pid, type_id, txtval);
        RETURN 'insert';
    END IF;
    RETURN 'modify';
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_bool_node_param(pid bigint, boolval boolean, tname text DEFAULT 'boolean') RETURNS reasons AS $$
BEGIN
    RETURN set_str_node_param(pid, boolval::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_int_node_param(pid bigint, intval bigint, tname text DEFAULT 'bigint') RETURNS reasons AS $$
BEGIN
    RETURN set_str_node_param(pname, intval::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_interval_node_param(pid bigint, ival interval, tname text DEFAULT 'interval') RETURNS reasons AS $$
BEGIN
    RETURN set_str_node_param(pid, ival::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_str_node_param(pid bigint, tname text DEFAULT 'text') RETURNS text AS $$
DECLARE
    res text;
    type_id bigint;
BEGIN
    SELECT param_type_id INTO type_id FROM param_types WHERE param_type_name = tname;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    SELECT param_value INTO res FROM node_params WHERE node_id = pid AND param_type_id = type_id;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    RETURN res;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_bool_node_param(pid bigint, tname text DEFAULT 'boolean') RETURNS boolean AS $$
BEGIN
    IF get_str_node_param(pid,tname)::boolean THEN
        RETURN true;
    ELSE
        RETURN false;
    END IF;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_int_node_param(pid bigint, tname text DEFAULT 'bigint') RETURNS bigint AS $$
BEGIN
    RETURN get_str_node_param(pid,tname)::bigint;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_interval_node_param(pid bigint, tname text DEFAULT 'interval') RETURNS interval AS $$
BEGIN
    RETURN get_str_node_param(pname,tname)::interval;
END
$$ LANGUAGE plpgsql;

INSERT INTO param_types
    (param_type_name,    param_type_type,   param_type_note)    VALUES
    ('inventory_number',    'text',          'Leltári szám'),
    ('serial_number',       'text',          'Gyári szám');
