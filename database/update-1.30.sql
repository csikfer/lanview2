-- Majd egyszer ...


DROP TABLE IF EXISTS srv_diagram_types, srv_diagram_type_vars, service_diagrams;
DROP TYPE IF EXISTS srvdiagramtypetext, srvdiagramtypevartext;

ALTER TYPE tablefortext ADD VALUE IF NOT EXISTS 'service_diagram_types';
ALTER TYPE tablefortext ADD VALUE IF NOT EXISTS 'srv_diagram_type_vars';
CREATE TYPE srvdiagramtypetext AS ENUM ('title', 'vertical_label');
CREATE TYPE srvdiagramtypevartext AS ENUM ();

CREATE TABLE srv_diagram_types (
    srv_diagram_type_id         bigserial   PRIMARY KEY,
    srv_diagram_type_name       text        NOT NULL,
    srv_diagram_type_note       text        DEFAULT NULL,
    service_id                  bigint      NOT NULL
                REFERENCES services(service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    text_id                     bigint      NOT NULL DEFAULT nextval('text_id_sequ'),
    height                      integer     NOT NULL CHECK height >  50 AND height <= 2160,
    width                       integer     NOT NULL CHECK width  > 100 AND width  <= 2840,
    hrule                       text        NOT NULL DEFAULT '',
    features                    text        DEFAULT NULL,
    UNIQUE (srv_diagram_type_name, service_id)
);

CREATE TABLE srv_diagram_type_vars (
    srv_diagram_type_var_id     bigserial   PRIMARY KEY,
    srv_diagram_type_var_name   text        NOT NULL,
    srv_diagram_type_var_note   text        DEFAULT NULL,
    srv_diagram_type_id         bigint      NOT NULL
                REFERENCES srv_diagram_types(srv_diagram_type_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    service_var_name            text        NOT NULL,
    rpn                         text        DEFAULT NULL,
    features                    text        DEFAULT NULL,
    UNIQUE (srv_diagram_type_id, srv_diagram_type_var_name),
    UNIQUE (srv_diagram_type_id, service_var_name)
};

CREATE TABLE service_diagrams (
    service_diagram_id          bigserial   NOT NULL PRIMARY KEY,
    host_service_id             bigint      NOT NULL
                REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    srv_diagram_type_id         bigint      NOT NULL
                REFERENCES srv_diagram_types(srv_diagram_type_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    features                    text        DEFAULT NULL,
    UNIQUE (host_service_id, srv_diagram_type_id)
);
ALTER TABLE service_diagrams OWNER TO lanview2;
COMMENT ON TBALE service_diagrams IS 'Szolgáltatás lekérdezéshez rendelt diagram leírója, ill kapcsoló rekord';
