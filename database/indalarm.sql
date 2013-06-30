-- views


-- View: indalarm_view_groups

-- DROP VIEW indalarm_view_groups;

CREATE OR REPLACE VIEW public.indalarm_view_groups(
    "Név",
    "Leírás",
    "jogkör",
    "Helyiség",
    group_id,
    place_id)
AS
  SELECT g.group_name AS "Név",
         g.group_note AS "Leírás",
         g.group_rights AS "Jogkör",
         CASE
           WHEN g.place_id IS NULL THEN 'Nincs helyiség beállítva' ::character
             varying
           ELSE p.place_name
         END AS "Helyiség",
         g.group_id,
         g.place_id
  FROM groups g
       LEFT JOIN places p ON g.place_id = p.place_id;
ALTER TABLE indalarm_view_groups
  OWNER TO lanview2;
COMMENT ON VIEW indalarm_view_groups
  IS 'IndAlarm kliens által használt nézet a csoportok lista megjelenítéséhez';


-- View: indalarm_view_users

-- DROP VIEW indalarm_view_users;

CREATE OR REPLACE VIEW public.indalarm_view_users(
    "Felhasználónév",
    "Név",
    "Engedélyezett",
    "Telefon",
    "Cím",
    "Helyiség",
    passwd,
    user_id,
    place_id,
    enabled)
AS
  SELECT u.user_name AS "Felhasználónév",
         u.user_note AS "Név",
         CASE
           WHEN u.enabled = true THEN 'Igen' ::text
           ELSE 'Nem' ::text
         END AS "Engedélyezett",
         u.tel AS "Telefon",
         u.addresses AS "Cím",
         CASE
           WHEN p.place_name IS NULL THEN 'Nincs helyiség beállítva' ::character
             varying
           ELSE p.place_name
         END AS "Helyiség",
         u.passwd,
         u.user_id,
         u.place_id,
         u.enabled
  FROM users u
       LEFT JOIN places p ON u.place_id = p.place_id;
ALTER TABLE indalarm_view_users
  OWNER TO lanview2;
COMMENT ON VIEW indalarm_view_users
  IS 'Listázza a felhasználókat';


-- View: indalarm_view_activealarms

-- DROP VIEW indalarm_view_activealarms;

CREATE OR REPLACE VIEW public.indalarm_view_activealarms(
    alarm,
    superior_alarm,
    ack_time,
    alarm_id,
    host_service_id,
    begin_time,
    end_time,
    event_note ,
    noalarm,
    host_service_note,
    host_service_alarm_msg,
    service_id,
    service_name,
    service_note,
    service_alarm_msg,
    node_id,
    node_name,
    node_note,
    node_alarm_msg,
    place_id,
    place_name,
    place_note,
    orig_image_id,
    parent_id,
    frame,
    tel,
    place_alarm_msg,
    image_data,
    image_type,
    parent_image_id)
AS
  SELECT (arec.place_name::text || '-' ::text) || arec.node_name::text AS alarm,
         arec.superior_alarm,
         arec.ack_time,
         arec.alarm_id,
         arec.host_service_id,
         arec.begin_time,
         arec.end_time,
         arec.event_note,
         arec.noalarm,
         arec.host_service_note,
         arec.host_service_alarm_msg,
         arec.service_id,
         arec.service_name,
         arec.service_note,
         arec.service_alarm_msg,
         arec.node_id,
         arec.node_name,
         arec.node_note,
         arec.node_alarm_msg,
         arec.place_id,
         arec.place_name,
         arec.place_note,
         arec.image_id AS orig_image_id,
         arec.parent_id,
         arec.frame,
         arec.tel,
         arec.place_alarm_msg,
         ii.image_data,
         ii.image_type,
         (
           SELECT pa.image_id
           FROM places pa
           WHERE pa.place_id = arec.parent_id
         ) AS parent_image_id
  FROM (alarms
       LEFT JOIN host_services hs USING (host_service_id)
       LEFT JOIN services s USING (service_id)
       LEFT JOIN nodes n USING (node_id)
       LEFT JOIN places p USING (place_id)) arec
       LEFT JOIN images ii ON ii.image_id =((
                                              SELECT pa.image_id
                                              FROM places pa
                                              WHERE pa.place_id = arec.parent_id
       ))
  WHERE arec.superior_alarm IS NULL AND
        arec.noalarm IS FALSE AND
        arec.ack_time IS NULL
  ORDER BY arec.begin_time DESC;
  
