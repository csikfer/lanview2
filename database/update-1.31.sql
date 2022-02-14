
-- Javítások 2022.02.07.

ALTER FUNCTION alarm_id2name(bigint)                STABLE PARALLEL SAFE;
ALTER FUNCTION alarm_message(bigint, notifswitch)   STABLE PARALLEL SAFE;
CREATE OR REPLACE FUNCTION app_err_id2name(bigint)  RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    name TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT app_name || ':' || err_name INTO name FROM app_errs WHERE applog_id = $1;
    IF NOT FOUND THEN
        name := '#' || $1::text;
    END IF;
    RETURN name;
END;
$BODY$;
CREATE OR REPLACE FUNCTION db_err_id2name(bigint) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    name TEXT;
BEGIN
    SELECT func_name || ':' || err_name INTO name
        FROM db_errs JOIN errors USING(error_id)
        WHERE dblog_id = $1;
    IF NOT FOUND THEN
        name := '#' || $1::text;
    END IF;
    RETURN name;
END;
$BODY$;
CREATE OR REPLACE FUNCTION enum_val_id2name(bigint) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    name text;
BEGIN
    IF $1 IS NULL THEN
        RETURN NULL;
    END IF;
    SELECT enum_val_name || '::' || enum_type_name  INTO name FROM enum_vals WHERE enum_val_id = $1;
    IF NOT FOUND THEN
        name := '#' || $1::text;
    END IF;
    RETURN name;
    END 
$BODY$;
ALTER FUNCTION cast_to_bigint(text, bigint)         IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_boolean(text, boolean)       IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_cidr(text, cidr)             IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_date(text, date)             IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_datetime(text, timestamp without time zone) IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_double(text, double precision) IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_inet(text, inet)             IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_integer(text, bigint)        IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_interval(text, interval)     IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_mac(text, macaddr)           IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_point(text, point)           IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_real(text, double precision) IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION cast_to_time(text, time)             IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION check_paramtype(text, paramtype)     IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION check_platform(bigint, text, text)   STABLE PARALLEL SAFE;
ALTER FUNCTION check_shared(portshare, portshare)   IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION chk_flapping(bigint, interval, integer) STABLE PARALLEL SAFE;
ALTER FUNCTION compare_db_version(integer,integer)  STABLE PARALLEL SAFE;
ALTER FUNCTION current_mactab_stat(bigint, macaddr, mactabstate[]) STABLE PARALLEL SAFE;
ALTER FUNCTION dow2int(dayofweek)                   IMMUTABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS duplicate_serial_numbers(OUT text, OUT bigint); -- egy hibajavításhoz kellett
DROP FUNCTION IF EXISTS enum_name2note(text, text);                     -- nem használt
DROP FUNCTION IF EXISTS error_by_id(bigint);                            -- nem használt
INSERT INTO errors(error_id, error_name, error_type) VALUES (-1, 'unknown', 'Fatal'::errtype)
    ON CONFLICT DO NOTHING;
INSERT INTO localizations(text_id, table_for_text, language_id, texts)
	VALUES (
        (SELECT text_id FROM errors WHERE error_id = -1),
        'errors'::tablefortext,
        (SELECT language_id FROM languages WHERE language_name = 'US English'),
        '{"Invalid error"}'
    ) ON CONFLICT DO NOTHING;
CREATE OR REPLACE FUNCTION error_by_name(text) RETURNS errors
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    err errors%ROWTYPE;
BEGIN
    SELECT * INTO err FROM errors WHERE error_name = $1;
    IF NOT FOUND THEN
        SELECT * INTO err FROM errors WHERE error_id = -1;
        err.error_note := 'Invalid error_id #' || $1::text;
    END IF;
    RETURN err;
