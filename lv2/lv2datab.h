#ifndef LV2DATAB_H
#define LV2DATAB_H
#include "lv2_global.h"
#include "strings.h"
#include "cdebug.h"
#include "cerror.h"
#include "lv2types.h"
#include "lv2sql.h"
#include "others.h"
#include <typeinfo>

/*!
@file lv2datab.h
Az adatbázis interfész bázis objektuma, és a rekord ill. mező leíró/kezelő objektumok.
*/

/* ********** BASE ********** BASE ********** BASE ********** BASE ********** BASE ********** BASE ********** BASE ********** */
EXT_  bool metaIsInteger(int id);
EXT_  bool variantIsInteger(const QVariant & _v);
EXT_  bool metaIsString(int id);
EXT_  bool variantIsString(const QVariant & _v);
EXT_  bool metaIsFloat(int id);
EXT_  bool variantIsFloat(const QVariant & _v);
EXT_  bool variantIsNum(const QVariant & _v);

/// Egy stringet bool-lá konvertál:
/// Ha a string (b) értéke "t", "true", "y", "yes", "on" vagy "1", akkor true-val tér vissza,
/// Ha a string (b) értéke "f", "false", "n", "no", "off" vagy "0", akkor false-val tér vissza,
/// Egyébb értékeknél, ha __ex true, akkor dob egy kizárást, ha viszont __ex értéke hamis, akkor
/// üres string esetén false, ellenkező esetben true a visszaadott érték.
bool str2bool(const QString& _b, bool __ex = true);
/// A bool bol stringet csinál, a nyelvi beállításoknak megfelelően.
QString langBool(bool b);

/// A megadott nevű shchema OID-jével tér vissza.
/// Ha nincs ilyen schema, vagy hiba történt dob egy kizárást
qlonglong schemaoid(QSqlQuery q, const QString& __s);
/// A tableoid lekérdezése, hiba esetén dob egy kizárást. Ha nincs ilyen tábla akkor is kizárást dob.
/// @param _t SQL tábla neve
/// @param _sid SQL tábla schema oid (vagy NULL_ID, ha az alapértelmezett "public" a schema)
/// @param __ex Ha értéke true, és nem létezik a megadott nevű tábla, akkor a NULL:IS-vel tér vissza, és nem dob kizárást.
/// @return a tabloid
qlonglong tableoid(QSqlQuery q, const QString& __t, qlonglong __sid = NULL_ID, bool __ex = true);
/// A tableoid érték alapján visszaadja a tábla és schema nevet, ha nincs ilyen tábla, vagy hiba esetén dob egy kizárást.
/// @return A first adattag a tábla neve, a second pedig a schema név.
QStringPair tableoid2name(QSqlQuery q, qlonglong toid);

class cRecord;
class cRecordFieldRef;
class cRecordFieldConstRef;
template <class T>  class pRecordLis;
class cAlternate;

/* ******************************************************************************************************
   *                                         cColStaticDescr                                            *
   ******************************************************************************************************/
class cRecord;

/// @class cColStaticDescr
/// SQL tábla mező tulajdonságait tartalmazó objektum
/// A virtuális metódusok végzik a mező adatkonverzióit.
/// Az QString ős objektum a mező nevét tartalmazza
class LV2SHARED_EXPORT cColStaticDescr : public QString {
public:
    enum eFieldType {
        FT_ARRAY    = 0x40000000L,
        FT_ANY      =          0,
        FT_TIME,
        FT_DATE,
        FT_DATE_TIME,
        FT_INTERVAL,
        FT_INTEGER,
        FT_REAL,
        FT_TEXT,
        FT_BINARY,
        FT_BOOLEAN,
        FT_POLYGON,
        FT_ENUM,
        FT_SET          = FT_ENUM + FT_ARRAY,
        FT_INTEGER_ARRAY= FT_INTEGER + FT_ARRAY,
        FT_REAL_ARRAY   = FT_REAL + FT_ARRAY,
        FT_TEXT_ARRAY   = FT_TEXT + FT_ARRAY,
        FT_MAC      = 0x00000100L,
        FT_INET,
        FT_CIDR,
        FT_ADDR_MASK= 0x00000300L,
        FT_NEXT_TYPE= 0x00001000L
    };
    enum eFKeyType {
        FT_NONE     = 0,            /// Nem távoli kulcs
        FT_PROPERTY = FT_NEXT_TYPE, /// A távoli kulcs egy tulajdonságot reprezebtéló rekordra mutat
        FT_OWNER,                   /// A távoli kulcs egy tuljdonos objektumra nutat
        FT_SELF                     /// Önmagára mutató távoli kulcs (pl. fa strukturát reprezentál)
    };

    /// Konstruktor, egy üres objetumot hoz létre
    cColStaticDescr(int __t = FT_ANY);
    /// Copy konstruktor
    cColStaticDescr(const cColStaticDescr& __o);
    /// Másoló operátor
    cColStaticDescr& operator=(const cColStaticDescr __o);
    /// A QString ős objektum referenciájával tér vissza, ami a mező neve
    QString&        colName()       { return *(QString *)this; }
    /// A QString ős objektum konstatns referenciájával tér vissza, ami a mező neve
    const QString&  colName() const { return *(QString *)this; }
    /// A mező név idézőjelek között
    QString colNameQ() const        { return quotedString(colName()); }
    /// Kideríti a mező típusát a szöveges típus nevek alapján, vagyis meghatározza az eColType adattag értékét.
    void typeDetect();
    // virtual
    /// Az adatbázisból beolvasott adatot konvertálja a rekord konténerben tárolt formára.
    virtual QVariant fromSql(const QVariant& __f) const;
    /// A rekord konténerben tárolt formából konvertálja az adatot, az adatbázis kezelőnek átadható formára.
    virtual QVariant toSql(const QVariant& __f) const;
    /// Értékadáskor végrehajtandó esetleges konverziók
    /// @param _f Forrás adat
    /// @param rst A rekord status referenciája
    virtual QVariant set(const QVariant& _f, int& str) const;
    /// Konverzió érték kiolvasásakor, ha az eredményt string formában kérjük
    virtual QString toName(const QVariant& _f) const;
    /// Konverzió érték kiolvasásakor, ha az eslredményt integer formában kérjük
    virtual qlonglong toId(const QVariant& _f) const;
    ///
    virtual QString toView(QSqlQuery& q, const QVariant& _f) const;
    /// Clone object
    virtual cColStaticDescr *dup() const;
    // Adattagok
    /// A maző poziciója, nem zeró pozitív szám (1,2,3...)
    int         pos;
    /// A mező sorrendet jelző szám (ordinal_position), jó esetben azonos pos-al, de ha mező volt törölve, akkor nem folytonos
    int         ordPos;
    /// A mező típusa.
    /// Ha a mező tömb, vagy felhasználó által definiált típus, akkor a
    /// colType értéke "ARRAY" ill. "USER-DEFINED"
    QString     colType;
    /// Szintén típus, tömb ill. USER DEFINED típus esetén ez tartalmazza a típus nevet.
    QString     udtName;
    /// A mező alaértelmezett értéke
    QString     colDefault;
    /// Ha igaz, akkor a mező felveheti a NULL slértéket
    bool        isNullable;
    /// Enumerációs típus esetén az értékkészlet
    QStringList enumVals;
    /// A mező (oszlop) typusa. A fordítónál szabadabb értelmezésben a member típusa: eFieldType
    int         eColType;
    /// Ha a mező módosítható, akkor értéke true
    bool        isUpdatable;
    /// Ha a mező egy távoli kulcs, akkor a tábla schema neve, ahol távoli kulcs
    QString     fKeySchema;
    /// Ha a mező egy távoli kulcs, akkor a tábla neve, ahol távoli kulcs
    QString     fKeyTable;
    /// Ha a mező egy távoli kulcs, akkor a kulcsmező neve, a távoli táblában
    QString     fKeyField;
    /// A távoli kulcs típusa
    eFKeyType   fKeyType;
    /// Ha a mező egy távoli kulcs, akkor a táblák nevei, ahol távoli kulcs, ha egy öröklési léncról van szó
    QStringList fKeyTables;
    /// Egy opcionális függvény név, ami ID-k esetén elvégzi a stringgé (névvé) konvertálást
    QString     fnToName;
    /// Az objektum tartalmát striggé konvertálja pl. nyomkövetésnél használható.
    QString toString() const;
    /// Ha a mező egy távoli kulcs, és nem önmagára hivatkozik, akkor a hivatkozott objektumra mutat, egyébként NULL.
    cAlternate *pFRec;
};
TSTREAMO(cColStaticDescr)

/// @class cColStaticDescrBool
/// Az ós cColStaticDescr osztályt a boolean típus konverziós függvényivel egészíti ki.
class LV2SHARED_EXPORT cColStaticDescrBool : public cColStaticDescr {
public:
    /// A konstruktor kitölti a enumVals konténert is, hogy enumerációként is kezelhető legyen
    cColStaticDescrBool(const cColStaticDescr& __o) : cColStaticDescr(__o) { init(); }
    /// A konstruktor kitölti a enumVals konténert is, hogy enumerációként is kezelhető legyen
    cColStaticDescrBool(int t) : cColStaticDescr(t) { init(); }
    virtual QVariant  fromSql(const QVariant& __f) const;
    virtual QVariant  toSql(const QVariant& __f) const;
    virtual QVariant  set(const QVariant& _f, int &str) const;
    virtual QString   toName(const QVariant& _f) const;
    virtual qlonglong toId(const QVariant& _f) const;
    virtual QString toView(QSqlQuery&, const QVariant& _f) const;
    virtual cColStaticDescr *dup() const;
private:
    void init();
};

/// @def CSD_INHERITOR(T)
/// Egy cColStaticDescr osztályból származtatott osztály deklarációja, melyben csak a
/// virtuális metódusok lesznek módossítva.
/// A makróban a class definíció nincs lezárva a '}' parakterrel!
#define CSD_INHERITOR(T) \
    class LV2SHARED_EXPORT T : public cColStaticDescr { \
        public: \
        T(int t) : cColStaticDescr(t) { ; } \
        T(const cColStaticDescr& __o) : cColStaticDescr(__o) { ; } \
        virtual QVariant  fromSql(const QVariant& __f) const; \
        virtual QVariant  toSql(const QVariant& __f) const; \
        virtual QVariant  set(const QVariant& _f, int &rst) const; \
        virtual QString   toName(const QVariant& _f) const; \
        virtual qlonglong toId(const QVariant& _f) const; \
        virtual cColStaticDescr *dup() const;

/// @class cColStaticDescrAddr
/// Az ós cColStaticDescr osztályt az macaddr, inet és cidr típus konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrAddr)
};

/// @class cColStaticDescrArray
/// Az ós cColStaticDescr osztályt a álltatános tömb típus konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrArray)
virtual QString toView(QSqlQuery& q, const QVariant& _f) const;
};

/// @class cColStaticDescrPolygon
/// Az ós cColStaticDescr osztályt a polygon típus konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrPolygon)
};

/// @class cColStaticDescrEnum
/// Az ós cColStaticDescr osztályt autómatikusan kezelt enumeráció konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrEnum)
};

/// @class cColStaticDescrSet
/// Az ós cColStaticDescr osztályt autómatikusan kezelt set (enumeráció tömb) konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrSet)
};

/// @class cColStaticDescrDate
/// Az ós cColStaticDescr osztályt dátum konverziós függvényivel egészíti ki.
/// Nincs implementálva.
CSD_INHERITOR(cColStaticDescrDate)
};

/// @class cColStaticDescrTime
/// Az ós cColStaticDescr osztályt idő konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrTime)
};

/// @class cColStaticDescrDateTime
/// Az ós cColStaticDescr osztályt dátum és idő (timestamp) konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrDateTime)
};

