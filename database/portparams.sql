CREATE TABLE port_params (
    port_param_id       bigserial      PRIMARY KEY,
    param_type_id       bigint         NOT NULL
            REFERENCES param_types(param_type_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    port_id             bigint         NOT NULL,   -- REFERENCES nports(port_id) kivéve pports
    param_value         text   DEFAULT NULL,
    UNIQUE (param_type_id, port_id)
);
ALTER TABLE port_params OWNER TO lanview2;
COMMENT ON TABLE  port_params IS 'Port extra paraméter értékek.';
COMMENT ON COLUMN port_params.port_param_id IS 'A paraméter érték egyedi azonosítója.';
COMMENT ON COLUMN port_params.param_type_id IS 'A paraméter tulajdonságait definiáló param_types rekord azonosítója.';
COMMENT ON COLUMN port_params.port_id IS 'A tulajdonos port rekordjának az azonosítója.';
COMMENT ON COLUMN port_params.param_value IS 'A parméter érték.';

CREATE OR REPLACE FUNCTION set_str_port_param(pid bigint, txtval text, tname varchar(32) DEFAULT 'text') RETURNS reasons AS $$
DECLARE
    type_id bigint;
BEGIN
    SELECT param_type_id INTO type_id FROM param_types WHERE param_type_name = tname;
    IF NOT FOUND THEN
        RETURN 'notfound';
    END IF;
    IF 0 < COUNT(*) FROM port_params WHERE port_id = pid AND param_type_id = type_id AND port_param_value = txtval THEN
        RETURN 'found';
    END IF;
    UPDATE port_params SET param_value = txtval WHERE port_id = pid AND param_type_id = type_id;
    IF NOT FOUND THEN
        INSERT INTO port_params(port_id, param_type_id, param_value) VALUES (pid, type_id, txtval);
        RETURN 'insert';
    END IF;
    RETURN 'modify';
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_bool_port_param(pid bigint, boolval boolean, tname varchar(32) DEFAULT 'boolean' ) RETURNS reasons AS $$
BEGIN
    RETURN set_str_port_param(pid, boolval::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_int_port_param(pid bigint, intval bigint, tname varchar(32) DEFAULT 'bigint') RETURNS reasons AS $$
BEGIN
    RETURN set_str_port_param(pname, intval::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_interval_port_param(pid bigint, ival interval, tname varchar(32) DEFAULT 'interval') RETURNS reasons AS $$
BEGIN
    RETURN set_str_port_param(pid, ival::text, tname);
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_str_port_param(pid bigint, tname varchar(32) DEFAULT 'text') RETURNS text AS $$
DECLARE
    res text;
    type_id bigint;
BEGIN
    SELECT param_type_id INTO type_id FROM param_types WHERE param_type_name = tname;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    SELECT param_value INTO res FROM port_params WHERE port_id = pid AND param_type_id = type_id;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    RETURN res;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_bool_port_param(pid bigint, tname varchar(32) DEFAULT 'boolean') RETURNS boolean AS $$
BEGIN
    IF get_str_port_param(pid,tname)::boolean THEN
        RETURN true;
    ELSE
        RETURN false;
    END IF;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_int_port_param(pid bigint, tname varchar(32) DEFAULT 'bigint') RETURNS bigint AS $$
BEGIN
    RETURN get_str_port_param(pid,tname)::bigint;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_interval_port_param(pid bigint, tname varchar(32)  DEFAULT 'interval') RETURNS interval AS $$
BEGIN
    RETURN get_str_port_param(pname,tname)::interval;
END
$$ LANGUAGE plpgsql;
