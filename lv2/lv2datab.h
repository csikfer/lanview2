#ifndef LV2DATAB_H
#define LV2DATAB_H
#include "lanview.h"
#include <typeinfo>

/*!
@file lv2datab.h
Az adatbázis interfész bázis objektuma, és a rekord ill. mező leíró/kezelő objektumok.
*/

/// Egy adat típusról dönti el, hogy egész számként értelmezhető-e
/// @param id A típushoz tartozó QMetaType::Type érték.
EXT_  bool metaIsInteger(int id);
/// Egy QVariant adat típusáról dönti el, hogy egész számként értelmezhető-e
/// @param _v Az adat referenciája
static inline  bool variantIsInteger(const QVariant & _v)
{
    return metaIsInteger(_v.userType());
}
/// Egy adat típusról dönti el, hogy string-ként értelmezhető-e
/// @param id A típushoz tartozó QmetaType::Type érték.
EXT_  bool metaIsString(int id);
/// Egy QVariant adat típusáról dönti el, hogy string-ként értelmezhető-e
/// @param id Az adat referenciája
static inline  bool variantIsString(const QVariant & _v)
{
    return metaIsString(_v.userType());
}
/// Egy adat típusról dönti el, hogy lebegőpontos számként értelmezhető-e
/// @param id A típushoz tartozó QmetaType::Type érték.
EXT_  bool metaIsFloat(int id);
/// Egy QVariant adat típusáról dönti el, hogy lebegőpontos számként értelmezhető-e
/// @param _v Az adat referenciája
static inline  bool variantIsFloat(const QVariant & _v)
{
    return metaIsFloat(_v.userType());
}
/// Egy QVariant adat típusáról dönti el, hogy számként értelmezhető-e
/// @param _v Az adat referenciája
static inline  bool variantIsNum(const QVariant & _v)
{
    return variantIsInteger(_v) || variantIsFloat(_v);
}
/// Egy stringet bool-lá konvertál:
/// Ha a string (b) értéke "t", "true", "y", "yes", "on" vagy "1", akkor true-val tér vissza,
/// Ha a string (b) értéke "f", "false", "n", "no", "off" vagy "0", akkor false-val tér vissza,
/// Egyéb értékeknél, ha __ex nem EX_IGNORE, akkor dob egy kizárást, ha viszont __ex értéke EX_IGNORE, akkor
/// üres string esetén false, ellenkező esetben true a visszaadott érték.
EXT_ bool str2bool(const QString& _b, enum eEx __ex = EX_ERROR);
/// Hasonló az bool str2bool(const QString& _b, enum eEx __ex); függvényhez, de:
/// A true értéknek a TS_TRUE, a false értéknek a TS_FALSE felel meg.
/// Ha a _b paraméter üres vagy NULL string, akkor TS_NULL -lal tér vissza.
/// Ha __ex = EX_IGNORE, és a string értéke nem értelmezhető, akkor TS_NULL -lal tér vissza.
EXT_ eTristate str2tristate(const QString& _b, enum eEx __ex);

/// A bool-ból stringet csinál, a nyelvi beállításoknak megfelelően.
EXT_ QString langBool(bool b);

static inline QString langTristate(eTristate t)
{
    QString r;
    if (t != TS_NULL) r = langBool(toBool(t));
    return r;
}


/// A megadott nevű séma OID-jével tér vissza.
/// Ha nincs ilyen séma, vagy más hiba történt dob egy kizárást
/// @param q Az adatbázis művelethez használható objektum.
/// @param __s A séma neve
EXT_ qlonglong schemaoid(QSqlQuery q, const QString& __s);
/// A tableoid lekérdezése, hiba esetén dob egy kizárást. Ha nincs ilyen tábla akkor is kizárást dob.
/// @param q Az adatbázis művelethez használható objektum.
/// @param __t SQL tábla neve
/// @param __sid SQL tábla séma OID (vagy NULL_ID, ha az alapértelmezett "public" a séma)
/// @param __ex Ha értéke nem EX_IGNORE, és nem létezik a megadott nevű tábla, akkor a NULL:IS-vel tér vissza, és nem dob kizárást.
/// @return a tabloid
EXT_ qlonglong tableoid(QSqlQuery q, const QString& __t, qlonglong __sid = NULL_ID, enum eEx __ex = EX_ERROR);
/// A tableoid érték alapján visszaadja a tábla és séma nevet, ha nincs ilyen tábla, vagy hiba esetén dob egy kizárást.
/// @param q Az adatbázis művelethez használható objektum.
/// @param toid A table OID érték.
/// @return A first adattag a tábla neve, a second pedig a séma név.
EXT_ tStringPair tableoid2name(QSqlQuery q, qlonglong toid);

/// @def CHKENUM
/// @brief Egy enumerációs típus konverziós függvényeinek az ellenörzése.
///
/// Makró, a chkEnum() hívására egy cRecord leszármazot egy metódusában, tipikusan a statikus rekord leíró inicializálása után.
/// A rekord leíró objektum példányra mutató pointer neve _pRecordDescr.
/// A makró feltételezi, hogy a két konverziós függvény neve azonos, és csak a típusuk különböző (tE2S és tS2E)
/// Ha az enumeráció kezelés a hívás szerint hibás, akkor dob egy kizárást.
/// @param n Az enumeráció típusú mező neve, vagy indexe
/// @param f A konverziós függvény(ek) neve. A két függvény azonos nevű, de más típusú.
#define CHKENUM(n, f)   (*_pRecordDescr)[n].checkEnum( f, f, __FILE__, __LINE__,__PRETTY_FUNCTION__);

/// Az enumerációs típus (int) stringgé konvertáló függvény pointerének a típusa.
typedef const QString& (*tE2S)(int e, eEx __ex);
/// A stringet enumerációs típussá (int) konvertáló függvény pointerének a típusa.
typedef int (*tS2E)(const QString& n, eEx __ex);

/// @enum eReasons
/// @brief Okok ill. műveletek eredményei
enum eReasons {
    REASON_INVALID = -1,
    REASON_OK = 0,  ///< OK
    R_NEW,          ///< Új elem
    R_INSERT,       ///< Új elem beszúrása/beszúrva.
    R_REMOVE,       ///< Elem eltávolítása/eltávolítva
    R_EXPIRED,      ///< Lejárt, elévült
    R_MOVE,         ///< Az elem át-mozgatása/mozgatva
    R_RESTORE,      ///< Helyreálitás/helyreállítva
    R_MODIFY,       ///< Az elem módosítva (összetetteb módosítás)
    R_UPDATE,       ///< Az elem módosítva
    R_UNCHANGE,     ///< Nincs változás
    R_FOUND,        ///< Találat, az elem már létezik...
    R_NOTFOUND,     ///< Nincs találat, valamelyik objektum hiányzik.
    R_DISCARD,      ///< Nincs művelet, az adat eldobásra került.
    R_CAVEAT,       ///< Valamilyen ellentmondás van az adatok között
    R_AMBIGUOUS,    ///< kétértelmüség
    R_ERROR,        ///< Egyébb hiba
    R_CLOSE,        ///< Lezárva
    R_TIMEOUT,      ///< Idő tullépés történt
    R_UNKNOWN       ///< Ismeretlen
};

class cRecord;
class cRecordFieldRef;
class cRecordFieldConstRef;
template <class T> class tRecordList;
class cRecordAny;
class cColStaticDescrBool;

/* ******************************************************************************************************
   *                                         cColEnumType                                               *
   ******************************************************************************************************/
/*!
@class cColEnumType
@brief
Object containing SQL enumeration field type properties.
*/
class LV2SHARED_EXPORT cColEnumType : public QString {
    friend class cColStaticDescrBool;
protected:
    cColEnumType(qlonglong id, const QString& name, const QStringList& values);
public:
    cColEnumType(const cColEnumType& _o);
    static const cColEnumType *fetchOrGet(QSqlQuery& q, const QString& name, enum eEx __ex = EX_ERROR);
    QString     toString() const;
    const qlonglong   typeId;
    const QStringList enumValues;
    bool check(const QString& v) const  { return enumValues.contains(v, Qt::CaseInsensitive); }
    bool check(const QStringList& v) const;
    bool checkSet(qlonglong b) const    { return b < (1LL << enumValues.size()); }
    bool checkEnum(int i) const   { return i < enumValues.size(); }
    const QString& enum2str(qlonglong e, enum eEx __ex = EX_ERROR) const;
    QStringList set2lst(qlonglong b, enum eEx __ex = EX_ERROR) const;
    int str2enum(const QString& s, enum eEx __ex = EX_ERROR) const;
    qlonglong str2set(const QString& s, enum eEx __ex = EX_ERROR) const;
    qlonglong lst2set(const QStringList& s, enum eEx __ex = EX_ERROR) const;
    tIntVector lst2lst(const QStringList& _lst, enum eEx __ex = EX_ERROR) const;
    QStringList lst2lst(const tIntVector& _lst, enum eEx __ex = EX_ERROR) const;
    QString     normalize(const QString& nm, bool *pok = nullptr) const;
    QStringList normalize(const QStringList& lst, bool *pok = nullptr) const;
    void checkEnum(tE2S e2s, tS2E s2e) const;
    static void checkEnum(QSqlQuery &q, const QString &_type, tE2S e2s, tS2E s2e);
private:
    // Egy belső konstruktor a kereséshez
    cColEnumType(const QString& _n) :QString(_n), typeId(NULL_ID) , enumValues() { ; }
    static QSet<cColEnumType>  colEnumTypes;
public:
    static const cColEnumType *find(const QString& name);
    static const cColEnumType& get(const QString& name);
};

TSTREAMO(cColEnumType)


/* ******************************************************************************************************
   *                                         cColStaticDescr                                            *
   ******************************************************************************************************/
class cRecStaticDescr;
class cRecord;

/*!
@class cColStaticDescr
@brief 
SQL tábla mező tulajdonságait tartalmazó objektum.

Az osztály írja le a cRecStaticDescr osztályban (annak részeként) a mezők tulajdonságait, ill. a virtuális metódusok végzik a mező adatkonverzióit.
Az QString ős objektum a mező nevét tartalmazza
Az alap osztály a numerikus skalár, és string típusú mezők adatkonverzióit végzi.
*/
class LV2SHARED_EXPORT cColStaticDescr : public QString {
    friend class cRecStaticDescr;
    friend class cRecord;
public:
    /// A mező típus konstansok
    enum eFieldType {
        FT_ARRAY  = 0x40000000L,///<A mező egy tömb
        FT_ANY    =          0, ///< ismeretlen
        FT_TIME,                ///< idő (time)
        FT_DATE,                ///< dátum (date)
        FT_DATE_TIME,           ///< dátum és idő (timestamp)
        FT_INTERVAL,            ///< Idő intervallum (interval)
        FT_INTEGER,             ///< egész szám (minden egész qlonglon-ban tárolódik)
        FT_REAL,                ///< lebegőpontos szám
        FT_TEXT,                ///< szöveges adat
        FT_BINARY,              ///< bináris (bytea)
        FT_BOOLEAN,             ///< bináris érték (boolean)
        FT_POINT,               ///< point
        FT_POLYGON,             ///< poligon (polygon)
        FT_ENUM,                ///< enumerációs típus
        FT_SET          = FT_ENUM    + FT_ARRAY, ///< set típus, ami az adatbázisban egy enumerációs tömb
        FT_INTEGER_ARRAY= FT_INTEGER + FT_ARRAY, ///< egész tömb
        FT_REAL_ARRAY   = FT_REAL    + FT_ARRAY, ///< lebegőpontos tömb
        FT_TEXT_ARRAY   = FT_TEXT    + FT_ARRAY, ///< Söveg tömb
        FT_MAC    = 0x00000100L,///< MAC cím
        FT_INET,                ///< hálózati cím
        FT_CIDR,                ///< Hálózati címtartomány
        FT_ADDR_MASK= 0x00000100L,///< Mask a cím típusok felismeréséhez
        ///< Titkosított adat
        FT_CRYPTED  = 0x00000200L,
        FT_NEXT_TYPE= 0x00001000L ///< esetleges további típusok ...
    };
    /// Foreingn keys, Not only are SQL supported keys
    enum eFKeyType {
        FT_NONE     = 0,            ///< Not a foreign key
        FT_PROPERTY = FT_NEXT_TYPE, ///< Foreign key to property
        FT_OWNER,                   ///< Foreign key to owner
        FT_SELF,                    ///< Foreign key to self (tree)
        FT_TEXT_ID                  ///< This is text_id
    };
    /// Egy mező érték ellenörzésének az eredménye.
    enum eValueCheck {
        VC_INVALID = 0,     ///< Nem megengedett érték, nem konvertálható
        VC_TRUNC,           ///< Típuskonverzió szükséges, de csak adatvesztéssel (csonkolás)
        VC_NULL,            ///< NULL, a mező felveheti a NULL értéket
        VC_DEFAULT,         ///< NULL, nem lehet NULL, de van alapértelmezett érték (DEFAULT)
        VC_CONVERT,         ///< Értékhatáron bellül, típuskonverzió szükséges, és lehetséges
        VC_OK               ///< Értékhatáron bellül típus azonos érték
    };
    /// Konstruktor, egy üres objektumot hoz létre
    /// @param __t Ha megadjuk a paramétert, akkor az eColType adattag ezt az értéket fogja felvenni.
    cColStaticDescr(const cRecStaticDescr *_p, int __t = FT_ANY);
    /// Copy konstruktor
    cColStaticDescr(const cColStaticDescr& __o);
    virtual ~cColStaticDescr();
    /// Másoló operátor
    cColStaticDescr& operator=(const cColStaticDescr __o);
    /// A QString ős objektum referenciájával tér vissza, ami a mező neve
    QString&        colName()       { return *static_cast<QString *>(this); }
    /// A QString ős objektum konstatns referenciájával tér vissza, ami a mező neve
    const QString&  colName() const { return *static_cast<const QString *>(this); }
    /// A mező név idézőjelek között
    QString colNameQ() const        { return dQuoted(colName()); }
    /// Kideríti a mező típusát a szöveges típus nevek alapján, vagyis meghatározza az eColType adattag értékét.
    void typeDetect();
    // virtuális metódusok
    /// Adat ellenörzés. Megvizsgálja, hogy a megadott érték hozzárendelhető-e a mezőhöz.
    /// Csak elleőrzi az értéket.
    /// @param v Az ellenőrizendő érték
    /// @param acceptable Ha az elenörzés értéke kisebb, akkor dob egy kizárást. Alapértelmezetten nincs kizárás.
    /// @return Az ellenörzés eredményével tér vissza, lsd.: az eValueCheck típust.
    virtual enum cColStaticDescr::eValueCheck check(const QVariant& v, cColStaticDescr::eValueCheck acceptable = cColStaticDescr::VC_INVALID) const;
    /// Az adatbázisból beolvasott adatot konvertálja a rekord konténerben tárolt formára.
    virtual QVariant fromSql(const QVariant& __f) const;
    /// A rekord konténerben tárolt formából konvertálja az adatot, az adatbázis kezelőnek átadható formára.
    virtual QVariant toSql(const QVariant& __f) const;
    /// A mező értékadásakor végrehajtandó esetleges konverziók
    /// @param _f Forrás adat
    /// @param rst A rekord status referenciája
    /// @return A cRecord-ban tárolt formátumra konvertált adat. Ha nem sikerült az adatot a megfelelő
    /// formátumra konvertálni, akkor str -be bebillenti az ES_DEFECTIVE bitet.
    virtual QVariant set(const QVariant& _f, qlonglong &str) const;
    /// Konverzió érték kiolvasásakor, ha az eredményt string formában kérjük
    /// @param _f Forrás adat, a mező értéke.
    /// @return A strinngé konvertált érték.
    virtual QString toName(const QVariant& _f) const;
    /// Konverzió érték kiolvasásakor, ha az eslredményt integer formában kérjük. Nem feltétlenül ad vissza értelmezhető adatot.
    /// @param _f Forrás adat, a mező értéke.
    /// @return Az egésszé konvertált érték, vagy NULL_ID ha az adat nem konvertálható.
    virtual qlonglong toId(const QVariant& _f) const;
    /// Hasonló a toName() metódushoz, de ebben az esetben ember álltal értelmezhető formára törekszik. Ha _f értéke
    /// NULL, akkor a „[NULL]” stringgel, ha viszont egy távoli kulcs, akkor annak a rekordnak a nevével, melynek az
    /// ID-jét adtuk meg az _f -ben, ill. ha nincs ilyen rekord, akkor az ID-vel, de beszúr elé egy '#' karaktert.
    /// @param _f Forrás adat, a mező értéke.
    /// @return A strinngé konvertált érték.
    virtual QString toView(QSqlQuery& q, const QVariant& _f) const;
    /// Usually identical as toView()
    virtual QString toViewIx(QSqlQuery&q, const QVariant& _f, int _ix) const;
    /// Clone object
    /// Az eredeti osztály copy konstruktorát hívja az alapértelmezett definíció.
    virtual cColStaticDescr *dup() const;
    /// Az enumeráció kezelés konzisztenciájának ellenörzése.
    /// Enumeráció esetén a numerikus érték az adatbázisban az enum típusban megadott listabeli sorszáma (0,1 ...)
    /// SET esetén pedig a numerikus értékben a megadott sorszámú bit reprezentál egy enumerációs értéket.
    /// Az API-ban lévő sring - enumeráció kovertáló függvényeknél ügyelni kell arra, hogy a C-ben definiált
    /// enumerációs értékek megfeleljenek az adatbázisban a megfelelő enumerációs érték sorrendjének.
    /// A vizsgálat csak az adatbázis szerini értékek irányából ellenőriz, a konverziós függvényeknél,
    /// a numerikusból stringbe konvertálásnál csak azt ellenörzi, hogy az utolsó elem után ad-e vissza nem null értéket.
    /// @param e2s Az enumerációból stringgé konvertáló függvény pointere
    /// @param s2e A stringból enumerációs konstanba konvertáló függvény pointere.
    ///
    /// @exception cError *, Ha sikerült eltérést detektálni a kétféle enum értelmezés között.
    void checkEnum(tE2S e2s, tS2E s2e, const char * src, int lin, const char * fn) const;
    // Adattagok
    ///
    const cRecStaticDescr *pParent;
    /// A mező pozíciója, nem zéró pozitív szám (1,2,3...)
    int         pos;
    /// A mező sorrendet jelző szám (ordinal_position), jó esetben azonos pos-al, de ha mező volt törölve, akkor nem folytonos
    int         ordPos;
    /// character maximum lenght
    int         chrMaxLenghr;
    /// A mező (oszlop) típusa, lásd: eFieldType
    int         eColType;
    /// Ha a mező módosítható, akkor értéke igaz
    bool        isUpdatable;
    /// Ha igaz, akkor a mező felveheti a NULL értéket
    bool        isNullable;
    /// A távoli kulcs típusa
    eFKeyType   fKeyType;
    /// A mező típusa. Az adatbázisból kiolvasott típus string (information_schema.columns.data_type)
    /// Ha a mező tömb, vagy felhasználó által definiált típus, akkor a
    /// colType értéke "ARRAY" ill. "USER-DEFINED"
    QString     colType;
    /// Szintén típus, tömb ill. USER DEFINED típus esetén ez tartalmazza a típus nevet.
    QString     udtName;
    /// A mező alapértelmezett értéke (SQL szintaxis)
    QString     colDefault;
    /// Enumerációs ill. set típus esetén a leíró pointere, vagy NULL pointer
    const cColEnumType *pEnumType;
    /// Enumerációs ill. set típus leíró referenciája, ha nincs dob egy kizárást.
    const cColEnumType& enumType() const { if (pEnumType == nullptr) EXCEPTION(EDATA); return *pEnumType; }
    /// Enumerációs ill. set típus leíró pointere, vagy NULL
    const cColEnumType *getPEnumType() const { return pEnumType; }
    /// Ha a mező egy távoli kulcs, akkor a tábla séma neve, ahol távoli kulcs
    QString     fKeySchema;
    /// Ha a mező egy távoli kulcs, akkor a tábla neve, ahol távoli kulcs
    QString     fKeyTable;
    /// Ha a mező egy távoli kulcs, akkor a kulcsmező neve, a távoli táblában
    QString     fKeyField;
    /// Ha a mező egy távoli kulcs, akkor a táblák nevei, ahol távoli kulcs, ha egy öröklési láncról van szó
    QStringList fKeyTables;
    /// Egy opcionális függvény név, ami ID-k esetén elvégzi a stringgé (névvé) konvertálást
    QString     fnToName;
    /// Az objektum tartalmát striggé konvertálja pl. nyomkövetésnél használható.
    QString toString() const;
    /// Az objektum tartalmát striggé konvertálja pl. nyomkövetésnél használható.
    /// Részletesebb mint a toString() metódus.
    QString allToString() const;
    /// Ha a mező egy távoli kulcs, és nem önmagára hivatkozik, akkor a hivatkozott objektumra mutat, egyébként NULL.
    cRecordAny *pFRec;
    /// Az eValueCheck enumerációs értéket reprezentáló stringgel tér vissza.
    static const QString valueCheck(int e);
    ///
    int fieldIndex() const { return pos -1; }
    static const QString  rNul;
    static const QString  rBin;
    QString fKeyId2name(QSqlQuery &q, qlonglong id, eEx __ex = EX_IGNORE) const;
protected:
    eValueCheck checkIfNull() const {
        if (isNullable)             return cColStaticDescr::VC_NULL;     // NULL / OK
        if (colDefault.isEmpty())   return cColStaticDescr::VC_INVALID;  // NULL / INVALID
        else                        return cColStaticDescr::VC_DEFAULT;  // NULL / DEFAULT
    }
    eValueCheck ifExcep(eValueCheck result, eValueCheck acceptable, const QVariant &val) const;
};
TSTREAMO(cColStaticDescr)

/// @class cColStaticDescrBool
/// Az ős cColStaticDescr osztályt a boolean típus konverziós függvényivel egészíti ki.
class LV2SHARED_EXPORT cColStaticDescrBool : public cColStaticDescr {
    friend class cRecStaticDescr;
public:
    /// A konstruktor kitölti a enumType pointert is, hogy enumerációként is kezelhető legyen
    cColStaticDescrBool(const cColStaticDescr& __o) : cColStaticDescr(__o)
    {
        init();
    }
    enum cColStaticDescr::eValueCheck  check(const QVariant& v, cColStaticDescr::eValueCheck acceptable = cColStaticDescr::VC_INVALID) const;
    virtual QVariant  fromSql(const QVariant& __f) const;
    virtual QVariant  toSql(const QVariant& __f) const;
    virtual QVariant  set(const QVariant& _f, qlonglong &str) const;
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
/// A makróban a class definíció nincs lezárva a '}' karakterrel!
#define CSD_INHERITOR(T) \
    class LV2SHARED_EXPORT T : public cColStaticDescr { \
    friend class cRecStaticDescr; \
      public: \
        T(const cColStaticDescr& __o) : cColStaticDescr(__o) { ; } \
        virtual enum cColStaticDescr::eValueCheck check(const QVariant& v, cColStaticDescr::eValueCheck acceptable = cColStaticDescr::VC_INVALID) const; \
        virtual QVariant  fromSql(const QVariant& __f) const; \
        virtual QVariant  toSql(const QVariant& __f) const; \
        virtual QVariant  set(const QVariant& _f, qlonglong &rst) const; \
        virtual QString   toName(const QVariant& _f) const; \
        virtual qlonglong toId(const QVariant& _f) const; \
        virtual cColStaticDescr *dup() const;

/// @class cColStaticDescrAddr
/// Az ős cColStaticDescr osztályt az macaddr, inet és cidr típus konverziós függvényivel egészíti ki.
CSD_INHERITOR(cColStaticDescrAddr)
};

/// @class cColStaticDescrArray
/// Az ős cColStaticDescr osztályt a általános tömb típus konverziós függvényeivel egészíti ki.
CSD_INHERITOR(cColStaticDescrArray)
public:
    virtual QString toView(QSqlQuery& q, const QVariant& _f) const;
    static const QVariant emptyVariantList;
    static const QVariant emptyStringList;
};

