-- Hibajavítás

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
    
    