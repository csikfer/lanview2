#ifndef LV2DATA_H
#define LV2DATA_H

#include "lv2datab.h"
#include "lv2cont.h"
#include "qsnmp.h"

/*!
@file lv2data.h
Az adatbázis interfész objektumok.
*/

/// @enum eReasons
/// @brief Okok ill. műveletek eredményei
enum eReasons {
    R_INVALID = -1, ///< Csak hiba jelzésre
    R_NEW = 0,      ///< Új elem
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
    R_ERROR         ///< Egyébb hiba
};

int reasons(const QString& _r, bool __ex = true);
const QString& reasons(int _r, bool __ex = true);

/// @def CHKENUM
/// @brief Egy enumerációs típus konverziós függvényeinek az ellenörzése.
///
/// Makró, a chkEnum() hívására egy cRecord leszármazot egy metódusában, pl. a statikus rekord leíró inicializálása után.
/// A rekord leíró objektum példányra mutató pointer neve _pRecordDescr.
/// A makró feltételezi, hogy a két konverziós függvény neve azonos, és csak a típusuk különböző (tE2S és tS2E)
/// Ha az enumeráció kezelés a hívás szerint hibás, akkor dob egy kizárást.
/// @param n Az enumeráció típusú mező neve, vagy indexe
/// @param f A konverziós függvény(ek) neve.
#define CHKENUM(n, f)   { \
    const cRecStaticDescr& r = *_pRecordDescr; \
    if (false == r[n].checkEnum( f, f)) \
        EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("Enumeráció %1 konverziós hiba!").arg(r[n].udtName)); \
}

/* -------------------- Enum, enum - string konverziók -------------------- */
/// @enum eSubNetType
/// @brief SubNet típusok
enum eSubNetType {
    NT_INVALID = -1,    ///< Csak hiba jelzésre
    NT_PRIMARY =  0,    ///< Elsődleges IP tartomány
    NT_SECONDARY,       ///< Másodlagos tartomány azonos fizikai hálózaton
    NT_PSEUDO,          ///< Nem valós tartomány
    NT_PRIVATE          ///< Privat (elszigetelt, nam routolt) címtartomány
};

/// Cím tartomány típus stringgel (az adatbázísban a típust reprezentáló értékkel) tér vissza, a megadott konstans alapján.
/// @param __at A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást. (NT_INVALID esetén is).
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ const QString& subNetType(int __at, bool __ex = true);
/// Cím típus konstanssal tér vissza, a megadott típus név alapján
EXT_ int subNetType(const QString& __at, bool __ex = true);

/// @enum eAddressType
/// @brief Ip cím típusok
enum eAddressType {
    AT_INVALID = -1,    ///< Csak hiba jelzésre
    AT_FIXIP   =  0,    ///< Egyedi fix IP cím
    AT_PRIVATE,         ///< Csak lokálisan használt valós cím, ütközhet bármilyen egyébb IP címmel.
    AT_EXTERNAL,        ///< Külső cím (fix cím, de nincs hozzá subnet
    AT_DYNAMIC,         ///< dinamikus IP cím
    AT_PSEUDO           ///< Egyedi nem valós IP cím, csak azonosításra
};

/// Cím típus stringgel tér vissza, a megadott konstans alapján.
/// @param __at A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ const QString& addrType(int __at, bool __ex = true);
/// Cím típus konstanssal tér vissza, a megadott név alapján
/// @param __at A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ int addrType(const QString& __at, bool __ex = true);

/// Port - VLAN összerendelés típusa
enum eVlanType {
    VT_INVALID = -1,    ///< Csak hiba jelzésre
    VT_NO      =  0,    ///< Nincs összerendelés
    VT_NOTKNOWN,        ///< Ismeretlen
    VT_FORBIDDEN,       ///< Tiltott
    VT_AUTO,            ///< automatikus
    VT_TAGGED,          ///< Tagged (802.1q)
    VT_UNTAGGED,        ///< Untagged
    VT_VIRTUAL,         ///< Virtuális interfész a VLAN-hoz létrehozva
    VT_HARD             ///< Az eszköz által nem kezelt összeremdelés
};

/// VLAN típus névvel tér vissza, a megadott konstans alapján.
/// @param __n A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ int vlanType(const QString& __n, bool __ex = true);
/// VLAN típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const QString& vlanType(int __e, bool __ex = true);

/// Port - VLAN összerendelés forrása
enum eSetType  {
    ST_INVALID = -1,    ///< Csak hiba jelzésre
    ST_AUTO,            ///< automatikus
    ST_QUERY,           ///< Egy lekérdezés eredménye
    ST_MANUAL           ///< Manuális összerendelés
};

/// port - VLAN összerendelés típus névvel tér vissza, a megadott konstans alapján.
/// @param __n A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ int  setType(const QString& __n, bool __ex = true);
/// ort - VLAN összerendelés típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const QString& setType(int __e, bool __ex = true);

/// Image types
enum eImageType {
    IT_INVALID = -1,    ///< Csak hiba jelzésre
    IT_BMP,             ///< Windows bit map
    IT_GIF,             ///< GIF
    IT_JPG,             ///< Jpeg
    IT_JPEG,            ///< Jpeg
    IT_PNG,             ///< PNG
    IT_PBM,
    IT_PGM,
    IT_PPM,
    IT_XBM,
    IT_XPM,
    IT_BIN              ///< Other binary
};

/// kép típus névvel tér vissza, a megadott konstans alapján.
/// @param __n A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ int  imageType(const QString& __n, bool __ex = true);
/// Kép típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const QString&   imageType(int __e, bool __ex = true);
/// Kép típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név. Egyes Qt függvények ezt a típust vátják.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const char *    _imageType(int __e, bool __ex = true);

/// @enum eParamType
/// Paraméter adattípus konstansok
enum eParamType {
    PT_INVALID = -1,        ///< Nem valós típus, csak hibajelzésre
    PT_BOOLEAN,             ///< boolean típus
    PT_BIGINT,              ///< 8bype egész szám
    PT_DOUBLE_PRECISION,    ///< duplapontosságú lebegőpontos szám
    PT_TEXT,                ///< szöveg
    PT_INTERVAL,            ///< idő intervallum
    PT_DATE,                ///< dátum
    PT_TIME,                ///< időpont (egy napon bellül)
    PT_TIMESTAMP,           ///< időpont dátummal
    PT_INET,                ///< Hálózati cím, vagy cím tartomány
    PT_BYTEA                ///< bináris adat
};

/// Paraméter típus név konverzió
/// @param __n A paraméter típus neve (SQL enumerációs érték)
/// @param __ex Ha értéke true, és nem valós típusnevet adtunk meg, akkor dob egy kizárást.
/// @return A típus konstanssal tér vissza, ha nincs ilyen típus, és __ex értéke false, akkor a PT_INVALID konstanssal.
EXT_ int paramType(const QString& __n, bool __ex = true);
/// Paraméter típus név konverzió
/// @param __e A paraméter típus konstans
/// @param __ex Ha értéke true, és nem valós típus konstanst adtunk meg, akkor dob egy kizárást.
/// @return A típus névvel tér vissza, ha nincs ilyen típus, és __ex értéke false, akkor egy üres stringgel.
EXT_ const QString& paramType(int __e, bool __ex = true);

/// @enum eNodeType
/// Hálózati elemek típus azonosítók (set)
enum eNodeType {
    // NT_INVALID = -1,
    NT_NODE = 0,
    NT_HOST,
    NT_SWITCH,
    NT_HUB,
    NT_VIRTUAL,
    NT_SNMP
};

/// Node típus név konverzió
/// @param __n A node típus neve (SQL enumerációs érték)
/// @param __ex Ha értéke true, és nem valós típusnevet adtunk meg, akkor dob egy kizárást.
/// @return A típus konstanssal tér vissza, ha nincs ilyen típus, és __ex értéke false, akkor a NT_INVALID konstanssal.
EXT_ int nodeType(const QString& __n, bool __ex = true);
/// Paraméter típus név konverzió
/// @param __e A node típus konstans
/// @param __ex Ha értéke true, és nem valós típus konstanst adtunk meg, akkor dob egy kizárást.
/// @return A típus névvel tér vissza, ha nincs ilyen típus, és __ex értéke false, akkor egy üres stringgel.
EXT_ const QString& nodeType(int __e, bool __ex = true);

/* ------------------------------------------------------------------ */
/// Paraméter típus leíró rekord
class LV2SHARED_EXPORT cParamType : public cRecord {
    CRECORD(cParamType);
public:
    /// Egy új port_params rekord beinzertálása (paraméter név/típus/dimenzió objektumlétrehozása).
    /// Hiba esetén, ha __ex igaz (vagy nincs megadva), akkor dob egy kizárást,
    /// @param q Az adatbázis művelethet használt objektum
    /// @param __n A paraméter neve
    /// @param __de Egy megjegyzés a paraméter típushoz
    /// @param __t A paraméter adat típusa
    /// @param __di A paraméter dimenziója, opcionális
    /// @return Az új rekord azonisítója (ID), hiba esetén, ha ::ex hamis volt, akkor NULL_ID-vel tér vissza.
    static qlonglong insertNew(QSqlQuery &q, const QString& __n, const QString& __de, const QString __t, const QString __di = QString(), bool __ex = true);
    /// Egy új port_params rekord beinzertálása (paraméter név/típus/dimenzió objektumlétrehozása).
    /// Hiba esetén, ha __ex igaz (vagy nincs megadva), akkor dob egy kizárást,
    /// @param q Az adatbázis művelethet használt objektum
    /// @param __n A paraméter neve
    /// @param __de Egy megjegyzés a paraméter típushoz
    /// @param __t A paraméter adat típusa (lsd.: eParamType).
    /// @param __di A paraméter dimenziója, opcionális
    /// @return Az új rekord azonisítója (ID), hiba esetén, ha ::ex hamis volt, akkor NULL_ID-vel tér vissza.
    static qlonglong insertNew(QSqlQuery &q, const QString& __n, const QString& __de, int __t, const QString __di = QString(), bool __ex = true);
    /// Egy új port_params rekord beinzertálása (paraméter név/típus/dimenzió objektumlétrehozása).
    /// Hiba esetén, ha __ex igaz (vagy nincs megadva), akkor dob egy kizárást,
    /// @param __n A paraméter neve
    /// @param __de Egy megjegyzés a paraméter típushoz
    /// @param __t A paraméter adat típusa
    /// @param __di A paraméter dimenziója, opcionális
    /// @return Az új rekord azonisítója (ID), hiba esetén, ha ::ex hamis volt, akkor NULL_ID-vel tér vissza.
    static qlonglong insertNew(const QString& __n, const QString& __de, const QString __t, const QString __di = QString(), bool __ex = true) {
        QSqlQuery q = getQuery();
        return insertNew(q, __n, __de, __t, __di, __ex);
    }
    /// Egy új port_params rekord beinzertálása (paraméter név/típus/dimenzió objektumlétrehozása).
    /// Hiba esetén, ha __ex igaz (vagy nincs megadva), akkor dob egy kizárást,
    /// @param __n A paraméter neve
    /// @param __de Egy megjegyzés a paraméter típushoz
    /// @param __t A paraméter adat típusa (lsd.: eParamType).
    /// @param __di A paraméter dimenziója, opcionális
    /// @return Az új rekord azonisítója (ID), hiba esetén, ha ::ex hamis volt, akkor NULL_ID-vel tér vissza.
    static qlonglong insertNew(const QString& __n, const QString& __de, int __t, const QString __di = QString(), bool __ex = true) {
        QSqlQuery q = getQuery();
        return insertNew(q, __n, __de, __t, __di, __ex);
    }
    ///
    static QString paramToString(eParamType __t, const QVariant& __v, bool __ex = true);
    ///
    static QVariant paramFromString(eParamType __t, QString& __v, bool __ex = true);
};

