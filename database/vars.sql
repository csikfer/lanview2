DROP TABLE IF EXISTS rrd_beats CASCADE;
DROP TABLE IF EXISTS service_var_types CASCADE;
DROP TABLE IF EXISTS service_vars CASCADE;
DROP TABLE IF EXISTS graphs CASCADE;
DROP TABLE IF EXISTS graph_vars CASCADE;
DROP TYPE IF EXISTS aggregatetype;
DROP TYPE IF EXISTS servicevartype;
DROP TYPE IF EXISTS Drawtype;

CREATE TYPE aggregatetype AS ENUM ('AVERAGE', 'MIN', 'MAX', 'LAST');
ALTER TYPE aggregatetype OWNER TO lanview2;

CREATE TYPE servicevartype AS ENUM ('GAUGE','COUNTER', 'DCOUNTER', 'DERIVE', 'DDERIVE', 'ABSOLUTE', 'COMPUTE');
ALTER TYPE servicevartype OWNER TO lanview2;

CREATE TYPE drawtype AS ENUM ('LINE', 'AREA', 'STACK', 'NONE');
ALTER TYPE drawtype OWNER TO lanview2;

CREATE TABLE rrd_beats (
    rrd_beat_id         bigserial           PRIMARY KEY,
    rrd_beat_name       text                NOT NULL UNIQUE,
    rrd_beat_note       text                DEFAULT NULL,
    step                interval            NOT NULL CHECK (extract(epoch from step) > 10),
    heartbeat           interval            NOT NULL,
    daily_step          integer             DEFAULT NULL CHECK (daily_step > 0),
    daily_size          integer             DEFAULT NULL CHECK (daily_size > 0),
    daily_aggregates    aggregatetype[]     NOT NULL DEFAULT ARRAY[]::aggregatetype[],
    weekly_step         integer             DEFAULT NULL CHECK (weekly_step > 0),
    weekly_size         integer             DEFAULT NULL CHECK (weekly_size > 0),
    weekly_aggregates   aggregatetype[]     NOT NULL DEFAULT ARRAY[]::aggregatetype[],
    monthly_step        integer             DEFAULT NULL CHECK (monthly_step > 0),
    monthly_size        integer             DEFAULT NULL CHECK (monthly_size > 0),
    monthly_aggregates  aggregatetype[]     NOT NULL DEFAULT ARRAY[]::aggregatetype[],
    yearly_step         integer             DEFAULT NULL CHECK (yearly_step > 0),
    yearly_size         integer             DEFAULT NULL CHECK (yearly_size > 0),
    yearly_aggregates   aggregatetype[]     NOT NULL DEFAULT ARRAY[]::aggregatetype[],
    features            text                DEFAULT NULL,
    deleted             boolean             NOT NULL DEFAULT false,
    CHECK (heartbeat > step),
    CHECK ((daily_aggregates   = ARRAY[]::aggregatetype[]) = (daily_step   IS NULL)),
    CHECK ((weekly_aggregates  = ARRAY[]::aggregatetype[]) = (weekly_step  IS NULL)),
    CHECK ((monthly_aggregates = ARRAY[]::aggregatetype[]) = (monthly_step IS NULL)),
    CHECK ((yearly_aggregates  = ARRAY[]::aggregatetype[]) = (yearly_step  IS NULL))
);

INSERT INTO rrd_beats
    (rrd_beat_id, rrd_beat_name, rrd_beat_note, step, heartbeat,
        daily_step, daily_size, daily_aggregates,
        weekly_step, weekly_size, weekly_aggregates,
        monthly_step, monthly_size, monthly_aggregates,
        yearly_step, yearly_size, yearly_aggregates) VALUES
    (0, 'std5min', 'Normál 5 percenkénti lekérdezés', '5:00', '10:00',
        1, 400, '{AVERAGE}',            -- ~33.3 hour/  5 min.
        6, 400, '{AVERAGE, MIN, MAX}',  -- ~ 8.5 day / 30 min.
       24, 400, '{AVERAGE, MIN, MAX}',  -- ~33.3 day /  2 hour
      288, 400, '{AVERAGE, MIN, MAX}'), -- 400   day /  1 day
    (1, 'std1min', 'Normál 1 percenkénti lekérdezés', '1:00',  '5:00',
        5, 400, '{AVERAGE, MIN, MAX}',  -- ~33.3 hour/  5 min.
       30, 400, '{AVERAGE, MIN, MAX}',  -- ~ 8.5 day / 30 min.
      120, 400, '{AVERAGE, MIN, MAX}',  -- ~33.3 day /  2 hour
     1440, 400, '{AVERAGE, MIN, MAX}'); -- 400   day /  1 day
