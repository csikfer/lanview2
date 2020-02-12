
ALTER TABLE interfaces ADD COLUMN delegate_oper_stat  boolean DEFAULT FALSE;
ALTER TABLE interfaces ADD COLUMN delegate_admin_stat boolean DEFAULT TRUE;
COMMENT ON COLUMN interfaces.delegate_oper_stat  IS 'Delegate port op. state to portvars service';
COMMENT ON COLUMN interfaces.delegate_admin_stat IS 'Delegate port admin state to portvars service';


SELECT set_db_version(1, 26);
