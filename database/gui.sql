
DROP TABLE IF EXISTS table_shape_filters, table_shape_fields, table_shapes, enum_vals, id_view_procs CASCADE;
DROP TYPE  IF EXISTS tableshapetype, filtertype, filterdatatype, ordtype, tableinherittype;

CREATE TYPE tableshapetype AS ENUM ('no', 'simple', 'tree', 'owner', 'child', 'switch', 'link');
ALTER TYPE tableshapetype OWNER TO lanview2;
COMMENT ON TYPE tableshapetype IS
'table shape típusa:
no          Csak dialógus
simple      Egyszerű tála.
tree        Fa struktúrájú objektumok
owner       A tábla egy másik tábla tulajdonosa (pl a ''nodes'' az ''nports'' -nak)
child       A tábla egy owner táblához tartozik
switch      Kapcsoló tábla
link        Link tábla';

CREATE TYPE tableinherittype AS ENUM ('no', 'only', 'on', 'all', 'reverse', 'listed', 'listed_rev', 'listed_all');
ALTER TYPE tableinherittype OWNER TO lanview2;
COMMENT ON TYPE tableinherittype IS
'A tábla leszármazottainak és őseinek a kezelése
no          Nincs leszármazott
only        Vannak leszármazottak, de azokat nem jelenítjük meg
on          A leszármazottakat is megjelenítése
all         Az ősök és leszármazottak megjelenítése
reverse     Az ősöket is meg kell jeleníteni
listed      Csak a felsorolt leszármazottak megjelenítése
listed_rev  Csak a felsorolt ősök megjelenítése
listed_all  Csak a felsorolt leszármazottak és ősök megjelenítése';

