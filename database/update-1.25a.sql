-- BugFix : is_dyn_addr() is not boolean !
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
                    IF ipa.ip_address_type = 'dynamic' OR is_dyn_addr(NEW.address) IS NOT NULL THEN
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