END;
$BODY$;
DROP FUNCTION IF EXISTS error_id2name(bigint);          -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS error_name2id(text);            -- nem használt
DROP FUNCTION IF EXISTS error_name2note(text);          -- nem használt
DROP FUNCTION IF EXISTS first_node_id2name(bigint[]);   -- nem használt
ALTER FUNCTION get_bool_port_param(bigint, text)    STABLE PARALLEL SAFE;
ALTER FUNCTION get_bool_sys_param(text)             STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS  get_image(bigint);             -- rename -> get_image_id_by_place_id
CREATE OR REPLACE FUNCTION get_image_id_by_place_id(pid bigint) RETURNS bigint
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    iid bigint;
BEGIN
    SELECT image_id INTO iid FROM places WHERE place_id = pid;
    IF iid IS NOT NULL THEN
        RETURN iid;
    END IF;
    RETURN get_parent_image(pid);
END
$BODY$;
ALTER FUNCTION get_int_sys_param(text)              STABLE PARALLEL SAFE;
ALTER FUNCTION get_interval_sys_param(pname text)   STABLE PARALLEL SAFE;
ALTER FUNCTION get_parent_image(idr bigint)         STABLE PARALLEL SAFE;
ALTER FUNCTION get_text_node_param(bigint, text)    STABLE PARALLEL SAFE;
ALTER FUNCTION get_text_port_param(bigint, text)    STABLE PARALLEL SAFE;
ALTER FUNCTION get_text_sys_param(text)             STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS group_id2name(bigint);      -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS group_name2id(text);        -- nem használt
ALTER FUNCTION host_service_id2name(bigint)         STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS iftype_id2name(bigint);     -- Automatikusan generált, kód modosítva
CREATE OR REPLACE FUNCTION iftype_name2id(itn text) RETURNS bigint
    LANGUAGE 'plpgsql'STABLE PARALLEL SAFE AS $BODY$
DECLARE
    n bigint;
BEGIN
    BEGIN
        SELECT iftype_id INTO n FROM iftypes WHERE iftype_name = itn;
        IF NOT FOUND THEN
            n := 0;
        END IF;
    END;
    RETURN n;
END;
$BODY$;
DROP FUNCTION IF EXISTS image_id2name(bigint);      -- Automatikusan generált, kód modosítva
ALTER FUNCTION int2dow(integer)                     IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION ip2full_port_name(inet)              STABLE PARALLEL SAFE;
ALTER FUNCTION is_content_arp(macaddr)              STABLE PARALLEL SAFE;
ALTER FUNCTION is_content_oui(macaddr)              STABLE PARALLEL SAFE;
ALTER FUNCTION is_dyn_addr(inet)                    STABLE PARALLEL SAFE;
ALTER FUNCTION is_host_service_in_zone(bigint, bigint) STABLE PARALLEL SAFE;
ALTER FUNCTION is_noalarm(noalarmtype, timestamp without time zone, timestamp without time zone, timestamp without time zone)
        IMMUTABLE PARALLEL SAFE;
ALTER FUNCTION is_node_in_zone(bigint, bigint)      STABLE PARALLEL SAFE;
ALTER FUNCTION is_parent_place(bigint, bigint)      STABLE PARALLEL SAFE;
ALTER FUNCTION is_place_in_zone(bigint, bigint)     STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS language_id2name(bigint);    -- Automatikusan generált, kód modosítva
ALTER FUNCTION link_type(bigint, bigint, linktype)  STABLE PARALLEL SAFE;
ALTER FUNCTION localization_texts(bigint, tablefortext) STABLE PARALLEL SAFE;
ALTER FUNCTION mac2full_port_name(macaddr)          STABLE PARALLEL SAFE;
ALTER FUNCTION mac2node_name(macaddr)               STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS menu_item_id2name(bigint);  -- Automatikusan generált, kód modosítva
ALTER FUNCTION min_shared(portshare, portshare)     IMMUTABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS names2language_id(character varying, character varying);    -- nem használt
ALTER FUNCTION next_dow(dayofweek)                  IMMUTABLE PARALLEL SAFE;
CREATE OR REPLACE FUNCTION node_id2name(bigint) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL UNSAFE AS $BODY$
DECLARE
    name text;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT node_name INTO name FROM patchs WHERE node_id = $1;
    IF NOT FOUND THEN
        name := '#' || $1::text;
    END IF;
    RETURN name;