ALTER TABLE indalarm_view_activealarms
  OWNER TO lanview2;
COMMENT ON VIEW indalarm_view_activealarms
  IS 'Listázza az aktív riasztásokat';


-- View: indalarm_view_usertypes

-- DROP VIEW indalarm_view_usertypes;

CREATE OR REPLACE VIEW public.indalarm_view_usertypes(
    enum_schema,
    enum_name,
    enum_value)
AS
  SELECT n.nspname AS enum_schema,
         t.typname AS enum_name,
         e.enumlabel AS enum_value
  FROM pg_type t
       JOIN pg_enum e ON t.oid = e.enumtypid
       JOIN pg_namespace n ON n.oid = t.typnamespace;
ALTER TABLE indalarm_view_usertypes
  OWNER TO lanview2;
COMMENT ON VIEW indalarm_view_usertypes
  IS 'Lekérdezi az adatbázisban definiált típusokat azok értékeivel együtt';
  
-- View: public.indalarm_view_host_service_alarmtype_list

-- DROP VIEW public.indalarm_view_host_service_alarmtype_list;

CREATE OR REPLACE VIEW public.indalarm_view_host_service_alarmtype_list AS 
 SELECT pl.place_name, pl.place_note, n.node_name, n.node_note, 
        CASE
            WHEN hs.noalarm_flag = 'off'::noalarmtype THEN 'aktív'::text
            ELSE 'kikapcsolva'::text
        END AS noalarmtext, 
		hs.noalarm_from, hs.noalarm_to, 
		s.service_name, hs.host_service_id, 
		hs.host_service_note, hs.node_id, 
		hs.service_id, hs.port_id, 
		p.port_name, hs.delegate_host_state, 
		pl.place_id, hs.noalarm_flag
   FROM host_services hs
   LEFT JOIN nodes n USING (node_id)
   LEFT JOIN services s USING (service_id)
   LEFT JOIN nports p ON p.port_id = hs.port_id
   LEFT JOIN places pl USING (place_id)
  ORDER BY pl.place_name, n.node_name;

ALTER TABLE public.indalarm_view_host_service_alarmtype_list
  OWNER TO lanview2;
COMMENT ON VIEW public.indalarm_view_host_service_alarmtype_list
  IS 'A noalarm flag kezeléséhez lekérdezi a host_services táblát';

  
-- View: public.indalarm_view_group_users

-- DROP VIEW public.indalarm_view_group_users;

  
CREATE OR REPLACE VIEW public.indalarm_view_group_users(
    group_id,
    group_name,
    group_user_id,
    userid,
	group_rights)
AS
  SELECT g.group_id,
         g.group_name,
         gu.group_user_id,
         CASE
           WHEN gu.user_id IS NULL THEN (- 1)
           ELSE 
         (
           SELECT u.user_id
           FROM users u
           WHERE u.user_id = gu.user_id
         )
         END AS userid, g.group_rights 
  FROM groups g
       LEFT JOIN group_users gu USING (group_id)
  ORDER BY g.group_name;  
ALTER TABLE public.indalarm_view_group_users
  OWNER TO lanview2;
COMMENT ON VIEW public.indalarm_view_group_users
  IS 'Listázza a groups tábla tartalmát, plusz a group_user tábla értékeit, hogy meghatározható legyen, adott user mely csoportokhoz tartozik és melyekhez nem';

-- functions

-- Function: public.indalarm_set_alarm_acknowledged(userid integer, alarmid integer, note text)

