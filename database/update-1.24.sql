CREATE TYPE regexpattr AS ENUM ('casesensitive', 'exactmatch', 'loop');

ALTER TABLE query_parsers ALTER  COLUMN case_sensitive DROP DEFAULT;
ALTER TABLE query_parsers RENAME COLUMN case_sensitive TO regexp_attr;
ALTER TABLE query_parsers ALTER  COLUMN regexp_attr SET DATA TYPE regexpattr[] USING
    (CASE WHEN  regexp_attr THEN ARRAY['casesensitive', 'exactmatch']
          ELSE                   ARRAY['exactmatch']
    END)::regexpattr[];
ALTER TABLE query_parsers ALTER  COLUMN regexp_attr SET DEFAULT '{exactmatch}';

UPDATE table_shape_fields SET table_field_name = 'regexp_attr', table_shape_field_name = 'regexp_attr'
    WHERE table_field_name = 'case_sensitive';

-- ...

CREATE OR REPLACE FUNCTION service_value2text(val text, typeid bigint, raw boolean DEFAULT false) RETURNS text AS $$
DECLARE
    dim text;
BEGIN
    IF raw THEN
        SELECT param_type_dim INTO dim
            FROM service_var_types AS vt
            JOIN param_types       AS pt ON pt.param_type_id = vt.raw_param_type_id
            WHERE vt.service_var_type_id = typeid;
    ELSE
        SELECT param_type_dim INTO dim
            FROM service_var_types AS vt
            JOIN param_types       AS pt ON pt.param_type_id = vt.param_type_id
            WHERE vt.service_var_type_id = typeid;
    END IF;
    IF dim IS NULL OR dim = '' THEN
        dim := '';
    ELSE
        dim := ' ' || dim;
    END IF;
    RETURN val || dim;
END;
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION service_value2text(val text, typeid bigint, raw boolean) IS
    'Segéd függvény a service_vars értékeinek megjelenítéséhez mértékegységgel.';

-- ------------------------------------------------------------------------------------------------

SELECT set_db_version(1, 24);