class LV2SHARED_EXPORT cSysParam  : public cRecord {
    CRECORD(cSysParam);
public:
    /// Törli a paramType adattagot.
    virtual void    clearToEnd();
    /// A toEnd(int i) metódust hívja a port paraméter (típus) rekord id mező indexével.
    virtual void 	toEnd ();
    /// Ha megváltozik a port param (típus) id, akkor betölti, vagy törli a megfelelp értéket a paramType adattagba.
    /// Nincs ilyen id-vel port_params rekord (és nem NULL az id), akkor a statusban bebillenti az ES_DEFECZIVE bitet.
    virtual bool 	toEnd (int i);
    /// A paraméter adat típus enumerációs értékkel tér vissza
    qlonglong valueType()   const { return paramType.getId(_sParamTypeType); }
    /// A paraméter adat típus névvel tér vissza
    const QString& valueTypeName(bool __ex = true) const {return ::paramType(valueType(), __ex); }
    /// A paraméter dimenzió ill. mértékegység nevével tér vissza
    QString valueDim()    const { return paramType.getName(__sParamTypeDim); }
    /// A paraméter értékkel tér vissza
    QVariant value(bool __ex = true) const;
    /// A paraméter típusának a beállítása
    /// @param __id A paraméter típus rekord ID-je
    cSysParam& setType(qlonglong __id)        { set(_ixParamTypeId, __id); return *this; }
    /// @param __n A paraméter típus rekord név mezőjenek az értéke
    cSysParam& setType(const QString& __n)    { paramType.fetchByName(__n); _set(_ixParamTypeId, paramType.getId()); return *this; }
    /// Értékadás az érték mezőnek.
    cSysParam& setValue(const QVariant& __v, bool __ex = true);
    /// Értékadás az érték mezőnek.
    cSysParam& operator=(const QVariant& __v) { return setValue(__v); }
    static enum eReasons setTextSysParam(QSqlQuery& _q, const QString& __nm, const QString& _val, const QString& _tn = _sText) {
        execSqlFunction(_q, "set_text_sys_param", QVariant(__nm), QVariant(_val), QVariant(_tn));
        return (enum eReasons)reasons(_q.value(0).toString());
    }
    static enum eReasons setBoolSysParam(QSqlQuery& _q, const QString& __nm, bool _val, const QString& _tn = _sText) {
        execSqlFunction(_q, "set_bool_sys_param", QVariant(__nm), QVariant(_val), QVariant(_tn));
        return (enum eReasons)reasons(_q.value(0).toString());
    }
    static enum eReasons setIntSysParam(QSqlQuery& _q, const QString& __nm, qlonglong _val, const QString& _tn = _sText) {
        execSqlFunction(_q, "set_int_sys_param", QVariant(__nm), QVariant(_val), QVariant(_tn));
        return (enum eReasons)reasons(_q.value(0).toString());
    }
    static enum eReasons setIntervalSysParam(QSqlQuery& _q, const QString& __nm, qlonglong _val, const QString& _tn = _sText) {
        execSqlFunction(_q, "set_interval_sys_param", QVariant(__nm), QVariant(intervalToStr(_val)), QVariant(_tn));
        return (enum eReasons)reasons(_q.value(0).toString());
    }
    static enum eReasons setSysParam(QSqlQuery& _q, const QString& __nm, const QVariant& _val, const QString& _tn, bool __ex = true) {
        cSysParam   po;
        po.setType(_tn);
        po.setValue(_val, __ex);
        enum eReasons r = setTextSysParam(_q, __nm, po.getName(_ixParamValue), _tn);
        if (__ex && r == R_NOTFOUND) EXCEPTION(EFOUND, -1, _tn);
        return r;
    }
    static QVariant getSysParam(QSqlQuery& _q, const QString& _nm, bool __ex = true) {
        cSysParam   po;
        if (po.fetchByName(_q, _nm)) {
            return po.value(__ex);
        }
        if(__ex) EXCEPTION(EFOUND, -1, _nm);
        return QVariant();
    }

protected:
    /// A port paraméter értékhez tartozó típus rekord/objektum
    cParamType  paramType;
    static int _ixParamTypeId;
    static int _ixParamValue;
};

/* ------------------------------------------------------------------ */
/*!
@class cTpow
@brief Az (heti)időintervallumok napi időintervallum elemei
*/
class LV2SHARED_EXPORT cTpow : public cRecord {
    CRECORD(cTpow);
};

/*!
@class cTimePeriod
@brief A timeperiods tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cTimePeriod : public cRecord {
    CRECORD(cTimePeriod);
};

typedef tGroup<cTimePeriod, cTpow> tTimePeriodTpow;

/* ******************************  ****************************** */

/*!
@class cImage
@brief Az images tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cImage : public cRecord {
    CRECORD(cImage);
public:
    /// Betölt az objektumba egy képet
    /// @param __fn A kép fájl neve
    bool load(const QString& __fn, bool __ex = true);
    /// Kiír egy képet egy fájlba
    /// @param __fn A kép fájl neve
    bool save(const QString& __fn, bool __ex = true);
    /// A bianaris adatot adja vissza
    QByteArray getImage() const     { return get(_ixImageData).toByteArray(); }
    /// A bináris adattartalmat állítja be
    void setImage(QByteArray __a)   { set(_ixImageData, QVariant(__a)); }
    /// Az kép típusának a nevét adja vissza
    const char * getType() const    { return _imageType(getId(_ixImageType)); }
protected:
    static int  _ixImageData;
    static int  _ixImageType;
};

/*!
@class cPlace
@brief A places tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cPlace : public cRecord {
    CRECORD(cPlace);
public:
    qlonglong parentImageId(QSqlQuery& q);
};

/*!
@class cPlaceGroup
@brief A places tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cPlaceGroup : public cRecord {
    CRECORD(cPlaceGroup);
public:
    /// Egy új rekord beszúrása.
    /// @param __n A név mező értéke
    /// @param __d A descr mező értéke
    /// @return Az új rekord id-je.
    static qlonglong insertNew(const QString& __n, const QString& __d);
};

typedef tGroup<cPlaceGroup, cPlace> cGroupPlace;

/* ******************************  ****************************** */

/*!
@class cSubNet
@brief A subnets tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cSubNet : public cRecord {
    CRECORD(cSubNet);
public:
    /// További műveletek az objektum rekord kitöltése után:
    /// A toEnd(int _i) metódust hívja a "vlan_id" idexével.
    virtual void toEnd();
    /// További műveletek az egy mező kitöltése után:
    /// Ha a "vlan_id" idexével hívják, akkor ellenörzi a VLAN ID-t
    /// A negatív vagy 0 értékett NULL-ra állírja.
    virtual bool toEnd(int _i);
    /// A "netaddr" mezőt állítja a megadott címbe.
    cSubNet& operator=(const netAddress& __a) { set(_ixNetAddr, QVariant::fromValue(__a)); return *this; }
    /// Az addr adattag aktuális értékével tér vissza (a NULL ??)
    const netAddress address() const { return get(_ixNetAddr).value<netAddress>(); }
    /// Egy új rekord beillesztése a subnets táblába, a megadott paraméterek alapján.
    /// Hiba esetén dob egy kizárást
    /// @param __n Az új szubnet neve (subnets rekord subnet_name mező)
    /// @param __d Az új szubnet subnet_descr mezőjének a tartalma
    /// @param __a Az új rekordban a szubnet cím (subnets rekord netaddress mező)
    /// @param _v VLAN id
    /// @param __t Az új rekordban az subnet_type értéke. Enum : ("primary","secundary","pseudo")
    /// @return A beillesztett új rekord ID-je.
    static qlonglong insertNew(const QString& __n, const QString& __d, const netAddress& __a, qlonglong _v, const QString& __t);
    /// Rekord beolvasása ip cím alapján.
    /// @param __addr IP cím, melyet tartalmaznia kell a keresett subnet-nek.
    /// @return A feltételnek megfelelő rekordok száma
    int getByAddress(QSqlQuery & __q, const QHostAddress &__addr);
protected:
    /// A netaddr nevű mező indexe. Mivel nem öröklődik a tábla, ezért lehet statikus.
    static int              _ixNetAddr;
    static int              _ixVlanId;
    static int              _ixSubNetType;
};

/*!
@class cVLan
@brief A vlans tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cVLan : public cRecord {
    CRECORD(cVLan);
public:
    /// Egy új rekord beillesztése a vlans táblába, a megadott paraméterek alapján.
    /// Hiba esetén dob egy kizárást
    /// @param __i A VLAN id
    /// @param __n Az új vlan neve
    /// @param __d Az új vlan description
    /// @param __s Az új vlan statusa
    /// @return Az beillesztett új rekord ID-je.
    static long insertNew(long _i, const QString& __n, const QString& __d, bool _s);
};

/* ******************************  ****************************** */
/*!
@class cIpAddress
@brief A ipaddresses tábla egy rekordját reprezentáló osztály.
Az objektum automatikusan kezeli a subnet_id mezőt (és subNet adattagot). Vagyis az IP cím megadásánál
kitölti azt is, ill. ha ez nem lehetséges, akkor törli azt.
*/
class LV2SHARED_EXPORT cIpAddress : public virtual cRecord {
    friend class cInterface;
    CRECORD(cIpAddress);
protected:
    /// Konstruktor
    /// Nem inicializálja a belső adattagjait, feltételezi, hogy egy leszármazott konstruktorból hívják
    /// @param __dummy A paraméter csak jelzés értékű, a paraméter nélküli konstruktor már definiálva van/foglalt.
    explicit cIpAddress(no_init_& __dummy) : cRecord()
            { (void)__dummy; _ixSubNetId = _ixAddress = _ixIpAddressType = -1; cIpAddress::descr(); }
public:
    /// Konstruktor
    /// Kitölti a paraméter alapján a cím és típus nezőt
    cIpAddress(const QHostAddress& __a, const QString& __t = _sFixIp);
    /// Érték adás a cím mezőnek
    cIpAddress& operator=(const QHostAddress& __a) { set(_ixAddress, QVariant::fromValue(__a)); return *this; }
    /// Érték adás a cím mezőnek, és a típus mezőnek.
    cIpAddress& setAddress(const QHostAddress& __a, const QString& __t = _sNul);
    /// A cím mező értékének a lekérése.
    QHostAddress address() const;
    /// Ha nincs subnet a megadott címhez, akkor a típust 'external'-ra állítja
    /// Ha nincs valós cím az objektumban akkor nem csinál semmit.
    /// @return true, ha a típust 'external'-ba állította, egyébként false.
    bool thisIsExternal(QSqlQuery &q);
    static QList<QHostAddress> lookupAll(const QString& hn, bool __ex = true);
    static QString lookup(const QHostAddress& ha, bool __ex = true);
    static QHostAddress lookup(const QString& hn, bool __ex = true);
    QHostAddress setIpByName(const QString& _hn, const QString &_t = _sNul, bool __ex = true);
protected:
    static int _ixPortId;
    static int _ixAddress;
    static int _ixSubNetId;
    static int _ixIpAddressType;
};