-- DROP FUNCTION public.indalarm_set_alarm_acknowledged(userid integer, alarmid integer, note text);

  
CREATE OR REPLACE FUNCTION public.indalarm_set_alarm_acknowledged (
  userid integer,
  alarmid integer,
  note text
)
RETURNS boolean AS'
DECLARE 
    alarmrec alarms;
BEGIN
    SELECT * INTO alarmrec FROM alarms WHERE alarm_id = alarmid;
    IF NOT FOUND THEN
        RETURN FALSE;
    END IF;
    UPDATE alarms SET ack_time = now(), ack_user = userid, ack_msg = note WHERE alarm_id = alarmrec.alarm_id;
    RETURN TRUE;
END
'LANGUAGE 'plpgsql'
VOLATILE
CALLED ON NULL INPUT
SECURITY INVOKER
COST 100;
ALTER FUNCTION public.indalarm_set_alarm_acknowledged(userid integer, alarmid integer, note text)
  OWNER TO lanview2;

COMMENT ON FUNCTION public.indalarm_set_alarm_acknowledged(userid integer, alarmid integer, note text)
IS 'Bejegyzi az érintett alarm rekordba a notice_time és notice_user értékeket.';

-- Function: public.indalarm_set_alarm_noticed(userid integer, alarmid integer)

-- DROP FUNCTION public.indalarm_set_alarm_noticed(userid integer, alarmid integer);


CREATE OR REPLACE FUNCTION public.indalarm_set_alarm_noticed (
  userid integer,
  alarmid integer
)
RETURNS boolean AS'
DECLARE 
    alarmrec alarms;
BEGIN
    SELECT * INTO alarmrec FROM alarms WHERE alarm_id = alarmid;
    IF NOT FOUND THEN
        RETURN FALSE;
    END IF;
    UPDATE alarms SET notice_time = now(), notice_user = userid WHERE alarm_id = alarmrec.alarm_id;
    RETURN TRUE;
END
'LANGUAGE 'plpgsql'
VOLATILE
CALLED ON NULL INPUT
SECURITY INVOKER
COST 100;
ALTER FUNCTION public.indalarm_set_alarm_noticed(userid integer, alarmid integer)
  OWNER TO lanview2;

COMMENT ON FUNCTION public.indalarm_set_alarm_noticed(userid integer, alarmid integer)
IS 'Bejegyzi az érintett alarm rekordba a notice_time és notice_user értékeket.';


-- Function: indalarm_alarm_notification()

-- DROP FUNCTION indalarm_alarm_notification();

CREATE OR REPLACE FUNCTION indalarm_alarm_notification()
  RETURNS trigger AS
$BODY$
DECLARE 
    payload text;
BEGIN
    payload := CAST(NEW.alarm_id AS TEXT);
    PERFORM pg_notify('newalarm', payload);
    RETURN NULL;
END
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION indalarm_alarm_notification()
  OWNER TO lanview2;

--triggers

-- Trigger: indalarm_on_alarm_insert on alarms

-- DROP TRIGGER indalarm_on_alarm_insert ON alarms;
CREATE TRIGGER indalarm_on_alarm_insert
  AFTER INSERT 
  ON public.alarms FOR EACH ROW 
  EXECUTE PROCEDURE public.indalarm_alarm_notification();
  

-- Function: indalarm_update_password(integer, character varying)

-- DROP FUNCTION indalarm_update_password(integer, character varying);

CREATE OR REPLACE FUNCTION indalarm_update_password(uid integer, newpasswd character varying)
  RETURNS users AS
$BODY$
DECLARE 
    usr users;
BEGIN
    UPDATE users set passwd = crypt(newpasswd, gen_salt('md5')) WHERE user_id = uid
        RETURNING * INTO usr;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', uid, 'user_id', 'indalarm_update_password()', 'users');
    END IF;
    RETURN usr;
END
$BODY$
  LANGUAGE plpgsql VOLATILE
  COST 100;
ALTER FUNCTION indalarm_update_password(integer, character varying)
  OWNER TO lanview2;

COMMENT ON FUNCTION public.indalarm_update_password(uid integer, newpasswd character varying)
IS 'Módosítja a uid azonosítójú felhasználó jelszavát';

