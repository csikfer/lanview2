-- //// LOC.PLACES

CREATE TYPE imagetype AS ENUM ('BMP', 'GIF', 'JPG', 'JPEG', 'PNG', 'PBM', 'PGM', 'PPM', 'XBM', 'XPM', 'BIN');

CREATE TABLE images (
    image_id        bigserial          PRIMARY KEY,
    image_name      varchar(32)     NOT NULL UNIQUE,
    image_note      text    DEFAULT NULL,
    image_type      imagetype       NOT NULL DEFAULT 'PNG',
    image_sub_type  varchar(32)     DEFAULT NULL,
    image_data      bytea           NOT NULL,
    image_hash      bytea           NOT NULL
);
ALTER TABLE images OWNER TO lanview2;
COMMENT ON TABLE  images             IS 'Bináris adatokat, tipikusan képeket tartalmazó tábla';
COMMENT ON COLUMN images.image_id    IS 'A kép ill bináris adathamaz egyedi azonosítója';
COMMENT ON COLUMN images.image_name  IS 'A kép ill bináris adathamaz egyedi neve';
COMMENT ON COLUMN images.image_note  IS 'A kép ill bináris adathamaz opcionális megjegyzés, leírás';
COMMENT ON COLUMN images.image_type  IS 'A kép ill bináris adathamaz típusa, a kezelt kép formátumok, vagy BIN';
COMMENT ON COLUMN images.image_sub_type IS 'Bináris adathamaz esetén (image_type = BIN) egy opcionális típus azonosító string';
COMMENT ON COLUMN images.image_data  IS 'A kép ill bináris adathamaz.';
COMMENT ON COLUMN images.image_hash  IS 'A kép ill bináris adathamaz SHA512 HASH. NULL esetén a trigger függvény autómatikusan meggenerálja';

CREATE OR REPLACE FUNCTION set_image_hash_if_NULL() RETURNS TRIGGER AS $$
BEGIN
    IF NEW.image_hash IS NULL THEN
        NEW.image_hash := digest(NEW.image_data, 'sha512');
    END IF;
    RETURN NEW;
END
$$ LANGUAGE plpgsql;

CREATE TRIGGER set_image_hash_if_NULL BEFORE INSERT OR UPDATE ON images FOR EACH ROW EXECUTE PROCEDURE set_image_hash_if_NULL();

CREATE TYPE placetype AS ENUM ('root', 'unknown', 'real');
COMMENT ON TYPE placetype IS '
A places rekord típus
''root''	Mindenki őse (Az unknown típusnak, és önmagának nem)
''unknown''    Ha nem ismert
''real''       Valódi hely ill helyiség
';
ALTER TYPE placetype OWNER TO lanview2;

CREATE TABLE places (
    place_id    bigserial           PRIMARY KEY,
    place_name  varchar(32)         NOT NULL UNIQUE,
    place_note  text        DEFAULT NULL,
    place_type  placetype           DEFAULT 'real',
    parent_id   bigint              DEFAULT NULL REFERENCES places(place_id) MATCH SIMPLE
                                        ON DELETE RESTRICT ON UPDATE RESTRICT,
    image_id    bigint              DEFAULT NULL REFERENCES images(image_id) MATCH SIMPLE
                                        ON DELETE SET NULL ON UPDATE RESTRICT,
    frame       polygon             DEFAULT NULL,
    tels        varchar(20)[]       DEFAULT NULL
);
ALTER TABLE places OWNER TO lanview2;
COMMENT ON TABLE  places            IS 'Helyiségek, helyek leírói a térképen ill. alaprajzon';
COMMENT ON COLUMN places.place_id   IS 'A térkép ill. alaprajz azososítója ID';
COMMENT ON COLUMN places.place_name IS 'A térkép ill. alaprajz azososítója név';
COMMENT ON COLUMN places.place_note IS '';
COMMENT ON COLUMN places.image_id   IS 'Opcionális térkép/alaprajz kép ID';
COMMENT ON COLUMN places.frame      IS 'Határoló poligon/pozició a szülőn';
COMMENT ON COLUMN places.tels       IS 'Telefonszámok a helyiségben';
COMMENT ON COLUMN places.parent_id  IS 'A térkép ill. alaprajz szülő, ha nincs szülő, akkor NULL';

-- //// LOC.PLACE_GROUPS

CREATE TABLE place_groups (
    place_group_id      bigserial       PRIMARY KEY,
    place_group_name    varchar(32)     NOT NULL UNIQUE,
    place_group_note    text            DEFAULT NULL
);
ALTER TABLE place_groups OWNER TO lanview2;
COMMENT ON TABLE  place_groups                      IS 'Helyiségek, helyek csoportjai';
COMMENT ON COLUMN place_groups.place_group_id       IS 'Csoport azososítója ID';
COMMENT ON COLUMN place_groups.place_group_name     IS 'Csoport azososító neve';
COMMENT ON COLUMN place_groups.place_group_note     IS 'Megjegyzés';

-- //// LOC.PLACE_GROUPS_MEMBERS

