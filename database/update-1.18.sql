-- Service var can be disable

ALTER TABLE service_vars ADD  COLUMN disabled boolean NOT NULL DEFAULT false;
DROP VIEW portvars;
CREATE VIEW portvars AS 
 SELECT
    sv.service_var_id AS portvar_id,
    sv.service_var_name,
    sv.service_var_note,
    sv.service_var_type_id,
    sv.host_service_id,
    hs.port_id,
    sv.delegate_port_state,
    sv.service_var_value,
    sv.var_state,
    sv.last_time,
    sv.features,
    sv.raw_value,
    sv.delegate_service_state,
    sv.state_msg,
    sv.disabled
   FROM service_vars AS sv
     JOIN host_services AS hs USING (host_service_id)
   WHERE NOT sv.deleted AND NOT hs.deleted AND hs.port_id IS NOT NULL;
ALTER TABLE service_vars DROP COLUMN IF EXISTS rrd_beat_id;

-- --------------------------------------------------------

-- Feleslegesen lassítások törlése ...

CREATE OR REPLACE FUNCTION check_shared(portshare, portshare) RETURNS boolean AS $$
BEGIN
    IF $1 = 'NC' OR $2 = 'NC' THEN
        RETURN NULL;
    END IF;
    IF $1 = '' OR $2 = '' THEN
        RETURN FALSE;
    END IF;
    RETURN ( $1 = 'A'  AND ($2 = 'B' OR $2 = 'BA' OR $2 = 'BB'))
        OR ( $1 = 'AA' AND ($2 = 'B' OR $2 = 'BA' OR $2 = 'BB' OR $2 = 'AB' OR $2 = 'C' OR $2 = 'D'))
        OR ( $1 = 'AB' AND ($2 = 'B' OR $2 = 'BA' OR $2 = 'BB' OR $2 = 'AA'))
        OR ( $1 = 'B'  AND ($2 = 'A' OR $2 = 'AA' OR $2 = 'AB'))
        OR ( $1 = 'BA' AND ($2 = 'A' OR $2 = 'AA' OR $2 = 'AB' OR $2 = 'BB'))
        OR ( $1 = 'BB' AND ($2 = 'A' OR $2 = 'AA' OR $2 = 'AB' OR $2 = 'BA' OR $2 = 'C' OR $2 = 'D'))
        OR ( $1 = 'C'  AND ($2 = 'D' OR $2 = 'AA' OR $2 = 'BB'))
        OR ( $1 = 'D'  AND ($2 = 'C' OR $2 = 'AA' OR $2 = 'BB'));
END;
$$ LANGUAGE plpgsql IMMUTABLE;
COMMENT ON FUNCTION check_shared(portshare, portshare) IS
'Megvizsgálja, hogy a megadott két megosztás típus átfedi-e egymást,ha nem akkor igazzal tér vissza.
Ha bármelyik paraméter NC, akkor NULL-lal tér vissza.';

CREATE OR REPLACE FUNCTION dow2int(dayofweek) RETURNS integer AS $$
BEGIN
    CASE $1
        WHEN 'sunday'    THEN  RETURN 0;
        WHEN 'monday'    THEN  RETURN 1;
        WHEN 'tuesday'   THEN  RETURN 2;
        WHEN 'wednesday' THEN  RETURN 3;
        WHEN 'thursday'  THEN  RETURN 4;
        WHEN 'friday'    THEN  RETURN 5;
        WHEN 'saturday'  THEN  RETURN 6;
        ELSE                   RETURN NULL;
    END CASE;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

-- Hét napjainak a nevét numerikus kóddá alakítja
CREATE OR REPLACE FUNCTION int2dow(integer) RETURNS dayofweek AS $$
BEGIN
    CASE $1
        WHEN 0  THEN  RETURN 'sunday';
        WHEN 1  THEN  RETURN 'monday';
        WHEN 2  THEN  RETURN 'tuesday';
        WHEN 3  THEN  RETURN 'wednesday';
        WHEN 4  THEN  RETURN 'thursday';
        WHEN 5  THEN  RETURN 'friday';
        WHEN 6  THEN  RETURN 'saturday';
        ELSE          RETURN NULL;
    END CASE;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION is_noalarm(flg noalarmtype, tm_from timestamp, tm_to timestamp, tm timestamp DEFAULT CURRENT_TIMESTAMP) RETURNS isnoalarm AS $$
BEGIN
    CASE
        WHEN  flg = 'off'                                       THEN RETURN  'off';      -- Nincs tiltás
        WHEN  flg = 'on'                                        THEN RETURN  'on';      -- Időkorlát nélküli tiltás
        WHEN (flg = 'to' OR flg = 'from_to') AND tm >  tm_to    THEN RETURN  'expired'; -- Már lejárt
        WHEN  flg = 'to'                     AND tm <= tm_to    THEN RETURN  'on';      -- Tiltás
        WHEN                                     tm >= tm_from  THEN RETURN  'on';      -- Tiltás
        WHEN                                     tm <  tm_from  THEN RETURN  'off';     -- Még nem lépett életbe
    END CASE;
END
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION next_dow(dayofweek) RETURNS dayofweek AS $$
DECLARE
    dows  dayofweek[];
BEGIN
    dows = enum_range($1);
    IF array_length(dows, 1) > 1 THEN
        RETURN dows[1];
    ELSE
        RETURN enum_first($1);
    END IF;
END;
$$ LANGUAGE plpgsql IMMUTABLE;

CREATE OR REPLACE FUNCTION public.xor(boolean, boolean) RETURNS boolean AS $$
BEGIN
    RETURN ( $1 and not $2) or ( not $1 and $2);
END
$$ LANGUAGE plpgsql IMMUTABLE;


- ------------------------------------------------------------------------------------------------
-- Under construction !!!
-- SELECT set_db_version(1, 17);
