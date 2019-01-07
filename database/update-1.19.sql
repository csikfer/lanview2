-- 

CREATE TYPE usability AS ENUM ('map', 'flag', 'icon');

ALTER TABLE images ADD COLUMN usabilityes usability[] NOT NULL DEFAULT '{}'::usability[];

-- ------------------------------------------------------------------------------------------------
SELECT set_db_version(1, 19);