typedef tRecordList<cIpAddress> cIpAddresses;
/* ******************************  ****************************** */

class cPortParams;
class cNPort;

/// Port paraméter érték
class LV2SHARED_EXPORT cPortParam : public cRecord {
    friend class cPortParams;
    friend class cNPort;
    friend class cInterface;
    CRECORD(cPortParam);
public:
    /// Törli a paramType adattagot.
    virtual void    clearToEnd();
    /// A toEnd(int i) metódust hívja a port paraméter (típus) rekord id mező indexével.
    virtual void 	toEnd ();
    /// Ha megváltozik a port param (típus) id, akkor betölti, vagy törli a megfelelp értéket a paramType adattagba.
    /// Nincs ilyen id-vel port_params rekord (és nem NULL az id), akkor a statusban bebillenti az ES_DEFECZIVE bitet.
    virtual bool 	toEnd (int i);
    /// A port paraméter nevével tér vissza
    QString name()   const { return paramType.getName(); }
    /// A port paraméter típus enumerációs értékkel tér vissza
    qlonglong type()   const { return paramType.getId(_sParamTypeType); }
    /// A port paraméter dimenzió ill. mértékegység nevével tér vissza
    QString dim()    const { return paramType.getName(__sParamTypeDim); }
    /// A port paraméter értékére mutató referencia objektummal tér vissza.
    cRecordFieldConstRef value() const { return (*this)[_sParamValue]; }
    /// A port paraméter értékére mutató referencia objektummal tér vissza.
    cRecordFieldRef      value()       { return (*this)[_sParamValue]; }
    /// A paraméter típusának a beállítása
    /// @param __id A paraméter típus rekord ID-je
    cPortParam& setType(qlonglong __id)        { set(_ixParamTypeId, __id); return *this; }
    /// @param __n A paraméter típus rekord név mezőjenek az értéke
    cPortParam& setType(const QString& __n)    { paramType.fetchByName(__n); _set(_ixParamTypeId, paramType.getId()); return *this; }
    /// Értékadás az érték mezőnek.
    cPortParam& operator=(const QString& __v)  { setName(_sParamValue, __v); return *this; }
protected:
    /// A port paraméter értékhez tartozó típus rekord/objektum
    cParamType  paramType;
    static int _ixParamTypeId;
    static int _ixPortId;
};

/// Port paraméter értékek listája
class LV2SHARED_EXPORT cPortParams : public tRecordList<cPortParam> {
public:
    /// Konstruktor. Üres listát készít.
    cPortParams();
    /// Konstruktor. A listának egy eleme lessz, a megadott objektum klónja.
    cPortParams(const cPortParam& __v);
    /// Konstruktor. A listét feltölti az adatbázisból, hogy a megadott porthoz (ID) tartozó összes paramétert tartalmazza.
    cPortParams(QSqlQuery& __q, qlonglong __port_id);
    /// Copy konstrultor. A konténer összes elemét klónozza.
    cPortParams(const cPortParams& __o);
    /// Másoló operátor. A konténer összes elemét klónozza.
    cPortParams& operator=(const cPortParams& __o);
    /// A listét feltölti az adatbázisból, hogy a megadott porthoz (ID) tartozó összes paramétert tartalmazza.
    int fetch(QSqlQuery& __q, qlonglong __port_id) {
        if (cPortParam::_ixPortId < 0) cPortParam();    // Ha még nem volt init.
        PDEB(VVERBOSE) << "Call: tRecordList<cPortParam>::fetch(__q, false, " << cPortParam::_ixPortId << _sCommaSp << __port_id << QChar(')') << endl;
        return tRecordList<cPortParam>::fetch(__q, false, cPortParam::_ixPortId, __port_id);
    }
    /// Index operátor: egy elem a paraméter név alapján
    const cPortParam& operator[](const QString& __n) const;
    /// Index operátor: egy elem a paraméter név alapján
    cPortParam&       operator[](const QString& __n);
    /// Az összes elem kiírása az adatbázisba.
    /// @return A kiírt rekordok száma
    int       insert(QSqlQuery &__q, qlonglong __port_id, bool __ex = true);
};
/* ------------------------------------------------------------------------ */
class LV2SHARED_EXPORT cIfType : public cRecord {
    CRECORD(cIfType);
protected:
    /// Az összes if_types rekordot tartalmazó konténer
    /// Nem frissül automatikusan, ha változik az adattábla.
    static tRecordList<cIfType> ifTypes;
    /// Egy üres objektumra mutató pointer.
    /// Az ifTypes feltöltésekor hozza létre az abjektumot a fetchIfTypes();
    static cIfType *pNull;
public:
    /// Feltölti az adatbázisból az ifTypes adattagot, elötte törli.
    static void fetchIfTypes(QSqlQuery& __q);
    /// Egy ifTypes objektumot ad vissza a név alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    static const cIfType& ifType(const QString& __nm, bool __ex = true);
    /// Egy ifTypes objektumot ad vissza az ID alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    static const cIfType& ifType(qlonglong __id, bool __ex = true);
    /// Egy ifTypes objektum Azonosítóját adja vissza a név alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    static qlonglong ifTypeId(const QString& __nm, bool __ex = true) { return ifType(__nm, __ex).getId(); }
    /// Egy ifTypes objektum nevét adja vissza az ID alapján, ha nincs ilyen azonosítójú típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    static QString   ifTypeName(qlonglong __id, bool __ex = true)    { return ifType(__id, __ex).getName(); }
    /// Visszakeresi az ifTypes konténer azon elemét, melynek az iftype_iana_id értéke megeggyezik a
    /// a paraméterben magadott értékkel, és a preferred értéke true. Ha van ilyen objektum a
    /// konténerben, akkor az első pointerével, ha nincs NULL pointerrel tér vissza.
    /// Ha a talált rekordban az iana_id_link mező nem NULL, akkor annak értékével hívja a metódust rekorzívan.
    /// A metódus nem tartalmaz védelmet az esetleges végtelen rekurzióra!
    static const cIfType *fromIana(int _iana_id);
protected:
    /// Ha nincs feltöltve az ifTypes adattag , akkor feltölti az adatbázisból,
    /// Vagyis hívja a void fetchIfTypes(QSqlQuery& __q); metódust.
    static void checkIfTypes() { if (ifTypes.count() == 0) { QSqlQuery q = getQuery(); fetchIfTypes(q); } }
};

/* ======================================================================== */

/*!
@class cNPort
@brief A nports tábla egy rekordját reprezentáló osztály.
 */
class LV2SHARED_EXPORT cNPort : public cRecord {
    friend class cNode;
    friend class cSnmpDevice;
    CRECORD(cNPort);
protected:
    /// Konstruktor a leszámazottak számára.
    explicit cNPort(no_init_& __dummy) : cRecord() { __dummy = __dummy; cNPort::descr(); }
public:
    /// Törli a params konténert.
    virtual void clearToEnd();
    /// A port id indexével hívja a toEnd(int i) metódust
    virtual void toEnd();
    /// Ha a port id indexével hívtuk, akkor ha szükséges törli a params konténert, lásd az atEndCont() metódust.
    virtual bool toEnd(int i);
    /// Beszúr egy port rekordot az adatbázisba a járulékos adatokkal (rekordokkal) együtt.
    virtual bool insert(QSqlQuery &__q, bool __ex = false);

