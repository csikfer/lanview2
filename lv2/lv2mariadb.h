#ifndef LV2MARIADB_H
#define LV2MARIADB_H
#include "lanview.h"
#include <typeinfo>
#include <QSqlDatabase>

/*!
@file lv2mariadb.h
A mariadb (mysql?) adatbázis interfész bázis objektuma, és a rekord ill. mező leíró/kezelő objektumok.
A kiegészítés alapvetően a GLPI MySQL adatbázisának a kezelését célozza meg.
*/

/// @class cMariaDb
/// A Maria DB kezeló objektum (hasonlóan a lanView osztályhoz, csak egy példány!)
class LV2SHARED_EXPORT cMariaDb : public QSqlDatabase {
public:
    static bool init(eEx __ex = EX_IGNORE);
    static bool drop(eEx __ex = EX_IGNORE);
    static cMariaDb * pInstance(eEx __ex = EX_ERROR);
    static const QString& tablePrefix() { return pInstance()->sTablePrefix; }
    static QString schemaName()   { return pInstance()->databaseName(); }
    static QSqlQuery getQuery(eEx __ex = EX_ERROR);
    static QSqlQuery * newQuery(eEx __ex = EX_ERROR);
    static const QString sConnectionType;
    static const QString sConnectionName;
protected:
    cMariaDb();
    ~cMariaDb();
    QString sTablePrefix;
    static cMariaDb *_pInstance;
};

class cMyRec;
class cMyRecFieldRef;
class cMyRecFieldConstRef;
template <class T> class tRecordList;
class cMyRecAny;
class cColStaticMyDescrBool;

/* ******************************************************************************************************
   *                                         cColEnumType                                               *
   ******************************************************************************************************/

// A GLPI nem használja az enum típusokat, kihagyva ...

/* ******************************************************************************************************
   *                                         cColStaticMyDescr                                            *
   ******************************************************************************************************/
class cMyRecStaticDescr;

/*!
@class cColStaticMyDescr
@brief 
SQL tábla mező tulajdonságait tartalmazó objektum.

Az osztály írja le a cMyRecStaticDescr osztályban (annak részeként) a mezők tulajdonságait, ill. a virtuális metódusok végzik a mező adatkonverzióit.
Az QString ős objektum a mező nevét tartalmazza
Az alap osztály a numerikus skalár, és string típusú mezők adatkonverzióit végzi.
*/
class LV2SHARED_EXPORT cColStaticMyDescr : public cColStaticDescr {
    friend class cMyRecStaticDescr;
    friend class cMyRec;
public:
    /// Konstruktor, egy üres objektumot hoz létre
    /// @param __t Ha megadjuk a paramétert, akkor az eColType adattag ezt az értéket fogja felvenni.
    cColStaticMyDescr(const cMyRecStaticDescr *_p, int __t = FT_ANY);
    /// Copy konstruktor
    cColStaticMyDescr(const cColStaticMyDescr& __o);
    ///
    virtual ~cColStaticMyDescr();
    /// Másoló operátor
    cColStaticMyDescr& operator=(const cColStaticMyDescr __o);
    /// A mező név idézőjelek között
    virtual QString colNameQ() const;
    /// Kideríti a mező típusát a szöveges típus nevek alapján, vagyis meghatározza az eColType adattag értékét.
    virtual void typeDetect();

    /// Clone object
    /// Az eredeti osztály copy konstruktorát hívja az alapértelmezett definíció.
    virtual cColStaticMyDescr *dup() const;
    virtual QString fKeyId2name(QSqlQuery &q, qlonglong id, eEx __ex = EX_IGNORE) const;
protected:
};
TSTREAMO(cColStaticMyDescr)


