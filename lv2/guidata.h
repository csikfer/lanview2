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
    TS_UNKNOWN_PAD, ///< Dummy érték a CHKENU miatt (ne jelezzen hibát), további lehetséges értékek nem megengedettek az adatbázisban
    TS_IGROUP,      ///< "igroup"  Jobb oldali groupoknak, az aktuális bal rekord tagja
    TS_NGROUP,      ///< "ngroup"  Jobb oldali groupoknak, az aktuális bal rekord nem tagja
    TS_IMEMBER,     ///< "imember" Jobb oldali tagok, az aktuális bal csoportnak tagjai
    TS_NMEMBER      ///< "nmember" Jobb oldali tagok, az aktuális bal csoportnak nem tagjai
};
/// Konverziós függvény a eTableShapeType enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy TS_UNKNOWN, ha ismeretlen a név, és __ex hamis.
EXT_ int tableShapeType(const QString& n, enum eEx __ex = EX_ERROR);
/// Konverziós függvény a eTableShapeType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams, és __ex hamis.
EXT_ const QString&       tableShapeType(int e, enum eEx __ex = EX_ERROR);

/// @enum eTableInheritType
/// A tábla öröklés kezelése
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
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy TIT_UNKNOWN, ha ismeretlen a név, és __ex hamis.
EXT_ int tableInheritType(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény a eTableInheritType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs konstans esetén kizárást dob.
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
    OT_NEXT,        ///< A következő metódus (NO->INC->DEC) (a string konvertáló függvény nem kezeli!)
    OT_ID_ASC       ///< Növekvő sorrend az ID alapján, csak a cRecordListModel -ben (a string konvertáló függvény nem kezeli!)
};
/// Konverziós függvény a eOrderType enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy OT_UNKNOWN, ha ismeretlen a név, és __ex hamis.
EXT_ int orderType(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény a eOrderType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams, és __ex hamis.
EXT_ const QString&  orderType(int e, eEx __ex = EX_ERROR);

enum eFieldFlag {
    FF_UNKNOWN = -1,    ///< ismeretlen, csak hibajelzésre
    FF_TABLE_HIDE=  0,  ///< A táblázatos megjelenítésnél a mező rejtett
    FF_DIALOG_HIDE,     ///< A dialógusban (insert, modosít) a mező rejtett
    FF_READ_ONLY,       ///<
    FF_PASSWD,          ///< A mező egy jelszó (tartlma rejtett)
    FF_HUGE,            ///< A TEXT típusú mező több soros, enum-nal hosszú név megjelenítése
    FF_BATCH_EDIT,      ///< A mező értéke csoportosan is beállítható
    FF_BG_COLOR,
    FF_FG_COLOR,
    FF_FONT,
    FF_TOOL_TIP
};

EXT_ int fieldFlag(const QString& n, eEx __ex = EX_ERROR);
EXT_ const QString& fieldFlag(int e, eEx __ex = EX_ERROR);

/// @enum eFilterType
/// Szűrési metódusok
enum eFilterType {
    FT_UNKNOWN = -1, ///< ismeretlen, csak hibajelzésre
    FT_BEGIN ,   ///< Szó eleji eggyezés (ua. mint az FF_LIKE, de a pattern végéhezhez hozzá lessz fűzve egy '%')
    FT_LIKE,     ///< Szűrés a LIKE operátorral
    FT_SIMILAR,  ///< Szűrés a SIMILAR operátorral
    FT_REGEXP,   ///< Szűrés reguláris kifejezéssel kisbetű-nagy betű érzékeny
    FT_REGEXPI,  ///< Szűrés reguláris kifejezéssel nem kisbetű-nagy betű érzékeny
    FT_BIG,      ///< Numerikus mező a magadott értéknél nagyobb
    FT_LITLE,    ///< Numerikus mező a magadott kisebb nagyobb
    FT_INTERVAL, ///< Numerikus mező értéke a magadott tartományban
    FT_PROC,     ///< Szűrés egy függvényen (PL) keresztül.
    FT_SQL_WHERE,///< SQL WHERE ...
    FT_BOOLEAN,  ///< Szűrés igaz, vagy hamis értékre
    // A további konstansokat a string konvertáló függvény nem kezeli!
    FT_DEFAULT,  ///< Az előző, vagy az alapértelmezett metódus megtartása (a string konvertáló függvény nem kezeli!)
    FT_NO,       ///< nincs szűrés(a string konvertáló függvény nem kezeli!)
    FT_FKEY_ID   ///< Szűrés a tulajdonos, vagy valamilyen tulajdonság objektum ID-je alapján (a string konvertáló függvény nem kezeli!)
};
/// Konverziós függvény a eFilterType enumerációs típushoz
/// @param n Az enumerációs értéket reprezentáló string az adatbázisban
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs név esetén kizárást dob.
/// @return Az enumerációs névhez tartozó enumeráxiós konstans, vagy FT_UNKNOWN, ha ismeretlen a név, és __ex hamis.
EXT_ int filterType(const QString& n, eEx __ex = EX_ERROR);
/// Konverziós függvény a eFilterType enumerációs típushoz
/// @param e Az enumerációs konstans
/// @param __ex hibakezekés: ha __ex igaz, akkor ismeretlen enumerációs konstans esetén kizárást dob.
/// @return Az enumerációs konstanshoz tartozó enumeráxiós név, vagy üres string, ha ismeretlen a konstams, és __ex hamis.
EXT_ const QString&   filterType(int e, eEx __ex = EX_ERROR);

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
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual bool insert(QSqlQuery &__q, eEx __ex = EX_ERROR);
    virtual bool rewrite(QSqlQuery &__q, eEx __ex = EX_ERROR);
    cTableShape& setShapeType(qlonglong __t);
    int fetchFields(QSqlQuery& q);
    /// Az objektumhoz feltölti a ShapeFields konténert, default értékekkel.
    /// Az adatokat az adatbázisban nem tárolja el, feltételezi, hogy az adattábla neve ismert.
    /// Ha a tábla nem létezik, akkor dob egy kizárást.
    bool setDefaults(QSqlQuery &q, bool _disable_tree = false);
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
    /// Egy új szűrő objektum hozzáadáse a mező leíróhoz
    /// @param _fnl A mező neve a táblában, melynek a leírója az objektum
    /// @param _t A szűrő típusa
    /// @param _d A szűrő megjelenített neve/leírása
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező) dob egy kizárást.
    /// @return Ha beállította minden megadott mezőre az értéket, akkor true.
    bool addFilter(const QString& _fn, const QString& _t, const QString& _d, eEx __ex = EX_ERROR);
    /// Egy új szűrő objektum hozzáadáse a mező leíróhoz
    /// @param _fnl A mezők nevei a táblában, melynek a leíróta az objektum
    /// @param _t A szűrő típusa
    /// @param _d A szűrő megjelenített neve/leírása
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező) dob egy kizárást.
    /// @return Ha beállította minden megadott mezőre az értéket, akkor true.
    bool addFilter(const QStringList& _fnl, const QString& _t, const QString& _d, eEx __ex = EX_ERROR);
    bool addFilter(const QStringList& _fnl, const QStringList& _ftl, eEx __ex = EX_ERROR);
    bool setAllOrders(QStringList& _ord, eEx __ex = EX_ERROR);
    bool setOrders(const QStringList& _fnl, QStringList& _ord, eEx __ex = EX_ERROR);
    /// A mazők megjelenítésési sorrendjének a megváltoztatása
    /// @param _fnl A felsorolt nevű mezők sorrendjének a megadása
    /// @param last Az utolsó sorszám, melyhez tartozó mező sorrrendje nem változik, ill. ahonnan folytatjuk a sorszámozást.
    /// @param __ex Ha olyan mező nevet adunk meg, ami nem létezik, dob egy kizárást.
    /// @return Hiba esetén false, egyébként true.
    bool setFieldSeq(const QStringList& _fnl, int last = 0, eEx __ex = EX_ERROR);
    bool setOrdSeq(const QStringList& _fnl, int last = 0, eEx __ex = EX_ERROR);

    ///
    bool typeIs(eTableShapeType _t) const { return getId(_sTableShapeType) & enum2set(_t); }
    const cRecStaticDescr * getRecDescr() const { return cRecStaticDescr::get(getName(_sTableName)); }
    /// Létrehoz, és hozzáad egy mező onjekrumot a shapeFields konténerhez.
    /// @param _name  A mező/oszlop neve
    /// @param _title   A mező/oszlop fejlécen kiírt neve
    /// @param _note Megjegyzés
    /// @return Az új (konténerbe helyezett) objektum pointere.
    /// @exception Ha már létezik ilyen nevű elem a konténerben.
    cTableShapeField *addField(const QString& _name, const QString& _note = _sNul, eEx __ex = EX_ERROR);
    ///
    void addRightShape(QStringList& _sn);
    void setRightShape(QStringList& _sn) { clear(_sRightShapeIds); addRightShape(_sn); }

    /// Visszaadja az _sn nevű table_shapes rekordhoz tartozó _fn nevű table_shape_fields rekordnak a
    /// dialog_title mező értékét. Ha valamelyik rekord hiányzik, és __ex értéke nem EX_WARNING vagy
    /// EX_NOOP, akkor _fn-t adja vissza. Ha hiányzik valamelyik rekord, és __ex értéke EX_WARNING vagy
    /// EX_NOOP, akkor kizárást dob.
    static const QString& getFieldDialogTitle(QSqlQuery& q, const QString& _sn, const QString& _fn, eEx __ex = EX_ERROR);
    static void resetCacheData() { fieldDialogTitleMap.clear(); }
    ///
    virtual QString objectExport(QSqlQuery& q, int indent = 0) const;
    ///
    tTableShapeFields   shapeFields;
