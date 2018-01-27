-- bugfixes
DROP TRIGGER IF EXISTS nports_check_reference_node_id           ON nports;
DROP TRIGGER IF EXISTS node_param_value_check_reference_node_id ON node_params;

DROP TRIGGER IF EXISTS port_params_check_reference_port_id      ON port_params;
DROP TRIGGER IF EXISTS port_param_value_check_reference_port_id ON port_params;

-- A perl függvények, és triggerek is valamikor (jó régen) elvesztek. Talán a CREATE LANGUAGE maradhatott el egy reinstallnál ?
CREATE OR REPLACE PROCEDURAL LANGUAGE plperl;

CREATE OR REPLACE FUNCTION check_reference_port_id() RETURNS TRIGGER AS $$
    ($null, $inTable, $exTable) = @{$_TD->{args}};
    if ((defined($null) && $null && !defined($_TD->{new}{port_id}))
     || ($_TD->{new}{port_id} == $_TD->{old}{port_id})) { return; } # O.K.
    if (!defined($inTable)) { $inTable = 'nports'; }
    $rv = spi_exec_query('SELECT COUNT(*) FROM ' . $inTable .' WHERE port_id = ' . $_TD->{new}{port_id});
    if( $rv->{status} ne SPI_OK_SELECT) {
        spi_exec_query("SELECT error('DataError', 'Status not SPI_OK_SELECT, but $rv->{status}', '$_TD->{table_name}', '$_TD->{event}');");
    }
    $nn  = $rv->{rows}[0]->{count};
    if ($nn == 0) {
        spi_exec_query("SELECT error('InvRef', $_TD->{new}{port_id}, '$inTable.port_id', 'check_reference_port_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if ($nn != 1) {
        spi_exec_query("SELECT error('DataError', $_TD->{new}{port_id}, '$inTable.port_id', 'check_reference_port_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if (defined($exTable) && $exTable) {
        $rv = spi_exec_query("SELECT COUNT(*) FROM $exTable WHERE port_id = $_TD->{new}{port_id}");
        $nn = $rv->{rows}[0]->{count};
        if ($nn != 0) {
            $cmd = "SELECT error('InvRef', $_TD->{new}{port_id}, '$exTable' ,'check_reference_port_id()', '$_TD->{table_name}', '$_TD->{event}');";
            # elog(NOTICE, $cmd);
            spi_exec_query($cmd);
            return "SKIP";
        }
    }
    return;
$$ LANGUAGE plperl;
COMMENT ON FUNCTION check_reference_port_id() IS 
'Ellenőrzi, hogy a port_id mező valóban egy port rekordra mutat-e.
Ha az opcionális első paraméter true, akkor megengedi a NULL értéket is.
Ha megadjuk a második paramétert, akkor az a tábla neve, amelyikben és amelyik leszármazottai között szerepelnie kell a port rekordnak
Ha a paraméter nincs megadva, akkor az összes port táblában keres (nport)
Ha mega van adva egy harmadik paraméter, akkor az egy tábla név, melyben nem szerpelhet a rekord (csak "pport" lehet)';

CREATE OR REPLACE FUNCTION check_reference_node_id() RETURNS TRIGGER AS $$
    ($null, $inTable, $outTable) = @{$_TD->{args}};
    if ((defined($null) && $null && !defined($_TD->{new}{node_id}))
     || ($_TD->{new}{node_id} == $_TD->{old}{node_id})) { return; } # O.K.
    if (!defined($inTable)) { $inTable = 'patchs'; }
    $rv = spi_exec_query('SELECT COUNT(*) FROM ' . $inTable .' WHERE node_id = ' . $_TD->{new}{node_id});
    $nn  = $rv->{rows}[0]->{count};
    if ($nn == 0) {
        spi_exec_query("SELECT error('InvRef', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if ($nn != 1) {
        spi_exec_query("SELECT error('DataError', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
        return "SKIP";
    }
    if (defined($outTable) && $outTable) {
        $rv = spi_exec_query('SELECT COUNT(*) FROM ONLY ' . $outTable .' WHERE node_id = ' . $_TD->{new}{node_id});
        $nn  = $rv->{rows}[0]->{count};
        if ($nn != 0) {
            spi_exec_query("SELECT error('DataError', $_TD->{new}{node_id}, '$inTable', 'check_reference_node_id()', '$_TD->{table_name}', '$_TD->{event}');");
            return "SKIP";
        }
    }
    return;
$$ LANGUAGE plperl;
COMMENT ON FUNCTION check_reference_node_id() IS
'Ellenőrzi, hogy a node_id mező valóban egy node rekordra mutat-e.
Ha az opcionális első paraméter true, akkor megengedi a NULL értéket is.
Ha megadjuk a második paramétert, akkor az a tábla neve, amelyikben és amelyik leszármazottai között szerepelnie kell a node rekordnak
Ha a paraméter nincs megadva, akkor az összes node táblában keres (patchs)
Ha megadjuk a harmadik paramétert akkor az itt megadott táblában nem szerepelhet a node_id (ONLY !!)';

CREATE TRIGGER nports_check_reference_node_id           BEFORE UPDATE OR INSERT ON nports      FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'nodes');
CREATE TRIGGER node_param_value_check_reference_node_id BEFORE UPDATE OR INSERT ON node_params FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'patchs');
CREATE TRIGGER port_params_check_reference_port_id      BEFORE UPDATE OR INSERT ON port_params FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id();

CREATE OR REPLACE FUNCTION add_member_to_all_group() RETURNS TRIGGER AS $$
    ($table, $midname, $gidname, $gid) = @{$_TD->{args}};
    $mid = $_TD->{new}->{$midname};
    spi_exec_query("INSERT INTO $table ($gidname, $midname) VALUES ( $gid, $mid) ON CONFLICT DO NOTHING");
    return;
$$ LANGUAGE plperl;
COMMENT ON FUNCTION add_member_to_all_group() IS
'Létrehoz egy kapcsoló tábla rekordot, egy group ás tag között (ha az INSERT-ben valamelyik szabály nem teljesül, akkor nem csinál semmit).
Vagyis egy megadott objektumot betesz egy megadott csoportba
Paraméterek (sorrendben):
$table   tábla név, A kapcsoló tábla neve,
$midname a member azonosító (id) neve a kapcsoló, és a member táblában (feltételezzük, hogy azonos),
$gidname a group azonosító (id) neve a kapcsoló táblában,
$gid     a group azonisítója (id)';

CREATE INDEX place_group_places_place_id_index       ON place_group_places USING btree (place_id);
CREATE INDEX place_group_places_place_group_id_index ON place_group_places USING btree (place_group_id);
DROP TRIGGER IF EXISTS add_place_to_all_group ON places;
CREATE TRIGGER add_place_to_all_group AFTER INSERT ON places
    FOR EACH ROW EXECUTE PROCEDURE add_member_to_all_group('place_group_places', 'place_id', 'place_group_id', 1);

-- régi nem használt konverziós függvények törlése
DROP FUNCTION IF EXISTS text2bigint(val text, def bigint);
DROP FUNCTION IF EXISTS text2double(val text, def double);
DROP FUNCTION IF EXISTS text2time(val text, def time);
DROP FUNCTION IF EXISTS text2date(val text, def date);
DROP FUNCTION IF EXISTS text2timestamp(val text, def timestamp);
DROP FUNCTION IF EXISTS text2interval(val text, def interval);
DROP FUNCTION IF EXISTS text2inet(val text, def inet);
DROP FUNCTION IF EXISTS text2macaddr(val text, def macaddr);

-- bugfixes end

CREATE OR REPLACE FUNCTION cast_to_boolean(text, boolean DEFAULT NULL) RETURNS boolean AS $$
BEGIN
    RETURN cast($1 as boolean);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_integer(text, bigint DEFAULT NULL) RETURNS bigint AS $$
BEGIN
    RETURN cast($1 as bigint);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_real(text, double precision DEFAULT NULL) RETURNS double precision AS $$
BEGIN
    RETURN cast($1 as double precision);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_date(text, date DEFAULT NULL) RETURNS date AS $$
BEGIN
    RETURN cast($1 as date);
EXCEPTION
    WHEN invalid_datetime_format THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_time(text, time DEFAULT NULL) RETURNS time AS $$
BEGIN
    RETURN cast($1 as time);
EXCEPTION
    WHEN invalid_datetime_format THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_datetime(text, timestamp DEFAULT NULL) RETURNS timestamp AS $$
BEGIN
    RETURN cast($1 as timestamp);
EXCEPTION
    WHEN invalid_datetime_format THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_interval(text, interval DEFAULT NULL) RETURNS interval AS $$
BEGIN
    RETURN cast($1 as interval);
EXCEPTION
    WHEN invalid_datetime_format THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_inet(text, inet DEFAULT NULL) RETURNS inet AS $$
BEGIN
    RETURN cast($1 as inet);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_cidr(text, cidr DEFAULT NULL) RETURNS cidr AS $$
BEGIN
    RETURN cast($1 as cidr);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_mac(text, macaddr DEFAULT NULL) RETURNS macaddr AS $$
BEGIN
    RETURN cast($1 as macaddr);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION cast_to_point(text, point DEFAULT NULL) RETURNS point AS $$
BEGIN
    RETURN cast($1 as point);
EXCEPTION
    WHEN invalid_text_representation THEN
        RETURN $2;
END;
$$ LANGUAGE plpgsql IMMUTABLE;


DROP TYPE text2type; -- ==> param_types
-- Change paramtype enum type
ALTER TABLE param_types ALTER COLUMN param_type_type SET DATA TYPE text;
DROP TYPE paramtype;
CREATE TYPE paramtype AS ENUM ( 'text', 'boolean', 'integer', 'real', 'date', 'time', 'datetime', 'interval', 'inet', 'cidr', 'mac', 'point', 'bytea');
ALTER TABLE param_types ALTER COLUMN param_type_type SET DATA TYPE paramtype USING
    CASE param_type_type
        WHEN 'text'    		THEN 'text'::paramtype
        WHEN 'boolean' 		THEN 'boolean'::paramtype
        WHEN 'bigint'  		THEN 'integer'::paramtype
        WHEN 'double precision' THEN 'real'::paramtype
        WHEN 'date' 		THEN 'date'::paramtype
        WHEN 'time' 		THEN 'time'::paramtype
        WHEN 'timestamp' 	THEN 'datetime'::paramtype
        WHEN 'interval' 	THEN 'interval'::paramtype
        WHEN 'inet' 		THEN 'inet'::paramtype
        WHEN 'bytea' 		THEN 'bytea'::paramtype
    END;

CREATE OR REPLACE FUNCTION check_paramtype(v text, t paramtype) RETURNS text AS $$
BEGIN
    RETURN CASE t
        WHEN 'text'     THEN v
        WHEN 'boolean'  THEN CASE v::boolean WHEN true THEN 'true' ELSE 'false' END
        WHEN 'integer'  THEN v::bigint::text
        WHEN 'real'     THEN v::double precision::text
        WHEN 'date'     THEN v::date::text
        WHEN 'time'     THEN v::time::text
        WHEN 'datetime' THEN v::timestamp::text
        WHEN 'interval' THEN v::interval::text
        WHEN 'inet'     THEN v::inet::text
        WHEN 'cidr'     THEN v::cidr::text
        WHEN 'mac'      THEN v::macaddr::text
        WHEN 'point'    THEN v::point::text
        WHEN 'bytea'    THEN v
    END;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION check_before_param_value() RETURNS TRIGGER AS $$
DECLARE
    pt  paramtype;
BEGIN
    SELECT param_type_type INTO pt FROM param_types WHERE param_type_id = NEW.param_type_id;
    NEW.param_value = check_paramtype(NEW.param_value, pt);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

ALTER TABLE node_params DROP CONSTRAINT IF EXISTS node_param_check_value;
ALTER TABLE port_params DROP CONSTRAINT IF EXISTS port_param_check_value;
ALTER TABLE sys_params  DROP CONSTRAINT IF EXISTS sys_param_check_value;
CREATE TRIGGER node_param_check_value BEFORE INSERT OR UPDATE ON node_params FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();
CREATE TRIGGER port_param_check_value BEFORE INSERT OR UPDATE ON port_params FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();
CREATE TRIGGER sys_param_check_value  BEFORE INSERT OR UPDATE ON sys_params  FOR EACH ROW EXECUTE PROCEDURE check_before_param_value();

CREATE OR REPLACE FUNCTION check_before_service_value() RETURNS TRIGGER AS $$
DECLARE
    tid bigint;
    pt  paramtype;
BEGIN
    SELECT param_type_id   INTO tid FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id;
    SELECT param_type_type INTO pt  FROM param_types       WHERE param_type_id = tid;
    NEW.param_value = check_paramtype(NEW.param_value, pt);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

ALTER TABLE service_vars  DROP CONSTRAINT IF EXISTS service_vars_check_value;
CREATE TRIGGER service_vars_check_value  BEFORE INSERT OR UPDATE ON service_vars  FOR EACH ROW EXECUTE PROCEDURE check_before_service_value();

-- ************************************************* BUGFIX! *********************************************************************

DROP TRIGGER IF EXISTS alarm_delete_alarms          ON alarms;
DROP TRIGGER IF EXISTS disabled_alarm_delete_alarms ON disabled_alarms;
CREATE TRIGGER alarm_delete_alarms           BEFORE DELETE ON alarms            FOR EACH ROW EXECUTE PROCEDURE delete_alarms();
CREATE TRIGGER disabled_alarm_delete_alarms  BEFORE DELETE ON disabled_alarms   FOR EACH ROW EXECUTE PROCEDURE delete_alarms();
CREATE TRIGGER interfaces_check_reference_node_id    BEFORE UPDATE OR INSERT ON interfaces    FOR EACH ROW EXECUTE PROCEDURE check_reference_node_id('false', 'nodes');
ALTER TABLE port_vlans DROP CONSTRAINT port_vlans_port_id_fkey;
CREATE TRIGGER port_vlans_check_reference_port_id    BEFORE UPDATE OR INSERT ON port_vlans FOR EACH ROW EXECUTE PROCEDURE check_reference_port_id('false', 'nports', 'pports');

-- ************************************************ BUGFIX #2 ******************************************************************* 

CREATE OR REPLACE FUNCTION check_before_service_value() RETURNS TRIGGER AS $$
DECLARE
    tid bigint;
    pt  paramtype;
BEGIN
    SELECT param_type_id   INTO tid FROM service_var_types WHERE service_var_type_id = NEW.service_var_type_id;
    SELECT param_type_type INTO pt  FROM param_types       WHERE param_type_id = tid;
    NEW.param_value = check_paramtype(NEW.service_var_value, pt);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    noipa   boolean := false;
    joint   boolean := false;
    oip     ip_addresses;
    net     cidr;
    sni     bigint;
    n       integer;
    t       text;
BEGIN
    RAISE INFO 'ARP : % - %; %', ipa, hwa, stp;
    -- check / update ip_addresses table
    BEGIN       -- IP -> Get old ip_addresses record
        SELECT * INTO STRICT oip FROM ip_addresses WHERE address = ipa;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                noipa := true;
            WHEN TOO_MANY_ROWS THEN
                IF 0 < COUNT(*) FROM ip_addresses WHERE address = ipa AND ip_address_type <> 'joint'::addresstype THEN
                    PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
                END IF;
                joint := true;
    END;
    IF stp = 'config' THEN  -- We assume this is fixip
        IF noipa THEN   -- not found ip_addresses record
            RAISE INFO 'Nothing address record (config) : %', ipa;
            SELECT ip_addresses.* INTO oip FROM ip_addresses JOIN interfaces USING(port_id) WHERE ip_address_type = 'dynamic' AND hwaddress = hwa;
            GET DIAGNOSTICS n = ROW_COUNT;
            IF n = 1 THEN   -- One record ?
		t := 'Modify by config : ' || oip.ip_address_type  || ' -> fixip ; ' || oip.address || ' -> ' || ipa;
		PERFORM error('DataWarn', oip.ip_address_id, t, 'replace_arp', 'ip_addresses');
                DECLARE
                    msg text;
                    det text;
                    hnt text;
                BEGIN 
                    UPDATE ip_addresses SET address = ipa, ip_address_type = 'fixip' WHERE ip_address_id = oip.ip_address_id;
                EXCEPTION WHEN OTHERS THEN
                    GET STACKED DIAGNOSTICS
                        msg = MESSAGE_TEXT,
                        det = PG_EXCEPTION_DETAIL,
                        hnt = PG_EXCEPTION_HINT;
                    t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id)
                      || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
                    PERFORM ticket_alarm('critical', t, hsi);
                END;
            END IF;
        ELSIF joint THEN    -- Check ip_addresses record if joint
            IF 0 < COUNT(*) FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa AND hwaddress <> hwa THEN
                t := ARRAY(SELECT port_id2full_name(port_id) FROM ip_addresses WHERE address = ipa)::text;
                t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                  || 'The existing IP address record ports names : ' || t;
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        ELSE                -- Check ip_addresses record if not joint
            RAISE INFO 'Address record (config) : % - %', ipa, port_id2full_name(oip.port_id);
            IF hwa = hwaddress FROM interfaces WHERE port_id = oip.port_id THEN -- Check MAC
                IF oip.ip_address_type = 'dynamic' THEN
                    UPDATE ip_addresses SET ip_address_type = 'fixip' WHERE ip_address_id = oip.ip_address_id;
                END IF;
            ELSE    -- ip address collision
                t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                  || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        END IF;
    ELSE    -- stp <> 'config' ( = 'query')
        IF joint THEN    -- Check ip_addresses record if joint
            t := ARRAY(SELECT port_id2full_name(port_id) FROM ip_addresses WHERE address = ipa)::text;
            RAISE INFO 'Address record (%) : % - [%]', stp, ipa, t;
            IF 0 < COUNT(*) FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa AND hwaddress <> hwa THEN
                t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is config. '
                  || 'The existing IP address record ports names : ' || t;
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        ELSE
            IF NOT noipa THEN
                RAISE INFO 'Address record (%) : % - %', stp, ipa, port_id2full_name(oip.port_id);
                IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
                    IF oip.ip_address_type = 'dynamic' THEN     -- Clear old dynamic IP
                        RAISE INFO 'Delete address (%) : % - %', stp, port_id2full_name(oip.port_id), ipa;
                        UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
                        noipa := true;
                    ELSE
                        t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is query. '
                        || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
                        PERFORM ticket_alarm('critical', t, hsi);
                    END IF;
                END IF;
            END IF;
            IF noipa THEN
                RAISE INFO 'Nothing address record (%) : %', stp, ipa;
                SELECT ip_addresses.* INTO oip FROM ip_addresses JOIN interfaces USING(port_id) WHERE ip_address_type = 'dynamic' AND hwaddress = hwa;
                GET DIAGNOSTICS n = ROW_COUNT;
                RAISE INFO '% record by hwaddress', n;
                IF n = 1 THEN   -- One record ?
                    DECLARE
                        msg text;
                        det text;
                        hnt text;
                    BEGIN
                        RAISE INFO 'Update ip_address : % -> %', ipa, port_id2full_name(oip.port_id);
                        UPDATE ip_addresses SET address = ipa WHERE ip_address_id = oip.ip_address_id;
                    EXCEPTION WHEN OTHERS THEN
                        GET STACKED DIAGNOSTICS
                            msg = MESSAGE_TEXT,
                            det = PG_EXCEPTION_DETAIL,
                            hnt = PG_EXCEPTION_HINT;
                        t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is query. '
                        || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id)
                        || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
                        PERFORM ticket_alarm('critical', t, hsi);
                    END;
                END IF;
            END IF;
        END IF;
    END IF;
    -- update arps table
    RAISE INFO 'Get arps record : %', ipa;
    SELECT * INTO arp FROM arps WHERE ipaddress = ipa;
    IF NOT FOUND THEN
        RAISE INFO 'Insert arps: % - %', ipa, hwa;
        INSERT INTO arps(ipaddress, hwaddress,set_type, host_service_id, arp_note) VALUES (ipa, hwa, stp, hsi, t);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = hwa THEN
	    IF arp.set_type < stp THEN
                RAISE INFO 'Update arps: % - %', ipa, hwa;
	        UPDATE arps SET set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'update';
	    ELSE
                RAISE INFO 'Touch arps: % - %', ipa, hwa;
	        UPDATE arps SET last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'found';
	    END IF;
        ELSE
            RAISE INFO 'Move arps: % - % -> %', ipa, arp.hwaddress, hwa;
            UPDATE arps
                SET hwaddress = hwa,  first_time = CURRENT_TIMESTAMP, set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP, arp_note = t
                WHERE ipaddress = arp.ipaddress;
            INSERT INTO
                arp_logs(reason, ipaddress, hwaddress_new, hwaddress_old, set_type_old, host_service_id_old, first_time_old, last_time_old)
                VALUES( 'move',  ipa,       hwa,           arp.hwaddress, arp.set_type, arp.host_service_id, arp.first_time, arp.last_time);
            RETURN 'modify';
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION replace_arp(inet, macaddr, settype, bigint) IS
'A detektált MAC - IP cím pár alapján modosítja az arps táblát, és kezeli a napló táblát is
Ellenörzi, és frissíti az ip_addresses táblát is (ha stp = "config", akkor feltételezi, hogy ez egy fixip típusú cím)
Ha ütközést észlel, akkor létrehoz egy alarms rekordot (szolgáltatás : ticket, host_service_id = 0).
Paraméterek:
    ipa     IP cím
    hwa     MAC
    stp     adat forrás, default "query"
    hsi     opcionális host_service_id
Visszatérési érték:
    Ha létrejött egy új rekord, akkor "insert".
    Ha nincs változás (csak a last_time frissül), akkor "found".
    Ha a cím összerendelés nem változott, de a set_type igen, akkor "update".
    Ha az IP cím egy másik MAC-hez lett rendelve, akkor "modfy".';

-- END BUGFIX

SELECT set_db_version(1, 9);