/// @def CSMD_INHERITOR(T)
/// Egy cColStaticMyDescr osztályból származtatott osztály deklarációja, melyben csak a
/// virtuális metódusok lesznek módossítva.
/// A makróban a class definíció nincs lezárva a '}' karakterrel!
#define CSMD_INHERITOR(T) \
    class LV2SHARED_EXPORT T : public cColStaticMyDescr { \
    friend class cMyRecStaticDescr; \
      public: \
        T(const cColStaticMyDescr& __o) : cColStaticMyDescr(__o) { ; } \
        virtual enum cColStaticMyDescr::eValueCheck check(const QVariant& v, cColStaticMyDescr::eValueCheck acceptable = cColStaticMyDescr::VC_INVALID) const; \
        virtual QVariant  fromSql(const QVariant& __f) const; \
        virtual QVariant  toSql(const QVariant& __f) const; \
        virtual QVariant  set(const QVariant& _f, qlonglong &rst) const; \
        virtual QString   toName(const QVariant& _f) const; \
        virtual qlonglong toId(const QVariant& _f) const; \
        virtual cColStaticMyDescr *dup() const;


CSMD_INHERITOR(cColStaticMyDescrBool)
QString toView(QSqlQuery&, const QVariant& _f) const;
};

/// @class cColStaticMyDescrDate
/// Az ős cColStaticMyDescr osztályt dátum konverziós függvényeivel egészíti ki.
CSMD_INHERITOR(cColStaticMyDescrDate)
};

/// @class cColStaticMyDescrTime
/// Az ós cColStaticMyDescr osztályt idő konverziós függvényeivel egészíti ki.
CSMD_INHERITOR(cColStaticMyDescrTime)
};

/// @class cColStaticMyDescrDateTime
/// Az ős cColStaticMyDescr osztályt dátum és idő (timestamp) konverziós függvényeivel egészíti ki.
/// Mindig QDateTime típusban tárolódik kivéve a NULL, és a NOW, ez utóbbi QString.
CSMD_INHERITOR(cColStaticMyDescrDateTime)
};

/* ******************************************************************************************************
   *                                         cMyRecStaticDescr                                            *
   ******************************************************************************************************/