CREATE TABLE place_group_places (
    place_group_place_id   bigserial  PRIMARY KEY,
    place_group_id         bigint REFERENCES place_groups(place_group_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    place_id               bigint REFERENCES places(place_id)             MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    UNIQUE(place_group_id, place_id)
);
ALTER TABLE place_group_places OWNER TO lanview2;

-- ///

INSERT INTO place_groups(place_group_id, place_group_name, place_group_note) VALUES
    ( 0, 'none',    'No any location'   ),
    ( 1, 'all',     'All locations'     );
-- Inkrementálunk egyet, mert PostgreSql egy barom
SELECT nextval('place_groups_place_group_id_seq');

-- Ez a RULE nem működik,
-- CREATE RULE add_place_to_all_group   AS ON INSERT TO places
--    DO INSERT INTO place_group_members (place_group_id, place_id) VALUES ( 1, NEW.place_id);
-- Helyette van ez a trigger függvény
-- Létrehoz egy kapcsoló tábla rekordot, egy group ás tag között.
-- Vagyis egy megadott objektumot betesz egy megadott csoportba
-- Paraméterek (sorrendben):
-- $table   tábla név, A kapcsoló tábla neve
-- $midname a member azonosító (id) neve a kapcsoló, és a member táblában (feltételezzük, hogy azonos)
-- $gidname a group azonosító (id) neve a kapcsoló táblában
-- $gid     a group azonisítója (id)
CREATE OR REPLACE FUNCTION add_member_to_all_group() RETURNS TRIGGER AS $$
    ($table, $midname, $gidname, $gid) = @{$_TD->{args}};
    $mid = $_TD->{new}->{$midname};
    spi_exec_query("INSERT INTO $table ($gidname, $midname) VALUES ( $gid, $mid)");
    return;
$$ LANGUAGE plperl;

-- Minden új place bekerül az all nevű place_group -ba
CREATE TRIGGER add_place_to_all_group AFTER INSERT ON places
    FOR EACH ROW EXECUTE PROCEDURE add_member_to_all_group('place_group_places', 'place_id', 'place_group_id', 1);

INSERT INTO places(place_id, place_name, place_note, place_type, parent_id) VALUES
    ( 0, 'unknown', 'unknown pseudo location',  'unknown',  NULL),
    ( 1, 'root',    'root pseudo location',     'root',     NULL);
-- Inkrementálunk egyet, mert PostgreSql egy barom (az első új rekord ütközni fog enélkül)
SELECT nextval('places_place_id_seq');

-- A megadott hely névhez visszaadja a hely ID-t
-- Ha nincs ilyen nevű hely akkor dob egy kizárást.
CREATE OR REPLACE FUNCTION place_name2id(varchar(32)) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT place_id INTO id FROM places WHERE place_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'place_name2id()', 'places');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;
CREATE OR REPLACE FUNCTION is_parent_place(idr bigint, idq bigint) RETURNS boolean AS $$
DECLARE
    n integer;
    id bigint := idr;
BEGIN
    n := 0;
    LOOP
        IF id = idq THEN
            RETURN TRUE;
        END IF;
        n := n + 1;
        IF N > 10 THEN
            PERFORM error('Loop', idr, '', 'is_parent_place()', 'places');
        END IF;
        SELECT parent_id INTO id FROM places WHERE place_id = id;
        IF NOT FOUND OR idr IS NULL THEN
            RETURN FALSE;
        END IF;
    END LOOP;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION is_parent_place(bigint, bigint) IS
'Lekérdezi, hogy az idr azonosítójú places rekord parentje-e az idq-azonosítójúnak.';

CREATE OR REPLACE FUNCTION get_parent_image(idr bigint) RETURNS bigint AS $$
DECLARE
    n integer;
    pid bigint := idr;
    iid bigint;
BEGIN
    SELECT parent_id INTO pid FROM places WHERE place_id = pid;
    IF NOT FOUND THEN
        RETURN NULL;
    END IF;
    n := 0;
    LOOP
        n := n + 1;
        IF N > 16 THEN
            PERFORM error('Loop', idr, '', 'is_parent_place()', 'places');
        END IF;
        SELECT parent_id, image_id INTO pid, iid FROM places WHERE place_id = pid;
        IF NOT FOUND THEN
            RETURN NULL;
        END IF;
        IF iid IS NOT NULL THEN
            RETURN iid;
        END IF;
    END LOOP;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION get_parent_image(bigint) IS
'Lekéri a legközelebbi parent image_id-jét. Addig megy a gyökér felé, amíg nem NULL értéket talál,
vagy nincs több parent.';

CREATE OR REPLACE FUNCTION get_image(pid bigint) RETURNS bigint AS $$
DECLARE
    iid bigint;
BEGIN
    SELECT image_id INTO iid FROM places WHERE place_id = pid;
    IF iid IS NOT NULL THEN
        RETURN iid;
    END IF;
    RETURN get_parent_image(pid);
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION get_image(bigint) IS
'Lekéri a rekord vagy a legközelebbi parent image_id-jét. Addig megy a gyökér felé, amíg nem NULL értéket talál,
vagy nincs több parent.';
    

CREATE OR REPLACE FUNCTION is_group_place(idr bigint, idq bigint) RETURNS boolean AS $$
DECLARE
    n integer;
BEGIN
    SELECT COUNT(*) INTO n FROM place_group_places WHERE place_group_id = idq AND (place_id = idr OR is_parent_place(idr, place_id));
    RETURN n > 0;
END
$$ LANGUAGE plpgsql;
COMMENT ON FUNCTION is_group_place(bigint, bigint) IS
'Lekérdezi, hogy az idr azonosítójú places tagja-e az idq-azonosítójú place_groups csoportnak,
vagy valamelyik parentje tag-e';