protected:
    QString emFildsIsEmpty();
    QString emFieldNotFound(const QString& __f);
    static int              _ixTableShapeType;
    static QMap<QString, QString>   fieldDialogTitleMap;
};

class cTableShapeFilter;
class cTableShapeField;
typedef tOwnRecords<cTableShapeFilter, cTableShapeField> tTableShapeFilters;
/*!
@class cTableShapeField
@brief Adattáblák mezőinek a megjelenítési leírói
*/
class LV2SHARED_EXPORT cTableShapeField : public cRecord {
    CRECORD(cTableShapeField);
    FEATURES(cTableShapeField)
public:
    cTableShapeField(QSqlQuery& q);
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual bool insert(QSqlQuery &__q, eEx __ex = EX_ERROR);
    virtual bool rewrite(QSqlQuery &__q, eEx __ex = EX_ERROR);
    void setTitle(const QStringList& _tt);
    int fetchFilters(QSqlQuery& q);
    bool addFilter(const QString& _t, const QString& _d = QString(), eEx __ex = EX_ERROR);
    bool fetchByNames(QSqlQuery& q, const QString& tsn, const QString& fn, eEx __ex = EX_ERROR);
    static qlonglong getIdByNames(QSqlQuery& q, const QString& tsn, const QString& fn);
    tTableShapeFilters shapeFilters;
    STATICIX( cTableShapeField, ixTableShapeId)
protected:
    static cRecStaticDescr  _staticDescr;
};

