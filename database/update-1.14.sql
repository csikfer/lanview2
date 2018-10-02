
CREATE TYPE menuitemtype AS ENUM ('shape', 'own', 'exec', 'menu');
ALTER TABLE menu_items ADD COLUMN menu_item_type menuitemtype NOT NULL DEFAULT 'menu';
ALTER TABLE menu_items ADD COLUMN menu_param text DEFAULT NULL;
ALTER TABLE menu_items ADD CONSTRAINT check_type_and_param CHECK
    ((menu_item_type = 'menu' AND menu_param IS NULL) OR (menu_item_type <> 'menu' AND menu_param IS NOT NULL));
UPDATE menu_items
  SET menu_item_type = split_part(btrim(features, ':'), '=', 1)::menuitemtype,
      menu_param = split_part(btrim(features, ':'), '=', 2)
  WHERE features <> ':sub:';
UPDATE menu_items
  SET features = NULL
  WHERE features = ':sub:' OR NOT menu_param LIKE '%:%';
UPDATE menu_items
  SET features = ':' || split_part(menu_param, ':', 2) || ':',
      menu_param = split_part(menu_param, ':', 1)
  WHERE menu_param LIKE '%:%';
ALTER TABLE menu_items ADD COLUMN menu_item_note text DEFAULT NULL;

SELECT set_db_version(1, 14);