#ifndef GUIDATA_H
#define GUIDATA_H

/*!
@file guidata.h
A megjelenítéssel kapcsolatos adattáblákat kezelő objektumok.
*/

#include "lanview.h"
#include "lv2datab.h"

/// @enum eTableShapeType Tábla Shape típusok (set)
enum eTableShapeType {
    TS_UNKNOWN =-1, ///< ismeretlen, csak hibajelzésre
    TS_NO = 0,      ///< "no"       Csak diakógus (egy rekord megjelenítése, módosítása ill. beszúrása)
    TS_SIMPLE,      ///< "simple'   Egyszerű
    TS_TREE,        ///< "tree"     fa strukrúra
    TS_OWNER,       ///< "owner"
    TS_CHILD,       ///< "child"
    TS_SWITCH,      ///< "switch"   Kapcsoló tábla'
    TS_LINK         ///< "link"     Hasonló a kapcsoló táblához, de egy tábla (és leszármazottai) közötti kapcsolatot reprezentál (linkek)
};
EXT_ int tableShapeType(const QString& n, bool __ex = true);
EXT_ const QString&       tableShapeType(int e, bool __ex = true);

enum eTableInheritType {
    TIT_UNKNOWN = -1,
    TIT_NO = 0,
    TIT_ONLY,
    TIT_ON,
    TIT_ALL,
    TIT_REVERSE,
    TIT_LISTED,
    TIT_LISTED_REV,
    TIT_LISTED_ALL
};
EXT_ int tableInheritType(const QString& n, bool __ex = true);
EXT_ const QString& tableInheritType(int e, bool __ex = true);

/// @enum eOrderType Rendezési lehetőségek
enum eOrderType {
    OT_UNKNOWN =-1, ///< ismeretlen, csak hibajelzésre
    OT_NO = 0,   ///< Nincs rendezés
    OT_ASC,      ///< Rendezés növekvő sorrendbe
    OT_DESC,     ///< Rendezés csökkenő sorrendbe
    OT_DEFAULT,  ///< Az előző, vagy az alapértelmezett metódus megtartása (a string konvertáló függvény nem kezeli!)
    OT_NEXT      ///< A következő metódus (NO->INC->DEC) (a string konvertáló függvény nem kezeli!)
};
EXT_ int orderType(const QString& n, bool __ex = true);
EXT_ const QString&  orderType(int e, bool __ex = true);


/// @enum eFilterType Szűrési metódusok
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
    FT_DEFAULT,  ///< Az előző, vagy az alapértelmezett metódus megtartása (a string konvertáló függvény nem kezeli!)
    FT_NO        ///< nincs szűrés
};
EXT_ int filterType(const QString& n, bool __ex = true);
EXT_ const QString&   filterType(int e, bool __ex = true);

