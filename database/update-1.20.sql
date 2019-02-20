-- disable NULL states
UPDATE interfaces SET port_astat = 'unknown' WHERE port_astat IS NULL;
ALTER TABLE interfaces ALTER COLUMN port_astat SET DEFAULT 'unknown';
ALTER TABLE interfaces ALTER COLUMN port_astat SET NOT NULL;
ALTER TABLE interfaces ALTER COLUMN port_stat SET NOT NULL;

UPDATE nodes SET node_stat = 'unknown' WHERE node_stat IS NULL;
ALTER TABLE nodes ALTER COLUMN node_stat SET DEFAULT 'unknown';
ALTER TABLE nodes ALTER COLUMN node_stat SET NOT NULL;


CREATE OR REPLACE FUNCTION replace_mactab(
    pid bigint,     -- switch port ID
    mac macaddr,    -- MAC in addres table
    typ settype DEFAULT 'query',
    mst mactabstate[] DEFAULT '{}'
) RETURNS reasons AS $$
DECLARE
    mt       mactab;
BEGIN
    IF get_bool_port_param(pid, 'suspected_uplink') THEN
        RETURN 'discard';
    END IF;
    mst := current_mactab_stat(pid, mac, mst);
    SELECT * INTO mt FROM mactab WHERE hwaddress = mac;
    IF NOT FOUND THEN            -- NEW
        INSERT INTO mactab(hwaddress, port_id, mactab_state,set_type) VALUES (mac, pid, mst, typ);
        RETURN 'insert';
    ELSIF mt.port_id = pid THEN  -- No changed, refresh state
        RETURN mactab_changestat(mt, mst, typ, true);
    ELSIF get_bool_port_param(mt.port_id, 'suspected_uplink') THEN -- Move, old port is suspect: simple move
        PERFORM mactab_move(mt, pid, mac, typ, mst);
        RETURN 'move';
    ELSE                        -- Move, check suspected uplink
        DECLARE 
            mactab_move_check_count    CONSTANT integer  := get_int_sys_param('mactab_move_check_count');
            mactab_move_check_interval CONSTANT interval := get_interval_sys_param('mactab_move_check_interval');
            begin_time      timestamp;
            my_moved_cnt    integer;    -- moved my port
            an_moved_cnt    integer;    -- moved other port
        BEGIN
            begin_time := NOW - mactab_move_check_interval;
            SELECT COUNT(*) INTO my_moved_cnt FROM mactab_logs
                WHERE begin_time < date_of
                  AND (port_id_old = pid OR port_id_new = pid)
                  AND reason = 'move';
            IF mactab_move_check_count < my_moved_cnt THEN
                -- Suspect, which?
                SELECT COUNT(*) INTO an_moved_cnt FROM mactab_logs
                    WHERE begin_time < date_of
                      AND (port_id_old = mt.port_id OR port_id_new = mt.port_id)
                      AND reason = 'move';
                IF my_moved_cnt < an_moved_cnt THEN
                    -- another port is suspect
                    PERFORM set_bool_port_param(mt.port_id, true, 'suspected_uplink');
                    PERFORM mactab_move(mt, pid, mac, typ, mst);
                    UPDATE mactab_logs SET be_void = true
                        WHERE date_of > begin_time
                          AND (port_id_old = mt.port_id OR port_id_new = mt.port_id)
                          AND hwaddress = mac
                          AND reason = 'restore';
                    RETURN 'restore';
                ELSE
                    -- my port is suspect
                    PERFORM set_bool_port_param(pid, true, 'suspected_uplink');
                    UPDATE mactab_logs SET be_void = true
                        WHERE date_of > begin_time
                          AND (port_id_old = pid OR port_id_new = pid)
                          AND hwaddress = mac
                          AND reason = 'move';
                    RETURN 'discard';
                END IF;
            ELSE
                -- Symple move
                PERFORM mactab_move(mt, pid, mac, typ, mst);
                RETURN 'move';
            END IF;
        END;
    END IF;
END;
$$ LANGUAGE plpgsql;

-- ----------------------------

SELECT set_db_version(1, 20);
