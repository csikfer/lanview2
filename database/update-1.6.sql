
CREATE TABLE languages (
    language_id     serial      NOT NULL PRIMARY KEY,
    language_name   text        NOT NULL UNIQUE,
    lang_id         integer     NOT NULL,
    country_id      integer     NOT NULL,
    country_a2      varchar(2)  NOT NULL,
    lang_2          varchar(2)  NOT NULL,
    lang_3          varchar(3)  NOT NULL,
    flag_image      bigint      DEFAULT NULL
        REFERENCES images(image_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    next_id         integer     DEFAULT NULL
        REFERENCES languages(language_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    UNIQUE (lang_id, country_id),
    UNIQUE (country_a2, lang_2),
    UNIQUE (country_a2, lang_3)
);

COMMENT ON COLUMN languages.lang_id     IS 'Qt enum QLocale::Language';
COMMENT ON COLUMN languages.country_id  IS 'Qt enum QLocale::Country';
COMMENT ON COLUMN languages.country_a2  IS 'ISO Alpha-2';
COMMENT ON COLUMN languages.lang_2      IS 'ISO 639-1';
COMMENT ON COLUMN languages.lang_3      IS 'ISO 639-5';

CREATE OR REPLACE FUNCTIOn language_id2code(lid bigint) RETURNS text AS $$
DECLARE
    n text;
BEGIN
    SELECT lang_2 || '_' || country_a2 INTO n FROM languages WHERE language_id = lid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', lid, 'language_id', 'language_id2name()', 'languages');
    END IF;
    RETURN n;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION names2language_id(c varchar(2), l varchar(2)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT language_id INTO id FROM languages WHERE l = lang_2 AND c = country_a2;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, c || '_' || l, 'names2language_id()', 'languages');
    END IF;
    RETURN id;
END;
$$ LANGUAGE plpgsql;

INSERT INTO
    languages (language_name, lang_id, country_id, country_a2, lang_2, lang_3)
    VALUES    (     'Magyar',     50,          98,       'HU',   'hu',  'hun'),
              ( 'US English',     31,         225,       'US',   'en',  'eng'),
              (    'Deutsch',     42,          82,       'DE',   'de',  'deu');

INSERT INTO
    languages (language_name, lang_id, country_id, country_a2, lang_2, lang_3, next_id)
    VALUES    ( 'UK English',     31,         224,       'GB',   'en',  'eng', names2language_id('US', 'en'));
                 
SELECT set_int_sys_param('default_language',  names2language_id('US', 'en'));
SELECT set_int_sys_param('failower_language', names2language_id('HU', 'hu'));

CREATE TYPE tablefortext AS ENUM ('alarm_messages', 'errors', 'enum_vals', 'menu_items', 'table_shapes', 'table_shape_fields');
CREATE SEQUENCE text_id_sequ;
CREATE TABLE localizations (
    text_id         bigint  NOT NULL,
    table_for_text  tablefortext NOT NULL,
    language_id     integer NOT NULL
        REFERENCES languages(language_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE CASCADE,
    texts           text[]  NOT NULL,
    PRIMARY KEY (text_id, table_for_text, language_id)
);
CREATE INDEX localization_texts_text_id_index       ON localizations(text_id, table_for_text);

CREATE OR REPLACE FUNCTION check_after_localization_text() RETURNS TRIGGER AS $$
DECLARE
    n integer;
BEGIN
    IF TG_OP = 'UPDATE' THEN
        IF NEW.text_id <> OLD.text_id THEN
            PERFORM error('Constant', OLD.text_id, NEW.text_id::text, 'text_id', TG_TABLE_NAME, TG_OP);
        END IF;
        IF NEW.table_for_text <> OLD.table_for_text THEN
            PERFORM error('Constant', NEW.text_id, OLD.table_for_text || ' - ' || NEW.table_for_text, 'table_for_text', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    EXECUTE 'SELECT count(*) FROM ' || NEW.table_for_text || ' WHERE text_id = $1'
        INTO n
        USING NEW.text_id;
    IF n <> 1  THEN
        IF n > 1 THEN
            PERFORM error('IdNotUni', NEW.text_id, NEW.text_type, 'text_type', TG_TABLE_NAME, TG_OP);
        ELSE
            PERFORM error('IdNotFound', NEW.text_id, NEW.text_type, 'text_type', TG_TABLE_NAME, TG_OP);
        END IF;
        RETURN NULL;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER check_after_localization_text_trigger
AFTER INSERT OR UPDATE ON localizations
    FOR EACH ROW EXECUTE PROCEDURE check_after_localization_text();


CREATE OR REPLACE FUNCTION set_language(id integer) RETURNS integer AS $$
BEGIN
    PERFORM set_config('lanview2.language_id', id::text, false);
    RETURN id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION set_language(l varchar(2), c varchar(2) DEFAULT NULL) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    IF c IS NULL THEN
        id := language_id FROM languages WHERE l = lang_2 LIMIT 1;
    ELSE 
        id := COALESCE(
            (SELECT language_id FROM languages WHERE l = lang_2 AND c = country_a2),
            (SELECT language_id FROM languages WHERE l = lang_2 LIMIT 1),
            (SELECT language_id FROM languages WHERE c = country_a2 LIMIT 1));
    END IF;
    IF id IS NULL THEN
        id := COALESCE(get_int_sys_param('default_language'), get_int_sys_param('failower_language'));
    END IF;
    PERFORM set_config('lanview2.language_id', id::text, false);
    RETURN id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_language_id() RETURNS integer AS $$
BEGIN
    RETURN CAST(current_setting('lanview2.language_id') AS integer);
    EXCEPTION
        WHEN OTHERS THEN
            RETURN COALESCE(get_int_sys_param('default_language'), get_int_sys_param('failower_language'));
    RETURN id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION localization_texts(tid bigint, tft tablefortext) RETURNS localizations AS $$
DECLARE
    r   localizations;
    lid integer;
    lidd integer;
    lidf integer;
BEGIN
    lid := get_language_id();	-- actual
    SELECT * INTO r FROM localizations WHERE text_id = tid AND language_id = lid;
    IF FOUND THEN
        RETURN r;
    END IF;
    lidd := get_int_sys_param('default_language');	-- default
    IF lid <> lidd THEN
        SELECT * INTO r FROM localizations WHERE text_id = tid AND table_for_text = tft AND language_id = lidd;
        IF FOUND THEN
            RETURN r;
        END IF;
    END IF;
    lidf := get_int_sys_param('failower_language');	-- failower
    IF lid <> lidf AND lidd <> lidf THEN
        SELECT * INTO r FROM localizations WHERE text_id = tid AND table_for_text = tft AND language_id = lidf;
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
    r.language_id := lid;
    -- r.texts := ARRAY['Unknown text id ' || tft || '#' || tid::text];
    return r;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_record_text() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM localizations WHERE text_id = OLD.text_id AND table_for_text = TG_TABLE_NAME;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

-- alarm_messages
ALTER TABLE alarm_messages ADD COLUMN text_id bigint NOT NULL DEFAULT nextval('text_id_sequ');
INSERT INTO localizations SELECT text_id, 'alarm_messages', names2language_id('HU', 'hu'), ARRAY[message, short_msg] FROM alarm_messages;
ALTER TABLE alarm_messages DROP COLUMN message;
ALTER TABLE alarm_messages DROP COLUMN short_msg;
CREATE TRIGGER alarm_messages_delete_record_text AFTER DELETE ON errors FOR EACH ROW EXECUTE PROCEDURE delete_record_text();
CREATE TYPE alarmmessagetext AS ENUM ('message', 'short_msg');
COMMENT ON TYPE alarmmessagetext IS
    'In table "error_messages", because of the localization of languages, the names of text fields stored in the "localizations" table.';

CREATE OR REPLACE FUNCTION alarm_message(sid bigint, st notifswitch) RETURNS text AS $$
DECLARE
    msg         text;
    hs          host_services;
    stid        bigint;
    s           services;
    n           nodes;
    pl          places;
    lid		integer;
    tid         bigint DEFAULT NULL;         -- text id
BEGIN
    SELECT * INTO hs FROM host_services WHERE host_service_id = sid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', sid, 'host_service_id', 'alarm_message()', 'host_services');
    END IF;
    
    SELECT * INTO s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'alarm_message()', 'services');
    END IF;
    
    SELECT text_id INTO tid FROM alarm_messages WHERE service_type_id = s.service_type_id AND status = st;
    IF NOT FOUND THEN
        SELECT text_id INTO tid FROM alarm_messages WHERE service_type_id = -1 AND status = st; -- service_type = unmarked
    END IF;
    -- RAISE INFO 'text_id = %', tid;
    IF tid IS NOT NULL THEN
	lid = get_language_id();
	-- RAISE INFO 'Get text, lid = %', lid;
        SELECT texts[1] INTO msg FROM localizations WHERE text_id = tid AND table_for_text = 'alarm_messages' AND language_id = lid;
        IF NOT FOUND THEN
            -- RAISE INFO 'localization (%) text (%) not found', lid, tid;
            msg := COALESCE(
                (SELECT texts[1] FROM localizations WHERE text_id = tid AND table_for_text = 'alarm_messages' AND language_id = get_int_sys_param('default_language')),
                (SELECT texts[1] FROM localizations WHERE text_id = tid AND table_for_text = 'alarm_messages' AND language_id = get_int_sys_param('failower_language')));
        END IF;
    END IF;
    IF msg IS NULL THEN
	-- RAISE INFO 'localization text not found, set default';
        msg := '$hs.name is $st';
    END IF;

    msg := replace(msg, '$st', st::text);
    IF msg LIKE '%$hs.%' THEN 
        IF msg LIKE '%$hs.name%' THEN
            msg := replace(msg, '$hs.name',  host_service_id2name(hs.host_service_id));
        END IF;
        msg := replace(msg, '$hs.note',  COALESCE(hs.host_service_note, ''));
    END IF;

    IF msg LIKE '%$s.%' THEN 
        msg := replace(msg, '$s.name',   s.service_name);
        msg := replace(msg, '$s.note',   COALESCE(s.service_note, ''));
    END IF;

    IF msg LIKE '%$n.%' OR msg LIKE '%$pl.%' THEN
        SELECT * INTO n FROM nodes WHERE node_id = hs.node_id;
        IF NOT FOUND THEN
            PERFORM error('IdNotFound', hs.node_id, 'node_id', 'alarm_message()', 'nodes');
        END IF;
        IF msg LIKE '%$n.%' THEN
            msg := replace(msg, '$n.name',     n.node_name);
            msg := replace(msg, '$n.note',     COALESCE(n.node_note, ''));
        END IF;
        IF msg LIKE '%$pl.%' THEN
            SELECT * INTO pl FROM placess WHERE place_id = n.place_id;
            IF NOT FOUND THEN
                PERFORM error('IdNotFound', n.place_id, 'place_id', 'alarm_message()', 'places');
            END IF;
            msg := replace(msg, '$pl.name',     pl.place_name);
            msg := replace(msg, '$pl.note',     COALESCE(pl.place_note, ''));
        END IF;
    END IF;
    RETURN msg;
END
$$ LANGUAGE plpgsql;

--- errors
ALTER TABLE errors ADD COLUMN text_id bigint NOT NULL DEFAULT nextval('text_id_sequ');
INSERT INTO localizations SELECT text_id, 'errors', names2language_id('HU', 'hu'), ARRAY[error_note, error_name] FROM errors;
CREATE TRIGGER errors_delete_record_text AFTER DELETE ON errors FOR EACH ROW EXECUTE PROCEDURE delete_record_text();
CREATE TYPE errortext AS ENUM ('error_title');
COMMENT ON TYPE errortext IS
    'In table "errors", because of the localization of languages, the names of text fields stored in the "localizations" table.';

--- enums
ALTER TABLE enum_vals ADD COLUMN text_id bigint NOT NULL DEFAULT nextval('text_id_sequ');
INSERT INTO localizations SELECT text_id, 'enum_vals', names2language_id('HU', 'hu'), ARRAY[view_long, view_short, tool_tip] FROM enum_vals;
ALTER TABLE enum_vals DROP COLUMN view_long;
ALTER TABLE enum_vals DROP COLUMN view_short;
ALTER TABLE enum_vals DROP COLUMN tool_tip;
CREATE TRIGGER enum_vals_delete_record_text AFTER DELETE ON enum_vals FOR EACH ROW EXECUTE PROCEDURE delete_record_text();
CREATE TYPE enumvaltext AS ENUM ('view_long', 'view_short', 'tool_tip');
COMMENT ON TYPE enumvaltext IS
    'In table "enum_vals", because of the localization of languages, the names of text fields stored in the "localizations" table.';

--- menu_items
ALTER TABLE menu_items ADD COLUMN text_id bigint NOT NULL DEFAULT nextval('text_id_sequ');
INSERT INTO localizations SELECT text_id, 'menu_items', names2language_id('HU', 'hu'), ARRAY[menu_title, tab_title, tool_tip, whats_this] FROM menu_items;
ALTER TABLE menu_items DROP COLUMN menu_title;
ALTER TABLE menu_items DROP COLUMN tab_title;
ALTER TABLE menu_items DROP COLUMN tool_tip;
ALTER TABLE menu_items DROP COLUMN whats_this;
CREATE TRIGGER menu_items_delete_record_text AFTER DELETE ON menu_items FOR EACH ROW EXECUTE PROCEDURE delete_record_text();
CREATE TYPE menuitemtext AS ENUM ('menu_title', 'tab_title', 'tool_tip', 'whats_this');
COMMENT ON TYPE menuitemtext IS
    'In table "menu_items", because of the localization of languages, the names of text fields stored in the "localizations" table.';
--- table_shapes
ALTER TABLE table_shapes ADD COLUMN text_id bigint NOT NULL DEFAULT nextval('text_id_sequ');
INSERT INTO localizations
    SELECT text_id, 'table_shapes', names2language_id('HU', 'hu'), ARRAY[table_title, dialog_title, dialog_tab_title, member_title, not_member_title]
        FROM table_shapes;
ALTER TABLE table_shapes DROP COLUMN table_title;
ALTER TABLE table_shapes DROP COLUMN dialog_title;
ALTER TABLE table_shapes DROP COLUMN dialog_tab_title;
ALTER TABLE table_shapes DROP COLUMN member_title;
ALTER TABLE table_shapes DROP COLUMN not_member_title;
CREATE TRIGGER table_shapes_delete_record_text AFTER DELETE ON table_shapes FOR EACH ROW EXECUTE PROCEDURE delete_record_text();
CREATE TYPE tableshapetext AS ENUM ('table_title', 'dialog_title', 'dialog_tab_title', 'member_title', 'not_member_title');
COMMENT ON TYPE tableshapetext IS
    'In table "table_shapes", because of the localization of languages, the names of text fields stored in the "localizations" table.';

--- table_shape_fields
ALTER TABLE table_shape_fields ADD COLUMN text_id bigint NOT NULL DEFAULT nextval('text_id_sequ');
INSERT INTO localizations
    SELECT text_id, 'table_shape_fields', names2language_id('HU', 'hu'), ARRAY[table_title, dialog_title, tool_tip, whats_this]
        FROM table_shape_fields;
ALTER TABLE table_shape_fields DROP COLUMN table_title;
ALTER TABLE table_shape_fields DROP COLUMN dialog_title;
ALTER TABLE table_shape_fields DROP COLUMN tool_tip;
ALTER TABLE table_shape_fields DROP COLUMN whats_this;
CREATE TRIGGER table_shape_fields_delete_record_text AFTER DELETE ON table_shape_fields FOR EACH ROW EXECUTE PROCEDURE delete_record_text();
CREATE TYPE tableshapefieldtext AS ENUM ('table_title', 'dialog_title', 'tool_tip', 'whats_this');
COMMENT ON TYPE tableshapefieldtext IS
    'In table "table_shape_fields", because of the localization of languages, the names of text fields stored in the "localizations" table.';

ALTER TYPE unusualfkeytype ADD VALUE 'text';
COMMENT ON TYPE public.unusualfkeytype IS
'SQL-supported and non-supported foreign keys types:
"property"    A foreign key pointing to a property
"self"        A foreign key pointing to the same type of parent (tree)
"owner"       The foreign key to the owner
"text"        Text ID';

COMMENT ON TABLE fkey_types IS 'The table defining the type of foreign keys (for non property ties)';
COMMENT ON COLUMN fkey_types.table_schema IS 'The name of the foreign key scheme';
COMMENT ON COLUMN fkey_types.table_name  IS 'The name of the foreign key table';
COMMENT ON COLUMN fkey_types.column_name IS 'The name of the foreign key, or enum type name if type is text';
COMMENT ON COLUMN fkey_types.unusual_fkeys_type IS 'The remote key type';

INSERT INTO fkey_types
  ( table_name,             column_name,            unusual_fkeys_type) VALUES
  ( 'alarm_messages',       'alarmmessagetext',     'text'            ),
  ( 'errors',               'errortext',            'text'            ),
  ( 'enum_vals',            'enumvaltext',          'text'            ),
  ( 'menu_items',           'menuitemtext',         'text'            ),
  ( 'table_shapes',         'tableshapetext',       'text'            ),
  ( 'table_shape_fields',   'tableshapefieldtext',  'text'            );

ALTER TYPE datacharacter add VALUE 'text';

SELECT set_db_version(1, 6);