class cTableShapeField;
typedef tRecordList<cTableShapeField> tTableShapeFields;
/*!
@class cTableShape
@brief Adattáblák táblázatos megjelenítésének a leírója
*/
class LV2SHARED_EXPORT cTableShape : public cRecord {
    template <class S> friend void _SplitMagicT(S& o, bool __ex);
    CRECORD(cTableShape);
public:
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual bool insert(QSqlQuery &__q, bool __ex = true);
    int fetchFields(QSqlQuery& q);
    /// Az objektumhoz feltölti a ShapeFields konténert, default értékekkel.
    /// Az adatokat az adatbázisban nem tárolja el, feltételezi, hogy az adattábla neve ismert.
    /// Ha a tábla nem létezik, akkor dob egy kizárást.
    bool setDefaults(QSqlQuery &q);
    /// Egy érték beállítása a megadott nevű mező leírójában
    /// @param _fn A mező neve a táblában, melynek a leíróta az objektum
    /// @param _fpn A mező leíró paraméterének a neve
    /// @param _v a beállítandó érték
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező, vagy paraméter) dob egy kizárást.
    /// @return Ha beállította az értéket, akkor true.
    bool fset(const QString& _fn, const QString& _fpn, const QVariant& _v, bool __ex = true);
    /// Egy érték beállítása a megadott mezők leíróiban
    /// @param _fnl A mezők nevei a táblában, melynek a leíróta az objektum
    /// @param _fpn A mező leíró paraméterének a neve
    /// @param _v a beállítandó érték
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező, vagy paraméter) dob egy kizárást.
    /// @return Ha beállította minden megadott mezőre az értéket, akkor true.
    bool fsets(const QStringList& _fnl, const QString& _fpn, const QVariant& _v, bool __ex = true);
    /// Egy új szűrő objektum hozzáadáse a mező leíróhoz
    /// @param _fnl A mező neve a táblában, melynek a leírója az objektum
    /// @param _t A szűrő típusa
    /// @param _d A szűrő megjelenített neve/leírása
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező) dob egy kizárást.
    /// @return Ha beállította minden megadott mezőre az értéket, akkor true.
    bool addFilter(const QString& _fn, const QString& _t, const QString& _d, bool __ex = true);
    /// Egy új szűrő objektum hozzáadáse a mező leíróhoz
    /// @param _fnl A mezők nevei a táblában, melynek a leíróta az objektum
    /// @param _t A szűrő típusa
    /// @param _d A szűrő megjelenített neve/leírása
    /// @param __ex Ha értéke true, akkor hiba esetén (nincs ilyen nevű mező) dob egy kizárást.
    /// @return Ha beállította minden megadott mezőre az értéket, akkor true.
    bool addFilter(const QStringList& _fnl, const QString& _t, const QString& _d, bool __ex = true);
    bool addFilter(const QStringList& _fnl, const QStringList& _ftl, bool __ex = true);
    bool setAllOrders(QStringList& _ord, int last = 0, bool __ex = true);
    bool setOrders(const QStringList& _fnl, QStringList& _ord, int last = 0, bool __ex = true);
    bool setFieldSeq(const QStringList& _fnl, int last = 0, bool __ex = true);
    bool setOrdSeq(const QStringList& _fnl, int last = 0, bool __ex = true);

    /// A rekordban a atrubutes nezőt vágja szát, és az elemeket elhelyezi a pMagicMap pointer által mutatott konténerbe.
    /// Ha pMagicMap egy NULL pointer, akkor a művelet elött megallokálja a konténert, ha nem NULL, akkor pedig törli a konténer tartalmát.
    tMagicMap& splitMagic(bool __ex = true);
    /// Visszaadja a pMagicMap által mutatott konténer referenciáját. Ha pMagicMap értéke NULL, akkor hívja a splitMagic() metódust, ami megallokálja
    /// és feltölti a konténert.
    tMagicMap& magicMap(bool __ex = true)                       { if (_pMagicMap == NULL) splitMagic(__ex); return *_pMagicMap; }
    /// A megadott kulcs alapján visszaadja a magicMap konténerből a paraméter értéket a név alapján. Ha a konténer nincs megallokálva, akkor megallokálja
    /// és feltölti.
    /// @return ha a kulcshoz tartozik megadott néven paraméter, akkor az értéket adja vissza, vagy üres stringet.
    QString magicParam(const QString& __nm, bool __ex = true)   { return ::magicParam(__nm, magicMap(__ex)); }
    /// Magadot kulcsal egy paraméter keresése.
    /// @return találat esetén true.
    bool findMagic(const QString &_nm, bool __ex = true)        { return ::findMagic(_nm, magicMap(__ex)); }
    ///
    bool typeIs(eTableShapeType _t) const { return getId(_sTableShapeType) & enum2set(_t); }
    bool fetchLeft(QSqlQuery& q, cTableShape * _po, bool _ex) const;
    bool fetchRight(QSqlQuery& q, cTableShape * _po, bool _ex) const;
    const cRecStaticDescr * getRecDescr() const { return cRecStaticDescr::get(getName(_sTableName)); }
    tTableShapeFields       shapeFields;
protected:
    static int              _ixProperties;
    /// magicMap konténer, vagy null pointer, ha még nincs feltöltve
    tMagicMap          *_pMagicMap;
};

class cTableShapeFilter;
typedef tRecordList<cTableShapeFilter> tTableShapeFilters;

