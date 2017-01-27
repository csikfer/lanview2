
DROP TABLE IF EXISTS table_shape_filters, table_shape_fields, table_shapes, enum_vals, id_view_procs CASCADE;
DROP TYPE  IF EXISTS tableshapetype, filtertype, fieldflag, filterdatatype, ordtype, tableinherittype;

CREATE TYPE tableshapetype AS ENUM ('simple', 'tree', 'bare', 'owner', 'child', 'link', 'dialog', 'table' , 'member', 'group', 'read_only');
ALTER TYPE tableshapetype OWNER TO lanview2;
COMMENT ON TYPE tableshapetype IS
'table shape típusa:
simple      Egyszerű tábla.
tree        Fa struktúrájú objektumok
bare        Fejléc nélküli, beágyzott
owner       A tábla egy másik tábla tulajdonosa (pl a ''nodes'' az ''nports'' -nak)
child       A tábla egy owner táblához tartozik
link        Link tábla (Adattartalommal is rendelkező kapcsoló tábla)
dialog      Csak dialogus megjelenítése, tábla tiltva
table       Csak tábla megjelenítése, dialogus tiltva
member      A megjelenített tábla elemei csoport tagok, a jobb oldalon a csoport rekordok lesznek megjelenítva
group       A megjelenített tábla elemei csoportok, jobboldalon a csoport tagjai.
read_only   Nem modosítható'
;

CREATE TYPE tableinherittype AS ENUM ('no', 'only', 'on', 'all', 'reverse', 'listed', 'listed_rev', 'listed_all');
ALTER TYPE tableinherittype OWNER TO lanview2;
COMMENT ON TYPE tableinherittype IS
'A tábla leszármazottainak és őseinek a kezelése
no          Nincs leszármazott
only        Vannak leszármazottak, de azokat nem jelenítjük meg
on          A leszármazottakat is meg kell jeleníteni
all         Az ősök és leszármazottak kell megjeleníteni
reverse     Az ősöket is meg kell jeleníteni
listed      Csak a felsorolt leszármazottak megjelenítése
listed_rev  Csak a felsorolt ősök megjelenítése
listed_all  Csak a felsorolt leszármazottak és ősök megjelenítése';

CREATE TABLE table_shapes (
    table_shape_id      bigserial       PRIMARY KEY,
    table_shape_name    text            NOT NULL UNIQUE,
    table_shape_note    text            DEFAULT NULL,
    table_title         text            DEFAULT NULL,
    dialog_title        text            DEFAULT NULL,
    dialog_tab_title    text            DEFAULT NULL,
    member_title        text            DEFAULT NULL,
    not_member_title    text            DEFAULT NULL,
    table_shape_type   tableshapetype[] DEFAULT '{simple}',
    table_name          text            NOT NULL,
    schema_name         text            NOT NULL DEFAULT 'public',
    table_inherit_type tableinherittype DEFAULT 'no',
    inherit_table_names text[]          DEFAULT NULL,
    refine              text            DEFAULT NULL,
    features            text            DEFAULT NULL,
    right_shape_ids     bigint[]        DEFAULT NULL, -- REFERENCES table_shapes(table_shape_id) MATCH SIMPLE ON DELETE SET NULL ON UPDATE RESTRICT,
    auto_refresh        interval        DEFAULT NULL,
    view_rights         rights          DEFAULT 'viewer',
    edit_rights         rights          DEFAULT 'operator',
    insert_rights       rights          DEFAULT 'operator',
    remove_rights       rights          DEFAULT 'admin'
);
ALTER TABLE table_shapes OWNER TO lanview2;

COMMENT ON TABLE  table_shapes                      IS 'Tábla megjelenítő lírók (Qt GUI) táblája';
COMMENT ON COLUMN table_shapes.table_shape_id       IS 'Egyedi azonosító ID';
COMMENT ON COLUMN table_shapes.table_shape_name     IS 'A shape neve, egyedi azonosító';
COMMENT ON COLUMN table_shapes.table_shape_note     IS 'A shape leírása ill. megjegyzés.';
COMMENT ON COLUMN table_shapes.table_title          IS 'A táblázat címe.';
COMMENT ON COLUMN table_shapes.dialog_title         IS 'A dialógus címe.';
COMMENT ON COLUMN table_shapes.dialog_tab_title     IS 'A tab címe, dialógus esetén (öröklésnál vannak tab-ok)';
COMMENT ON COLUMN table_shapes.member_title         IS 'Tagok altábla (jobb oldali tábla) címe';
COMMENT ON COLUMN table_shapes.member_title         IS 'Nem tagok altábla (jobb oldali tábla) címe';
COMMENT ON COLUMN table_shapes.table_shape_type     IS 'A megjelenítés típusa.';
COMMENT ON COLUMN table_shapes.table_name           IS 'A shape álltal megjelenítendő tábla neve, a mező hivatkozások mindíg erre vonatkoznak';
COMMENT ON COLUMN table_shapes.table_inherit_type   IS 'Öröklés esetén a geyerek/szülő táblák kezelési módja';
COMMENT ON COLUMN table_shapes.inherit_table_names  IS 'A megjelenítendő rokon táblák listája, a table_name mezőben megadott táblanavet nem tartalmazza';
COMMENT ON COLUMN table_shapes.refine               IS 'Egy opcionális feltétel, ha a táblának csak egy részhalmaza kell (WHERE clause)';
COMMENT ON COLUMN table_shapes.features             IS 'További paraméterek.';
COMMENT ON COLUMN table_shapes.right_shape_ids      IS 'A jobb oldali, gyerek, vagy csoport táblákat megjelenítő leírókra mutatnak az elemei';
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

