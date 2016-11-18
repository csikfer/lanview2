CREATE TYPE execstate AS ENUM ('wait', 'execute', 'ok', 'faile', 'aborted');
ALTER TYPE  execstate OWNER TO lanview2;

COMMENT ON TYPE execstate IS '
Az importnak küldött üzenet feldolgozásának az állapota:
wait    Feldolgozásra várakozik.
execute Felgolgozás alatt.
ok      Feldolgozva, nem volt hiba.
faile   A feldolgozás sikertelen.
aborted Az import az üzenetet eldobta
';

CREATE TABLE imports (
    import_id       bigserial   PRIMARY KEY,
    target_id       bigint      DEFAULT NULL
        REFERENCES host_services(host_service_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL,
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    user_id         bigint      NOT NULL
        REFERENCES users(user_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    node_id         bigint	NOT NULL
        REFERENCES nodes(node_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    app_name        text DEFAULT NULL,
    import_text     text        NOT NULL,
    exec_state      execstate   NOT NULL DEFAULT 'wait',
    pid             bigint      DEFAULT NULL,
    started         timestamp   DEFAULT NULL,
    ended           timestamp   DEFAULT NULL,
    result_msg      text        DEFAULT NULL,
    applog_id       bigint      DEFAULT NULL
        REFERENCES app_errs(applog_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE CASCADE
);
ALTER TABLE imports OWNER TO lanview2;

COMMENT ON TABLE  imports IS 'Az import számára küldött feldolgozandó források táblája.';
COMMENT ON COLUMN imports.import_id IS 'Egyedi rekord azonosító.';
COMMENT ON COLUMN imports.date_of IS 'A rekord létrehozásának az időpontja.';
COMMENT ON COLUMN imports.user_id IS 'Felhasználói azonosító, aki a feldolgozást kezdeményezte.';
COMMENT ON COLUMN imports.node_id IS 'A munkaállomás azonosítója, ahonnan a feldolgozást kezdeményezték.';
COMMENT ON COLUMN imports.app_name IS 'Az applikáció neve, ahonnan a feldogozást kezdeményezték.';
COMMENT ON COLUMN imports.import_text IS 'A feldolgozandó szöveg.';
COMMENT ON COLUMN imports.exec_state IS 'A feldolgozás állapota.';
COMMENT ON COLUMN imports.pid IS 'A feldolgozást végző import példány PID-je.';
COMMENT ON COLUMN imports.started IS 'A feldolgozás megkezdésének az időpontja.';
COMMENT ON COLUMN imports.ended IS 'A feldolgozás befejezésének az időpontja.';
COMMENT ON COLUMN imports.result_msg IS 'Az import válasza a feldolgozásra (hibaüzenet, megjegyzés, stb.).';
COMMENT ON COLUMN imports.applog_id IS 'Ha feldolgozáskor hiba rekord készült, akkor annak az azonosítója.';

CREATE TYPE templatetype AS ENUM ('macros', 'patchs', 'nodes', 'script');

CREATE TABLE import_templates (
    import_template_id	bigserial       PRIMARY KEY,
    template_name	text            NOT NULL,
    template_type	templatetype	NOT NULL,
    template_note	text	        DEFAULT NULL,
    template_text	text		NOT NULL,
    UNIQUE (template_type, template_name)
);
ALTER TABLE import_templates OWNER TO lanview2;

COMMENT ON TABLE  import_templates IS 'Az importban mentett makrók és egyébb minták táblája.';
COMMENT ON COLUMN import_templates.import_template_id IS 'Egyedi rekord azonosító.';
COMMENT ON COLUMN import_templates.template_type IS 'A template típusa (macros, patchs,...)';
COMMENT ON COLUMN import_templates.template_name IS 'A minta vagy makró neve';
COMMENT ON COLUMN import_templates.template_note IS 'Opcionális megjegyzés';
COMMENT ON COLUMN import_templates.template_text IS 'A makró vagy minta tartalma.';

-- QUERY PARSER --

CREATE TYPE parsertype AS ENUM ('prep', 'parse', 'post');

CREATE TABLE query_parsers (
    query_parser_id             bigserial       PRIMARY KEY,
    query_parser_note           text            DEFAULT NULL,
    service_id                  bigint          NOT NULL REFERENCES services(service_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    parse_type                  parsertype      DEFAULT 'parse',
    item_sequence_number        integer         DEFAULT NULL,
    case_sensitive              boolean         DEFAULT false,
    regular_expression          text            DEFAULT NULL,
    import_expression           text            NOT NULL,
    CONSTRAINT check_expression CHECK ((parse_type = 'parse' AND regular_expression IS NOT NULL) OR (parse_type <> 'parse' AND regular_expression IS NULL))
);
ALTER TABLE query_parsers OWNER TO lanview2;


CREATE TABLE object_syntaxs (
    object_syntax_id            bigserial       PRIMARY KEY,
    object_syntax_name          text            NOT NULL UNIQUE,
    sentence                    text            NOT NULL
);
ALTER TABLE object_syntaxs OWNER TO lanview2;