/*!
@class cTableShapeField
@brief Adattáblák mezőinek a megjelenítési leírói
*/
class LV2SHARED_EXPORT cTableShapeField : public cRecord {
    template <class S> friend void _SplitMagicT(S& o, bool __ex);
    CRECORD(cTableShapeField);
public:
    cTableShapeField(QSqlQuery& q);
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual bool insert(QSqlQuery &__q, bool __ex);
    int fetchFilters(QSqlQuery& q);
    bool addFilter(const QString& _t, const QString& _d, bool __ex = true);
    /// A rekordban a atrubutes nezőt vágja szát, és az elemeket elhelyezi a pMagicMap pointer által mutatott konténerbe.
    /// Ha pMagicMap egy NULL pointer, akkor a művelet elött megallokálja a konténert, ha nem NULL, akkor pedig törli a konténer tartalmát.
    tMagicMap& splitMagic(bool __ex = true);
    /// Visszaadja a pMagicMap által mutatott konténer referenciáját. Ha pMagicMap értéke NULL, akkor hívja a splitMagic() metódust, ami megallokálja
    /// és feltölti a konténert.
    tMagicMap& magicMap(bool __ex = true)                       { if (_pMagicMap == NULL) splitMagic(__ex); return *_pMagicMap; }
    /// A megadott kulcs alapján visszaadja a magicMap konténerből a paraméter értéket a név alapján. Ha a konténer nincs megallokálva, akkor megallokálja
    /// és feltölti.
    /// @return ha a kulcshoz tartozik megadott néven paraméter, akkor az értéket adja vissza, vagy üres stringet.
    QString magicParam(const QString& __nm, bool __ex = true)   { return ::magicParam(__nm, magicMap(__ex)); }
    /// Magadot kulcsal egy paraméter keresése.
    /// @return találat esetén true.
    bool findMagic(const QString &_nm, bool __ex = true)        { return ::findMagic(_nm, magicMap(__ex)); }
    tTableShapeFilters  shapeFilters;
protected:
    static cRecStaticDescr  _staticDescr;
    static int _ixProperties;
    /// magicMap konténer, vagy null pointer, ha még nincs feltöltve
    tMagicMap          *_pMagicMap;
};

class LV2SHARED_EXPORT cTableShapeFilter : public cRecord {
    CRECORD(cTableShapeFilter);
};

/// @class cEnumVal
/// @brief Enumerációs értékekhez opcionálisan tartozó beszédesebb szövegeket tartalmazó tábla.
class LV2SHARED_EXPORT cEnumVal : public cRecord {
    CRECORD(cEnumVal);
public:
    /// A hívás lekéri az enumerációhoz tartozó beszédesebb nevet, ha van.
    /// Ha nincs ilyen érték, akkor az enumerációs értékkel tér vissza.
    /// Ha az enumerációs érték egyértelmüen azonosítható a típus megadásának a hiányában is, akkor azt nem szükséges megadni.
    /// Ha en adtunk meg enumerációs típust, és a lekérdezés eredménye nem egyértelmű, akkor SQL hibát kapunk (kizárást dob).
    static QString title(QSqlQuery& q, const QString& _ev, const QString& _tn = _sNul);
    /// Rekord(ok) törlésa az enumeráció típus név alapján
    /// @param  q
    /// @param __n Az enum_type_name mező értéke, vagy minta a mezőre
    /// @param __pat Ha értéke true, akkor az __n paraméter minta stringnek tekintendő.
    /// @return a törölt rekordok száma
    int delByTypeName(QSqlQuery& q, const QString& __n, bool __pat = false);
    /// Rekord(ok) törlésa az enumeráció típus név alapján
    /// @param  q
    /// @param __t Az enum_type_name mező értéke
    /// @param __n Az enum__name mező értéke
    /// @return true ha törölt rekordt
    bool delByNames(QSqlQuery& q, const QString& __t, const QString& __n);
};

class LV2SHARED_EXPORT cMenuItem : public cRecord {
    CRECORD(cMenuItem);
public:
    QString title() const { return isNull(_sMenuItemTitle) ? getName(_sMenuItemName) : getName(_sMenuItemTitle); }
    bool fetchFirstItem(QSqlQuery &q, const QString &_appName, qlonglong upId = NULL_ID);
    int delByAppName(QSqlQuery &q, const QString &__n, bool __pat = false) const;
};

#endif // GUIDATA_H
