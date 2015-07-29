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
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    user_id         bigint      NOT NULL
        REFERENCES users(user_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    node_id         bigint	NOT NULL
        REFERENCES nodes(node_id) MATCH FULL ON UPDATE RESTRICT ON DELETE CASCADE,
    app_name        varchar(32) DEFAULT NULL,
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

CREATE TYPE templatetype AS ENUM ('macros', 'patchs', 'nodes');

CREATE TABLE import_templates (
    import_template_id	bigserial       PRIMARY KEY,
    template_name	varchar(32)     NOT NULL,
    template_type	templatetype	NOT NULL,
    template_note	text	DEFAULT NULL,
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