/// @class cColStaticDescrInterval
/// Az ós cColStaticDescr osztályt idő intervallum konverziós függvényivel egészíti ki.
/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Negatív tartomány nincs értelmezve, és az óra:perc:másodperc formán túl csak a napok megadása támogatott.
CSD_INHERITOR(cColStaticDescrInterval)
};

/// Egy adatbázisból beolvasott bool értéket kovertál stringgé
inline static const QString& boolFromSql(const QVariant& __f) { return __f.isNull() ? _sNul : str2bool(__f.toString(), false) ? _sTrue : _sFalse; }
/// Egy adatbázisból beolvasott text tömböt értéket kovertál string listává
inline static QStringList stringArrayFromSql(const QVariant& __f) { return cColStaticDescrArray(cColStaticDescr::FT_TEXT | cColStaticDescr::FT_ARRAY).fromSql(__f).toStringList(); }
inline static QString stringArrayFromSql(const QVariant& __f, const QString& __s)   { return stringArrayFromSql(__f).join(__s); }
inline static QString stringArrayFromSql(const QVariant& __f, const QChar& __s)     { return stringArrayFromSql(__f).join(__s); }


/* ******************************************************************************************************
   *                                         cColStaticDescrList                                        *
   ******************************************************************************************************/
/// @class cColStaticDescrList
/// SQL tábla mezőinek listálya, (egy tábla mezőinek a tulajdonságát leíró lista)
/// A listában a mezők kötött sorrendben szerepelnek.
class LV2SHARED_EXPORT cColStaticDescrList : public QList<cColStaticDescr *> {
public:
    typedef QList<cColStaticDescr *>::iterator          iterator;
    typedef QList<cColStaticDescr *>::const_iterator    const_iterator;
    typedef QList<cColStaticDescr *>                    list;
    /// üres listát létrehozó konstruktor
    cColStaticDescrList();
    /// Nincs copy
    cColStaticDescrList(const cColStaticDescrList&) : QList<cColStaticDescr *>() { EXCEPTION(ENOTSUPP); }
    /// Destruktor
    virtual ~cColStaticDescrList();
    /// Az operátor egy mező leírót hozzáad a listához.
    /// @exception cError* Csak sorrendben adhatóak a listához a mezők, vagyis az __o.ordPos -nak meg kell
    /// egyeznie a lista új (elem hozzáadása utáni) méretével.
    cColStaticDescrList& operator<<(cColStaticDescr *__o);
    /// Hasonló a pointeres operátor változathoz, de a copy konstruktorral létrehozott objektumot illeszti be.
    cColStaticDescrList& operator<<(const cColStaticDescr& __o) { (*this) << new cColStaticDescr(__o); return *this; }
    /// Megketesi az elemet a listában
    /// @param __n a keresett mező neve
    /// @return ha van ilyen nevű mező a listában, akkor annak az indexe, ha nincs, akkor -1.
    int toIndex(const QString& __n, bool __ex = true) const;
    /// A megadott nevű mező leírójánask a referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    cColStaticDescr& operator[](const QString& __n);
    /// A megadott nevű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    const cColStaticDescr& operator[](const QString& __n) const;
    /// A megadott indexű mező leírójánask a referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param i a keresett mező indexe (0,1,...).
    cColStaticDescr& operator[](int i);
    /// A megadott indexű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param i a keresett mező indexe (0,1,...).
    const cColStaticDescr& operator[](int i) const;
};

/* ******************************************************************************************************
   *                                         cRecStaticDescr                                            *
   ******************************************************************************************************/
/*! @enum eTableType
   Tábla típus konstansok.
   Az egyszerű switch típusú táblához nem tartozik konstans, mert azokat nem cRecord típusú objektummal kezeljük.
 */
typedef enum {
    UNKNOWN_TABLE,
    BASE_TABLE,     ///< Egyszerű tábla
    VIEW_TABLE,     ///< Nézet tábla
    SWITCH_TABLE,   ///< Switch tábla + tuajdonságok
    LINK_TABLE,     ///< Link tábla (+ szimmetrikus VIEW)
    CHILD_TABLE     ///< A tábla olyan objektumot ír le, mely egy másik tilajdona
}   eTableType;