    /// Egy port rekord beolvasása a port név és a node id alapján.
    /// @param __q Az adatbázis műveletjez haszbált objektum.
    /// @param __port_name A keresett port neve
    /// @param __node_id A hálózati elem ID, melynek a portját keressük
    /// @param __iftype_id Opcionális paraméter, a port típusa
    /// @return Ha egy rekordot beolvasott akkor true. Ha nem adtunk meg a __port_type_id paramétert, ill. értéke NULL_ID,
    ///     akkor több találat is lehet, akkor beolvasásra kerül az első port adatai, és a visszaadott érték false.
    ///     Ez utobbi esetben a next() metódussal olvasható be a következő találat.
    bool fetchPortByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, qlonglong __iftype_id = NULL_ID);
    /// Egy port rekord beolvasása/objektum feltültése a port név és a node id, ill. opcionálisan a port típus alapján.
    /// @param __q Az adatbázis műveletjez haszbált objektum.
    /// @param __port_name A keresett port neve
    /// @param __node_id A hálózati elem ID, melynek a portját keressük
    /// @param __iftype_id Opcionális paraméter, a port típusa
    /// @return Az objektum referencia
    /// @exceptions Ha nincs ilyen port, vagy ha nem adtuk meg a típust, és a megadott név nen egyedi, akkor dob egy kizárást.
    cNPort& setPortByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, qlonglong __iftype_id = NULL_ID) {
        if (!fetchPortByName(__q, __port_name, __node_id, __iftype_id)) {
            QString s = __port_name;
            if (__iftype_id != NULL_ID) s += inParentheses(cIfType::ifTypeName(__iftype_id, false));
            EXCEPTION(EDATA,__node_id, s);
        }
        return *this;
    }
    /// Egy port id  lekérése a port név és a node_id alapján.
    static qlonglong getPortIdByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, qlonglong __iftype_id = NULL_ID, bool __ex = true);
    /// Egy port rekord beolvasása a port név és a node neve alapján.
    /// @param __ex Ha értéke false, és a megadott nevű node nem létezik, akkor dob egy kizárást, egyébként visszatér false-val.
    /// @return Ha egy rekordot beolvasott akkor true
    bool fetchPortByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, qlonglong __iftype_id = NULL_ID, bool __ex = true);
    /// Egy port rekord beolvasása a port név és a node neve alapján.
    /// Ha nem tudja beolvasni a rekordot, akkor dob egy kizárást.
    cNPort& setPortByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, qlonglong __iftype_id = NULL_ID) {
        if (!fetchPortByName(__q, __port_name, __node_name, __iftype_id)) {
            QString s = QString("%1:%2").arg(__node_name,__port_name);
            if (__iftype_id != NULL_ID) s += inParentheses(cIfType::ifTypeName(__iftype_id, false));
            EXCEPTION(EDATA, -1, s);
        }
        return *this;
    }
    /// Egy port id  lekérése a port név és a node név alapján.
    static qlonglong getPortIdByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, qlonglong __iftype_id = NULL_ID, bool __ex = true);
    /// Egy port rekord beolvasása a port index és a node_id alapján.
    bool fetchPortByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id);
    /// Egy port rekord beolvasása a port index és a node_id alapján.
    /// Ha nem tudja beolvasni a rekordot, akkor dob egy kizárást.
    cNPort& setPortByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id) {
        if (!fetchPortByIndex(__q, __port_index, __node_id))
            EXCEPTION(EDATA, __port_index, QString::number(__node_id));
        return *this;
    }
    /// Egy port id  lekérése a port index és a node_id alapján.
    static qlonglong getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id, bool __ex = true);
    /// Egy port rekord beolvasása a port index és a node név alapján.
    bool fetchPortByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name, bool __ex = true);
    /// Egy port rekord beolvasása a port index és a node_id alapján.
    /// Ha nem tudja beolvasni a rekordot, akkor dob egy kizárást.
    cNPort& setPortByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name) {
        if (!fetchPortByIndex(__q, __port_index, __node_name))
            EXCEPTION(EDATA, __port_index, __node_name);
        return *this;
    }
    /// Egy port id  lekérése a port index és a node_id alapján.
    static qlonglong getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name, bool ex = true);

    /// Megallokál egy új cNPort, cInterface vagy cIfaceAddr objektumot, a megadott cIfType objektum szerint.
    /// Az objektum típust a cIfType (egy adatbázisból beolvasott iftypes rekord alapján) az iftype_obj_type
    /// mező értéke adja meg.
    /// A megallokát objektumban beállítja az iftype_id mezőt.
    /// @param __t cIfType objektum, egy beolvasott iftypes rekordal.
    /// @param __i Opcionális index.
    /// @return A megallokált objektum pointere.
    static cNPort *newPortObj(const cIfType& __t);
    /// Megallokál egy új cNPort, cInterface vagy cIfaceAddr objektumot, a meghatározott cIfType objektum szerint.
    /// Ld.: static cNPort *newObject(const cIfType& __t)
    /// @param __ifTypeName Az interfész típus neve (iftypes rekord iftype:name mező értéke)
    /// @return A megallokált objektum pointere.
    static cNPort *newPortObj(const QString& __ifTypeName) {
        return newPortObj(cIfType::ifType(__ifTypeName));
    }
    static cNPort * getPortObjById(QSqlQuery& q, qlonglong __port_id, bool __ex = true);
    /// Allokál és beolvas egy port objektumot a megadott port_id alapján.
    /// Az allokált objektum típusa megfelel a megadott tábla OID-nek, ha a tábla az nports, pports vagy interfaces.
    /// @param __port_id A port rekord id-je (port_id)
    /// @param __tableoid A típus azonosító tábla OID
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cNPort * getPortObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __port_id, bool __ex);
    /// Az objektum iftpes rekordjával tér vissza. Ha az iftype_id mező null, vagy nem egy létező iftpes rekord ID-je,
    /// akkor dob egy kizárást, vagy üres objektummal tér vissza.
    /// @param __ex Hiba esetén nem üres objektummal tér vissza, hanem dob egy kizárást, ha értéke igaz (ez az alapértelmezés).
    const cIfType& ifType(bool __ex = true) const { return cIfType::ifType(getId(_sIfTypeId), __ex); }
    /// Az iftype hívással lekéri az iftypes rekordot, és összehasonlítja a megadott névvel, a rekord név mezőjét,
    /// Ha azonos true-val tér vissza.
    /// @param q Az adatbázis lekérdezéshez használt query objektum.
    /// @param __iftypename A keresett név.
    /// @param __ex Hiba esetén dob egy kizárást, ha igaz (ez az alapértelmezés). Ha false, akkor hiba esetén false-vel tér vissza,
    ///           hacsak az __iftypename nem null.
    bool isIfType(const QString& __iftypename, bool __ex = true) const { return ifType(__ex).getName() == __iftypename; }
    /// Feltölti az adatbázisból a params konténert.
    int fetchParams(QSqlQuery& q) { return params.fetch(q, getId()); q.finish(); }
    /// Egy névvel térvissza, mely alapján a port egyedileg azonosítható.
    /// A nevet a node és a port nevéből állítja össze, szeparátor a ':' karakter
    /// Az objektumnak csak a node_id mezője és a név mezője alapján állítja elő a visszaadott értéket.
    /// @param q Az adatbázis lekérdezéshez használt query objektum.
    QString getFullName(QSqlQuery& q, bool _ex = true);
    /// Port paraméterek, nincs automatikusan feltöltve
    cPortParams   params;
protected:
    /// A tulajdonos patchs rekordra mutató id mező indexe
    static int _ixNodeId;
    /// A port_index mező indexe
    static int _ixPortIndex;
    /// Port iftype_id field index
    static int _ixIfTypeId;
};

class cPatch;
/*!
@class cPPort
@brief A pports tábla egy rekordját reprezentáló osztály.

 */
class LV2SHARED_EXPORT cPPort : public cNPort {
    CRECORD(cPPort);
    friend class cPatch;
public:
    /// Beállítja a port alapértelmezett (kötelező?) típusát
    cPPort& setDefaultType() { set(_ixIfTypeId, _ifTypePatch); return *this; }
protected:
    /// Patch port iftype_id default value
    static qlonglong _ifTypePatch;
};

EXT_ int ifStatus(const QString& _n, bool __ex);
EXT_ const QString& ifStatus(int _i, bool __ex);

class cPortVlan;
/*!
@class cInterface
@brief A interfaces tábla egy rekordját reprezentáló osztály.
 */
class LV2SHARED_EXPORT cInterface : public cNPort {
    CRECORD(cInterface);
public:
    /// Interfész állapotok
    typedef enum {
        PS_INVALID = -1,    ///< Csak hibajelzésre
        UP,                 ///< SNMP interfész státusz up
        DOWN,               ///< SNMP interfész státusz down
        TESTING,            ///< SNMP interfész státusz teszt
        UNKNOWN,            ///< SNMP interfész státusz ismeretlen
        DORMANT,            ///< SNMP interfész státusz
        NOTPRESENT,         ///< SNMP interfész státusz
        LOWERLAYERDOWN,     ///< SNMP interfész státusz
        IA_INVERT,          ///< IndAlarm szenzor státusz fordított bekötés (down)
        IA_SHORT,           ///< IndAlarm szenzor státusz rövidzár (down)
        IA_BROKEN,          ///< IndAlarm szenzor státusz szakadás (down)
        IA_ERROR            ///< IndAlarm szenzor státusz hiba (down)
    }   eIfStatus;
    //
    virtual void clearToEnd();
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual bool insert(QSqlQuery &__q, bool __ex = true);
    virtual QString toString() const;
    // A trunkMembers konténer adattaghoz hozzáad egy port indexet
    void addTrunkMember(int __ix) { trunkMembers << __ix;  }
    int updateTrunkMembers(QSqlQuery& q, bool __ex);
    /// A trunkMembers adattag konstans referenciájával tér vissza.
    /// Ha a trunkMembers fel van töltve, akkor a konténer TRUNK port esetén a trunk tagjainak az indexét tartalmazza (port_index mező értéke).
    const tIntVector& getTrunkMembers() const { return trunkMembers; }
    void joinVlan(qlonglong __id, enum eVlanType __t, enum eSetType __st = ST_MANUAL);
    /// Beolvas egy objektumot/rekordot a MAC alapján
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett MAC
    /// @return a találatok száma, ha nulla, akkor az objektum üres lessz, egyébként az első rekorddal lessz feltöltve.
    int fetchByMac(QSqlQuery& q, const cMac& a);
    /// Beolvas egy objektumot/rekordot a IP cím alapján
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett cím
    /// @return true, ha beolvasott egy rekordot, ill. false, ha nem atálta a megadott címet.
    bool fetchByIp(QSqlQuery& q, const QHostAddress& a);
    /// Feltölti a vlans konténert
    int fetchVlans(QSqlQuery& q);
    /// Feltölti at addresses konténert.
    int fetchAddressess(QSqlQuery& q);
    /// Egy cím hozzáadása az interfészhez.
    /// @param __a A cím objektum (rekord)
    cIpAddress& addIpAddress(const cIpAddress& __a);
    /// Egy cím hozzáadása az interfészhez.
    /// @param __a Az ip cím.
    /// @param __t Az IP cím típusa, alapértelmezett a _sFixIp
    /// @param __d Cím rekord megjegyzés mező (alapértelmezetten üres)
    cIpAddress& addIpAddress(const QHostAddress& __a, const QString& __t = _sFixIp, const QString& __d = _sNul);
    /// Értékadása a hwaddress mezőnek.
    cInterface& operator=(const cMac& _m) { set(_sHwAddress, QVariant::fromValue(_m)); return *this; }

    /// VLAN hozzárendejések, nincs automatikus feltöltés
    tRecordList<cPortVlan> vlans;
    /// Ip címek listája (nincs automatikus feltöltés
    cIpAddresses    addresses;
protected:
    /// Trubk port esetén a trunk tagjainak az indexe (port_index mező)
    /// Nincs automatikusan feltöltve.
    tIntVector trunkMembers;
    /// A MAC cím mező indexe
    static int _ixHwAddress;
};

/* ****************************** NODES (patchs, nodes, snmphosts ****************************** */