END
$BODY$;
ALTER FUNCTION node_id2table_name(bigint)           STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS node_name2id(text);         -- nem használt
DROP FUNCTION IF EXISTS param_type_id2name(bigint); -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS place_group_id2name(bigint); -- Automatikusan generált, kód modosítva
ALTER FUNCTION place_group_param(bigint, text)      STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS place_id2name(bigint);      -- Automatikusan generált, kód modosítva
ALTER FUNCTION place_param(bigint, text)            STABLE PARALLEL SAFE;
CREATE OR REPLACE FUNCTION port_id2full_name(bigint) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    name text;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT node_id2name(node_id) || ':' || port_name INTO name FROM nports WHERE port_id = $1;
    IF NOT FOUND THEN
        name := ':#' || $1::text;
    END IF;
    RETURN name;
END
$BODY$;
CREATE OR REPLACE FUNCTION port_id2name(bigint) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    name text;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT port_name INTO name FROM nports WHERE port_id = $1;
    IF NOT FOUND THEN
        name := '#' || $1::text;
    END IF;
    RETURN name;
END
$BODY$;
ALTER FUNCTION port_id2table_name(bigint)           STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS protocol_id2name(bigint);   -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS rrd_beat_id2name(bigint);   -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS rrd_file_id2name(bigint);   -- Az rrd_files tábla törölve.
CREATE OR REPLACE FUNCTION service_id2name(bigint) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE
AS $BODY$
DECLARE
    name TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT service_name INTO name FROM services WHERE service_id = $1;
    IF NOT FOUND THEN
        name := '#' || $1::text;
    END IF;
    RETURN name;
END;
$BODY$;
DROP FUNCTION IF EXISTS service_type_id2name(bigint); -- Automatikusan generált, kód modosítva
ALTER FUNCTION service_value2text(text, bigint, boolean) STABLE PARALLEL SAFE;
-- Hibajavítás
CREATE OR REPLACE FUNCTION service_var2service_rrd_var(vid bigint) RETURNS service_rrd_vars
    LANGUAGE 'plpgsql' VOLATILE PARALLEL UNSAFE AS $BODY$
DECLARE
    sv  service_vars;
    srv service_rrd_vars;
BEGIN
    SELECT * INTO sv FROM ONLY service_vars WHERE service_var_id = vid;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', vid, 'service_var_id', 'service_var2service_rrd_var(bigint)', 'service_vars');
        RETURN NULL;
    END IF;
    DELETE FROM service_vars WHERE service_var_id = vid;
    INSERT INTO service_rrd_vars(
            service_var_id, service_var_name, service_var_note, service_var_type_id, 
            host_service_id, service_var_value, var_state, last_time, features, 
            deleted, raw_value, delegate_service_state, state_msg, delegate_port_state, 
            disabled, flag, rarefaction, rrdhelper_id, rrd_beat_id, rrd_disabled)
        VALUES (
            vid, sv.service_var_name, sv.service_var_note, sv.service_var_type_id, 
            sv.host_service_id, sv.service_var_value, sv.var_state, sv.last_time, sv.features, 
            sv.deleted, sv.raw_value, sv.delegate_service_state, sv.state_msg, sv.delegate_port_state, 
            sv.disabled, sv.flag, sv.rarefaction, NULL, NULL, true)
        RETURNING * INTO srv;
    RETURN srv;
END;
$BODY$;
DROP FUNCTION IF EXISTS service_var_id2name(bigint); -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS service_var_type_id2name(bigint); -- Automatikusan generált, kód modosítva
ALTER FUNCTION shares_filt(portshare[], portshare)  IMMUTABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS subnet_id2name(bigint);     -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS table_is_exists(text);      -- nem használt
DROP FUNCTION IF EXISTS table_or_view_is_exists(text); -- nem használt
DROP FUNCTION IF EXISTS table_shape_field_id2name(bigint); -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS table_shape_field_name2id(text, text); -- nem használt
DROP FUNCTION IF EXISTS table_shape_id2name(bigint); -- Automatikusan generált, kód modosítva
CREATE OR REPLACE FUNCTION table_shape_id2name(bigint[]) RETURNS text
    LANGUAGE 'plpgsql' STABLE PARALLEL SAFE AS $BODY$
