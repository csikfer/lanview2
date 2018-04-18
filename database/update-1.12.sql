
CREATE TABLE port_vlan_logs (
    port_vlan_log_id    bigserial       PRIMARY KEY,
    date_of             timestamp       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    reason              reasons         NOT NULL,
    port_id             bigint          NOT NULL,
    vlan_id             bigint          NOT NULL
         REFERENCES vlans(vlan_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    old_type            vlantype        NOT NULL,
    first_time_old      timestamp without time zone,
    last_time_old       timestamp without time zone,
    new_type            vlantype        DEFAULT NULL,
    acknowledged        boolean         DEFAULT false
);
CREATE INDEX port_vlan_logs_date_of_index ON port_vlan_logs (date_of);
CREATE INDEX port_vlan_logs_port_id       ON port_vlan_logs (port_id);
CREATE INDEX port_vlan_logs_vlan_id       ON port_vlan_logs (vlan_id);
ALTER TABLE port_vlan_logs OWNER TO lanview2;

CREATE OR REPLACE VIEW named_port_vlans AS
    SELECT port_vlan_id, node_id, node_name, port_id, port_name, vlan_id, vlan_name, vlan_type 
    FROM port_vlans JOIN vlans USING(vlan_id) JOIN nports USING(port_id) JOIN nodes USING(node_id);

CREATE OR REPLACE FUNCTION delete_port_post() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM port_params       WHERE port_id = OLD.port_id;
    DELETE FROM host_services     WHERE port_id = OLD.port_id;
    DELETE FROM port_vlans        WHERE port_id = OLD.port_id;
    DELETE FROM port_vlan_logs    WHERE port_id = OLD.port_id;
    DELETE FROM mactab            WHERE port_id = OLD.port_id;
    DELETE FROM phs_links_table   WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    DELETE FROM lldp_links_table  WHERE port_id1 = OLD.port_id OR port_id2 = OLD.port_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION update_port_vlan(pid bigint, vid bigint, vt vlantype, st settype DEFAULT 'query'::settype) RETURNS reasons AS $$
DECLARE
    rec port_vlans;
    r reasons;
BEGIN
    SELECT * INTO rec FROM port_vlans WHERE port_id = pid AND vlan_id = vid;
    IF NOT FOUND THEN
        IF 0 = COUNT(*) FROM vlans WHERE vlan_id = vid THEN
            INSERT INTO vlans(vlan_id, vlan_name) VALUES(vid, 'AUTO_INSERTED_VLAN' || vid::text);
            r := 'new';
        ELSE
            r := 'insert';
        END IF;
        INSERT INTO port_vlans (port_id, vlan_id, vlan_type, set_type, flag) VALUES (pid, vid, vt, st, true);
        RETURN r;
    END IF;
    IF rec.vlan_type = vt THEN
        UPDATE port_vlans SET last_time = now(), flag = true WHERE port_id = pid AND vlan_id = vid;
        RETURN 'unchange';
    END IF;
    UPDATE port_vlans SET vlan_type = vt, set_type = st, first_time = now(), last_time = now(), flag = true WHERE port_id = pid AND vlan_id = vid;
    INSERT INTO port_vlan_logs(reason, port_id, vlan_id,   old_type, first_time_old, last_time_old, new_type)
                        VALUES('modify', pid,    vid, rec.vlan_type, rec.first_time, rec.last_time, vt);
    RETURN 'modify';
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION rm_unmarked_port_vlan(nid bigint) RETURNS integer AS $$
DECLARE
    rec port_vlans;
    n integer;
BEGIN
    n := 0;
    FOR rec IN SELECT port_vlans.* FROM port_vlans JOIN nports USING(port_id) WHERE port_vlans.flag = false AND node_id = nid LOOP
        n := n + 1;
        INSERT INTO port_vlan_logs(reason,       port_id,     vlan_id,      old_type, first_time_old, last_time_old)
                            VALUES('remove', rec.port_id, rec.vlan_id, rec.vlan_type, rec.first_time, rec.last_time);
        DELETE FROM port_vlans WHERE port_vlan_id = rec.port_vlan_id;
        
    END LOOP;
    RETURN n;
END;
$$ LANGUAGE plpgsql;

INSERT INTO services(service_name, superior_service_mask, check_cmd, features,
                     max_check_attempts, normal_check_interval, flapping_interval, flapping_max_change)
            VALUES  ('portvlan', 'lv2d', 'portvlan $S -R $host_service_id', ':process=continue:timing=passive:superior:logrot=500M,8:method=inspector:',
                     1, '00:05:00', '00:30:00' , 5);
INSERT INTO services(service_name, superior_service_mask, features,
                     max_check_attempts, normal_check_interval, retry_check_interval, flapping_interval, flapping_max_change)
            VALUES  ('pvlan', 'portvlan', ':timing=timed:delay=1000:', 3, '00:30:00', '00:05:00', '03:00:00', 4 );
            
ALTER TABLE patchs ADD COLUMN location point DEFAULT NULL;

-- ************************************************* BUGFIX! *********************************************************************

INSERT INTO unusual_fkeys
  ( table_name,   column_name, unusual_fkeys_type, f_table_name, f_column_name, f_inherited_tables) VALUES
  ( 'port_vlans', 'port_id',   'owner',            'nports',     'port_id',     '{interfaces}');
  

-- *********************************************** BUGFIX! #2 *******************************************************************

-- Ellenőrzi, hogy a address (ip cím) mező rendben van-e
-- Nem ütközik más címmel
-- Ha csak a cím változott, és az új cím másik subnet, akkor kizárást dobott, a javított fg. modosítja a subnet_id -t.
CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
    ipa ip_addresses;
    nip boolean;
    snid bigint;
    cnt integer;
BEGIN
 -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím
    nip := NEW.ip_address_type IS NULL;
    IF nip THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) IS NOT NULL THEN
            NEW.ip_address_type := 'dynamic';
        ELSE
            NEW.ip_address_type := 'fixip';
        END IF;
    END IF;
    IF NEW.address IS NOT NULL THEN
        -- Nincs subnet (id), keresünk egyet
        IF NEW.subnet_id IS NULL AND NEW.ip_address_type <> 'external' THEN
            BEGIN
                SELECT subnet_id INTO STRICT NEW.subnet_id FROM subnets WHERE netaddr >> NEW.address;
                EXCEPTION
                    WHEN NO_DATA_FOUND THEN     -- nem találtunk
                        PERFORM error('NotFound', -1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
                    WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                        PERFORM error('Ambiguous',-1, 'subnet address for : ' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END;
            -- RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        -- Ha megadtuk a subnet id-t is, akkor a címnek benne kell lennie
        ELSIF NEW.ip_address_type <> 'external' THEN
            SELECT netaddr INTO n FROM subnets WHERE subnet_id = NEW.subnet_id;
            IF NOT FOUND THEN
                PERFORM error('InvRef', NEW.subnet_id, 'subnet_id', 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
            IF NOT n >> NEW.address AND TG_OP = 'UPDATE' THEN   -- Update: csak az IP változott, subnet javítása, ha kell
                IF NEW.subnet_id = OLD.subnet_id AND NEW.address <> OLD.address THEN
                    SELECT subnet_id INTO snid FROM subnets WHERE netaddr >> NEW.address;
                    GET DIAGNOSTICS cnt = ROW_COUNT;
                    IF cnt = 1 THEN
                        NEW.subnet_id = snid;
                        SELECT netaddr INTO n FROM subnets WHERE subnet_id = snid;
                    END IF;
                END IF;
            END IF;
            IF NOT n >> NEW.address THEN
                PERFORM error('InvalidNAddr', NEW.subnet_id, CAST(n AS TEXT) || '>>' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        ELSE
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha nem private, akkor vizsgáljuk az ütközéseket
        IF NEW.address IS NOT NULL AND NEW.ip_address_type <> 'private' THEN
            -- Azonos IP címek?
            FOR ipa IN SELECT * FROM ip_addresses WHERE NEW.address = address LOOP
                IF ipa.ip_address_id <> NEW.ip_address_id THEN 
                    IF ipa.ip_address_type = 'dynamic' THEN
                    -- Ütköző dinamikust töröljük.
                        UPDATE ip_addresses SET address = NULL WHERE ip_address_id = ipa.ip_address_id;
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
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Valamiért voltak cím ütközések, a VIEW segít kideríteni (sajnos az okát nem)
CREATE OR REPLACE VIEW ip_address_cols AS SELECT address, ip_address_type, port_id2full_name(port_id) FROM ip_addresses AS ipa
       WHERE address IS NOT NULL AND 1 <> (SELECT COUNT(*) FROM ip_addresses AS ict WHERE ict.address = ipa.address);
  
CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    noipa   boolean := false;
    joint   boolean := false;
    oip     ip_addresses;
    net     cidr;
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
                RAISE INFO 'No ip_addresses record.';
            WHEN TOO_MANY_ROWS THEN
		RAISE INFO 'More ip_addresses record.';
                IF 0 < COUNT(*) FROM ip_addresses WHERE address = ipa AND ip_address_type <> 'joint'::addresstype THEN
                    PERFORM error('DataError', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses');
                END IF;
                RAISE INFO 'ip_addresses records is all join.';
                joint := true;
    END;
    IF joint THEN    -- Check ip_addresses record if joint
        IF 0 = COUNT(*) FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa AND hwaddress = hwa THEN
	    -- not found, fake MAC ?
	    IF 0 = COUNT(*) FROM ip_addresses JOIN port_params USING(port_id) JOIN param_types USING(param_type_id) WHERE param_type_name = 'override_mac' AND address = ipa AND cast_to_mac(param_value) = hwa THEN
		t := 'IP address (join) missing : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
		 || 'The existing IP address record ports names : '
		 || ARRAY(SELECT ' ' || port_id2full_name(port_id) || ' / ' || hwaddress || ' '  FROM ip_addresses JOIN interfaces USING(port_id) WHERE address = ipa)::text;
		RAISE WARNING 'Ticket : %', t; 
		PERFORM ticket_alarm('critical', t, hsi);
	    END IF;
        END IF;
    ELSIF stp = 'config' THEN  -- We assume this is fixip
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
                    RAISE WARNING 'Ticket : %', t; 
                    PERFORM ticket_alarm('critical', t, hsi);
                END;
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
                RAISE WARNING 'Ticket : %', t; 
                PERFORM ticket_alarm('critical', t, hsi);
            END IF;
        END IF;
    ELSE    -- stp <> 'config' ( = 'query')
        IF NOT noipa THEN
	    RAISE INFO 'Address record (%) : % - %', stp, ipa, port_id2full_name(oip.port_id);
	    IF hwa <> hwaddress FROM interfaces WHERE port_id = oip.port_id THEN
		IF oip.ip_address_type = 'dynamic' THEN     -- Clear old dynamic IP
		    RAISE INFO 'Delete address (%) : % - %', stp, port_id2full_name(oip.port_id), ipa;
		    UPDATE ip_addresses SET address = NULL WHERE ip_address_id = oip.ip_address_id;
		    noipa := true;
		ELSE
		    t := 'IP address collision : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
		      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id);
		    RAISE WARNING 'Ticket : %', t; 
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
		    t := 'IP address update error : ' || ipa::text || ' -- ' || hwa::text || ' type is ' || stp || '. '
		      || 'The existing IP address record port name : ' || port_id2full_name(oip.port_id)
		      || ' . Message : ' || msg || ' Detail : ' || det || ' Hint : ' hnt;
		    RAISE WARNING 'Ticket : %', t; 
		    PERFORM ticket_alarm('critical', t, hsi);
		END;
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

-- *********************************************** END BUGFIX! *******************************************************************
  
  
SELECT set_db_version(1, 12);