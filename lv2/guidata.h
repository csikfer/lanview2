#ifndef GUIDATA_H
#define GUIDATA_H

/*!
@file guidata.h
A megjelenítéssel kapcsolatos adattáblákat kezelő objektumok.
*/

#include "lanview.h"
#include "lv2cont.h"

/// @enum eTableShapeType
/// Tábla Shape típusok (set)
/// @relates cTableShape
enum eTableShapeType {
    TS_UNKNOWN =-1, ///< ismeretlen, csak hibajelzésre
    TS_SIMPLE = 0,  ///< "simple'   Egyszerű
    TS_TREE,        ///< "tree"     fa strukrúra
    TS_BARE,        ///< "bare"     Dekoráció nélkül, beágyazott táblázat
    TS_OWNER,       ///< "owner"    Tulajdonos (van child tánlája)
    TS_CHILD,       ///< "child"    A sorok egy másik objektum részei
    TS_LINK,        ///< "link"     Hasonló a kapcsoló táblához, de egy tábla (és leszármazottai) közötti kapcsolatot reprezentál (linkek)
    TS_DIALOG,      ///< "dialog"   Csak dialógus, tábla tiltva
    TS_TABLE,       ///< "table"    Csak táblázat, dialógus tiltva
    TS_MEMBER,      ///< "member"   A jobboldalon a csoport rekordok (tag/nem tag)
    TS_GROUP,       ///< "group"
    TS_READ_ONLY,   ///< "read_only" Nem modosítható
    TS_TOOLS,       ///< Table for tools_objects
    /// Dummy érték a CHKENU miatt (ne jelezzen hibát, a tableShapeType() üres stringet ad vissza),
    /// A további lehetséges értékek nem megengedettek az adatbázisban, csak az API használja.
    TS_UNKNOWN_PAD,
    TS_IGROUP,      ///< "igroup"  Jobb oldali groupoknak, az aktuális bal rekord tagja
    TS_NGROUP,      ///< "ngroup"  Jobb oldali groupoknak, az aktuális bal rekord nem tagja
    TS_IMEMBER,     ///< "imember" Jobb oldali tagok, az aktuális bal csoportnak tagjai
    TS_NMEMBER      ///< "nmember" Jobb oldali tagok, az aktuális bal csoportnak nem tagjai
};
/// Konverziós függvény a eTableShapeType enumerációs típushoz.
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex értéke nem EX_WARNING vagy nagyobb, akkor az adatbázisban ismeretlen enumerációs konstans esetén kizárást dob,
///                          ha __ex értke EX_ERROR akkor megengedettek a csak az API által hsznált értékek is.
/// @return Az enumerációs névhez tartozó enumeráxiós érték, vagy -1, ha ismeretlen a konstams, és __ex értéke EX_IGNORE.
EXT_ int tableShapeType(const QString& n, enum eEx __ex = EX_ERROR);
/// Konverziós függvény a eTableShapeType enumerációs típushoz.  Az TS_UNKNOWN_PAD értéket nem konvertálja.
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex értéke nem EX_WARNING vagy nagyobb, akkor az adatbázisban ismeretlen enumerációs konstans esetén kizárást dob,
///                          ha __ex értke EX_ERROR akkor megengedettek a csak az API által hsznált értékek is.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams, és __ex értéke EX_IGNORE.
EXT_ const QString&       tableShapeType(int e, enum eEx __ex = EX_ERROR);