/// @class cShareBack
/// @brief Egy hátlapi megosztást reprezentáló objektum.
class LV2SHARED_EXPORT cShareBack {
protected:
    /// A megosztott portok (hátlapi) port indexei, ha nincs a megadott megosztás, akkor NULL_IX.
    int     a, ab, b, bb;
    /// Ha AB ill. BA megosztás is van, és értéke true, akkor azok típusa C és D, egyébbként érdektelen.
    bool    cd;
public:
    /// Üres konstruktor.
    cShareBack() { a = ab = b = bb = NULL_IX; cd = false; }
    /// Konstruktor, az adattagok értékeivel. Nem adható meg két azonos port_index (csak ha az NULL_IX)
    cShareBack(int __a, int __b, int __ab = NULL_IX, int __bb = NULL_IX, bool __cd = false) { set(__a, __b, __ab, __bb, __cd); }
    /// Copy operátor. Mivel ellenörzi az értékeket az öres objektum nem másolható.
    cShareBack& operator=(const cShareBack& __o) { return set(__o.a, __o.b, __o.ab, __o.bb, __o.cd); }
    /// Az objektum feltöltése, két azonos index nem adható meg, mindegyik index nem lehet NULL_IX.
    /// Az A ill AA típusú megosztású port indexe nem lehet NULL_IX!
    cShareBack& set(int __a, int __b, int __ab = NULL_IX, int __bb = NULL_IX, bool __cd = false);
    /// Copy konstruktor. Nem ellenöriz.
    cShareBack(const cShareBack& __o)            { a = __o.a; ab = __o.ab; b = __o.b; bb = __o.bb; cd = __o.cd; }
    /// True, ha a két objektum a,b,ab,bb adattagjaiból bármelyik, bármelyikkel egyenlő, és nem NULL_IX.
    bool operator==(const cShareBack& __o) const;
    /// True, ha az objektum a,b,ab,bb adattagjaiból bármelyikel egyenlő __o -val, és nem NULL_IX.
    bool operator==(int __o) const;
    int getA() const    { return a;  }
    int getB() const    { return b;  }
    int getAB() const   { return ab; }
    int getBB() const   { return bb; }
    bool isCD() const   { return cd; }
};
inline uint qHash(const cShareBack& sh) { return (uint)qHash(sh.getA()); }

/*!
@class cPatch
@brief A patchs tábla egy rekordját reprezentáló osztály.

Patch panel, falicsatlakozó, ill. egyébb csatlakozo ill. csatlakozó csoport objektum.
 */
class LV2SHARED_EXPORT cPatch : public cRecord {
    CRECORD(cPatch);
protected:
    explicit cPatch(no_init_&) : cRecord(), ports(), pShares(NULL)  { cPatch::descr(); }
public:
    /// A létrehozott üres objektumban kitölti a port_name és port_descr mezőket.
    //! @param __name a port_name mező új értéke.
    /// @param __note a port_descr mező új értéke.
    cPatch(const QString& __name, const QString& __note = QString());
    virtual void clearToEnd();
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual bool insert(QSqlQuery &__q, bool __ex = true);
    /// Kitölti a ports adattagot, hiba esetén dob egy kizárást.
    virtual int fetchPorts(QSqlQuery& __q, bool __ex = true);

    // Csak a cPatch-ban támogatott virtualis metódusok, a cNode-ban ujraimplementált metódusok kizárást dobnak.
    virtual void clearShares();
    /// Beállít egy (hátlapi) kábel megosztást. A ports adattagot fel kell tölteni a hívás elött!
    /// @param __a    Az A vagy AA típusú megosztott port indexe (port port_index mező értéke)
    /// @param __ab   Az AB vagy C típusú megosztott port indexe, ha értéke -1, akkor __a egy A típusú megosztást jelöl
    /// @param __b    Az B vagy BA típusú megosztott port indexe (port port_index mező értéke)
    /// @param __bb   Az BB vagy D típusú megosztott port indexe, ha értéke -1, akkor __b egy B típusú megosztást jelöl
    /// @param __cd   Ha __ab vagy __bb nem negatív, és __cd értéke true, akkor __ab és __bb egy C ill. D típusú megosztást jelöl.
    /// @return true, ha beállította a megosztásokat a ports objektumon, és false, ha hiba történt.
    virtual bool setShare(int __a, int __ab = NULL_IX, int __b = NULL_IX, int __bb = NULL_IX, bool __cd = false);
    /// A megosztásokat is kiírja az adatbázisba.
    /// Feltételezi, hogy a ports és az adatbázis tartalma megeggyezik
    virtual bool updateShares(QSqlQuery& __q, bool __clr = false, bool __ex = true);
    /// A ports konténer bővítése egy patch port objektummal.
    /// @param __name port neve
    /// @param __note megjegyzés, leírás
    /// @param __ix Port_index, ha értéke NULL_IX, akkor az index mező NULL lessz.
    virtual cPPort *addPort(const QString& __name, const QString &__note, int __ix);
    /// A port liasta bővítése egy patch port sorozattal.
    /// @param __pref Port név minta.
    /// @param __noff Egy eltolás a név kébzéséhez
    /// @param __from Legkisebb idex
    /// @param __to Legnagyobb idex
    /// @param __off Egy eltolás a port indexhez
    virtual cPPort *addPorts(const QString& __np, int __noff, int __from, int __to, int __off);

    /// A megadott port paraméter beállítása, ha nem találja a portot, akkor dob egy kizárást.
    /// Csak a ports adattagot módosítja!
    /// @param __name Port neve
    /// @param __fn Megváltoztatandó mező neve
    /// @param __v Az új érték(ek)
    cNPort *portSet(const QString& __name,  const QString& __fn, const QVariant& __v);
    /// A megadott port paraméter beállítása, ha nem találja a portot, akkor dob egy kizárást.
    /// Csak a ports adattagot módosítja!
    /// @param __ix port_index (kezdő érték)
    /// @param __fn Megváltoztatandó mező neve
    /// @param __v Az új érték
    cNPort *portSet(int __ix, const QString& __fn, const QVariant& __v);
    /// A megadott port paraméter beállítása, kezdő indextől, ha nem találja a portot, akkor dob egy kizárást.
    /// Csak a ports adattagot módosítja!
    /// @param __ix port_index (kezdő érték)
    /// @param __fn Megváltoztatandó mező neve (NEM POINTER!)
    /// @param __v Az új érték(ek)
    cNPort *portSet(int __ix, const QString& __fn, const QVariantList& __vl);

    /// A név szerint megadott port paraméter hozzáadása ill. modosítása
    /// @param __port A port neve
    /// @param __par A paraméter neve
    /// @param __val A paraméter új értéke.
    cNPort *portSetParam(const QString& __port, const QString& __par, const QString& __val);
    /// A sorszám/index szerint megadott port paraméter hozzáadása ill. modosítása
    /// @param __port_index A port sorszáma ill. indexe
    /// @param __par A paraméter neve
    /// @param __val A paraméter új értéke.
    cNPort *portSetParam(int __port_index, const QString& __par, const QString& __val);
    /// A sorszám/index szerint megadott portok paraméter hozzáadása ill. modosítása
    /// @param __port_index A port sorszáma ill. indexe
    /// @param __par A paraméter neve
    /// @param __val A paraméter új értékei, rendre a következő surszámú porthoz rendelve.
    cNPort *portSetParam(int __port_index, const QString& __par, const QStringList& __val);

    /// A port keresése az index mező értéke alapján.
    cNPort * getPort(int __ix, bool __ex = true);
    /// A port keresése a port név alapján.
    cNPort * getPort(const QString& __pn, bool __ex = true);
    /// Allokál és beolvas egy node objektumot a megadott node_id alapján.
    /// Az allokált objektum típusa megfelel a táblának, ha a rekor az nodes, hosts, snmpdevices táblában van.
    /// @param __node_id A node rekord id-je (port_id)
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cPatch * getNodeObjById(QSqlQuery& q, qlonglong __node_id, bool __ex = true);
    /// Allokál és beolvas egy node objektumot a megadott node_id alapján.
    /// Az allokált objektum típusa megfelel a táblának, aminek a tableoid értékét megadtuk.
    /// @param __node_id A node rekord id-je (port_id)
    /// @param __tableoid Az allokálandó objektum típusát, ill. a hozzázatozó táblát azonosító OID
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cPatch * getNodeObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __node_id, bool __ex);

    /// Egy node neve alapján visszaadja az id-t
    static qlonglong getNodeIdByName(QSqlQuery& q, const QString& _n, bool __ex = true) {
        return cPatch().getIdByName(q, _n, __ex);
    }

    /// A konténer referenciáját adja vissza, amire a pShare mutat.
    /// Ha pShare értéke NULL, akkor ezt egy leszármazottból hívtuk, ahol ez a hívás nem támogatott,
    /// ezért a metódus dob egy kizárást.
    QSet<cShareBack>& shares() {
        if (pShares == NULL) EXCEPTION(ENOTSUPP);
        return *pShares;
    }

    /// A ports portokon törli az összes megosztást (hátlapi megosztások)
    /// Ha ports nincs feltöltve, akkor nem csinál semmit.
    static qlonglong   patchPortTypeId() { (void)cPPort(); return cPPort::_ifTypePatch; }

    /// Az objektum portjai, nincs automatikus feltöltés
    /// Ha módosítjuk az ID-t akkor törlődhet lásd az atEndCont() metódust
    tRecordList<cNPort> ports;
private:
    /// Megosztások konténer. (csak a cPatch osztályban)
    /// Nincs automatikusan feltöltve, de a clearToEnd(); törli, ill. az atEnd() törölheti.
    QSet<cShareBack> * pShares;

};

/// A sablon metódus a megadott ID-vel és a típus paraméternek megfelelő típusú
/// objektumot hozza létre (new) és tölti be az adatbázisból
template<class P> static inline P * getObjByIdT(QSqlQuery& q, qlonglong  __id, bool __ex = true)
{
    P *p = new P();
    p->setId(__id);
    if (p->completion(q) != 1) {
        delete p;
        if (__ex) EXCEPTION(EDATA);
        return NULL;
    }
    return p;
}

/*!
@class cNode
@brief A nodes tábla egy rekordját reprezentáló osztály.

A cPatch-bol származik. Bár müködése különbözik a cPatch-tól, és az objektum így tartalmaz
néhány felesleges komponenst, de igy nem kell eltérni az adatbázisban definiált származási sortol.
 */