/// @class cColStaticDescrPoint
/// Az ős cColStaticDescr osztályt a point típus konverziós függvényeivel egészíti ki.
CSD_INHERITOR(cColStaticDescrPoint)
};

/// @class cColStaticDescrPolygon
/// Az ős cColStaticDescr osztályt a poligon típus konverziós függvényeivel egészíti ki.
CSD_INHERITOR(cColStaticDescrPolygon)
};

/// @class cColStaticDescrEnum
/// Az ős cColStaticDescr osztályt automatikusan kezelt enumeráció konverziós függvényeivel egészíti ki.
CSD_INHERITOR(cColStaticDescrEnum)
};

/// @class cColStaticDescrSet
/// Az ós cColStaticDescr osztályt automatikusan kezelt set (enumeráció tömb) konverziós függvényeivel egészíti ki.
CSD_INHERITOR(cColStaticDescrSet)
};

/// @class cColStaticDescrDate
/// Az ős cColStaticDescr osztályt dátum konverziós függvényeivel egészíti ki.
/// Nincs implementálva.
CSD_INHERITOR(cColStaticDescrDate)
};

/// @class cColStaticDescrTime
/// Az ós cColStaticDescr osztályt idő konverziós függvényeivel egészíti ki.
CSD_INHERITOR(cColStaticDescrTime)
};

/// @class cColStaticDescrDateTime
/// Az ős cColStaticDescr osztályt dátum és idő (timestamp) konverziós függvényeivel egészíti ki.
/// Mindig QDateTime típusban tárolódik kivéve a NULL, és a NOW, ez utóbbi QString.
CSD_INHERITOR(cColStaticDescrDateTime)
};

/// @class cColStaticDescrInterval
/// Az ős cColStaticDescr osztályt idő intervallum konverziós függvényeivel egészíti ki.
/// Az intervallum qlonglong-ban tárolódik, és mSec-ben értendő.
/// Negatív tartomány nincs értelmezve, és az óra:perc:másodperc formán túl csak a napok megadása támogatott.
CSD_INHERITOR(cColStaticDescrInterval)
};

/// Egy adatbázisból beolvasott bool értéket kovertál stringgé
inline static const QString& boolFromSql(const QVariant& __f) { return __f.isNull() ? _sNul : str2bool(__f.toString(), EX_IGNORE) ? _sTrue : _sFalse; }

QString intervalToStr(qlonglong i);

/* ******************************************************************************************************
   *                                         cColStaticDescrList                                        *
   ******************************************************************************************************/
/// @class cColStaticDescrList
/// SQL tábla mezőinek listája, (egy tábla mezőinek a tulajdonságát leíró lista)
/// A listában a mezők kötött, az adattáblával azonos sorrendben szerepelnek.
class LV2SHARED_EXPORT cColStaticDescrList : public QList<cColStaticDescr *> {
public:
    typedef QList<cColStaticDescr *>::iterator          iterator;
    typedef QList<cColStaticDescr *>::const_iterator    const_iterator;
    typedef QList<cColStaticDescr *>                    list;
    /// üres listát létrehozó konstruktor
    cColStaticDescrList(cRecStaticDescr *par);
    /// Copy konstruktor, nem támogatott, dob egy kizárást.
    cColStaticDescrList(const cColStaticDescrList&) : QList<cColStaticDescr *>() { EXCEPTION(ENOTSUPP); }
    /// Destruktor
    virtual ~cColStaticDescrList();
    /// Az operátor egy mező leírót hozzáad a listához.
    /// @exception cError* Csak sorrendben adhatóak a listához a mezők, vagyis az __o.pos -nak meg kell
    /// egyeznie a lista új (elem hozzáadása utáni) méretével.
    cColStaticDescrList& operator<<(cColStaticDescr *__o);
    /// Hasonló a pointeres operátor változathoz, de a copy konstruktorral létrehozott objektumot illeszti be.
    cColStaticDescrList& operator<<(const cColStaticDescr& __o) { (*this) << new cColStaticDescr(__o); return *this; }
    /// Megketesi az elemet a listában
    /// @param __n a keresett mező neve
    /// @return ha van ilyen nevű mező a listában, akkor annak az indexe, ha nincs, akkor -1.
    int toIndex(const QString& __n, enum eEx __ex = EX_ERROR) const;
    /// A megadott nevű mező leírójánask a referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    cColStaticDescr& operator[](const QString& __n);
    /// A megadott nevű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    const cColStaticDescr& operator[](const QString& __n) const;
    /// A megadott indexű mező leírójának a referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param i a keresett mező indexe (0,1,...).
    cColStaticDescr& operator[](int i);
    /// A megadott indexű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param i a keresett mező indexe (0,1,...).
    const cColStaticDescr& operator[](int i) const;
    /// parent
    const cRecStaticDescr *pParent;
    QString fullColName(const QString& _col) const;
    QString tableName() const ;
};

/* ******************************************************************************************************
   *                                         cRecStaticDescr                                            *
   ******************************************************************************************************/
/*! @enum eTableType
   Tábla típus konstansok.
   Az egyszerű switch típusú táblához nem tartozik konstans, mert azokat nem cRecord típusú objektummal kezeljük.
 */
typedef enum {
    UNKNOWN_TABLE   = 0,
    TT_BASE_TABLE,     ///< Egyszerű tábla
    TT_SWITCH_TABLE,   ///< Switch tábla + tuajdonságok
    TT_LINK_TABLE,     ///< Link tábla (+ szimmetrikus VIEW)
    TT_CHILD_TABLE,    ///< A tábla olyan objektumot ír le, mely egy másik tulajdona
    TT_MASK         = 0x00ff,
    TT_VIEW_TABLE   = 0x0100,     ///< Nézet tábla
}   eTableType;

/// Egy rekord tulajdonságait leíró objektum. A rekord adat objektumban a pointere egy statikus adattag (kivéve cRecordAny)
/// Az összes létrehozott objektum a statikus _recDescrMap konténerben a tábla OID szerint van elhelyezve.
/// Így egy táblához csak egy objektum készül, és csak akkor ha hivatkozás történik rá.
class LV2SHARED_EXPORT cRecStaticDescr {
    friend class cRecord;
protected:
    /// Az eddig létrehozott objektumok konténere, a tábla OID-vel kulcsolva
    static QMap<qlonglong, cRecStaticDescr *>   _recDescrMap;
    /// A _recDescrMap konténerrel végzett műveleteket védő mutex objektum
    static QMutex   _mapMutex;
    /// Ha a bármelyik mező módosítható, akkor értéke true
    bool                _isUpdatable;
    /// Az adattábla séma neve
    QString             _schemaName;
    /// Az adattábla neve
    QString             _tableName;
    /// Opcionális nézet tábla neve. Ha nincs VIEW tábla, ill. csak az van, akkor azonos a _tableName értékével.
    /// A fetch-ek erre a tábla névre vonatkoznak. Ha a két tábla név nem azonos, akkor feltételezés, hogy a két tábla
    /// rekord szerkezete azonos. Ezt a rendszer nem ellenörzi, de hibás működéshez vezethet.
    QString             _viewName;
    /// A tábla OID-je
    qlonglong           _tableOId;
    /// A séma (schema/namespace) OID-je
    qlonglong           _schemaOId;
    /// Az ID mező indexe, vagy NULL_IX (negatív), ha nincs, vagy nem ismert
    int                 _idIndex;
    /// Az név mező indexe, vagy NULL_IX (negatív), ha nincs, vagy nem ismert
    int                 _nameIndex;
    /// Az note (title) mező indexe, vagy NULL_IX (negatív), ha nincs, vagy nem ismert
    int                 _noteIndex;
    /// A deleted mező indexe, vagy NULL_IX (negatív), ha nincs ilyen mező
    int                 _deletedIndex;
    /// A flag mező indexe, vagy NULL_IX (negatív), ha nincs ilyen mező
    int                 _flagIndex;
    ///
    int                 _textIdIndex;
    // / A tábla tulajdonságát leíró adatrekord kerül ide beolvasásra
    // QSqlRecord          _tableRecord;
    /// A tábla oszlopainak tulajdonságait leíró adatrekordok kerülnek ide beolvasásra.
    QList<QSqlRecord>   _columnRecords;
    /// Az adattábla oszlopait leíró objektumok listája
    cColStaticDescrList _columnDescrs;
    /// Az adattábla típusa
    quint16             _tableType; // enum eTableType
    /// Az objektumban a Primary Key-hez rendelt mezőkkel azonos indexű bitek 1-be vannak állítva.
    /// Ha nincs Primary Key, akkor nincs egyes bit a tömbben.
    QBitArray           _primaryKeyMask;
    /// Egy egyedi kulcsot definiáló mask, ami vagy csak a név mezőt tartalmazza, vagy azon mezőket is, amikkel együtt a név mez egyedi lessz.
    /// Ha nincs név mező, vagy több ilyen maszk is lehetséges, akkor nem lessz egyes bit a tömbben
    QBitArray           _nameKeyMask;
    /// A Lista elemei hasonlóak az _primaryKeyMask-hoz, de az Egyedi kulcsokhoz tartozó
    /// mezőket azonosítja.
    QList<QBitArray>    _uniqueMasks;
    /// Az autó inkrementális mezők maszkja
    QBitArray           _autoIncrement;
    /// Az oszlopok száma a táblában
    int                 _columnsNum;
    /// Az rewrite metódusban a védet mezők, amik csak akkor kerülnek fellülírásra, ha az új érték nem NULL.
    QBitArray           _protectedForRewriting;
    /// Ha származtatott tábláról van szó (az adatbázisban), akkor az ős tábla rekord leíróinak a pointerei
    QVector<const cRecStaticDescr *>  _parents;
    /// Inicializálja az objektumot.
    /// @exception cError* Hiba esetén dob egy kizárást-
    /// @param _t SQL tábla neve
    /// @param _s SQL tábla séma név, opcionális, ha nincs megadva, akkor a séma neve 'public'
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
    /// Konstruktor. A _set() metódust hívja az inicializáláshoz. Ha ez kész, akkor beteszi az objektumot a
    /// _recDescrMap konténerbe, az addMap() metódus hívásával.
    cRecStaticDescr(const QString& __t, const QString& __s = QString());
    /// Destruktor
    ~cRecStaticDescr();
    /// Egy objektum "beszerzése". Ha a _recDescrMap konténerben megtalálja az objektumot, akkor a pointerével tér vissza.
    /// Ha nem, akkor létrehozza, inicializálja, beteszi a _recDescCache konténerbe, és a pointerrel tér vissza.
    /// @param _t SQL tábla neve
    /// @param _s SQL tábla schema név, opcionális
    /// @param find_only Ha true, akkor csak a konténerben keres, na nem találja az objektumot, akkor NULL-al tér vissza.
    /// @exception cError * Ha _setReCallCnt értéke nagyobb vagy egyenlő mint 10, és objektumot kellene kreálni, akkor dob egy kizárást.
    static const cRecStaticDescr *get(const QString& _t, const QString& _s = QString(), bool find_only = false);
    /// Egy objektum "beszerzése". Ha a _recDescrMap, vagy a _recDescCache konténerben megtalálja az objektumot, akkor a pointerével tér vissza.
    /// Ha nem, akkor létrehozza, inicializálja, beteszi a _recDescCache konténerbe, és a pointerrel tér vissza.
    /// @param _oid a tábla OID-je
    /// @param find_only Ha true, akkor NULL pointerrel tér vissza
    /// @exception cError * Ha _setReCallCnt értéke nagyobb vagy egyenlő mint 10, és objektumot kellene kreálni, akkor dob egy kizárást.
    static const cRecStaticDescr *get(qlonglong _oid, bool find_only = false);
    /// A tábla esetén megnézi, hogy létezik-e a rekord ID-t névvé konvertáló függvény.
    /// Ha létezik, akkor a nevével tér vissza. Ha nem, akkor megkísérli megkreálni.
    QString checkId2Name(QSqlQuery& q) const;
    /// A megadott tábla esetén megnézi, hogy létezik-e a rekord ID-t névvé konvertáló függvény.
    /// Ha létezik, akkor a nevével tér vissza. Ha nem, akkor megkísérli megkreálni.
    /// Ha a megadott tábla descriptora még nem létezik, és __ex false, akkor egy üres stringgel tér vissza.
    static QString checkId2Name(QSqlQuery &q, const QString& _tn, const QString& _sn, enum eEx __ex = EX_ERROR);
    /// Az index alapján vissza adja az oszlop leíró referenciáját.
    const cColStaticDescr& colDescr(int i) const { chkIndex(i); return _columnDescrs[i]; }
    /// Ha bármelyik mező módosítható az adatbázisban, akkor true-val tér vissza.
    bool isUpdatable() const                    { return _isUpdatable; }
    /// Ha a megadott indexű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen indexű mező, dob egy kizárást.
    bool isUpdatable(int i) const               { return colDescr(i).isUpdatable; }
    /// Ha a megadott nevű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen nevű mező, dob egy kizárást.
    bool isUpdatable(const QString& fn) const   { return colDescr(toIndex(fn)).isUpdatable; }
    /// Ellenőrzi a megadott mező indexet
    /// @param i Az ellenőrizendő mező/oszlop index érték
    /// @return ha i egy valós index, akkor true, ha nem false
    bool isIndex(int i) const { return (i >= 0 && i < _columnsNum); }
    /// Megnézi, hogy a megadott indexű elem része-e egy egyedi kulcsnak (a primary key-t nem vizsgálja !)
    bool isKey(int i) const;
    /// Ellenörzi, hogy a megadott név egy mezőnév-e
    /// @param __fn Az ellenőrizendő mező/oszlop név
    /// @return ha __fn egy valós mező név, akkor true, ha nem false
    bool isFieldName(const QString& __fn) const     { return isIndex(toIndex(__fn, EX_IGNORE)); }
    /// Ellenörzi a megadott mező/oszlop indexet.
    /// @param i Az ellenőrizendő index érték
    /// @param __ex
    /// @return i értékével tér vissza, ha nem index, akkor NULL_IX-el
    /// @exception cError* Ha i értéke nem lehet index, akkor dob egy kizárást
    int chkIndex(int i, eEx __ex = EX_ERROR) const {
        if (isIndex(i)) return i;
        if (__ex) EXCEPTION(ENOINDEX, i, fullTableName());
        return NULL_IX;
    }
    /// A megadott indexű mező nevével tér vissza.
    /// @exception cError * Ha i értéke nem lehet index. és __ex nem EX_IGNORE (alapértelmezett), akkor dob egy kizárást.
    const QString&  columnName(int i, enum eEx __ex = EX_ERROR) const  { return chkIndex(i, __ex) < 0 ? _sNul : static_cast<const QString&>(_columnDescrs[i]); }
    /// A megadott indexű mező nevével tér vissza. Hasonló a _columnName() metódushoz, de a visszaadott nevet kettős idézőjelek közé teszi
    QString columnNameQ(int i) const                 { return dQuoted(columnName(i)); }
    /// Megkeresi a megadott nevű mezőt a mező leíró listában
    /// Ha a névben pontok vannak, akkor feltételezi, hogy a névben szerepel a tábla név esetleg a séma név is.
    /// Ha meg van adva a tábla ill. a séma név, akkor azokat ellenőrzi, és ha nem egyeznek -1-el lép ki.
    /// Ha a név (ill. nevek) kettős idézőjelben vannak, akkor az összehasonlításokat az idézőjelek nélkül végzi.
    /// @param __n A keresett mező neve
    /// @param __ex Ha értéke nem EX_IGNORE, és nem találja a keresett mezőt, akkor dob egy kizárást.
    /// @return A mező indexe, vagy ha nem talált ilyen nevű mezőt, és __ex értéke nem EX_IGNORE, akkor NULL_IX (negatív).
    int toIndex(const QString& __n, enum eEx __ex = EX_ERROR) const { return _columnDescrs.toIndex(__n, __ex); }
    /// A séma nevet adja vissza
    const QString&  schemaName() const              { return _schemaName; }
    /// A tábla nevet adja vissza
    const QString&  tableName() const               { return _tableName; }
    /// A mező leíró lista referenciájával tér vissza
    const cColStaticDescrList& columnDescrs() const { return _columnDescrs; }
    /// A tábla OID értékkel tér vissza
    qlonglong tableoid() const                      { return _tableOId; }
    /// A tábla teljes nevével tér vissza, amit ha a séma név nem a "public" kiegészít azzal is, a tábla és séma név nincs idézőjelbe rakva.
    QString fullTableName() const                   { return _schemaName == _sPublic ? _tableName : mCat(_schemaName, _tableName); }
    /// A view tábla teljes nevével tér vissza, amit ha a séma név nem a "public" kiegészít azzal is, a tábla és séma név nincs idézőjelbe rakva.
    /// Lásd még a _viewName adattagot.
    QString fullViewName() const                    { return _schemaName == _sPublic ? _viewName : mCat(_schemaName, _viewName); }
    /// A teljes mező névvel (tábla és ha szükséges a séma névvel kiegészített) tér vissza. a nevek nincsenek idézőjelbe rakva.
    /// @param _i A mező indexe
    QString fullColumnName(int i) const{ return mCat(fullTableName(), columnName(i)); }
    /// A teljes mező névvel (tábla és ha szükséges a séma névvel kiegészített) tér vissza. a nevek nincsenek idézőjelbe rakva.
    /// @param _c (rövid) mező név
    QString fullColumnName(const QString& _c) const{ return mCat(fullTableName(), _c); }
    /// A tábla teljes nevével (ha szükséges a séma névvel kiegészített) tér vissza, a tábla és séma név idézőjelbe van rakva.
    QString fullTableNameQ() const                   { return _schemaName == _sPublic ? dQuoted(_tableName) : dQuotedCat(_schemaName, _tableName); }
    /// A view tábla teljes nevével (ha szükséges a séma névvel kiegészített) tér vissza, a tábla és séma név idézőjelbe van rakva.
    /// Lásd még a _viewName adattagot.
    QString fullViewNameQ() const                   { return _schemaName == _sPublic ? dQuoted(_viewName) : dQuotedCat(_schemaName, _viewName); }
    /// A teljes mező névvel (tábla és ha sükséges a séma névvel kiegészített) tér vissza. a nevek idézőjelbe vannak rakva.
    /// @param i a mező indexe
    QString fullColumnNameQ(int i) const             { return mCat(fullTableNameQ(), dQuoted(columnName(i))); }
    /// A teljes mező névvel (tábla és ha szükséges a séma névvel kiegészített) tér vissza. a nevek idézőjelbe vannak rakva.
    /// @param _c (rövid) mező név
    QString fullColumnNameQ(const QString& _c) const { return mCat(fullTableNameQ(), dQuoted(_c)); }
    ///
    QStringList columnNames(QBitArray mask) const;
    QStringList columnNames(tIntVector ixs) const;
    /// A tábla típusával tér vissza
    quint16 tableType() const                       { return _tableType; }
    /// A visszaadott objektum referenciában a Primary Key-hez rendelt mezükkel azonos indexű bitek 1-be vannak
    /// állítva. Ha nincs Primary Key, akkor nincs egyes bit a tömbben.
    /// @return Az elsőbleges kulcsként használt mezőket azonosító bit tömb objektum referenciája.
    const QBitArray& primaryKey() const             { return _primaryKeyMask; }
    /// A bit tömbök értelmezése hasonló mint a promaryKey() metódusnál.
    /// @return Az egyedi mezőket azonosító bit tümbök listálya
    const QList<QBitArray>& uniques() const         { return _uniqueMasks; }
    /// Egy bit vektorral tér vissza, ahol minden bit true, mellyel azonos indexű mező autoincrement.
    const QBitArray& autoIncrement() const          { return _autoIncrement; }
    /// Az ID mező indexével tér vissza, ha nincs ID, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int idIndex(enum eEx __ex = EX_ERROR) const     {
        if (__ex && !isIndex(_idIndex)) EXCEPTION(EFOUND, _idIndex, QObject::trUtf8("Table %1 nothing ID field.").arg(fullTableName()));
        return _idIndex;
    }
    /// Az NAME mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int nameIndex(enum eEx __ex = EX_ERROR) const   {
        if (__ex && !isIndex(_nameIndex)) EXCEPTION(EFOUND, _nameIndex, QObject::trUtf8("Table %1 nothing name field.").arg(fullTableName()));
        return _nameIndex;
    }
    /// Az NOTE mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int noteIndex(enum eEx __ex = EX_ERROR) const   {
        if (__ex && !isIndex(_noteIndex)) EXCEPTION(EFOUND, _noteIndex, QObject::trUtf8("Table %1 nothing note field.").arg(fullTableName()));
        return _noteIndex;
    }
    /// Az DELETED mező indexével tér vissza, ha nincs deleted mező, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int deletedIndex(enum eEx __ex = EX_ERROR) const {
        if (__ex && !isIndex(_deletedIndex)) EXCEPTION(EFOUND, _deletedIndex, QObject::trUtf8("Table %1 nothing delete field.").arg(fullTableName()));
        return _deletedIndex;
    }
    /// Az DELETED mező indexével tér vissza, ha nincs deleted mező, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int flagIndex(enum eEx __ex = EX_ERROR) const   {
        if (__ex && !isIndex(_flagIndex)) EXCEPTION(EFOUND, _flagIndex, QObject::trUtf8("Table %1 nothing flag field.").arg(fullTableName()));
        return _flagIndex;
    }
    /// Az text_id mező indexével tér vissza, ha nincs deleted mező, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int textIdIndex(enum eEx __ex = EX_ERROR) const   {
        if (__ex && !isIndex(_textIdIndex)) EXCEPTION(EFOUND, _textIdIndex, QObject::trUtf8("Object nothing text_id field.").arg(fullTableName()));
        return _textIdIndex;
    }

    /// A mezők számával tér vissza
    int cols() const                                { return _columnsNum; }
    /// A parent táblák leíróinak a pointerei, amennyiben a tábla származtatott, egyébként NULL pointer
    const QVector<const cRecStaticDescr *>& parent(void) const { return _parents; }
    /// Összehasonlítja két leíró séma és tábla nevét.
    /// Ha a séma és táblanév azonos, akkor true-val tér vissza
    bool operator==(const cRecStaticDescr& __o) const { return _schemaName == __o._schemaName && _tableName == __o._tableName; }
    /// Összehasonlítja két leíró séma és tábla nevét.
    /// Ha a séma vagy táblanév nem azonos, akkor true-val tér vissza
    bool operator!=(const cRecStaticDescr& __o) const { return _schemaName != __o._schemaName || _tableName != __o._tableName; }
    /// Ha az objektum által kezelt tábla leszármazottja az __o objektum által kezelt táblának, akkor true-val tér vissza
    bool operator<(const cRecStaticDescr& __o) const;
    /// Ha az objektum által kezelt  tábla __o objektum által kezelt táblából lett származtatva (ill. rekurzívan annak őséből), akkor true-val térvissza
    bool operator>(const cRecStaticDescr& __o) const { return __o < *this; }
    /// Ha a két objektum egyenlő, vagy 
    /// ha az objektum által kezelt tábla leszármazottja az __o objektum által kezelt táblának, akkor true-val tér vissza
    bool operator<=(const cRecStaticDescr& __o) const { return __o == *this || __o > *this; }
    /// Ha a két objektum egyenlő, vagy 
    /// ha az objektum által kezelt  tábla __o objektum által kezelt táblából lett származtatva
    /// (ill. rekurzívan annak őséből), akkor true-val tér vissza
    bool operator>=(const cRecStaticDescr& __o) const { return __o == *this || __o < *this; }
    /// A megadott nevű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    const cColStaticDescr& operator[](const QString& __n) const     { return _columnDescrs[__n]; }
    /// A megadott indexű mező leírójának a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param 1 a keresett mező indexe (0,1,...).
    const cColStaticDescr& operator[](int __i) const                { return _columnDescrs[__i]; }
    /// Az id mező nevével tér vissza, ha van id mező, egyébként dob egy kizárást
    /// @param __ex Ha értéke hamis és nincs id mező, akkor nem dob kizárást, hanem egy üres stringgel tér vissza.
    const QString& idName(enum eEx __ex = EX_ERROR) const   { return columnName(idIndex(__ex), __ex); }
    /// Az név mező nevével tér vissza, ha van név mező, egyébként dob egy kizárást
    /// @param __ex Ha értéke hamis és nincs név mező, akkor nem dob kizárást, hanem egy üres stringgel tér vissza.
    const QString& nameName(enum eEx __ex = EX_ERROR) const { return columnName(nameIndex(__ex), __ex); }
    /// Az descr mező nevével tér vissza, ha van descr mező, egyébként dob egy kizárást
    /// @param __ex Ha értéke hamis és nincs descr mező, akkor nem dob kizárást, hanem egy üres stringgel tér vissza.
    const QString& noteName(enum eEx __ex = EX_ERROR) const { return columnName(noteIndex(__ex), __ex); }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id.
    /// Hiba esetén, vagy ha nincs meg a a keresett ID, és __ex értéke true, akkor dob egy kizárást,
    /// Ha viszont __ex értéke false, és hiba van, vagy nincs ID akkor NULL_ID-vel tér vissza.
    qlonglong getIdByName(QSqlQuery& __q, const QString& __n, enum eEx __ex = EX_ERROR) const;
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Hiba esetén, vagy ha nincs meg a a keresett ID, és __ex értéke true, akkor dob egy kizárást,
    /// Ha viszont __ex értéke false, és hiba van, vagy nincs ID akkor NULL_ID-vel tér vissza.
    qlonglong getIdByName( const QString& __n, enum eEx __ex = EX_ERROR) const
        { QSqlQuery *pq = newQuery(); qlonglong id = getIdByName(*pq, __n, __ex); delete pq; return id; }
    /// A ID alapján visszaadja a rekord név mező értékét, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    QString getNameById(QSqlQuery& __q, qlonglong __id, eEx ex = EX_ERROR) const;
    /// A ID alapján visszaadja a rekord név mező értékét, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    QString getNameById(qlonglong __id, enum eEx __ex = EX_ERROR) const
        { QSqlQuery *pq = newQuery(); QString n = getNameById(*pq, __id, __ex); delete pq; return n; }
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
    /// Ha a megadott név bármelyike nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1, const QString& __n2) const
        { QBitArray m = mask(__n1); m.setBit(chkIndex(toIndex(__n2))); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyike nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3) const
        { QBitArray m = mask(__n1, __n2); m.setBit(chkIndex(toIndex(__n3))); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyike nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3, const QString& __n4) const
        { QBitArray m = mask(__n1, __n2, __n3); m.setBit(chkIndex(toIndex(__n4))); return m; }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyike nem egy mező név, akkor dob egy kizárást.
    QBitArray mask(const QStringList& __nl, enum eEx __ex = EX_ERROR) const;
    QBitArray inverseMask(const QBitArray& __m) const;
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 0-be állítja,
    /// a többit 1-be.
    /// Ha a megadott név bármelyike nem egy mező név, akkor dob egy kizárást.
    QBitArray inverseMask(const QStringList& __nl) const { return inverseMask(mask(__nl)); }

    /// Létrehoz egy tIntVector objektumot, egy elemmel, ami a megadott mező név indexe.
    tIntVector   iTab(const QString& __n1) const                      { return ::iTab(chkIndex(toIndex(__n1))); }
    /// Létrehoz egy tIntVector objektumot, két elemmel, ami a megadott mező nevek indexei.
    tIntVector   iTab(const QString& __n1, const QString& __n2) const { return ::iTab(chkIndex(toIndex(__n1)), chkIndex(toIndex(__n2))); }
    /// Létrehoz egy tIntVector objektumot, három elemmel, ami a megadott mező nevek indexei.
    tIntVector   iTab(const QString& __n1, const QString& __n2, const QString& __n3) const
        { return ::iTab(chkIndex(toIndex(__n1)), chkIndex(toIndex(__n2)), chkIndex(toIndex(__n3))); }
    /// Létrehoz egy tIntVector objektumot, négy elemmel, ami a megadott mező nevek indexei.
    tIntVector   iTab(const QString& __n1, const QString& __n2, const QString& __n3, const QString& __n4) const
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
    /// Ha a rekordban több távoli kulcs van, akkor az owner típusú indexével tér vissza.
    /// Ha nincs owner típusú, akkor a property típusú indexével, feltéve hogy csak egy van.
    /// Több távoli kulcs mező esetén, vagy ha ismert az idegen tábla, akkor a metódus másik változatát kell hívni.
    int ixToOwner(enum eEx __ex = EX_ERROR) const;
    ///
    int ixToOwner(const QString sFTable, enum eEx __ex = EX_ERROR) const;
    /// Az önmagára mutató távoli kulcs mező megkeresése
    int ixToParent(enum eEx __ex = EX_ERROR) const;