/// @enum eTableInheritType
/// A tábla öröklés kezelése (nincs minden lehetőség implementálva)
/// @relates cTableShape
enum eTableInheritType {
    TIT_UNKNOWN = -1,   ///< Ismeretlen, csak hibajelzésre
    TIT_NO = 0,         ///< A tábla nem vesz rész öröklésben (nincs ős és leszármazott)
    TIT_ONLY,           ///< Vannak leszármazottak, de azok kizárva
    TIT_ON,             ///< A leszármazottakat is meg kel jeleníteni
    TIT_ALL,            ///< Az összes ös és leszármazott megjelenítése
    TIT_REVERSE,        ///< Az ősöket is meg kell jeleníteni
    TIT_LISTED,         ///< A listázott leszármazottakat is meg kell jeleníteni
    TIT_LISTED_REV,     ///< A listázott ősöket is meg kell jeleníteni
    TIT_LISTED_ALL      ///< A listázott ősöket ill. leszármazottakat is meg kell jeleníteni.
};
/// Konverziós függvény a eTableInheritType enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy TIT_UNKNOWN, ha ismeretlen a név, és __ex hamis.
EXT_ int tableInheritType(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény a eTableInheritType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams, és __ex hamis.
EXT_ const QString& tableInheritType(int e, eEx __ex = EX_ERROR);

/// @enum eOrderType
/// Rendezési lehetőségek
enum eOrderType {
    OT_UNKNOWN =-1, ///< ismeretlen, csak hibajelzésre
    OT_NO = 0,      ///< Nincs rendezés
    OT_ASC,         ///< Rendezés növekvő sorrendbe
    OT_DESC,        ///< Rendezés csökkenő sorrendbe
    OT_DEFAULT,     ///< Az előző, vagy az alapértelmezett metódus megtartása (a string konvertáló függvény nem kezeli!)
    OT_NEXT         ///< A következő metódus (NO->INC->DEC) (a string konvertáló függvény nem kezeli!)
};
/// Konverziós függvény az eOrderType enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy OT_UNKNOWN, ha ismeretlen a név.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ int orderType(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény az eOrderType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ const QString&  orderType(int e, eEx __ex = EX_ERROR);

/// @enum eFieldFlag
/// A mező megjelenítésének attributumai.
/// @relates cTableShapeField
enum eFieldFlag {
    FF_UNKNOWN = -1,    ///< ismeretlen, csak hibajelzésre
    FF_TABLE_HIDE=  0,  ///< A táblázatos megjelenítésnél a mező rejtett
    FF_DIALOG_HIDE,     ///< A dialógusban (insert, modosít) a mező rejtett
    FF_READ_ONLY,       ///< Csak olvasható, nem modosítható mező
    FF_PASSWD,          ///< A mező egy jelszó (tartlma rejtett)
    FF_HUGE,            ///< A TEXT típusú mező több soros, vagy enum-nal a hosszú név megjelenítése
    FF_HTML_TEXT,       ///< HTML TEXT típusú mező, több soros
    FF_BATCH_EDIT,      ///< A mező értéke csoportosan is beállítható
    FF_BG_COLOR,        ///< A mező egy háttérszín
    FF_FG_COLOR,        ///< A mező egy karakterszín
    FF_FONT,            ///< A mező egy font név
    FF_TOOL_TIP,        ///< A ToolTip megjelenítése enumerációs típus esetén
    FF_HTML,            ///< HTML (szűkített) táblában megjelenítendő
    FF_RAW,             ///< A mező tartalma eredeti formában történő megjelenítése
    FF_IMAGE,           ///< Megjelenítés képként (is)
    FF_NOTEXT           ///< Képként megjelenítés esetén nincs szöveg
};

/// Konverziós függvény az eFieldFlag enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy FF_UNKNOWN, ha ismeretlen a név.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ int fieldFlag(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény az eFieldFlag enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ const QString& fieldFlag(int e, eEx __ex = EX_ERROR);

/// @enum eManuItemType
/// A menu elem típusok.
/// @related cMenuItem
enum eManuItemType {
    MT_UNKNOWN = -1,
    MT_SHAPE = 0,
    MT_OWN,
    MT_EXEC,
    MT_MENU
};

/// Konverziós függvény az eManuItemType enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy MI_UNKNOWN, ha ismeretlen a név.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ int menuItemType(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény az eManuItemType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ const QString& menuItemType(int e, eEx __ex = EX_ERROR);


class cTableShapeField;
class cTableShape;
typedef tRecordList<cTableShape>    tTableShapes;
typedef tOwnRecords<cTableShapeField, cTableShape>  tTableShapeFields;
/*!
@class cTableShape
@brief Adattáblák táblázatos megjelenítésének a leírója
*/
class LV2SHARED_EXPORT cTableShape : public cRecord {
    CRECORD(cTableShape);
    FEATURES(cTableShape)
public:
    /// Szöveg azonosítók
    enum eTextIndex {
        LTX_TABLE_TITLE = 0,    ///< Title of the table
        LTX_DIALOG_TITLE,       ///< Title of the dialog
        LTX_DIALOG_TAB_TITLE,   ///< Title of the dialog tab
        LTX_MEMBER_TITLE,       ///< Title of the members table
        LTX_NOT_MEMBER_TITLE,   ///< Title of the not members table
        LTX_TOOL_TIP            ///< Unused (Maybe later)
    };

    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();

    /// Egy table_shapes rekord beszúrása.
    virtual bool insert(QSqlQuery &__q, eEx __ex = EX_ERROR);
    /// Egy table_shapes rekord beszúrása, vagy fellülírása név szerint, ha már létezik
    virtual bool rewrite(QSqlQuery &__q, eEx __ex = EX_ERROR);
    /// Beállítja a típus mező értékét.
    /// A típus mezőnek lehetnek olyan értékei is, melyek az adatbázisban nem szerepelnek, viszont a megjelenítésnél van szerepük.
    /// Az alapértelmezett metódus ezeket az értékeket eldobná, természetesen ezek az extra értékek elvesznek,
    /// ha az objektumoot kiírnámk az adatbázisba. Az objektum státuszában bebillentjük az invalid bitet, ha extra típus lett megadva.
    /// Az extra értékeket a getId() matódus nem adja vissza, mert a konverziónál elvesznek, csak a get() metódus használható,
    /// ami egy QStringList -et ad vissza, és a enum2set() és tableShapeType() függvénnyekkel konvertálható qlonglong típussá.
    cTableShape& setShapeType(qlonglong __t);
    int fetchFields(QSqlQuery& q, bool raw = false);
    /// Az objektumhoz feltölti a ShapeFields konténert, default értékekkel.
    /// Az adatokat az adatbázisban nem tárolja el, feltételezi, hogy az adattábla neve ismert.
    /// Ha a tábla nem létezik, akkor dob egy kizárást.
    bool setDefaults(QSqlQuery &q, bool _disable_tree = false);
    /// A title típusú (LTX_TABLE_TITLE, LTX_DIALOG_TITLE, LTX_DIALOG_TAB_TITLE, LTX_MEMBER_TITLE) beállítása.
    /// A maximum 4 értéket az indexek sorrendjében állítja be. Ha egy tömb elem értéke "@", akkor annak
    /// értéke az index szerinti előző értékkel azonos, az első érték nem lehet "@".
    void setTitle(const QStringList& _tt);
    /// Egy érték beállítása a megadott nevű mező leírójában
    /// @param _fn A mező neve a (megjelenítendő) táblában.
    /// @param _fpn A mező leíró paraméterének a neve
    /// @param _v a beállítandó érték
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező, vagy paraméter) dob egy kizárást.
    /// @return Ha beállította az értéket, akkor true.
    bool fset(const QString& _fn, const QString& _fpn, const QVariant& _v, eEx __ex = EX_ERROR);
    /// Egy érték beállítása a megadott mezők leíróiban
    /// @param _fnl A mezők nevei a (megjelenítendő) táblában.
    /// @param _fpn A mező leíró paraméterének a neve
    /// @param _v a beállítandó érték
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező, vagy paraméter) dob egy kizárást.
    /// @return Ha beállította minden megadott mezőre az értéket, akkor true.
    bool fsets(const QStringList& _fnl, const QString& _fpn, const QVariant& _v, eEx __ex = EX_ERROR);
    /// A felsorolt nevű mezőkre az összes rendezési lehetőséget beállítja.
    bool setAllOrders(QStringList& _ord, eEx __ex = EX_ERROR);
    /// A felsorolt nevű mezőkre a megadott nevű rendezési lehetőséget beállítja.
    bool setOrders(const QStringList& _fnl, QStringList& _ord, eEx __ex = EX_ERROR);
    /// A mazők megjelenítésési sorrendjének a megváltoztatása
    /// @param _fnl A felsorolt nevű mezők sorrendjének a megadása
    /// @param last Az utolsó sorszám, melyhez tartozó mező sorrrendje nem változik, ill. ahonnan folytatjuk a sorszámozást.
    /// @param __ex Ha értéke nem EX_IGNORE, akkor ha olyan mező nevet adunk meg, ami nem létezik, dob egy kizárást.
    /// @return Hiba esetén false, egyébként true.
    bool setFieldSeq(const QStringList& _fnl, int last = 0, eEx __ex = EX_ERROR);
    /// A mazők rendezési sorrendjének a megváltoztatása
    /// @param _fnl A felsorolt nevű mezők rendezési sorrendjének a megadása
    /// @param last Az utolsó sorszám, melyhez tartozó mező sorrrendje nem változik, ill. ahonnan folytatjuk a sorszámozást.
    /// @param __ex Ha értéke nem EX_IGNORE, akkor ha olyan mező nevet adunk meg, ami nem létezik, dob egy kizárást.
    /// @return Hiba esetén false, egyébként true.
    bool setOrdSeq(const QStringList& _fnl, int last = 0, eEx __ex = EX_ERROR);
    ///
    bool typeIs(eTableShapeType _t) const { return getId(_sTableShapeType) & enum2set(_t); }
    /// A hivatkozott (tábla név a table_name nevű mezőben) tábla leírójával tér vissza.
    /// @exception cError* Ha nincs ijen tábla.
    const cRecStaticDescr * getRecDescr() const { return cRecStaticDescr::get(getName(_sTableName)); }
    /// Létrehoz, és hozzáad egy mező onjekrumot a shapeFields konténerhez.
    /// @param _name  Az oszlop ill. a leíró neve neve
    /// @param _tname Az oszlop neve a táblázatban.
    /// @param _title   A mező/oszlop fejlécen kiírt neve
    /// @param _note Megjegyzés
    /// @return Az új (konténerbe helyezett) objektum pointere.
    /// @exception Ha már létezik ilyen nevű elem a konténerben.
    cTableShapeField *addField(const QString& _name, const QString &_tname, const QString& _note = _sNul, eEx __ex = EX_ERROR);
    ///
    void addRightShape(QStringList& _sn);
    void setRightShape(QStringList& _sn) { clear(_sRightShapeIds); addRightShape(_sn); }

    cTableShape& setFieldFlags(const QString& __fn, qlonglong _onBts, qlonglong _offBits = 0LL);

    /// Visszaadja az _sn nevű table_shapes rekordhoz tartozó _fn nevű table_shape_fields rekordnak a
    /// dialog_title mező értékét. Ha valamelyik rekord hiányzik, és __ex értéke nem EX_WARNING vagy
    /// EX_NOOP, akkor _fn-t adja vissza. Ha hiányzik valamelyik rekord, és __ex értéke EX_WARNING vagy
    /// EX_NOOP, akkor kizárást dob.
    static const QString& getFieldDialogTitle(QSqlQuery& q, const QString& _sn, const QString& _fn, eEx __ex = EX_ERROR);
    static void resetCacheData() { fieldDialogTitleMap.clear(); }
    ///
    tTableShapeFields   shapeFields;
