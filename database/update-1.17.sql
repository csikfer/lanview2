-- table_shape_field_name is not unique !!

ALTER TABLE table_shape_fields ADD COLUMN table_field_name text;
UPDATE table_shape_fields SET table_field_name = table_shape_field_name;
ALTER TABLE table_shape_fields ALTER COLUMN table_field_name SET NOT NULL;

CREATE OR REPLACE FUNCTION check_shape_field() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.table_field_name IS NULL THEN
	NEW.table_field_name = NEW.table_shape_field_name;
    END IF;
    RETURN NEW;
END;
$$ language plpgsql;

CREATE TRIGGER table_shape_fields_before_insert
  BEFORE INSERT
  ON table_shape_fields
  FOR EACH ROW
  EXECUTE PROCEDURE check_shape_field();

-- check duplicate, and correction !!!
-- SELECT table_shape_id2name(f1.table_shape_id) || ':' || f1.table_shape_field_name
--     FROM table_shape_fields AS f1
--     JOIN table_shape_fields AS f2 ON f1.table_shape_id = f2.table_shape_id AND f1.table_shape_field_name = f2.table_shape_field_name
--     GROUP BY f1.table_shape_id, f1.table_shape_field_name
--     HAVING count(*) > 1;

DROP INDEX IF EXISTS table_shape_fields_shape_index;
ALTER TABLE table_shape_fields ADD CONSTRAINT table_shape_fields_name_unique_index UNIQUE (table_shape_id, table_shape_field_name);

COMMENT ON COLUMN public.table_shape_fields.table_shape_field_name IS 'Column name.';
COMMENT ON COLUMN public.table_shape_fields.table_field_name IS 'Table field name (using of query).';

-- 2018.11.23.

ALTER TYPE datacharacter ADD VALUE 'question';

SELECT set_db_version(1, 17);