protected:
    bool addMap();
    static int _setReCallCnt;
};
TSTREAMO(cRecStaticDescr)

class LV2SHARED_EXPORT cLangTexts {
public:
    cLangTexts(cRecord *_par);
    ~cLangTexts();
    void setTexts(const QString& _langName, const QStringList& texts);
    void setTexts(int _langId, const QStringList& texts) { textMap[_langId] = texts; }
    // static QString tableName2textTypeName(const QString& _tn);
    static void saveText(QSqlQuery& q, const QStringList& texts, cRecord *po, int _lid = NULL_IX);
    static void saveText(QSqlQuery& q, const QString& sTableName, const cColEnumType* pEnumType, qlonglong tid, int _lid, const QStringList &texts);
    void saveTexts(QSqlQuery& q);
    void loadTexts(QSqlQuery& q);
protected:
    cRecord *               parent;
    QMap<int, QStringList>  textMap;    ///< language_id ==> texts
};
/* ******************************************************************************************************
   *                                              cRecord                                               *
   ******************************************************************************************************/
// enum eStat helyetti definíciók:
/// enum eStat: A rekord üres, és nincs típusa, ill nincs hozzárendelve adattáblához. Ez az állapot csak a cRecordAny típusú objektumokban lehetséges.
#define ES_FACELESS          -2LL
/// enum eStat: A rekord üres, a _fields adattagnak nincs egy eleme sem
#define ES_NULL              -1LL
/// enum eStat: A rekord üres, a _fields adattag feltöltve, de minden eleme null
#define ES_EMPTY         0x0000LL
/// enum eStat: O.K. Az adatbázisban létező rekord
#define ES_EXIST         0x0001LL
/// enum eStat: O.K. Az adatbázisban nem létező rekord
#define ES_NONEXIST      0x0002LL
/// enum eStat: Minden kötelező mező feltöltve
#define ES_COMPLETE      0x0004LL
/// enum eStat: Hiányos
#define ES_INCOMPLETE    0x0008LL
/// enum eStat: Módosítva
#define ES_MODIFY        0x0010LL
/// enum eStat: A rendelkezésre álló mezők egyértelműen azonosíthatnak egy rekordot
#define ES_IDENTIF       0x0020LL
/// enum eStat: A rendelkezésre álló mezők nem feltétlenül azonosítanak egy rekordot
#define ES_UNIDENT       0x0040LL
/// enum eStat: A rekord, vagy leíró inkonzisztens, nem konvertálható érték megadása
#define ES_DEFECTIVE     0x0080LL
/// enum eStat: A hibás értékadásban résztvevő mező(ke)t azonosító bit(ek), bit_ix = mező_ix + 8
#define ES_INV_FLD_MSK   0x0FFFFFFFFFFF0000LL