protected:
    /// Hiba string
    QString emFildsIsEmpty();
    /// Hiba string
    QString emFieldNotFound(const QString& __f);
    static tStringMap   fieldDialogTitleMap;
    STATICIX(cTableShape, TableShapeType)
};

class cTableShapeField;
/*!
@class cTableShapeField
@brief Adattáblák mezőinek a megjelenítési leírói
*/
class LV2SHARED_EXPORT cTableShapeField : public cRecord {
    CRECORD(cTableShapeField);
    FEATURES(cTableShapeField)
public:
    enum eTextIndex {
        LTX_TABLE_TITLE = 0,
        LTX_DIALOG_TITLE,
        LTX_TOOL_TIP,
        LTX_WHATS_THIS
    };

    cTableShapeField(QSqlQuery& q);
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual bool insert(QSqlQuery &__q, eEx __ex = EX_ERROR);
    virtual bool rewrite(QSqlQuery &__q, eEx __ex = EX_ERROR);
    void setTitle(const QStringList& _tt);
    bool fetchByNames(QSqlQuery& q, const QString& tsn, const QString& fn, eEx __ex = EX_ERROR);
    static qlonglong getIdByNames(QSqlQuery& q, const QString& tsn, const QString& fn);
    /// Display string referenced from the field.
    QString view(QSqlQuery &q, const cRecord& o, qlonglong fix = -1) const;
    ///
    QString colName(bool icon = false);
protected:
    static cRecStaticDescr  _staticDescr;
    STATICIX(cTableShapeField, TableFieldName)
    STATICIX(cTableShapeField, TableShapeId)
    STATICIX(cTableShapeField, FieldFlags)
};