/// Egy rekord tulajdonságait leíró objektum. A rekord adat objektumban egy statikus adattag.
class LV2SHARED_EXPORT cRecStaticDescr {
    friend class cRecord;
protected:
    /// Az eddíg létrehozott, és inicializált objektumok konténere, a tábla OID-vel kulcsolva
    static QMap<qlonglong, cRecStaticDescr *>   _recDescrMap;
    /// A _recDescrMap konténerrel végzett műveleteket védő mutex objektum
    static QMutex   _mapMutex;
    /// Ha a bármelyik mező módosítható, akkor értéke true
    bool                _isUpdatable;
    /// Az adattábla schema neve, vagy Null (schema name = 'public')
    QString             _schemaName;
    /// Az adattábla neve
    QString             _tableName;
    /// Opcionális nézet tábla neve. Ha nem null. akkor a fetch-ek erre a táblára vonatkoznak.
    QString             _viewName;
    /// A tábla OID-je
    qlonglong           _tableOId;
    /// A schema (namespace) OID-je
    qlonglong           _schemaOId;
    /// Az ID mező indexe, vagy -1, ha nincs, vagy nem ismert
    int                 _idIndex;
    /// Az név mező indexe, vagy -1, ha nincs, vagy nem ismert
    int                 _nameIndex;
    /// Az descr (title) mező indexe, vagy -1, ha nincs, vagy nem ismert
    int                 _descrIndex;
    /// A deleted mező indexe, vagy -1, ha nincs ilyen mező
    int                 _deletedIndex;
    /// A tábla tulajdonságát leíró adatrekord kerül ide beolvasásra
    QSqlRecord          _tableRecord;
    /// A tábla oszlopainak tulajdonságait leíró adatrekordok kerülnek ide beolvasásra.
    QList<QSqlRecord>   _columnRecords;
    /// Az adattábla oszlopait leíró objektumok listálya
    cColStaticDescrList _columnDescrs;
    /// Az adattábla típusa
    eTableType          _tableType;
    /// Az objektumban a Primary Key-hez rendelt mezökkel azonos indexű bitek 1-be vannak állítva.
    /// Ha nincs Primary Key, akkor nincs egyes bit a tömbben.
    QBitArray           _primaryKeyMask;
    /// A Lista elemei hasonlóak az _primaryKeyMask-hoz, de az Egyedi kulcsokhoz tartozó
    /// mezőket azonosítja.
    QList<QBitArray>    _uniqueMasks;
    /// Az auto inkrementális mezők maszkja
    QBitArray           _autoIncrement;
    /// Az oszlopok száma a táblában
    int                 _columnsNum;
    /// Ha származtatott rekordról van szó, akkor az ős rekord leíróinak a pointerei
    QVector<const cRecStaticDescr *>  _parents;
    /// Ha a rekord őse más rekordoknak
    bool                _isParent;
    /// Inicializálja az objektumot, és _inited értékét true-ra állítja.
    /// @exception cError* Hiba esetén dob egy kizárást-
    /// @param _t SQL tábla neve
    /// @param _s SQL tábla schema név, opcionális, ha nincs megadva, akkor a schema neve 'public'
    void _set(const QString& __t, const QString& __s = QString());
    /// A megadott nevű mező leírójánask a referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    cColStaticDescr& operator[](const QString& __n) { return _columnDescrs[__n]; }
    /// A megadott indexű mező leírójánask a referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __i a keresett mező indexe (0,1,...).
    cColStaticDescr& operator[](int __i)            { return _columnDescrs[__i]; }
public:
    /// Üres objektum konstruktor, _inited adattag értéke false lessz.
    cRecStaticDescr(const QString& __t, const QString& __s = QString());
    /// Destruktor
    ~cRecStaticDescr();
    /// Egy objektum "beszerzése". Ha a _recDescrMap, vagy a _recDescCache konténerben megtalálja az objektumot, akkor a pointerével tér vissza.
    /// Ha nem, akkor létrehozza, inicializálja, beteszi a _recDescCache konténerbe, és a pointerrel tér vissza.
    /// @param _t SQL tábla neve
    /// @param _s SQL tábla schema név, opcionális
    /// @param find_only Ha true, akkor NULL pointerrel tér vissza
    /// @exception cError * Ha _setReCallCnt értéke nagyobb vagy egyenlő mint 10, és objektumot kellene kreálni, akkor dob egy kizárást.
    static const cRecStaticDescr *get(const QString& _t, const QString& _s = QString(), bool find_only = false);
    /// Egy objektum "beszerzése". Ha a _recDescrMap, vagy a _recDescCache konténerben megtalálja az objektumot, akkor a pointerével tér vissza.
    /// Ha nem, akkor létrehozza, inicializálja, beteszi a _recDescCache konténerbe, és a pointerrel tér vissza.
    /// @param _oid a tábla OID-je
    /// @param find_only Ha true, akkor NULL pointerrel tér vissza
    /// @exception cError * Ha _setReCallCnt értéke nagyobb vagy egyenlő mint 10, és objektumot kellene kreálni, akkor dob egy kizárást.
    static const cRecStaticDescr *get(qlonglong _oid, bool find_only = false);
    /// A megadott tábla esetén megnézi, hogy létezik-e a rekord ID-t névvé konvertáló függvény.
    /// Ha létezik, akkor a nevével tér vissza. Ha nem, akkor megkisérli megkreálni.
    /// Ha a megadott tábla descriptora még nem létezik, és __ex false, akkor egy üres stringgel tér vissza.
    static QString checkId2Name(QSqlQuery &q, const QString& _tn, const QString& _sn, bool __ex = true);
    /// Az index alapján vissza adja az oszlop leíró referenciáját.
    const cColStaticDescr& colDescr(int i) const { chkIndex(i); return _columnDescrs[i]; }
    /// Ha bármelyik mező módosítható az adatbázisban, akkor true-val tér vissza.
    bool isUpdatable() const                    { return _isUpdatable; }
    /// Ha a megadott indexű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen indexű mező, dosb egy kizárást.
    bool isUpdatable(int i) const               { return colDescr(i).isUpdatable; }
    /// Ha a megadott nevű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen nevű mező, dosb egy kizárást.
    bool isUpdatable(const QString& fn) const   { return colDescr(toIndex(fn)).isUpdatable; }
    /// Ellenörzi a megadott mező indexet
    /// @param i Az ellenőrizendő mező/oszlop index érték
    /// @param __ex Ha értéke true, akkor a false visszatérési érték helyett dob egy kizárást, alapértéemezetten a paraméter false.
    /// @return ha i egy valós index, akkor true, ha nem false
    bool isIndex(int i, bool __ex = false) const { bool r = (i >= 0 && i < _columnsNum); if (__ex && !r) EXCEPTION(ENOINDEX, i, fullTableName()); return r; }
    /// Ellenörzi, hogy a megadott név egy mezőnév-e
    /// @param __fn Az ellenőrizendő mező/oszlop név
    /// @return ha __fn egy valós mező név, akkor true, ha nem false
    bool isFieldName(const QString& __fn) const     { return isIndex(toIndex(__fn, false)); }
    /// Ellenörzi a megadott mező/oszlop indexet.
    /// @param i Az ellenőrizendő index érték
    /// @return i értékével tér vissza.
    /// @exception cError* Ha i értéke nem lehet index, akkor dob egy kizárást
    int chkIndex(int i) const { isIndex(i, true); return i; }
    /// A megadott indexű mező nevével tér vissza.
    /// @exception cError* Ha i értéke nem lehet index. és __ex true (alapértelmezett), akkor dob egy kizárást
    const QString&  columnName(int i, bool __ex = true) const  { return isIndex(i, __ex) ? (const QString&)_columnDescrs[i] : _sNul; }
    /// A megadott indexű mező nevével tér vissza. Hasonló a _columnName() metódushoz, de a visszaadot nevet kettős idézőjelek közé teszi
    QString columnNameQ(int i) const                 { return dQuoted(columnName(i)); }
    /// Megkeresi a megadott nevű mezőt a mező leíró listában
    /// Ha a névben pontok vannak, akkor feltételezi, hogy a névben szerepel a tábla név esetleg a séma név is.
    /// Ha meg van adva a tábla ill. a séma név, akkor azokat ellenőrzi, és ha nem egyeznek -1-el lép ki.
    /// Ha a név (ill. nevek) kettős idézőjelben vannak, akkor az összehasonlításokat az idézőjelek nélkül végzi.
    /// @param __n A keresett mező neve
    /// @param __ex Ha értéke true, és nem találja a keresett mezőt, akkor dob egy kizárást.
    /// @return A mező indexe, vagy ha nem talált ilyen nevű mezőt, és __ex értéke false, akkor NULL_IX (negatív).
    int toIndex(const QString& __n, bool __ex = true) const;
    /// A schema nevet adja vissza
    const QString&  schemaName() const              { return _schemaName; }
    /// A tábla nevet adja vissza
    const QString&  tableName() const               { return _tableName; }
    /// A mező leíró lista referenciájával tér vissza
    const cColStaticDescrList& columnDescrs() const { return _columnDescrs; }
    /// A tábla OID értékkel tér vissza
    qlonglong tableoid() const                      { return _tableOId; }
    /// A tábla teljes nevével (ha szükséges, akkor a schema névvel kiegészített) tér vissza, a tábla és schema név nincs idézőjelbe rakva.
    QString fullTableName() const                   { return _schemaName == _sPublic ? _tableName : mCat(_schemaName, _tableName); }
    QString fullViewName() const                    { return  _viewName.isNull() ? fullTableName() : _schemaName == _sPublic ? _viewName : mCat(_schemaName, _viewName); }
    /// A teljes mező névvel (tábla és schema névqtcreator failed ptracevel kiegészített) tér vissza. a nevek nincsenek idézőjelbe rakva.
    /// @param _c (rövid) mező név
    QString fullColumnName(const QString& _c) const{ return mCat(fullTableName(), _c); }
    /// A tábla teljes nevével (schema névvel kiegészített) tér vissza, a tábla és schema név idézőjelbe van rakva.
    QString fullTableNameQ() const                   { return _schemaName == _sPublic ? dQuoted(_tableName) : dQuotedCat(_schemaName, _tableName); }
    /// A teljes mező névvel (tábla és schema névvel kiegészített) tér vissza. a nevek idézőjelbe vannak rakva.
    /// @param i a mező indexe
    QString fullColumnNameQ(int i) const             { return mCat(fullTableNameQ(), dQuoted(columnName(i))); }
    /// A teljes mező névvel (tábla és ha szükséges a schema névvel kiegészített) tér vissza. a nevek idézőjelbe vannak rakva.
    /// @param _c (rövid) mező név
    QString fullColumnNameQ(const QString& _c) const { return mCat(fullTableNameQ(), dQuoted(_c)); }
    /// A tábla típusával tér vissza
    eTableType tableType() const                    { return _tableType; }
    /// A visszaadot objektum referenciában a Primary Key-hez rendelt mezökkel azonos indexű bitek 1-be vannak
    /// állítva. Ha nincs Primary Key, akkor nincs egyes bit a tömbben.
    /// @return Az elsőbleges kulcsként használlt mezőket azonosító bit tömb objektum referenciája.
    const QBitArray& primaryKey() const             { return _primaryKeyMask; }
    /// A bit tömbök értelmezése hasonló mint a promaryKey() metódusnál.
    /// @return Az egyedi mezőket azonositó bit tümbök listálya
    const QList<QBitArray>& uniques() const         { return _uniqueMasks; }
    /// Egy bit vektorral tér vissza, ahol minden bit true, mellyel azonos indexű mező autoincrement.
    const QBitArray& autoIncrement() const          { return _autoIncrement; }
    /// Az ID mező indexével tér vissza, ha nincs ID, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int idIndex(bool _ex = true) const             { if (_ex && !isIndex(_idIndex)) EXCEPTION(EFOUND, _idIndex, "Nothing ID field."); return _idIndex; }
    /// Az NAME mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int nameIndex(bool _ex = true) const           { if (_ex && !isIndex(_nameIndex)) EXCEPTION(EFOUND, _nameIndex, "Nothing name field."); return _nameIndex; }
    /// Az DESCR mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int descrIndex(bool _ex = true) const          { if (_ex && !isIndex(_descrIndex)) EXCEPTION(EFOUND, _descrIndex, "Nothing descr field."); return _descrIndex; }
    /// Az DELETED mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int deletedIndex(bool _ex = true) const        { if (_ex && !isIndex(_deletedIndex)) EXCEPTION(EFOUND, _deletedIndex, "Nothing descr field."); return _deletedIndex; }
    /// A mezők számával tér vissza
    int cols() const                                { return _columnsNum; }
    /// A parent táblák leírójinak a pointerei, amnnyiben a tábla származtatott, egyébként NULL pointer
    const QVector<const cRecStaticDescr *>& parent(void) const { return _parents; }
    /// Összehasonlítja két leíró schema és tábla nevét.
    /// Ha a schema és táblanév azonos, akkor true-val tér vissza
    bool operator==(const cRecStaticDescr& __o) const { return _schemaName == __o._schemaName && _tableName == __o._tableName; }
    /// Összehasonlítja két leíró schema és tábla nevét.
    /// Ha a schema vagy táblanév nem azonos, akkor true-val tér vissza
    bool operator!=(const cRecStaticDescr& __o) const { return _schemaName != __o._schemaName || _tableName != __o._tableName; }
    /// Ha az objektum leszármazotja __o, akkor true-val tér vissza
    bool operator<(const cRecStaticDescr& __o) const;
    /// Ha a tábla __o-ból lett származtatva (ill. rekurzívan annak őséből), akkor true-val térvissza
    bool operator>(const cRecStaticDescr& __o) const { return __o < *this; }
    /// Ha a két objektum egyenlő, vagy ha az objektum leszármazotja __o, akkor true-val tér vissza
    bool operator<=(const cRecStaticDescr& __o) const { return __o == *this || __o > *this; }
    /// Ha a két objektum egyenlő, vagy ha a tábla __o-ból lett származtatva (ill. rekurzívan annak őséből), akkor true-val térvissza
    bool operator>=(const cRecStaticDescr& __o) const { return __o == *this || __o < *this; }
    /// A megadott nevű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    const cColStaticDescr& operator[](const QString& __n) const     { return _columnDescrs[__n]; }
    /// A megadott indexű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param 1 a keresett mező indexe (0,1,...).
    const cColStaticDescr& operator[](int __i) const                { return _columnDescrs[__i]; }
    /// Az id mező nevével tér vissza, ha van id mező, egyébként dob egy kizárást
    const QString& idName() const   { return columnName(idIndex()); }
    /// Az név mező nevével tér vissza, ha van név mező, egyébként dob egy kizárást
    const QString& nameName(bool __ex = true) const { return columnName(nameIndex(__ex), __ex); }
    /// Az descr mező nevével tér vissza, ha van descr mező, egyébként dob egy kizárást
    const QString& descrName() const { return columnName(descrIndex()); }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id.
    /// Hiba esetén, vagy ha nincs mega a keresett ID, és __ex értéke true, akkor dob egy kizárást,
    /// Ha viszont __ex értéke false, és hiba van, vagy nincs ID akkor NULL_ID-vel tér vissza.
    qlonglong getIdByName(QSqlQuery& __q, const QString& __n, bool ex = false) const;
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Hiba esetén, vagy ha nincs mega a keresett ID, és __ex értéke true, akkor dob egy kizárást,
    /// Ha viszont __ex értéke false, és hiba van, vagy nincs ID akkor NULL_ID-vel tér vissza.
    qlonglong getIdByName( const QString& __n, bool ex = false) const
        { QSqlQuery *pq = newQuery(); qlonglong id = getIdByName(*pq, __n, ex); delete pq; return id; }
    /// A ID alapján visszaadja a rekord név mező értékétt, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    QString getNameById(QSqlQuery& __q, qlonglong __id, bool ex = false) const;
    /// A ID alapján visszaadja a rekord név mező értékétt, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    QString getNameById(qlonglong __id, bool ex = false) const
        { QSqlQuery *pq = newQuery(); QString n = getNameById(*pq, __id, ex); delete pq; return n; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott sorszámú bitet 1-be állítja.
    /// Ha a megadott index nem egy mező sorszám, akkor dob egy kizárást.
    QBitArray mask(int __i1) const                              { return _mask(_columnsNum, chkIndex(__i1)); }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott sorszámú biteket 1-be állítja.
    /// Ha a megadott index valamelyike nem egy mező sorszám, akkor dob egy kizárást.
    QBitArray mask(int __i1, int __i2) const                    { QBitArray m = mask(__i1); m.setBit(chkIndex(__i2)); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott sorszámú biteket 1-be állítja.
    /// Ha a megadott index valamelyike nem egy mező sorszám, akkor dob egy kizárást.
    QBitArray mask(int __i1, int __i2, int __i3) const          { QBitArray m = mask(__i1, __i2); m.setBit(chkIndex(__i3)); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott sorszámú biteket 1-be állítja.
    /// Ha a megadott index valamelyike nem egy mező sorszám, akkor dob egy kizárást.
    QBitArray mask(int __i1, int __i2, int __i3, int __i4) const{ QBitArray m = mask(__i1, __i2, __i3); m.setBit(chkIndex(__i4)); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőnévnek megfelelő sorszámú bitet 1-be állítja.
    /// Ha a megadott név nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1) const   { return _mask(_columnsNum, chkIndex(toIndex(__n1))); }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyika nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1, const QString& __n2) const
        { QBitArray m = mask(__n1); m.setBit(chkIndex(toIndex(__n2))); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyika nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3) const
        { QBitArray m = mask(__n1, __n2); m.setBit(chkIndex(toIndex(__n3))); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyika nem egy mező nqtcreator failed ptraceév, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3, const QString& __n4) const
        { QBitArray m = mask(__n1, __n2, __n3); m.setBit(chkIndex(toIndex(__n4))); return m; }
    /// Létrehoz egy intVector objektumot, egy elemmel, ami a megadott mező név indexe.
    tIntVector   iTab(const QString __n1) const                      { return ::iTab(chkIndex(toIndex(__n1))); }
    /// Létrehoz egy intVector objektumot, két elemmel, ami a megadott mező nevek indexei.
    tIntVector   iTab(const QString __n1, const QString __n2) const  { return ::iTab(chkIndex(toIndex(__n1)), chkIndex(toIndex(__n2))); }
    /// Létrehoz egy intVector objektumot, három elemmel, ami a megadott mező nevek indexei.
    tIntVector   iTab(const QString __n1, const QString __n2, const QString __n3) const
        { return ::iTab(chkIndex(toIndex(__n1)), chkIndex(toIndex(__n2)), chkIndex(toIndex(__n3))); }
    /// Létrehoz egy intVector objektumot, négy elemmel, ami a megadott mező nevek indexei.
    tIntVector   iTab(const QString __n1, const QString __n2, const QString __n3, const QString __n4) const
        { return ::iTab(chkIndex(toIndex(__n1)), chkIndex(toIndex(__n2)), chkIndex(toIndex(__n3)), chkIndex(toIndex(__n4))); }
    /// Megvizsgálja, hogy a mező értéke lehet-e NULL.
    /// @param __ix A mező indexe.
    /// @return true, ha a mező lehet NULL.
    bool isNullable(int __ix) const             { return (*this)[__ix].isNullable; }
    /// Megvizsgálja, hogy a mező értéke lehet-e NULL.
    /// @param __ix A mező neve.
    /// @return true, ha a mező lehet NULL.
    bool isNullable(const QString& __nm) const  { return (*this)[__nm].isNullable; }
    /// Stringgé konvertálja az objektum adat tartalmát, többé-kevésbé egy SQL tábla CREATE parancs formájában.
    /// Nyomkövetési információkhoz.
    QString toString() const;
    /// A tulajdonosra mutató távoli kulcs mező megkeresése
    int ixToOwner(bool __ex = true) const;
protected:
    bool addMap();
    bool addCache();
    static int _setReCallCnt;
};
TSTREAMO(cRecStaticDescr)


/* ******************************************************************************************************
   *                                              cRecord                                               *
   ******************************************************************************************************/
/// @class cRecord
/// @brief Az adat rekord objektumok tisztán virtuális ős objektuma.
///
/// Egy adat rekord objektum, az adattábla leíró cRecStaticDescr objektumot statikus adattagként tartalmazza.
/// Az objektum egy rekordot tárolhat, egy a _fields adattagban, ami egy QVariant lista.
/// A _fields adattag ha nem üres. mindíg annyi mezőt tartalmaz, mint az adattábla, a mező neveket,
/// ill annak tulajdonságait egy statikus cRecStaticDescr típusú objektum tartalmazza.
/// Az adatkezelés álltalában virtuális metódusokon keresztül történik, azok a metódusok, amik
/// nem virtuálisak, '_' karakterrel kezdődnek, ebben az esetben ha szükség van a leíró
/// objektumra, akkor azt is paraméterként kell megadni. Ha a metódus nem hív virtuális metódust,
/// és nem '_'-al kezdődik, de alapvető jelentőségű metódus, akkor az álltalában inline metódusként
/// újra van definiálva '_' előtaggal.
/// Bizonyos metódusoknak két változata van, az alap metódus, csak a stat adattagot vizsgálja, míg a másik változat
/// átvizsgálja az objektumot, beállítja a stat-ot, és ez alapján tér vissza, ilyen esetekben az a metódus,
/// mely csak a stat alapján ad vissza értéket '_' karakterrel végződik, ha van értelme amegkülönböztetésnek.
class LV2SHARED_EXPORT cRecord : public QObject {
    template <class T> friend class tRecordList;
    friend class cRecordFieldConstRef;
    friend class cRecordFieldRef;
    Q_OBJECT
protected:
    /// Az objektum adattartalma, a mező értékek, megadott sorrendben, vagy üres.
    QVariantList    _fields;
    /// Csak egy segéd konstans, ha egy üres konstans QVariant-ra mutató referenciát kel visszaadnia egy metódusnak.
    static const QVariant   _vNul;
public:
    /// @enum eStat
    /// Rekord státusz konstansok
    typedef enum {
        ES_FACELESS     = -2,   ///< A rekord üres, és nincs típusa, ill nincs hozzárendelve adattáblához.
        ES_NULL         = -1,   ///< A rekord üres, a _fields adattagnak nincs egy eleme sem
        ES_EMPTY        =  0,   ///< A rekord üres, a _fields adattag feltöltve, de minden eleme null
        ES_EXIST        =  1,   ///< O.K. Az adatbázisban létező rekord
        ES_NONEXIST     =  2,   ///< O.K. Az adatbázisban nem létező rekord
        ES_COMPLETE     =  4,   ///< Minden kötelező mező feltöltve
        ES_INCOMPLETE   =  8,   ///< Hiányos
        ES_MODIFY       = 16,   ///< Módosítva
        ES_IDENTIF      = 32,   ///< A rendelkezésreálló mezők egyértelmüen azonositanak egy rekordot
        ES_UNIDENT      = 64,   ///< A rendelkezésreálló mezők nem feltétlenül azonositanak egy rekordot
        ES_DEFECTIVE    =128    ///< A rekord, vagy leíró inkonzisztens
    } eStat;
    int _stat;  ///< A rekord állapota
    /// Enumeráció (bitek) a deleted mező kezeléséhez.
    enum eDeletedBehavior {
        NO_EFFECT        = 0,   ///< Nem kezeli a deleted mezőt
        FILTERED         = 1,   ///< Autómatikus kiszűrése a deleted = true mezőknek, ha nincs hivatkozás a deleted mezőre.
        REPLACED_BY_NAME = 2,   ///< Insert esetén, ha azonos nevő törölt mező van, akkor annak a felülírása.
        DELETE_LOGICAL   = 4    ///< Törlés esetén a rekordot csak logikailag töröljük (deleted = true beállításával)
    };
    /// Az objektum viselkedése, ha van deleted mező (lsd.: enum eDeletedBehavior -et).
    /// Ha az objektum konstruktorban nincs inicializálva, akkor ennek alapértelmezése NO_EFFECT,
    int _deletedBehavior;
    /// Lekérdezéseknél ha a mező index szerinti bit létezik, és true, akkor egyenlőség helyett minta keresés lessz (LIKE)
    /// Mindíg üres tömbnek van inicializálva.
    QBitArray _likeMask;
    /// Konstruktor.
    cRecord();
    /// Destruktor
    virtual ~cRecord();
    // Tisztán virtuális függvények
    /// A táblát leíró statikus objektum referenciájával tér vissza
    /// Tisztán virtuális függvény, konstruktorból nem hívható.
    /// A leszármazottakban implementált metódus az első hívás alkalmával inicializálja
    /// a minden objektum típushoz definiált statikus cRecStaticDescr típusú objektumot.
    /// ld.: bool cRecStaticDescr::set(const QString& _t, const QString& _s)
    virtual const cRecStaticDescr&  descr() const = 0;
    /// Egy azonos típusú üres objektum allokálása
    virtual cRecord *newObj() const = 0;
    /// Az objektum klónozása
    virtual cRecord *dup() const = 0;
    // Virtuális metódusok
    /// A clear() metódus hívja visszatérés elött, elvégzendő a származtatott objektumokban még
    /// szükséges műveleteket. Alapértelmezés zserint egy létező üres függvény.
    virtual void clearToEnd();
    /// Értékadás után a származtatott osztályban a még szükséges műveleteket végzi el.
    /// Alapértelmezés szerint egy létező üres függvény.
    /// Visszatérés előt a következő metódusok hívják: cRecord& set(const QSqlRecord& __r)
    virtual void toEnd();
    /// Értékadás után a származtatott osztályban a még szükséges műveleteket végzi el.
    /// Alapértelmezés szerint egy üres függvény, ami false-val tér vissza
    /// Visszatérés elöt a következő metódusok hívják: cRecord& set(int __i, const QVariant& __v)
    /// Az esetleges csak a mezőt érintő adatkonverziók a statikus leíró mező leíró objektumának a
    /// virtuális metódusain keresztül lehetséges.
    /// @return true, ha egy olyan mező indexét adtuk meg, ami az objektum hatáskörébe tartozik, és valamilyen tevékenység tartozik hozzá, akkor true,
    ///               ha nem akkor false
    virtual bool toEnd(int i);
    /// Lekérdezi egy mező értékét. Ha a mező értéke NULL, vagy nincs ilyem mező, akkor egy NULL objektummal tér vissza.
    /// Ha a mező tartalmaz értékrt, akkor olyan formára konvertálja, mely megjeleníthető
    /// A távoli kulcsokat megkisérli névvé konvertálni.
    virtual QString view(QSqlQuery &q, int __i) const;
    /// Leellenörzi, lehet-e mező (oszlop) index a paraméter.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param A viszgálandó index érték
    /// @return Ha a paraméter értéke az aktuálisan lehetséges index értéktartományon bellül van, akkor true.
    bool isIndex(int i) const                           { return descr().isIndex(i); }
    /// Leellenörzi, lehet-e mező (oszlop) index a paraméter.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @exception cError* Ha a paraméter értéke az aktuálisan lehetséges index értéktartományon kívül van dob egy kizárást.
    /// @param A viszgálandó index érték
    /// @return Az index értékkel tér vissza.
    int chkIndex(int i) const                       { descr().chkIndex(i); return i; }
    /// A megadott mező névhez tartozó mező indexel tér vissza.
    /// Ha a mező név tartalmazza a schema nevet vagy a tábla nevet, akkor azoket ellenőrzi,
    /// és ha nem erre a táblára vonatkoznak, akkor -1-el tér vissza. A nevek kettős idézőjelbe tehetőek (egyenként!)
    /// @return a mező indexe, vagy -1, ha nincs ilyem mező, vagy a megadott tábla ill. schema név nem ezé a tábláé.
    int toIndex(const QString& __n, bool __ex = true) const { return descr().toIndex(__n, __ex); }
    /// Leellenörzi, lehet-e mező (oszlop) név a paraméter.
    /// @param A viszgálandó név
    /// @return Ha a paraméter értéke lehet mező név, akkor true.
    bool isIndex(const QString& n) const            { return isIndex(toIndex(n, false)); }
    /// A schema nevével tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QString& schemaName() const               { return descr().schemaName(); }
    /// A tábla nevével (nem teljes név, nem tartalmazza a schema nevet) tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QString& tableName() const                { return descr().tableName(); }
    /// Az összes mező nevét, tulajdonságait (sorrendben) tartalmazó listával tér vissza. A mező nevek nem tartalmazzák a
    /// schema és tábla nevet
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const cColStaticDescrList& columnDescrs() const { return descr().columnDescrs(); }
    /// Az oszlop névvel tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param i az oszlop indexe
    /// @exception cError* Ha i értéke nem lehet index, akkor dob egy kizárást
    const QString& columnName(int i) const         { return descr().columnName(i); }
    /// A megadott indexű mező nevével tér vissza. Hasonló a columnName() metódushoz, de a visszaadot nevet kettős idézőjelek közé teszi
    QString columnNameQ(int i) const                 { return descr().columnNameQ(i); }
    /// A teljes schema névvel kiegészített tábla névvel tér vissza, a nevek nincsenek idézőjelben.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    QString fullTableName() const                  { return descr().fullTableName(); }
    /// A teljes (schema és tábla névvel kiegészített) mezőnévvel tér vissza, a nevek nincsenek idézőjelben.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param _c A kiegészítendő mező (oszlop) neve
    QString fullColumnName(const QString& _c) const{ return descr().fullColumnName(_c); }
    /// A teljes schema névvel kiegészített tábla névvel tér vissza, a nevek idézőjelek közé vannak zárva.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívhatóvoid t.
    QString fullTableNameQ() const                   { return descr().fullTableNameQ(); }
    /// A teljes (schema és tábla névvel kiegészített) mezőnévvel tér vissza, a nevek idézőjelek közé vannak zárva.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param i A kiegészítendő mező (oszlop) indexe
    /// @exception cError* Ha i értéke nem lehet index, akkor dob egy kizárást
    QString fullColumnNameQ(int i) const             { return descr().fullColumnNameQ(i); }
    /// A teljes (schema és tábla névvel kiegészített) mezőnévvel tér vissza, a nevek idézőjelek közé vannak zárva.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param _c A kiegészítendő mező (oszlop) neve
    QString fullColumnNameQ(const QString& _c) const { return descr().fullColumnNameQ(_c); }
    /// A tábla típusával tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    eTableType tableType() const                    { return descr().tableType(); }
    /// Az mezők (oszlopok) teljes számával tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    int cols() const                                { return descr().cols(); }
    /// Az elsődleges kulcs mező(k) maszkjával tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QBitArray& primaryKey() const             { return descr().primaryKey(); }
    /// Az egyedi kulcs(ok) mező(k) maszk vektorával tér vissza.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QList<QBitArray>& uniques() const         { return descr().uniques(); }
    /// Egy bit vektorral tér vissza, ahol minden bit true, mellyel azonos indexű mező autoincrement.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QBitArray& autoIncrement() const          { return descr().autoIncrement(); }
    /// Az ID mező indexével tér vissza, ha nincs ID, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int idIndex(bool _ex = true) const              { return descr().idIndex(_ex); }
    /// Az NAME mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int nameIndex(bool _ex = true) const            { return descr().nameIndex(_ex); }
    /// Az DESCR mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int descrIndex(bool _ex = true) const           { return descr().descrIndex(_ex); }
    /// Az deleted mező indexével tér vissza, ha nincs deleted, vagy nem ismert az indexe, és _ex értéke true, akkor dob egy kizárást.
    int deletedIndex(bool _ex = true) const         { return descr().deletedIndex(_ex); }
    ///
    bool isParent() const                           { return descr()._isParent; }
    /// Ha bármelyik mező módosítható az adatbázisban, akkor true-val tér vissza.
    bool isUpdatable() const                    { return descr()._isUpdatable; }
    /// Ha a megadott indexű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen indexű mező, dosb egy kizárást.
    bool isUpdatable(int i) const               { return descr().colDescr(i).isUpdatable; }
    /// Ha a megadott nevű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen nevű mező, dosb egy kizárást.
    bool isUpdatable(const QString& fn) const   { return descr().colDescr(toIndex(fn)).isUpdatable; }
    /// Törli a rekordot. És meghívja a clearToEnd() virtuális metódust.
    cRecord& clear();
    /// Törli a megadott indexű mezőt. És meghívja a toEnd(int i) virtuális metódust.
    /// @param __ix A törlendő mező indexe.
    /// @param __ex Ha értéke true, és nem létezik a megadott indexű mező, akkor dob egy kizárást, ha értéke false, és nem létezik a mező, akkor nem csinál semmit.
    cRecord& clear(int __ix, bool __ex = true);
    /// Törli a megadott nevű mezőt. És meghívja a toEnd(int i) virtuális metódust, ahol i a megnevezett mező indexe.
    /// @param __n A törlendő mező neve.
    /// @param __ex Ha értéke true, és nem létezik a megadott mező, akkor dob egy kizárást, ha értéke false, és nem létezik a mező, akkor nem csinál semmit.
    cRecord& clear(const QString& __n, bool __ex = true) { return clear(toIndex(__n, __ex), __ex); }
    /// Ha nincs rekord deszkriptor a status szerint, akkor true-val tér vissza
    bool isFaceless() const { return _stat == ES_FACELESS; }
    /// Ha a mező adat lista (_fields) üres, akkor true, egyébként false. Nem a stat-ot vizsgálja, és annak értékét nem módosítja.
    bool isNull() const                             { return _fields.isEmpty();  }
    /// Azonos az isNull() metódussal, mivel nem hív virtuális metódust, ezen a néven újra lett definiálva.
    bool _isNull() const                            { return _fields.isEmpty();  }
    /// Ha isNull() visszaadott értéke true, vagy minden mező NULL, akkor true-val tér vissza, utóbbi állapot kiderítésére csak a stat értékét vizsgálja.
    bool isEmpty_() const                           { return isNull() || _stat == ES_EMPTY; }
    /// Ha a _stat adattag szerint módosítva lett az objketum, akkor true-val tér vissza.
    bool isModify_() const                          { return (_stat & ES_MODIFY) != 0; }
    /// Létrehozza az összes mezőt sorrendben (DSqlField elemet) null tartalommal. Elötte törli az objektumot
    /// Tisztán virtuális metódust hív, konstruktorból nem hívható.
    cRecord& set();
    /// Létrehozza az összes mezőt sorrendben (DSqlField elemet) az __r-ben lévő tartalommal.
    /// Elötte törli az objektumot.
    /// Tisztán virtuális metódust hív, konstruktorból nem hívható.
    /// @param __r Forrás rekord objektum. A mezők név szerint lesznek azonosítva.
    /// @param __fromp Ha nem NULL, akkor a *__fromp indextől végzi a feltöltést, visszatéréskor az első nem olvasott indexe
    /// @param __size Ha értéke pozitív, akkor össz. ennyi mezőt fog olvasni.
    cRecord& set(const QSqlRecord& __r, int* __fromp = NULL, int __size = -1);
    /// Beállítja a megadott indexű mező értékét, majd hívja a toEnd(__i) metódust. Ha az objektum NULL, akkor feltölti elöszőr a _fields-t NULL értékekkel.
    /// @param __i A mező indexe, ha nem egy valós mező indexe, akkor dob egy kizárást.
    /// @param __v A megadott mező új értéke, az értéket elöszőr konvertálja a mező leíró set() virtuális metódusával.
    /// @return Az objektumra mutató referencia
    cRecord& set(int __i, const QVariant& __v);
    /// Hasonló a cRecord& set(int __i, const QVariant& __v); híváshoz, de itt a mező azonosító a mező neve.
    /// Ha nincs ilyen nevű mező, akkor dob egy kizárást.
    /// @param __fn A mező neve
    /// @param __v A megadott mező új értéke
    /// @return Az objektumra mutató referencia
    cRecord& set(const QString& __fn, const QVariant& __v) { return set(chkIndex(toIndex(__fn)), __v); }
    /// Hasonló a set(const QSqlRecord& __r); híváshoz, a rekord a query aktuális rekordja.
    cRecord& set(const QSqlQuery& __q, int * __fromp = NULL, int __size = -1)   { return set(__q.record(), __fromp, __size);   }
    /// A megadott indexű mező értékének a lekérdzése. Az indexet ellenőrzi, ha nem megfelelő dob egy kizárást
    /// A mező értékére egy referenciát ad vissza, ez a referencia csak addig valós, amíg nem hajtunk végre
    /// az objektumon egy olyan metódust, amely ujra kreálja az adat konténert. Azon értékadó műveletek, melyek
    /// az objektum egészére vonatkoznak, általában újra létrehozzák az adat konténert.
    /// Ha az objektumra isNull() == true, akkor egy statikus NULL értékű QVariant változó referenciáját adja vissza.
    /// @param __i A lekérdezendő mező indexe
    /// @return A megadott indexű mező értékére mutató referencia.
    const QVariant& get(int __i) const;
    /// Hasonló a get(int __i) metódushoz, de nincs index ellenörzés, ha nincs a keresett mező, akkor a belső statikus null értékre ad referenciát.
    const QVariant& _get(int __i) const;
    /// A megadott nevű mező értékének a lekérdzése. Hasonló a másik get metódushoz.
    /// Ha nincs ilyen nevű mező, akkor dob egy kizárást
    /// @param __i A lekérdezendő mező neve
    /// @return A megadott mező értéke
    const QVariant& get(const QString& __n) const   { return get(toIndex(__n)); }
    /// Az ID mező értékét adja vissza, ha a mező értéke null, akkor a NULL_ID konstanssal tér vissza.
    qlonglong getId() const                         { return getId(idIndex()); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy ID, vagy numerikus. Ha a mező NULL, vagy nem numerikus, akkor NULL_ID-vel tér vissza.
    qlonglong getId(int i) const                    { return descr()[i].toId(get(i)); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy ID. Ha a mező NULL, vagy nem numerikus, akkor NULL_ID-vel tér vissza.
    qlonglong getId(const QString& __n) const       { return getId(toIndex(__n)); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy string, ill. annak értékét stringgé konvertálja.
    QString getName(int i) const                    { return descr()[i].toName(get(i)); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy string, ill. annak értékét stringgé konvertálja.
    QString getName(const QString& __n) const       { return getName(toIndex(__n)); }
    /// Az név (name) mező értékével tér vissza, ha van, egyébként dob egy kizárást
    QString getName() const                         { return getName(nameIndex()); }
    /// Az descriptor/cím mező értékével tér vissza, ha van, egyébként dob egy kizárást
    QString getDescr() const                        { return getName(descrIndex()); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egyvoid t logikai érték, ill. annak értékét bool típusúvá konvertálja.
    bool getBool(int __i) const                     { return get(__i).toBool(); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy logikai érték, ill. annak értékét bool típusúvá konvertálja.
    bool getBool(const QString& __i) const          { return get(__i).toBool(); }
    /// Lekérdezi egy mező értékét. Lsd. a másik view(int __i) metódust.
    QString view(QSqlQuery& q, const QString& __i) const    { return view(q, toIndex(__i, false)); }
    /// Az ID mező értékét állítja be. Ha NULL_ID-t adunk meg, akkor törli a mezőt.
    /// @param __id Az ID új értéke
    cRecord& setId(qlonglong __id)                  { if (__id == NULL_ID) clear(idIndex()); else set(idIndex(), QVariant(__id)); return *this; }
    /// A megadott indexű mezőt állitja be, feltételezve, hogy az egy ID.  Ha NULL_ID-t adunk meg, akkor törli a mezőt.
    cRecord& setId(int __i, qlonglong __id)         { if (__id == NULL_ID) clear(__i); else set(__i, QVariant(__id)); return *this; }
    /// A megadott indexű mezőt állitja be, feltételezve, hogy az egy ID.  Ha NULL_ID-t adunk meg, akkor törli a mezőt.
    cRecord& setId(const QString& __i, qlonglong __id)  { if (__id == NULL_ID) clear(__i); else set(__i, QVariant(__id)); return *this; }
    /// Az megadott mező értékét állítja be, feltételezve, hogy az egy string, ill. az új érték egy string lessz.
    /// @param __i A mező indexe
    /// @param __n Az mező új értéke, ha null stringet adunk át, akkor NULL lessz.
    cRecord& setName(int __i, const QString& __n)   { if (__n.isNull()) clear(__i); else set(__i, QVariant(__n)); return *this; }
    /// Az megadott mező értékét állítja be, feltételezve, hogy az egy string, ill. az új érték egy string lessz.
    /// @param __i A mező neve
    /// @param __n Az mező új értéke, ha null stringet adunk át, akkor NULL lessz.
    cRecord& setName(const QString& __i, const QString& __n)    { if (__n.isNull()) clear(__i); else set(__i, QVariant(__n)); return *this; }
    /// Az név (name) mező értékét állítja be
    /// @param __n Az név új értéke, ha null stringet adunk át, akkor NULL lessz.
    cRecord& setName(const QString& __n)            { return setName(nameIndex(), __n); }
    /// Az ID mezőket hasonlítja össze. Ha nincs vagy nem ismert az ID mező, akkor dob egy kizárást.
    /// Nem ellenörzi, hogy a két objektum valójában milyen típusú, akkor is elvégzi az összehasonlítást, ha ez logikailag értelmetlen.
    /// Ha az ID értéke NULL, akkor az kisebb minden lehetséges ID értéknél.
    /// @return ha az objektum id-je kissebb, mint az operátor jobb oldalán elhelyezkedö.
    bool operator< (const cRecord& __o) const       { return getId() <  __o.getId(); }
    /// Az ID mezőket hasonlítja össze. Ha nincs vagy nem ismert az ID mező, akkor dob egy kizárást
    /// Nem ellenörzi, hogy a két objektum valójában milyen típusú, akkor is elvégzi az összehasonlítást, ha ez logikailag értelmetlen.
    /// @return Ha a két id egyenlő, akkor true, de ha bármelyik id NULL, akkor false.
    bool operator==(const cRecord& __o) const       { return !isNullId() && !__o.isNullId() &&  getId() == __o.getId(); }
    /// Az ID mezőket hasonlítja össze. Ha nincs vagy nem ismert az ID mező, akkor dob egy kizárást
    /// Nem ellenörzi, hogy a két objektum valójában milyen típusú, akkor is elvégzi az összehasonlítást, ha ez logikailag értelmetlen.
    /// @return Ha a két id nem egyenlő, akkor true, de ha bármelyik id NULL, akkor false.
    bool operator!=(const cRecord& __o) const       { return !isNullId() && !__o.isNullId() &&  getId() != __o.getId(); }
    /// Ha a megadott indexű mező NULL, vagy nincs ilyen indexü mező akkor true-val tér vissza
    /// Nem hív virtuális metódust.
    bool isNull(int i) const                        { return isNull() || !(i >= 0 && i < _fields.size()) || _fields[i].isNull(); }
    /// Az isNull(int i) újradefiniálása, a név konvenció miatt.
    bool _isNull(int i) const                       { return isNull(i); }
    /// Ha a megadott Nevű mező NULL, vagy nincs ilyen nincs mező akkor true-val tér vissza
    /// Virtuális metódust hív, konstruktorban nem használható.
    bool isNull(const QString& __n) const           { int i = toIndex(__n); return isNull(i); }
    /// Ha az ID mező NULL, akkor true-val tér vissza
    /// Virtuális metódust hív, konstruktorban nem használható.
    bool isNullId() const                           { return isNull(idIndex()); }
    /// Ha a név mező NULL, akkor true-val tér vissza
    /// Virtuális metódust hív, konstruktorban nem használható.
    bool isNullName() const                         { return isNull(nameIndex()); }
    /// Beinzertál egy rekordot a megfelelő adattáblába. Az insert utasításban azok a mezők
    /// lesznek magadva, melyek nek nem NULL az értékük.
    /// Ha sikeres volt az insert, akkor az objektumot újra tölti, az adatbázisban keletkezett új rekord alapján.
    /// A rekord kiírását, és visszaolvasását egy parancsal valósítja meg "INSERT ... RETURNING *"
    /// @param __q Az insert utasítás ezel az objektummal lesz kiadva
    /// @param __ex Ha értéke true (ez az alapértelmezés), akkor kizárást dob, ha adat hiba van, ill. nem történt meg az insert
    /// @exception cError* Hiba esetén dob egy kizárást
    /// @return ha történt változás az adatbázisban, akkor true.
    virtual bool insert(QSqlQuery& __q, bool __ex = true);
    /// Egy WHERE stringet állít össze.
    QString whereString(QBitArray& __fm);
    /// Végrehajt egy query-t a megadott sql query stringgel, és az objektum mezőivel (bind) az __arg-ban megadott sorrendben.
    /// @arg __q A QSqlQuery objektum, amin keresztül a lekérdezést a metódus elvégzi
    /// @arg sql A SQL query string
    /// @arg __arg A vektor elemei a mező indexei, amit bind-elni kell a lekérdezéshez.
    /// @arg __ex Ha értéke true, és az exec hibával tér vissza, akkor dob egy kizárást
    /// @exception Ha a QSqlQuery::prepare() metódus sikertelen, akkor dob egy kizárást
    /// @return Az QSqlQuery::exec() metódus által visszaadott érték.
    bool query(QSqlQuery& __q, const QString& sql, const tIntVector& __arg, bool __ex = true);
    /// Végrehajt egy query-t a megadott sql query stringgel, és az objektum mezőivel ...
    /// @arg __q A QSqlQuery objektum, amin keresztül a lekérdezést a metódus elvégzi
    /// @arg sql A SQL query string
    /// @arg __fm Bit vektor, ha a vektor elem true, akkor az azonos indexű mezőt kell bind-olni
    /// @arg __ex Ha értéke true, és az exec hibával tér vissza, akkor dob egy kizárást
    /// @exception Ha a QSqlQuery::prepare() metódus sikertelen, akkor dob egy kizárást
    /// @return Az QSqlQuery::exec() metódus által visszaadott érték. ha __ex = tru, akkor ha visszatér, akkor true.
    bool query(QSqlQuery& __q, const QString& sql, const QBitArray& __fm, bool __ex = true);
    /// Összeálít egy lekérdezést a beállított, ill. a megadott adatok alapján. A lekérdezást végrehajtja,
    /// Az eredmény a apar,éterként megadott QSqlQuery objektumban.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __only Ha megadjuk és értéke true, akkor a származtatott táblákan nem keres.
    /// @param __fn A mező(k) maszk, alapértelmezése üres, ekkor a használt maszk az elsődleges kulcs mező(k).
    /// @param __ord Mely mezők szerint legyenek rendezve a találatok ( a vektor elemei mező indexek, a záró elem -1)
    /// @param __lim Limit. Ha értéke 0, akkor nincs.
    /// @param __off Offset. Ha értéke 0, akkor nincs.
    /// @param __s A SELECT kulcs szó utáni lista. Ha nincs megadva (vagy NULL), akkor az alapértelmezés a "*"
    /// @param __w A WHERE kulcs szó utáni feltétel, Feltéve, hogy nem üres string, __fn nem alapértelmezés szerinti, és nincs benne egy 1-es bit sem.
    void fetchQuery(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), const tIntVector& __ord = tIntVector(), int __lim = 0, int __off = 0, QString __s = QString(), QString __w = QString());
    /// Beolvassa az adatbázisból azt a rekordot/rekordokat, amik a megadott mező maszk esetén egyeznek az
    /// objektumban tárolt mező(k) érték(ek)kel.
    /// Az első rekordot beolvassa az objektumba, ill ha nincs egyetlen rekord, akkor törli az objektumot.
    /// Kilépés után a lekérdezés eredménye a __q által hivatkozott QSqlRecord objektumban marad. Kurzor az első rekordon.
    /// Ha a megadott maszk (__fn) paraméter men üres, de minden eleme false, akkor a tábla összes sora ki lesz szelektálva.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __only Ha megadjuk és értéke true, akkor a származtatott táblákan nem keres.
    /// @param __fn A mező(k) maszk, alapértelmezése üres, ekkor a használt maszk az elsődleges kulcs mező(k).
    /// @param __ord Mely mezők szerint legyenek rendezve a találatok ( a vektor elemei mező indexek, a záró elem -1)
    /// @param __lim Limit. Ha értéke 0, akkor nincs.
    /// @param __off Offset. Ha értéke 0, akkor nincs.
    /// @return true, ha a feltételnek megfelelt legalább egy rekord, és azt beolvasta. false, ha nincs egyetlen rekord sem
    virtual bool fetch(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), const tIntVector& __ord = tIntVector(), int __lim = 0, int __off = 0);
    bool fetch(bool __only = false, const QBitArray& __fm = QBitArray(), const tIntVector& __ord = tIntVector(), int __lim = 0, int __off = 0)
            { QSqlQuery q = getQuery(); return fetch(q, __only, __fm, __ord, __lim, __off); }
    /// Beolvas egy rekordot a megadott id alapján. Ld.: fetch(QSqlQuery& __q, int __fn) metódust
    /// Ha nincs, vagy nem ismert az ID mező, akkor dob egy kizárást
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __id Opcionális ID paraméter, ha nem adjuk meg, akkor az objektum aktuális ID alapján olvassa be a rekordot.
    /// @return Ha sikerült beolvasni legalább egy rekordot, akkor true.
    bool fetchById(QSqlQuery& __q, qlonglong __id = NULL_ID)    { if (__id != NULL_ID) setId(__id); return fetch(__q, false, _bit(idIndex())); }
    bool fetchById(qlonglong __id = NULL_ID)                    { if (__id != NULL_ID) setId(__id); return fetch(false, _bit(idIndex())); }
    cRecord& setById(QSqlQuery& __q, qlonglong __id = NULL_ID)  { if (!fetchById(__q, __id)) EXCEPTION(EFOUND, __id, descr()._tableName); return *this; }
    cRecord& setById(qlonglong __id = NULL_ID)                  { if (!fetchById(__id)) EXCEPTION(EFOUND, __id, descr()._tableName); return *this; }
    /// Beolvas egy rekordot a megadott név alapján. Ld.: fetch(QSqlQuery& __q, int __fn) metódust
    /// Ha nincs vagy nem ismert a név mező, akkor dob egy kizárást
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __name Opcionális név paraméter, ha nem adjuk meg, akkor az objektum aktuális neve alapján olvassa be a rekordot.
    /// @return Ha sikerült beolvasni legalább egy rekordot, akkor true.
    bool fetchByName(QSqlQuery& __q, const QString& __name = QString()) { if (!__name.isEmpty()) setName(__name); return fetch(__q, false, _bit(nameIndex())); }
    bool fetchByName(const QString& __name = QString())                 { if (!__name.isEmpty()) setName(__name); return fetch(false, _bit(nameIndex())); }
    /// Hasonló, mint a fetchByName hívás, de ha nem tudott beolvasni rekordot, akkor dob egy kizárást.
    cRecord& setByName(QSqlQuery& __q, const QString& __name = QString())   { if (!fetchByName(__q, __name)) EXCEPTION(EFOUND,-1, __name); return *this; }
    cRecord& setByName(const QString& __name = QString())                   { if (!fetchByName(__name)) EXCEPTION(EFOUND,-1, __name); return *this; }
    /// A tábla egy rekord módosítása az adatbázisban
    /// @param __q A művelethez használt QSqlQuery objektum.
    /// @param __only A modosításokat csak a megadott táblában, a leszármazottakban nem, ha true
    /// @param __set Bitmap, a módosítandó mezőkkel azonos indexű biteket 1-be kell állítani. Ha üres, ill nincs benne egyetlen true sem, akkor a where bitek negáltja.
    /// @param __where Opbionális bitmap, a feltételben szereplő mezőkkel azonos idexű biteket 1-be kell állítani. Alapértelmezetten az elsődeleges kúlcs mező(ke)t használja,
    ///                ha egy üres tömböt adunk át, ha viszont nem öres, de egyetlen true értéket sem tartalmazó tömböt adunk meg, akkor az a tábla
    ///                összes elemét kiválasztja.
    bool update(QSqlQuery& __q, bool __only, const QBitArray& __set = QBitArray(), const QBitArray& __where = QBitArray(), bool __ex = true);
    ///
    bool remove(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), bool __ex = true);
    /// Adat ellenőrzést végez
    /// Beállítja stat értékét
    virtual bool checkData();
    /// Egy bitvektort ad vissza, ahol minden bit 1, mellyel azonos indexű mező nem null az objektumban.
    QBitArray getSetMap();
    /// Hasonló a fetch-hez. A lekérdezésben az összes nem null mezőt (és értékét) megadja a WHERE után.
    /// @return A feltételeknek megfelelő rekordok száma.
    int completion(QSqlQuery& __q);
    int completion()                { QSqlQuery q = getQuery(); return completion(q); }
    bool first(QSqlQuery& __q);
    bool next(QSqlQuery& __q);
    /// Hasonló a fetch metódushoz, de csak a rekordok számát kérdezi le
    int rows(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray()) {
        fetchQuery(__q, __only, __fm, tIntVector(), 0,0,QString("count(*)"));
        return __q.value(0).toInt();
    }
    int rows(bool __only = false, const QBitArray& __fm = QBitArray()) { QSqlQuery q = getQuery(); return rows(q, __only, __fm);  }
    /// Az objektúm típusnak megfelelő tableoid-vel tér vissza. Ez nem feltétlenül azonos azzal a tableoid-vel, amely táblából beolvastuk a rekordot, ha beolvastuk.
    /// A rekord tárolására használlt eredeti objektum típust azonosítja.
    qlonglong tableoid()        { return descr().tableoid(); }
    /// Csak a tableoid mezőt olvassa be, a megadott mezők alapján. Ha a magadott mezők nem egyértelmüen azonosítanak egy rekordot,
    /// akkor ha __ex igaz dob egy kizárást, ill. ha __ex hamis, akkor NULL_ID-vel térvissza.
    qlonglong fetchTableOId(QSqlQuery& __q, bool __ex = true);
    /// Megvizsgálja, hogy a kitöltött mezők alapján a hozzá tartozó rekord egyértelműen meghatározott-e.
    /// Vagyis ki kell töltve lennie legalább egy kulcs mező csoportnak, vagy a primary kulcs mező(k)nek.
    /// @return ha az objektum adattartalma csak egy rekordot jelenthet, akkor true
    bool isIdent();
    /// Megvizsgálja, hogy a megadott mezők nem null értéküek-e
    /// @param __m A bit mezőben a viszgálandó mezővel azonos indexű bitek 1-es értéküek.
    /// @return Ha egyik jelzett érték sem null, akkor true.
    bool isIdent(const QBitArray& __m);
    /// A forrásból átmásol minden olyan mezőt, amelyik mindkét rekordban létezik, és a melyik a forrásban nem NULL
    /// A mező adatokat nem törli a másolás elött. Hasonló a _copy metódushoz, de ez virtuális metódust is hív.
    /// A másolási művelet után meghívja a checkData() metódust, hogy beállítsa a _stat értékét
    cRecord& set(const cRecord& __o) { if (_copy(__o, descr())) checkData(); return *this; }
    /// A forrásból átmásol minden olyan mezőt, amelyik mindkét rekordban létezik, és a melyik a forrásban nem NULL
    /// A mező adatokat nem törli a másolás elött. Hasonló a _copy metódushoz, de ez virtuális metódust is hív.
    /// A _stat értékére ugyanaz igaz, mint ami a _copy() metódusban le van írva.
    cRecord& set_(const cRecord& __o) { _copy(__o, descr()); return *this; }
    /// Mező érték, ill. referencia az index alapján.
    /// @param __i A mező indexe
    /// @return A megadott mezőre mutató referencia objektummal tér vissza
    /// @exception Ha a megadott index érték nem egy valós mező indexe.
    cRecordFieldRef operator[](int __i);
    /// Mező érték, ill. referencia az index alapján.
    /// @param __i A mező indexe
    /// @return A megadott mezőre mutató konstans referencia objektummal tér vissza
    /// @exception Ha a megadott index érték nem egy valós mező indexe.
    cRecordFieldConstRef operator[](int __i) const;
    /// Mező érték, ill. referencia az mező név alapján.
    /// @param __i A mező neve
    /// @return A megadott mezőre mutató referencia objektummal tér vissza
    /// @exception Ha a megadott név nem egy valós mező neve.
    cRecordFieldRef operator[](const QString& __fn);
    /// Mező érték, ill. referencia az mező név alapján.
    /// @param __i A mező neve
    /// @return A megadott mezőre mutató konstans referencia objektummal tér vissza
    /// @exception Ha a megadott név nem egy valós mező neve.
    cRecordFieldConstRef operator[](const QString& __fn) const;
    /// Az id mező nevével tér vissza, ha van id mező, egyébként dob egy kizárást
    const QString& idName() const   { return columnName(idIndex()); }
    /// Az név mező nevével tér vissza, ha van név mező, egyébként dob egy kizárást
    const QString& nameName(bool __ex = true) const { return descr().nameName(__ex); }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befojásolja a
    /// működését. És az objektum értéke nem változik.
    qlonglong getIdByName(QSqlQuery& __q, const QString& __n, bool __ex = true) const { return descr().getIdByName(__q, __n, __ex); }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befojásolja a
    /// működését.
    qlonglong getIdByName(const QString& __n, bool __ex = true) const { return descr().getIdByName(__n, __ex); }
    /// A ID alapján visszaadja a rekord nevet, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befojásolja a
    /// működését.
    QString getNameById(QSqlQuery& __q, qlonglong __id, bool __ex = true) const { return descr().getNameById(__q, __id, __ex); }
    /// A ID alapján visszaadja a rekord nevet, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befojásolja a
    /// működését.
    QString getNameById(qlonglong __id, bool __ex = true) const { return descr().getNameById(__id, __ex); }
    ///
    QBitArray mask(int __i1) const                              { return descr().mask(__i1); }
    QBitArray mask(int __i1, int __i2) const                    { return descr().mask(__i1, __i2); }
    QBitArray mask(int __i1, int __i2, int __i3) const          { return descr().mask(__i1, __i2, __i3); }
    QBitArray mask(int __i1, int __i2, int __i3, int __i4) const{ return descr().mask(__i1, __i2, __i3, __i4); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lessz, ahány mező van a táblában, és a megadott mezőnevekkel azonos idexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1) const                                               { return descr().mask(__n1); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lessz, ahány mező van a táblában, és a megadott mezőnevekkel azonos idexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1, const QString& __n2) const                          { return descr().mask(__n1, __n2); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lessz, ahány mező van a táblában, és a megadott mezőnevekkel azonos idexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3) const     { return descr().mask(__n1, __n2, __n3); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lessz, ahány mező van a táblában, és a megadott mezőnevekkel azonos idexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3, const QString& __n4) const { return descr().mask(__n1, __n2, __n3, __n4); }
    tIntVector   iTab(const QString __n1)                        { return descr().iTab(__n1); }
    tIntVector   iTab(const QString __n1, const QString __n2)    { return descr().iTab(__n1, __n2); }
    tIntVector   iTab(const QString __n1, const QString __n2, const QString __n3)    { return descr().iTab(__n1, __n2, __n3); }
    tIntVector   iTab(const QString __n1, const QString __n2, const QString __n3, const QString __n4)    { return descr().iTab(__n1, __n2, __n3, __n4); }
    /// Az objektumot (csak az adattartlmát) stringé konvertálja.
    QString toString() const;
    /// Az objektumot stringé konvertálja, a rekord leíróval együtt.
    QString allToString() const { return descr().toString() + " = " + toString(); }
    /// Rekord(ok) törlésa a név alapján
    /// @param  q
    /// @param __n A név mező értéke, vagy minta a név mezőre
    /// @param __pat Ha értéke true, akkor az __n paraméter minta stringnek tekintendő.
    /// @param __only Ha értéke true, ekkor a származtatott táblákban nincs törlés.
    /// @return a törölt rekordok száma
    int delByName(QSqlQuery& q, const QString& __n, bool __pat = false, bool __only = false);
    /// Megvizsgálja, hogy a mező értéke lehet-e NULL.
    /// @param __ix A mező indexe.
    /// @return true, ha a mező lehet NULL.
    bool isNullable(int __ix) const             { return descr()[__ix].isNullable; }
    /// Megvizsgálja, hogy a mező értéke lehet-e NULL.
    /// @param __ix A mező neve.
    /// @return true, ha a mező lehet NULL.
    bool isNullable(const QString& __nm) const  { return descr()[__nm].isNullable; }
    /// Ellenörzi, hogy az objektum eredeti típusa megegyezik-e a megadott típussal.
    /// A statikus leíró _tabkeOId adattagjait hasonlítja össze.
    /// @param __eq ha false, akkor az eredeti típus leszármazottja is lehet a megadott típusnak
    /// @param __ex ha értéke true, és nincs eggyezés, akkor dob egy kizárást.
    /// @return true, ha eggyezés van, flase ha nincs eggyezés, és __ex false.
    template <class T> bool chkObjType(bool __eq = false, bool __ex = true) {
        if (descr().tableoid() == T::_descr().tableoid()) return true;                  // Azonos
        if (__eq == false && descr().tableoid() > T::_descr().tableoid()) return true;  // Nem azonos, de konvertálható
        if (__ex == true) EXCEPTION(EDATA, -1, QString(QObject::trUtf8("Invalid object type, %1 ~ %2").arg(descr().tableoid()).arg(T::_descr().tableoid())));
        return false;
    }
    /// Az objektum pointert visszaalakítja az eredeti és megadott típusba.
    /// Ha az eredeti, és a megadott típus nem eggyezik, vagy az eredeti típusnak nem szülője a megadott típus, akkor dob egy kizárást
    template <class T> T * reconvert() {
        chkObjType<T>();
        return dynamic_cast<T *>(this);
    }
    /// Megviszgálja, rendeben van-e a kitöltött objektum, és nem üres-e.
    /// @return Ha az objektum üres, vagy a státusz változó szerint nincs rendben, akkor true-val tér vissza
    bool operator !() { return _stat <= ES_EMPTY || ((_stat & (ES_INCOMPLETE || ES_DEFECTIVE)) != 0); }
    /// Ha az objektum nem tartalmaz egyetlen nem null mezőt sem, akkor true-val tér vissza.
    /// Nem a stat adattag alapján tér vissza, ill. azt beállítja ha a vissaadott érték true.
    bool isEmpty();
    /// Azonos az IsEmpty() hívással. Mivel nem hív virtuális metódust, le lett definiálva ezen a néven is.
    bool _isEmpty()                        { return isEmpty(); }
protected:
    /// Copy constructor. Nem támogatott konstrktor. Dob egy kizárást.
    cRecord(const cRecord& __o);
    /// átmásolja ellenörzés nélkül az objektum adattagjait, a mező adatok kivételével.
    void __cp(const cRecord& __o) {
        _stat    = __o._stat;
        _deletedBehavior = __o._deletedBehavior;
        _likeMask = __o._likeMask;
    }
    /// átmásolja ellenörzés nélkül az objektum adattagjait
    void _cp(const cRecord& __o) {
        _fields = __o._fields;
        __cp(__o);
    }
    /// Átmásolja a paraméterként megadott objektum mezőít. Elötte nem törli az objektum mező adatait.
    /// Ha a forrás olyan mezőt tartalmaz, amit a cél nem, akkor azokat figyelmen kívül hagyja.
    /// A _stat adattagot nem módősítja, csak ha ST_NULL, volt, akkor hívja _set(d); metódust, ezért az ekkor ST_EMPTY lessz !
    /// A mezők azonosságát csak a név alapján dönti el.
    /// @return Ha módosította az objektumot, akkor true
    bool _copy(const cRecord& __o, const cRecStaticDescr& d);
    /// Törli az összes mező adatot (_fields konténer teljes tartalmát), továbbá törli a likeMask adattagot is.
    /// stat-ot ES_NULL -ra állítja.
    cRecord& _clear();
    /// Törli a megadott indexű mezőt. Ha ezután minden mező NULL lessz, akkor stat-ot ES_EMPTY-re, ha nem, akkor
    /// stat-ban bebillenti az ES_MODIFY bitet.
    cRecord& _clear(int __ix);
    /// Törli az adatmezőket, és újra létrehozza üres tartalommal. A stat-ot ES_EMPTY-re állítja.
    /// Konstruktorból is hívható.
    /// @param __d Tábla leíró objektum
    cRecord& _set(const cRecStaticDescr& __d);
    /// Létrehozza az összes mezőt sorrendben, és feltölti (a mezőnevek alapján) az __r-ben lévő tartalommal.
    /// Nem hív virtuális metódust.
    /// Ha _r olyan mezőt tartalmaz, melyet az objektum nem, akkor az figyelmen kívül lessz hagyva.
    /// Elötte nem törli az objektum tartalmát
    /// @param __r A kitöltendő mező adatokat tartalmazó rekord objektum.
    /// @param __d Tábla leíró objektum
    /// @param __fromp Egy sorszámra mutató pointer, ha nem null, akkor a beolvasot rekord első figyelembevett eleme az itt megadott sorszámú.
    ///              a sorszám értéke a függvény visszatértekor az utolsó felhasznált mező sorszáma +1 lessz, ha nem NULL a pointer.
    /// @param __size Ha értéke nem -1, akkor a beolvasott rekordból csak ennyi mező lessz figyelembevéve.
    cRecord& _set(const QSqlRecord& __r, const cRecStaticDescr& __d, int* __fromp = NULL, int __size = -1);
    /// Beállítja a megadott sorszámú mező értékét. Az objektum, ill a _field konténer nem lehet üres, egyébként dob egy kizárást.
    /// Nem hív virtuális metódust, így a toEnd() metódusokat sem, igy egyébb adatott nem módosít, a statust sem.
    cRecord& _set(int __i, const QVariant& __v) { if (isNull()) EXCEPTION(EPROGFAIL); _fields[__i] =  __v; return *this; }
    /// Megvizsgálja, hogy a megadott indexű bit a likeMask-ban milyen értékű, és azzal tér vissza, ha nincs ilyen elem, akkor false-val.
    bool _isLike(int __ix) { return __ix < 0 || _likeMask.size() <= __ix ? false : _likeMask[__ix]; }
    /// Hasonló a get(int __i) metódushoz, de nincs index ellenörzés, ha nincs a keresett mező dob egy kizárást. A visszaadott lrték nem konstans referencia.
    QVariant& _get(int __i)  { if (__i < 0 || __i >=  _fields.size()) EXCEPTION(EPROGFAIL,__i); return _fields[__i]; }
signals:
    /// Signal: Ha egy mező értéke megváltozott (A modified szignált hívó metódus nem hívja ezt a szignált mezőnként)
    void fieldModified(int ix);
    /// Signal: Ha a rekord teljes tartalma megváltozott, pl. adatbázis műveletnél beolvasásra került egy rekord.
    void modified();
    /// Signal: Ha az adattartalma lett törölve az objektumnak
    void cleared();
};
TSTREAMO(cRecord)

/// Hash készítése egy cRecord objektumból, a hash-t az ID-ből képzi.
inline static uint qHash(const cRecord& key) { return qHash(key.getId()); }

/// A statikus rekord leíró objektum pointer inicializálása
/// @return Ha inicializálni kellett a pointert, akkor true.
template <class R> bool initPDescr(const QString& _tn, const QString& _sn = _sPublic)
{
    if (R::_pRecordDescr == NULL) {
        R::_pRecordDescr = cRecStaticDescr::get(_tn, _sn);
        return true;
    }
    return false;
}

/// A statikus rekord leíró objektum pointer inicializálása, ha ez még nem történt meg
/// @return A leíró pointere
template <class R> const cRecStaticDescr *getPDescr(const QString& _tn, const QString& _sn = _sPublic)
{
    initPDescr<R>(_tn, _sn);
    return R::_pRecordDescr;
}

/// @def CRECORD(R)
/// Egy cRecord leszármazottban néhány feltétlenül szükséges deklatáció
#define CRECORD(R) \
        friend bool initPDescr<R>(const QString& _tn, const QString& _sn); \
        friend const cRecStaticDescr *getPDescr<R>(const QString& _tn, const QString& _sn); \
    public: \
        R(); \
        R(const R& __o); \
        virtual const cRecStaticDescr&  descr() const; \
        virtual ~R(); \
        virtual cRecord *newObj() const; \
        virtual cRecord *dup()const; \
        static const cRecStaticDescr& _descr_##R() { if (R::_pRecordDescr == NULL) EXCEPTION(EPROGFAIL); return *R::_pRecordDescr; } \
    protected: \
        static const cRecStaticDescr * _pRecordDescr

/// @def CRECCNTR(R)
/// Egy default cRecord leszármazott objektum konstruktorok
#define CRECCNTR(R) \
    R::R()             : cRecord() { _set(R::descr()); } \
    R::R(const R& __o) : cRecord() { _cp(__o); }

/// @def CRECDEF(R)
/// A cRecord leszármazottakban a newObj() és dup() virtuális metódusokat definiáló makró
/// @param T Az osztály neve, melyhez a metódusok tartoznak
#define CRECDEF(R) \
    const cRecStaticDescr * R::_pRecordDescr = NULL; \
    cRecord *R::newObj() const { return new R; }   \
    cRecord *R::dup()const     { return new R(*this); }

#define CRECDDCR(R, tn)     const cRecStaticDescr&  R::descr() const { return *getPDescr<R>(tn); }

/// @def CRECDEFD(R)
/// A cRecord leszármazottakban a destruktort, a newObj() és dup() virtuális metódusokat definiáló makró
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CRECDEFD(R)    CRECDEF(R) R::~R() { ; }

/// @def DEFAULTCRECDEF(R)
/// Egy default cRecord leszármazott objektum teljes definíciója
#define DEFAULTCRECDEF(R, tn)   CRECCNTR(R) CRECDEFD(R) CRECDDCR(R, tn)

/// Az objektum egy referencia, valamely cRecord leszármazott példány egy mezőjére.
class LV2SHARED_EXPORT cRecordFieldConstRef {
    friend class cRecord;
    friend class cRecordFieldRef;
protected:
    const cRecord&  _record;
    int             _index;
    cRecordFieldConstRef(const cRecord& __r, int __ix) : _record(__r) {
        _index = __ix;
        _record.chkIndex(_index);
    }
    cRecordFieldConstRef(const cRecord& __r,const QString& __n)  : _record(__r) {
        _index = __r.toIndex(__n);
        _record.chkIndex(_index);
    }
public:
    cRecordFieldConstRef(const cRecordFieldConstRef& __r) : _record(__r._record) {
        _index = __r._index;
    }
    operator const QVariant&() const   { return _record.get(_index); }
    operator qlonglong() const  { return _record.getId(_index); }
    operator QString() const    { return _record.getName(_index); }
    QString toString() const    { return *this; }
    const int& stat() { return _record._stat; }
    const cRecord& record() const           { return _record; }
    const cRecStaticDescr& recDescr() const { return _record.descr(); }
    const cColStaticDescr& descr() const    { return _record.descr().colDescr(_index); }
    int index()                             { return _index; }
};
TSTREAMO(cRecordFieldConstRef)

/// Az objektum egy konstans referencia, valamely cRecord leszármazott példány egy mezőjére.
class LV2SHARED_EXPORT cRecordFieldRef {
    friend  class cRecord;
protected:
    cRecord&    _record;
    int         _index;
    cRecordFieldRef(cRecord& __r, int __ix) : _record(__r) {
        _index = __ix;
        _record.chkIndex(_index);
    }
    cRecordFieldRef(cRecord& __r,const QString& __n)  : _record(__r) {
        _index = __r.toIndex(__n);
        _record.chkIndex(_index);
    }
public:
    cRecordFieldRef(const cRecordFieldRef& __r) : _record(__r._record) {
        _index = __r._index;
    }
    cRecordFieldRef operator=(const QVariant& __v) const {
        _record.set(_index, __v);
        return *this;
    }
    cRecordFieldRef operator=(const cRecordFieldRef& __r) {
        _record.set(_index, __r);
        return *this;
    }
    cRecordFieldRef operator=(const cRecordFieldConstRef& __r) {
        _record.set(_index, __r);
        return *this;
    }
    cRecordFieldRef operator=(qlonglong __v) const {
        _record.setId(_index, __v);
        return *this;
    }
    cRecordFieldRef operator=(const QString& __v) const {
        _record.setName(_index, __v);
        return *this;
    }
    operator const QVariant&() const   { return _record.get(_index); }
    operator qlonglong() const  { return _record.getId(_index); }
    operator QString() const    { return _record.getName(_index); }
    cRecordFieldRef& clear()    { _record.clear(_index); return *this; }
    QString toString() const    { return *this; }
    QString view(QSqlQuery& q)  { return _record.view(q, _index); }
    int& stat()                 { return _record._stat; }
    bool update(QSqlQuery& q, bool __ex = true) const {
        return _record.update(q, false, _record.mask(_index), _record.primaryKey(), __ex);
    }
    const cRecord& record() const           { return _record; }
    const cRecStaticDescr& recDescr() const { return _record.descr(); }
    const cColStaticDescr& descr() const    { return _record.descr().colDescr(_index); }
    int index()                             { return _index; }
};
TSTREAMO(cRecordFieldRef)

inline cRecordFieldRef cRecord::operator[](const QString& __fn)             { return (*this)[chkIndex(toIndex(__fn))]; }
inline cRecordFieldConstRef cRecord::operator[](const QString& __fn) const  { return (*this)[chkIndex(toIndex(__fn))]; }

/*!
@class cAlternate
@brief Álltalános interfész osztály.
Azt hogy melyik adattáblát kezeli az a konstruktorban, vagy a setType() metódusokban kell beállítani.
Természetesen csak az alapfunkciókra képes, amik a cRecord-ban meg lettek valósítva.
 */
class cAlternate : public cRecord {
public:
    /// Üres konstruktor. Használat elütt hivni kell valamelyik setType() metódust.
    /// Ha nincs beállítva típus, akkor minden olyan hívásra, melynek szüksége van
    /// a descriptor objektumra egy kizárást fog dobni.
    cAlternate();
    /// A konstruktor a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// objektumot. Az objektum adatt tartalma (mező adat konténer) üres lessz.
    /// @param _tn Az adattáble neve
    /// @param _sn A schema név mely az adattáblát tartalmazza. Obcionális, alapértelmezett a "public".
    cAlternate(const QString& _tn, const QString& _sn = _sNul);
    /// A konstruktor a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// Az objektum adatt tartalma (mező adat konténer) üres lessz.
    /// @param _p A leíró objektum pointere
    cAlternate(const cRecStaticDescr * _p);
    /// Copy konstruktor
    cAlternate(const cAlternate &__o);
    /// "Univerzális" copy (szerű) konstruktor
    cAlternate(const cRecord &__o);
    /// A metódus a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// objektumot. Az objektum adatt tartalma (mező adat konténer) üres lessz, ill. azt törli.
    /// @param _tn Az adattáble neve
    /// @param _sn A schema név mely az adattáblát tartalmazza. Obcionális, alapértelmezett a "public".
    cAlternate& setType(const QString& _tn, const QString& _sn = _sNul);
    /// A metódus az __o -ban már megadott, vagy ezt implicite tartalmazó adattáblához rendeli, ill. annak kezelésére készíti fel.
    /// Ha __o-nál nincs megadva adattábla, ill. ismeretlen a descriptor objektum, akkor dobni fog egy kizárást.
    cAlternate& setType(const cRecord& __o);
    cAlternate& setType(const cRecStaticDescr *_pd);
    /// Alaphelyzetbe állítja az objektumot, mint az üres konstruktor után.
    cAlternate& clearType();
    /// A descriptor objektum referenciájával tér vissza.
    /// Ha nem adtuk meg a kezelendő adattáblát, akkor dob egy kizárást.
    virtual const cRecStaticDescr& descr() const;
    /// destruktor. (üres metódus).
    virtual ~cAlternate();
    /// Egy új objektum allokálása. Az új objektum örökli a hívó descriptorát.
    virtual cRecord *newObj() const;
    /// Másolatot készít az objektumról.
    virtual cRecord *dup()const;
    /// Másoló operátor. Átmásolja a forrás objektum descriptor mutatóját (típusát) és a mező adatokat.
    cAlternate& operator=(const cRecord& __o);
protected:
    const cRecStaticDescr *pStaticDescr;
};

/// Template függvény a "properties" mező felbontására
template <class S> void _SplitMagicT(S& o, bool __ex)
{
    if (o._pMagicMap == NULL) o._pMagicMap = new tMagicMap();
    else                      o._pMagicMap->clear();
    *o._pMagicMap = splitMagic(o.getName(S::_ixProperties), *o._pMagicMap, __ex);
}
/// Egy módosított map visszaírása a "properties" mezőbe.
template <class S> void _Magic2PropT(S& o)
{
    QString prop;
    if (o._pMagicMap != NULL) prop = joinMagic(*o._pMagicMap);
    if (o.getName(S::_ixProperties) != prop) { // Nem túl hatékony, de bonyi lenne.
        o._set(S::_ixProperties, QVariant(prop));   // nincs atEnd() !
        o._stat |= cRecord::ES_MODIFY;
    }
}

#endif // LV2DATAB_H