/// containef valid flag : texts
#define CV_LL_TEXT  1
/*!
@class cRecord
@brief Az adat rekord objektumok tisztán virtuális ős objektuma.

Az objektum leszármazottai egy-egy megfeletetést végeznek az adatbázis adattábláival. Ez az egy-egy megfeleltetés
nem mindig végezhető el, ill. nem mindíg eredményez teljes az ős objektumban definiált funkcionalitást.
Az adatkezelés általában virtuális metódusokon keresztül történik, azok a metódusok, amik
nem virtuálisak, '_' karakterrel kezdődnek, ebben az esetben ha szükség van a leíró
objektumra, akkor azt is paraméterként kell megadni. Ha a metódus nem hív virtuális metódust,
és nem '_'-al kezdődik, de alapvető jelentőségű metódus, akkor az általában inline metódusként
újra van definiálva '_' előtaggal.
Az adat _stat adattagja a legtöbb az objektumon végrehajtott műveletet követ, annak megfelelően áll be, de
az összetettebb vizsgálatot kívánó állapotokat nem képes autómatikusan követni.
Ezért bizonyos állapot lekérdező metódusoknak két változata van, az alap metódus, csak a _stat adattagot vizsgálja, míg a másik változat
átvizsgálja az objektumot, beállítja a _stat-ot, és ez alapján tér vissza, ilyen esetekben az a metódus,
mely csak a _stat alapján ad vissza értéket '_' karakterrel végződik, ha van értelme megkülönböztetésnek.

A cRecord objektum működésének alapja egy leíró objektum a cRecStaticDescr, mely a tábla tulajdonságait tartalmazza.
Ez az objektum minden leszármazott osztályhoz, a leszármazott osztályban egy statikus pointeren keresztül kapcsolódik,
mint egy típus leíró. (egy kivétellel, a cRecordAny ezt a pointert nem statikusan tartalmazza, vagyis annak „típusa”
dinamikus). Így minden leszármazott definíciójakor az egyedi tulajdonságoknak csak egy részét kell implementálni,
azok a tulajdonságok, melyek az adatbázisból is kiolvashatóak, a tábla azonosításával automatikusan érvényesülnek.

Sok metódusa a cRecord, és a belőle származtatott osztályoknak csak egy teljesen, vagy részben átlátszó interfész a
leíró osztályok, a cRecStaticDescr és a cColStaticDescr osztályok metódusai, ill. adattagjai felé. Ilyenek a tábla
vagy mező tulajdonságait visszaadó metódusok, vagy az ezekkel összefüggő adatszerkezetek előállítása. Ezek mindegyike
egy tisztán virtuális függvényen a descr() keresztül valósul meg, ami egy konstans referenciát ad vissza a
cRecStaticDescr leíró objektumra. Ezen metódusok:schemaName(), tableName(),
fullTableName(), fullTableNameQ(), columName(), columnNameQ(), fullColumName(), fullColumnNameQ(),
isIndex(), toIndex(), chkIndex(), cols(), columnDescrs(), primaryKey(), uniques(), autoIncrement(), idIndex(),
isName(), nameIndex(), nameName(), noteIndex(), deletedIndex(), isParent(), isUpdatable(),
tableoid(), isNullable(), mask(), iTab()

A cRecord objektum egy rekord adatait tartalmazhatja, a _fields adattagban, mely közvetlenül nem elérhető.
A _fields egy QVarinatList konténer, és a rekord mezőinek értékét mindíg a táblában definiált sorrendben tartalmazza.
Az egyes mező értékeket reprezentáló QVariant objektum a mező típusának megfelelően adott típusú adatot tartalmaz,
vagy „invalid”, ha a mező értéke NULL. A mező adatok kezelése a cColStaticDescr, és a belőle származtatott
objektumokon keresztül történik, melyet a cRecStaticDescr tartalmaz. A kezelt típusok a következők lehetnek:
- Egész számok\n
  SQL típusok: smallint, integer, bigint, serial, bigserial. Más egész típus nem támogatott.
  A tárolási típus mindig a qlonglong, az adatkonverziókat az  cColStaticDescr osztály végzi.
- Lebegőpontos számok\n
  SQL típusok: real, double precision. Más lebegőpontos típus nem támogatott.
  A tárolási típus mindig a double, az adatkonverziókat az  cColStaticDescr osztály végzi.
- Szöveges adat\n
  SQL típusok: character varying(n) , varchar(n), character(n), char(n), text
  A tárolási typus mindig a QString, az adatkonverziókat a cColStaticDescr osztály végzi.
- Tömbök\n
  A fentebb felsorol három típuscsoport tömbje támogatott. (az enumerációs tömböt az API nem tömbként kezeli, lásd később).
  A tárolási típus numerikus tömb esetén QVariantList , ahol a lista elemek típusa qlonglong ill. double, szöveges tömb esetén pedig QStringList.
  Tömbök esetén a konverziókat a cColStaticDescrArray osztály végzi.
- Enumerációk\n
  Az enumerációs típusnál tárolási típus a QString. A konverziókat pedig a cColStaticDescrEnum osztály végzi. Az enumeráció mind egész mind string
  formában használható. Ha egész számként adjuk meg, ill. egész számmá konvertáljuk, akkor az adatbázisbeli enumeráció neveknek a numerikus értéke
  a definícióban szereplő sorszámuk lesz: 0, 1, 2,... .
  Mivel az enumerációs adattípusokat nem tudjuk az SQL típusok alapján automatikusan létrehozni, ugyanígy a numerikus érték és string konverziókat
  sem, ezért az API tartalmaz egy ellenőrző mechanizmust, hogy a kétféleképpen leírt, de azonos jelentésű adatok valóban egyezzenek. Ehhez a
  string – numerikus érték közötti konverziós függvényeknek megadott prototípusa kell legyen, és ebben az esetben a két értelmezés szerinti értékeknek
  az egyezését a CHKENUM() makróval tudjuk ellenőrizni lásd az API dokumentációt.
- Set vagy enumerációs tömb\n
  A MySQL-ben létező set típust a PostgreSQL nem támogatja, de helyette használható az enumerációs tömb. A tárolási típus a QStringList, a konverziókat
  pedig a cColStaticDescrSet osztály végzi. Hasonlóan az enumerációs típushoz a set típus használható string ill. string lista, és numerikus formában.
  Ebben az esetben egy enumerációs érték és az értéknek a definícióbeli sorszáma a numerikus értékben a megfelelő sorszámú bitjével egyenértékű.
- Poligon\n
  SQL típus a polygon.
  A tárolási típus a tPolygonF, az adatkonverziókat az  cColStaticDescrPolygon osztály végzi.
  @note Mivel a QPolygonF a Qt GUI modul része, ezért, ha az applikációnk nem GUI-val fordítottuk, ill. a QCoreApplication -t példányosítottuk a
  main() -ben, akkor a QPolygonF használatakor elszállhat a programunk. Ezért az lv2 modul a QPolygonF osztály helyett mindíg a tPolygonF template
  osztályt használja, a két tíous lényegében ekvivalens, de egymásba konvertálásuk nem automatikus.

- IP cím\n
  SQL típus az inet.
  A tárolási típus a netAddress, az adatkonverziókat az  cColStaticDescrAddr osztály végzi. Értékadásnál használható még a QHostAddress típus is,
  mivel ezeket a típusokat közvetlenül a QVariant nem támogatja, az API regisztrálja a hiányzó típusokat, valamint a típuskonverziós lehetőségek is
  szűkebbek, ill. azok nem mindig autómatikusak, lásd a QVariant dokumentációját. Bár az SQL-ben az inet typus címtartomány tárolására is alkalmas,
  és az API sem tesz szigorú különbséget a cím és címtartományok között a cRecord szintjén, az inet típust mindig csak IP cím tárolására használjuk.
  IP címtartományok\n
  SQL típus az cidr.
  A tárolási típus a netAddress, az adatkonverziókat az  cColStaticDescrAddr osztály végzi. A cRecord osztály nem tesz különbséget a cím és címtartomány
  között.
- MAC cím\n
  SQL típus a macaddr.
  A tárolási típus a cMac, az adatkonverziókat az  cColStaticDescrAddr osztály végzi. A QVariant közvetlenül nem támogatja a cMac osztályt, a típust az
  API regisztrálja, lásd a QVariant dokumentációját is.
- Időpont (egy napon belül)
  SQL típus a time. Időzóna kezelés (egyelőre) nincs.
  A tárolási típus a QTime, az adatkonverziókat az  cColStaticDescrTime osztály végzi.
- Dátum\n
  SQL típus a date. Időzóna kezelés (egyelőre) nincs.
  A tárolási típus a QDate, az adatkonverziókat az  cColStaticDescrDate osztály végzi.
- Dátum és idő\n
  SQL típusok a timestamp. Időzóna kezelés (egyelőre) nincs.
  A tárolási típus a QTimeDate, az adatkonverziókat az  cColStaticDescrDateTime osztály végzi.
- Időintervallum\n
  SQL típusok az interval.
  A tárolási típus a qlonglong, az időintervallum ezredmásodpercben értendő, az adatkonverziókat az  cColStaticDescrInterval osztály végzi.
  Nem támogatja az összes formátumát a PostgreSQL-beli időintervallumnak, csak a nap, óra, perc, másodperc adható meg (a másodperc mint lebegőpontos
  érték).
- Logikai\n
  SQL típus a boolean.
  A tárolási típus a bool, az adatkonverziókat az  cColStaticDescrBool osztály végzi.
- Bináris adatok\n
  SQL típus a bytea.
  Tárolási típus a QByteArray, az adatkonverziókat az  cColStaticDescr osztály végzi.
Más mező adat típust nem támogat a cRecord osztály.

A mező adatokhoz a set() és get() metódusokkal ill. az operator[]() -on keresztül tudunk hozzáférni.
Van néhány speciális mező (nem kötelező, a rendszer ezeket csak akkor detektálja, ha azok megfelelnek a szabályoknak),
ilyen az ID, a név, megjegyzés (note) mezők, melyeket a típus és a mező név végződése azonosít, valamint a deleted és flag mezők.
Ezek némelyike esetén nem kell megadni a mező nevét vagy indexét: getId(), getName(), getNote(), setId(i), setName(n), SRB..
És van néhány metódus, ami elvégez helyettünk néhány gyakoribb adat konverziót (az első paraéter a mezőt azonosítja, név, vagy index alapján):
getId(const QString&), getName(const QString&), getBool(const QString&), getId(int), getName(int),
getBool(int), view(int), setId(const QString&,qlonglong), setName(const QString&, const QString&), setId(int,qlonglong), setName(int, const QString&) stb..

A set metódus csoport további tagjai:
- set()	NULL mezőkkel tölti fel az objektumot
- set(const cRecord &  	__o)	Ahol egy QSqlRecord objektumból tölti fel a mező adatokat.
- set(const QSqlQuery & __q, int * __fromp, int	__size)	Ahol egy QSqlQuery objektum a forrás, továbbá lehetőség van több tábla rekordját tartalmazó lekérdezést használni forrásként,
  és több objektumot célként (ismételt hívásával).

További adatmanipuláló metódus a clear(), ami kiüríti az egész mező adat konténert, és a clear(int) vagy clear(const QString&) ami egy mező
értékét állítja NULL ill. „invalid” értékre.

Egyszerű adatbázis műveletekre, adat beszúrására, módosítására használható metódusok: fetch(), rows(), completion(),
fetchById(), setById() fetchByName(), setByName(), fetchTableOid(), getIdByName(), getNameById(), insert(), update(),
remove(), delByName().
És az SQL string összeállítását, és az adatbázis műveleteket segítő metódusok a: query(),
first(), next(), getSetMap(), whereString().

Az osztály állapot lekérdező metódusai: checkData(), isNull(), isNull(c), _isNull(), _isNull(), isEmpty(), isEmpty(),
isEmpty_(), isIdent(), isModify_(), isNullId(), isNullName() ezen felül a _state adattag közvetlenül is elérhető.
Van egy állapot lekérdezés, ami csak a cRecordAny osztály esetén ad nem „hamis” eredményt, ez az isFaceless().

A cRecord leszármazott osztályainál járulékos adattagok kezelését segítik a következő virtuális metóduaok, melyek
a cRecord ősben üres függvényként vannak definiálva: toEnd(), toEnd(c), clearToEnd(). Ezen függvények
újraimplementálásával elvégezhetjük azokat a műveleteket, melyek az mező adatok megváltozásakor el kell végeznünk.
A népes cRecord objektum család tagjai közti konverziókat ill. ellenőrzéseket segítő metódusok a chkObjeType()
és a reconvert(). Egyes esetekben jelentősen leegyszerűsíti az objektumcsalád használatát pl. konténerekben két
további virtuális metódus, a newObj() és a dup(), ezeket alapesetben makrók segítségével definiálhatjuk. Az objektum klónozásakor,
nem teljesen azonos az így létrehozott objektum, mivel a QObject is ős, és ez a művelet a QObject esetén nem
támogatott, vagyis a QObject-hez tartozó tulajdonságok nem kerülnek az új objektumba.

Ha nyomkövetéshez ki kell írni egy objektum értékét, akkor a toString(), ha a leíró objektumok tartalmát is látni
akarjuk, akkor a toAllString() metódust használhatjuk, aminek az eredménye a kiírandó string. Az API nyomkövető
(cDebug) esetén csak az objetumot kell megadni, a kiírt string a toString() metódus által generált string lesz.

*/
class LV2SHARED_EXPORT cRecord : public QObject {
    template <class T> friend class tRecordList;
    friend class cRecordFieldConstRef;
    friend class cRecordFieldRef;
    friend class cLldpScan;
    Q_OBJECT
protected:
    /// Az objektum adattartalma, a mező értékek, megadott sorrendben, vagy üres.
    QVariantList    _fields;
    /// Csak egy segéd konstans, ha egy üres konstans QVariant-ra mutató referenciát kel visszaadnia egy metódusnak.
    static const QVariant   _vNul;
public:
    /* A 32 bites kompatibilitás miatt nem lehet enum, mert akkor az csak 32 bites lesz, és nem kényszeríthető ki a 64 bit.
    /// @enum eStat
    /// Rekord státusz konstansok
    typedef enum {
        ES_FACELESS     =     -2LL, ///< A rekord üres, és nincs típusa, ill nincs hozzárendelve adattáblához. Ez az állapot csak a cRecordAny típusú objektumokban lehetséges.
        ES_NULL         =     -1LL, ///< A rekord üres, a _fields adattagnak nincs egy eleme sem
        ES_EMPTY        = 0x0000LL, ///< A rekord üres, a _fields adattag feltöltve, de minden eleme null
        ES_EXIST        = 0x0001LL, ///< O.K. Az adatbázisban létező rekord
        ES_NONEXIST     = 0x0002LL, ///< O.K. Az adatbázisban nem létező rekord
        ES_COMPLETE     = 0x0004LL, ///< Minden kötelező mező feltöltve
        ES_INCOMPLETE   = 0x0008LL, ///< Hiányos
        ES_MODIFY       = 0x0010LL, ///< Módosítva
        ES_IDENTIF      = 0x0020LL, ///< A rendelkezésre álló mezők egyértelműen azonosíthatnak egy rekordot
        ES_UNIDENT      = 0x0040LL, ///< A rendelkezésre álló mezők nem feltétlenül azonosítanak egy rekordot
        ES_DEFECTIVE    = 0x0080LL, ///< A rekord, vagy leíró inkonzisztens, nem konvertálható érték megadása
        ES_INV_FLD_MSK  = 0x0FFFFFFFFFFF0000LL   ///< A hibás értékadásban résztvevő mező(ke)t azonosító bit(ek), bit_ix = mező_ix + 8
    } eStat; */
    typedef qlonglong sStat;    /// Az enum eStat helyetti definíció
    qlonglong _stat;  ///< A rekord állapota
    /// Enumeráció (bitek) a deleted mező kezeléséhez.
    enum eDeletedBehavior {
        NO_EFFECT        = 0,   ///< Nem kezeli a deleted mezőt
        FILTERED         = 1,   ///< Automatikus kiszűrése a deleted = true mezőknek, ha nincs hivatkozás a deleted mezőre.
        REPLACED_BY_NAME = 2,   ///< Inzert esetén, ha azonos nevű törölt mező van, akkor annak a felülírása.
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
    /// Visszatérés előtt a következő metódusok hívják: cRecord& set(int __i, const QVariant& __v)
    /// Az esetleges csak a mezőt érintő adatkonverziók a statikus leíró mező leíró objektumának a
    /// virtuális metódusain keresztül lehetséges.
    /// @return true, ha egy olyan mező indexét adtuk meg, ami az objektum hatáskörébe tartozik, és valamilyen tevékenység tartozik hozzá, akkor true,
    ///               ha nem akkor false
    virtual bool toEnd(int i);
    /// Lekérdezi egy mező értékét. A távoli kulcsokat megkisérli névvé konvertálni.
    /// Mindíg egy ember álltal értelmezhető értékkel (próbál) visszatérni.
    /// Ha megadjuk a pFeatures paramétert, akkor ha a konténerben létezik "view.func" kulcs, akkor annak az értéke
    /// egy SQL függvény kell legyen, melynek egy paraétere a mező érték, és ekkor a metódus a függvény visszatérési értékét adja vissza.
    /// Ha megadjuk a pFeatures paramétert, akkor ha a konténerben létezik "view.expr" kulcs, akkor annak az értéke
    /// egy SQL kifelyezés kell legyen, melynek egy paraétere a mező érték, és ekkor a metódus a kifelyezés értékét adja vissza.
    /// @param q Ha adatbázisműveletre van szükség, akkor az ezen keresztül.
    /// @param __i A mező indexe
    /// @param pFeatures Egy opcionális features érték.
    virtual QString view(QSqlQuery &q, int __i, const cFeatures *pFeatures = nullptr) const;
    ///
    virtual bool isContainerValid(qlonglong __mask) const;
    virtual void setContainerValid(qlonglong __set, qlonglong __clr = 0);
    /// Leellenőrzi, lehet-e mező (oszlop) index a paraméter.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param A viszgálandó index érték
    /// @return Ha a paraméter értéke az aktuálisan lehetséges index értéktartományon belül van, akkor true.
    bool isIndex(int i) const                           { return descr().isIndex(i); }
    /// Leellenőrzi, lehet-e mező (oszlop) index a paraméter.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @exception cError* Ha a paraméter értéke az aktuálisan lehetséges index értéktartományon kívül van dob egy kizárást.
    /// @param A viszgálandó index érték
    /// @return Az index értékkel tér vissza, ha nem index, akkor NULL_IX-el
    int chkIndex(int i, eEx __ex = EX_ERROR) const   { descr().chkIndex(i, __ex); return i; }
    /// A megadott mező névhez tartozó mező indexel tér vissza.
    /// Ha a mező név tartalmazza a séma nevet vagy a tábla nevet, akkor azokat ellenőrzi,
    /// és ha nem erre a táblára vonatkoznak, akkor -1-el tér vissza. A nevek kettős idézőjelbe tehetőek (egyenként!)
    /// @return a mező indexe, vagy -1, ha nincs ilyen mező, vagy a megadott tábla ill. séma név nem ezé a tábláé.
    /// @exception Ha __ex értéke true, és -1 (vagyis nem egy mező index) lenne a visszaadott érték, akkor dob egy kizárást.
    int toIndex(const QString& __n, enum eEx __ex = EX_ERROR) const { return descr().toIndex(__n, __ex); }
    /// Leellenörzi, lehet-e mező (oszlop) név a paraméter.
    /// @param A vizsgálandó név
    /// @return Ha a paraméter értéke lehet mező név, akkor true.
    bool isIndex(const QString& n) const            { return isIndex(toIndex(n, EX_IGNORE)); }
    /// A séma nevével tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QString& schemaName() const               { return descr().schemaName(); }
    /// A tábla nevével (nem teljes név, nem tartalmazza a séma nevet) tér vissza
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
    /// A teljes séma névvel kiegészített tábla névvel tér vissza, a nevek nincsenek idézőjelben.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    QString fullTableName() const                  { return descr().fullTableName(); }
    /// A teljes (séma és tábla névvel kiegészített) mezőnévvel tér vissza, a nevek nincsenek idézőjelben.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param _c A kiegészítendő mező (oszlop) neve
    QString fullColumnName(const QString& _c) const{ return descr().fullColumnName(_c); }
    /// A teljes séma névvel kiegészített tábla névvel tér vissza, a nevek idézőjelek közé vannak zárva.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívhatóvoid t.
    QString fullTableNameQ() const                   { return descr().fullTableNameQ(); }
    /// A teljes (séma és tábla névvel kiegészített) mezőnévvel tér vissza, a nevek idézőjelek közé vannak zárva.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param i A kiegészítendő mező (oszlop) indexe
    /// @exception cError* Ha i értéke nem lehet index, akkor dob egy kizárást
    QString fullColumnNameQ(int i) const             { return descr().fullColumnNameQ(i); }
    /// A teljes (séma és tábla névvel kiegészített) mezőnévvel tér vissza, a nevek idézőjelek közé vannak zárva.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    /// @param _c A kiegészítendő mező (oszlop) neve
    QString fullColumnNameQ(const QString& _c) const { return descr().fullColumnNameQ(_c); }
    ///
    QStringList columnNames(QBitArray mask) const { return descr().columnNames(mask); }
    QStringList columnNames(tIntVector ixs) const { return descr().columnNames(ixs); }
    /// A tábla típusával tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    quint16 tableType() const                       { return descr().tableType(); }
    /// Az mezők (oszlopok) teljes számával tér vissza
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    int cols() const                                { return descr().cols(); }
    /// Az elsődleges kulcs mező(k) maszkjával tér vissza.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QBitArray& primaryKey() const             { return descr().primaryKey(); }
    /// Az egyedi kulcs(ok) mező(k) maszk vektorával tér vissza.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QList<QBitArray>& uniques() const         { return descr().uniques(); }
    /// Egy bitmaszkkal tér vissza, mely a név mezőt, továbbá azon mezőket tartalmazza, mellyel az egyedi kulccsá egészül ki.
    /// Ha a név egyedi kulcs, akkor csak a név mező indexével azonos indexű bit egyes. Ha nem csak egy olyan mezőkombináció
    /// van ami trtalmazza a név mezőt, és egyedi kulcs, akkor dob egy kizárást, szintén kizárást dob, ha nincs név mező,
    /// vagy az egyetlen egyedi kulcsban sem szerepel.
    const QBitArray& nameKeyMask(eEx __ex = EX_ERROR) const
    {
        if (descr()._nameKeyMask.count(true) == 0 && __ex != EX_IGNORE)
            EXCEPTION(EDATA, -1 , QObject::trUtf8("_nameKeyMask is empty."));
        return descr()._nameKeyMask;
    }
    /// Egy bit vektorral tér vissza, ahol minden bit true, mellyel azonos indexű mező autoincrement.
    /// Tisztán virtuális függvényt hív, konstruktorból nem hívható.
    const QBitArray& autoIncrement() const          { return descr().autoIncrement(); }
    /// Az ID mező indexével tér vissza, ha nincs ID, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int idIndex(enum eEx __ex = EX_ERROR) const     { return descr().idIndex(__ex); }
    /// Az NAME mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int nameIndex(enum eEx __ex = EX_ERROR) const   { return descr().nameIndex(__ex); }
    /// Az NOTE mező indexével tér vissza, ha nincs név, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int noteIndex(enum eEx __ex = EX_ERROR) const   { return descr().noteIndex(__ex); }
    /// Az deleted mező indexével tér vissza, ha nincs deleted, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int deletedIndex(enum eEx __ex = EX_ERROR) const{ return descr().deletedIndex(__ex); }
    /// Az 'flag' mező indexével tér vissza, ha nincs flag, vagy nem ismert az indexe, és __ex értéke true, akkor dob egy kizárást.
    int flagIndex(enum eEx __ex = EX_ERROR) const   { return descr().flagIndex(__ex); }
    /// Ha bármelyik mező módosítható az adatbázisban, akkor true-val tér vissza.
    bool isUpdatable() const                    { return descr()._isUpdatable; }
    /// Ha a megadott indexű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen indexű mező, dob egy kizárást.
    bool isUpdatable(int i) const               { return descr().colDescr(i).isUpdatable; }
    /// Ha a megadott nevű mező módosítható az adatbázisban, akkor true-val tér vissza.
    /// Ha nincs ilyen nevű mező, dob egy kizárást.
    bool isUpdatable(const QString& fn) const   { return descr().colDescr(toIndex(fn)).isUpdatable; }
    /// Törli az objektumot (fields konténert). Majd meghívja a clearToEnd() virtuális metódust.
    cRecord& clear();
    /// Törli a megadott indexű mezőt az objektumban. És meghívja a toEnd(int i) virtuális metódust.
    /// @param __ix A törlendő mező indexe.
    /// @param __ex Ha értéke true, és nem létezik a megadott indexű mező, akkor dob egy kizárást, ha értéke false, és nem létezik a mező, akkor nem csinál semmit.
    cRecord& clear(int __ix, enum eEx __ex = EX_ERROR);
    /// Törli a megadott nevű mezőt az objektumban. És meghívja a toEnd(int i) virtuális metódust, ahol i a megnevezett mező indexe.
    /// @param __n A törlendő mező neve.
    /// @param __ex Ha értéke true, és nem létezik a megadott mező, akkor dob egy kizárást, ha értéke false, és nem létezik a mező, akkor nem csinál semmit.
    cRecord& clear(const QString& __n, enum eEx __ex = EX_ERROR) { return clear(toIndex(__n, __ex), __ex); }
    /// Minden olyan mezőre hívja a clear(int) metódust, mellyel azonos idexű bit igaz az __m bitmap-ban.
    /// Ha __ex értéke igaz, és az __m több elemet tartalmaz, mint az objektum mezőt, akkor dob egy kizárást, egyébként figyelmenkívül hagyja az
    /// extra biteket.
    cRecord& clear(const QBitArray& __m, enum eEx __ex = EX_ERROR);
    /// A felsorolt nevű mezőkre hívja a clear(QString) metódust. Ha __ex igaz, és a listában olyan név van, ami nem mezőnév, dob egy kizárást,
    /// ha hamis akkor a nevet figyelmenkívöl hagyja.
    cRecord& clear(const QStringList& __fl, enum eEx __ex = EX_ERROR) { return clear(mask(__fl, __ex)); }
    ///
    cRecord& clearId()      { return clear(idIndex()); }
    cRecord& clearName()    { return clear(nameIndex()); }
    cRecord& clearNote()    { return clear(noteIndex()); }
    /// Ha nincs rekord deszkriptor a status szerint, akkor true-val tér vissza.
    /// Csak a cRecordAny leszármazott osztály esetén lehet hamis. A leírót statikus adattagként tartalmazó leszármazottak esetén mindig false a
    /// visszadott érták.
    bool isFaceless() const { return _stat == ES_FACELESS; }
    /// Ha a mező adat lista (_fields) üres, akkor true, egyébként false. Nem a stat-ot vizsgálja, és annak értékét nem módosítja.
    bool isNull() const                             { return _fields.isEmpty();  }
    /// Azonos az isNull() metódussal, mivel nem hív virtuális metódust, ezen a néven újra lett definiálva.
    bool _isNull() const                            { return _fields.isEmpty();  }
    /// Ha isNull() visszaadott értéke true, vagy minden mező NULL, akkor true-val tér vissza, utóbbi állapot kiderítésére csak a _stat értékét vizsgálja.
    bool isEmpty_() const                           { return _stat == ES_EMPTY || isNull(); }
    /// Ha a _stat adattag szerint módosítva lett az objektum, akkor true-val tér vissza.
    bool isModify_() const                          { return (_stat & ES_MODIFY) != 0; }
    /// Ha a _stat adattag szerint adat hiba történt
    bool isDefective() const                        { return (_stat & ES_DEFECTIVE) != 0; }
    /// Egy listát ad vissza, azon mezőnevek listáját, melyeknél a _stat-ban a megfelelő bit be van állítva.
    /// Ha a _stat -ban az ES_DEFECTIVE bit nincs beállítva, akkor üres listát ad vissza.
    /// Ha az ES_DEFECTIVE nincs beállítva, de nincs megjelülve a _stat -ban egy mező sem, akkor egy elemú listát ad vissza,
    /// ahol az egy elem értéke "[unknown]".
    QStringList defectiveFields() const;
    /// Létrehozza az összes mezőt sorrendben null tartalommal. Előtte törli az objektumot
    /// Tisztán virtuális metódust hív, konstruktorból nem hívható.
    cRecord& set();
    /// Létrehozza az összes mezőt sorrendben az __r-ben lévő tartalommal.
    /// Előtte törli az objektumot. Ha a lekérdezésben, ill. megadott részében olyan mezőnév
    /// van, ami a rekordban nincs, akkor azt az értéket figyelmenkívül hagyja.
    /// Tisztán virtuális metódust hív, konstruktorból nem hívható.
    /// @param __r Forrás rekord objektum. A mezők név szerint lesznek azonosítva.
    /// @param __fromp Ha nem NULL, akkor a *__fromp indextől végzi a feltöltést, visszatéréskor az első nem olvasott mező indexe
    /// @param __size Ha értéke pozitív, akkor össz. ennyi mezőt fog olvasni, ha negatív, vagy nincs megadva, akkor az objektum mező száma.
    cRecord& set(const QSqlRecord& __r, int* __fromp = nullptr, int __size = -1);
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
    cRecord& set(const QSqlQuery& __q, int * __fromp = nullptr, int __size = -1)   { return set(__q.record(), __fromp, __size);   }
    /// A megadott indexű mező értékének a lekérdezése. Az indexet ellenőrzi, ha nem megfelelő dob egy kizárást
    /// A mező értékére egy referenciát ad vissza, ez a referencia csak addig valós, amíg nem hajtunk végre
    /// az objektumon egy olyan metódust, amely ujra kreálja a _fields adat konténert. Azon értékadó műveletek, melyek
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
    /// Az név (name) mező értékével tér vissza, ha van.
    /// Ha nincs név mező megkísérli a rekord ID-ből az id2name SQL föggvénnyel konvertálni a nevet.
    /// Ha a nibcs név mező, és nincs ID mező, vagy konvertáló függvény akkor kizárást dob.
    /// Ha nincs név és az ID mező értéke NULL, akkor egy NULL stringgel tér vissza.
    QString getName() const;
    /// Feltételezve, hogy a megadott indexű mező egy MAC, annk értékével tér vissza.
    cMac    getMac(int __i, enum eEx __ex = EX_ERROR) const;
    /// Feltételezve, hogy a megadott nevű mező egy MAC, annk értékével tér vissza.
    cMac    getMac(const QString& __n, enum eEx __ex = EX_ERROR) const { return getMac(toIndex(__n, __ex), __ex); }
    /// Az megjegyzés/cím mező értékével tér vissza, ha van, egyébként dob egy kizárást
    QString getNote() const                        { return getName(noteIndex()); }
    /// Ha a megjegyzés/cím mező nem NULL, akkor azzal, egyébként a névvel tér vissza.
    QString getNoteOrName() const                  { QString s = getNote(); return s.isEmpty() ? getName() : s; }
    /// Ha az n1 nevű mező nem NULL, akkor azzal, egyébként az n2 nevű mező értékével tér vissza.
    QString getName2(const QString& n1, const QString& n2) const { QString s = getName(n1); return s.isEmpty() ? getName(n2) : s; }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy logikai érték, ill. annak értékét bool típusúvá konvertálja.
    bool getBool(int __i) const                     { return get(__i).toBool(); }
    /// Egy mező értékével tér vissza, feltételezve, hogy az egy logikai érték, ill. annak értékét bool típusúvá konvertálja.
    bool getBool(const QString& __i) const          { return get(__i).toBool(); }
    /// Feltételezi, hogy egy set típusú mezőről van szó ..,
    bool getBool(int __i, int __e) const            { return (getBigInt(__i) & enum2set(__e)) != 0; }
    /// Feltételezi, hogy egy set típusú mezőről van szó ..,
    bool getBool(const QString& __i, int __e) const { return (getBigInt(__i) & enum2set(__e)) != 0; }
    /// Lekérdezi egy mező értékét. Lsd. a másik view(int __i) metódust.
    QString view(QSqlQuery& q, const QString& __i) const    { return view(q, toIndex(__i, EX_IGNORE)); }
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
    cRecord& setName(int __i, const QString& __n)   {
        if (__n.isNull())
            clear(__i);
        else
            set(__i, QVariant(__n));
        return *this; }
    /// Az megadott mező értékét állítja be, feltételezve, hogy az egy string, ill. az új érték egy string lessz.
    /// @param __i A mező neve
    /// @param __n Az mező új értéke, ha null stringet adunk át, akkor NULL lessz.
    cRecord& setName(const QString& __i, const QString& __n)    { return setName(toIndex(__i), __n); }
    /// Az név (name) mező értékét állítja be
    /// @param __n Az név új értéke, ha null stringet adunk át, akkor NULL lessz.
    cRecord& setName(const QString& __n)            { return setName(nameIndex(), __n); }
    /// Az megjegyzés (note) mező értékét állítja be
    /// @param __n Az note mező új értéke, ha null stringet adunk át, akkor NULL lessz.
    cRecord& setNote(const QString& __n)            { return setName(noteIndex(), __n); }
    ///
    cRecord& setBool(int __i, bool __f)             { return set(__i, QVariant(__f)); }
    cRecord& setBool(const QString& __n, bool __f)  { return set(__n, QVariant(__f)); }
    cRecord& setFlag(bool __f = true)               { return setBool(descr()._flagIndex, __f); }
    cRecord& setOn(int __i)                         { return setBool(__i, true); }
    cRecord& setOn(const QString& __n)              { return setBool(__n, true); }
    cRecord& setOff(int __i)                        { return setBool(__i, false); }
    cRecord& setOff(const QString& __n)             { return setBool(__n, false); }
    ///
    cRecord& setMac(int __i, const cMac& __a, enum eEx __ex = EX_ERROR);
    cRecord& setMac(const QString& __n, const cMac& __a, enum eEx __ex = EX_ERROR)   { return setMac(toIndex(__n), __a, __ex); }
    /// \brief setIp
    /// Feltételezve, hogy a mező típusa IP cím, beállítja a mező értékét.
    /// \param __i  A mező indexe
    /// \param __a  A neállítandó IP cím
    /// \param __ex Ha értéke true (ez az alapértelmezés) akkor hiba esetén dob egy kizárást.
    /// \return Az obkeltum referenciával tér vissza
    cRecord& setIp(int __i, const QHostAddress& __a, enum eEx __ex = EX_ERROR);
    /// \brief setIp
    /// Feltételezve, hogy a mező típusa IP cím, beállítja a mező értékét.
    /// \param __n  A mező neve
    /// \param __a  A neállítandó IP cím
    /// \param __ex Ha értéke true (ez az alapértelmezés) akkor hiba esetén dob egy kizárást.
    /// \return Az obkeltum referenciával tér vissza
    cRecord& setIp(const QString& __n, const QHostAddress& __a, enum eEx __ex = EX_ERROR)   { return setIp(toIndex(__n), __a, __ex); }
    /// \brief getBigInt
    ///  Hasonló a getId-hez, de ha a mező érték NULL, akkor nem a NULL_ID-vel, hanem 0-val tér vissza.
    /// \param __ix A mező indexe
    /// \return A mező numerikus értéke, NULL esetén 0LL.
    qlonglong getBigInt(int __ix) const {
        if (isNull(__ix)) return 0LL;
        return getId(__ix);
    }
    /// \brief getBigInt
    ///  Hasonló a getId-hez, de ha a mező érték NULL, akkor nem a NULL_ID-vel, hanem 0-val tér vissza.
    /// \param __n A mező neve
    /// \return A mező numerikus értéke, NULL esetén 0LL.
    qlonglong getBigInt(const QString& __n) const { return getBigInt(toIndex(__n)); }
    /// \brief setOn
    /// Set típusú mezőben (enumerációs tömb) maszkkal megadott értékek beállítása ill. hozzáadása.
    /// \param __ix A mező indexe
    /// \param __set A SET-ben beállítandó érték maszk
    /// \return Az objektum referenciával tér vissza.
    cRecord& setOn(int __ix, qlonglong __set) { return setId(__ix, getBigInt(__ix) | __set); }
    /// \brief setOn
    /// Set típusú mezőben (enumerációs tömb) maszkkal megadott értékek beállítása ill. hozzáadása.
    /// \param __n A mező neve
    /// \param __set A SET-ben beállítandó érték maszk
    /// \return Az objektum referenciával tér vissza.
    cRecord& setOn(const QString& __n, qlonglong __set) { return setOn(toIndex(__n), __set); }
    /// \brief enum2setOn
    /// Set típusú mezőben (enumerációs tömb) egy enumerációs értéke beállítása/hozzáadása, az enumeráció numerikus értékének a megadásával.
    /// \param __ix A mező indexe
    /// \param __e A hozzáadott enumerációs érték numerikus megfleelője ( a set numerikus értékében a bit indexe)
    /// \return Az objektum referenciával tér vissza.
    cRecord& enum2setOn(int __ix, int __e)                      { setId(__ix, getBigInt(__ix) | enum2set(__e)); return *this; }
    /// \brief enum2setOn
    /// Set típusú mezőben (enumerációs tömb) két enumerációs értéke beállítása/hozzáadása, az enumeráció numerikus értékének a megadásával.
    /// \param __ix A mező indexe
    /// \param _e1 Az egyik hozzáadott enumerációs érték numerikus megfleelője
    /// \param _e2 A másik hozzáadott enumerációs érték numerikus megfleelője
    /// \return Az objektum referenciával tér vissza.
    cRecord& enum2setOn(int __ix, int _e1, int _e2)             { setId(__ix, getBigInt(__ix) | enum2set(_e1, _e2)); return *this; }
    cRecord& enum2setOn(const QString& __n, int __e)            { enum2setOn(toIndex(__n), __e); return *this; }
    cRecord& enum2setOn(const QString& __n, int _e1, int _e2)   { enum2setOn(toIndex(__n), _e1, _e2); return *this; }
    /// \brief setOff
    /// Set típusú mezőben (enumerációs tömb) maszkkal megadott értékek törlése.
    /// \param __ix A mező indexe
    /// \param __set A SET-ben törlendő érték maszk
    /// \return Az objektum referenciával tér vissza.
    cRecord& setOff(int __ix, qlonglong __set) { return setId(__ix, getBigInt(__ix) & ~__set); }
    /// \brief setOff
    /// Set típusú mezőben (enumerációs tömb) maszkkal megadott értékek törlése.
    /// \param __n A mező neve
    /// \param __set A SET-ben törlendő érték maszk
    /// \return Az objektum referenciával tér vissza.
    cRecord& setOff(const QString& __n, qlonglong __set) { return setOff(toIndex(__n), __set); }
    ///
    cRecord& enum2setOff(int __ix, int __e)                     { setId(__ix, getBigInt(__ix) & ~enum2set(__e)); return *this; }
    cRecord& enum2setOff(int __ix, int _e1, int _e2)            { setId(__ix, getBigInt(__ix) & ~enum2set(_e1, _e2)); return *this; }
    cRecord& enum2setOff(const QString& __n, int __e)           { enum2setOff(toIndex(__n), __e); return *this; }
    cRecord& enum2setOff(const QString& __n, int _e1, int _e2)  { enum2setOff(toIndex(__n), _e1, _e2); return *this; }
    ///
    cRecord& enum2setBool(int __ix, int __e, bool __f)          { if (__f) enum2setOn(__ix, __e); else enum2setOff(__ix,__e); return *this; }
    cRecord& enum2setBool(const QString& __n, int __e, bool __f){ enum2setBool(toIndex(__n), __e, __f); return *this; }