CREATE TYPE fieldflag AS ENUM ('table_hide', 'dialog_hide', 'auto_set', 'read_only', 'passwd', 'huge', 'batch_edit');
ALTER TYPE fieldflag OWNER TO lanview2;
COMMENT ON TYPE fieldflag IS
'A mező tulajdonságok logokai (igen/nem):
table_hide      A táblázatos megjelenítésnél a mező rejtett
dialog_hide     A dialógusban (insert, modosít) a mező rejtett
auto_set        A mező értéke automatikusan kap értéket
read_only       A mező nem szerkeszthető
passwd          A mező egy jelszó (tartlma rejtett)
huge            A TEXT típusú mező több soros
batch_edit      A mező kötegelten modosítható
';

CREATE TABLE table_shape_fields (
    table_shape_field_id    bigserial       PRIMARY KEY,
    table_shape_field_name  text            NOT NULL,
    table_shape_field_note  text            DEFAULT NULL,
    table_title             text            DEFAULT NULL,
    dialog_title            text            DEFAULT NULL,
    table_shape_id          bigint          NOT NULL REFERENCES table_shapes(table_shape_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    field_sequence_number   integer         NOT NULL,
    ord_types               ordtype[]       DEFAULT NULL,
    ord_init_type           ordtype         DEFAULT 'no',
    ord_init_sequence_number integer        DEFAULT NULL,
    field_flags             fieldflag[]     DEFAULT '{}',
    expression              text            DEFAULT NULL,
    default_value           text            DEFAULT NULL,
    features                text            DEFAULT NULL,
    tool_tip                text            DEFAULT NULL,
    whats_this              text            DEFAULT NULL,
    view_rights             rights          DEFAULT NULL,
    edit_rights             rights          DEFAULT NULL,
    flag                    boolean         DEFAULT false,
    UNIQUE (table_shape_id, table_shape_field_name)
);
ALTER TABLE table_shape_fields OWNER TO lanview2;

COMMENT ON TABLE  table_shape_fields                         IS 'Tábla shape oszlop leíró tábla';
COMMENT ON COLUMN table_shape_fields.table_shape_field_id    IS 'Egyedi azonosító ID';
COMMENT ON COLUMN table_shape_fields.table_shape_field_name  IS 'Mező név, a query-ben használt név.';
COMMENT ON COLUMN table_shape_fields.table_shape_field_note  IS 'A mező dialog box-ban megjelenő neve';
COMMENT ON COLUMN table_shape_fields.table_title             IS 'Megjelenített mező (oszlop) cím';
COMMENT ON COLUMN table_shape_fields.dialog_title            IS 'Megjelenített mező név a dialogusban';
COMMENT ON COLUMN table_shape_fields.table_shape_id          IS 'Távoli kulcs a tulajdonos rekordra.';
COMMENT ON COLUMN table_shape_fields.field_sequence_number   IS 'A mező sorrendje a táblázatban / dialog boxban';
COMMENT ON COLUMN table_shape_fields.ord_types               IS 'A mező rendezési lehetőségei';
COMMENT ON COLUMN table_shape_fields.ord_init_type           IS 'Opcionális, a mező érték szerinti rendezés alap beállítása.';
COMMENT ON COLUMN table_shape_fields.ord_init_sequence_number IS 'Opcionális, a mező érték szerinti rendezés alap sorrendje.';
COMMENT ON COLUMN table_shape_fields.field_flags             IS 'A mező fleg-ek';
COMMENT ON COLUMN table_shape_fields.default_value           IS 'Egy opcionális default érték.';
COMMENT ON COLUMN table_shape_fields.expression              IS 'Egy opcionális SQL kifelyezés a mező értékére';
COMMENT ON COLUMN table_shape_fields.features                IS 'További paraméterek.';
COMMENT ON COLUMN table_shape_fields.view_rights             IS 'Minimális jogosultsági szint a mező megtekintéséhez, NULL esetén a táblánál magadottak az érvényesek';
COMMENT ON COLUMN table_shape_fields.edit_rights             IS 'Minimális jogosultsági szint a mező szerkestéséhez, NULL esetén a táblánál magadottak az érvényesek';

CREATE TYPE filtertype AS ENUM ('begin', 'like', 'similar', 'regexp', 'regexpi', 'big', 'litle', 'interval', 'proc', 'SQL');
ALTER TYPE filtertype OWNER TO lanview2;
COMMENT ON TYPE filtertype IS
'Opcionális filter típusa egy mezőre:
begin   A megadott string illeszkedik a mező elejére.
like    A megadott sztring illeszkedik a mezőre egy LIKE kifejezésben.
similar A megadott sztring illeszkedik a mezőre egy SIMILAR kifejezésben.
regexp  A megadott sztring illeszkedik a mezőre egy regexp kifejezésben.
regexpi A megadott sztring illeszkedik a mezőre egy regexp kifejezésben, nem kisbetű érzékeny.
big     Numerikus mező a magadott értéknél nagyobb
litle   Numerikus mező a magadott kisebb nagyobb
interval Numerikus mező értéke a magadott tartományban
proc    Szűrés egy függvényen leresztül.
SQL	egy WHERE feltétel megadása';

CREATE TABLE table_shape_filters (
    table_shape_filter_id       bigserial      PRIMARY KEY,
    table_shape_filter_note     text   DEFAULT NULL,
    table_shape_field_id        bigint         REFERENCES table_shape_fields(table_shape_field_id) MATCH FULL ON DELETE CASCADE ON UPDATE RESTRICT,
    filter_type                 filtertype     NOT NULL,
    features                    text           DEFAULT NULL,
    flag                        boolean        DEFAULT false
);
ALTER TABLE table_shape_filters OWNER TO lanview2;

COMMENT ON TABLE table_shape_filters                IS 'A mezőkhöz definiálható szűrök paraméterei.';
COMMENT ON COLUMN table_shape_filters.table_shape_field_id IS 'Kulcs a mezőre, meyhez a filter tartozik.';
COMMENT ON COLUMN table_shape_filters.filter_type   IS 'Alkalmazható filter típusa';


CREATE TABLE enum_vals (
    enum_val_id         bigserial       PRIMARY KEY,
    enum_val_name       text     NOT NULL,
    enum_val_note       text,
    enum_type_name      text     NOT NULL,
    UNIQUE (enum_type_name, enum_val_name)
);
ALTER TABLE enum_vals OWNER TO lanview2;

COMMENT ON TABLE  enum_vals IS 'Enumerációs értékek megjelenítését segítő tábla.';
COMMENT ON COLUMN enum_vals.enum_val_name IS 'Az enumerációs típus egy lehetséges értéke';
COMMENT ON COLUMN enum_vals.enum_val_note IS 'Az enumerációs értékhez tartozó leírás, megjelenítednó szöveg';
COMMENT ON COLUMN enum_vals.enum_type_name IS 'Az enumerációs típusnak a neve';


DROP TABLE IF EXISTS menu_items;
CREATE TABLE menu_items (
    menu_item_id            bigserial   PRIMARY KEY,
    menu_item_name          text        NOT NULL,
    app_name                text        NOT NULL,
    upper_menu_item_id      bigint      DEFAULT NULL REFERENCES menu_items(menu_item_id) MATCH SIMPLE ON DELETE CASCADE ON UPDATE RESTRICT,
    item_sequence_number    integer     DEFAULT NULL,
    menu_title              text        NOT NULL,
    tab_title               text        DEFAULT NULL,
    features                text        DEFAULT NULL,
    tool_tip                text        DEFAULT NULL,
    whats_this              text        DEFAULT NULL,
    menu_rights             rights      NOT NULL DEFAULT 'none',
    UNIQUE (app_name, menu_item_name),
    UNIQUE (app_name, upper_menu_item_id, item_sequence_number)
);
ALTER TABLE menu_items OWNER TO lanview2;
COMMENT ON TABLE menu_items IS 'GUI menü elemek definíciója';
COMMENT ON COLUMN menu_items.menu_item_id IS 'Menü elem egyedi azonosító';
COMMENT ON COLUMN menu_items.menu_item_name IS 'Applikáción bellül egyedi név';
COMMENT ON COLUMN menu_items.item_sequence_number IS 'A sorrendet meghatározó szám';
COMMENT ON COLUMN menu_items.menu_title IS 'Megjelenített név.';
COMMENT ON COLUMN menu_items.tab_title IS 'A megjelenített tab neve.';
COMMENT ON COLUMN menu_items.app_name IS 'Aplikáció név, melyhez a menü elem tartozik.';
COMMENT ON COLUMN menu_items.upper_menu_item_id IS 'Al menű esetén az elemet tartalmazó felsőbb szintű menü elem azonosítója.';
COMMENT ON COLUMN menu_items.features IS 'Járulékos paraméterek (a menü elem típusát határozza meg): ":shape=<név>:", ":exec=<név>:", ":own=<név>:", ...';
COMMENT ON COLUMN menu_items.tool_tip IS '';
COMMENT ON COLUMN menu_items.whats_this IS '';
COMMENT ON COLUMN menu_items.menu_rights IS 'Milyen minimális jogosutságnál jelenik meg az elem. Ha ez a végrajtandó elemnél ilyen paraméter nem létezik.';

CREATE OR REPLACE FUNCTION table_shape_name2id(text) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT table_shape_id INTO id FROM table_shapes WHERE table_shape_name = $1;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'table_shape_name2id()', 'table_shapes');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION table_shape_id2name(bigint) RETURNS text AS $$
DECLARE
    name text;