class LV2SHARED_EXPORT cTableShapeFilter : public cRecord {
    CRECORD(cTableShapeFilter);
    STATICIX(cTableShapeFilter, ixTableShapeFieldId)
};

enum eFontAttr {
    FA_BOOLD, FA_ITALIC, FA_UNDERLINE, FA_STRIKEOUT
};

enum eBoolVal {
    BE_TYPE = -1, BE_FALSE = 0, BE_TRUE
};

inline static int str2boolVal(const QString& val)
{ return val.isEmpty() ? BE_TYPE : val == _sFalse ? BE_FALSE : val == _sTrue ? BE_TRUE : NULL_IX; }
inline static int bool2boolVal(bool b) { return b ? BE_TRUE : BE_FALSE; }


/// @class cEnumVal
/// @brief Enumerációs értékekhez opcionálisan tartozó beszédesebb szövegeket tartalmazó tábla.
class LV2SHARED_EXPORT cEnumVal : public cRecord {
    CRECORD(cEnumVal);
public:
    /// A hívás lekéri az enumerációhoz tartozó beszédesebb nevet, ha van.
    /// Ha nincs ilyen érték, akkor az enumerációs értékkel tér vissza.
    /// Ha az enumerációs érték egyértelmüen azonosítható a típus megadásának a hiányában is, akkor azt nem szükséges megadni.
    /// Ha nem adtunk meg enumerációs típust, és a lekérdezés eredménye nem egyértelmű, akkor SQL hibát kapunk (kizárást dob).
    static QString title(QSqlQuery& q, const QString& _ev, const QString& _tn);
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

protected:
    /// Az összes enum_vals rekordot tartalmazó konténer
    /// Nem frissül automatikusan, ha változik az adattábla.
    static QList<cEnumVal *>                        enumVals;
    /// A típus, és név alapján az enumVals elemei.
    static QMap<QString, QVector<cEnumVal *> >      mapValues;
    /// Egy üres objektumra mutató pointer.
    /// Az enumVals feltöltésekor hozza létre az abjektumot a fetchEnumVals();
    static cEnumVal *pNull;
public:
    /// Feltölti az adatbázisból az enumVals adattagot, ha nem üres a konténer, akkor frissíti.
    /// Iniciaéizálja a pNull pountert, ha az NULL. Egy üres objektumra fog mutatni.
    /// Frissítés esetén feltételezi, hogy rekordot törölni nem lehet, ezt ellenörzi.
    static void fetchEnumVals();
    /// Egy cEnumVal objektumot ad vissza a típus és érték alapján, ha nincs ilyen nevű objektum, akkor dob egy kizárást.
    /// Az enumVals adattagban keres, ha enumVals még nincs feltöltve, akkor feltülti.
    /// Ha __ex értéke EX_WARNING-nál kisebb, akkor kizárás helyett, egy üres objektummal tér vissza
    ///  (amire a pNull adattag mutat)
    /// @param _tn Az enumerációs típus neve
    /// @param e enumerációs érték, numerikus megfelelő, a -1 is értelmezve van mint típus
    static const cEnumVal& enumVal(const QString& _tn, int e, enum eEx __ex = EX_ERROR);
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
    static QString viewShort(const QString& _tn, int e, const QString &_d);
    /// A hívás lekéri az enumerációhoz tartozó beszédesebb nevet, ha van.
    /// Ha nincs ilyen érték, akkor amegadott alapértelmezett értékkel tér vissza.
    /// @param _tn A típus név
    /// @param e A numerikus érték
    /// @param _d Ha nincs rövid név, akkor ezzel tér vissza
    static QString viewLong(const QString& _tn, int e, const QString &_d);
    static QString toolTip(const QString& _tn, int e)
        { return enumVal(_tn, e).getName(_ixToolTip); }