enum eFontAttr {
    FA_BOOLD, FA_ITALIC, FA_UNDERLINE, FA_STRIKEOUT
};


enum eBoolVal {
    BE_TYPE = -1, BE_FALSE = 0, BE_TRUE
};
inline int bool2boolVal(bool b) { return b ? BE_TRUE : BE_FALSE; }


/// @class cEnumVal
/// @brief Enumerációs értékekhez opcionálisan tartozó beszédesebb szövegeket,
/// valamint a dekorációs lehetőségeket tartalmazó táblát (enum_vals) reprezentáló osztály.
/// A legtőbb művelethet az előre beolvasott teljes táblát használja, így csökkentve az adatbázis műveletek számát,
/// ezért a legtöbb esetben az enum_vals táblában eszközölt változtatások csak a program újraindításakor jutnak érvényre.
/// @sa
/// - const QColor& bgColorByEnum(const QString& __t, int e)
/// - const QColor& bgColorByBool(const QString& _tn, const QString& _fn, bool f)
/// - const QColor& dcBgColor(int id)
/// - const QColor& fgColorByEnum(const QString& __t, int e)
/// - const QColor& fgColorByBool(const QString& _tn, const QString& _fn, bool f)
/// - const QColor& dcFgColor(int id)
/// - const QFont& fontByEnum(const QString& __t, int _e)
/// - const QFont& fontByEnum(const cEnumVal& ev)
/// - const QFont& dcFont(int id)
/// - void enumSetColor(QWidget *pW, const QString& _t, int id)
/// - void enumSetBgColor(QWidget *pW, const QString& _t, int id)
/// - void enumSetFgColor(QWidget *pW, const QString& _t, int id)
/// - void dcSetColor(QWidget *pW, int id)
/// - void dcSetBgColor(QWidget *pW, int id)
/// - void dcSetFbColor(QWidget *pW, int id)
/// - void enumSetD(QWidget *pW, const cEnumVal& ev, int id = NULL_IX)
/// - void enumSetD(QWidget *pW, const QString& _t, int id)
/// - void enumSetD(QWidget *pW, const QString& _t, int id, qlonglong ff, int dcId)
/// - void dcSetD(QWidget *pW, int id)
/// - void enumSetShort(W *pW, const QString& __t, int id, const QString& _ds)
/// - void enumSetShort(W *pW, const QString& __t, int id, const QString& _ds, qlonglong ff, int dcId)
/// - void dcSetShort(W *pW, int id)
/// - QVariant enumRole(const cEnumVal& ev, int role, int e)
/// - QVariant enumRole(const QString& _t, int id, int role, const QString& dData)
/// - QVariant dcRole(int id, int role)
///
class LV2SHARED_EXPORT cEnumVal : public cRecord {
    CRECORD(cEnumVal);
public:
    /// Nyelvi szöveg indexek
    enum eTextIndex {
        LTX_VIEW_LONG = 0,  ///< hosszú név
        LTX_VIEW_SHORT,     ///< rövid név
        LTX_TOOL_TIP,       ///< ToolTip (help)
        LTX_NUMBER          ///< Nyelvi szöveg mezők száma
    };
    /// Konstruktor, az alapértelmezett értékek beállításával (type)
    cEnumVal(const QString _tn);
    /// Konstruktor, az alapértelmezett értékek beállításával (value)
    cEnumVal(const QString _tn, const QString _en);
    virtual int replace(QSqlQuery &__q, eEx __ex = EX_ERROR);
    /// Rekord(ok) törlésa az enumeráció típus név alapján
    /// @param  q
    /// @param __n Az enum_type_name mező értéke, vagy minta a mezőre
    /// @param __pat Ha értéke true, akkor az __n paraméter minta stringnek tekintendő.
    /// @return a törölt rekordok száma
    int delByTypeName(QSqlQuery& q, const QString& __n, bool __pat = false);
    /// Rekord(ok) törlésa az enumeráció típus és érték név alapján
    /// @param  q
    /// @param __t Az enum_type_name mező értéke
    /// @param __n Az enum__name mező értéke
    /// @return true ha törölt rekordt
    bool delByNames(QSqlQuery& q, const QString& __t, const QString& __n);
    int toInt(eEx __ex = EX_ERROR) const;
    void setView(const QStringList& _tt);
protected:
    QStringList texts;
    /// Az összes enum_vals rekordot tartalmazó konténer
    /// Nem frissül automatikusan, ha változik az adattábla.
    static QList<cEnumVal *>                        enumVals;
    /// A típus, és név alapján az enumVals elemei.
    static QMap<QString, QVector<cEnumVal *> >      mapValues;
    /// Egy üres objektumra mutató pointer.
    /// Az enumVals feltöltésekor hozza létre az abjektumot a fetchEnumVals();
    static cEnumVal *pNull;
    static QStringList forcedList;
public:
    int textName2ix(QSqlQuery &q, const QString& _n, eEx __ex = EX_ERROR) const;
    /// Feltölti az adatbázisból az enumVals adattagot, ha nem üres a konténer, akkor frissíti.
    /// Iniciaéizálja a pNull pointert, ha az NULL. Egy üres objektumra fog mutatni.
    /// Frissítés esetén feltételezi, hogy rekordot törölni nem lehet, ezt ellenörzi.
    static void fetchEnumVals();
    /// Egy cEnumVal objektumot ad vissza a típus és érték alapján.
    /// Az enumVals adattagban keres, ha enumVals még nincs betöltve, akkor betölti.
    /// Ha __ex értéke EX_WARNING-nál kisebb, akkor kizárás helyett, egy üres objektummal tér vissza
    ///  (amire a pNull adattag mutat)
    /// @param _tn Az enumerációs típus neve
    /// @param e enumerációs érték, numerikus megfelelő, a -1 is értelmezve van mint típus
    static const cEnumVal& enumVal(const QString& _tn, int e, enum eEx __ex = EX_ERROR);
    /// Ellenörzi, hogy az megadott nevű enumerációs típus összes értékéhez létezik-e az
    /// cEnumVal objektum az enumVals konténerben, enumVal() hívásokkal.
    /// Ha nem létezik, akkor egy alapértelmezett objektumot elhelyez az enumVals konténerbe.
    /// @param _tn Az enumerációs típus neve
    /// @return Ha a rekordok léteztek, akkor 0, ha létre kellett hozni őket, akkor azok száma.
    static int enumForce(QSqlQuery& q, const QString& _tn);
    /// A háttér szín mező értékét adja vissza. Az enumVals konténerben keres.
    /// Ha nem találja a rekordot, akkor üres (_sNul) stringgel tér vissza.
    /// @param _tn Enumerációs típus neve
    /// @param e Enumerációs érték numerikus
    static QString sBgColor(const QString& _tn, int e, eEx __ex = EX_ERROR)
        { return enumVal(_tn, e, __ex).getName(_ixBgColor); }
    /// A karakter szín mező értékét adja vissza. Az enumVals konténerben keres.
    /// Ha nem találja a rekordot, akkor üres (_sNul) stringgel tér vissza.
    /// @param _tn Enumerációs típus neve
    /// @param e Enumerációs érték numerikus
    static QString sFgColor(const QString& _tn, int e, eEx __ex = EX_ERROR)
        { return enumVal(_tn, e, __ex).getName(_ixFgColor); }
    /// A háttér szín mező értékét adja vissza. Az enumVals konténerben keres.
    /// Boolean típusú adatnál a szín beállításhoz az értékeket a enum_type_name = teljes mező név: (<tábla>.<mező>),
    /// és enum_val_name = "true"|"false" rekordból olvassa ki.
    /// Ha nem találja a rekordot, akkor üres (_sNul) stringgel tér vissza.
    /// @param _tn A bool típusú mező tábla név
    /// @param _fn A bool típusú mező neve
    /// @param _f  A bool mező értéke.
    static QString sBgColor(const QString& _tn, const QString& _fn, bool _f, eEx __ex = EX_ERROR)
        { return enumVal(mCat(_tn, _fn), _f ? BE_TRUE : BE_FALSE, __ex).getName(_ixBgColor); }
    /// A karakter szín mező értékét adja vissza. Az enumVals konténerben keres.
    /// Boolean típusú adatnál a szín beállításhoz az értékeket a enum_type_name = teljes mező név: (<tábla>.<mező>),
    /// és enum_val_name = "true"|"false" rekordból olvassa ki.
    /// Ha nem találja a rekordot, akkor üres (_sNul) stringgel tér vissza.
    /// @param _tn A bool típusú mező tábla név
    /// @param _fn A bool típusú mező neve
    /// @param _f  A bool mező értéke.
    static QString sFgColor(const QString& _tn, const QString& _fn, bool _f, eEx __ex = EX_ERROR)
        { return enumVal(mCat(_tn, _fn), _f ? BE_TRUE : BE_FALSE, __ex).getName(_ixFgColor); }
    /// A hívás lekéri az enumerációhoz tartozó rövid beszédesebb nevet, ha van.
    /// Ha nincs ilyen érték, akkor amegadott alapértelmezett értékkel tér vissza.
    /// @param _tn A típus név
    /// @param e A numerikus érték
    /// @param _d Ha nincs rövid név, akkor ezzel tér vissza
    static QString viewShort(const QString& _tn, int e, const QString &_d)
        { return enumVal(_tn, e).getText(LTX_VIEW_SHORT, _d); }
    /// A hívás lekéri az enumerációhoz tartozó beszédesebb nevet, ha van.
    /// Ha nincs ilyen érték, akkor amegadott alapértelmezett értékkel tér vissza.
    /// @param _tn A típus név
    /// @param e A numerikus érték
    /// @param _d Ha nincs rövid név, akkor ezzel tér vissza
    static QString viewLong(const QString& _tn, int e, const QString &_d)
        { return enumVal(_tn, e).getText(LTX_VIEW_LONG, _d); }
    static QString toolTip(const QString& _tn, int e)
        { return enumVal(_tn, e).getText(LTX_TOOL_TIP); }
    QString typeName() const { return getName(ixEnumTypeName()); }

