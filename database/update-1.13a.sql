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