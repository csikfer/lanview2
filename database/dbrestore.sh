if [ -z $1 ]
  then
    echo "usage: $0 <sql dump file name>"
    exit
fi

PASSWD="edcrfv+11"
MYNAME="lanview2"

adduser --disabled-password --gecos "$MYNAME pseudo user" $MYNAME

su postgres -c psql <<END

CREATE EXTENSION IF NOT EXISTS adminpack;

DROP DATABASE IF EXISTS $MYNAME;
DROP ROLE     IF EXISTS $MYNAME;

CREATE ROLE $MYNAME
    LOGIN PASSWORD '$PASSWD'
    SUPERUSER CREATEDB CREATEROLE
    VALID UNTIL 'infinity';

CREATE DATABASE $MYNAME
    WITH OWNER = $MYNAME
    ENCODING = 'UTF8'
    LC_COLLATE = 'hu_HU.UTF-8'
    LC_CTYPE = 'hu_HU.UTF-8'
    CONNECTION LIMIT = -1;

\q
END

echo Execute $1 script...
su $MYNAME -c "pg_restore -d $MYNAME $1"