BEGIN
    SELECT table_shape_name INTO name FROM table_shapes WHERE table_shape_id = $1;
    IF NOT FOUND THEN
        PERFORM error('IdNotFound', $1, '', 'table_shape_id2name(bigint)', 'table_shapes');
    END IF;
    RETURN name;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION table_shape_id2name(bigint[]) RETURNS text AS $$
DECLARE
    tsid bigint;
    name text;
    rlist text := '';
BEGIN
    IF $1 IS NULL THEN
        RETURN NULL;
    END IF;
    FOREACH tsid IN ARRAY $1 LOOP
        SELECT table_shape_name INTO name FROM table_shapes WHERE table_shape_id = tsid;
        IF NOT FOUND THEN
            PERFORM error('IdNotFound', tsid, '', 'table_shape_id2name(bigint)', 'table_shapes');
        END IF;
        rlist := rlist || name || ',';
    END LOOP;
    RETURN trim( trailing ',' FROM rlist);
END
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION check_table_shape() RETURNS TRIGGER AS $$
DECLARE
    tsid  bigint;
BEGIN
    IF NEW.right_shape_ids IS NULL THEN
        RETURN NEW;
    END IF;
    IF array_length(NEW.right_shape_ids,1) = 0 THEN
        NEW.right_shape_ids = NULL;
        RETURN NEW;
    END IF;
    FOREACH tsid  IN ARRAY NEW.right_shape_ids LOOP
        IF 1 <> COUNT(*) FROM table_shapes WHERE table_shape_id = tsid THEN
            PERFORM error('IdNotFound', tsid, NEW.table_shape_name, 'check_table_shape()', TG_TABLE_NAME, TG_OP);
            RETURN NULL;
        END IF;
    END LOOP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER check_table_shape BEFORE UPDATE OR INSERT ON table_shapes FOR EACH ROW EXECUTE PROCEDURE check_table_shape();

