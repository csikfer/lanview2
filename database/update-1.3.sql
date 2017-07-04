BEGIN;  -- Version 1.2 ==> 1.3

-- nodes.sql

-- A tábla neve (vagy a mezők elnevezése) nem követte a szabályokat, és a program nem minden mezőt ismert fel
-- Az 'ip_address' bázis név -> 'ip_addresses' táblanév miatt a szabályokat is büvíteni kell:
-- nem egy 's' , hanem 'es' utótagot kapott 
ALTER TABLE ipaddresses RENAME TO ip_addresses;

CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
BEGIN
 -- RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
    -- Az új rekordban van ip cím
    IF NEW.ip_address_type IS NULL THEN
        IF NEW.address IS NULL OR is_dyn_addr(NEW.address) THEN
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
            IF NOT n >> NEW.address THEN
                PERFORM error('InvalidNAddr', NEW.subnet_id, CAST(n AS TEXT) || '>>' || CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
        ELSE
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha van már ilyen fix vagy pseudo ip cím, az baj, de privat címneknél nincs ütközés
        IF NEW.address IS NOT NULL AND NEW.ip_address_type <> 'private' THEN
            -- Töröljük az azonos dinamikus címmeketm ha van
            IF NEW.ip_address_type = 'dynamic' THEN
                UPDATE ip_addresses SET address = NULL WHERE address = NEW.address AND ip_address_type = 'dynamic' AND ip_address_id <> NEW.ip_address_id;
            -- minden egyébb ütközés hiba
            ELSIF 0 < COUNT(*) FROM ip_addresses WHERE
                         ( ip_address_type = 'fixip' OR ip_address_type = 'pseudo') AND
                         address = NEW.address AND ip_address_id <> NEW.ip_address_id THEN
                PERFORM error('IdNotUni', n, CAST(NEW.address AS TEXT), 'check_ip_address()', TG_TABLE_NAME, TG_OP);
            END IF;
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



-- LanView.sql

UPDATE fkey_types SET table_name = 'ip_addresses' WHERE table_name = 'ipaddresses';

-- mactab.sql

CREATE OR REPLACE VIEW arps_shape AS
    SELECT
        ipaddress,
        hwaddress,
        set_type,
        host_service_id,
        first_time,
        last_time,
        arp_note,
        port_id2full_name(ip.port_id) AS port_by_ipa,
        first_node_id2name(array_agg(ih.node_id)) AS node_by_hwa,
        array_agg(port_id2name(ih.port_id)) AS ports_by_hwa
    FROM arps AS a
    LEFT OUTER JOIN interfaces  AS ih USING(hwaddress) -- many
    LEFT OUTER JOIN ip_addresses AS ip ON ip.address = a.ipaddress
    GROUP BY ipaddress, hwaddress, set_type, host_service_id, first_time, last_time, arp_note, ip.port_id
;

CREATE OR REPLACE FUNCTION replace_arp(ipa inet, hwa macaddr, stp settype DEFAULT 'query', hsi bigint DEFAULT NULL) RETURNS reasons AS $$
DECLARE
    arp     arps;
    aid     bigint;
    oip     ip_addresses;
    net     cidr;
    sni     bigint;
BEGIN
    BEGIN       -- MAC -> IP check
        SELECT ip_address_id INTO STRICT aid FROM ip_addresses JOIN interfaces USING(port_id) WHERE
             ip_address_type = 'dynamic' AND hwaddress = hwa AND (address <> ipa OR address IS NULL);
        EXCEPTION
            WHEN NO_DATA_FOUND THEN
                aid := NULL;
            WHEN TOO_MANY_ROWS THEN
                PERFORM error('DataWarn', -1, 'address', 'replace_arp(' || ipa::text || ', ' || hwa::text || ')', 'ip_addresses JOIN interfaces');
                aid := NULL;
    END;
    IF aid IS NOT NULL THEN
        SELECT * INTO oip FROM ip_addresses WHERE ip_address_id = aid;     -- régi IP
        SELECT netaddr INTO net FROM subnets WHERE subnet_id = oip.subnet_id;
        IF net >> ipa THEN      -- Ha nem változott a subnet
            sni := oip.subnet_id;
        ELSE
            sni := NULL;
        END IF;
        UPDATE ip_addresses SET address = ipa, subnet_id = sni WHERE ip_address_id = aid;   -- új ip
        INSERT INTO dyn_ipaddress_logs(ipaddress_new, ipaddress_old, set_type, port_id)
                                VALUES (ipa, oip.address, stp, oip.port_id);
    END IF;
    SELECT * INTO arp FROM arps WHERE ipaddress = ipa;
    IF NOT FOUND THEN
        INSERT INTO arps(ipaddress, hwaddress,set_type, host_service_id) VALUES (ipa, hwa, stp, hsi);
        RETURN 'insert';
    ELSE
        IF arp.hwaddress = hwa THEN
	    IF arp.set_type < stp THEN
	        UPDATE arps SET set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP WHERE ipaddress = arp.ipaddress;
		RETURN 'update';
	    ELSE
	        UPDATE arps SET last_time = CURRENT_TIMESTAMP WHERE ipaddress = arp.ipaddress;
		RETURN 'found';
	    END IF;
        ELSE
            UPDATE arps
                SET hwaddress = hwa,  first_time = CURRENT_TIMESTAMP, set_type = stp, host_service_id = hsi, last_time = CURRENT_TIMESTAMP
                WHERE ipaddress = arp.ipaddress;
            INSERT INTO
                arp_logs(reason, ipaddress, hwaddress_new, hwaddress_old, set_type_old, host_service_id_old, first_time_old, last_time_old)
                VALUES( 'move',  ipa,       hwa,           arp.hwaddress, arp.set_type, arp.host_service_id, arp.first_time, arp.last_time);
            RETURN 'modify';
        END IF;
    END IF;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE VIEW mactab_shape AS
    SELECT
        n.node_id               AS node_id,
        n.node_name             AS node_name,
        t.port_id               AS port_id,
        p.port_name             AS port_name,
        t.hwaddress             AS hwaddress,
        t.mactab_state          AS mactab_state,
        t.first_time            AS first_time,
        t.last_time             AS last_time,
        t.state_updated_time    AS state_updated_time,
        t.set_type              AS set_type,
        r.node_name             AS r_node_name,
        port_id2name(i.port_id) AS r_port_name,
        -- A rendszer nem kezeli a cím tömboket
        ARRAY(SELECT ipaddress::text FROM arps         WHERE arps.hwaddress = t.hwaddress)    AS ipaddrs_by_arp,
        ARRAY(SELECT address::text   FROM ip_addresses WHERE ip_addresses.port_id = i.port_id) AS ipaddrs_by_rif
    FROM mactab AS t
      JOIN nports AS p USING (port_id)
      JOIN nodes  AS n USING (node_id)
      LEFT OUTER JOIN interfaces AS i USING(hwaddress)
      LEFT OUTER JOIN nodes      AS r ON r.node_id = i.node_id
      ;
      
CREATE OR REPLACE VIEW arps_shape AS
    SELECT
        ipaddress,
        hwaddress,
        set_type,
        host_service_id,
        first_time,
        last_time,
        arp_note,
        port_id2full_name(ip.port_id) AS port_by_ipa,
        first_node_id2name(array_agg(ih.node_id)) AS node_by_hwa,
        array_agg(port_id2name(ih.port_id)) AS ports_by_hwa
    FROM arps AS a
    LEFT OUTER JOIN interfaces   AS ih USING(hwaddress) -- many
    LEFT OUTER JOIN ip_addresses AS ip ON ip.address = a.ipaddress
    GROUP BY ipaddress, hwaddress, set_type, host_service_id, first_time, last_time, arp_note, ip.port_id
;

-- SET database version: 1.3
SELECT set_db_version(1, 3);

-- hibajavítás
ALTER TABLE phs_links_table ADD CONSTRAINT phs_links_table_create_user_id_fkey FOREIGN KEY (create_user_id)
      REFERENCES users(user_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
ALTER TABLE phs_links_table ADD CONSTRAINT phs_links_table_modify_user_id_fkey FOREIGN KEY (modify_user_id)
      REFERENCES users(user_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
      
-- felesleges, nem használtuk semmire:
ALTER TABLE services DROP COLUMN protocol_id;
DROP TABLE ipprotocols;

-- Plussz egy szűrés típus:
ALTER TYPE filtertype ADD VALUE 'boolean';

-- Enumerációs értékek - opcionális szín, font összerendelés

CREATE TYPE fontattr AS ENUM ('bold','italic', 'underline', 'strikeout');
ALTER TYPE fontattr OWNER TO lanview2;


ALTER TABLE enum_vals ADD COLUMN bg_color text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN fg_color text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN view_short text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN view_long text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN tool_tip text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN font_family text DEFAULT NULL;
ALTER TABLE enum_vals ADD COLUMN font_attr fontattr[] DEFAULT NULL;

CREATE TYPE datacharacter AS ENUM ('head','data', 'id', 'name', 'primary', 'key', 'fname', 'derived', 'tree', 'foreign', 'null', 'default', 'auto', 'info', 'warning', 'error', 'not_permit', 'have_no');
ALTER TYPE datacharacter OWNER TO lanview2;

-- change field flags set (enum array) type (remove: 'auto_set')
ALTER TYPE fieldflag RENAME TO fieldflag_old;

CREATE TYPE fieldflag AS ENUM ('table_hide', 'dialog_hide', 'read_only', 'passwd', 'huge', 'batch_edit',
                               'bg_color', 'fg_color', 'font', 'tool_tip');
ALTER TYPE fieldflag OWNER TO lanview2;
COMMENT ON TYPE fieldflag IS
'A mező tulajdonságok logokai (igen/nem):
table_hide      A táblázatos megjelenítésnél a mező rejtett
dialog_hide     A dialógusban (insert, modosít) a mező rejtett
read_only       A mező nem szerkeszthető
passwd          A mező egy jelszó (tartlma rejtett)
huge            A TEXT típusú mező több soros
batch_edit      A mező kötegelten modosítható
bg_color        Háttér szín beállítása enum_vals szerint.
fg_color        karakter szín beállítása enum_vals szerint.
font            Font beállítása enum_vals szerint.
tool_tip        Tool tip beállítása enum_vals szerint.
';

ALTER TABLE table_shape_fields ALTER COLUMN field_flags DROP DEFAULT;
ALTER TABLE table_shape_fields ALTER COLUMN field_flags TYPE fieldflag[]
    USING(array_remove(field_flags::text[], 'auto_set')::fieldflag[]);
ALTER TABLE table_shape_fields ALTER COLUMN field_flags SET DEFAULT '{}'::fieldflag[];
DROP TYPE fieldflag_old;

ALTER TABLE table_shapes ADD COLUMN style_sheet TEXT;

ALTER TABLE interfaces ADD COLUMN  ifmtu            integer;
ALTER TABLE interfaces ADD COLUMN  ifspeed          bigint;
ALTER TABLE interfaces ADD COLUMN  ifinoctets       bigint;
ALTER TABLE interfaces ADD COLUMN  ifinucastpkts    bigint;
ALTER TABLE interfaces ADD COLUMN  ifinnucastpkts   bigint;
ALTER TABLE interfaces ADD COLUMN  ifindiscards     bigint;
ALTER TABLE interfaces ADD COLUMN  ifinerrors       bigint;
ALTER TABLE interfaces ADD COLUMN  ifoutoctets      bigint;
ALTER TABLE interfaces ADD COLUMN  ifoutucastpkts   bigint;
ALTER TABLE interfaces ADD COLUMN  ifoutnucastpkts  bigint;
ALTER TABLE interfaces ADD COLUMN  ifoutdiscards    bigint;
ALTER TABLE interfaces ADD COLUMN  ifouterrors      bigint;
ALTER TABLE interfaces ADD COLUMN  ifdescr          text;
ALTER TABLE interfaces ADD COLUMN  stat_last_modify timestamp;

ALTER TABLE host_services ADD COLUMN flag boolean DEFAULT false;

-- ---

CREATE OR REPLACE FUNCTION current_mactab_stat(
    pid bigint,
    mac macaddr,        
    mst mactabstate[] DEFAULT '{}'
) RETURNS mactabstate[] AS $$
DECLARE
    ret mactabstate[] := '{}';
    suspect boolean;
BEGIN
    suspect := mst && ARRAY['suspect'::mactabstate];
    IF is_content_oui(mac) THEN
        ret = array_append(ret, 'oui'::mactabstate);
        suspect := false;
    END IF;
    IF 0 < COUNT(*) FROM interfaces WHERE hwaddress = mac THEN
        ret := array_append(ret, 'likely'::mactabstate);
        suspect := false;
        IF is_linked(pid, mac) THEN
            ret := array_append(ret, 'link'::mactabstate);
        END IF;
    END IF;
    IF is_content_arp(mac) THEN
        ret := array_append(ret, 'arp'::mactabstate);
    END IF;
    IF suspect THEN
        ret := array_append(ret, 'suspect'::mactabstate);
    END IF;
    RETURN ret;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION refresh_mactab() RETURNS integer AS $$
DECLARE
    mt  mactab;
    ret integer := 0;
BEGIN
    FOR mt IN SELECT * FROM mactab
        WHERE  state_updated_time < (now() - get_interval_sys_param('mactab_check_stat_interval'))
    LOOP
        IF 'update' = mactab_changestat(mt, current_mactab_stat(mt.port_id, mt.hwaddress, mt.mactab_state)) THEN
            ret := ret + 1;
        END IF;
    END LOOP;
    
    FOR mt IN SELECT * FROM mactab
        WHERE ( last_time < (now() - get_interval_sys_param('mactab_suspect_expire_interval'))  AND mactab_state && ARRAY['suspect']::mactabstate[] )
           OR ( last_time < (now() - get_interval_sys_param('mactab_expire_interval'))      AND NOT mactab_state && ARRAY['arp','likely']::mactabstate[] )
           OR ( last_time < (now() - get_interval_sys_param('mactab_reliable_expire_interval')))
    LOOP
        PERFORM mactab_remove(mt, 'expired');
        ret := ret +1;
    END LOOP;
    
    FOR mt IN SELECT DISTINCT(mactab.*)
	FROM mactab
	JOIN port_params USING(port_id)
	JOIN param_types USING(param_type_id)
	WHERE (param_type_name = 'query_mac_tab'    AND NOT param_value::boolean) 
	   OR (param_type_name = 'suspected_uplink' AND param_value::boolean) 
    LOOP
        PERFORM mactab_remove(mt, 'discard');
        ret := ret +1;
    END LOOP;
    
    FOR mt IN SELECT mactab.*
 	FROM mactab
	JOIN lldp_links ON port_id = port_id1
	WHERE NOT get_bool_port_param(port_id, 'query_mac_tab')
    LOOP
        PERFORM mactab_remove(mt, 'discard');
        ret := ret +1;
    END LOOP;
   
    RETURN ret;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION refresh_mactab() IS
'Frissíti a mactab tábla rekordokban a mactab_state mezőket.
Törli azokat a rekordokat a mactab táblából, melyeknál a last_time értéke túl régi.
A lejárati időintervallumokat a "mactab_suspect_expire_interval", "mactab_reliable_expire_interval"
és "mactab_expire_interval" rendszerváltozó tartalmazza.
Azt, hogy melyik lejárati idő érvényes, azt a rekord mactab_state mező értéke határozza meg.
A törlés oka "expired"lessz.
Szintén törl azokat a rekordokat, melyeknél a portra létezik a "query_mac_tab" paraméter hamis értékkel,
vagy a "suspected_uplink" paraméter igaz értékkel. Ebben az esetben a törlés oka "discard" lesz.
Végül azokat a rekordokat is törli, melyre portokra létezik lldp_links rekord, és nincs a portra
"query_mac_tab" paraméter igaz értékkel. Ebben az esetben is "discard" lesz a törlés oka.
Visszatérési érték a törölt rekordok száma. ';

-- Plussz szűrés típus, ahol kellhet a negált érték:
ALTER TYPE filtertype ADD VALUE 'notbegin';
ALTER TYPE filtertype ADD VALUE 'notlike';
ALTER TYPE filtertype ADD VALUE 'notsimilar';
ALTER TYPE filtertype ADD VALUE 'notregexp';
ALTER TYPE filtertype ADD VALUE 'notregexpi';
ALTER TYPE filtertype ADD VALUE 'notinterval';

-- Szűrés típusoknak nem kell külön tábla, elég egy tömb is
ALTER TABLE table_shape_fields ADD COLUMN filter_types filtertype[];
UPDATE table_shape_fields AS f
    SET filter_types = t.types 
FROM (SELECT table_shape_field_id AS id, array_agg(filter_type) AS types FROM table_shape_filters GROUP BY table_shape_field_id) AS t
    WHERE t.id = f.table_shape_field_id;
DROP TABLE table_shape_filters;
DELETE FROM table_shape WHERE table_name = 'table_shape_filters';
-- A table_shape_fields rekordban van egy nem szabályos távoli kulcs hivatkozás, a törölt rekord ID-re, azt is törölni kell!!

ALTER TABLE interfaces DROP COLUMN ifmtu;
ALTER TABLE interfaces DROP COLUMN ifspeed;
ALTER TABLE interfaces DROP COLUMN ifinoctets;
ALTER TABLE interfaces DROP COLUMN ifinucastpkts;
ALTER TABLE interfaces DROP COLUMN ifinnucastpkts;
ALTER TABLE interfaces DROP COLUMN ifindiscards;
ALTER TABLE interfaces DROP COLUMN ifinerrors;
ALTER TABLE interfaces DROP COLUMN ifoutoctets;
ALTER TABLE interfaces DROP COLUMN ifoutucastpkts;
ALTER TABLE interfaces DROP COLUMN ifoutnucastpkts;
ALTER TABLE interfaces DROP COLUMN ifoutdiscards;
ALTER TABLE interfaces DROP COLUMN ifouterrors;


DROP FUNCTION IF EXISTS is_group_place(bigint, text);
DROP FUNCTION IF EXISTS is_group_place(bigint, bigint);

CREATE OR REPLACE FUNCTION is_place_in_zone(idr bigint, idq bigint) RETURNS boolean AS $$
DECLARE
    n integer;
BEGIN
    CASE idq
        WHEN 0 THEN -- none
            RETURN FALSE;
        WHEN 1 THEN -- all
            RETURN TRUE;
        ELSE
            SELECT COUNT(*) INTO n FROM place_group_places WHERE place_group_id = idq AND (place_id = idr OR is_parent_place(idr, place_id));
            RETURN n > 0;
    END CASE;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION is_place_in_zone(bigint, bigint) IS
'Lekérdezi, hogy az idr azonosítójú places tagja-e az idq-azonosítójú place_groups zónának,
vagy valamelyik parentje tag-e';

-- Az online_alarms helyett külön VIEW a nyugtázott és nyugtázatlan riasztásoknak, a régi iszonyat lassú volt.

DROP VIEW IF EXISTS online_alarm_acks;
DROP VIEW IF EXISTS online_alarm_unacks;
DROP VIEW IF EXISTS online_alarms;
DELETE FROM unusual_fkeys WHERE table_name = 'online_alarms' OR table_name = 'online_alarms_ack' OR table_name = 'online_alarms_unack';

CREATE OR REPLACE VIEW online_alarms AS
        SELECT
         alarm_id, array_agg(user_id) AS online_user_ids
        FROM user_events
        JOIN alarms USING(alarm_id)
        WHERE event_type  =  'notice'
          AND event_state <> 'dropped'
     --   AND (end_time IS NULL OR end_time + (SELECT param_value FROM sys_params WHERE sys_param_name = 'user_notice_timeout')::interval > now())
        GROUP BY (alarm_id, begin_time)
        ORDER BY begin_time;

CREATE OR REPLACE VIEW online_alarm_unacks AS
    SELECT 
        alarm_id AS online_alarm_unack_id,
        host_service_id,
        host_service_id2name(host_service_id) AS host_service_name,
        node_name,
        place_name,
        place_id,
        superior_alarm_id,
        begin_time,
        end_time,
        first_status,
        max_status,
        last_status,
        event_note,
        alarm_message(host_service_id, max_status)      	AS msg,
        online_user_ids,
        (SELECT array_agg(user_id) FROM user_events WHERE alarm_id = alarms.alarm_id AND event_type = 'notice' AND user_id = ANY (online_user_ids) AND event_state = 'happened') AS notice_user_ids,
        (SELECT array_agg(user_id) FROM user_events WHERE alarm_id = alarms.alarm_id AND event_type = 'view'   AND user_id = ANY (online_user_ids)) AS view_user_ids
    FROM online_alarms
    JOIN alarms USING(alarm_id)
    JOIN host_services AS h USING (host_service_id)
    JOIN services      AS s USING(service_id)
    JOIN nodes         AS n USING(node_id)
    JOIN places        AS p USING(place_id)
    WHERE 0 = (SELECT COUNT(*) FROM user_events WHERE alarm_id = alarms.alarm_id AND event_type = 'acknowledge');
COMMENT ON VIEW online_alarm_unacks IS 'On-line nem nyugtázott riasztások';
    
CREATE OR REPLACE VIEW online_alarm_acks AS
    WITH oaa AS (
	SELECT
	    *,
            (SELECT array_agg(user_id) FROM user_events WHERE alarm_id = e.alarm_id AND user_id = ANY (online_user_ids) AND event_type = 'notice' AND event_state = 'happened') AS notice_user_ids,
            (SELECT array_agg(user_id) FROM user_events WHERE alarm_id = e.alarm_id AND user_id = ANY (online_user_ids) AND event_type = 'view')  AS view_user_ids,
            (SELECT array_agg(user_id) FROM user_events WHERE alarm_id = e.alarm_id AND event_type = 'acknowledge') AS ack_user_ids
        FROM online_alarms AS e
    )
    SELECT 
        alarm_id AS online_alarm_ack_id,
        host_service_id,
        host_service_id2name(host_service_id) AS host_service_name,
        node_name,
        place_name,
        place_id,
        superior_alarm_id,
        begin_time,
        first_status,
        max_status,
        last_status,
        event_note,
        alarm_message(host_service_id, max_status)      	AS msg,
        online_user_ids,
        notice_user_ids,
        view_user_ids,
        ack_user_ids,
        (SELECT string_agg(user_event_note, '\n') FROM user_events WHERE alarm_id = oaa.alarm_id AND event_type = 'acknowledge') AS ack_user_note
    FROM oaa
    JOIN alarms USING(alarm_id)
    JOIN host_services AS h USING (host_service_id)
    JOIN services      AS s USING(service_id)
    JOIN nodes         AS n USING(node_id)
    JOIN places        AS p USING(place_id)
    WHERE end_time IS NULL
      AND 0 < array_length(ack_user_ids, 1);
COMMENT ON VIEW online_alarm_acks IS 'On-line nyugtázott, még aktív riasztások';

INSERT INTO unusual_fkeys
  ( table_name,             column_name,        unusual_fkeys_type, f_table_name,   f_column_name) VALUES
  ( 'online_alarm_unacks', 'online_user_ids',    'property',       'users',        'user_id'),
  ( 'online_alarm_unacks', 'notice_user_ids',    'property',       'users',        'user_id'),
  ( 'online_alarm_unacks', 'view_user_ids',      'property',       'users',        'user_id'),
  ( 'online_alarm_unacks', 'superior_alarm_id',  'property',       'host_services','host_service_id'),
  ( 'online_alarm_acks',   'online_user_ids',    'property',       'users',        'user_id'),
  ( 'online_alarm_acks',   'notice_user_ids',    'property',       'users',        'user_id'),
  ( 'online_alarm_acks',   'view_user_ids',      'property',       'users',        'user_id'),
  ( 'online_alarm_acks',   'ack_user_ids',       'property',       'users',        'user_id'),
  ( 'online_alarm_acks',   'superior_alarm_id',  'property',       'host_services','host_service_id');

-- A nyugtázatlan ill. még nem kiértesített azonos szolgáltatáspéldányhoz tartozó user_event rekordok státuszát is dropped-re kell állítani.
-- Normál körülmények esetén ez felesleges, de ha nem megy a cron, akkor kellemetlenül felszaporodhatnak a felesleges riasztás értesítések.
CREATE OR REPLACE FUNCTION alarm_notice() RETURNS TRIGGER AS $$
DECLARE
    gids bigint[];
BEGIN
    IF NOT NEW.noalarm AND NEW.superior_alarm_id IS NULL THEN
        IF TG_OP = 'INSERT' THEN
            UPDATE user_events SET event_state = 'dropped'
                WHERE alarm_id IN ( SELECT alarm_id FROM alarms WHERE host_service_id = NEW.host_service_id)
                  AND event_state = 'necessary';
            SELECT COALESCE(online_group_ids, (SELECT online_group_ids FROM services WHERE service_id = host_services.service_id))
                    INTO gids FROM host_services WHERE host_service_id = NEW.host_service_id;
            IF gids IS NOT NULL THEN
                INSERT INTO user_events(user_id, alarm_id, event_type)
                    SELECT DISTINCT user_id, NEW.alarm_id, 'notice'::usereventtype   FROM group_users WHERE group_id = ANY (gids);
            END IF;
            SELECT COALESCE(offline_group_ids, (SELECT offline_group_ids FROM services WHERE service_id = host_services.service_id))
                    INTO gids FROM host_services WHERE host_service_id = NEW.host_service_id;
            IF gids IS NOT NULL THEN
                INSERT INTO user_events(user_id, alarm_id, event_type)
                    SELECT DISTINCT user_id, NEW.alarm_id, 'sendmail'::usereventtype FROM group_users WHERE group_id = ANY (gids);
            END IF;
        END IF;
        NOTIFY alarm;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

DROP TRIGGER IF EXISTS alarms_before_insert_or_update ON alarms;
DROP TRIGGER IF EXISTS alarms_after_insert_or_update ON alarms;
CREATE TRIGGER alarms_after_insert_or_update  AFTER UPDATE OR INSERT ON alarms  FOR EACH ROW EXECUTE PROCEDURE alarm_notice();

CREATE OR REPLACE FUNCTION expired_online_alarm(did bigint) RETURNS VOID AS $$
DECLARE
    expi interval;
BEGIN
    SELECT param_value::interval INTO expi FROM sys_params WHERE sys_param_name = 'user_notice_timeout';
    UPDATE user_events SET event_state = 'dropped'
        WHERE event_state = 'necessary' AND (created + expi) < NOW();
END
$$ LANGUAGE plpgsql;

-- A host_service_logs bővítése 

ALTER TYPE reasons ADD VALUE 'unknown';
ALTER TYPE reasons ADD VALUE 'close' BEFORE 'unknown';
ALTER TYPE reasons ADD VALUE 'timeout' BEFORE 'unknown';
ALTER TABLE host_service_logs ADD COLUMN alarm_id bigint
    DEFAULT NULL
        REFERENCES alarms(alarm_id) MATCH SIMPLE ON UPDATE RESTRICT ON DELETE SET NULL;
ALTER TABLE host_service_logs ADD COLUMN alarm_do reasons NOT NULL DEFAULT 'unknown';

CREATE OR REPLACE FUNCTION set_service_stat(
    hsid        bigint,             -- A host_services rekord id-je
    state       notifswitch,        -- Az új státusz
    note        text DEFAULT '',    -- Az eseményhez tartozó üzenet (opcionális)
    dmid        bigint DEFAULT NULL,-- Daemon host_service_id
    forced      boolean DEFAULT false)  -- Hiba esetén azonnali statusz állítás (nem számolgat)
RETURNS host_services AS $$
DECLARE
    hs          host_services;  -- Az új host_services rekord
    old_hs      host_services;  -- A régi
    s           services;       -- A hozzá tartozó services rekord
    na          isnoalarm;      -- Alarm tiltás állpota
    sup         bigint;
    tflapp      interval;       -- Flapping figyelés időablakának a hossza
    iflapp      integer;        -- Az isőablakon bellüli státuszváltozások száma
    alid        bigint;
    aldo        reasons := 'unknown';
BEGIN
    hs := touch_host_service(hsid);
    SELECT * INTO  s FROM services WHERE service_id = hs.service_id;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', hs.service_id, 'service_id', 'set_service_stat()', 'services');
    END IF;
    IF state = hs.host_service_state AND state = hs.hard_state AND state = hs.soft_state THEN   -- Ha nincs változás
        RETURN hs;
    END IF;
    old_hs := hs;
    alid := old_hs.act_alarm_log_id;
    CASE state
        WHEN 'on','recovered' THEN
            hs.host_service_state := state;
            hs.hard_state := 'on';
            hs.soft_state := 'on';
            hs.check_attempts := 0;
        WHEN 'warning', 'unreachable','down','unknown','critical' THEN
            IF hs.hard_state <> 'on' THEN   -- nem most rommlott el
                hs.hard_state := state;
                hs.soft_state := state;
                hs.host_service_state := state;
                hs.check_attempts := 0;
            ELSE                            -- most vagy nem rég romlott el, hihető?
                IF hs.soft_state = 'on' THEN
                    hs.check_attempts := 1; -- pont most lett rossz, kezdünk számolni
                ELSE
                    hs.check_attempts := hs.check_attempts + 1; -- tovább számolunk
                END IF;
                hs.soft_state := state;
                IF hs.max_check_attempts IS NULL THEN
                    IF s.max_check_attempts IS NULL THEN
                        PERFORM error('WNotFound', hs.host_service_id, 'max_check_attempts', 'set_service_stat()');
                        hs.max_check_attempts := 1;
                    ELSE
                        hs.max_check_attempts := s.max_check_attempts;
                    END IF;
                END IF;
                IF forced OR hs.check_attempts >= hs.max_check_attempts THEN  -- Elhisszük, hogy baj van
                    hs.hard_state := state;
                    hs.host_service_state := state;
                    hs.check_attempts := 0;
                END IF;
            END IF;
        ELSE
            PERFORM error('Params', state, 'notifswitch', 'set_service_stat()');    -- kilép!
    END CASE;
    tflapp := COALESCE(hs.flapping_interval, s.flapping_interval);
    iflapp := COALESCE(hs.flapping_max_change, s.flapping_max_change);
    IF chk_flapping(hsid, tflapp, iflapp) THEN
        hs.host_service_state := 'flapping';
    END IF;
    PERFORM set_host_status(hs);
    sup := check_superior_service(hs);
    na := is_noalarm(hs.noalarm_flag, hs.noalarm_from, hs.noalarm_to);
    IF na = 'expired' THEN  -- Lejárt, töröljük a noalarm attributumokat, ne zavarjon
        hs.noalarm_flag := 'off';
        hs.noalarm_from := NULL;
        hs.noalarm_to   := NULL;
        na := 'off';
    END IF;
    IF hs.last_changed IS NULL OR old_hs.host_service_state <> hs.host_service_state THEN
        hs.last_changed = CURRENT_TIMESTAMP;
    END IF;
    -- Alarm
    CASE
        -- Riasztás lezárása
        WHEN hs.host_service_state = 'recovered' THEN
            IF hs.act_alarm_log_id IS NULL THEN
                -- PERFORM error('DataWarn', hsid, 'act_alarm_log_id', 'set_service_stat()', 'host_services');
                RAISE INFO 'Status is recovered and alarm record not set.';
            ELSE
                RAISE INFO 'Close alarms record';
                UPDATE alarms SET
                        end_time = CURRENT_TIMESTAMP
                    WHERE alarm_id = hs.act_alarm_log_id AND end_time IS NULL;
                IF NOT FOUND THEN
                    PERFORM error('DataWarn', hs.act_alarm_log_id, 'end_time', 'set_service_stat()', 'alarms');
                ELSE
                    hs.last_alarm_log_id := hs.last_alarm_log_id;
                END IF;
                hs.act_alarm_log_id := NULL;
            END IF;
            aldo := 'close';
        -- Új riasztás,
        WHEN (old_hs.host_service_state < 'warning' OR old_hs.host_service_state = 'unknown')
         AND hs.host_service_state >= 'warning' THEN
            RAISE INFO 'New alarms record';
            INSERT INTO alarms (host_service_id, daemon_id, first_status, max_status, last_status, event_note, superior_alarm_id, noalarm)
                VALUES(hsid, dmid, hs.host_service_state, hs.host_service_state, hs.host_service_state, note, sup, na = 'on' )
                RETURNING alarm_id INTO hs.act_alarm_log_id;
            aldo := 'new';
            alid := hs.act_alarm_log_id;
        -- A helyzet fokozódik
        WHEN old_hs.host_service_state < hs.host_service_state OR old_hs.host_service_state = 'unknown' THEN
        RAISE INFO 'Update alarms record Up';
            UPDATE alarms SET
                    max_status  = hs.host_service_state,
                    last_status = hs.host_service_state,
                    noalarm     = (na = 'on'),
                    superior_alarm_id = sup
                WHERE alarm_id = hs.act_alarm_log_id;
            aldo := 'modify';
        -- Alacsonyabb szint, de marad a riasztás
        WHEN old_hs.host_service_state < hs.host_service_state THEN
            RAISE INFO 'Update alarms record Down';
            UPDATE alarms SET
                    last_status = hs.host_service_state,
                    noalarm     = (na = 'on'),
                    superior_alarm_id = sup
                WHERE alarm_id = hs.act_alarm_log_id;
            aldo := 'modify';
        ELSE
            RAISE INFO 'No mod alarms, old_hs = %, hs = %' ,old_hs, hs;
            aldo := 'unchange';
    END CASE;
    UPDATE host_services SET
            max_check_attempts = hs.max_check_attempts,
            host_service_state = hs.host_service_state,
            hard_state         = hs.hard_state,
            soft_state         = hs.soft_state,
            state_msg          = note,
            check_attempts     = hs.check_attempts,
            last_changed       = hs.last_changed,
            act_alarm_log_id   = hs.act_alarm_log_id,
            last_alarm_log_id  = hs.last_alarm_log_id,
            noalarm_flag       = hs.noalarm_flag,
            noalarm_from       = hs.noalarm_from,
            noalarm_to         = hs.noalarm_to
        WHERE host_service_id  = hsid;
    -- RAISE INFO '/ set_service_stat() = %', hs;
    -- Create log
    IF hs.host_service_state <> old_hs.host_service_state OR
       hs.hard_state         <> old_hs.hard_state OR
       hs.soft_state         <> old_hs.soft_state THEN
        INSERT INTO host_service_logs(host_service_id, old_state, old_soft_state, old_hard_state,
                           new_state, new_soft_state, new_hard_state, event_note, superior_alarm_id, noalarm,
                           alarm_id, alarm_do)
            VALUES  (hsid, old_hs.host_service_state, old_hs.soft_state, old_hs.hard_state,
                           hs.host_service_state, hs.soft_state, hs.hard_state, note, sup, na = 'on',
                           alid, aldo);
    END IF;
    RETURN hs;
END
$$ LANGUAGE plpgsql;

-- Néhány lekérdezés nagyon lassú, mert nincsenek indexek...
CREATE INDEX pports_node_id_index       ON pports(node_id);
CREATE INDEX nports_node_id_index       ON nports(node_id);
CREATE INDEX interfaces_node_id_index   ON interfaces(node_id);
CREATE INDEX host_services_superior_index ON host_services(superior_host_service_id);
CREATE INDEX ip_addresses_port_id_index ON ip_addresses(port_id);
CREATE INDEX mactab_port_id_index       ON mactab(port_id);
CREATE INDEX node_params_node_id_index  ON node_params(node_id);
CREATE INDEX port_params_port_id_index  ON port_params(port_id);
CREATE INDEX port_vlans_port_id_index   ON port_vlans(port_id);
CREATE INDEX table_shape_fields_shape_index ON table_shape_fields(table_shape_id);

ALTER TABLE service_vars ADD COLUMN state_msg text;

CREATE OR REPLACE VIEW portvars AS 
 SELECT
    service_vars.service_var_id AS portvar_id,
    service_vars.service_var_name,
    service_vars.service_var_note,
    service_vars.service_var_type_id,
    service_vars.host_service_id,
    host_services.port_id,
    service_vars.rrd_beat_id,
    service_vars.service_var_value,
    service_vars.var_state,
    service_vars.last_time,
    service_vars.features,
    service_vars.raw_value,
    service_vars.delegate_service_state,
    service_vars.state_msg
   FROM service_vars
     JOIN host_services USING (host_service_id)
   WHERE NOT service_vars.deleted;

INSERT INTO unusual_fkeys
  ( table_name, column_name,           unusual_fkeys_type, f_table_name,       f_column_name, f_inherited_tables) VALUES
  ( 'portvars', 'port_id',             'owner',            'nports',           'port_id',     '{nports, interfaces}');
INSERT INTO unusual_fkeys
  ( table_name, column_name,           unusual_fkeys_type, f_table_name,        f_column_name) VALUES
  ( 'portvars', 'service_var_type_id', 'property',         'service_var_types', 'service_var_type_id'),
  ( 'portvars', 'host_service_id',     'property',         'host_services',     'host_service_id');
  