    STATICIX(cEnumVal, EnumTypeName)
    STATICIX(cEnumVal, EnumValName)
    STATICIX(cEnumVal, BgColor)
    STATICIX(cEnumVal, FgColor)
    STATICIX(cEnumVal, FontFamily)
    STATICIX(cEnumVal, FontAttr)
};

/// @enum eDataCharacter
/// Alap dekorációhoz kapcsolódó enumerációs konstansok.
enum eDataCharacter {
    DC_INVALID = -1,    ///< Ismeretlen, csak hibajelzésre
    DC_HEAD    = 0,     ///< Fejléc, cím
    DC_DATA,            ///< normál adat
    DC_ID,              ///< ID, numerikus egyedi rekord azonosító
    DC_NAME,            ///< Név, szöveges rekord azonosító
    DC_PRIMARY,         ///< Elsődleges kulcs mező
    DC_KEY,             ///< Kulcs mező
    DC_FNAME,           ///< Hivatkozott rekord név mező értéke
    DC_DERIVED,         ///< Hivatkozott rekord név (nincs név mező)
    DC_TREE,            ///< Saját táblára mutató távoli kulcs
    DC_FOREIGN,         ///< Távoli kulcs mező
    DC_NULL,            ///< NULL érték
    DC_DEFAULT,         ///< Alapértelmezett érték
    DC_AUTO,            ///< Automatikus érték
    DC_INFO,            ///< Információ
    DC_WARNING,         ///< Figyelmeztetés
    DC_ERROR,           ///< Hiba
    DC_NOT_PERMIT,      ///< nem engedélyezett
    DC_HAVE_NO,         ///< Az adat nem létezik
    DC_TEXT,            ///< Nyelvi szöveg
    DC_QUESTION         ///< Kérdés
};


