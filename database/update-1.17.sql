-- table_shape_field_name is not unique !!

ALTER TABLE table_shape_fields ADD COLUMN table_field_name text;
UPDATE table_shape_fields SET table_field_name = table_shape_field_name;
ALTER TABLE table_shape_fields ALTER COLUMN table_field_name SET NOT NULL;

CREATE OR REPLACE FUNCTION check_shape_field() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.table_field_name IS NULL THEN
	NEW.table_field_name := NEW.table_shape_field_name;
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

-- 2018.11.26.

ALTER TYPE fieldflag ADD VALUE 'image';

-- 2018.11.26.

ALTER TYPE nodetype ADD VALUE 'unix_like';
ALTER TYPE nodetype ADD VALUE 'thin_client';
ALTER TYPE nodetype ADD VALUE 'display';

-- 2018.12.02. Hibajavítás (Nem vette figyelembe a languages.next_id mezőt.)

CREATE OR REPLACE FUNCTION localization_texts(tid bigint, tft tablefortext) RETURNS localizations AS $$
DECLARE
    r   localizations;
    lid integer;
    lids integer[];
BEGIN
    lid := get_language_id();	-- actual
    SELECT * INTO r FROM localizations WHERE text_id = tid AND language_id = lid;
    IF FOUND THEN
        RETURN r;
    END IF;
    lids := ARRAY[lid];
    SELECT next_id INTO lid FROM languages WHERE language_id = lid;    -- next
    IF lid IS NOT NULL THEN
        SELECT * INTO r FROM localizations WHERE text_id = tid AND table_for_text = tft AND language_id = lid;
        IF FOUND THEN
            RETURN r;
        END IF;
    END IF;
    lids := lids || lid;
    SELECT language_id INTO lid FROM languages WHERE language_name = get_text_sys_param('default_language');	-- default
    IF lid IS NOT NULL AND NOT lid = ANY (lids) THEN
        SELECT * INTO r FROM localizations WHERE text_id = tid AND table_for_text = tft AND language_id = lid;
        IF FOUND THEN
            RETURN r;
        END IF;
    END IF;
    lids := lids || lid;
    SELECT language_id INTO lid FROM languages WHERE language_name = get_text_sys_param('failower_language');	-- failower
    IF lid IS NOT NULL AND NOT lid = ANY (lids) THEN
        SELECT * INTO r FROM localizations WHERE text_id = tid AND table_for_text = tft AND language_id = lid;
        IF FOUND THEN
            RETURN r;
        END IF;
    END IF;
    -- any
    SELECT * INTO r FROM localizations WHERE text_id = tid AND table_for_text = tft LIMIT 1;
    IF FOUND THEN
        RETURN r;
    END IF;
    r.text_id := tid;
    r.language_id := lids[1];
    -- r.texts := ARRAY['Unknown text id ' || tft || '#' || tid::text];
    return r;
END;
$$ LANGUAGE plpgsql;

-- --------------------------------------------------------
SELECT set_db_version(1, 17);
