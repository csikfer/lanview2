-- Version 1.4 ==> 1.5

-- INSERT INTO sys_params
--    (sys_param_name,                    param_type_id,                 param_value,    sys_param_note) VALUES
--    ('ticet_reapeat_time',              param_type_name2id('interval'),'14 days',      'Ha ennél régebbi az azonos tiket riasztás, akkor új riasztás');

CREATE OR REPLACE FUNCTION ticket_alarm(
    lst notifswitch,        -- stat (last_state)
    msg text,               -- stat message
    did bigint DEFAULT NULL,-- daemon ID
    aid bigint DEFAULT NULL,-- superior alarm ID         
    fst notifswitch DEFAULT NULL,-- first stat
    mst notifswitch DEFAULT NULL -- max stat
) RETURNS alarms AS $$
DECLARE
    ar alarms;
    hs host_services;
    repi interval;
    noa boolean;
    sid bigint := 0;
BEGIN
    SELECT * INTO hs FROM host_services WHERE host_service_id = sid;
    IF NOT FOUND THEN
        IF 0 = COUNT(*) FROM nodes WHERE node_id = sid THEN
            INSERT INTO nodes(node_id, node_name, node_note,    node_type )
               VALUES        (sid,     'nil',   'Independent', '{node, virtual}');
        END IF;
        IF 0 = COUNT(*) FROM services WHERE service_id = sid THEN
            INSERT INTO services (service_id, service_name, service_note, disabled, service_type_id)
                 VALUES          (  sid,      'ticket',     'Hiba jegy',  true,     sid );
        END IF;
        INSERT INTO host_services (host_service_id, node_id, service_id,  host_service_note, disabled)
             VALUES               (  sid,           sid,     sid,         'Hiba jegy',       true)
             RETURNING * INTO hs;
    END IF;
    IF 'on' = is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to) THEN
        noa := true;
    ELSE
        noa := false;
    END IF;
    repi := COALESCE(get_interval_sys_param('ticet_reapeat_time'), '14 days'::interval);
    SELECT * INTO ar FROM alarms
		WHERE host_service_id = sid
                  AND (begin_time + repi) > NOW()
		  AND end_time IS NULL
		  AND lst = last_status
		  AND COALESCE(aid, -1) = COALESCE(superior_alarm_id, -1)
		  AND COALESCE(did, -1) = COALESCE(daemon_id, -1)
		  AND msg = event_note
		LIMIT 1;
    IF NOT FOUND THEN
	INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id, noalarm)
		    VALUES (sid,             did, COALESCE(fst, lst), COALESCE(mst, lst), lst, msg,        aid,               noa)
		RETURNING * INTO ar;
    END IF;
    RETURN ar;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    newip   boolean := false;
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
                newip := true;
            WHEN TOO_MANY_ROWS THEN
                PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
    END;
    IF stp = 'config' THEN  -- We assume this is fixip
        IF newip THEN   -- NEW ip_addresses record
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
        ELSE            -- Check ip_addresses record
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
    ELSE
        IF newip THEN   -- NEW ip_addresses record
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
        ELSE
            RAISE INFO 'Address record (%) : % - %', stp, ipa, port_id2full_name(oip.port_id);
            IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
                IF oip.ip_address_type = 'dynamic' THEN     -- Clear old dynamic IP
                    UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
                ELSE
                    t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is query. '
                      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
                    PERFORM ticket_alarm('critical', t, hsi);
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
    
ALTER TABLE imports ADD COLUMN out_msg text;

CREATE OR REPLACE FUNCTION alarm_id2name(bigint) RETURNS TEXT AS $$
DECLARE
    rname TEXT;
BEGIN
    IF $1 IS NULL THEN
        return NULL;
    END IF;
    SELECT host_service_id2name(host_service_id) || '/' || alarm_message(host_service_id, max_status) INTO rname
        FROM alarms
        WHERE alarm_id = $1;
    IF NOT FOUND THEN
        rname := host_service_id2name(host_service_id) || '/ #' || $1 || 'not found'; 
    END IF;
    RETURN rname;
END;
$$ LANGUAGE plpgsql;

    
SELECT set_db_version(1, 5);