    STATICIX(cEnumVal, ixTypeName)
    STATICIX(cEnumVal, ixValName)
    STATICIX(cEnumVal, ixBgColor)
    STATICIX(cEnumVal, ixFgColor)
    STATICIX(cEnumVal, ixToolTip)
    STATICIX(cEnumVal, ixViewShort)
    STATICIX(cEnumVal, ixViewLong)
    STATICIX(cEnumVal, ixFontFamily)
    STATICIX(cEnumVal, ixFontAttr)
};

enum eDataCharacter {
    DC_INVALID = -1,
    DC_HEAD    = 0,
    DC_DATA,
    DC_ID,
    DC_NAME,
    DC_PRIMARY,
    DC_KEY,
    DC_FNAME,
    DC_DERIVED,
    DC_TREE,
    DC_FOREIGN,
    DC_NULL,
    DC_DEFAULT,
    DC_AUTO,
    DC_INFO,
    DC_WARNING,
    DC_ERROR,
    DC_NOT_PERMIT,
    DC_HAVE_NO
};

EXT_ int dataCharacter(const QString& n, eEx __ex = EX_ERROR);
EXT_ const QString& dataCharacter(int e, eEx __ex = EX_ERROR);

static inline QString dcViewShort(int id) { return cEnumVal::viewShort(_sDatacharacter, id, dataCharacter(id)); }
static inline QString dcViewLong(int id)  { return cEnumVal::viewLong( _sDatacharacter, id, dataCharacter(id)); }

class LV2SHARED_EXPORT cMenuItem : public cRecord {
    CRECORD(cMenuItem);
    FEATURES(cMenuItem)
public:
    bool fetchFirstItem(QSqlQuery &q, const QString &_appName, qlonglong upId = NULL_ID);
    int delByAppName(QSqlQuery &q, const QString &__n, bool __pat = false) const;
/*
    static const cMenuItem& nullItem();
private:
    static cMenuItem *pNullItem;
*/
};

#endif // GUIDATA_H
