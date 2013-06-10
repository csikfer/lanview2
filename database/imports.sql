CREATE TYPE execstate AS ENUM ('wait', 'execute', 'ok', 'faile', 'aborted');
ALTER TYPE  execstate OWNER TO lanview2;

CREATE TABLE imports (
    import_id       serial      PRIMARY KEY,
    date_of         timestamp   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    auth_user_id    integer     DEFAULT NULL,
    import_text     text        NOT NULL,
    exec_state      execstate   NOT NULL DEFAULT 'wait',
    pid             integer     DEFAULT NULL,
    started         timestamp   DEFAULT NULL,
    ended           timestamp   DEFAULT NULL,
    result_msg      text        DEFAULT NULL,
    applog_id       integer     DEFAULT NULL
        REFERENCES app_errs(applog_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE CASCADE
);
ALTER TABLE imports OWNER TO lanview2;

CREATE TABLE import_templates (
    import_template_id	serial		PRIMARY KEY,
    template_type	varchar(32)	NOT NULL,
    template_name	varchar(32)	NOT NULL,
    template_descr	varchar(255)	DEFAULT NULL,
    template_text	text		NOT NULL,
    UNIQUE (template_type, template_name)
);
ALTER TABLE import_templates OWNER TO lanview2;