/// Konverziós függvény az dataCharacter enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy DC_UNKNOWN, ha ismeretlen a név.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ int dataCharacter(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény az dataCharacter enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex értéke nem EX_IGNORE, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams.
/// @exception cError Ha nem konvertálható az érték, és __ex értéke nem EX_IGNORE.
EXT_ const QString& dataCharacter(int e, eEx __ex = EX_ERROR);

/// A datacharacter (eDataCharacter) enumerációs értékhez rendelt rövid névvel (LTX_VIEW_SHORT) tér vissza.
/// Ha az adatbázis nincs nyiva, akkor az enumerációs érték nevével.
/// @param id Az enumeráció numerikus értéke
EXT_ QString dcViewShort(int id);
/// A datacharacter (eDataCharacter) enumerációs értékhez rendelt hosszú névvel (LTX_VIEW_LONG) tér vissza.
/// @param id Az enumeráció numerikus értéke
inline QString dcViewLong(int id)  { return cEnumVal::viewLong( _sDatacharacter, id, dataCharacter(id)); }

class LV2SHARED_EXPORT cMenuItem : public cRecord {
    CRECORD(cMenuItem);
    FEATURES(cMenuItem)
public:
    /// Nyelvi szöveg indexek
    enum eTextIndex {
        LTX_MENU_TITLE = 0,
        LTX_TAB_TITLE,
        LTX_TOOL_TIP,
        LTX_WHATS_THIS
    };
    bool fetchFirstItem(QSqlQuery &q, const QString &_appName, qlonglong upId = NULL_ID);
    int delByAppName(QSqlQuery &q, const QString &__n, bool __pat = false) const;
    STATICIX(cMenuItem, MenuItemType)
    STATICIX(cMenuItem, MenuParam)
public:
    int getType() { return int(getId(_ixMenuItemType)); }
    QString getParam() { return getName(_ixMenuParam); }
    void setTitle(const QStringList& _tt);
};

#endif // GUIDATA_H