/// Egy rekord tulajdonságait leíró objektum. A rekord adat objektumban a pointere egy statikus adattag (kivéve cMyRecAny)
/// Az összes létrehozott objektum a statikus _recDescrMap konténerben a tábla OID szerint van elhelyezve.
/// Így egy táblához csak egy objektum készül, és csak akkor ha hivatkozás történik rá.
/// Nincs több szál kezelés!
class LV2SHARED_EXPORT cMyRecStaticDescr : public cRecStaticDescr {
    friend class cMyRec;
protected:
    /// Az eddig létrehozott objektumok konténere, a tábla OID-vel kulcsolva
    static QMap<QString, cMyRecStaticDescr *>   _recMyDescrMap;
    /// Inicializálja az objektumot. Beolvassa a tábla tulajdonságait.
    /// @exception cError* Hiba esetén dob egy kizárást
    /// @param _t SQL tábla neve
    void _setting(const QString& __t);
public:
    /// Konstruktor. A _set() metódust hívja az inicializáláshoz. Ha ez kész, akkor beteszi az objektumot a
    /// _recDescrMap konténerbe, az addMap() metódus hívásával.
    cMyRecStaticDescr(const QString& __t);
    /// Destruktor
    ~cMyRecStaticDescr();
    /// Egy objektum "beszerzése". Ha a _recDescrMap konténerben megtalálja az objektumot, akkor a pointerével tér vissza.
    /// Ha nem, akkor létrehozza, inicializálja, beteszi a _recDescCache konténerbe, és a pointerrel tér vissza.
    /// @param _t SQL tábla neve
    /// @param find_only Ha true, akkor csak a konténerben keres, na nem találja az objektumot, akkor NULL-al tér vissza.
    /// @exception cError* Ha _setReCallCnt értéke nagyobb vagy egyenlő mint 10, és objektumot kellene kreálni, akkor dob egy kizárást.
    ///     Ha find_only értéke false és nem tudja létrehozni az objektumot, akkor szintén kizárást dob.
    static const cMyRecStaticDescr *get(const QString& _t, bool find_only = false);
    /// Nincs implementálva, kizárást dob
    virtual QString checkId2Name(QSqlQuery& q) const;
    /// Nincs implementálva, kizárást dob
    static QString checkId2Name(QSqlQuery &q, const QString& _tn, const QString& _sn, enum eEx __ex = EX_ERROR);
    /// Az index alapján vissza adja az oszlop leíró referenciáját.
    const cColStaticMyDescr& colMyDescr(int i) const { return dynamic_cast<const cColStaticMyDescr&>(_columnDescrs[i]);}
    /// Az név alapján vissza adja az oszlop leíró referenciáját.
    const cColStaticMyDescr& colMyDescr(const QString& __fn) const
    { return dynamic_cast<const cColStaticMyDescr&>(_columnDescrs[toIndex(__fn)]); }
    /// A megadott indexű mező nevével tér vissza. Hasonló a _columnName() metódushoz, de a visszaadott nevet idézőjelek közé teszi
    QString columnNameQ(int i) const                 { return quoted(columnName(i), QChar('`')); }
    /// A tábla teljes nevével tér vissza, amit ha a séma név nem a "public" kiegészít azzal is, a tábla és séma név nincs idézőjelbe rakva.
    QString fullTableName() const                   { return mCat(_schemaName, _tableName); }
    /// A view tábla teljes nevével tér vissza, amit ha a séma név nem a "public" kiegészít azzal is, a tábla és séma név nincs idézőjelbe rakva.
    /// Lásd még a _viewName adattagot.
    QString fullViewName() const                    { return mCat(_schemaName, _viewName); }
    /// A teljes mező névvel (tábla és ha szükséges a séma névvel kiegészített) tér vissza. a nevek nincsenek idézőjelbe rakva.
    /// @param _i A mező indexe
    QString fullColumnName(int i) const{ return mCat(fullTableName(), columnName(i)); }
    /// A teljes mező névvel (tábla és ha szükséges a séma névvel kiegészített) tér vissza. a nevek nincsenek idézőjelbe rakva.
    /// @param _c (rövid) mező név
    QString fullColumnName(const QString& _c) const{ return mCat(fullTableName(), _c); }
    /// A tábla teljes nevével (ha szükséges a séma névvel kiegészített) tér vissza, a tábla és séma név idézőjelbe van rakva.
    QString fullTableNameQ() const                   { return mCat(quoted(_schemaName, QChar('`')), quoted(_tableName, QChar('`'))); }
    /// A view tábla teljes nevével (ha szükséges a séma névvel kiegészített) tér vissza, a tábla és séma név idézőjelbe van rakva.
    /// Lásd még a _viewName adattagot.
    QString fullViewNameQ() const                   { return mCat(quoted(_schemaName, QChar('`')), quoted(_viewName, QChar('`'))); }
    /// A teljes mező névvel (tábla és ha sükséges a séma névvel kiegészített) tér vissza. a nevek idézőjelbe vannak rakva.
    /// @param i a mező indexe
    QString fullColumnNameQ(int i) const             { return mCat(fullTableNameQ(), columnNameQ(i)); }
    /// A teljes mező névvel (tábla és ha szükséges a séma névvel kiegészített) tér vissza. a nevek idézőjelbe vannak rakva.
    /// @param _c (rövid) mező név
    QString fullColumnNameQ(const QString& _c) const { return mCat(fullTableNameQ(), quoted(_c, QChar('`'))); }

    /// A megadott nevű mező leírójánask a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param __n a keresett mező neve.
    const cColStaticMyDescr& operator[](const QString& __n) const {
        return static_cast<const cColStaticMyDescr&>(_columnDescrs[__n]);
    }
    /// A megadott indexű mező leírójának a konstans referenciájával tér vissza.
    /// @exception cError* Ha nincs ilyen mező, akkor dob egy kizárást.
    /// @param 1 a keresett mező indexe (0,1,...).
    const cColStaticMyDescr& operator[](int __i) const {
        return static_cast<const cColStaticMyDescr&>(_columnDescrs[__i]);
    }
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id.
    /// Hiba esetén, vagy ha nincs meg a a keresett ID, és __ex értéke nem EX_IGNORE, akkor dob egy kizárást,
    /// Ha viszont __ex értéke false, és hiba van, vagy nincs ID akkor NULL_ID-vel tér vissza.
    qlonglong getIdByName(QSqlQuery& __q, const QString& __n, enum eEx __ex = EX_ERROR) const;
    /// A név alapján visszaadja a rekord ID-t, feltéve, ha van név és id mező, egyébként dob egy kizárást.
    /// Hiba esetén, vagy ha nincs meg a a keresett ID, és __ex értéke nem EX_IGNORE, akkor dob egy kizárást,
    /// Ha viszont __ex értéke false, és hiba van, vagy nincs ID akkor NULL_ID-vel tér vissza.
    qlonglong getIdByName( const QString& __n, enum eEx __ex = EX_ERROR) const
        { QSqlQuery *pq = cMariaDb::newQuery(); qlonglong id = getIdByName(*pq, __n, __ex); delete pq; return id; }
    /// A ID alapján visszaadja a rekord név mező értékét, feltéve, ha van név és id mező, egyébként dob egy kizárást,
    /// ha __ex értéke nem EX_IGNORE.
    QString getNameById(QSqlQuery& __q, qlonglong __id, eEx ex = EX_ERROR) const;
    /// A ID alapján visszaadja a rekord név mező értékét, feltéve, ha van név és id mező, egyébként dob egy kizárást,
    /// ha __ex nem EX_IGNORE
    QString getNameById(qlonglong __id, enum eEx __ex = EX_ERROR) const
        { QSqlQuery *pq = cMariaDb::newQuery(); QString n = getNameById(*pq, __id, __ex); delete pq; return n; }
};
TSTREAMO(cMyRecStaticDescr)


