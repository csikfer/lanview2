
ALTER TYPE vlantype ADD VALUE IF NOT EXISTS 'auth';
ALTER TYPE vlantype ADD VALUE IF NOT EXISTS 'unauth';

SELECT set_db_version(1, 27);