class LV2SHARED_EXPORT cNode : public cPatch {
    CRECORD(cNode);
protected:
    explicit cNode(no_init_& _dummy) : cPatch(_dummy) { cNode::descr(); }
public:
    cNode(const QString& __name, const QString& __note);
    cNode& operator=(const cNode& __o);
    // virtual void clearToEnd();
    // virtual void toEnd();
    // virtual bool toEnd(int i);
    /// A cPatch::insert(QSqlQuery &__q, bool __ex = true) hívása elött beállítja a
    /// node_type értékét, ha az NULL.
    virtual bool insert(QSqlQuery &__q, bool __ex = true);
    /// Kitölti a ports adattagot, hiba esetén dob egy kizárást.
    virtual int  fetchPorts(QSqlQuery& __q, bool __ex = true);
    /// A név alapján visszaadja a rekord ID-t, az objektum értéke nem változik.
    /// Ha a node típusban be lett állítva a host bit, akkor ha nincs találat a névre, akkor
    /// a keresett nevet kiegészíti a kereső domain nevekkel, és az így kapott nevekkel végrehajt mégegy keresést.
    /// A kereső domain nevek a sys_param táblában azok a rekordok, melyek típusának a neve "search domain".
    /// Ez utobbi esetben ha több találat van, akkor a sys_param.sys_param_name alapján rendezett első találattal tér vissza.
    virtual qlonglong getIdByName(QSqlQuery& __q, const QString& __n, bool __ex = true) const;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual void clearShares();
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual bool setShare(int __a, int __ab = NULL_IX, int __b = NULL_IX, int __bb = NULL_IX, bool __cd = false);
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual bool updateShares(QSqlQuery& __q, bool __clr = false, bool __ex = true);
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual cPPort *addPort(const QString& __name, const QString &__note, int __ix);
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual cPPort *addPorts(const QString& __np, int __noff, int __from, int __to, int __off);
    /// Kiírja a ports konténer tartalmát is
    virtual QString toString() const;

    /// portok lista bővítése egy elemmel.
    /// @param __t A port típusát definiáló objektum referenciája
    /// @param n Ha több objektum típust definiál a __t, akkor a típus indexe (0,1), illetve érdektelen
    /// @param __name port_name
    /// @param __note port_descr
    /// @param __ix Port_index, ha értéke NULL_IX, akkor az index mező NULL lessz.
    cNPort *addPort(const cIfType& __t, const QString& __name, const QString &__note, int __ix);
    cNPort *addPort(const QString& __t, const QString& __name, const QString &__note, int __ix) {
        return addPort(cIfType::ifType(__t), __name, __note, __ix);
    }

    /// A megadott port nevének és opcionálisan a megjegyzés módosítása, ha nem találja a portot, akkor dob egy kizárást.
    /// Csak a ports adattagot módosítja!
    /// @param __ix port_index (kezdő érték)
    /// @param __name Megváltoztatandó név
    /// @param __note Az új megjegyzés
    cNPort *portModName(int __ix, const QString& __name, const QString& __note = _sNul);
    /// A megadott port típusának, nevének és opcionálisan a megjegyzés módosítása, ha nem találja a portot, akkor dob egy kizárást.
    /// Csak a ports adattagot módosítja!
    /// @param __ix port_index (kezdő érték)
    /// @param __type Típus név
    /// @param __name Megváltoztatandó név
    /// @param __note Az új megjegyzés, opcionális, ha üres (nincs megadva) akkor változatlan
    /// @param __new_ix Az új index, opcionális, ha negatív (nincs megadva) akkor változatlan
    cNPort *portModType(int __ix, const QString& __type, const QString& __name, const QString& __note = _sNul, int __new_ix = NULL_IX);

    /// A port liasta bővítése egy sorozattal. Ha nem vesz fel egy portot sem, akkor dob egy kizárást.
    /// @param __t A port típusát definiáló objektum referenciája
    /// @param __pref Port név minta.
    /// @param __noff Egy eltolás a név kébzéséhez
    /// @param __from Legkisebb idex
    /// @param __to Legnagyobb idex
    /// @param __off Egy eltolás a port indexhez
    /// @return Az utolsó objektum pointere
    cNPort *addPorts(const cIfType& __t, const QString& __np, int __noff, int __from, int __to, int __off);
    /// A port liasta bővítése egy sorozattal. Ha nem vesz fel egy portot sem, akkor dob egy kizárást.
    /// @param __t A port típusát definiáló típus név
    /// @param __pref Port név minta.
    /// @param __noff Egy eltolás a név kébzéséhez
    /// @param __from Legkisebb idex
    /// @param __to Legnagyobb idex
    /// @param __off Egy eltolás a port indexhez
    /// @return Az utolsó objektum pointere
    cNPort *addPorts(const QString& __t, const QString& __np, int __noff, int __from, int __to, int __off)
        { return addPorts(cIfType::ifType(__t), __np, __noff, __from, __to, __off); }
    /// Hasonló mint az addPorts hívás, de a létrehozott portok típusa "sensor", továbbá kitölti az ip címet.
    /// A MAC NULL lessz, az ip cím típusa pedig "pseudo", és csak IPV4 formályú cím lehet
    cNPort *addSensors(const QString& __np, int __noff, int __from, int __to, int __off, const QHostAddress& __pip);

    /// Beolvas egy objektumot/rekordot az IP alapján. A metódus elöbb megkeresi a megadott IP címmel rendelkező
    /// iface_addrs rekordot, és az ehhez tartozó host-ot olvassa be. Ha esetleg cSnmpDevice objektummal hívjuk,
    /// és a beolvasandó rekord csak a parentben szerepel, akkor nem fog beolvasni semmit, vagyis nem lessz találat.
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett IP cím
    /// @return Ha van találat, ill. beolvasott rekord, akkor true, egyébként false
    bool fetchByIp(QSqlQuery& q, const QHostAddress& a);
    /// Beolvas egy objektumot/rekordot a MAC alapján.  Nem feltétlenül tartalmazza a beolvasott
    /// objektum a megadott címet. A metódus elöbb megkeresi a megadott MAC címmel rendelkező interfaces rekordot,
    /// és az ehhez tartozó host-ot olvassa be. Ha esetleg cSnmpDevice objektummal hívjuk, és a beolvasandó rekord
    /// csak a parentben szerepel, akkor nem fog beolvasni semmit, vagyis nem lessz találat.
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett MAC
    /// @return a találatok száma, egy vagy nulla lehet csak.
    int fetchByMac(QSqlQuery& q, const cMac& a);
    /// A saját cNode objektum adatait tölti be az adatbázisból.
    /// Ha meg van adva a HOSTNAME környezeti változó, akkor az ebben megadott nevű rekordot próbálja meg beolvasni.
    /// Ha nincs megadva a környezeti változó, vagy neincs ilyen nevű rekord, akkor lekérdezi a saját ip címeket,
    /// és a címek alapján próbál keresni egy host rekordot.
    /// Lásd még a lanView::testSelfName változót.
    /// @param q Az SQL müveletekhez használt query objektum.
    /// @param __ex Ha értéke true, és nem sikerült beolvasni ill. megtalálni a rekordot, akkor dob egy kizárást.
    /// @return Ha nem találja a saját host rekordot, akkor ha __ex értéke hamis, akkor false-val tér vissza,
    ///         ha __ex értéke Hosttrue, akkor dob egy kizárást. Ha beolvasta a keresett rekordot, akkor true-val tér vissza
    bool fetchSelf(QSqlQuery& q, bool __ex = true);
    cInterface *portAddAddress(const QString& __port, const QHostAddress& __a, const QString& __t, const QString &__d = _sNul);
    cInterface *portAddAddress(int __port_index, const QHostAddress& __a, const QString& __t, const QString &__d = _sNul);
    /// A megadott porthoz hozzáad egy VLAN összerendelést.
    cInterface *portSetVlan(const QString& __port, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st = ST_MANUAL);
    cInterface *portSetVlan(int __port_index, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st = ST_MANUAL);
    ///
    cInterface *portSetVlans(const QString& __port, const QList<qlonglong>& _ids);
    cInterface *portSetVlans(int __port_index, const QList<qlonglong>& _ids);
    /// A beolvasott port és cím rekoedokból kikeresi az első címet.
    QHostAddress getIpAddress() const;

    QList<QHostAddress> fetchAllIpAddress(QSqlQuery &q, qlonglong __id = NULL_ID) const;
    /// Összeállít egy nodes és egy nports rekordot
    /// A port neve és típusa is 'attach" lessz.
    /// @param __n A node neve
    /// @param __d A node megjegyzés/leírás
    /// @param __place Az eszköz helyét azonosító place_id
    /// @return Az objektum referenciája
    cNode& asmbAttached(const QString& __n, const QString& __d, qlonglong __place = 0);
    /// összeállít egy nodes és egy hozzá tartozó interfaces rekordot
    /// A portok neve és típusa is 'ethernet" lessz.
    /// @param __n Node neve
    /// @param __mac MAC cím
    /// @param __d megjegyzés/leírás mező értéke.
    /// @param __place Az eszköz helyét azonosító place_id
    /// @return Az objektum referenciája
    cNode& asmbWorkstation(QSqlQuery &q, const QString& __n, const cMac& __mac, const QString& __d, qlonglong __place = 0);
    /// Egy új port összeállítása
    /// @param ix Port index, vagy NULL_IX, ha az index NULL lessz-
    /// @param pt port típus név string.
    /// @param pn Port neve
    /// @param ip Pointer egy string pár, az első elem az IP cím, vagy az "ARP" string, ha az ip címet a MAC címéből kell meghatározni.
    ///           A második elem az ip cím típus neve. Ha a pointer NULL, akkor nincs IP cím.
    /// @param mac Ha NULL nincs mac, ha a variant egy string, akkor az a MAC cím stringgé konvertélva, vagy az "ARP" string,
    ///           ha variant egy int, akkor az annak a Portnak az indexe (port_index érték!), melynek ugyanez a MAC cíne.
    /// @param d Port leírás/megjegyzés szöveg, üres string esetén az NULL lessz.
    /// @return Az új port objektum pointere
    cNPort& asmbHostPort(QSqlQuery& q, int ix, const QString& pt, const QString& pn, const QStringPair *ip, const QVariant *mac, const  QString& d);
    /// Egy új host vagy snmp eszköz összeállítása
    /// @param name Az eszköz neve, vagy a "LOOKUP" string
    /// @param pp port neve/port typusa, vagy NULL, ha default.
    /// @param ip Pointer egy string pár, az első elem az IP cím, vagy az "ARP" ill. "LOOKUP" string, ha az ip címet a MAC címből ill.
    ///            a névből kell meghatározni.A második elem az ip cím típus neve.
    /// @param mac Vagy a MAC stringgé konvertálva, vagy az "ARP" string, ha az IP címből kell meghatározni.
    /// @param d node secriptorra/megjegyzés
    /// @param __ex Ha értéke true, akkor hiba esetén dob egy kizárást, ha false, akkor hiba esetén a ES_DEFECTIVE bitet állítja be.
    cNode& asmbNode(QSqlQuery& q, const QString& name, const QStringPair* pp, const QStringPair *ip, const QString *mac, const QString &d = _sNul, qlonglong __place = NULL_ID, bool __ex = true);
};