CREATE OR REPLACE FUNCTION table_shape_field_name2id(text, text) RETURNS bigint AS $$
DECLARE
    id bigint;
BEGIN
    SELECT table_shape_field_id  INTO id FROM table_shape_fields JOIN table_shapes USING (table_shape_id) WHERE table_shape_name = $1 AND table_shape_field_name = $2;
    IF NOT FOUND THEN
        PERFORM error('NameNotFound', -1, $1, 'table_shape_field_name2id()', 'table_shape_fields');
    END IF;
    RETURN id;
END
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION enum_name2note(text, text DEFAULT NULL) RETURNS text AS $$
DECLARE
    note text;
BEGIN
    IF $2 IS NULL THEN
        BEGIN
            SELECT enum_val_note INTO note FROM enum_vals WHERE enum_val_name = $1;
            EXCEPTION
                WHEN NO_DATA_FOUND THEN     -- nem találtunk
                    note = $1;
                WHEN TOO_MANY_ROWS THEN     -- több találat is van, nem egyértelmű
                    PERFORM error('Ambiguous', -1, $1, 'enum_name2note()', 'enum_vals');
        END;
    ELSE 
        SELECT enum_val_note INTO note FROM enum_vals WHERE enum_val_name = $1 AND enum_type_name = $2;
        IF NOT FOUND THEN
            note = $1;
        END IF;
    END IF;
    RETURN note;
END
$$ LANGUAGE plpgsql;

COMMENT ON FUNCTION enum_name2note(text, text) IS
'Egy enumerációs értékhez tartozó note stringet kéri le
Paraméterek:
    $1 Az enumerációs érték.
    $2 Az enumerációs típus neve opcionális.
Ha van megfelelő rekord enum_vals táblában, akkor enum_val_note értékével tér vissza, ha nem akkor az első paraméter értékével.
Ha nem adtuk meg az enumerációs típust, és az enumerációs értékre több találatot is kapunk, akkor a függvény hibát dob.';

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