/* ******************************************************************************************************
   *                                              cMyRec                                               *
   ******************************************************************************************************/
/*!
@class cMyRec
 */

class LV2SHARED_EXPORT cMyRec : public cRecord {
    friend class cRecMyAny;
    friend class cMyRecFieldConstRef;
    friend class cMyRecFieldRef;
    Q_OBJECT
public:
    /// Konstruktor.
    cMyRec();
    /// Destruktor
    ~cMyRec();
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
    /// Feltételezve, hogy a megadott indexű mező egy MAC, annak értékével tér vissza.
    /// \param __i  A mező indexe
    /// \param __ex Nem használt
    cMac    getMac(int __i, enum eEx = EX_ERROR) const;
    /// \brief Egy MAC cím típusú mező értékének a beállítása
    /// \param __i A mező indexe
    /// \param __a A MAC cím, a mező új értéke.
    /// \param __ex Nem használt
    cRecord& setMac(int __i, const cMac& __a, enum eEx = EX_ERROR);
    /// \brief Feltételezve, hogy a mező típusa IP cím, beállítja a mező értékét.
    /// \param __i  A mező indexe
    /// \param __a  A neállítandó IP cím
    /// \param __ex Nem használt
    /// \return Az objektum referenciával tér vissza
    cRecord &setIp(int __i, const QHostAddress& __a, enum eEx = EX_ERROR);
    ///
    QStringList getStringList(int __i, enum eEx = EX_ERROR) const;
    cRecord& setStringList(int __i, const QStringList& __v, enum eEx = EX_ERROR);

    /// Nemtámogatott.
    /// Ha __ex értéke nem EX_IGNORE, akkor kizárást dob, vagy NULL_ID-vel tér vissza.
    qlonglong fetchTableOId(QSqlQuery&, enum eEx __ex = EX_ERROR) const;
    /// Ellenörzi, hogy a konverzió elvégezhető-e. Csak az eredeti típusra konvertálás lehetséges, mivel nincs öröklés.
    template <class T> int chkMyObjType(enum eEx __ex = EX_ERROR) const {
        T o;
        if (typeid(T) == typeid(cMyRecAny) || typeid(*this) == typeid(cMyRecAny)) {
            if (__ex >= EX_ERROR) EXCEPTION(EDATA, 0, QString(QObject::tr("The object type can not be converted, %1 -> %2, but only one type is cMyRecAny").arg(descr().tableName()).arg(o.descr().tableName())));
            return -1;
        }
        if (descr().tableName() == o.descr().tableName()) return 0;  // Azonos
        if (__ex >= EX_ERROR) EXCEPTION(EDATA, 0, QString(QObject::tr("The object type can not be converted, %1 ~ %2").arg(descr().tableName()).arg(o.descr().tableName())));
        return -1;
    }
    /// Az objektum pointert visszaalakítja az eredeti ill. megadott típusba. Lást még a chkMyObjType<T>() -t.
    /// Ha az eredeti, és a megadott típus nem eggyezik, vagy az eredeti típusnak nem őse a megadott típus, akkor dob egy kizárást
    template <class T> T * reconvertMy() { chkMyObjType<T>(); return dynamic_cast<T *>(this); }
    /// Az objektum pointert visszaalakítja az eredeti ill. megadott típusba. Lást még a chkObjType<T>() -t.
    /// Ha az eredeti, és a megadott típus nem eggyezik, vagy az eredeti típusnak nem őse a megadott típus, akkor dob egy kizárást
    template <class T> const T * creconvert() const { chkObjType<T>(); return dynamic_cast<const T *>(this); }
    /// Megvizsgálja, hogy a megadott indexű bit a likeMask-ban milyen értékű, és azzal tér vissza, ha nincs ilyen elem, akkor false-val.
    bool _isLike(int __ix) const { return __ix < 0 || _likeMask.size() <= __ix ? false : _likeMask[__ix]; }
protected:
    /// Copy constructor. Nem támogatott konstruktor.
    cMyRec(const cMyRec& o) = delete;
};
TSTREAMO(cMyRec)