class LV2SHARED_EXPORT cSnmpDevice : public cNode {
    CRECORD(cSnmpDevice);
public:
    cSnmpDevice(const QString& __n, const QString& __d);
    /// A cPatch::insert(QSqlQuery &__q, bool __ex = true) hívása elött beállítja a
    /// node_type értékét, ha az NULL.
    virtual bool insert(QSqlQuery &__q, bool __ex = true);
    /// Lista bővítése egy elemmel.
    /// @param __t A port típusát definiáló objektum referenciája
    /// @param __name port_name
    /// @param __note port_
    /// @param __ix Port_index, A NULL_IX érték nem megengedett (kötelező az egyedi index)
    virtual cNPort * addPort(const cIfType& __t, const QString& __name, const QString &__note, int __ix);
    /// Az SNMP verzió konstanst adja vissza (net-snmp híváshoz)
    int snmpVersion() const;
    ///
    bool setBySnmp(const QString& __addr = _sNul, bool __ex = true);
    ///
    int open(QSqlQuery &q, cSnmp& snmp, bool __ex = true) const;
};

class LV2SHARED_EXPORT cIpProtocol : public cRecord {
    CRECORD(cIpProtocol);
};

class cHostService;

class LV2SHARED_EXPORT cService : public cRecord {
    template <class S> friend void _SplitMagicT(S& o, bool __ex);
    template <class S> friend void _Magic2PropT(S& o);
    CRECORD(cService);
public:
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    const cIpProtocol protocol() { return _protocol; }
    QString protocolName() { return protocol().getName(); }
    int protocolId()  { return (int)protocol().getId(); }
    /// A properties mezőt feldarabolja, és a kapott kulcs, paraméter párokat a _pMagicMap
    /// pointer által mutatott konténerbe teszi. Ha a pointer null, akkor megallokálja a
    /// konténert, ha már létezik, akkor pedig törli a tartalmát.
    tMagicMap&  splitMagic(bool __ex);
    /// A properties mező felbontásából képzett konténer referenciájával tér vissza
    /// Ha a konténer pointere NULL, akkor létrehozza azt. Ezzel ugyan változik az objektum,
    /// de ez nem jelent érdemi változást, a változás oka, hogy a konténert csak akkor hozza létre
    /// ha arra valóban szükség is van.
    const tMagicMap&  magicMap(bool __ex = true) const;
    /// A properties mező felbontásából képzett konténer referenciájával tér vissza
    /// Ha a konténer pointere NULL, akkor létrehozza azt.
    tMagicMap&  magicMap(bool __ex = true);
    const QString& magicParam(const QString &_nm) const { return ::magicParam(_nm, magicMap()); }
    cService &magic2prop();
protected:
    static int              _ixProtocolId;
    static int              _ixProperties;
    cIpProtocol             _protocol;
    tMagicMap              *_pMagicMap;
    /// Konténer ill. gyorstár a cService rekordoknak.
    /// Nem frissül automatikusan, ha változik az adattábla.
    static tRecordList<cService> services;
    /// Egy üres objektumra mutató pointer. Az első használat alkalmával jön létre ld,: _nul()
    static cService *pNull;
public:
    /// Egy services objektumot ad vissza a név alapján.
    /// Ha nincs ilyen nevű szervíz, akkor dob egy kizárást, vagy egy öres obbjektummal tér vissza.
    static const cService& service(QSqlQuery &__q, const QString& __nm, bool __ex = true);
    /// Egy services objektumot ad vissza az ID alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    static const cService& service(QSqlQuery &__q, qlonglong __id, bool __ex = true);
    static const cService& _nul() { if (pNull == NULL) pNull = new cService(); return *pNull; }
};

class LV2SHARED_EXPORT cHostService : public cRecord {
    template <class S> friend void _SplitMagicT(S& o, bool __ex);
    template <class S> friend void _Magic2PropT(S& o);
    CRECORD(cHostService);
public:
    cHostService& operator=(const cHostService& __o);
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    /// Státusz beállítása. A  set_service_stat() PL/pSQL függvényt hívja.
    /// @param __q Az adatbázis művelethez jasznált objektum.
    /// @param __st A szolgáltatással kapcsolatos művelet eredménye. Nem az új status, azt a set_service_stat()
    ///             határozza majd meg a mostani eredmény és az előélet alapján.
    /// @param __note Egy megjegyzés, ami, bele kerül a log rekordba, ha van változás, ill. ha keletkezik üzenet,
    ///             akkor abba is.
    /// @param __did Daemon host_service_id, opcionális
    /// @return Az objektum referenciája, aktualizálva az aktuális rekord értékkel.
    /// @exception Bármilyen hiba esetén dob egy kizázást cError * -al.
    cHostService& setState(QSqlQuery& __q, const QString& __st, const QString& __note = QString(), qlonglong __did = NULL_ID);
    /// A nevek alapján olvassa be egy rekordot.
    int fetchByNames(QSqlQuery& q, QString& __hn, const QString& __sn, bool __ex = true);
    /// Az ID-k alapján olvas be egy rekordot
    bool fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, bool __ex = true);
    /// A megadott nevek alapján türli a megadott rekord(ok)at
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __nn A node neve, vagy egy minta
    /// @param __sn A szolgálltatás típus neve, vagy egy minta
    /// @param __spat Ha értéke true, akkor az __sn paraméter egy minta, egyébként egy szervíz típus neve.
    /// @param __npat Ha értéke true, akkor az __nn paraméter egy minta, egyébként egy node neve.
    /// @return A törölt rekordok számával tér vissza.
    int delByNames(QSqlQuery& q, const QString& __nn, const QString& __sn, bool __spat = false, bool __npat = false);
    /// Beolvassa a saját host és host_service rekordokat. Az __sn egy beolvasott servces rekordot kell tartalmazzon.
    /// A beolvasott host a saját host rekord lessz.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __h A kitöltendő host objektum. Eredeti objektum típusa cNode vagy cSnmpDevice lehet.
    /// @param __s A szervíz rekordot reprezentáló kitöltött objektum, a metódus csak az ID mezőt használja.
    /// @param __ex Ha értéke true (ill nem adjuk meg), akkor hiba esetén, ill. ha nem létezik valamelyik keresett objektum, akkor dob egy kizárást.
    /// @return Ha sikerült beolvasni a rekordokat, akkor true, ha nem és __ex értéke true, akkor false.
    bool fetchSelf(QSqlQuery& q, cNode& __h, const cService& __s, bool __ex = true);
    /// Az alárendelt szolgálltatások lekérdezése
    int fetchSubs(QSqlQuery& __q, cHostService& __sup);
    /// Egy mező értékének a lekérdezése.
    /// Ha a mező értéke null, akkor annak alapértelmezett értéke a megadott s-objektum azonos nevű
    /// mezőjének értéke lessz. Ha az s objektumban is null az érték, akkor dob egy kizárást.
    /// Ha a keresett mező értéke null volt, akkor azt fellülírja az alapértelmezett értékkel.
    /// @param q Ha módosítja az objektumot, ezt használja az UPDATE parancs kiadásához.
    /// @param s Az objektumhoz tartozó, beolvasott services rekord objektum.
    /// @param f A keresett mező neve.
    /// @return A keresett mező értéke.
    /// @exceptions * cError Ha mindkét objektumban a keresett mező értéke null, vagy update esetén hiba történt.
    QVariant value(QSqlQuery& q, const cService& s, const QString& f);
    /// Rekord azonosító nevekből képez egy stringet: node:szolgáltatés alakban.
    /// A neveket az objektum nem tartalmazza, ezért azokat az adatbázisból kérdezi le.
    QString names(QSqlQuery& q);
    /// A properties mezőt feldarabolja, és a kapott kulcs, paraméter párokat a _pMagicMap
    /// pointer által mutatott konténerbe teszi. Ha a pointer null, akkor megallokálja a
    /// konténert, ha már létezik, akkor pedig törli a tartalmát.
    tMagicMap&  splitMagic(bool __ex);
    /// A properties mező felbontásából képzett konténer referenciájával tér vissza
    /// Ha a konténer pointere NULL, akkor létrehozza azt. Ezzel ugyan változik az objektum,
    /// de ez nem jelent érdemi változást, a változás oka, hogy a konténert csak akkor hozza létre
    /// ha arra valóban szükség is van.
    const tMagicMap&  magicMap(bool __ex = true) const;
    /// A properties mező felbontásából képzett konténer referenciájával tér vissza
    /// Ha a konténer pointere NULL, akkor létrehozza azt.
    tMagicMap&  magicMap(bool __ex = true);
    const QString& magicParam(const QString &_nm) { return ::magicParam(_nm, magicMap()); }
    /// A _pMagicMap által mutatott konténerből összeállítja a properties mezőt.
    /// Ha _pMagicMap értéke NULL, akkor az eredmény egy üres string lessz.
    cHostService& magic2prop();
    /// A prime_service mező álltal hivatkozott cService objektummal tér vissza.
    /// Ha a mező ártéke NULL, akkor egy üres objektum referenciájával.
    /// @param Az esetleges SQL lekérdezéshez használlt objektum (ha már be van olvasva a keresett objektum, akkor nem fordul az adatbázishoz)
    /// @param __ex Hiba esetén vagy, ha az id nem NULL, de mégsem találja az objektumot, akkor nem üres objektummal tér vissza, hanem dob egy kizárást, ha __ex értéke true.
    /// @return A keresett objektum referenciája, ill. ha hiba volt és __ex nem true, ill. ha az ID NULL, akkor egy üres objektum pointere.
    const cService& getPrimeService(QSqlQuery& __q, bool __ex = true)
    {
        qlonglong id = getId(_sPrimeServiceId);
        return id == NULL_ID ? cService::_nul() : cService::service(__q, id, __ex);
    }
    /// A proto_service mező álltal hivatkozott cService objektummal tér vissza.
    /// Ha a mező ártéke NULL, akkor egy üres objektum referenciájával.
    /// @param Az esetleges SQL lekérdezéshez használlt objektum (ha már be van olvasva a keresett objektum, akkor nem fordul az adatbázishoz)
    /// @param __ex Hiba esetén vagy, ha az id nem NULL, de mégsem találja az objektumot, akkor nem üres objektummal tér vissza, hanem dob egy kizárást, ha __ex értéke true.
    /// @return A keresett objektum referenciája, ill. ha hiba volt és __ex nem true, ill. ha az ID NULL, akkor egy üres objektum pointere.
    const cService& getProtoService(QSqlQuery& __q, bool __ex = true)
    {
        qlonglong id = getId(_sProtoServiceId);
        return id == NULL_ID ? cService::_nul() : cService::service(__q, id, __ex);
    }