SELECT nextval('rrd_beats_rrd_beat_id_seq');

CREATE TABLE service_var_types (
    service_var_type_id     bigserial           PRIMARY KEY,
    service_var_type_name   text                NOT NULL UNIQUE,
    service_var_type_note   text                DEFAULT NULL,
    param_type_id           bigint              NOT NULL
        REFERENCES param_types(param_type_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    service_var_type        servicevartype      DEFAULT NULL,
    plausibility_type       filtertype          DEFAULT NULL,
    plausibility_param1     text                DEFAULT NULL,
    plausibility_param2     text                DEFAULT NULL,
    warning_type            filtertype          DEFAULT NULL,
    warning_param1          text                DEFAULT NULL,
    warning_param2          text                DEFAULT NULL,
    critical_type          filtertype          DEFAULT NULL,
    critical_param1         text                DEFAULT NULL,
    critical_param2         text                DEFAULT NULL,
    features                text                DEFAULT NULL,
    deleted                 boolean             NOT NULL DEFAULT FALSE
);
ALTER TABLE service_var_types OWNER TO lanview2;

CREATE TABLE service_vars (
    service_var_id          bigserial           PRIMARY KEY,
    service_var_name        text                NOT NULL,
    service_var_note        text                DEFAULT NULL,
    service_var_type_id     bigint              NOT NULL
        REFERENCES service_var_types(service_var_type_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    host_service_id         bigint              NOT NULL
        REFERENCES host_services(host_service_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    rrd_beat_id             bigint              DEFAULT NULL,
    service_var_value       text                DEFAULT NULL,
    var_state               notifswitch         DEFAULT 'unknown',
    delegate_service_state  boolean             DEFAULT FALSE,
    last_time               timestamp           DEFAULT NULL,
    raw_value               text                DEFAULT NULL,
    features                text                DEFAULT NULL,
    deleted                 boolean             NOT NULL DEFAULT FALSE,
    UNIQUE (service_var_name, host_service_id)
);
ALTER TABLE service_vars OWNER TO lanview2;

INSERT INTO fkey_types
  ( table_name,             column_name,            unusual_fkeys_type) VALUES
  ( 'service_vars',        'host_service_id',      'owner'         );

CREATE TABLE graphs (
    graph_id                bigserial           PRIMARY KEY,
    graph_name              text                UNIQUE,
    graph_note              text                DEFAULT NULL,
    graph_title             text                DEFAULT NULL,
    rrd_beat_id             bigint              NOT NULL
        REFERENCES rrd_beats(rrd_beat_id) MATCH FULL ON DELETE RESTRICT ON UPDATE RESTRICT,
    -- ...
    features                text                DEFAULT NULL,
    deleted                 boolean             NOT NULL DEFAULT FALSE    
);
ALTER TABLE graphs OWNER TO lanview2;

CREATE TABLE graph_vars (
    graph_var_id            bigserial           PRIMARY KEY,
    graph_var_note          text                DEFAULT NULL,
    graph_id                bigint              NOT NULL
        REFERENCES graphs(graph_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    service_var_id          bigint              NOT NULL
        REFERENCES service_vars(service_var_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    draw_type               drawtype            NOT NULL DEFAULT 'LINE',
    sequence_number         integer             NOT NULL,
    -- ...
    features                text                DEFAULT NULL,
    deleted                 boolean             NOT NULL DEFAULT FALSE    
);
ALTER TABLE graph_vars OWNER TO lanview2;