DECLARE
    tsid bigint;
    name text;
    rlist text := '';
BEGIN
    IF $1 IS NULL THEN
        RETURN NULL;
    END IF;
    FOREACH tsid IN ARRAY $1 LOOP
        SELECT table_shape_name INTO name FROM table_shapes WHERE table_shape_id = tsid;
        IF NOT FOUND THEN
            name := '#' || tsid::text;
        END IF;
        rlist := rlist || name || ',';
    END LOOP;
    RETURN trim( trailing ',' FROM rlist);
END
$BODY$;
ALTER FUNCTION time_in_timeperiod(bigint,timestamp without time zone) STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS timeperiod_id2name(bigint); -- Automatikusan generált, kód modosítva
ALTER FUNCTION timeperiod_name2id(text) STABLE PARALLEL SAFE;
ALTER FUNCTION timeperiod_next_on_time(bigint, timestamp without time zone) STABLE PARALLEL SAFE;
ALTER FUNCTION to_choose(text, text,OUT selects) STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS tool_id2name(bigint); -- Automatikusan generált, kód modosítva
DROP FUNCTION IF EXISTS user_id2name(bigint); -- Automatikusan generált, kód modosítva
ALTER FUNCTION user_is_any_groups_member(bigint, bigint[]) STABLE PARALLEL SAFE;
ALTER FUNCTION user_is_group_member(bigint, bigint) STABLE PARALLEL SAFE;
DROP FUNCTION IF EXISTS user_name2id(text); -- nem használt
DROP FUNCTION IF EXISTS view_is_exists(text); -- nem használt
DROP FUNCTION IF EXISTS vlan_id2name(bigint); -- Automatikusan generált, kód modosítva
ALTER FUNCTION xor(boolean, boolean) IMMUTABLE PARALLEL SAFE;

ALTER FUNCTION check_before_param_value() STABLE;
ALTER FUNCTION check_insert_menu_items() STABLE;
ALTER FUNCTION check_shape_field() IMMUTABLE;
ALTER FUNCTION crypt_user_password() STABLE;
CREATE OR REPLACE FUNCTION delete_node_post() RETURNS trigger
    LANGUAGE 'plpgsql' VOLATILE AS $BODY$
BEGIN
    DELETE FROM nports          WHERE node_id = OLD.node_id;
    DELETE FROM host_services   WHERE node_id = OLD.node_id;
    DELETE FROM node_params     WHERE node_id = OLD.node_id;
    DELETE FROM tool_objects    WHERE object_id = OLD.node_id
        AND object_name = ANY (ARRAY['nodes', 'snmpdevices']);
    RETURN OLD;
END;
$BODY$;
ALTER FUNCTION set_image_hash_if_null() STABLE;
ALTER FUNCTION user_events_before() STABLE;

-- A régi log rekordok törlése
CREATE OR REPLACE FUNCTION cut_down_old_logs() RETURNS VOID
    LANGUAGE 'plpgsql' VOLATILE PARALLEL UNSAFE AS $BODY$
DECLARE
    min_time    timestamp;
BEGIN
    min_time := NOW() - COALESCE(get_interval_sys_param('log_max_age'), '90 Days'::interval);
    DELETE FROM alarms WHERE begin_time < min_time;
    DELETE FROM app_errs WHERE date_of < min_time;
    DELETE FROM app_memos WHERE date_of < min_time;
    DELETE FROM arp_logs WHERE date_of < min_time;
    DELETE FROM db_errs WHERE date_of < min_time;
    DELETE FROM dyn_ipaddress_logs WHERE date_of < min_time;
    DELETE FROM host_service_logs WHERE date_of < min_time;
    DELETE FROM host_service_noalarms WHERE date_of < min_time;
    DELETE FROM imports WHERE date_of < min_time;
    DELETE FROM ip_address_logs WHERE date_of < min_time;
    DELETE FROM mactab_logs WHERE date_of < min_time;
    DELETE FROM port_vlan_logs WHERE date_of < min_time;
    DELETE FROM user_events WHERE created < min_time;
END
$BODY$;

SELECT set_db_version(1, 31);
