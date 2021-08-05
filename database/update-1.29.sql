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

SELECT set_db_version(1, 29);
