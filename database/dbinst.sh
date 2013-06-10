if [ -z $1 ]
  then
    echo "usage: $0 <sql script file name>"
    exit
fi

adduser --disabled-password --gecos "LanView2 pseudo user" lanview2

LV2HOME=/home/lanview2
PASSWD="edcrfv+11"
PGSHAREDIR=`pg_config --sharedir`


su postgres -c psql <<END

DROP DATABASE IF EXISTS lanview2;

DROP ROLE IF EXISTS lanview2;

CREATE ROLE lanview2
    LOGIN PASSWORD '$PASSWD'
    SUPERUSER CREATEDB CREATEROLE
    VALID UNTIL 'infinity';


CREATE DATABASE lanview2
    WITH OWNER = lanview2
    ENCODING = 'UTF8'
    LC_COLLATE = 'hu_HU.UTF-8'
    LC_CTYPE = 'hu_HU.UTF-8'
    CONNECTION LIMIT = -1;

\q
END

su lanview2 -c psql <<END

DROP PROCEDURAL LANGUAGE IF EXISTS plperl  CASCADE;
DROP PROCEDURAL LANGUAGE IF EXISTS plpgsql CASCADE;
CREATE LANGUAGE plpgsql;
CREATE LANGUAGE plperl;

END

echo "Contrib: dblink..."
su lanview2 -c "psql -f $PGSHAREDIR/contrib/dblink.sql"
# echo "Contrib: pgcrypto..."
# su lanview2 -c "psql -f $PGSHAREDIR/contrib/pgcrypto.sql"

echo Execute $1 script...
su lanview2 -c "psql -f $1"