/// A rekord leíró objektum pointer inicializálása.
/// Ha a _pRecordDescr adattag NULL, akkor a cMyRecStaticDescr::get(const QString&) hívással
/// kér egyet, és ez lessz _pRecordDescr új értéke, és ekkor true-val tér vissza.
/// Ha a _pRecordDescr adattag nem NULL volt, akkor visszatér egy false értékkel.
/// Az R osztály a cMyRec egy leszármazottja kell legyen, és rendelkeznie kell egy
/// _pRecordDescr (cMyRecStaticDescr * típusú) adattaggal.
/// @relates cMyRec
/// @return Ha inicializálni kellett a pointert, akkor true.
template <class R> bool initPMyDescr(const QString& _tn)
{
    if (R::_pRecordDescr == nullptr) {
        R::_pRecordDescr = cMyRecStaticDescr::get(_tn);
        return true;
    }
    return false;
}

/// A statikus rekord leíró objektum pointer inicializálása, ha ez még nem történt meg
/// Hívja az initPDescr<R>(const QString&, const QString&) függvényt.
/// @relates cMyRec
/// @return A leíró pointere
template <class R> const cMyRecStaticDescr *getPMyDescr(const QString& _tn)
{
    initPMyDescr<R>(_tn);
    return R::_pRecordDescr;
}

/// @def CMYRECORD(R)
/// @relates cRecord
/// Egy cRecord leszármazottban néhány feltétlenül szükséges deklaráció
/// @param R Az osztály név
#define CMYRECORD(R) \
        friend bool initPMyDescr<R>(const QString& _tn); \
        friend const cMyRecStaticDescr *getPMyDescr<R>(const QString& _tn); \
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

/// @def CMYRECCNTR(R)
/// @relates cRecord
/// Egy alapértelmezett ill. egyszerű cRecord leszármazott objektum konstruktorainak a definiciói.
/// @param R Az osztály neve, melyhez a metódusok tartoznak
#define CMYRECCNTR(R) \
    R::R()             : cMyRec() { _set(R::descr()); } \
    R::R(const R& __o) : cMyRec() { _cp(__o); } \
    R& R::clone(const cRecord& __o) { clear(); copy(__o); return *this; }

/*!
@class cMyRecAny
@brief Általános interfész osztály.

Azt hogy melyik adattáblát kezeli azt a konstruktorban, vagy a setType() metódusokban kell beállítani.
Természetesen csak az alapfunkciókra képes, amik a cMyRec-ban meg lettek valósítva.
 */