    ///
    QStringList getStringList(int __i, enum eEx __ex = EX_ERROR) const;
    cRecord& setStringList(int __i, const QStringList& __v, enum eEx __ex = EX_ERROR);
    cRecord& addStringList(int __i, const QStringList& __v, enum eEx __ex = EX_ERROR);
    QStringList getStringList(const QString& __fn, enum eEx __ex = EX_ERROR) const {
        return getStringList(toIndex(__fn, __ex), __ex);
    }
    cRecord& setStringList(const QString& __fn, const QStringList& __v, enum eEx __ex = EX_ERROR) {
        return setStringList(toIndex(__fn, __ex), __v, __ex);
    }
    cRecord& addStringList(const QString& __fn, const QStringList& __v, enum eEx __ex = EX_ERROR) {
        return addStringList(toIndex(__fn, __ex), __v, __ex);
    }
    cRecord& addStringList(int __i, const QString& __v, enum eEx __ex = EX_ERROR) {
        QStringList l;
        l << __v;
        return addStringList(__i, l, __ex);
    }
    cRecord& addStringList(const QString& __fn, const QString& __v, enum eEx __ex = EX_ERROR) {
        return addStringList(toIndex(__fn, __ex), __v, __ex);
    }

    /// Az ID mezőket hasonlítja össze. Ha nincs vagy nem ismert az ID mező, akkor dob egy kizárást.
    /// Nem ellenőrzi, hogy a két objektum valójában milyen típusú, akkor is elvégzi az összehasonlítást, ha ez logikailag értelmetlen.
    /// Ha az ID értéke NULL, akkor az kisebb minden lehetséges ID értéknél.
    /// @return ha az objektum id-je kissebb, mint az operátor jobb oldalán elhelyezkedő.
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
    /// Az isNull(int i) változatlan újradefiniálása, a név konvenció miatt.
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
    /// Visszaad egy bitmap-et, ahol minden olyan bit igaz, mellyel azonos idexű mező értéke NULL.
    QBitArray areNull() const;
    /// Beszúr egy rekordot a megfelelő adattáblába. Az inzert utasításban azok a mezők
    /// lesznek megadva, melyeknek nem NULL az értékük.
    /// Ha sikeres volt az inzert, akkor az objektumot újra tölti, az adatbázisban keletkezett új rekord alapján.
    /// A rekord kiírását, és visszaolvasását egy SQL paranccsal valósítja meg "INSERT ... RETURNING *"
    /// Ha van név és deleted mező, és létezik egy azonos nevű de deleted = true rekord, akkor azt az
    /// update metódussal fellülírja. (teszelni kéne, hogy ez így valóban jól müködik-e)
    /// A saveText() metódus nem kerül meghívásra!
    /// @param __q Az inzert utasítás ezel az objektummal lesz kiadva
    /// @param __ex Ha értéke true (ez az alapértelmezés), akkor kizárást dob, ha adat hiba van, ill. nem történt meg az insert
    /// @exception cError* Hiba esetén dob egy kizárást, ha _ex értéke true ...
    /// @return ha történt változás az adatbázisban, akkor true.
    virtual bool insert(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    /// Hasonló az insert() metódushoz. Ha az insert metódus kizárást dobott, akkor a hiba objektum pointerével tér vissza.
    /// Ha rendben megtörtépnt a művelet, akkor NULL pointerrel.
    /// Manti a nyelvi szövegeket is ha vannak éa a text paraméter true (saveText() metódust is hívja)
    cError *tryInsert(QSqlQuery& __q, eTristate __tr = TS_NULL, bool text = false);
    /// Fellülír egy létező rekordot. A rekord azonosítása a nameKeyMask() alapján. A rekordot visszaolvassa.
    /// Ha a felülírás sikertelen, (nincs érintett rekord) és __ex értéke true, akkor dob egy kizárást.
    /// A nyelvi szövegeket nem modosítja! Nem hívja a saveText() metódust.
    virtual bool rewrite(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    /// Fellülír egy létező rekordot. A rekord azonosítása a nameKeyMask() alapján. A rekordot visszaolvassa.
    /// Ha rendben megtörtépnt a művelet, akkor NULL pointerrel, egyébként a hiba objektum pointerével tér vissza.
    /// Ha text értéke true, akkor menti a nyelvi szövegeket, ha text_id nem változott.
    cError *tryRewrite(QSqlQuery& __q, eTristate __tr = TS_NULL, bool text = false);

    /// Sablon metódus, egy járulékos tábla tartozik a rekordhoz, ami az objektum tulajdona
    /// @param __ch Gyerek objektum konténer
    /// @param __m  eContainerValid maszk, vagy 0, ha nincs feltétele a végrehajtásnak.
    template<class L> bool tRewrite(QSqlQuery& __q, L& __ch, qlonglong __m, enum eEx __ex = EX_ERROR)
    {
        bool r = cRecord::rewrite(__q, __ex);   // Kiírjuk magát a rekordot
        if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
        if (__m == 0 || isContainerValid(__m)) {
            __ch.setsOwnerId();
            r = __ch.replace(__q, __ex);
        }
        return r;
    }
    /// Sablon metódus, ha két járulékos tábla tartozik a rekordhoz, ami az objektum tulajdona
    template<class L1, class L2> bool tRewrite(QSqlQuery& __q, L1& __ch1, qlonglong __m1, L2& __ch2, qlonglong __m2, enum eEx __ex = EX_ERROR)
    {
        bool r = cRecord::rewrite(__q, __ex);   // Kiírjuk magát a rekordot
        if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
        if (__m1 == 0 || isContainerValid(__m1)) {
            __ch1.setsOwnerId();
            r =__ch1.replace(__q, __ex);
            if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
        }
        if (__m2 == 0 || isContainerValid(__m2)) {
            __ch2.setsOwnerId();
            r = __ch2.replace(__q, __ex);
        }
        return r;
    }
    /// Egy objektum újraírása (frissítése) az ID alapján.
    virtual bool rewriteById(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    /// Lényegében azonos a rewriteById() metódussal, de hiba esetén a hiba objektummal tér vissza (elkapja a hibát).
    cError *tryRewriteById(QSqlQuery& __q, eTristate __tr = TS_NULL, bool text = false);
    /// Sablon metódus, egy járulékos tábla tartozik a rekordhoz, ami az objektum tulajdona
    /// @param __ch Gyerek objektum konténer
    /// @param __m  eContainerValid maszk, vagy 0, ha nincs feltétele a végrehajtásnak.
    template<class L> bool tRewriteById(QSqlQuery& __q, L& __ch, qlonglong __m, enum eEx __ex = EX_ERROR)
    {
        bool r = cRecord::rewriteById(__q, __ex);   // Kiírjuk magát a rekordot
        if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
        if (__m == 0 || isContainerValid(__m)) {
            __ch.setsOwnerId();
            r = __ch.replace(__q, __ex);
        }
        return r;
    }
    /// Sablon metódus, ha két járulékos tábla tartozik a rekordhoz, ami az objektum tulajdona
    template<class L1, class L2> bool tRewriteById(QSqlQuery& __q, L1& __ch1, qlonglong __m1, L2& __ch2, qlonglong __m2, enum eEx __ex = EX_ERROR)
    {
        bool r = cRecord::rewriteById(__q, __ex);   // Kiírjuk magát a rekordot
        if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
        if (__m1 == 0 || isContainerValid(__m1)) {
            __ch1.setsOwnerId();
            r =__ch1.replace(__q, __ex);
            if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
        }
        if (__m2 == 0 || isContainerValid(__m2)) {
            __ch2.setsOwnerId();
            r = __ch2.replace(__q, __ex);
        }
        return r;
    }
    /// Beszúr vagy fellülír egy rekordot a megfelelő adattáblába. Az insert utasításban azok a mezők
    /// lesznek megadva, melyeknek nem NULL az értékük. Feelülírásnál a NULL értékú mezőknél ha van az
    /// alapértelmezett érték lesz kiírva, ha nincs akkor a NULL.
    /// Ha sikeres volt a művelet, akkor az objektumot újra tölti, az adatbázisban keletkezett/módosított rekord alapján.
    /// A rekord kiírását, és visszaolvasását egy SQL paranccsal valósítja meg : "... RETURNING *"
    /// Létező rekord azonosítása mindig a nameKeyMask() értéke alapján történik. Ha a mask üres, kizárást dob.
    /// @param __q Az inzert/update utasítás ezel az objektummal lesz kiadva
    /// @param __ex Ha értéke true (ez az alapértelmezés), akkor kizárást dob, ha adat hiba van, ill. nem történt meg az insert/update
    /// @exception cError* Hiba esetén dob egy kizárást, ha _ex értéke true
    /// @return Egy eReasons érték: R_ERROR, R_INSERT, R_UPDATE, esetleg (nem minden objektum esetén detektált) R_FOUND
    virtual int replace(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    /// Hasonló az replace() metódushoz. Ha a replace metódus kizárást dobott, akkor a hiba objektum pointerével tér vissza.
    /// Ha rendben megtörtépnt a művelet, akkor NULL pointerrel.
    cError *tryReplace(QSqlQuery& __q, eTristate __tr = TS_NULL, bool text = false);
    /// Egy WHERE stringet állít össze a következőképpen.
    /// A feltételben azok a mezők fognak szerepelni, melyek indexének megfelelő bit az __fm tömbben igaz.
    /// A feltétel, ha a mező NULL, akkor \<mező név\> IS NULL, ha nem NULL, akkor ha isLike() a mező indexére igaz,
    /// akkor \<mező név\> LIKE ?, ha hamis, akkor \<mező név\> = ? lessz.
    /// Ezután ha _deletedBehavior adattagban a FILTERED bit igaz, és van deleted mező, és az nem szerepelt az alöbbi mezők között,
    /// akkor kiegészíti a feltételeket a deleted = FALSE feltétellel.
    /// A feltételek közé az AND operátort teszi. És a visszaadott string, ha volt feltétel, akkor a ' WHERE ' stringgel fog kezdődni, egyébként
    /// üres stringgel tér vissza.
    /// A _likeMask alapértelmezése üres (ami alapján az isLike() visszatér), ha értéket adunk neki a feltétel összeállítás megváltoztatásához, akkor
    /// ne felejtsük el a lekérdezés után törölni azt, mert a késöbbi lekérdezések viselkedését is befolyásolni fogja.
    /// @param __fm Feltétel maszk. Ha üres tömb, akkor a használt maszk az elsődleges kulcsok maszkja lessz. Ha azt akarjuk, a feltétel
    ///     legyen üres (kivéve a deleted mező), akkor a paraméter egy nem üres, de egy true értéket sem tartalmazó tömb kell legyen.
    QString whereString(QBitArray& __fm) const;
    /// Végrehajt egy query-t a megadott sql query stringgel, és az objektum mezőivel (bind) az __arg-ban megadott sorrendben.
    /// A mező adatok bind-elése a mezők sorrendje szerint történik, attol nem lehet eltérni, egy mező egyszer szerepelhet.
    /// @param __q A QSqlQuery objektum, amin keresztül a lekérdezést a metódus elvégzi
    /// @param sql A SQL query string
    /// @param __arg A vektor elemei a mező indexei, amit bind-elni kell a lekérdezéshez.
    /// @exception Ha a QSqlQuery::prepare() vagy exec() metódus sikertelen, akkor dob egy kizárást.
    void query(QSqlQuery& __q, const QString& sql, const tIntVector& __arg) const;
    /// Végrehajt egy query-t a megadott sql query stringgel, és az objektum mezőivel ...
    /// A mező adatok bind-elése a magadott sorrendben szerint történik, egy mező többször is szerepelhet.
    /// @param __q A QSqlQuery objektum, amin keresztül a lekérdezést a metódus elvégzi
    /// @param sql A SQL query string
    /// @param __fm Bit vektor, ha a vektor elem true, akkor az azonos indexű mezőt kell bind-olni
    /// @exception Ha a QSqlQuery::prepare() vagy exec() metódus sikertelen, akkor dob egy kizárást.
    void query(QSqlQuery& __q, const QString& sql, const QBitArray& __fm) const;
    /// Összeálít egy lekérdezést a beállított, ill. a megadott adatok alapján. A leérdeklést végrehajtja,
    /// Az eredmény a a paraméterként megadott QSqlQuery objektumban. Az SQL parancs végrehajtása után
    /// hívja a __q.first() metódust, és azzal tér vissza.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __only Ha megadjuk és értéke true, akkor a származtatott táblákban nem keres.
    /// @param __fn A mező(k) maszk, alapértelmezése üres, ekkor a használt maszk az elsődleges kulcs mező(k). Ez alapján lessz
    ///             összeállítva a WHERE string a whereString() metódussal, kivéve, ha __w nem üres string.
    /// @param __ord Mely mezők szerint legyenek rendezve a találatok ( a vektor elemei mező indexek, negatív érték csökkenő sorrendet jelent, a 0. elem nem rendezhető csökkenő sorrendbe)
    /// @param __lim Limit. Ha értéke 0, akkor nincs.
    /// @param __off Offset. Ha értéke 0, akkor nincs.
    /// @param __s A SELECT kulcs szó utáni lista. Ha nincs megadva (vagy NULL), akkor az alapértelmezés a "*"
    /// @param __w A WHERE kulcs szó utáni feltétel, Feltéve, hogy nem üres string. Ebben az esetben a feltételre nincs hatással
    ///            az __fn paraméter, de ez alapján lessznek bind-elve a mező értékek. Tehát a megadott feltételnek annyi paraméter
    ///            kell várnia, ahány igaz bit van az __fn-ben, és a mező értékek sorrendje a rekordban lévő sorrendjük szerinti.
    /// @return A lekérdezés után, a __q.first() által visszaadott érték
    bool fetchQuery(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), const tIntVector& __ord = tIntVector(), int __lim = 0, int __off = 0, QString __s = QString(), QString __w = QString()) const;
    /// Beolvassa az adatbázisból azt a rekordot/rekordokat, amik a megadott mező maszk esetén egyeznek az
    /// objektumban tárolt mező(k) érték(ek)kel.
    /// Az első rekordot beolvassa az objektumba, ill ha nincs egyetlen rekord, akkor törli az objektumot.
    /// Kilépés után a lekérdezés eredménye a __q által hivatkozott QSqlRecord objektumban marad. Kurzor az első rekordon.
    /// Ha a megadott maszk (__fn) paraméter men üres, de minden eleme false, akkor a tábla összes sora ki lesz szelektálva.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __only Ha megadjuk és értéke true, akkor a származtatott táblákban nem keres.
    /// @param __fm A mező(k) maszk, alapértelmezése üres, ekkor a használt maszk az elsődleges kulcs mező(k). Lásd még a whereString() metódust.
    /// @param __ord Mely mezők szerint legyenek rendezve a találatok ( a vektor elemei mező indexek, negatív érték csökkenő sorrendet jelent, a 0. elem nem rendezhető csökkenő sorrendbe).
    /// @param __lim Limit. Ha értéke 0, akkor nincs.
    /// @param __off Ofszet. Ha értéke 0, akkor nincs.
    /// @return true, ha a feltételnek megfelelt legalább egy rekord, és azt beolvasta. false, ha nincs egyetlen rekord sem.
    virtual bool fetch(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), const tIntVector& __ord = tIntVector(), int __lim = 0, int __off = 0);
    /// Beolvassa az adatbázisból azt a rekordot/rekordokat, amik a megadott mező maszk esetén egyeznek az
    /// objektumban tárolt mező(k) érték(ek)kel.
    /// Az első rekordot beolvassa az objektumba, ill ha nincs egyetlen rekord, akkor törli az objektumot.
    /// Kilépés után a lekérdezés eredménye a __q által hivatkozott QSqlRecord objektumban marad. Kurzor az első rekordon.
    /// @param __only Ha megadjuk és értéke true, akkor a származtatott táblákban nem keres.
    /// @param __fm A mező(k) maszk, alapértelmezése üres, ekkor a használt maszk az elsődleges kulcs mező(k). Lásd még a whereString() metódust.
    /// @param __ord Mely mezők szerint legyenek rendezve a találatok ( a vektor elemei mező indexek, negatív érték csökkenő sorrendet jelent, a 0. elem nem rendezhető csökkenő sorrendbe)
    /// @param __lim Limit. Ha értéke 0, akkor nincs.
    /// @param __off Ofszet. Ha értéke 0, akkor nincs.
    /// @return true, ha a feltételnek megfelelt legalább egy rekord, és azt beolvasta. false, ha nincs egyetlen rekord sem
    bool fetch(bool __only = false, const QBitArray& __fm = QBitArray(), const tIntVector& __ord = tIntVector(), int __lim = 0, int __off = 0)
            { QSqlQuery q = getQuery(); return fetch(q, __only, __fm, __ord, __lim, __off); }
    /// Beolvas egy rekordot a megadott id alapján. Ld.: fetch() metódust
    /// Ha nincs, vagy nem ismert az ID mező, akkor dob egy kizárást
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __id Opcionális ID paraméter, ha nem adjuk meg, akkor az objektum aktuális ID alapján olvassa be a rekordot.
    /// @return Ha sikerült beolvasni legalább egy rekordot, akkor true.
    bool fetchById(QSqlQuery& __q, qlonglong __id = NULL_ID)    { if (__id != NULL_ID) setId(__id); return fetch(__q, false, _bit(idIndex())); }
    /// Beolvas egy rekordot a megadott id alapján. Lásd még a fetch() metódust.
    /// Ha nincs, vagy nem ismert az ID mező, akkor dob egy kizárást
    /// @param __id Opcionális ID paraméter, ha nem adjuk meg, akkor az objektum aktuális ID alapján olvassa be a rekordot.
    /// @return Ha sikerült beolvasni legalább egy rekordot, akkor true.
    bool fetchById(qlonglong __id = NULL_ID)                    { if (__id != NULL_ID) setId(__id); return fetch(false, _bit(idIndex())); }
    /// Beolvas egy rekordot a megadott id alapján. Ld.: fetchById(QSqlQuery& __q, qlonglong __id) metódust,
    /// de azzal ellentétben, ha nem találja a keresett rekordot, akkor dob egy kizárást.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __id Opcionális ID paraméter, ha nem adjuk meg, akkor az objektum aktuális ID alapján olvassa be a rekordot.
    /// @return a (bázis!) objektum referenciájával tér vissza
    cRecord& setById(QSqlQuery& __q, qlonglong __id = NULL_ID)  { if (!fetchById(__q, __id)) EXCEPTION(EFOUND, __id, descr()._tableName); return *this; }
    /// Beolvas egy rekordot a megadott id alapján. Ld.: fetchById(qlonglong __id) metódust,
    /// de azzal ellentétben, ha nem találja a keresett rekordot, akkor dob egy kizárást.
    /// @param __id Opcionális ID paraméter, ha nem adjuk meg, akkor az objektum aktuális ID alapján olvassa be a rekordot.
    /// @return a (bázis!) objektum referenciájával tér vissza
    cRecord& setById(qlonglong __id = NULL_ID)                  { if (!fetchById(__id)) EXCEPTION(EFOUND, __id, descr()._tableName); return *this; }
    /// Beolvas egy rekordot a megadott név alapján. Ld.: fetch(QSqlQuery& __q, int __fn) metódust
    /// Ha nincs vagy nem ismert a név mező, akkor dob egy kizárást
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __name Opcionális név paraméter, ha nem adjuk meg, akkor az objektum aktuális neve alapján olvassa be a rekordot.
    /// @return Ha sikerült beolvasni legalább egy rekordot, akkor true.
    bool fetchByName(QSqlQuery& __q, const QString& __name = QString()) { if (!__name.isEmpty()) setName(__name); return fetch(__q, false, _bit(nameIndex())); }
    /// Beolvas egy rekordot a megadott név alapján. Lásd még a fetch() metódust.
    /// Ha nincs vagy nem ismert a név mező, akkor dob egy kizárást
    /// @param __name Opcionális név paraméter, ha nem adjuk meg, akkor az objektum aktuális neve alapján olvassa be a rekordot.
    /// @return Ha sikerült beolvasni legalább egy rekordot, akkor true.
    bool fetchByName(const QString& __name = QString())                 { if (!__name.isEmpty()) setName(__name); return fetch(false, _bit(nameIndex())); }
    /// Beolvas egy rekordot a megadott név alapján. Ld.: fetchByName(QSqlQuery& __q, const QString& __name) metódust,
    /// de azzal ellentétben, ha nem találja a keresett rekordot, akkor dob egy kizárást.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __name Opcionális név paraméter, ha nem adjuk meg, akkor az objektum aktuális neve alapján olvassa be a rekordot.
    /// @return a (bázis!) objektum referenciájával tér vissza
    cRecord& setByName(QSqlQuery& __q, const QString& __name = QString())   { if (!fetchByName(__q, __name)) EXCEPTION(EFOUND,-1, __name); return *this; }
    /// Beolvas egy rekordot a megadott név alapján. Ld.: fetchByName(const QString& __name) metódust,
    /// de azzal ellentétben, ha nem találja a keresett rekordot, akkor dob egy kizárást.
    /// @param __name Opcionális név paraméter, ha nem adjuk meg, akkor az objektum aktuális neve alapján olvassa be a rekordot.
    /// @return a (bázis!) objektum referenciájával tér vissza
    cRecord& setByName(const QString& __name = QString())                   { if (!fetchByName(__name)) EXCEPTION(EFOUND,-1, __name); return *this; }
    /// A tábla rekord(ok) módosítása az adatbázisban. Az első rekordot visszaolvassa.
    /// Lásd még a whereString(QBitArray& __fm) metódust is.
    /// @param __q A művelethez használt QSqlQuery objektum.
    /// @param __only A módosításokat csak a megadott táblában, a leszármazottakban nem, ha true
    /// @param __set Bitmap, a módosítandó mezőkkel azonos indexű biteket 1-be kell állítani. Ha üres, ill nincs benne egyetlen true sem, akkor a where bitek negáltja.
    /// @param __where Opcionális bitmap, a feltételben szereplő mezőkkel azonos indexű biteket 1-be kell állítani. Alapértelmezetten az elsődleges kulcs mező(ke)t használja,
    ///                ha egy üres tömböt adunk át, ha viszont nem üres, de egyetlen true értéket sem tartalmazó tömböt adunk meg, akkor az a tábla
    ///                összes elemét kiválasztja.
    /// @param __ex Ha EX_NOOP, és nincs egyetlen modosított rekord sem, akkor dob egy kizárást.
    /// @return A modosított rekordok száma.
    int update(QSqlQuery& __q, bool __only, const QBitArray& __set = QBitArray(), const QBitArray& __where = QBitArray(), enum eEx __ex = EX_NOOP);
    /// Hasonló, mint az update metódus, azt hívja egy try blokkban.
    /// Hiba esetén, vagyis, ha a hívott update metódus kizárást dobott, akkor a hiba objektum pointerével tér vissza.
    /// Ha a __tr paraméter értéke TS_TRUE, vagy TS_NULL és nem vagyunk egy lezáratlan tranzakción bellül, akkor az update() metódus hívását egy
    /// tranzakciós blokba zárja. Ekkor hiba esetén a commit helyett a rolback parancssal zárja le azt. A __tr alapértelmezetten TS_NULL.
    /// @return NULL, vagy a hiba objektum pointere, ha valamilyen hiba volt.
    cError *tryUpdate(QSqlQuery& __q, bool __only, const QBitArray& __set = QBitArray(), const QBitArray& __where = QBitArray(), eTristate __tr = TS_NULL);
    /// Try blokkban frissíti egy rekord tartalmát, az azonosító az ID, minden egyébb mezőt frissít az objektum tartalma alapján.
    /// Akkor is hibával tér vissza, ha nem csak egy rekordot modosított.
    /// Kiírja a nyelvi szövegeket is, ha tartoznak ilyenek az objektumban, és a pTextList nem NULL.
    cError *tryUpdateById(QSqlQuery& __q, eTristate __tr = TS_NULL, bool text = false);

    ///
    bool updateFieldByName(QSqlQuery &__q, const QString& _name, const QString& _fn, const QVariant& val, eEx __ex = EX_NOOP);
    bool updateFieldById(QSqlQuery &__q, qlonglong _id, const QString& _fn, const QVariant& val, eEx __ex = EX_NOOP);
    /// Beállítja a 'flag' nevű mező értékét a kiválasztott rekordokban. Az objektum értéke nem változik.
    /// @param __q A művelethez használt QSqlQuery objektum.
    /// @param __flag A flag mezőbe beállítandó érték, alapértelmezettem true.
    /// @param __where Opcionális bitmap, a feltételben szereplő mezőkkel azonos indexű biteket 1-be kell állítani.
    ///                Ha egy üres tömböt adunk át, akkor az elsődleges kulcs mező(ket) használja.
    ///                Ha viszont nem üres, de egyetlen true értéket sem tartalmazó tömböt adunk meg, akkor az a tábla
    ///                összes elemét kiválasztja (ez az alapértelmezés)
    /// @return A modosított rekordok száma
    int mark(QSqlQuery& __q, const QBitArray& __where = QBitArray(1), bool __flag = true) const;
    /// Törli a kiválasztott, és 'flag' nevű mezővel megjelölt (értéke true) rekordokat. Az objektum értéke nem változik.
    /// @param __q A művelethez használt QSqlQuery objektum.
    /// @param __where Opcionális bitmap, a feltételben szereplő mezőkkel azonos indexű biteket 1-be kell állítani.
    ///                Ha egy üres tömböt adunk át, akkor az elsődleges kulcs mező(ket) használja.
    ///                Ha viszont nem üres, de egyetlen true értéket sem tartalmazó tömböt adunk meg, akkor az a tábla
    ///                összes elemét kiválasztja (ez az alapértelmezés)
    /// @return A törölt rekordok száma
    int removeMarked(QSqlQuery& __q, const QBitArray& __where = QBitArray(1)) const;
    /// Törli az adatbázisból azokat a rekordot/rekordokat, amik a megadott mező maszk esetén egyeznek az
    /// objektumban tárolt mező(k) érték(ek)kel. Ha csak logikailag töröl, akkor az első mezőt visszaolvassa.
    /// Lásd még a whereString(QBitArray& __fm) metódust is.
    /// @param __q Az QSqlQuery objektum referenciája, amivel a lekérdezést végezzük.
    /// @param __only Ha megadjuk és értéke true, akkor a származtatott táblákban nem keres.
    /// @param __fn A mező(k) maszk, alapértelmezése üres, ekkor a használt maszk az elsődleges kulcs mező(k).
    /// @param __ex Ha EX_NOOP, és nincs egyetlen törölt rekord sem, akkor dob egy kizárást.
    /// @exception Hiba esetén dob egy kizárást, valamint ha __ex EX_NOOP, és nincs egy törölt rekord sem.
    /// @return A törölt rekordok száma
    int remove(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), enum eEx __ex = EX_NOOP);
    /// Hasonló a remove metódushoz, de a hiba esetén a hiba objektum pointerével tér vissza, ha volt kizárás.
    /// Ha nem volt hiba akkor a visszaadott érték a NULL pointer.
    cError *tryRemove(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray(), eTristate __tr = TS_NULL);
    /// Adat ellenőrzést végez
    /// Beállítja _stat értékét
    virtual bool checkData();
    /// Egy bitvektort ad vissza, ahol minden bit 1, mellyel azonos indexű mező nem NULL az objektumban.
    QBitArray getSetMap() const;
    /// Hasonló a fetch-hez. A lekérdezésben az összes nem null mezőt (és értékét) megadja a WHERE után.
    /// Lásd még a whereString(QBitArray& __fm) metódust is.
    /// Az első rekord, ha volt legalább egy, akkor beolvasásra kerül az objektumba. A többi a next() metódussal
    /// érhető el.
    /// @param _ord Ha megadjuk a paramétert, akkor az a rendezési sorrendet definiálja (lásd fetch() metódust.)
    /// @return A feltételeknek megfelelő rekordok száma.
    int completion(QSqlQuery& __q, const tIntVector &_ord = tIntVector());
    /// Hasonló a másik completion() metódushoz, de a metódus egy saját QsqlQuery objektumot kér, és azt
    /// Lásd még a whereString(QBitArray& __fm) metódust is.
    /// visszatérés előtt felszabadítja, így további rekordok beolvasására nincs lehetőeég.
    int completion()                { QSqlQuery q = getQuery(); return completion(q); }
    /// A fetch() vagy complation() vagy más metódus által elvégzett lekérdezés eredményének első rekordját
    /// betölti az objektumba.
    /// @return true, ha feltöltötte az objektumot, ha nincs adat, akkor false.
    bool first(QSqlQuery& __q);
    /// A fetch() vagy complation() vagy más metódus által elvégzett lekérdezés eredményének következő rekordját
    /// betölti az objektumba.
    /// @return true, ha feltöltötte az objektumot, ha nincs több adat, akkor false.
    bool next(QSqlQuery& __q);
    /// Hasonló a fetch() metódushoz, de csak a rekordok számát kérdezi le, konstans metódus.
    int rows(QSqlQuery& __q, bool __only = false, const QBitArray& __fm = QBitArray()) const {
        fetchQuery(__q, __only, __fm, tIntVector(), 0,0,QString("count(*)"));
        return __q.value(0).toInt();
    }
    int existId(QSqlQuery& q, qlonglong __id) const;
    /// Megnézi létezik-e a rekord a név (és az azt kiegészítő, egyedivétevő mezők) alapján.
    /// Ha olyan mező alapján kell keresni, amely nincs kitöltve (NULL) azt kitölti az alapértelmezése szerint.
    /// Hívja a nemeKeyMask() metódust, ha egy a maszkban szereplő mező értéke NULL, és nincs alapértelmezett
    /// értéke, akkor is kizárást dob. Ha a lekérdezés szerint egynél több rekordot talál az is kizárást eredményez.
    bool existByNameKey(QSqlQuery& __q, eEx __ex = EX_ERROR) const;
    /// Hasonló a fetch() metódushoz, de csak a rekordok számát kérdezi le, konstans metódus
    int rows(bool __only = false, const QBitArray& __fm = QBitArray()) const { QSqlQuery q = getQuery(); return rows(q, __only, __fm);  }
    /// Az objektum típusnak megfelelő tableoid-vel tér vissza.
    /// Ez nem feltétlenül azonos azzal a tableoid-vel, amely táblából beolvastuk a rekordot, ha beolvastuk.
    qlonglong tableoid() const  { return descr().tableoid(); }
    /// Csak a tableoid mezőt olvassa be, a megadott mezők alapján.
    /// Ha a megadott, vagyis értékkel rendelkező mezők nem egyértelműen azonosítanak egy rekordot,
    /// akkor ha __ex igaz dob egy kizárást, ill. ha __ex hamis, akkor NULL_ID-vel tér vissza.
    /// Ha egy leszármazott tábla rekordját azonosítjuk, akkor a visszaadott érték nem lessz feltétlenül
    /// azonos a tableoid() álltal visszaadottal. Ha egy ős objektumba olvastunk be egy leszármazott rekordot,
    /// akkor a visszaadott érték a rekord valódi típusának megfelelő OID-t adja vissza.
    qlonglong fetchTableOId(QSqlQuery& __q, enum eEx __ex = EX_ERROR) const;
    /// Megvizsgálja, hogy a kitöltött mezők alapján a hozzá tartozó rekord egyértelműen meghatározott-e.
    /// Vagyis ki kell töltve lennie legalább egy kulcs mező csoportnak, vagy a primary kulcs mező(k)nek.
    /// @return ha az objektum adattartalma csak egy rekordot jelenthet, akkor true
    bool isIdent() const;
    /// Megvizsgálja, hogy a megadott mezők nem null értéküek-e
    /// @param __m A bit mezőben a viszgálandó mezővel azonos indexű bitek 1-es értéküek.
    /// @return Ha egyik jelzett érték sem null, akkor true.
    bool isIdent(const QBitArray& __m) const;
    /// A forrásból átmásol minden olyan mezőt, amelyik mindkét rekordban létezik, és a melyik a forrásban nem NULL
    /// A mező adatokat nem törli a másolás előtt. Hasonló a _copy metódushoz, de ez virtuális metódust is hív.
    /// A másolási művelet után meghívja a checkData() metódust, hogy beállítsa a _stat értékét
    cRecord& set(const cRecord& __o) { if (_copy(__o, descr())) checkData(); return *this; }
    /// A forrásból átmásol minden olyan mezőt, amelyik mindkét rekordban létezik, és a melyik a forrásban nem NULL
    /// A mező adatokat nem törli a másolás előtt. Hasonló a _copy metódushoz, de ez virtuális metódust is hív.
    /// A _stat értékére ugyanaz igaz, mint ami a _copy() metódusban le van írva.
    cRecord& set_(const cRecord& __o) { _copy(__o, descr()); return *this; }
    /// Csak két azonos típusú objektumok között használható metódus. (descr() == __o.descr()), ha ez a feltétel nem
    /// teljesül dob egy kizárást.
    /// Azokat a mezőket másolja át a __o objektumból, melyekkel megegyező indexű __m elem értéke true.
    /// A NULL értékek is másolva lesznek, ha az __m megfelelő indexű eleme igaz. Ha az __m üres, akkor az összes elem másolásra kerül.
    /// Az __m elemszáma nem lehet nagyobb, mint a mezők száma, ellenkező esetben kizárást dob a metódus.
    void fieldsCopy(const cRecord& __o, const QBitArray& __m = QBitArray());
    /// Látrehoz egy azonos típusú objektumot, és feltölti a név alapján. A névre a pName paraméter mutat, vagy ha az
    /// NULL vagy üres stringre mutat, akkor az objektum beállított neve.
    /// Az látrehozott objektumból azokat mezőket másolja át, melyekkel megegyező indexű __m elem értéke true.
    /// A NULL értékek is másolva lesznek, ha az __m megfelelő indexű eleme igaz. Ha az __m üres, akkor az összes elem másolásra kerül.
    /// Az __m elemszáma nem lehet nagyobb, mint a mezők száma, ellenkező esetben kizárást dob a metódus.
    void fieldsCopy(QSqlQuery& __q, QString *pName, const QBitArray& __m = QBitArray());
    /// Mező érték, ill. referencia az index alapján. Nem valódi referenciával tér vissza,
    /// hanem egy cRecordFieldRef példánnyal. Valódi, a mező értékre mutató referencia használata ebben az estben potesnciálisan
    /// veszályes lenne, valamit kikerülné a konverziós függvényeket is, ami nem cél.
    /// @param __i A mező indexe
    /// @return A megadott mezőre mutató referencia objektummal tér vissza
    /// @exception Ha a megadott index érték nem egy valós mező indexe.
    cRecordFieldRef operator[](int __i);
    /// Mező érték, ill. referencia az index alapján. Nem valódi referenciával tér vissza,
    /// hanem egy cRecordFieldConstRef példánnyal. Valódi, a mező értékre mutató referencia használata ebben az estben potesnciálisan
    /// veszályes lenne, valamit kikerülné a konverziós függvényeket, ami nem cél.
    /// @param __i A mező indexe
    /// @return A megadott mezőre mutató konstans referencia objektummal tér vissza
    /// @exception Ha a megadott index érték nem egy valós mező indexe.
    cRecordFieldConstRef operator[](int __i) const;
    /// Mező érték, ill. referencia az mező név alapján. Nem valódi referenciával tér vissza,
    /// hanem egy cRecordFieldRef példánnyal. Valódi, a mező értékre mutató referencia használata ebben az estben potesnciálisan
    /// veszályes lenne, valamit kikerülné a konverziós függvényeket, ami nem cél.
    /// @param __i A mező neve
    /// @return A megadott mezőre mutató referencia objektummal tér vissza
    /// @exception Ha a megadott név nem egy valós mező neve.
    cRecordFieldRef operator[](const QString& __fn);
    /// Mező érték, ill. referencia az mező név alapján.  Nem valódi referenciával tér vissza,
    /// hanem egy cRecordFieldConstRef példánnyal. Valódi, a mező értékre mutató referencia használata ebben az estben potesnciálisan
    /// veszályes lenne, valamit kikerülné a konverziós függvényeket, ami nem cél.
    /// @param __i A mező neve
    /// @return A megadott mezőre mutató konstans referencia objektummal tér vissza
    /// @exception Ha a megadott név nem egy valós mező neve.
    cRecordFieldConstRef operator[](const QString& __fn) const;
    cRecordFieldRef ref(int _ix);
    cRecordFieldConstRef cref(int _ix) const;
    cRecordFieldRef ref(const QString& _fn);
    cRecordFieldConstRef cref(const QString& _fn) const;
    /// Az id mező nevével tér vissza, ha van id mező, egyébként dob egy kizárást
    /// @param __ex Ha értéke hamis és nincs id mező, akkor nem dob kizárást, hanem egy üres stringgel tér vissza.
    const QString& idName(enum eEx __ex = EX_ERROR) const   { return descr().idName(__ex); }
    /// Az név mező nevével tér vissza, ha van név mező, egyébként dob egy kizárást
    /// @param __ex Ha értéke hamis és nincs név mező, akkor nem dob kizárást, hanem egy üres stringgel tér vissza.
    const QString& nameName(enum eEx __ex = EX_ERROR) const { return descr().nameName(__ex); }
    /// Az descr mező nevével tér vissza, ha van név mező, egyébként dob egy kizárást
    /// @param __ex Ha értéke hamis és nincs descr mező, akkor nem dob kizárást, hanem egy üres stringgel tér vissza.
    const QString& noteName(enum eEx __ex = EX_ERROR) const { return descr().noteName(__ex); }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befolyásolja a
    /// működését. És az objektum értéke sem változik.
    virtual qlonglong getIdByName(QSqlQuery& __q, const QString& __n, enum eEx __ex = EX_ERROR) const {
        return descr().getIdByName(__q, __n, __ex);
    }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befolyásolja a
    /// működését.
    qlonglong getIdByName(const QString& __n, enum eEx __ex = EX_ERROR) const { return descr().getIdByName(__n, __ex); }
    /// A ID alapján visszaadja a rekord nevet, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befolyásolja a
    /// működését.
    QString getNameById(QSqlQuery& __q, qlonglong __id, enum eEx __ex = EX_ERROR) const { return descr().getNameById(__q, __id, __ex); }
    /// A ID alapján visszaadja a rekord nevet, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Nem static, mivel virtuális függvénytagokat hív, bár az objektum aktuális értéke nem befolyásolja a
    /// működését.
    QString getNameById(qlonglong __id, enum eEx __ex = EX_ERROR) const { return descr().getNameById(__id, __ex); }
    /// Egy mask előállítása, ahol a paraméterrel megadott indexű bit igaz, a többi hamis. A bitek száma azonos a mezők számával.
    QBitArray mask(int __i1) const                              { return descr().mask(__i1); }
    /// Egy mask előállítása, ahol a paraméterrekkel megadott indexű bitek igazak, a többi hamis. A bitek száma azonos a mezők számával.
    QBitArray mask(int __i1, int __i2) const                    { return descr().mask(__i1, __i2); }
    /// Egy mask előállítása, ahol a paraméterrekkel megadott indexű bitek igazak, a többi hamis. A bitek száma azonos a mezők számával.
    QBitArray mask(int __i1, int __i2, int __i3) const          { return descr().mask(__i1, __i2, __i3); }
    /// Egy mask előállítása, ahol a paraméterrekkel megadott indexű bitek igazak, a többi hamis. A bitek száma azonos a mezők számával.
    QBitArray mask(int __i1, int __i2, int __i3, int __i4) const{ return descr().mask(__i1, __i2, __i3, __i4); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lessz, ahány mező van a táblában, és a megadott mezőnevekkel azonos indexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1) const                                               { return descr().mask(__n1); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lesz, ahány mező van a táblában, és a megadott mezőnevekkel azonos indexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1, const QString& __n2) const                          { return descr().mask(__n1, __n2); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lessz, ahány mező van a táblában, és a megadott mezőnevekkel azonos indexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3) const     { return descr().mask(__n1, __n2, __n3); }
    /// Mező bit maszk előállítása, mező nevek szerint.
    /// A bit vektornak annyi eleme lesz, ahány mező van a táblában, és a megadott mezőnevekkel azonos indexű bitek 1-be lesznek állítva.
    QBitArray mask(const QString& __n1, const QString& __n2, const QString& __n3, const QString& __n4) const { return descr().mask(__n1, __n2, __n3, __n4); }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 1-be állítja.
    /// Ha a megadott név bármelyike nem egy mező név, akkor dob egy kizárást, ha __ex igaz, ha ex hamis figyelmenkívül hagyja a nevet.
    QBitArray mask(const QStringList& __nl, enum eEx __ex = EX_ERROR) const { return descr().mask(__nl, __ex); }
    QBitArray inverseMask(const QBitArray& __m) const { return descr().inverseMask(__m); }
    /// Létrehoz egy bit tömböt ugyan akkora mérettel, mint a mezők száma, és a megadott mezőneveknek megfelelő sorszámú biteket 0-be állítja,
    /// a többit 1-be.
    QBitArray inverseMask(const QStringList& __nl, enum eEx __ex = EX_ERROR) const { return descr().inverseMask(mask(__nl, __ex)); }
    /// Egy egész szám vektort állít elő, ahol a vektor elemei a név szzerint megadott mező indexe, lezárva egy -1 -értékkel.
    tIntVector   iTab(const QString& __n1)                        { return descr().iTab(__n1); }
    /// Egy egész szám vektort állít elő, ahol a vektor elemei a név szzerint megadott mezők indexe, lezárva egy -1 -értékkel.
    tIntVector   iTab(const QString& __n1, const QString& __n2)    { return descr().iTab(__n1, __n2); }
    /// Egy egész szám vektort állít elő, ahol a vektor elemei a név szzerint megadott mezők indexe, lezárva egy -1 -értékkel.
    tIntVector   iTab(const QString& __n1, const QString& __n2, const QString& __n3)    { return descr().iTab(__n1, __n2, __n3); }
    /// Egy egész szám vektort állít elő, ahol a vektor elemei a név szzerint megadott mezők indexe, lezárva egy -1 -értékkel.
    tIntVector   iTab(const QString& __n1, const QString& __n2, const QString& __n3, const QString& __n4)    { return descr().iTab(__n1, __n2, __n3, __n4); }
    /// Az objektumot (csak az adattartalmát) stringé konvertálja.
    virtual QString toString() const;
    /// Az objektumot stringé konvertálja, a rekord leíróval együtt.
    QString allToString() const { return descr().toString() + " = " + toString(); }
    /// Rekord(ok) törlésa a név alapján
    /// @param  q
    /// @param __n A név mező értéke, vagy minta a név mezőre
    /// @param __pat Ha értéke true, akkor az __n paraméter minta stringnek tekintendő (LIKE).
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
    /// Ellenőrzi, hogy az objektum eredeti típusa megegyezik-e a megadott típussal.
    /// A statikus leíró _tableOId adattagjait hasonlítja össze.
    /// A metódus feltételezi, hogy az öröklődési láncok ekvivalensek!
    /// Ha T, és/vagy az objektum típusa cRecordAny, akkor -1 -el tér bissza, vagy kizárást dob..
    /// @param __ex ha értéke EX_WARNING vagy nagyobb, akkor ha nincs egyezés kizárást dob,
    ///             Ha viszont értéke EX_ERROR és nem konvertálható a típus, akkor kizárást dob.
    ///             Ha értéke nem EX_IGNORE, és T vagy az objektum cRecodeAny, kizárást dob.
    /// @return 0, ha egyezés van, 1 ha konvertálható, és -1 ha nem konvertálható.
    template <class T> int chkObjType(enum eEx __ex = EX_ERROR) const {
        T o;
        if (typeid(T) == typeid(cRecordAny) || typeid(*this) == typeid(cRecordAny)) {
            if (__ex >= EX_ERROR) EXCEPTION(EDATA, 0, QString(QObject::trUtf8("The object type can not be converted, %1 ~ %2").arg(descr().tableoid()).arg(o.descr().tableoid())));
            return -1;
        }
        if (descr().tableoid() == o.descr().tableoid()) return 0;  // Azonos
        if (__ex >= EX_WARNING) EXCEPTION(EDATA, 1, QString(QObject::trUtf8("Object type is not equal, %1 ~ %2").arg(descr().tableoid()).arg(o.descr().tableoid())));
        if (descr() > o.descr()) return 1;        // Nem azonos, de konvertálható
        if (__ex >= EX_ERROR)   EXCEPTION(EDATA, 2, QString(QObject::trUtf8("The object type can not be converted, %1 ~ %2").arg(descr().tableoid()).arg(o.descr().tableoid())));
        return -1;
    }
    /// Az objektum pointert visszaalakítja az eredeti és megadott típusba. Lást még a chkObjType<T>() -t.
    /// Ha az eredeti, és a megadott típus nem eggyezik, vagy az eredeti típusnak nem leszármazottja a megadott típus, akkor dob egy kizárást
    template <class T> T * reconvert() { chkObjType<T>(); return dynamic_cast<T *>(this); }
    template <class T> const T * creconvert() const { chkObjType<T>(); return dynamic_cast<const T *>(this); }
    /// Megvizsgálja, rendeben van-e a kitöltött objektum, és nem üres-e.
    /// @return Ha az objektum üres, vagy a státusz változó szerint nincs rendben, akkor true-val tér vissza
    bool operator !() { return _stat <= ES_EMPTY || ((_stat & (ES_INCOMPLETE | ES_DEFECTIVE)) != 0); }
    /// Ha az objektum nem tartalmaz egyetlen nem null mezőt sem, akkor true-val tér vissza.
    /// Nem a _stat adattag alapján tér vissza, ill. azt beállítja ha a visszaadott érték true.
    /// Konstans objektum esetén használjuk az isEmpty_() metódust, az a _stat értéke alapján tér vissza.
    bool isEmpty();
    /// Azonos az IsEmpty() hívással. Mivel nem hív virtuális metódust, le lett definiálva ezen a néven is.
    bool _isEmpty()                        { return isEmpty(); }
    /// Megvizsgálja, hogy a megadott indexű bit a likeMask-ban milyen értékű, és azzal tér vissza, ha nincs ilyen elem, akkor false-val.
    bool _isLike(int __ix) const { return __ix < 0 || _likeMask.size() <= __ix ? false : _likeMask[__ix]; }
    /// Az aktuális időt írja a last_time nevű mezőbe, az első módosított rekord aktuális tartalmát visszaolvassa.
    /// Azt, hogy mely rekordokat kell módosítani, a _where tartalma határozza meg
    /// Azok a rekordok lesznek módosítva, melyeket az o.completion() beolvasna, de a módosítandó mező ki van zárva a feltételből.
    /// @param q Az adatbázis művelethez használható query objektum.
    /// @param _fn Opcionális paraméter, ha megadjuk akkor nem a last_time lessz módosítva, hanem a megadott nevű mező.
    /// @param __where Opcionális bitmap, a feltételben szereplő mezőkkel azonos indexű biteket 1-be kell állítani.
    ///        Alapértelmezetten az elsődleges kulcs mező(ke)t használja, ha egy üres tömböt adunk át,
    ///        ha viszont nem üres, de egyetlen true értéket sem tartalmazó tömböt adunk meg,
    ///        akkor az a tábla összes rekordját kiválasztja, kivéve, ha vab deleted mező és értéke true.
    /// @return A módosított rekordok száma.
    int touch(QSqlQuery& q, const QString&_fn = _sNul, const QBitArray &_where = QBitArray());
    /// Az aktuális időt írja a megadott indexű mezőbe, az első módosított rekord aktuális tartalmát visszaolvassa.
    /// Azt, hogy mely rekordokat kell módosítani, az objektum adattartalma határozza meg
    /// Azok a rekordok lesznek módosítva, melyeket az o.completion() beolvasna, de a módosítandó mező ki van zárva a feltételből.
    /// @param q Az adatbázis művelethez használható query objektum.
    /// @param _fi A módosítandó mező indexe.
    /// @return A módosított rekordok száma. Ha üres objektummal hívjuk, akkor -1
    int touch(QSqlQuery& q, int _fi) { return touch(q, columnName(_fi)); }
    /// A megadott indexű mezőleíró objektum referenciájával tér vissza
    const cColStaticDescr& colDescr(int _ix) const { return descr().colDescr(_ix); }
    /// A megadott nevű mezőleíró objektum referenciájával tér vissza
    const cColStaticDescr& colDescr(const QString& _fn) const { return colDescr(toIndex(_fn)); }
    /// A megadott indexű mező értékét adja vissza, konvertálva az SQL-nek átadható formára.
    /// Hibás adat esetén kizárást dob!
    QVariant toSql(int __ix) const { return colDescr(__ix).toSql(get(__ix)); }
    /// A megadott nevű mező értékét adja vissza, konvertálva az SQL-nek átadható formára.
    /// Hibás adat esetén kizárást dob!
    QVariant toSql(const QString& __nm) const { return toSql(toIndex(__nm)); }
    /// Paraméter megadása (bind) egy SQL lekérdezéshez, ahol a paraméter érték a rekord objektum egy mezője.
    /// @param _ix A mező indexe a rekord objektumban
    /// @param __q A lekérdezéshez használt objektum.
    /// @param _i  A paraméter indexe a lekérdezésben
    const cRecord& bind(int _ix, QSqlQuery& __q, int _i) const {
        const cColStaticDescr& f = colDescr(_ix);
        QSql::ParamType t = f.eColType == cColStaticDescr::FT_BINARY ? QSql::In | QSql::Binary : QSql::In;
        QVariant v = f.toSql(get(_ix));
        // PDEB(VVERBOSE) << QString("#%1 << [%2] = %3").arg(_i).arg(_ix).arg(debVariantToString(v)) << endl;
        __q.bindValue(_i, v, t);
        return *this;
    }
    /// Paraméter megadása (bind) egy SQL lekérdezéshez, ahol a paraméter érték a rekord objektum egy mezője.
    /// @param _fn A mező neve a rekord objektumban
    /// @param __q A lekérdezéshez használt objektum.
    /// @param _i  A paraméter indexe a lekérdezésben
    const cRecord& bind(const QString& _fn, QSqlQuery& __q, int _i) const {
        return bind(toIndex(_fn), __q, _i);
    }
    /// Paraméter megadása (bind) egy SQL lekérdezéshez, ahol a paraméter érték a rekord objektum egy mezője.
    /// @param _ix A mező indexe a rekord objektumban
    /// @param __q A lekérdezéshez használt objektum.
    /// @param _ph  A paraméter neve a lekérdezésben
    const cRecord& bind(int _ix, QSqlQuery& __q, const QString& _ph) const {
        const cColStaticDescr& f = colDescr(_ix);
        QSql::ParamType t = f.eColType == cColStaticDescr::FT_BINARY ? QSql::In | QSql::Binary : QSql::In;
        __q.bindValue(_ph, f.toSql(get(_ix)), t);
        return *this;
    }
    /// Paraméter megadása (bind) egy SQL lekérdezéshez, ahol a paraméter érték a rekord objektum egy mezője.
    /// @param _fn A mező neve a rekord objektumban
    /// @param __q A lekérdezéshez használt objektum.
    /// @param _ph  A paraméter neve a lekérdezésben
    const cRecord& bind(const QString& _fn, QSqlQuery& __q, const QString& _ph) const {
        return bind(toIndex(_fn), __q, _ph);
    }
    /// Paraméter megadása (bind) egy SQL lekérdezéshez, ahol a paraméter érték a rekord objektum egy mezője.
    /// @param __q A lekérdezéshez használt objektum.
    /// @param _i A mező indexe a rekord objektumban éa a paraméter indexe a lekérdezésben
    const cRecord& bind(int _i,  QSqlQuery& __q) const {
        return bind(_i, __q, _i);
    }
    /// Paraméter megadása (bind) egy SQL lekérdezéshez, ahol a paraméter érték a rekord objektum egy mezője.
    /// @param __q A lekérdezéshez használt objektum.
    /// @param _n A mező neve a rekord objektumban és a paraméter neve a lekérdezésben
    const cRecord& bind(const QString& _n,  QSqlQuery& __q) const {
        return bind(_n, __q, _n);
    }
    /// Egy SQL lekérdezésből egy érték bemásolása a rekord objektum egy mezőjébe.
    /// @param _ix A cél mező indexe
    /// @param __q A lekérdezés eredményét tartalmazó objektum
    /// @param _i Az adat indexe a lekérdezés eredményében
    cRecord& setq(int _ix, QSqlQuery& __q, int _i) {
        return set(_ix, descr().colDescr(_ix).fromSql(__q.value(_i)));
    }
    /// Egy SQL lekérdezésből egy érték bemásolása a rekord objektum egy mezőjébe.
    /// @param _fn A cél mező neve
    /// @param __q A lekérdezés eredményét tartalmazó objektum
    /// @param _i Az adat indexe a lekérdezés eredményében
    cRecord& setq(const QString& _fn, QSqlQuery& __q, int _i) {
        return setq(toIndex(_fn), __q, _i);
    }
    /// Egy SQL lekérdezésből egy érték bemásolása a rekord objektum egy mezőjébe.
    /// @param _ix A cél mező indexe
    /// @param __q A lekérdezés eredményét tartalmazó objektum
    /// @param _rn Az adat neve a lekérdezés eredményében
    cRecord& setq(int _ix, QSqlQuery& __q, const QString& _rn) {
        return set(_ix, descr().colDescr(_ix).fromSql(__q.value(_rn)));
    }
    /// Egy SQL lekérdezésből egy érték bemásolása a rekord objektum egy mezőjébe.
    /// @param _fn A cél mező neve
    /// @param __q A lekérdezés eredményét tartalmazó objektum
    /// @param _rn Az adat neve a lekérdezés eredményében
    cRecord& setq(const QString& _fn, QSqlQuery& __q, const QString& _rn) {
        return setq(toIndex(_fn), __q, _rn);
    }
    /// Egy SQL lekérdezésből egy érték bemásolása a rekord objektum egy mezőjébe.
    /// @param _ix A cél mező indexe és az adat indexe a lekérdezés eredményében
    /// @param __q A lekérdezés eredményét tartalmazó objektum
    cRecord& setq(int _ix, QSqlQuery& __q) {
        return setq(_ix, __q, _ix);
    }
    /// Egy SQL lekérdezésből egy érték bemásolása a rekord objektum egy mezőjébe.
    /// @param _fn A cél mező neve és az adat neve a lekérdezés eredményében
    /// @param __q A lekérdezés eredményét tartalmazó objektum
    cRecord& setq(const QString& _fn, QSqlQuery& __q) {
        return setq(_fn, __q, _fn);
    }
    /// Átmásolja az objektum tartalmát, elötte ellenörzi a típus azonosságot vagyis
    /// a két statikus leírónak azonosnak kell lennie (a konkrét objektum típus lehet különböző,
    /// ha pl. az egyik a cRecordAny.
    cRecord& copy(const cRecord& o) {
        if (descr() != o.descr()) EXCEPTION(EDATA, -1, QString("%1 != %2").arg(fullTableName(), o.fullTableName()));
        _cp(o);
        return *this;
    }
    /// Egy mező értékének a beállítása a megadott rekordokban.
    /// @param q
    /// @param _nl A beállítandó rekordok név mező értékeinek a listája.
    /// @param _fn A beállítandó mező neve
    /// @param _v A beállítandó érték.
    int updateFieldByNames(QSqlQuery& q, const QStringList& _nl, const QString& _fn, const QVariant& _v) const;
    /// Egy array mező tartalmához hozzáad egy értéket a megadott nevű rekordokban. Konstans metódus, csak az adatbázis tartalma változik.
    /// @param q
    /// @param _nl A modosítandó rekordok nevei
    /// @param _fn A mező neve
    /// @param _v az érték, amit a mező tartalmához hozzá kell adni.
    int addValueArrayFieldByNames(QSqlQuery& q, const QStringList& _nl, const QString& _fn, const QVariant& _v) const;
    /// Egy mező értékének a beállítása a megadott rekordokban.
    /// @param q
    /// @param _il A beállítandó rekordok ID mezői értékeinek a listája.
    /// @param _fn A beállítandó mező neve
    /// @param _v A beállítandó érték.
    int updateFieldByIds(QSqlQuery& q, const QList<qlonglong>& _il, const QString& _fn, const QVariant& _v) const;
    /// Egy mező értékének a beállítása a megadott rekordokban.
    /// @param q
    /// @param _nl A beállítandó rekordok név mezőinek a listája.
    /// @param _fi A beállítandó mező idexe
    /// @param _v A beállítandó érték.
    int updateFieldByNames(QSqlQuery& q, const QStringList& _nl, int _fi, const QVariant& _v) const
    {
        return updateFieldByNames(q, _nl, columnName(_fi), _v);
    }
    /// Egy mező értékének a beállítása a megadott rekordokban.
    /// @param q
    /// @param _il A beállítandó rekordok ID mezői értékeinek a listája.
    /// @param _fi A beállítandó mező indexe
    /// @param _v A beállítandó érték.
    int updateFieldByIds(QSqlQuery& q, const QList<qlonglong>& _il, int _fi, const QVariant& _v) const
    {
        return updateFieldByIds(q, _il, columnName(_fi), _v);
    }
    bool isEmpty(int _ix) const;
    /// Áltaéános rekord azonosító szöveg. Pl. hibaüzeneteknél az objektum beazonosításához.
    /// @param t A típust is beteszi a kimenetbe, ha igaz. Alapértelmezetten igaz.
    QString identifying(bool t = true) const;
    /// Rekord azonosító szöveg. Alapértelmezetten az identifying() -al azonos,
    /// De virtuális, és objektumonként megadható a kimenet tartalma
    /// @param t A típust is beteszi a kimenetbe, ha igaz. Alapértelmezetten hamis.
    virtual QString show(bool t = false) const;

protected:
    QString objectExportLine(QSqlQuery& q, int _indent, QString& line) const;
    QString getCondString(QSqlQuery& q, QString::const_iterator& i, const QString::const_iterator& e) const;
    QString parseString(QSqlQuery& q, const QString& __src, const QStringList &__pl, int __indent, int *__pIx = nullptr) const;
    int parseParams(QSqlQuery& q, QStringList &pl, int ss) const;

    qlonglong _defectiveFieldMask() const {
        return _stat >> 16;
    }
    qlonglong defectiveFieldMask() const {
        return _defectiveFieldMask() & ((1 << cols()) -1);
    }
    void _setDefectivFieldBit(int ix) {
        _stat |= 1LL << (ix + 16);
    }
    void setDefectivFieldBit(int ix) {
        chkIndex(ix);
        _setDefectivFieldBit(ix);
    }
    void _clearDefectivFieldBit(int ix) {
        _stat &= ~(1LL << (ix + 16));
    }
    void clearDefectivFieldBit(int ix) {
        chkIndex(ix);
        _clearDefectivFieldBit(ix);
    }
    ///
    static void setNameKeyMask(const cRecStaticDescr *pD,  const QBitArray& __m)
    {
        const_cast<cRecStaticDescr *>(pD)->_nameKeyMask = __m;
    }
    /// Copy constructor. Nem támogatott konstruktor. Dob egy kizárást.
    cRecord(const cRecord& o);
    /// átmásolja ellenörzés nélkül az objektum adattagjait, a mező adatok kivételével.
    void __cp(const cRecord& __o) {
        _stat    = __o._stat;
        _deletedBehavior = __o._deletedBehavior;
        _likeMask = __o._likeMask;
        containerValid = __o.containerValid;
    }
    /// átmásolja ellenörzés nélkül az objektum adattagjait
    void _cp(const cRecord& __o) {
        _fields = __o._fields;
        __cp(__o);
        pDelete(pTextList);
        if (__o.pTextList != nullptr) pTextList = new QStringList(*__o.pTextList);
    }
    /// Átmásolja a paraméterként megadott objektum mezőit. Előtte nem törli az objektum mező adatait.
    /// Ha a forrás olyan mezőt tartalmaz, amit a cél nem, akkor azokat figyelmen kívül hagyja.
    /// A _stat adattagot nem módősítja, csak ha ST_NULL, volt, akkor hívja _set(d); metódust, ezért az ekkor ST_EMPTY lessz !
    /// A mezők azonosságát csak a név alapján dönti el.
    /// @return Ha módosította az objektumot, akkor true
    bool _copy(const cRecord& __o, const cRecStaticDescr& d);
    /// Törli az összes mező adatot (_fields konténer teljes tartalmát), továbbá törli a likeMask adattagot is.
    /// stat-ot ES_NULL -ra állítja.
    cRecord& _clear();
    /// Törli a megadott indexű mezőt. Ha ezután minden mező NULL lesz, akkor _stat-ot ES_EMPTY-re, ha nem, akkor
    /// _stat-ban bebillenti az ES_MODIFY bitet.
    cRecord& _clear(int __ix);
    /// Törli az adatmezőket, és újra létrehozza üres tartalommal. A _stat-ot ES_EMPTY-re állítja.
    /// @param __d Tábla leíró objektum
    /// @param clear_text Ha értéke true (ez az alapértelmezett) akkor törli a nyelvi szövegeket is, ha voltak.
    cRecord& _set(const cRecStaticDescr& __d, bool clear_text = true);
    /// Létrehozza az összes mezőt sorrendben, és feltölti (a mezőnevek alapján) az __r-ben lévő tartalommal.
    /// Nem hív virtuális metódust.
    /// Ha _r olyan mezőt tartalmaz, melyet az objektum nem, akkor az figyelmen kívül lesz hagyva.
    /// Előtte nem törli az objektum tartalmát. A mezők feltöltése után meghívja a paraméter nélküli toEnd() virtuális metódust,
    /// majd a modified() signalt.
    /// (A mezők feltöltésekor a toEnd(int) metódusok nem kerülnek meghívásra.
    /// @param __r A kitöltendő mező adatokat tartalmazó rekord objektum.
    /// @param __d Tábla leíró objektum
    /// @param __fromp Egy sorszámra mutató pointer, ha nem null, akkor a beolvasott rekord első figyelembevett eleme az itt megadott sorszámú.
    ///              a sorszám értéke a függvény visszatértekor az utolsó felhasznált mező sorszáma +1 lessz, ha nem NULL a pointer.
    /// @param __size Ha értéke nem -1, akkor a beolvasott rekordból csak ennyi mező lesz figyelembe véve.
    cRecord& _set(const QSqlRecord& __r, const cRecStaticDescr& __d, int* __fromp = nullptr, int __size = -1);
    /// Beállítja a megadott sorszámú mező értékét. Az objektum, ill. a _field konténer nem lehet üres, egyébként dob egy kizárást.
    /// Nem hív virtuális metódust, így a toEnd() metódusokat sem, így egyéb adatott nem módosít, a státust sem.
    cRecord& _set(int __i, const QVariant& __v) { if (isNull()) EXCEPTION(EPROGFAIL); _fields[__i] =  __v; return *this; }
    /// Hasonló a get(int __i) metódushoz, de nincs index ellenőrzés, ha nincs a keresett mező dob egy kizárást.
    /// A visszaadott érték nem konstans referencia. Csak kellő körültekintéssel használható !
    QVariant& _get(int __i)  { if (__i < 0 || __i >=  _fields.size()) EXCEPTION(EPROGFAIL,__i); return _fields[__i]; }
    /// Ellenőrzi, hogy egy az objektum "tulajdonában lévő" rekordok listáját törölni kell-e. (pl. egy node esetén a port lista)
    /// Üres konténert nem töröl.
    /// A konténernek csak az első elemét vizsgálja. Ha az ID mező nincs kitöltve, akkor feltételezi, hogy a rekordok
    /// még nincsenek rögzítve, és nem törli a konténert.
    /// Ha az első vizsgállt elemben a tulajdonos id-je megegyezik az objektum id-vel szintén nem törli a konténert.
    /// Minden egyébb esetben törli a konténer tartalmát.
    /// @param Ch A konténerben tárolt adatok típusa
    /// @param It Az index paraméter típusa (int vagy QString lehet)
    /// @param c A konténer referenciája
    /// @param ixOwnerId A konténer elemeiben a tulajdonos rekord id-jét tartalmazó mezőt azonosító index, vagy név.
    /// @return Ha törölte a konténert, akkor true, egyébként false;
    template <class Ch, class It> bool atEndCont(tRecordList<Ch>& c, It ixOwnerId)
    {
           // Ha üres nem töröljük, mert minek,
           if (c.isEmpty()) return false;
           // Az első elemet fogjuk vizsgálni
           Ch& ch = *c.first();
           // ha az elemeknél nincs kitöltve az ID, akkor is békénhagyjuk. Az még nincs rögzítve az adatbázisban.
           if (ch.getId() == NULL_ID) return false;
           // ha stimmel az owner id-je akkor sem töröljük
           if (ch.getId(ixOwnerId) == getId()) return false;
           c.clear();
           return true;
    }
signals:
    /// Signal: Ha egy mező értéke megváltozott (A modified szignált hívó metódus nem hívja ezt a szignált mezőnként)
    void fieldModified(int ix);
    /// Signal: Ha a rekord teljes tartalma megváltozott, pl. adatbázis műveletnél beolvasásra került egy rekord.
    void modified();
    /// Signal: Ha az adattartalma lett törölve az objektumnak
    void cleared();
// Localization
protected:
    /// Pointer a nyelvi szövegekre ha vannak, és beolvasásra kerültek, egyébbként NULL pointer.
    QStringList    *pTextList;
    /// Törli a nyelvi szövegeket
    void delTextList();
    /// Feltételessen törli a nyelvi szövegeket
    /// @param _ix A modosított mező indexe, ha értéke nem NULL_IX, akkor egyeznie kell a text_id mező indexszel, ha nem nincs törlés.
    /// @param _tid Az új text_id mező érték, ha nem egyezik a régi értékkel, vagy mindkettő nem NULL, akkor törlöl.
    void        condDelTextList(int _ix = NULL_IX, const QVariant& _tid = QVariant());
public:
    qlonglong   getTextId(eEx __ex = EX_ERROR) { int ix = descr().textIdIndex(__ex); return 0 > ix ? NULL_ID : getId(ix); }
    QString     getText(int _tix, const QString& _d = QString()) const;
    QString     getText(const QString& _tn, const QString& _d = QString()) const;
    QStringList getTexts() const { return pTextList == nullptr ? QStringList() : *pTextList; }
    cRecord&    setText(int _tix, const QString& _t);
    cRecord&    setText(const QString& _tn, const QString& _t);
    cRecord&    setTexts(const QStringList& _txts);
    bool fetchText(QSqlQuery& _q, bool __force = true);
    void saveText(QSqlQuery& _q);
    qlonglong   containerValid;
};
TSTREAMO(cRecord)

/// Hash készítése egy cRecord objektumból, a hash-t az első mező intté konvertált értékéből képzi, mely álltalában az ID.
inline static uint qHash(const cRecord& key) { return qHash(key.getId(0)); }

/// A rekord leíró objektum pointer inicializálása.
/// Ha a _pRecordDescr adattag NULL, akkor a cRecStaticDescr::get(const QString&, const QString&) hívással
/// kér egyet, és ez lessz _pRecordDescr új értéke, és ekkor true-val tér vissza.
/// Ha a _pRecordDescr adattag nem NULL volt, akkor visszatér egy false értékkel.
/// Az R osztály a cRecord egy leszármazottja kell legyen, és rendelkeznie kell egy
/// _pRecordDescr (cRecStaticDescr * típusú) adattaggal.
/// @relates cRecord
/// @return Ha inicializálni kellett a pointert, akkor true.
template <class R> bool initPDescr(const QString& _tn, const QString& _sn = _sPublic)
{
    if (R::_pRecordDescr == nullptr) {
        R::_pRecordDescr = cRecStaticDescr::get(_tn, _sn);
        return true;
    }
    return false;
}

/// A statikus rekord leíró objektum pointer inicializálása, ha ez még nem történt meg
/// Hívja az initPDescr<R>(const QString&, const QString&) függvényt.
/// @relates cRecord
/// @return A leíró pointere
template <class R> const cRecStaticDescr *getPDescr(const QString& _tn, const QString& _sn = _sPublic)
{
    initPDescr<R>(_tn, _sn);
    return R::_pRecordDescr;
}

/// @def CRECORD(R)
/// @relates cRecord
/// Egy cRecord leszármazottban néhány feltétlenül szükséges deklaráció
/// @param R Az osztály név
#define CRECORD(R) \
        friend bool initPDescr<R>(const QString& _tn, const QString& _sn); \
        friend const cRecStaticDescr *getPDescr<R>(const QString& _tn, const QString& _sn); \
        template <typename R1, typename R2> friend class tOwnRecords; \
    public: \
        R(); \
        R(const R& __o); \
        virtual const cRecStaticDescr&  descr() const; \
        virtual ~R(); \
        virtual cRecord *newObj() const; \
        virtual cRecord *dup()const; \
        virtual R& clone(const cRecord& __o); \
        static const cRecStaticDescr& _descr_##R() { if (R::_pRecordDescr == nullptr) R().descr(); return *R::_pRecordDescr; } \
    protected: \
        static const cRecStaticDescr * _pRecordDescr

/// @def DESCR(R)
/// @relates cRecord
/// Egy cRecord-ból származtatott osztály statikus leíró objektumának a referenciája.
/// A cRecordAny -nál nem alkalmazható, a makró feltételezi, hogy az osztály statikus
/// leíró statikus pointerének a neve a CRECORD() makróval, vagy annak megfelelően lett deklarálva.
/// @param az osztály név, melynek a statikus leíróját kérjük.
#define DESCR(R)    R::_descr_##R()

/// @def CRECCNTR(R)
/// @relates cRecord
/// Egy alapértelmezett ill. egyszerű cRecord leszármazott objektum konstruktorainak a definiciói.
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CRECCNTR(R) \
    R::R()             : cRecord() { _set(R::descr()); } \
    R::R(const R& __o) : cRecord() { _cp(__o); }


/// @def CRECDEFNC(R)
/// @relates cRecord
/// A cRecord leszármazottakban a newObj() és dup() virtuális metódusokat definiáló makró
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CRECDEFNC(R) \
    const cRecStaticDescr * R::_pRecordDescr = nullptr; \
    cRecord *R::newObj() const { return new R(); }   \
    cRecord *R::dup()const     { return new R(*this); } \

/// @def CRECDEF(R)
/// @relates cRecord
/// A cRecord leszármazottakban a newObj() és dup() virtuális metódusokat definiáló makró
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CRECDEF(R) \
    CRECDEFNC(R) \
    R& R::clone(const cRecord& __o) { clear(); copy(__o); return *this; }

/// @def  CRECDDCR(R, tn)
/// @relates cRecord
/// Az alapértelmezett descr() metódus definiciója.
/// @param R Az osztály neve (mely a tn nevű tábla kezelését végzi)
/// @param tn Az adatbázis tábla neve
#define CRECDDCR(R, tn)     const cRecStaticDescr&  R::descr() const { return *getPDescr<R>(tn); }

/// @def CRECDEFD(R)
/// @relates cRecord
/// A cRecord leszármazottakban a destruktort, a newObj() és dup() virtuális metódusokat definiáló makró
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CRECDEFD(R)    CRECDEF(R) R::~R() { ; }

/// @def CRECDEFNCD(R)
/// @relates cRecord
/// A cRecord leszármazottakban a destruktort, a newObj() és dup() virtuális metódusokat definiáló makró
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CRECDEFNCD(R)    CRECDEFNC(R) R::~R() { ; }

/// @def DEFAULTCRECDEF(R)
/// @relates cRecord
/// @param R Az osztály neve (mely a tn nevű tábla kezelését végzi)
/// @param tn Az adatbázis tábla neve
/// Egy alapértelmezett cRecord leszármazott objektum teljes definíciója
#define DEFAULTCRECDEF(R, tn)   CRECCNTR(R) CRECDEFD(R) CRECDDCR(R, tn)

template <class R> R * dup(R *p) { return dynamic_cast<R *>(p->dup()); }

/// @relates cRecord
class LV2SHARED_EXPORT cRecordFieldConstRef {
    friend class cRecord;
protected:
    /// A hivatkozott objektum referenciája
    const cRecord *  _pRecord;
    /// A hivatkozott mező indexe
    int             _index;
    /// Konstruktor a mező indexével
    cRecordFieldConstRef(const cRecord& __r, int __ix) {
        _pRecord = &__r;
        _index = __ix;
        _pRecord->chkIndex(_index);
    }
    /// Konstruktor a mező nevével
    cRecordFieldConstRef(const cRecord& __r,const QString& __n) {
        _pRecord = &__r;
        _index = __r.toIndex(__n);
        _pRecord->chkIndex(_index);
    }
public:
    /// Copy konstruktor
    cRecordFieldConstRef(const cRecordFieldConstRef& __r)  { _index = __r._index; _pRecord = __r._pRecord; }
    ///
    cRecordFieldConstRef& operator =(const cRecordFieldConstRef& __r)  { _index = __r._index; _pRecord = __r._pRecord; return *this; }
    /// A hivatkozott mező értéke. Vigyázat a függvény egy referencsiát ad vissza, ami potenciálisan veszályes lehet,
    /// ha megváltozik a cDecord::_field adattagja.
    operator const QVariant&() const   { return _pRecord->get(_index); }
    /// A hivatkozott mező értéke egész számként
    operator qlonglong() const  { return _pRecord->getId(_index); }
    /// A hivatkozott mező értéke stringként
    operator QString() const    { return _pRecord->getName(_index); }
    /// A hivatkozott mező értéke stringként
    QString toString() const    { return _pRecord->getName(_index); }
    operator bool() const       { return _pRecord->getBool(_index); }
    /// A hivatkozott objektum statuszának a referenciája
    const qlonglong& stat() { return _pRecord->_stat; }
    /// A hivatkozott objektum referenciája
    const cRecord& record() const           { return *_pRecord; }
    /// A hivatkozott mező leíró objektumának a referenciája
    const cRecStaticDescr& recDescr() const { return _pRecord->descr(); }
    /// A hivatkozott objektum leíró objektumának a referenciája
    const cColStaticDescr& descr() const    { return _pRecord->descr().colDescr(_index); }
    /// A hivatkozott mező indexe
    int index() const           { return _index; }
    /// Ha a mező értéke NULL, akkor true-val tér vissza
    bool isNull() const         { return _pRecord->isNull(_index); }
    /// Ha a mező értéke az adatbázisban modosítható, akkor true-val tér vissza
    bool isUpdatable() const    { return _pRecord->isUpdatable(_index); }
    /// Ha a mező értéke lehet NULL, akkor true-val tér vissza
    bool isNullable() const     { return _pRecord->isNullable(_index); }
    /// A mező névvel tár vissza
    QString columnName() const { return descr().colName(); }
    /// A teljes (tábla névvel kiegészített) mező névvel tér vissza
    QString fullColumnName() const { return recDescr().fullColumnName(columnName()); }
    ///
    QString view(QSqlQuery& q) { return _pRecord->view(q, _index); }
};
TSTREAMO(cRecordFieldConstRef)

/// Egy referencia osztály, valamely cRecord leszármazott példány egy mezőjére.
/// @relates cRecord
class LV2SHARED_EXPORT cRecordFieldRef {
    friend  class cRecord;
protected:
    /// A hivatkozott objektum referenciája
    cRecord&    _record;
    /// A hivatkozott mező indexe
    int         _index;
    /// Konstruktor a mező indexével
    cRecordFieldRef(cRecord& __r, int __ix) : _record(__r) {
        _index = __ix;
        _record.chkIndex(_index);
    }
    /// Konstruktor a mező nevével
    cRecordFieldRef(cRecord& __r,const QString& __n)  : _record(__r) {
        _index = __r.toIndex(__n);
        _record.chkIndex(_index);
    }
public:
    /// Copy konstruktor
    cRecordFieldRef(const cRecordFieldRef& __r);
    /// Értékadás a hivatkozott mezőnek, lásd a cRecord::set(int, const QVariant&)
    cRecordFieldRef operator=(const QVariant& __v) const {
        _record.set(_index, __v);
        return *this;
    }
    /// Értékadás a hivatkozott mezőnek, lásd a cRecord::set(int, const QVariant&)
    cRecordFieldRef operator=(const cRecordFieldRef& __r) const {
        _record.set(_index, __r);
        return *this;
    }
    /// Értékadás a hivatkozott mezőnek, lásd a cRecord::set(int, const QVariant&)
    cRecordFieldRef operator=(const cRecordFieldConstRef& __r) const {
        _record.set(_index, __r);
        return *this;
    }
    /// Értékadás a hivatkozott mezőnek, lásd a cRecord::setId(int, qlonglong)
    cRecordFieldRef operator=(qlonglong __v) const {
        _record.setId(_index, __v);
        return *this;
    }
    /// Értékadás a hivatkozott mezőnek, lásd a cRecord::setName(int, const QString&)
    cRecordFieldRef operator=(const QString& __v) const {
        _record.setName(_index, __v);
        return *this;
    }
    /// A hivatkozott mező értéke. Vigyázat a függvény egy referencsiát ad vissza, ami potenciálisan veszályes lehet,
    /// ha megváltozik a cDecord::_field adattagja.
    operator const QVariant&() const   { return _record.get(_index); }
    /// A hivatkozott mező értéke egész számként, lásd még a cRecord::getId(int) metódust.
    operator qlonglong() const  { return _record.getId(_index); }
    /// A hivatkozott mező értéke stringként, lásd még a cRecord::getName(int) metódust.
    operator QString() const    { return _record.getName(_index); }
    /// Törli a hivatkozott mezőt, lásd még a cRecord::clear(int) metódust.
    cRecordFieldRef& clear()    { _record.clear(_index); return *this; }
    /// A hivatkozott mező értéke stringként, lásd még a cRecord::getName(int) metódust.
    QString toString() const    { return *this; }
    /// A hivatkozott mező értéke stringként, lásd még a cRecord::view(QSqlQuery&, int) metódust.
    QString view(QSqlQuery& q) const { return _record.view(q, _index); }
    /// Ha a mező értéke NULL, akkor true-val tér vissza
    bool isNull() const         { return _record.isNull(_index); }
    /// Ha a mező értéke az adatbázisban modosítható, akkor true-val tér vissza
    bool isUpdatable() const    { return _record.isUpdatable(_index); }
    /// Ha a mező értéke lehet NULL, akkor true-val tér vissza
    bool isNullable() const     { return _record.isNullable(_index); }
    /// A hivatkozott objektum statuszának a referenciája
    const qlonglong& stat() const { return _record._stat; }
    /// Frissíti a hivatkozott mező értéket az adatbázisban, a rekordot az elsődleges kulcs azonosítja, lásd méga a cRecord::update() metódust.
    bool update(QSqlQuery& q, eEx __ex = EX_NOOP) const {
        return _record.update(q, false, _record.mask(_index), _record.primaryKey(), __ex);
    }
    /// A hivatkozott objektum referenciája
    const cRecord& record() const           { return _record; }
    /// A hivatkozott mező leíró objektumának a referenciája
    const cRecStaticDescr& recDescr() const { return _record.descr(); }
    /// A hivatkozott objektum leíró objektumának a referenciája
    const cColStaticDescr& descr() const    { return _record.descr().colDescr(_index); }
    /// A hivatkozott mező indexe
    int index() const                       { return _index; }
    /// A mező névvel tár vissza
    const QString& columnName() const { return descr().colName(); }
    /// A teljes (tábla névvel kiegészített) mező névvel tér vissza
    QString fullColumnName() const { return recDescr().fullColumnName(columnName()); }
};
TSTREAMO(cRecordFieldRef)

inline cRecordFieldRef cRecord::operator[](const QString& __fn)             { return (*this)[chkIndex(toIndex(__fn))]; }
inline cRecordFieldConstRef cRecord::operator[](const QString& __fn) const  { return (*this)[chkIndex(toIndex(__fn))]; }
inline cRecordFieldRef cRecord::ref(int _ix)                        { return (*this)[_ix]; }
inline cRecordFieldConstRef cRecord::cref(int _ix) const            { return (*this)[_ix]; }
inline cRecordFieldRef cRecord::ref(const QString& _fn)             { return (*this)[_fn]; }
inline cRecordFieldConstRef cRecord::cref(const QString& _fn) const { return (*this)[_fn]; }


/*!
@class cRecordAny
@brief Általános interfész osztály.
Azt hogy melyik adattáblát kezeli azt a konstruktorban, vagy a setType() metódusokban kell beállítani.
Természetesen csak az alapfunkciókra képes, amik a cRecord-ban meg lettek valósítva.
 */
class LV2SHARED_EXPORT cRecordAny : public cRecord {
public:
    /// Üres konstruktor. Használat elütt hivni kell valamelyik setType() metódust.
    /// Ha nincs beállítva típus, akkor minden olyan hívásra, melynek szüksége van
    /// a descriptor objektumra egy kizárást fog dobni.
    cRecordAny();
    /// A konstruktor a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// objektumot. Az objektum adatt tartalma (mező adat konténer) üres lessz.
    /// @param _tn Az adattáble neve
    /// @param _sn A schema név mely az adattáblát tartalmazza. Obcionális, alapértelmezett a "public".
    cRecordAny(const QString& _tn, const QString& _sn = _sNul);
    /// A konstruktor a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// Az objektum adatt tartalma (mező adat konténer) üres lessz.
    /// @param _p A leíró objektum pointere
    cRecordAny(const cRecStaticDescr * _p);
    /// Copy konstruktor
    cRecordAny(const cRecordAny &__o);
    /// "Univerzális" copy (szerű) konstruktor
    cRecordAny(const cRecord &__o);
    /// A metódus a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// objektumot. Az objektum adatt tartalma (mező adat konténer) üres lessz, ill. azt törli.
    /// @param _tn Az adattáble neve
    /// @param _sn A schema név mely az adattáblát tartalmazza. Obcionális, alapértelmezett a "public".
    cRecordAny& setType(const QString& _tn, const QString& _sn = _sNul, eEx __ex = EX_ERROR);
    /// A metódus az __o -ban már megadott, vagy ezt implicite tartalmazó adattáblához rendeli, ill. annak kezelésére készíti fel.
    /// Ha __o-nál nincs megadva adattábla, ill. ismeretlen a descriptor objektum, akkor dobni fog egy kizárást.
    cRecordAny& setType(const cRecord& __o);
    cRecordAny& setType(const cRecStaticDescr *_pd);
    /// Alaphelyzetbe állítja az objektumot, mint az üres konstruktor után.
    cRecordAny& clearType();
    /// A descriptor objektum referenciájával tér vissza.
    /// Ha nem adtuk meg a kezelendő adattáblát, akkor dob egy kizárást.
    virtual const cRecStaticDescr& descr() const;
    /// destruktor. (üres metódus).
    virtual ~cRecordAny();
    /// Egy új objektum allokálása. Az új objektum örökli a hívó descriptorát.
    virtual cRecord *newObj() const;
    /// Az objektumr klónozása
    virtual cRecord *dup()const;
    /// Másoló operátor. Átmásolja a forrás objektum descriptor mutatóját (típusát) és a mező adatokat.
    cRecordAny& operator=(const cRecord& __o);
    /// Másoló operátor. Átmásolja a forrás objektum descriptor mutatóját (típusát) és a mező adatokat.
    cRecordAny& operator=(const cRecordAny& __o) { return *this = static_cast<const cRecord&>(__o); }
    ///
    static int updateFieldByNames(QSqlQuery& q, const cRecStaticDescr * _p, const QStringList& _nl, const QString& _fn, const QVariant& _v);
    static int updateFieldByIds(QSqlQuery& q, const cRecStaticDescr * _p, const QList<qlonglong>& _il, const QString& _fn, const QVariant& _v);
    static int addValueArrayFieldByNames(QSqlQuery& q, const cRecStaticDescr * _p, const QStringList& _nl, const QString& _fn, const QVariant& _v);
protected:
    const cRecStaticDescr *pStaticDescr;
};

/// Template függvény a "features" mező felbontására
/// A sablon föggvény számít arra, hogy az objektumnak van egy
/// _pFeatures konténere, ahova az eredményt helyezi, továbbá egy _ixFeatures statikus adattagja.
/// Ha a konténer meg volt allokálva (_pFeatures != NULL), akkor kiüríti.
/// Ha nem volt megallokálva, akkor létrehozza.
/// ami a features mező indexe.
/// @param R Egy cRecord leszármazott osztály
/// @param o Az objektumpéldány
/// @param __ex ha értéke EX_ERROR, akkor hiba esetén dob egy kizárást, EX_WARNING esetén akkor is, ha a konténer már létezik.
/// @relates cRecord
/// @return Ha hibás a features mező, akkor false-val tér vissza, feltéve, hogy nem dobott kizárást (lásd: __ex)
template <class R> bool _SplitFeatureT(R& o, eEx __ex = EX_ERROR)
{
    if (o._pFeatures == nullptr) o._pFeatures = new cFeatures();
    else {
        if (__ex >= EX_WARNING) EXCEPTION(EREDO);
        o._pFeatures->clear();
    }
    QString fv = o.getName(R::ixFeatures());
    bool r = o._pFeatures->split(fv, false, EX_IGNORE);
    if (!r) {
        QString msg = QObject::trUtf8(
                    "feature mezőjének a formátuma nem megfelelő."
                    "A feature mező értéke : %1. %2").
                arg(dQuoted(fv), o.identifying());
        EXCEPTION(EDATA, 0, msg);
    }
    return r;
}
/// Egy módosított map visszaírása a "features" mezőbe. Nem hiívja az cRecord::atEnd(int) metódust.
/// A sablon föggvény számít arra, hogy az objektumnak van egy
/// _pFeatures konténere, továbbá egy _ixFeatures statikus adattagja, ami a features mező indexe.
/// @param S Egy cRecord leszármazott osztály
/// @param o Az objektumpéldány
/// @relates cRecord
template <class R> void _JoinFeatureT(R& o)
{
    if (o._pFeatures == nullptr ) return;
    QString prop;
    prop = o._pFeatures->join();
    if (o.getName(R::ixFeatures()) != prop) { // Nem túl hatékony (sorrend változhat), de bonyi lenne.
        o._set(R::ixFeatures(), QVariant(prop));   // nincs atEnd() !
        o._stat |= ES_MODIFY;
    }
}

template <class R> qlonglong intFeature(const R& o, const QString& key, qlonglong _def)
{
    QString v = o.feature(key);
    if (v.isEmpty()) return _def;
    bool ok;
    qlonglong r = v.toLongLong(&ok);
    if (ok) return r;
    return _def;
}

/// @def STATICIX(R, n)
/// Deklarál (a cRecord leszármazott osztály definíción bellül) egy statikus mező index adattagot,
/// és egy publikus függvényt az eléréséhez. A függvény, ha kell inicializálja az
/// indexet, és ellenörzi, hiba esetén kizárást dob.
/// Az inicializálást az objektum konstruktorának meghívásával kényszeríti ki.
/// @param R Osztály név (cRecord leszármazott)
/// @param n Az index neve a "_ix" előtag nélkül.
#define STATICIX(R, n) \
 protected: \
    static int _ix##n; \
 public: \
    static int ix ## n() { \
        if (_ix##n < 0) { \
            R o; (void)o; \
            if (_ix ## n < 0) EXCEPTION(EPROGFAIL, _ix ## n, __STR(R::_ix##n)); \
        } \
        return _ix ## n; \
    }

/// @def STFIELDIX(R, n)
/// A statikus index inicializálása (lásd:STATICIX() makrót is)
/// @param c Osztály név (cRecord leszármazott)
/// @param m Az index neve a "_ix" előtag nélkül.
#define STFIELDIX(R,n)   _ix ## n = _descr_ ## R().toIndex(_s ## n)

/// @def FEATURES()
/// Egy cRecord leszármazott osztálydeklaráció törzsében használható makró.
/// A features mező kezeléséhez szükséges adattagot, és metóduaokat deklarálja.
/// A deklarált adattag:
/// @code
/// cFeatures *pFeatures;
/// @code
/// A paramétereket tároló konténer pointere.
/// @par
/// A definiált metódusok:
/// @code
/// cFeatures&  splitFeature(eEx __ex);
/// @code
/// Feltölti (ha kell allokálja) a konténert. Ha a konténer létezett, akor elöszőr törli a tartalmát.
/// Lásd: template <class R> bool _SplitFeatureT(R& o, eEx __ex)
/// @code
/// const cFeatures&  features(eEx __ex);
/// @code
/// Visszaadja a pFeatures adattag által mutatott cFeatures konténer objektumot.
/// Ha a a pFeatures pointer NULL, akkor megallokálja, és feltülti azt.
/// Mivel a pFeatures csak egy gyorstár jellegű konténer, a metódus ezt konxtans objektum esetén is elvégzi.
/// @code
/// cFeatures&  features(eEx __ex);
/// @code
/// Visszaadja a pFeatures adattag által mutatott cFeatures konténer objektumot.
/// Ha a a pFeatures pointer NULL, akkor megallokálja, és feltülti azt.
/// @code
/// const QString& feature(const QString &_nm);
/// @code
/// A megadott nevű paraméter értékével tér vissza (ha a konténer még nincs létrehozva, akkor létrehozza, és feltülti lásd: cFeatures&  features(eEx __ex);
/// @code
/// qlonglobf feature(const QString &_nm, qlonglong _def);
/// @code
/// A megadott nevű paraméter értékével tér vissza, ha nincs ilyen nevű paraméter, vagy nem értelmezhető egész számként,
/// akkor a _def fúggvényparaméter értékével tér vissza.
/// (ha a konténer még nincs létrehozva, akkor létrehozza, és feltülti lásd: cFeatures&  features(eEx __ex);

#define FEATURES(R) \
    friend bool _SplitFeatureT<R>(R& o, eEx __ex); \
    friend void _JoinFeatureT<R>(R& o); \
    STATICIX(R, Features) \
protected: \
    cFeatures *_pFeatures; \
public: \
    cFeatures&  splitFeature(eEx __ex = EX_ERROR) { _SplitFeatureT<R>(*this, __ex); return *_pFeatures; } \
    const cFeatures&  features(eEx __ex = EX_ERROR) const { if (_pFeatures == nullptr) const_cast<R *>(this)->splitFeature(__ex); return *_pFeatures; } \
    cFeatures&  features(eEx __ex = EX_ERROR) { if (_pFeatures == nullptr) this->splitFeature(__ex); return *_pFeatures; } \
    QString feature(const QString &_nm) const { return features().value(_nm); } \
    bool  isFeature(const QString &_nm) const { return features().contains(_nm); } \
    R& joinFeature() { _JoinFeatureT<R>(*this); return *this;  } \
    qlonglong feature(const QString &_nm, qlonglong _def) const { return intFeature(*this, _nm, _def); } \
    void modifyByFeature(const QString& _fn) { cRecordFieldRef fr = (*this)[_fn]; features().modifyField(fr); }


class LV2SHARED_EXPORT cLanguage : public cRecord {
    CRECORD(cLanguage);
public:
    cLanguage(const QString& _name, const QString& _lc, const QString& _l3, qlonglong _flagId, qlonglong _nextId);
    cLanguage& setByLocale(QSqlQuery &_q, const QLocale& _l = QLocale::system());
    cLanguage& setFromLocale(const QLocale& _l = QLocale::system());
    QLocale locale(eEx __ex = EX_ERROR);
    static void newLanguage(QSqlQuery &q, const QString& _name, const QString& _lc, const QString& _l3, qlonglong _flagId, qlonglong _nextId);
    static void repLanguage(QSqlQuery &q, const QString& _name, const QString& _lc, const QString& _l3, qlonglong _flagId, qlonglong _nextId);
};

#endif // LV2DATAB_H
