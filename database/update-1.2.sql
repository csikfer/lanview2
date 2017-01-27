BEGIN;

-- Version 1.1 ==> 1.2

-- RENAME patchable_port TO patchable_port_id
CREATE OR REPLACE VIEW patchable_ports AS
    SELECT 
        port_id AS patchable_port_id,
        port_name,
        port_note,
        port_tag,
        iftype_id,
        node_id,
        port_index,
        deleted
    FROM nports JOIN iftypes USING(iftype_id)
        WHERE iftype_link_type IN ('ptp', 'bus');

ALTER TABLE patchs ADD COLUMN inventory_number text DEFAULT NULL;
ALTER TABLE patchs ADD COLUMN serial_number    text DEFAULT NULL;

INSERT INTO errors
    ( error_name,     error_type, error_note) VALUES
    ( 'FieldNotUni',  'Error',    'Field is not unique ');

CREATE OR REPLACE FUNCTION node_check_before_insert() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
BEGIN
--  RAISE INFO 'Insert NODE, new id = %', NEW.node_id;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_id = NEW.node_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    SELECT COUNT(*) INTO n FROM patchs WHERE node_name = NEW.node_name;
    IF n > 0 THEN
        PERFORM error('NameNotUni', -1, 'node_name = ' || NEW.node_name, 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF NEW.serial_number IS NOT NULL THEN
        SELECT COUNT(*) INTO n FROM patchs WHERE serial_number = NEW.serial_number;
        IF n > 0 THEN
            PERFORM error('FieldNotUni', -1, 'serial_number = ' || NEW.seial_number, 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    IF NEW.inventory_number IS NOT NULL THEN
        SELECT COUNT(*) INTO n FROM patchs WHERE inventory_number = NEW.inventory_number;
        IF n > 0 THEN
            PERFORM error('FieldNotUni', -1, 'inventory_number = ' || NEW.inventory_number, 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    CASE TG_TABLE_NAME
        WHEN 'patchs' THEN
            NEW.node_type = '{patch}';
        WHEN 'nodes' THEN
            IF NEW.node_type IS NULL THEN
                NEW.node_type = '{node}';
            END IF;
        WHEN 'snmpdevices' THEN
            IF NEW.node_type IS NULL THEN
                NEW.node_type = '{host,snmp}';
            END IF;
        ELSE
            PERFORM error('DataError', NEW.node_id, 'node_id', 'node_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END CASE;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- restrict_modfy_port_id_before_update() --> node_check_before_update()
DROP FUNCTION IF EXISTS restrict_modfy_port_id_before_update() CASCADE;

CREATE OR REPLACE FUNCTION node_check_before_update() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
BEGIN
    IF NEW.node_id <> OLD.node_id THEN
        PERFORM error('Constant', NEW.node_id, 'node_id', 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF NEW.node_name <> OLD.node_name THEN
        SELECT COUNT(*) INTO n FROM patchs WHERE node_name = NEW.node_name;
        IF n > 0 THEN
            PERFORM error('NameNotUni', NEW.node_id, 'node_name = ' || NEW.node_name, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    IF NEW.serial_number IS NOT NULL AND  NEW.serial_number <> OLD.serial_number THEN
        SELECT COUNT(*) INTO n FROM patchs WHERE serial_number = NEW.serial_number;
        IF n > 0 THEN
            PERFORM error('NameNotUni', NEW.node_id, 'serial_number = ' || NEW.serial_number, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    IF NEW.inventory_number IS NOT NULL AND  NEW.inventory_number <> OLD.inventory_number THEN
        SELECT COUNT(*) INTO n FROM patchs WHERE inventory_number = NEW.inventory_number;
        IF n > 0 THEN
            PERFORM error('NameNotUni', NEW.node_id, 'inventory_number = ' || NEW.inventory_number, 'node_check_before_update()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER patchs_check_before_update      BEFORE UPDATE ON patchs      FOR EACH ROW EXECUTE PROCEDURE node_check_before_update();
CREATE TRIGGER nodes_check_before_update       BEFORE UPDATE ON nodes       FOR EACH ROW EXECUTE PROCEDURE node_check_before_update();
CREATE TRIGGER snmpdevices_check_before_update BEFORE UPDATE ON snmpdevices FOR EACH ROW EXECUTE PROCEDURE node_check_before_update();

UPDATE patchs
    SET inventory_number = node_params.param_value
    FROM node_params JOIN param_types USING(param_type_id)
    WHERE param_type_name = 'inventory_number' AND patchs.node_id = node_params.node_id 
--  RETURNING *
;

DELETE FROM node_params WHERE param_type_id = param_type_name2id('inventory_number');

UPDATE patchs
    SET serial_number = node_params.param_value
    FROM node_params JOIN param_types USING(param_type_id)
    WHERE param_type_name = 'serial_number' AND patchs.node_id = node_params.node_id 
--  RETURNING *
;

DELETE FROM node_params WHERE param_type_id = param_type_name2id('serial_number');

-- ----
CREATE OR REPLACE FUNCTION iftype_name2id(itn text) RETURNS bigint AS $$
DECLARE
    n bigint;
BEGIN
    BEGIN
        SELECT iftype_id INTO n FROM iftypes WHERE iftype_name = itn;
        EXCEPTION
            WHEN NO_DATA_FOUND THEN     -- nem találtunk
                PERFORM error('NameNotFound', -1, 'iftype_name = ' || itn, 'iftype_name2id(itn text)', 'iftypes', NULL);
    END;
    RETURN n;
END;
$$ LANGUAGE plpgsql;

-- check_port_id_before_insert() => port_check_before_insert()
DROP FUNCTION IF EXISTS check_port_id_before_insert() CASCADE;
CREATE OR REPLACE FUNCTION port_check_before_insert() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
    t text;
BEGIN
    SELECT COUNT(*) INTO n FROM nports WHERE port_id = NEW.port_id;
    IF n > 0 THEN
        PERFORM error('IdNotUni', NEW.port_id, 'port_id', 'port_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF 'pports' = TG_TABLE_NAME THEN
        NEW.iftype_id := iftype_name2id('patch');
    ELSE
        IF NEW.iftype_id = iftype_name2id('patch') THEN
            PERFORM error('DataError', NEW.iftype_id, 'iftype_id', 'port_check_before_insert()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    SELECT COUNT(*) INTO n FROM nports WHERE port_name = NEW.port_name AND node_id = NEW.node_id;
    IF n > 0 THEN
        PERFORM error('NameNotUni', -1, 'port_name', 'port_check_before_insert()', TG_TABLE_NAME, TG_OP);
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- restrict_modfy_port_id_before_update() => port_check_before_update()
DROP FUNCTION IF EXISTS restrict_modfy_port_id_before_update();
CREATE OR REPLACE FUNCTION port_check_before_update() RETURNS TRIGGER AS $$
DECLARE
    n bigint;
BEGIN
    IF NEW.port_id <> OLD.port_id THEN
        PERFORM error('Constant', -1, 'port_id', 'check_port_id_before_update()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF ('pports' = TG_TABLE_NAME) <> (NEW.iftype_id = iftype_name2id('patch')) THEN
        PERFORM error('DataError', NEW.iftype_id, 'iftype_id', 'port_check_before_update()', TG_TABLE_NAME, TG_OP);
    END IF;
    IF NEW.port_name <> OLD.port_name THEN
        SELECT COUNT(*) INTO n FROM nports WHERE port_name = NEW.port_name AND node_id = NEW.node_id;
        IF n > 0 THEN
            PERFORM error('NameNotUni', NEW.port_id, 'port_name = ' || NEW.ports_name, 'port_check_before_update()', TG_TABLE_NAME, TG_OP);
        END IF;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER nports_check_before_insert       BEFORE INSERT ON nports      FOR EACH ROW EXECUTE PROCEDURE port_check_before_insert();
CREATE TRIGGER pports_check_before_insert       BEFORE INSERT ON pports      FOR EACH ROW EXECUTE PROCEDURE port_check_before_insert();
CREATE TRIGGER interfaces_check_before_insert   BEFORE INSERT ON interfaces  FOR EACH ROW EXECUTE PROCEDURE port_check_before_insert();
CREATE TRIGGER nports_check_before_update       BEFORE UPDATE ON nports      FOR EACH ROW EXECUTE PROCEDURE port_check_before_update();
CREATE TRIGGER pports_check_before_update       BEFORE UPDATE ON pports      FOR EACH ROW EXECUTE PROCEDURE port_check_before_update();
CREATE TRIGGER interfaces_check_before_update   BEFORE UPDATE ON interfaces  FOR EACH ROW EXECUTE PROCEDURE port_check_before_update();

-- SET database version: 1.2
SELECT set_db_version(1, 2);

ALTER TABLE mactab     ADD CONSTRAINT mactab_check_mac     CHECK (NOT (hwaddress = '00:00:00:00:00:00' OR hwaddress = 'ff:ff:ff:ff:ff:ff'));
ALTER TABLE arps       ADD CONSTRAINT arps_check_mac       CHECK (NOT (hwaddress = '00:00:00:00:00:00' OR hwaddress = 'ff:ff:ff:ff:ff:ff'));
ALTER TABLE interfaces ADD CONSTRAINT interfaces_check_mac CHECK (NOT (hwaddress = '00:00:00:00:00:00' OR hwaddress = 'ff:ff:ff:ff:ff:ff'));

END;
