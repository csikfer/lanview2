p -- Hibajavítás

-- Nem lehet törölni sunet-et
CREATE OR REPLACE FUNCTION subnet_delete_before() RETURNS TRIGGER AS $$
BEGIN
    UPDATE ip_addresses SET subnet_id = NULL, address = NULL WHERE subnet_id = OLD.subnet_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER subnet_delete_beforeí_trigger BEFORE DELETE ON subnets FOR EACH ROW EXECUTE PROCEDURE subnet_delete_before();

-- A VLAN-ok szintén törölhetetlenek
CREATE OR REPLACE FUNCTION vlan_delete_before() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM subnets WHERE vlan_id = OLD.vlan_id;
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER vlan_delete_before_trigger BEFORE DELETE ON vlans FOR EACH ROW EXECUTE PROCEDURE vlan_delete_before();

-- A is_dyn_addr() hibát dobott, ha volt exclude tartomány
CREATE OR REPLACE FUNCTION is_dyn_addr(inet) RETURNS bigint AS $$
DECLARE
    id  bigint;
BEGIN
    SELECT dyn_addr_range_id INTO id FROM dyn_addr_ranges WHERE exclude = false AND begin_address <= $1 AND $1 <= end_address;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    IF 0 < COUNT(*) FROM dyn_addr_ranges WHERE exclude = true  AND begin_address <= $1 AND $1 <= end_address THEN
        RETURN NULL;
    END IF;
    RETURN id;
END;
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION is_dyn_addr(inet) IS 'Ellenőrzi, hogy a paraméterként megadott IP cím része-e egy dinamikus IP tartománynak';

-- Az lldp_link rekord törlése, ha egy azonos log_link rekordot törlünk.
CREATE OR REPLACE FUNCTION delete_lldp_link() RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM lldp_links_table
        WHERE (port_id1 = OLD.port_id1 AND port_id2 = OLD.port_id2)
           OR (port_id1 = OLD.port_id2 AND port_id2 = OLD.port_id1);
    RETURN OLD;
END;
$$ LANGUAGE plpgsql;
CREATE TRIGGER delete_log_links_table BEFORE DELETE ON log_links_table FOR EACH ROW EXECUTE PROCEDURE delete_lldp_link();

-- Egy VIEW a mactab_logs megjelenítéséhez
CREATE OR REPLACE FUNCTION mac2node_name(mac macaddr) RETURNS text AS $$
DECLARE
    nn text;
BEGIN
    SELECT node_name INTO nn 
        FROM interfaces
        JOIN nodes USING(node_Id)
        WHERE hwaddress = mac
        LIMIT 1;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    RETURN nn;
END;
$$ LANGUAGE plpgsql;

-- Az adatbázis hiba rekordban nem jelenik meg az user neve, csak az ID, mert hiányzik az fkey def.
ALTER TABLE db_errs ALTER COLUMN user_id DROP NOT NULL;
ALTER TABLE db_errs ADD CONSTRAINT db_errs_user_id_fkey FOREIGN KEY (user_id)
      REFERENCES public.users (user_id) MATCH SIMPLE
      ON UPDATE RESTRICT ON DELETE SET NULL;

-- Melyik címtáblában van a port

CREATE OR REPLACE VIEW port_in_mactab AS
    SELECT
        i.port_id AS port_id,
        n.node_id AS node_id,
        i.port_name AS port_name,
        n.node_name || ':' || i.port_name AS port_full_name,
        m.port_id AS mactab_port_id,
        port_id2full_name(m.port_id) AS mactab_port_full_name,
        m.mactab_state,
        m.first_time,
        m.last_time,
        m.set_type
    FROM mactab     AS m
    JOIN interfaces AS i USING(hwaddress)
    JOIN nodes      AS n USING(node_id);
    
    
-- Hibajavítás (2018.08.09.)

CREATE OR REPLACE FUNCTION check_ip_address() RETURNS TRIGGER AS $$
DECLARE
    n   cidr;
    ipa ip_addresses;
    nip boolean;    -- IP address type IS NULL
    snid bigint;
    cnt integer;
BEGIN
    RAISE INFO 'check_ip_address() %/% NEW = %',TG_TABLE_NAME, TG_OP , NEW;
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
            RAISE INFO 'Clear subnet id : %', NEW.subnet_id;
            NEW.subnet_id := NULL;  -- Clear invalid subnet_id
        END IF;
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
            RAISE INFO 'Set subnet id : %', NEW.subnet_id;
        ELSIF NEW.ip_address_type = 'external'  THEN
            -- external típusnál mindíg NULL a subnet_id
            NEW.subnet_id := NULL;
        END IF;
        -- Ha nem private, akkor vizsgáljuk az ütközéseket
        IF NEW.ip_address_type <> 'private' THEN
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
