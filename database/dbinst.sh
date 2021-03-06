#!/bin/bash
if [ -z $1 ]
  then
    echo "usage: $0 <sql script file name>"
    exit
fi

PASSWD="password"
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
    TEMPLATE template0
    CONNECTION LIMIT = -1;

\q
END

echo Execute $1 script...
su $MYNAME -s /bin/bash -c "psql -f $1"