protected:
    static int              _ixProperties;
    tMagicMap              *_pMagicMap;
};

class LV2SHARED_EXPORT cOui  : public cRecord {
    CRECORD(cOui);
public:
    enum eReasons replace(QSqlQuery& __q);
};

class LV2SHARED_EXPORT cMacTab  : public cRecord {
    CRECORD(cMacTab);
protected:
    static int _ixPortId;
    static int _ixHwAddress;
    static int _ixSetType;
    static int _ixMacTabState;
public:
    /// Inzertálja, vagy modosítja az ip cím, mint kulcs alapján a rekordot.
    /// A funkciót egy PGPLSQL fúggvény (insert_or_update_mactab) valósítja meg.
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @return A insert_or_update_arp függvény vissatérési értéke. Ld.: enum eReasons
    enum eReasons replace(QSqlQuery& __q);
    static int refresStats(QSqlQuery& __q);
};

class cArpTable;
class LV2SHARED_EXPORT cArp : public cRecord {
    CRECORD(cArp);
protected:
    static int _ixIpAddress;
    static int _ixHwAddress;
public:
    cArp& operator = (const QHostAddress& __a)  { set(_ixIpAddress, __a.toString()); return *this; }
    cArp& operator = (const cMac __m)           { set(_ixHwAddress, __m.toString()); return *this; }
    /// Az objektum (a beolvasott rekord) IP cím mezőjének az értékével tér vissza
    operator QHostAddress() const;
    /// Az objektum (a beolvasott rekord) MAC mezőjének az értékével tér vissza
    operator cMac()         const;
    /// Az objektum (a beolvasott rekord) MAC mezőjének az értékével tér vissza
    cMac getMac() const { return (cMac)*this; }
    /// Az objektum (a beolvasott rekord) IP cím mezőjének az értékével tér vissza
    QHostAddress getIpAddress() const { return (QHostAddress)*this; }
    /// Inzertálja, vagy modosítja az ip cím, mint kulcs alapján a rekordot.
    /// A funkciót egy PGPLSQL fúggvény (insert_or_update_arp) valósítja meg.
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @return A insert_or_update_arp függvény vissatérési értéke. Ld.: enum eReasons
    enum eReasons replace(QSqlQuery& __q);
    /// Inzertálja, vagy morosítja az ip cím, mint kulcs alapján a rekordokat
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @param __t A módosításokat tartalmazó konténer
    /// @return a kiírt, vagy módosított rekordok száma
    static int replaces(QSqlQuery& __q, const cArpTable& __t);
    static QList<QHostAddress> mac2ips(QSqlQuery& __q, const cMac& __m);
    static QHostAddress mac2ip(QSqlQuery& __q, const cMac& __m, bool __ex = true);
    static cMac ip2mac(QSqlQuery& __q, const QHostAddress& __a, bool __ex = true);
    static int checkExpired(QSqlQuery& __q);
};

/// Csak a cPhsLink és cLldpLink objektumokkal használható
/// A megadott porthoz tartozó linket törli.
/// @param q Az SQL lekérdezéshez használt objektum.
/// @param o Az objektum, melyhez tartozó táblából törölni szeretnénk (tartalma érdektelen, csak a táblát azonosítja)
/// @param __pid Port id (port_id1)
/// @return true, ha törölt egy rekordot, false, ha nem
EXT_ bool LinkUnlink(QSqlQuery& q, cRecord& o, qlonglong __pid);
/// Csak a cPhsLink, cLogLink és cLldpLink objektumokkal használható
/// Megadja, hogy az ID alapján megadott port, mely másik porttal van link-be
/// @param q Az SQL lekérdezéshez használt objektum.
/// @param o Az objektum, melyhez tartozó táblában keresni szeretnénk
/// @param __pid port ID
/// @return A talált port ID-je, vagy NULL_ID
EXT_ qlonglong LinkGetLinked(QSqlQuery& q, cRecord& o, qlonglong __pid);
/// Csak a cPhsLink, cLogLink és cLldpLink objektumokka használható
/// Megadja, hogy az ID alapján megadott két port, link-be van-e
/// @param q Az SQL lekérdezéshez használt objektum.
/// @param o Az objektum, melyhez tartozó táblában keresni szeretnénk
/// @param __pid1 az egyik port ID
/// @param __pid2 a másik port ID
/// @return A talált port ID-je, vagy NULL_ID
EXT_ bool LinkIsLinked(QSqlQuery& q, cRecord& o, qlonglong __pid1, qlonglong __pid2);

class LV2SHARED_EXPORT cPhsLink : public cRecord {
    CRECORD(cPhsLink);
public:
    /// Törli a megadott fizikai linket
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __nn A host neve, amihez a link tartozik
    /// @param __pn A port neve, vagy egy minta, amihez a link trtozik
    /// @param __pat Ha true, akkor nem konkrét port név lett megadva, hanem egy minta.
    /// @return A törölt rekordok száma
    int unlink(QSqlQuery &q, const QString& __nn, const QString& __pn, bool __pat);
    /// Törli a megadott fizikai linket
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __hn A host neve, amihez a link tartozik
    /// @param __ix A port indexe, amihez a link trtozik, vagy egy kező érték
    /// @param __ei Egy port index záró érték, vagy NULL_IX, ha nem egy trtományról van szó.
    /// @return A törölt rekordok száma
    int unlink(QSqlQuery &q, const QString& __nn, int __ix, int __ei);
    bool unlink(QSqlQuery &q, qlonglong __pid) { return LinkUnlink(q, *this, __pid); }
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }
};

class LV2SHARED_EXPORT cLogLink : public cRecord {
    CRECORD(cLogLink);
public:
    /// A tábla írása automatikus, az insert metódus tiltott
    virtual bool insert(QSqlQuery &, bool);
    /// A tábla írása automatikus, az update metódus tiltott
    virtual bool update(QSqlQuery &, bool, const QBitArray &, const QBitArray &, bool);
    /// A logikai link tábla alapján megadja, hogy a megadott id-jű port mely másik portal van összekötve
    /// Az aktuális link rekordot beolvassa, ha van találat.
    /// @param A port id, melyel linkelt portot keressük.
    /// @return A linkelt port id-je, vagy NULL_ID, ha nincs linkbe másik port a megadottal.
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }

};

class LV2SHARED_EXPORT cLldpLink : public cRecord {
    CRECORD(cLldpLink);
public:
    bool unlink(QSqlQuery &q, qlonglong __pid) { return LinkUnlink(q, *this, __pid); }
    /// A logikai link tábla alapján megadja, hogy a megadott id-jű port mely másik portal van összekötve
    /// Az aktuális link rekordot beolvassa, ha van találat.
    /// @param A port id, melyel linkelt portot keressük.
    /// @return A linkelt port id-je, vagy NULL_ID, ha nincs linkbe másik port a megadottal.
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }
};

enum eExecState {
    ES_INVALID = -1,
    ES_WAIT    =  0,
    ES_EXECUTE,
    ES_OK,
    ES_FAILE,
    ES_ABORTED
};

int execState(const QString& _n, bool __ex = true);
const QString& execState(int _e, bool __ex = true);

class LV2SHARED_EXPORT cImport : public cRecord {
    CRECORD(cImport);
};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cImportTemplate : public cRecord {
    CRECORD(cImportTemplate);
};

/// @class cTemplateMap
/// import_template record cache
class LV2SHARED_EXPORT cTemplateMap : public QMap<QString, QString> {
public:
    /// Konstruktor
    cTemplateMap() { EXCEPTION(ENOTSUPP); }
    /// Konstruktor
    /// @param __type Típus string, a konténerben azok a rekordok kerülnek, melyeknél a template_type mező értéke ezzel megeggyezik
    cTemplateMap(const QString& __type);
    /// Copy konstruktor
    cTemplateMap(const cTemplateMap& __o);
    /// copy operator, csak akkor másol, ha a type adattag azonos
    cTemplateMap& operator=(const cTemplateMap& __o) {
        if (__o.type != type) EXCEPTION(EPROGFAIL);
        *(QMap<QString, QString> *)this = (QMap<QString, QString>)__o;
        return *this;
    }
    /// Egy megadott nevű template lekérése, ha nincs a konténerben, akkor beolvassa az adatbázisból.
    const QString& get(QSqlQuery& __q, const QString& __name);
    /// Egy adott nevű template elhelyezése a konténerbe, de az adatbázisban nem.
    void set(const QString& __name, const QString& __cont);
    /// Egy adott nevű template elhelyezése a konténerbe, és az adatbázisban.
    void save(QSqlQuery& __q, const QString& __name, const QString& __cont, const QString& __note);
    /// Egy adott nevű template törlése a konténerből, és az adatbázisból.
    void del(QSqlQuery& __q, const QString &__name);
    /// A temlate rekordok típusa (template_type mező megkövetelt értéke / szűrési feltétel)
    const QString type;
};

/* */

class LV2SHARED_EXPORT cPortVlan : public cRecord {
    friend class cInterface;
    CRECORD(cPortVlan);
public:
    cPortVlan& setPortId(qlonglong __id)        { set(_ixPortId, __id); return *this; }
    cPortVlan& setVlanId(qlonglong __id)        { set(_ixVlanId, __id); return *this; }
    cPortVlan& setVlanId(const QString& __vn)   { set(_ixVlanId, cVLan().getIdByName(__vn)); return *this; }
    cPortVlan& setVlanType(enum eVlanType __e)  { setName(_ixVlanType, vlanType(__e)); return *this; }
    cPortVlan& setVlanType(const QString& __n)  { (void)vlanType(__n); setName(_ixVlanType, __n); return *this; }
    enum eVlanType getVlanType(bool __ex = true){ return (enum eVlanType)vlanType(getName(_ixVlanType), __ex); }
    cPortVlan& setSetType(enum eSetType __e)    { setName(_ixSetType, setType(__e)); return *this; }
    cPortVlan& setSetType(const QString& __n)   { (void)setType(__n); setName(_ixSetType, __n); return *this; }
    enum eSetType getSetType(bool __ex = true)  { return (enum eSetType)setType(getName(_ixSetType), __ex); }
protected:
    static int _ixPortId;
    static int _ixVlanId;
    static int _ixVlanType;
    static int _ixSetType;
};

#endif // LV2DATA_H