CREATE TABLE table_shapes (
    table_shape_id      serial          PRIMARY KEY,
    table_shape_name    varchar(32)     NOT NULL UNIQUE,
    table_shape_note   varchar(255)    DEFAULT NULL,
    table_shape_title   varchar(255)    DEFAULT NULL,
    table_shape_type    tableshapetype[] DEFAULT '{simple}',
    table_name          varchar(32)     NOT NULL,
    schema_name         varchar(32)     NOT NULL DEFAULT 'public',
    table_inherit_type tableinherittype DEFAULT 'no',
    inherit_table_names varchar(32)[]   DEFAULT NULL,
    is_read_only        boolean         DEFAULT 'f',
    refine              varchar(64)     DEFAULT NULL,
    properties          varchar(255)    DEFAULT NULL,
    left_shape_id       integer         DEFAULT NULL REFERENCES table_shapes(table_shape_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    right_shape_id      integer         DEFAULT NULL REFERENCES table_shapes(table_shape_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    view_rights         rights          DEFAULT 'operator',
    edit_rights         rights          DEFAULT 'admin',
    insert_rights       rights          DEFAULT 'admin',
    remove_rights       rights          DEFAULT 'admin'
);
ALTER TABLE table_shapes OWNER TO lanview2;

COMMENT ON TABLE  table_shapes                      IS 'Tábla megjelenítő lírók (Qt GUI) táblája';
COMMENT ON COLUMN table_shapes.table_shape_id       IS 'Egyedi azonosító ID';
COMMENT ON COLUMN table_shapes.table_shape_name     IS 'A shape neve, egyedi azonosító';
COMMENT ON COLUMN table_shapes.table_shape_note    IS 'A shape leírása ill. megjegyzés.';
COMMENT ON COLUMN table_shapes.table_shape_title    IS 'A shape megjelenítésekor megjelenő táblázat cím.';
COMMENT ON COLUMN table_shapes.table_shape_type     IS 'A listázandó objektum típusa.';
COMMENT ON COLUMN table_shapes.table_name           IS 'A shape álltal megjelenítendő tábla neve';
COMMENT ON COLUMN table_shapes.refine               IS 'Egy opcionális feltétel, ha a táblának csak egy részhalmaza kell (WHERE clause)';
COMMENT ON COLUMN table_shapes.properties           IS 'További paraméterek.';
COMMENT ON COLUMN table_shapes.left_shape_id        IS 'Az bal oldali, tulajdonos, vagy member tábla megjelenítő leíróra mutat';
COMMENT ON COLUMN table_shapes.right_shape_id       IS 'A jobb oldali, gyerek, vagy csoport tábla megjelenítő leíróra mutat';
COMMENT ON COLUMN table_shapes.view_rights          IS 'Minimális jogosultsági szint a táblába megtekintéséhez';
COMMENT ON COLUMN table_shapes.edit_rights          IS 'Minimális jogosultsági szint a tábábla szerkesztéséhez';
COMMENT ON COLUMN table_shapes.insert_rights        IS 'Minimális jogosultsági szint a táblába való beszúráshoz';
COMMENT ON COLUMN table_shapes.remove_rights        IS 'Minimális jogosultsági szint a tábából való törléshez';

CREATE TYPE ordtype AS ENUM ('no', 'asc', 'desc');
ALTER TYPE ordtype OWNER TO lanview2;
COMMENT ON TYPE ordtype IS
'A mező szerinti rendezés lehetséges értékei:
no      Nincs rendezés.
asc     Növekvő sorrend.
desc    Csökkenő sorrend.';

CREATE TABLE table_shape_fields (
    table_shape_field_id    serial          PRIMARY KEY,
    table_shape_field_name  varchar(32)     NOT NULL,
    table_shape_field_note varchar(255)    DEFAULT NULL,
    table_shape_field_title varchar(32)     DEFAULT NULL,
    table_shape_id          integer         NOT NULL REFERENCES table_shapes(table_shape_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    field_sequence_number   integer         NOT NULL,
    ord_types               ordtype[]       DEFAULT '{"no","asc","desc"}',
    ord_init_type           ordtype         DEFAULT NULL,
    ord_init_sequence_number integer        DEFAULT NULL,
    is_read_only            boolean         DEFAULT 'f',
    is_hide                 boolean         DEFAULT 'f',
    id2name                 varchar(32)     DEFAULT NULL,
    default_value           varchar(255)    DEFAULT NULL,
    properties              varchar(255)    DEFAULT NULL,
    view_rights             rights          DEFAULT 'operator',
    edit_rights             rights          DEFAULT 'admin',
    UNIQUE (table_shape_id, table_shape_field_name)
);
ALTER TABLE table_shape_fields OWNER TO lanview2;

COMMENT ON TABLE  table_shape_fields                         IS 'Tábla shape oszlop leíró tábla';
COMMENT ON COLUMN table_shape_fields.table_shape_field_id    IS 'Egyedi azonosító ID';
COMMENT ON COLUMN table_shape_fields.table_shape_field_name  IS 'Mező név, a query-ben használt név.';
COMMENT ON COLUMN table_shape_fields.table_shape_field_note IS 'A mező dialog box-ban megjelenő neve';
COMMENT ON COLUMN table_shape_fields.table_shape_field_title IS 'Megjelenített mező (oszlop) név';
COMMENT ON COLUMN table_shape_fields.table_shape_id          IS 'Távoli kulcs a tulajdonos rekordra.';
COMMENT ON COLUMN table_shape_fields.field_sequence_number   IS 'A mező sorrendje a táblázatban / dialog boxban';
COMMENT ON COLUMN table_shape_fields.ord_types               IS 'A mező rendezési lehetőségei';
COMMENT ON COLUMN table_shape_fields.ord_init_type           IS 'Opcionális, a mező érték szerinti rendezés alap beállítása.';
COMMENT ON COLUMN table_shape_fields.ord_init_sequence_number IS 'Opcionális, a mező érték szerinti rendezés alap sorrendje.';
COMMENT ON COLUMN table_shape_fields.is_read_only            IS 'Mindenképpen csak olvasható mező, ha értéke true';
COMMENT ON COLUMN table_shape_fields.is_hide                 IS 'A táblázatos megjelenítésnél rejtett';
COMMENT ON COLUMN table_shape_fields.default_value           IS 'Egy opcionális default érték.';
COMMENT ON COLUMN table_shape_fields.id2name                 IS 'Egy opcionális SQL függvény az ID mező stringgé konvertálásához';
COMMENT ON COLUMN table_shape_fields.properties              IS 'További paraméterek.';
COMMENT ON COLUMN table_shape_fields.view_rights             IS 'Minimális jogosultsági szint a mező megtekintéséhez';
COMMENT ON COLUMN table_shape_fields.edit_rights             IS 'Minimális jogosultsági szint a mező szerkestéséhez';

CREATE TYPE filtertype AS ENUM ('begin', 'like', 'similar', 'regexp', 'regexpi', 'big', 'litle', 'interval', 'proc', 'SQL');
ALTER TYPE filtertype OWNER TO lanview2;
COMMENT ON TYPE filtertype IS
'Opcionális filter típusa egy mezőre:
equal   A megadott érték megegyezik a mező értékével
begin   A megadott string illeszkedik a mező elejére.
like    A megadott sztring illeszkedik a mezőre egy LIKE kifejezésben.
similar A megadott sztring illeszkedik a mezőre egy SIMILAR kifejezésben.
regexp  A megadott sztring illeszkedik a mezőre egy regexp kifejezésben.
regexpi A megadott sztring illeszkedik a mezőre egy regexp kifejezésben, nem kisbetű érzékeny.
big     Numerikus mező a magadott értéknél nagyobb
litle   Numerikus mező a magadott kisebb nagyobb
interval Numerikus mező értéke a magadott tartományban
proc    Szűrés egy függvényen leresztül.
SQL	    egy WHERE feltétel megadása';

CREATE TABLE table_shape_filters (
    table_shape_filter_id       serial          PRIMARY KEY,
    table_shape_filter_note    varchar(255)    DEFAULT NULL,
    table_shape_field_id        integer         REFERENCES table_shape_fields(table_shape_field_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    filter_type                 filtertype      NOT NULL
);
ALTER TABLE table_shape_filters OWNER TO lanview2;

COMMENT ON TABLE table_shape_filters                IS 'A mezőkhöz definiálható szűrök paraméterei.';
COMMENT ON COLUMN table_shape_filters.table_shape_field_id IS 'Kulcs a mezőre, meyhez a filter tartozik.';
COMMENT ON COLUMN table_shape_filters.filter_type   IS 'Alkalmazható filter típusa';


CREATE TABLE enum_vals (
    enum_val_id         serial      PRIMARY KEY,
    enum_val_name       varchar(32) NOT NULL,
    enum_val_note      varchar(255),
    enum_type_name      varchar(32) NOT NULL,
    UNIQUE (enum_type_name, enum_val_name)
);
ALTER TABLE enum_vals OWNER TO lanview2;

COMMENT ON TABLE  enum_vals IS 'Enumerációs értékek megjelenítését segítő tábla.';
COMMENT ON COLUMN enum_vals.enum_val_name IS 'Az enumerációs típus egy lehetséges értéke';
COMMENT ON COLUMN enum_vals.enum_val_note IS 'Az enumerációs értékhez tartozó leírás, megjelenítednó szöveg';
COMMENT ON COLUMN enum_vals.enum_type_name IS 'Az enumerációs típusnak a neve';


CREATE OR REPLACE FUNCTION table_shape_name2id(varchar(32)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT table_shape_id INTO id FROM table_shapes WHERE table_shape_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'table_shape_name2id()', 'table_shapes');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION table_shape_field_name2id(varchar(32), varchar(32)) RETURNS integer AS $$
DECLARE
    id integer;
BEGIN
    SELECT table_shape_field_id  INTO id FROM table_shape_fields JOIN table_shapes USING (table_shape_id) WHERE table_shape_name = $1 AND table_shape_field_name = $2;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'table_shape_field_name2id()', 'table_shape_fields');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION enum_name2descr(varchar(32), varchar(32) DEFAULT NULL) RETURNS varchar(255) AS $$
DECLARE
    descr varchar(255);
BEGIN
    IF $2 IS NULL THEN
        BEGIN
            SELECT enum_val_note INTO descr FROM enum_vals WHERE enum_val_name = $1;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                    descr = $1;
                WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                    PERFORM error('Ambiguous', -1, $1, 'enum_name2descr()', 'enum_vals');
        END;
    ELSE 
        SELECT enum_val_note INTO descr FROM enum_vals WHERE enum_val_name = $1 AND enum_type_name = $2;
        IF NOT FOUND THEN
            descr = $1;
        END IF;
    END IF;
    RETURN descr;
END
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION enum_name2descr(varchar(32), varchar(32)) IS
'Egy enumerációs értékhez tartozó descr stringet kéri le
Paraméterek:
    $1 Az enumerációs érték.
    $2 Az enumerációs típus neve opcionális.
Ha van megfelelő rekord enum_vals táblában, akkor enum_val_note értékével tér vissza, ha nem akkor az első paraméter értékével.
Ha nem adtuk meg az enumerációs típust, és az enumerációs értékre több találatot is kapunk, akkor a függvény hibát dob.';


DROP TABLE IF EXISTS menu_items;
CREATE TABLE menu_items (
    menu_item_id            serial          PRIMARY KEY,
    menu_item_name          varchar(32)     NOT NULL,
    item_sequence_number    integer         DEFAULT NULL,
    menu_item_title         varchar(32)     DEFAULT NULL,
    app_name                varchar(32)     NOT NULL,
    upper_menu_item_id      integer         DEFAULT NULL REFERENCES menu_items(menu_item_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    properties              varchar(255)    DEFAULT NULL,
    tool_tip                text            DEFAULT NULL,
    whats_this              text            DEFAULT NULL,
    UNIQUE (app_name, menu_item_name),
    UNIQUE (app_name, upper_menu_item_id, item_sequence_number)
);
ALTER TABLE menu_items OWNER TO lanview2;

CREATE OR REPLACE FUNCTION check_insert_menu_items() RETURNS TRIGGER AS $$
DECLARE
    n   integer;
BEGIN
    IF NEW.item_sequence_number IS NULL THEN
        IF NEW.upper_menu_item_id IS NULL THEN
            SELECT MAX(item_sequence_number) INTO n FROM menu_items WHERE app_name = NEW.app_name AND upper_menu_item_id IS NULL;
        ELSE 
            SELECT MAX(item_sequence_number) INTO n FROM menu_items WHERE app_name = NEW.app_name AND upper_menu_item_id = NEW.upper_menu_item_id;
        END IF;
        IF n IS NULL THEN
            n := 0;
        END IF;
        NEW.item_sequence_number := n + 10;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER check_insert_menu_items BEFORE INSERT  ON menu_items FOR EACH ROW EXECUTE PROCEDURE check_insert_menu_items();


