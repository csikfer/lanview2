ALTER TYPE fieldflag ADD VALUE IF NOT EXISTS 'notext';
COMMENT ON TYPE fieldflag IS 'Field flags:
table_hide      The field is hidden in the table.
dialog_hide     The field is hidden in the dialog.
read_only       The field is read only.
passwd          The field is password.
huge            Long text.
batch_edit      The field modify Batch.
bg_color        Set background color by enum_vals record.
fg_color        Set character color by enum_vals record.
font            Set font by enum_vals record.
tool_tip        Show Tool tip text by enum_vals record.
HTML            This field is visible in the HTML report.
raw             Show raw value
image           Display an image or icon
notext          Hide text, but only if the image flag is exists.';

ALTER TYPE addresstype ADD VALUE IF NOT EXISTS 'manual';
COMMENT ON TYPE addresstype IS 'IP address types:
fixip   Fix ip address (automatic/DHCP).
private Private IP address, non routing, separated.
external    Expernal IP address.
dynamic     Dinamic IP address (DHCP).
pseudo      Not a real IP address.
joint       Not a unique IP address (Cluster, ...).
manual      Manually set address.';

DROP TABLE IF EXISTS ip_address_logs;
CREATE TABLE ip_address_logs (
    ip_addres_log_id    bigserial NOT NULL PRIMARY KEY,
    date_of             timestamp NOT NULL DEFAULT NOW(),
    reason              reasons,
    message             text,
    daemon_id           bigint
      REFERENCES host_services (host_service_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL,
    ip_address_id       bigint
      REFERENCES ip_addresses (ip_address_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL,
    address_new         inet,
    ip_address_type_new addresstype,
    port_id             bigint
      REFERENCES interfaces (port_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL,
    address_old         inet,
    ip_address_type_old addresstype
);
CREATE INDEX IF NOT EXISTS ip_address_logs_date_of_index ON ip_address_logs (date_of);

CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
    ipa ip_addresses;
    nip boolean;    -- IP address type IS NULL
    snid bigint;
    cnt integer;
BEGIN
    -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím ?
    nip := NEW.ip_address_type IS NULL;
    IF nip THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) IS NOT NULL THEN
            NEW.ip_address_type := 'dynamic';
        ELSE
            NEW.ip_address_type := 'fixip';
        END IF;
    END IF;
    IF NEW.address IS NOT NULL THEN
        -- Check subnet_id
        IF NEW.subnet_id IS NOT NULL AND NOT (SELECT NEW.address << netaddr FROM subnets WHERE subnet_id = NEW.subnet_id) THEN
            PERFORM error('Params', NEW.subnet_id, 'subnet for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
        -- Nincs subnet (id), keresünk egyet
        IF NEW.subnet_id IS NULL AND NEW.ip_address_type <> 'external' AND NEW.ip_address_type <> 'private' THEN
            BEGIN
                SELECT subnet_id INTO STRICT NEW.subnet_id FROM subnets WHERE netaddr >> NEW.address;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN     -- nem találtunk
                        PERFORM error('NotFound', -1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                        PERFORM error('Ambiguous',-1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END;
            -- RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        ELSIF NEW.ip_address_type = 'external'  THEN
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha nem private, akkor vizsgáljuk az ütközéseket
        IF NEW.ip_address_type <> 'private' THEN
            -- Azonos IP címek?
            FOR ipa IN SELECT * FROM ip_addresses WHERE NEW.address = address LOOP
                IF ipa.ip_address_id <> NEW.ip_address_id THEN 
                    IF ipa.ip_address_type = 'dynamic' OR is_dyn_addr(NEW.address) THEN
                    -- Ütköző dinamikust töröljük.
                        UPDATE ip_addresses SET address = NULL, ip_address_type = 'dynamic' WHERE ip_address_id = ipa.ip_address_id;
                    ELSIF ipa.ip_address_type = 'joint' AND (nip OR NEW.ip_address_type = 'joint') THEN
                    -- Ha közös címként van megadva a másik, ...
                        NEW.ip_address_type := 'joint';
                    ELSIF ipa.ip_address_type <> 'private' THEN
                    -- Minden más esetben ha nem privattal ütközik az hiba
                        PERFORM error('IdNotUni', 0, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    END IF;
                END IF;
            END LOOP;
        END IF;
        -- Ha a preferred nincs megadva, akkor az elsőnek megadott cím a preferált
        IF NEW.preferred IS NULL THEN
            SELECT 1 + COUNT(*) INTO NEW.preferred FROM interfaces JOIN ip_addresses USING(port_id) WHERE port_id = NEW.port_id AND preferred IS NOT NULL AND address IS NOT NULL;
        END IF;
    ELSE
        -- Cím ként a NULL csak a dynamic típusnál megengedett
        IF NEW.ip_address_type <> 'dynamic' THEN
            PERFORM error('DataError', 0, 'NULL ip as non dynamic type', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
        END IF;
        -- RAISE INFO 'IP address is NULL';
    END IF;
    -- RAISE INFO 'Return, NEW = %', NEW;
    IF TG_OP = 'UPDATE' THEN 
        IF NEW.ip_address_id <> OLD.ip_address_id THEN
            PERFORM error('Constant', OLD.ip_address_id, 'ip_address_id', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;


-- !!!!! Több hiba javítva : 2019.05.04. !!!!!
-- !!!!! Hiba javítás : 2019.05.06. !!!!!
CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;               -- arps record
    noipa   boolean := false;   -- Not found ip_address record
    joint   boolean := false;   -- Is joint type ip address
    oip     ip_addresses;       -- Found IP address record
    n       integer;            -- record number
    t       text;               -- Text for arps record (arp_note)
    msg     text;               -- Text for ip_address record (ip_address_note), or other message
    col     boolean := false;   -- Address collision
    adt addresstype := 'fixip';
BEGIN
    -- RAISE INFO 'ARP : % - %; %', ipa, hwa, stp;
    -- check ip_addresses table (set: noipa and joint)
    BEGIN       -- IP -> Get old () ip_addresses record
        SELECT * INTO STRICT oip FROM ip_addresses WHERE address = ipa AND ip_address_type <> 'private'::addresstype;
        joint := 'joint'::addresstype = oip.ip_address_type;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                noipa := true;
                -- RAISE INFO 'No ip_addresses record.';
            WHEN TOO_MANY_ROWS THEN
		-- RAISE INFO 'More ip_addresses record.';
                IF 0 < COUNT(*) FROM ip_addresses WHERE address = ipa AND NOT(ip_address_type = 'joint'::addresstype OR ip_address_type = 'private'::addresstype) THEN
                    PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
                END IF;
                -- RAISE INFO 'ip_addresses records is all join.';
                joint := true;
    END;
    IF NOT noipa THEN
        IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
            IF oip.ip_address_type = 'dynamic' THEN
                -- Delete outdated address
                UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
                INSERT INTO ip_address_logs(reason, daemon_id, ip_address_id, ip_address_type_new, port_id, address_old, ip_address_type_old)
                    VALUES('discard', hsi, oip.ip_address_id, 'dynamic', oip.port_id, oip.address, 'dynamic');
                noipa := true;
            ELSE
                col := true;
            END IF;
        END IF;
    END IF;
    IF noipa THEN   -- ip_addresses record not found by IP address (ipa)
        -- ip address record(s) by MAC
        SELECT ip_addresses.* INTO oip FROM ip_addresses JOIN interfaces USING(port_id)
                WHERE hwaddress = hwa AND (ip_address_type = 'fixip'::addresstype OR ip_address_type = 'dynamic'::addresstype);
        GET DIAGNOSTICS n = ROW_COUNT;
        IF n = 1 AND (stp = 'config' OR oip.ip_address_type = 'dynamic') THEN
            DECLARE
                det text;
                hnt text;
                snid bigint := oip.subnet_id;
            BEGIN 
                IF stp = 'query' AND is_dyn_addr(ipa) IS NOT NULL THEN
                    adt := 'dynamic';
                END IF;
                IF NOT ipa << netaddr FROM subnets WHERE subnet_id = snid THEN
                    snid := NULL;
                END IF;
                t := 'Modify by config : ' || oip.ip_address_type  || ' -> ' || adt || '; ' || oip.address || ' -> ' || ipa;
                msg := 'Modify by replace_arp(), service : ' || COALESCE(host_service_id2name(hsi), 'NULL') || ' ' || NOW()::text;
                UPDATE ip_addresses SET ip_address_note = msg, address = ipa, ip_address_type = adt, subnet_id = snid WHERE ip_address_id = oip.ip_address_id;
                INSERT INTO ip_address_logs(reason, message, daemon_id, ip_address_id, address_new, ip_address_type_new, port_id, address_old, ip_address_type_old)
                    VALUES('modify', msg, hsi, oip.ip_address_id, ipa, adt, oip.port_id, oip.address, oip.ip_address_type);
            EXCEPTION WHEN OTHERS THEN
                GET STACKED DIAGNOSTICS
                    msg = MESSAGE_TEXT,
                    det = PG_EXCEPTION_DETAIL,
                    hnt = PG_EXCEPTION_HINT;
                t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp
                    || ' The existing IP address record port name : ' || port_id2full_name(oip.port_id)
                    || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
                -- RAISE WARNING 'Ticket : %', t; 
                PERFORM ticket_alarm('critical', t, hsi);
            END;
        ELSE
            DECLARE
                pid bigint;
            BEGIN
                SELECT port_id INTO pid FROM interfaces WHERE hwaddress = hwa;
                GET DIAGNOSTICS n = ROW_COUNT;
                IF n = 1 THEN
                    IF stp = 'query' AND is_dyn_addr(ipa) IS NOT NULL THEN
                        adt := 'dynamic';
                    END IF;
                    t := 'Inser IP address (' || adt || ') record : port : ' || port_id2full_name(pid) || ' .';
                    msg := 'Insert by replace_arp(), service : ' || COALESCE(host_service_id2name(hsi), 'NULL') || ' ' || NOW()::text;
                    INSERT INTO ip_addresses(port_id, ip_address_note, address, ip_address_type) VALUES(pid, msg, ipa, adt);
                END IF;
            END;
        END IF;
    ELSIF col THEN  -- ip_addresses found by IP address (ipa) AND colision
        t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
          || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
        -- RAISE WARNING 'Ticket : %', t; 
        PERFORM ticket_alarm('critical', t, hsi);
    ELSE            -- ip_addresses found by IP address (ipa) AND NOT colision
        IF stp = 'config' AND oip.ip_address_type = 'dynamic' THEN
            DECLARE
                sid bigint := NULL;
            BEGIN
                UPDATE ip_addresses SET ip_address_type = 'fixip' WHERE ip_address_id = oip.ip_address_id;
                INSERT INTO ip_address_logs(reason, message, daemon_id, ip_address_id, address_new, ip_address_type_new, port_id, address_old, ip_address_type_old)
                    VALUES('update', msg, hsi, oip.ip_address_id, ipa, 'fixip', oip.port_id, ipa, 'dynamic');
            END;
        END IF;
    END IF;
    -- update arps table
    -- RAISE INFO 'Get arps record : %', ipa;
    SELECT * INTO arp FROM arps WHERE ipaddress = ipa;
    IF NOT FOUND THEN
        -- RAISE INFO 'Insert arps: % - %', ipa, hwa;
        INSERT INTO arps(ipaddress, hwaddress,set_type, host_service_id, arp_note) VALUES (ipa, hwa, stp, hsi, t);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = hwa THEN
	    IF arp.set_type < stp THEN
                -- RAISE INFO 'Update arps: % - %', ipa, hwa;
	        UPDATE arps SET set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'update';
	    ELSE
                -- RAISE INFO 'Touch arps: % - %', ipa, hwa;
	        UPDATE arps SET last_time = CURRENT_TIMESTAMP, arp_note = t WHERE ipaddress = arp.ipaddress;
		RETURN 'found';
	    END IF;
        ELSE
            -- RAISE INFO 'Move arps: % - % -> %', ipa, arp.hwaddress, hwa;
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
    sid bigint := 0;    -- host_service_id : nil.ticket == node_id : nil == service_id : ticket
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
    IF 'on' <> is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to) THEN
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
            INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id)
                        VALUES (sid,             did, COALESCE(fst, lst), COALESCE(mst, lst), lst, msg,        aid)
                    RETURNING * INTO ar;
        END IF;
    END IF;
    RETURN ar;
END;
$$ LANGUAGE plpgsql;

DROP VIEW mactab_logs_shape;

-- ------------------------------------------------------------------------------------------------

SELECT set_db_version(1, 23);
