-- Hibajavítás
ALTER TABLE service_var_types ALTER COLUMN service_var_type SET DEFAULT 'GAUGE';
UPDATE service_var_types SET service_var_type = DEFAULT WHERE service_var_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN service_var_type SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN raw_to_rrd SET DEFAULT false;
UPDATE service_var_types SET raw_to_rrd = DEFAULT WHERE raw_to_rrd IS NULL;
ALTER TABLE service_var_types ALTER COLUMN raw_to_rrd SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN plausibility_type SET DEFAULT 'no';
UPDATE service_var_types SET plausibility_type = DEFAULT WHERE plausibility_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN plausibility_type SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN warning_type SET DEFAULT 'no';
UPDATE service_var_types SET warning_type = DEFAULT WHERE warning_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN warning_type SET NOT NULL;

ALTER TABLE service_var_types ALTER COLUMN critical_type SET DEFAULT 'no';
UPDATE service_var_types SET critical_type = DEFAULT WHERE critical_type IS NULL;
ALTER TABLE service_var_types ALTER COLUMN critical_type SET NOT NULL;

DROP TRIGGER IF EXISTS service_vars_check_value     ON public.service_rrd_vars;
DROP TRIGGER IF EXISTS service_rrd_vars_check_value ON public.service_rrd_vars;
DROP TRIGGER IF EXISTS service_vars_check_value     ON public.service_vars;

CREATE OR REPLACE FUNCTION check_before_service_value()
  RETURNS trigger AS
$BODY$
DECLARE
    tids record;
    pt   paramtype;
BEGIN
    IF TG_OP = 'INSERT' THEN
        IF 0 < COUNT(*) FROM service_vars WHERE service_var_id = NEW.service_var_id THEN
            PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_id', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF 0 < COUNT(*) FROM service_vars WHERE service_var_name = NEW.service_var_name AND host_service_id = NEW.host_service_id THEN
            PERFORM error('IdNotUni', NEW.service_var_id, 'service_var_name', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
    ELSIF TG_OP = 'UPDATE' THEN
        IF OLD.service_var_id <> NEW.service_var_id THEN
            PERFORM error('Constant', OLD.service_var_id, 'service_var_id', 'check_before_service_value()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        IF OLD.service_var_name <> NEW.service_var_name THEN
            IF 0 < COUNT(*) FROM service_vars WHERE service_var_name = NEW.service_var_name AND host_service_id = NEW.host_service_id THEN
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
$BODY$
  LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION check_before_service_value() OWNER TO lanview2;


CREATE TRIGGER service_vars_check_value     BEFORE INSERT OR UPDATE OR DELETE ON service_vars
    FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();
CREATE TRIGGER service_rrd_vars_check_value BEFORE INSERT OR UPDATE OR DELETE ON service_rrd_vars
    FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();

CREATE OR REPLACE FUNCTION service_rrd_value_after() RETURNS trigger AS
$BODY$
DECLARE
    val text;
    pt paramtype;
    payload text;
BEGIN
    IF NOT NEW.rrd_disabled AND NEW.raw_value IS NOT NULL AND NEW.last_time IS NOT NULL THEN -- Next value?
        IF OLD.last_time IS NULL OR (NEW.last_time - OLD.last_time) > '1 second'::interval THEN
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
                    val := extract(EPOCH FROM val::interval)::text;
                END IF;
                payload := 'rrd '
                        || NEW.rrdhelper_id::text || ' '
                        || extract(EPOCH FROM NEW.last_time  AT TIME ZONE 'CETDST' AT TIME ZONE 'UTC')::bigint::text || ' '
                        || val || ' ' 
                        || NEW.service_var_id::text;
                PERFORM pg_notify('rrdhelper', payload);
            END IF;
        END IF;
    END IF;
    RETURN NEW;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION service_rrd_value_after() OWNER TO lanview2;

-- !!!!!!!!!!!!!!!!!!!!!!!!!!! Nem biztos, hogy végleges !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
-- Grafikonok
-- régebbi probálkozás törlése
DROP TABLE IF EXISTS graphs, graph_vars CASCADE;

ALTER TYPE tablefortext ADD VALUE IF NOT EXISTS TYPE service_diagram_types;
CREATE TYPE srvdiagramtypetext AS ENUM ('title', 'vertical_label');

CREATE TABLE srv_diagram_types (
    srv_diagram_type_id         bigserial   PRIMARY KEY,
    srv_diagram_type_name       text        NOT NULL,
    srv_diagram_type_note       text        DEFAULT NULL,
    srv_id                      bigint      NOT NULL
                REFERENCES services(service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    text_id                     bigint      NOT NULL DEFAULT nextval('text_id_sequ'),
    height                      integer     NOT NULL CHECK height >  50 AND height <= 2160,
    width                       integer     NOT NULL CHECK width  > 100 AND width  <= 2840,
    hrule                       text        NOT NULL DEFAULT '',
    features                    text        DEFAULT NULL,
    UNIQUE (srv_diagram_type_name, service_id)
);

CREATE TABLE srv_diagram_type_vars (
    srv_diagram_type_var_id     bigserial   PRIMARY KEY,
    srv_diagram_type_var_name   text        NOT NULL,
    srv_diagram_type_var_note   text        DEFAULT NULL,
    srv_diagram_type_id         bigint      NOT NULL
                REFERENCES srv_diagram_types(srv_diagram_type_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    service_var_name            text        NOT NULL,
    calculation                 text        DEFAULT NULL,
    features                    text        DEFAULT NULL,
    UNIQUE (srv_diagram_type_id, srv_diagram_type_var_name),
    UNIQUE (srv_diagram_type_id, service_var_name)
};

CREATE TABLE service_diagrams (
    service_diagram_id          bigserial   NOT NULL PRIMARY KEY,
    service_id                  bigint      NOT NULL
                REFERENCES services(service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    srv_diagram_type_id         bigint      NOT NULL
                REFERENCES srv_diagram_types(srv_diagram_type_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    features                    text        DEFAULT NULL,
    UNIQUE (service_diagram_name, service_id)
);
ALTER TABLE service_diagrams OWNER TO lanview2;
COMMENT ON TBALE service_diagrams IS 'Szolgáltatás lekérdezéshez rendelt diagram leírója, ill kapcsoló rekord';


-- ******************************
-- SELECT set_db_version(1, 28);
