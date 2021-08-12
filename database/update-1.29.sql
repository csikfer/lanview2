-- Egy plussz mező a rekord dialógok megjelenítéséhez, ha kellenek gyerek objektumok, csoport tagságok
ALTER TABLE table_shapes ADD COLUMN IF NOT EXISTS tables_on_dialog_ids bigint[] DEFAULT NULL;

INSERT INTO unusual_fkeys (table_name, column_name, unusual_fkeys_type, f_table_name, f_column_name)
    VALUES ('table_shapes', 'tables_on_dialog_ids', 'property', 'table_shapes', 'table_shape_id')
    ON CONFLICT DO NOTHING;

CREATE OR REPLACE FUNCTION public.check_table_shape() RETURNS trigger LANGUAGE 'plpgsql' AS $$
DECLARE
    tsid  bigint;
BEGIN
    IF NEW.right_shape_ids IS NULL THEN
        
    ELSIF array_length(NEW.right_shape_ids,1) = 0 THEN
        NEW.right_shape_ids = NULL;
    ELSE
        FOREACH tsid  IN ARRAY NEW.right_shape_ids LOOP
            IF 1 <> COUNT(*) FROM table_shapes WHERE table_shape_id = tsid THEN
                PERFORM error('IdNotFound', tsid, NEW.table_shape_name, 'check_table_shape()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
        END LOOP;
    END IF;
    IF NEW.tables_on_dialog_ids IS NULL THEN
        
    ELSIF array_length(NEW.tables_on_dialog_ids,1) = 0 THEN
        NEW.right_shape_ids = NULL;
    ELSE
        FOREACH tsid  IN ARRAY NEW.tables_on_dialog_ids LOOP
            IF 1 <> COUNT(*) FROM table_shapes WHERE table_shape_id = tsid THEN
                PERFORM error('IdNotFound', tsid, NEW.table_shape_name, 'check_table_shape()', TG_TABLE_NAME, TG_OP);
                RETURN NULL;
            END IF;
        END LOOP;
    END IF;
    RETURN NEW;
END;
$$;

-- -------------------------------

CREATE OR REPLACE FUNCTION is_node_in_zone(idn bigint, idq bigint) RETURNS boolean
    LANGUAGE 'plpgsql' AS $$
DECLARE
    idr bigint;
    n integer;
BEGIN
    CASE idq
        WHEN 0 THEN -- none
            RETURN FALSE;
        WHEN 1 THEN -- all
            RETURN TRUE;
        ELSE
            SELECT place_id INTO idr FROM patchs WHERE node_id = idn;
            SELECT COUNT(*) INTO n FROM place_group_places WHERE place_group_id = idq AND (place_id = idr OR is_parent_place(idr, place_id));
            RETURN n > 0;
    END CASE;
END
$$;

CREATE OR REPLACE FUNCTION is_host_service_in_zone(ids bigint, idq bigint) RETURNS boolean
    LANGUAGE 'plpgsql' AS $$
DECLARE
    idr bigint;
    n integer;
BEGIN
    CASE idq
        WHEN 0 THEN -- none
            RETURN FALSE;
        WHEN 1 THEN -- all
            RETURN TRUE;
        ELSE
            SELECT place_id INTO idr FROM host_services JOIN nodes USING(node_id)
                WHERE host_service_id = ids;
            SELECT COUNT(*) INTO n FROM place_group_places WHERE place_group_id = idq AND (place_id = idr OR is_parent_place(idr, place_id));
            RETURN n > 0;
    END CASE;
END
$$;

-- -------------------------------
-- 2021.08.09

DROP TABLE IF EXISTS place_group_params;
CREATE TABLE IF NOT EXISTS place_group_params
(
    place_group_param_id    bigserial   NOT NULL PRIMARY KEY,
    place_group_param_name  text        NOT NULL,
    place_group_param_note  text,
    place_group_id          bigint      NOT NULL
        REFERENCES place_groups (place_group_id) MATCH FULL
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    param_type_id           bigint      NOT NULL
        REFERENCES public.param_types (param_type_id) MATCH FULL
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    param_value             text,
    flag                    boolean     DEFAULT false,
    date_of                 timestamp   DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (place_group_param_name, place_group_param_id)
);

CREATE TRIGGER place_group_param_check_value
    BEFORE INSERT OR UPDATE 
    ON public.place_group_params
    FOR EACH ROW
    EXECUTE FUNCTION public.check_before_param_value();

CREATE OR REPLACE FUNCTION place_group_param(pgid bigint, pname text) RETURNS text LANGUAGE 'plpgsql' AS $$
BEGIN
    RETURN param_value FROM place_group_params WHERE place_group_id = pgid AND place_group_param_name = pname;
END;
$$;

DROP TABLE IF EXISTS place_params;
CREATE TABLE IF NOT EXISTS place_params
(
    place_param_id    bigserial   NOT NULL PRIMARY KEY,
    place_param_name  text        NOT NULL,
    place_param_note  text,
    place_id          bigint      NOT NULL
        REFERENCES places (place_id) MATCH FULL
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    param_type_id           bigint      NOT NULL
        REFERENCES public.param_types (param_type_id) MATCH FULL
        ON UPDATE RESTRICT ON DELETE RESTRICT,
    param_value             text,
    flag                    boolean     DEFAULT false,
    date_of                 timestamp   DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (place_param_name, place_param_id)
);

CREATE TRIGGER place_param_check_value
    BEFORE INSERT OR UPDATE 
    ON public.place_params
    FOR EACH ROW
    EXECUTE FUNCTION public.check_before_param_value();

CREATE OR REPLACE FUNCTION place_param(pid bigint, pname text) RETURNS text LANGUAGE 'plpgsql' AS $$
DECLARE
    val     text;
BEGIN
    SELECT param_value INTO val FROM place_params WHERE place_id = pid AND place_param_name = pname;
    IF NOT FOUND THEN
        SELECT param_value INTO val
            FROM place_group_params
            JOIN place_group_places USING (place_group_id)
            JOIN place_groups       USING (place_group_id)
            WHERE place_id = pid AND place_group_param_name = pname AND place_group_type = 'category'::placegrouptype
            LIMIT 1;
    END IF;
    RETURN val;
END;
$$;

-- -------------------------------
SELECT set_db_version(1, 29);
