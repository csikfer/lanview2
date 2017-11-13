-- bugfixes

ALTER TABLE localizations DROP CONSTRAINT localizations_pkey;
ALTER TABLE localizations ADD  CONSTRAINT localizations_pkey PRIMARY KEY (text_id, language_id);

CREATE OR REPLACE FUNCTION delete_record_text() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM localizations WHERE text_id = OLD.text_id AND table_for_text = TG_TABLE_NAME::tablefortext;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_language_id() RETURNS integer AS $$
BEGIN
    RETURN CAST(current_setting('lanview2.language_id') AS integer);
    EXCEPTION
        WHEN OTHERS THEN
            RETURN COALESCE(get_int_sys_param('default_language'), get_int_sys_param('failower_language'));
END;
$$ LANGUAGE plpgsql;


-- bugfixes end

ALTER TABLE table_shape_fields DROP COLUMN IF EXISTS filter_types;
DELETE FROM table_shape_fields WHERE table_shape_field_name = 'filter_types';
ALTER TABLE service_var_types ALTER COLUMN plausibility_type SET DATA TYPE text;
ALTER TABLE service_var_types ALTER COLUMN warning_type      SET DATA TYPE text;
ALTER TABLE service_var_types ALTER COLUMN critical_type     SET DATA TYPE text;
DROP   TYPE filtertype;
CREATE TYPE filtertype AS ENUM
    ('no', 'begin', 'like', 'similar', 'regexp', 'equal', 'litle', 'big', 'interval', 'boolean', 'enum', 'set', 'null', 'SQL');
ALTER TYPE filtertype OWNER TO lanview2;
COMMENT ON TYPE filtertype IS '
no      no filtering
begin   The specified string matches the beginning of the search text.
like    The string specified matches the search string (SQL LIKE operator).
similar The string specified matches the search string (SQL SIMILAR operator).
regexp  The search string matches the specified regular expression.
equal   
litle   The value you are looking for is less than
big     The value you are looking for is biger than
interval The value you are looking for is in the specified range
boolean Value as logical.
enum    Filter for a ENUM type
set     Filter for a SET type
null    Is NULL
SQL	Filter by SQL WHERE expression';

ALTER TABLE service_var_types ALTER COLUMN plausibility_type SET DATA TYPE filtertype USING plausibility_type::filtertype;
ALTER TABLE service_var_types ALTER COLUMN warning_type      SET DATA TYPE filtertype USING warning_type::filtertype;
ALTER TABLE service_var_types ALTER COLUMN critical_type     SET DATA TYPE filtertype USING critical_type::filtertype;

ALTER TABLE service_var_types ADD COLUMN plausibility_inverse boolean NOT NULL DEFAULT false;
ALTER TABLE service_var_types ADD COLUMN warning_inverse      boolean NOT NULL DEFAULT false;
ALTER TABLE service_var_types ADD COLUMN critical_inverse     boolean NOT NULL DEFAULT false;

CREATE TYPE text2type AS ENUM
    ('bigint', 'double precision', 'time', 'date', 'timestamp', 'interval', 'inet', 'macaddr');

CREATE OR REPLACE FUNCTION text2bigint(val text, def bigint DEFAULT NULL) RETURNS bigint AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::bigint;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2double(val text, def double precision DEFAULT NULL) RETURNS double precision AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::double precision;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2time(val text, def time DEFAULT NULL) RETURNS time AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::time;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2date(val text, def date DEFAULT NULL) RETURNS date AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::date;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2timestamp(val text, def timestamp DEFAULT NULL) RETURNS timestamp AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::timestamp;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2interval(val text, def interval DEFAULT NULL) RETURNS interval AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::interval;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2inet(val text, def inet DEFAULT NULL) RETURNS inet AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::inet;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION text2macaddr(val text, def macaddr DEFAULT NULL) RETURNS macaddr AS $$
BEGIN
    IF val IS NULL THEN
        RETURN def;
    END IF;
    RETURN val::macaddr;
    EXCEPTION
        WHEN OTHERS THEN
            RETURN def;
END;
$$ LANGUAGE plpgsql;


SELECT set_db_version(1, 7);