class LV2SHARED_EXPORT cMyRecAny : public cMyRec {
public:
    /// Üres konstruktor. Használat elütt hivni kell valamelyik setType() metódust.
    /// Ha nincs beállítva típus, akkor minden olyan hívásra, melynek szüksége van
    /// a descriptor objektumra egy kizárást fog dobni.
    cMyRecAny();
    /// A konstruktor a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// objektumot. Az objektum adatt tartalma (mező adat konténer) üres lessz.
    /// @param _tn Az adattáble neve
    cMyRecAny(const QString& _tn);
    /// A konstruktor a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// Az objektum adatt tartalma (mező adat konténer) üres lessz.
    /// @param _p A leíró objektum pointere
    cMyRecAny(const cMyRecStaticDescr * _p);
    /// Copy konstruktor
    cMyRecAny(const cMyRecAny &__o);
    /// "Univerzális" copy (szerű) konstruktor
    cMyRecAny(const cMyRec &__o);
    /// A metódus a megadott adattáblához rendeli, ill. annak kezelésére készíti fel az
    /// objektumot. Az objektum adatt tartalma (mező adat konténer) üres lessz, ill. azt törli.
    /// @param _tn Az adattáble neve
    /// @param _sn A schema név mely az adattáblát tartalmazza. Obcionális, alapértelmezett a "public".
    cMyRecAny& setType(const QString& _tn, eEx __ex = EX_ERROR);
    /// A metódus az _pd -ban megadott, adattáblához rendeli, ill. annak kezelésére készíti fel.
    cMyRecAny& setType(const cMyRecStaticDescr *_pd);
    /// Alaphelyzetbe állítja az objektumot, mint az üres konstruktor után.
    cMyRecAny& clearType();
    /// A descriptor objektum referenciájával tér vissza.
    /// Ha nem adtuk meg a kezelendő adattáblát, akkor dob egy kizárást.
    virtual const cMyRecStaticDescr& descr() const;
    /// destruktor. (üres metódus).
    virtual ~cMyRecAny();
    /// Egy új objektum allokálása. Az új objektum örökli a hívó descriptorát.
    virtual cMyRec *newObj() const;
    /// Az objektumr klónozása
    virtual cMyRec *dup()const;
    /// Másoló operátor. Átmásolja a forrás objektum descriptor mutatóját (típusát) és a mező adatokat.
    cMyRecAny& operator=(const cMyRec& __o);
    /// Másoló operátor. Átmásolja a forrás objektum descriptor mutatóját (típusát) és a mező adatokat.
    cMyRecAny& operator=(const cMyRecAny& __o) { return *this = static_cast<const cMyRec&>(__o); }
    /// \brief A _p leíró által definiált adattáblában a megadott nevű rekordokban a megadott nevű mezőt
    /// modosítja a megadott érékre.
    /// Nem végez ellenörzést, csak azoknál a rekordoknál számolja a modosított rekordot, ahol a név alapján
    /// pontosan egy rekord lett modosítva.
    /// \param q
    /// \param _p   A táblát azonosító leíró objektum.
    /// \param _nl  A modosítandó rekordok neveinek a listája
    /// \param _fn  A modosítandó mező neve
    /// \param _v   Az új mező érték
    /// \return A modosított rekordok száma.
    static int updateFieldByNames(QSqlQuery& q, const cMyRecStaticDescr * _p, const QStringList& _nl, const QString& _fn, const QVariant& _v);
    /// \brief A _p leíró által definiált adattáblában a megadott ID-jű rekordokban a megadott nevű mezőt
    /// modosítja a megadott érékre.
    /// Nem végez ellenörzést, csak azoknál a rekordoknál számolja a modosított rekordot, ahol a név alapján
    /// pontosan egy rekord lett modosítva.
    /// \param q
    /// \param _p   A táblát azonosító leíró objektum.
    /// \param _il  A modosítandó rekordok ID-inek a listája
    /// \param _fn  A modosítandó mező neve
    /// \param _v   Az új mező érték
    /// \return A modosított rekordok száma.
    static int updateFieldByIds(QSqlQuery& q, const cMyRecStaticDescr * _p, const QList<qlonglong>& _il, const QString& _fn, const QVariant& _v);
    /// \brief A _p leíró által definiált adattáblában a megadott nevű rekordokban a megadott nevű tömb típusú mezőt
    /// modosítja úgy, hogy a megadott értékkel bővíti a tömböt.
    /// Nem végez ellenörzést, csak azoknál a rekordoknál számolja a modosított rekordot, ahol a név alapján
    /// pontosan egy rekord lett modosítva.
    /// \param q
    /// \param _p   A táblát azonosító leíró objektum.
    /// \param _nl  A modosítandó rekordok neveinek a listálya
    /// \param _fn  A mező aktuális értékét ezzel az értékkel bővíti.
    /// \param _v   Az új mező érték
    /// \return A modosított rekordok száma.
    static int addValueArrayFieldByNames(QSqlQuery& q, const cMyRecStaticDescr * _p, const QStringList& _nl, const QString& _fn, const QVariant& _v);
protected:
    /// Az objektum viselkedését meghatározó leíró objektum pointere.
    const cMyRecStaticDescr *pStaticDescr;
};

#endif // LV2MARIADB_H
