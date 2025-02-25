#ifndef LV2DATA_H
#define LV2DATA_H

#include "lv2datab.h"
#include "lv2cont.h"
#include "qsnmp.h"

/*!
@file lv2data.h
Az adatbázis interfész objektumok.
*/

/// Lekérdezés eredmonye.
/// Ugyan ez az enumerációs típus reprezentálja az adatbázisban a notifswitch nevű enumerációs típust is,
/// de ott az RS_STAT_MASK és az aszt megelőző elemek nem használhatóak, ill nincs megfelelőjük.
/// Az enumerációs (numerikus) értékek, és az őket reprezentáló adatbázis beli string értékek között a
/// notifSwitch() függvény pár konvertál.
enum eNotifSwitch {
    RS_INVALID     =   -1,  ///< Csak hibajelzésre szolgál
    RS_STAT_SETTED = 0x80,  ///< A status beállítása megtörtént, mask nem valódi status érték, henem egy flag
    RS_SET_RETRY   = 0x40,  ///< Az időzítést normálból->retry-be kel váltani.
    RS_BREAK       = 0x20,
    RS_STAT_MASK   = 0x0f,  ///< A valódi státusz maszkja
    RS_ON          =    0,  ///< Az aktuális futási eredmény 'on'
    RS_RECOVERED,           ///< Az aktuális futási eredmény 'recovered'
    RS_WARNING,             ///< Az aktuális futási eredmény 'warning'
    RS_CRITICAL,            ///< Az aktuális futási eredmény 'critical'
    RS_UNREACHABLE,         ///< Az aktuális futási eredmény 'unreachable'
    RS_DOWN,                ///< Az aktuális futási eredmény 'down'
    RS_FLAPPING,            ///< Az aktuális futási eredmény 'flapping'
    RS_UNKNOWN              ///< Ismeretlen, még nincs beállítva, nem futási eredmény
};


EXT_ int reasons(const QString& _r, enum eEx __ex = EX_ERROR);
EXT_ const QString& reasons(int _r, enum eEx __ex = EX_ERROR);

/// A lekérdezés eredményét reprezebtáló enumerációs értéket konvertálja az adatbázisban tárolt string formára.
/// RS_STAT_SETTED érték konverzió elött ki lessz maszkolva. Ha a konverzió nem lehetséges, akkor dob egy kizárást.
EXT_ const QString& notifSwitch(int _ns, enum eEx __ex = EX_ERROR);
/// A lekérdezés eredményét reprezebtáló enumerációs érték adatbázisban tárolt string formáráját konvertálja
/// numerikus értékké.
EXT_ int notifSwitch(const QString& _nm, enum eEx __ex = EX_ERROR);

/* -------------------- Enum, enum - string konverziók -------------------- */
/// @enum eSubNetType
/// @brief SubNet típusok
enum eSubNetType {
    NT_PRIMARY =  0,    ///< Elsődleges IP tartomány
    NT_SECONDARY,       ///< Másodlagos tartomány azonos fizikai hálózaton
    NT_PSEUDO,          ///< Nem valós tartomány
    NT_PRIVATE          ///< Privat (elszigetelt, nam routolt) címtartomány
};

/// Cím tartomány típus stringgel (az adatbázísban a típust reprezentáló értékkel) tér vissza, a megadott konstans alapján.
/// @param __at A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ const QString& subNetType(int __at, enum eEx __ex = EX_ERROR);
/// Cím típus konstanssal tér vissza, a megadott típus név alapján
EXT_ int subNetType(const QString& __at, enum eEx __ex = EX_ERROR);

/// @enum eAddressType
/// @brief Ip cím típusok
enum eAddressType {
    AT_FIXIP   =  0,    ///< Egyedi fix IP cím (DHCP)
    AT_PRIVATE,         ///< Csak lokálisan használt valós cím, ütközhet bármilyen egyébb IP címmel.
    AT_EXTERNAL,        ///< Külső cím (fix cím, de nincs hozzá subnet
    AT_DYNAMIC,         ///< dinamikus IP cím
    AT_PSEUDO,          ///< Egyedi nem valós IP cím, csak azonosításra
    AT_JOINT,           ///< Közös cím (cluster)
    AT_MANUAL           ///< manuálisan beállítva
};

/// Cím típus stringgel tér vissza, a megadott konstans alapján.
/// @param __at A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ const QString& addrType(int __at, enum eEx __ex = EX_ERROR);
/// Cím típus konstanssal tér vissza, a megadott név alapján
/// @param __at A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ int addrType(const QString& __at, enum eEx __ex = EX_ERROR);

/// Port - VLAN összerendelés típusa
enum eVlanType {
    VT_NO      =  0,    ///< Nincs összerendelés
    VT_NOTKNOWN,        ///< Ismeretlen
    VT_FORBIDDEN,       ///< Tiltott
    VT_AUTO,            ///< automatikus
    VT_AUTO_TAGGED,     ///< auto+tagged
    VT_TAGGED,          ///< Tagged (802.1q)
    VT_UNTAGGED,        ///< Untagged
    VT_VIRTUAL,         ///< Virtuális interfész a VLAN-hoz létrehozva
    VT_HARD,            ///< Az eszköz által nem kezelt összeremdelés
    VT_AUTH,            ///< 802.1x auth. vlan
    VT_UNAUTH           ///< 802.1x un-auth. vlan
};

/// VLAN típus névvel tér vissza, a megadott konstans alapján.
/// @param __n A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ int vlanType(const QString& __n, enum eEx __ex = EX_ERROR);
/// VLAN típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const QString& vlanType(int __e, enum eEx __ex = EX_ERROR);

/// Port - VLAN összerendelés forrása
enum eSetType  {
    ST_AUTO,            ///< automatikus
    ST_QUERY,           ///< Egy lekérdezés eredménye
    ST_CONFIG,          ///< Konfig import
    ST_MANUAL           ///< Manuális összerendelés
};

/// port - VLAN összerendelés típus névvel tér vissza, a megadott konstans alapján.
/// @param __n A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ int  setType(const QString& __n, enum eEx __ex = EX_ERROR);
/// ort - VLAN összerendelés típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const QString& setType(int __e, enum eEx __ex = EX_ERROR);

/// Image types
enum eImageType {
    IT_BMP,             ///< Windows bit map
    IT_GIF,             ///< GIF
    IT_JPG,             ///< Jpeg
    IT_PNG,             ///< PNG
    IT_PBM,
    IT_PGM,
    IT_PPM,
    IT_XBM,
    IT_XPM,
    IT_BIN,             ///< Other binary
};

/// Image useability
enum eUsability {
    US_MAP,     ///< image is map
    US_FLAG,    ///< image is flag
};

/// kép típus névvel tér vissza, a megadott konstans alapján.
/// @param __n A típus konstans.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus string, ha nem megengedett konstanssal hívtuk, és __ex false volt, akkor a NULL string.
EXT_ int  imageType(const QString& __n, enum eEx __ex = EX_ERROR);
/// Kép típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const QString&   imageType(int __e, enum eEx __ex = EX_ERROR);
/// Kép típus konstanssal tér vissza, a megadott név alapján
/// @param __e A típus név. Egyes Qt függvények ezt a típust vátják.
/// @param __ex Ha nem megengedett értékkel hívjuk és értéke true, akkor dob egy kizárást.
/// @return A típus konstans, ha nem megengedett névvel hívtuk, és __ex false volt, akkor -1
EXT_ const char *    _imageType(int __e, enum eEx __ex = EX_ERROR);

EXT_ int usability(const QString& __n, eEx __ex);
EXT_ const QString&  usability(int __e, eEx __ex);

/// @enum eNodeType
/// Hálózati elemek típus azonosítók (set)
enum eNodeType {
    NT_PATCH  = 0,
    NT_NODE,
    NT_HOST,
    NT_SWITCH,
    NT_HUB,
    NT_VIRTUAL,
    NT_SNMP,
    NT_CONVERTER,
    NT_PRINTER,
    NT_GATEWAY,
    NT_AP,
    NT_WORKSTATION,
    NT_MOBILE,
    NT_DEVICE,
    NT_CONTROLLER,
    NT_UPS,
    NT_WINDOWS,
    NT_SERVER,
    NT_CLUSTER,
    NT_UNIX_LIKE,
    NT_THIN_CLIENT,
    NT_DISPLAY
};

/// Node típus név konverzió
/// @param __n A node típus neve (SQL enumerációs érték)
/// @param __ex Ha értéke true, és nem valós típusnevet adtunk meg, akkor dob egy kizárást.
/// @return A típus konstanssal tér vissza, ha nincs ilyen típus, és __ex értéke false, akkor a NT_INVALID konstanssal.
EXT_ int nodeType(const QString& __n, enum eEx __ex = EX_ERROR);
/// Paraméter típus név konverzió
/// @param __e A node típus konstans
/// @param __ex Ha értéke true, és nem valós típus konstanst adtunk meg, akkor dob egy kizárást.
/// @return A típus névvel tér vissza, ha nincs ilyen típus, és __ex értéke false, akkor egy üres stringgel.
EXT_ const QString& nodeType(int __e, enum eEx __ex = EX_ERROR);

/* ------------------------------------------------------------------ */
/// Paraméter típus leíró rekord
class LV2SHARED_EXPORT cParamType : public cRecord {
    CRECORD(cParamType);
public:
    /// Egy új paraméter típus rekord beinzertálása (paraméter név/típus/dimenzió objektumlétrehozása).
    /// Hiba esetén, ha __ex nem EX_IGNORE (vagy nincs megadva), akkor dob egy kizárást,
    /// @param q Az adatbázis művelethet használt objektum
    /// @param __n A paraméter típus neve
    /// @param __de Egy megjegyzés a paraméter típushoz
    /// @param __t A paraméter adat típusa
    /// @param __di A paraméter dimenziója, opcionális
    /// @param _replace Ha igaz, akkor ha létezik a rekord a név alapján, akkor modosítás van beszúrás helyett
    /// @return Az új rekord azonisítója (ID), hiba esetén, ha __ex hamis volt, akkor NULL_ID-vel tér vissza.
    static qlonglong insertNew(QSqlQuery &q, const QString& __n, const QString& __de, const QString __t, const QString __di = QString(), bool _replace = false, enum eEx __ex = EX_ERROR);
    /// Egy új paraméter típus rekord beinzertálása (paraméter név/típus/dimenzió objektumlétrehozása).
    /// Hiba esetén, ha __ex igaz (vagy nincs megadva), akkor dob egy kizárást,
    /// @param q Az adatbázis művelethet használt objektum
    /// @param __n A paraméter típus neve
    /// @param __de Egy megjegyzés a paraméter típushoz
    /// @param __t A paraméter adat típusa (lsd.: eParamType).
    /// @param __di A paraméter dimenziója, opcionális
    /// @param _replace Ha igaz, akkor ha létezik a rekord a név alapján, akkor modosítás van beszúrás helyett
    /// @return Az új rekord azonisítója (ID), hiba esetén, ha __ex hamis volt, akkor NULL_ID-vel tér vissza.
    static qlonglong insertNew(QSqlQuery &q, const QString& __n, const QString& __de, int __t, const QString __di = QString(), bool _replace = false, enum eEx __ex = EX_ERROR) {
        return insertNew(q, __n, __de, paramTypeType(__t), __di, _replace, __ex);
    }
    /// Egy QVariant típusból konvertálja az adatbázisba kiírandó text típusú értéket, a típusban meghatározott adattípusnak megfelelően.
    /// @param __t Az adat típus
    /// @param __v A konvertálandó érték
    /// @param __ex Hiba kezelés
    /// @return A konvertált string
    static QString paramToString(eParamType __t, const QVariant& __v, enum eEx __ex = EX_ERROR, bool *pOk = nullptr);
    /// Egy adatbázisból beolvasott stringet kovertál a típusnak megfelelő típuső értékké.
    /// @param __t Az adat típus
    /// @param __v A konvertálandó string
    /// @param __ex Hiba kezelés
    /// @return A konvertált érték
    static QVariant paramFromString(eParamType __t, const QString &__v, enum eEx __ex = EX_ERROR);
    /// Egy QVariant típusból konvertálja az adatbázisba kiírandó text típusú értéket, a típus leíróbanban meghatározott adattípusnak megfelelően.
    /// @param __v A konvertálandó érték
    /// @param __ex Hiba kezelés
    /// @return A konvertált string
    QString paramToString(const QVariant& __v, enum eEx __ex = EX_ERROR) const { return paramToString(eParamType(getId(_ixParamTypeType)), __v, __ex); }
    /// Egy adatbázisból beolvasott stringet kovertál a típus leírónak megfelelő típusú értékké.
    /// @param __v A konvertálandó string
    /// @param __ex Hiba kezelés
    /// @return A konvertált érték
    QVariant paramFromString(const QString& __v, enum eEx __ex = EX_ERROR) const { return paramFromString(eParamType(getId(_ixParamTypeType)), __v, __ex); }
protected:
    static tRecordList<cParamType>  paramTypes;
    static cParamType *pNull;
public:
    /// Feltölti, vagy frissíti az adatbázisból az paramTypes adattagot.
    /// A rekordok törlése nem támogatott, ellenörzi, ha törölni kellene a konténerből, akkor kizárást dob.
    static void fetchParamTypes(QSqlQuery& __q);
    /// Egy cParamType objektumot ad vissza a név alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az paramTypes adattagban keres, ha paramTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke hamis, akkor kizárás helyett, egy üres objektummal tér vissza (amire a pNull adattag mutat)
    static const cParamType& paramType(const QString& __nm, enum eEx __ex = EX_ERROR);
    /// Egy paramTypes objektumot ad vissza az ID alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az paramTypes adattagban keres, ha paramTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke hamis, akkor kizárás helyett, egy üres objektummal tér vissza (amire a pNull adattag mutat)
    static const cParamType& paramType(qlonglong __id, enum eEx __ex = EX_ERROR);
    /// Egy paramTypes objektum Azonosítóját adja vissza a név alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az paramTypes adattagban keres, ha paramTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke hamis, akkor kizárás helyett, NULL_ID -vel tér vissza.
    static qlonglong paramTypeId(const QString& __nm, enum eEx __ex = EX_ERROR) { return paramType(__nm, __ex).getId(); }
    /// Egy paramTypes objektum nevét adja vissza az ID alapján, ha nincs ilyen azonosítójú típus, akkor dob egy kizárást.
    /// Az paramTypes adattagban keres, ha paramTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke hamis, akkor kizárás helyett, üres stringgel tér vissza.
    static QString   paramTypeName(qlonglong __id, enum eEx __ex = EX_ERROR)    { return paramType(__id, __ex).getName(); }
protected:
    /// Ha nincs feltöltve az paramTypes adattag , akkor feltölti az adatbázisból,
    /// Vagyis hívja a void fetchParamTypes(QSqlQuery& __q); metódust.
    static void checkParamTypes() { if (pNull == nullptr) { QSqlQuery q = getQuery(); fetchParamTypes(q); } }
    STATICIX(cParamType, ParamTypeType)
};

class LV2SHARED_EXPORT cSysParam  : public cRecord {
    CRECORD(cSysParam);
public:
    /// Törli a paramType adattagot.
    virtual void    clearToEnd() override;
    /// A toEnd(int i) metódust hívja a port paraméter (típus) rekord id mező indexével.
    virtual void 	toEnd () override;
    /// Ha megváltozik a port param (típus) id, akkor betölti, vagy törli a megfelelp értéket a paramType adattagba.
    /// Nincs ilyen id-vel port_params rekord (és nem NULL az id), akkor a statusban bebillenti az ES_DEFECZIVE bitet.
    virtual bool 	toEnd (int i) override;
    /// A paraméter adat típus enumerációs értékkel tér vissza
    qlonglong valueType()   const { return paramType.getId(_sParamTypeType); }
    /// A paraméter adat típus névvel tér vissza
    const QString& valueTypeName(enum eEx __ex = EX_ERROR) const {return paramTypeType(int(valueType()), __ex); }
    /// A paraméter dimenzió ill. mértékegység nevével tér vissza
    QString valueDim()    const { return paramType.getName(__sParamTypeDim); }
    /// A paraméter értékkel tér vissza
    QVariant value(enum eEx __ex = EX_ERROR) const;
    /// A paraméter típusának a beállítása
    /// @param __id A paraméter típus rekord ID-je
    cSysParam& setType(qlonglong __id)        { set(_ixParamTypeId, __id); return *this; }
    /// @param __n A paraméter típus rekord név mezőjenek az értéke
    cSysParam& setType(const QString& __n)    { paramType.fetchByName(__n); _set(_ixParamTypeId, paramType.getId()); return *this; }
    /// Értékadás az érték mezőnek.
    cSysParam& setValue(const QVariant& __v, enum eEx __ex = EX_ERROR);
    /// Értékadás az érték mezőnek.
    cSysParam& operator=(const QVariant& __v) { return setValue(__v); }
    static enum eReasons setTextSysParam(QSqlQuery& _q, const QString& __nm, const QString& _val, const QString& _tn = _sText) {
        execSqlFunction(_q, "set_text_sys_param", QVariant(__nm), QVariant(_val), QVariant(_tn));
        return eReasons(reasons(_q.value(0).toString()));
    }
    static enum eReasons setBoolSysParam(QSqlQuery& _q, const QString& __nm, bool _val, const QString& _tn = _sBoolean) {
        execSqlFunction(_q, "set_bool_sys_param", QVariant(__nm), QVariant(_val), QVariant(_tn));
        return eReasons(reasons(_q.value(0).toString()));
    }
    static enum eReasons setIntegerSysParam(QSqlQuery& _q, const QString& __nm, qlonglong _val, const QString& _tn = _sInteger) {
        execSqlFunction(_q, "set_int_sys_param", QVariant(__nm), QVariant(_val), QVariant(_tn));
        return eReasons(reasons(_q.value(0).toString()));
    }
    static enum eReasons setIntervalSysParam(QSqlQuery& _q, const QString& __nm, qlonglong _val, const QString& _tn = _sInterval) {
        execSqlFunction(_q, "set_interval_sys_param", QVariant(__nm), QVariant(intervalToStr(_val)), QVariant(_tn));
        return eReasons(reasons(_q.value(0).toString()));
    }
    static enum eReasons setSysParam(QSqlQuery& _q, const QString& __nm, const QVariant& _val, const QString& _tn, enum eEx __ex = EX_ERROR) {
        cSysParam   po;
        po.setType(_tn);
        po.setValue(_val, __ex);
        enum eReasons r = setTextSysParam(_q, __nm, po.getName(_ixParamValue), _tn);
        if (__ex && r == R_NOTFOUND) EXCEPTION(EFOUND, -1, _tn);
        return r;
    }
    /// Rendszer paraméter név szerinti lekérdezése.
    /// @param _q
    /// @param __nm A paraméter neve
    /// @param __ex Ha értéke true és nem létezik a lekért paraméter, akkor dob egy kizárást.
    /// @return A kért paraméter értéke, vagy ha nem létezik és _ex false, akkor egy üres objektum.
    static QVariant getSysParam(QSqlQuery& _q, const QString& _nm, enum eEx __ex = EX_ERROR) {
        cSysParam   po;
        if (po.fetchByName(_q, _nm)) {
            return po.value(__ex);
        }
        if(__ex) EXCEPTION(EFOUND, -1, _nm);
        return QVariant();
    }
    static QString getTextSysParam(QSqlQuery& _q, const QString& _nm, const QString& __def = _sNul) {
        cSysParam   po;
        if (po.fetchByName(_q, _nm)) {
            return po.value().toString();
        }
        return __def;
    }
    static qlonglong getIntegerSysParam(QSqlQuery& _q, const QString& _nm, qlonglong __def = 0) {
        cSysParam   po;
        if (po.fetchByName(_q, _nm)) {
            bool ok;
            qlonglong r = po.value().toLongLong(&ok);
            if (ok) return r;
        }
        return __def;
    }
    static bool getBoolSysParam(QSqlQuery& _q, const QString& _nm, bool __def = false) {
        cSysParam   po;
        if (po.fetchByName(_q, _nm)) {
            QString r = po.value().toString();
            return str2bool(r);
        }
        return __def;
    }
    /// Az idő intervallumot egy qlonglong -ban tároljuk, és mSec-ben értendő.
    /// Bármilyen hiba esetén a default-al tér vissza.
    static qlonglong getIntervalSysParam(QSqlQuery& _q, const QString& _nm, qlonglong __def = 0) {
        cSysParam   po;
        qlonglong r = __def;
        if (po.fetchByName(_q, _nm)) {
            QVariant v = po.value(EX_IGNORE);
            if (variantIsInteger(v)) r = v.toLongLong();
        }
        return r;
    }
    STATICIX(cSysParam, ParamTypeId)
    STATICIX(cSysParam, ParamValue)
protected:
    /// A port paraméter értékhez tartozó típus rekord/objektum
    cParamType  paramType;
};

/* ------------------------------------------------------------------ */
/*!
@class cTpow
@brief Az (heti)időintervallumok napi időintervallum elemei

Az időpontok mindíg a helyi időre vonatkoznak.
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
public:
    bool isOnTime(QSqlQuery &q, const QDateTime& dt = QDateTime::currentDateTime());
    /// A megadott időpontról kideríti, hogy része-e az adott időintervallumoknak.
    /// @param q Query objektum az adatbázis művelethez
    /// @param id A timeperiods rekord/objektum azonosítója.
    /// @param dt A keresett időpont helyi idő szerint. Alapértelmezetten az aktuális időpont.
    /// @return Igaz, ha az időpont az időintervallumokon bellül van, egyébként hamis.
    /// @note Az időintervallumok mindíg helyi időre vonatkoznak. Az QTime helyi és UTC konverziója problémás, függ a dátumtól.
    static bool isOnTime(QSqlQuery &q, qlonglong id, const QDateTime& dt = QDateTime::currentDateTime());
    static bool isOnTime(QSqlQuery &q, const QString& name, const QDateTime& dt = QDateTime::currentDateTime());
    QDateTime nextOnTime(QSqlQuery &q, const QDateTime& dt = QDateTime::currentDateTime());
    /// Feltételezve, hogy a megadott időpont a megadott intervallumokon kívül van, megadja a legközelebbi időpontot
    /// Az itervallumokon bellül.
    /// @param q Query objektum az adatbázis művelethez
    /// @param id A timeperiods rekord/objektum azonosítója.
    /// @param dt A keresett időpont helyi idő szerint. Alapértelmezetten az aktuális időpont.
    /// @return A legközelebbi időintervallumokon bellüli időpont, helyi idő szerint.
    /// @note Az időintervallumok mindíg helyi időre vonatkoznak. Az QTime helyi és UTC konverziója problémás, függ a dátumtól.
    static QDateTime nextOnTime(QSqlQuery &q, qlonglong id, const QDateTime& dt = QDateTime::currentDateTime());
    static QDateTime nextOnTime(QSqlQuery &q, const QString& name, const QDateTime& dt = QDateTime::currentDateTime());
};

typedef tGroup<cTimePeriod, cTpow> tTimePeriodTpow;

#define NEVER_TIMEPERIOD_ID     -1LL
#define ALWAYS_TIMEPERIOD_ID     0LL

/* ******************************  ****************************** */

/*!
@class cImage
@brief Az images tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cImage : public cRecord {
    CRECORD(cImage);
public:
    /// Ha az image_data mező feltöltésekor törli a image_hash mezőt.
    /// Az image_hash mező, csak akkor törlődik, ha az image_data mezőt egyedileg modosítjuk,
    /// vagyis az objektum toEnd() (paraméter nélküli) metódusa az alapértelmezett metódus, ami nem cdinál semmit.
    bool toEnd(int _i) override;
    /// Betölt az objektumba egy képet
    /// @param __fn A kép fájl neve
    bool load(const QString& __fn, enum eEx __ex = EX_ERROR);
    /// Kiír egy képet egy fájlba
    /// @param __fn A kép fájl neve
    bool save(const QString& __fn, enum eEx __ex = EX_ERROR);
    /// A bináris adattartalmat állítja be
    void setImage(const QByteArray& __a)   { set(_ixImageData, QVariant(__a)); }
    /// Az kép típusának a nevét adja vissza
    const char * _getType() const    { return _imageType(int(getId(_ixImageType))); }
    /// Az kép típusának a nevét adja vissza
    const QString& getType() const   { return imageType(int(getId(_ixImageType))); }
    /// A típust állítja be.
    void setType(const QString& __t)   { set(_ixImageType, QVariant(__t)); }
    /// Ha az objektum tartalmaz bináris adatot, és típusa nem valamilyen kép, vagy NULL (vagyis BIN), akkor true értékkel tár vissza
    bool dataIsBin() const { return getId(_ixImageType) == IT_BIN && !isNull(_ixImageData); }
    /// Ha az objektum tartalmaz bináris adatot, és típusa valamilyen kép (nem BIN, vagy NULL), akkor true értékkel tár vissza
    bool dataIsPic() const {
        if (isNull(_ixImageType)) return false;
        if (getId(_ixImageType) >= IT_BIN) return false;
        if (isNull(_ixImageData)) return false;
        return true;
    }
    /// A bianaris adatot adja vissza
    QByteArray getImage() const     { return get(_ixImageData).toByteArray(); }
    /// A bianaris hash mező értékét adja vissza
    QByteArray getHash() const      { return get(_ixImageHash).toByteArray(); }
    /// Ha a imaga_data NULL, akkor true-val tér vissza
    bool dataIsNull() const         { return isNull(_ixImageData); }
    /// Ha a hash mező értéke NULL, akkor true-val térvissza. Elvileg az adatbázisban mindíg ki van töltve.
    /// Ha az objektumban modosítkuk a image_data mezőt, akkor törölva lessz a image_hash mező, melyet az
    /// az adatbázis logika kiíráskor majd kiszámol. és kitölt
    bool hashIsNull() const         { return isNull(_ixImageHash); }
    STATICIX(cImage, ImageType)
    STATICIX(cImage, ImageData)
    STATICIX(cImage, ImageHash)
};

#define UNKNOWN_PLACE_ID  0LL
#define ROOT_PLACE_ID     1LL
/*!
@class cPlace
@brief A places tábla egy rekordját reprezentáló osztály.
*/
class LV2SHARED_EXPORT cPlace : public cRecord {
    CRECORD(cPlace);
public:
    /// Megkeresi az első olyan szűlőt, melyhez hozzárendeltek térképet (image_id nem NULL),
    /// Ha talál ilyet, akkor az image_id-vel térvissza, ha nem akkor NULL_ID-vel.
    /// A get_parent_image() PLPGSQL függvényt hívja az ID-vel.
    qlonglong parentImageId(QSqlQuery& q);
    bool isUnknown() { return getId() <= UNKNOWN_PLACE_ID; }
    bool isRoot()    { return getId() == ROOT_PLACE_ID; }
    bool isValid()   { return getId() >  ROOT_PLACE_ID; }
    /// A helyiség kategória nevével tér vissza.
    /// A kategória egy category típusú place_groups rekord, egy helyiséghez
    /// elvileg egy category típusú plce_groups rekord rendelhető (nincs ellenörzés)
    /// Ha nincs hozzárendelt category típusú plce_groups rekord, akkor üres stringgel tér vissza.
    /// Ha több is van, akkor az egyik nevével, amit elöszőr visszaad az adatbázis.
    QString placeCategoryName();
    /// Egy hálózati elem nevéből konvertálja a gelyiség nevet, feltételezve, hogy az tartalmazza a helyiség nevét.
    /// A konverziót szabályozza a "node2place" rendszerváltozó:
    /// Ha ez nem létezik, akkor nincs konverzió. Ha létezik, akkor az egy egy tömb, aminek az első eleme
    /// a "upper", "lower", "insensitive", "sensitive" vagy üres string. A további elemek a
    /// a konverziót végző reguláris kifelyezések, amik a megadás sorrendjében lesznek illesztve a kiindulási névre.
    /// Ha nincs konverzió, akkor az egyébként üres objektum NOTE mezőbjébe írja a hiba üzenete.
    /// Ha van találat, akkor a talált place objektummal tér vissza.
    cPlace& nodeName2place(QSqlQuery& q, const QString& _nodeName);
};

EXT_ const QString _sNode2Place;

enum ePlaceGroupType {
    PG_GROUP,       ///< Grouping Places
    PG_CATEGORY,    ///< Categorize Places
    PG_ZONE         ///< It's a zone
};

EXT_ const QString& placeGroupType(int e, eEx __ex = EX_ERROR);
EXT_ int placeGroupType(const QString& s, eEx __ex = EX_ERROR);

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
    static qlonglong insertNew(QSqlQuery q, const QString& __n, const QString& __d, int _type);
    static qlonglong replaceNew(QSqlQuery q, const QString& __n, const QString& __d, int _type);
    static QStringList getAllZones(QSqlQuery q, QList<qlonglong> *pIds = nullptr, eEx __ex = EX_ERROR);
};

typedef tGroup<cPlaceGroup, cPlace> cGroupPlace;

#define NONE_PLACE_GROUP_ID 0LL
#define ALL_PLACE_GROUP_ID  1LL


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
    virtual void toEnd() override;
    /// További műveletek az egy mező kitöltése után:
    /// Ha a "vlan_id" idexével hívják, akkor ellenörzi a VLAN ID-t
    /// A negatív vagy 0 értékett NULL-ra állírja.
    virtual bool toEnd(int _i) override;
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
    STATICIX(cSubNet, NetAddr)
    STATICIX(cSubNet, VlanId)
    STATICIX(cSubNet, SubnetType)
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
@brief A ip_addresses tábla egy rekordját reprezentáló osztály.
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
    /// Ez a hívás nem támogatott, kizárást dob
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Ez a hívás nem támogatott, kizárást dob
    virtual int replace(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// A cím mező értékének a lekérése.
    QHostAddress address() const;
    /// Ha nincs subnet a megadott címhez, akkor a típust 'external'-ra állítja
    /// Ha nincs valós cím az objektumban akkor nem csinál semmit.
    /// @return true, ha a típust 'external'-ba állította, egyébként false.
    bool thisIsExternal(QSqlQuery &q);
    /// \brief thisIsJoint
    /// Ha talál ugyan ilyen címet, aminek a típusa 'joint', akkor a típust 'joint'-ra állítja.
    /// \param q
    /// \param _nodeId  Ha egy beregisztrált elem cím rekordja, akkor annak ID-je (önmagával nem ütközhet)
    /// \return  TS_NULL, ha nem talált ugyan ilyen címet, TS_FALSE, ha talált, de az nem volt 'joint' típusú,
    ///          végül, ha talált címet, az 'joint' típusú volt, és az objektum típusát is erre állította, akkor TS_TRUE.
    eTristate thisIsJoint(QSqlQuery &q, qlonglong _nodeId = NULL_ID);
    ///
    /// \brief lookupAll
    /// \param hn
    /// \param __ex
    /// \return
    static QList<QHostAddress> lookupAll(const QString& hn, enum eEx __ex = EX_ERROR);
    static QString lookup(const QHostAddress& ha, enum eEx __ex = EX_ERROR);
    static QHostAddress lookup(const QString& hn, enum eEx __ex = EX_ERROR);
    QHostAddress setIpByName(const QString& _hn, const QString &_t = _sNul, enum eEx __ex = EX_ERROR);
    STATICIX(cIpAddress, PortId)
    STATICIX(cIpAddress, Address)
    STATICIX(cIpAddress, SubNetId)
    STATICIX(cIpAddress, IpAddressType)
};

/* ******************************  ****************************** */

class cNPort;

/// Port paraméter érték
class LV2SHARED_EXPORT cPortParam : public cRecord {
    // friend class tOwnParams<cPortParam>;
    friend class cPPort;
    friend class cNPort;
    friend class cInterface;
    CRECORD(cPortParam);
public:
    /// Törli a paramType adattagot.
    virtual void    clearToEnd() override;
    /// A toEnd(int i) metódust hívja a port paraméter (típus) rekord id mező indexével.
    virtual void 	toEnd () override;
    /// Ha megváltozik a port param (típus) id, akkor betölti, vagy törli a megfelelp értéket a paramType adattagba.
    /// Nincs ilyen id-vel port_params rekord (és nem NULL az id), akkor a statusban bebillenti az ES_DEFECZIVE bitet.
    virtual bool 	toEnd (int i) override;
    /// A port paraméter nevével tér vissza
    QString typeName()   const { return paramType.getName(); }
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
    STATICIX(cPortParam, ParamTypeId)
    STATICIX(cPortParam, PortId)
protected:
    /// A port paraméter értékhez tartozó típus rekord/objektum
    cParamType  paramType;
};

// / Port paraméter értékek listája
// typedef tOwnParams<cPortParam> cPortParams;
/* ------------------------------------------------------------------------ */

/// A port típus a link alapján
enum eLinkType {
    LT_PTP = 0,         ///< Point to point /pecselhető
    LT_BUS,             ///< BUS (egyenlőre ugyanúgy kezeljük mint a ptp -t)
    LT_PATCH,           ///< Patch port Frot<-->Back / pecselhető az előlap és a hátlap is
    LT_LOGICAL,         ///< Nincs link / nem pecselhető
    LT_WIRELESS,        ///< Nincs link / nem pecselhető
    LT_UNKNOWN          ///< Nincs link / nem pecselhető
};

EXT_ int linkType(const QString& __n, eEx __ex = EX_ERROR);
EXT_ const QString&  linkType(int __e, eEx __ex = EX_ERROR);

enum ePortObjType {
    PO_NPORT = 0,
    PO_PPORT,
    PO_INTERFACE,
    PO_UNKNOWN
};

EXT_ int portObjeType(const QString& __n, eEx __ex = EX_ERROR);
EXT_ const QString&  portObjeType(int __e, eEx __ex = EX_ERROR);

class LV2SHARED_EXPORT cIfType : public cRecord {
    CRECORD(cIfType);
public:
    /// Csak annyiban különbözik az ős osztály metódusátol, hogy sikeres írás esetén törli a iftypes konténert,
    /// hogy ott is megjelenhesen az új adat, mivel ezzel kikényszerítjük a tábla újraolvasását
    virtual bool insert(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Csak annyiban különbözik az ős osztály metódusátol, hogy sikeres írás esetén modosítja a beolvasott
    /// rekordot is, ha volt.
    virtual int update(QSqlQuery &__q, bool __only, const QBitArray &__set = QBitArray(), const QBitArray &__where = QBitArray(), enum eEx __ex = EX_ERROR) override;
    ///
    static qlonglong insertNew(QSqlQuery &__q, bool _ir, const QString& __nm, const QString& __no, int __iid, int __lid);
    static qlonglong insertNew(QSqlQuery &__q, bool _ir, const QString& __nm, const QString& __no, int __iid, const QString& __ot, const QString& __lt, bool _pr);
protected:
    /// Az összes if_types rekordot tartalmazó konténer
    /// Nem frissül automatikusan, ha változik az adattábla.
    static tRecordList<cIfType> _ifTypes;
    /// Egy üres objektumra mutató pointer.
    /// Az ifTypes feltöltésekor hozza létre az abjektumot a fetchIfTypes();
    static cIfType *_pNull;
public:
    /// Feltölti az adatbázisból az ifTypes adattagot, ha nem üres a konténer, akkor frissíti.
    /// Iniciaéizálja a pNull pountert, ha az NULL. Egy üres objektumra fog mutatni.
    /// Frissítés esetén feltételezi, hogy rekordot törölni nem lehet, ezt ellenörzi.
    static void fetchIfTypes(QSqlQuery& __q);
    /// Egy cIfType objektumot ad vissza a név alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke hamis, akkor kizárás helyett, egy üres objektummal tér vissza (amire a pNull adattag mutat)
    static const cIfType& ifType(const QString& __nm, enum eEx __ex = EX_ERROR);
    /// Egy ifTypes objektumot ad vissza az ID alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke hamis, akkor kizárás helyett, egy üres objektummal tér vissza (amire a pNull adattag mutat)
    static const cIfType& ifType(qlonglong __id, enum eEx __ex = EX_ERROR);
    /// Egy ifTypes objektum Azonosítóját adja vissza a név alapján, ha nincs ilyen nevű típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke EX_IGNORE, akkor kizárás helyett, NULL_ID -vel tér vissza.
    static qlonglong ifTypeId(const QString& __nm, enum eEx __ex = EX_ERROR) { return ifType(__nm, __ex).getId(); }
    /// Egy ifTypes objektum nevét adja vissza az ID alapján, ha nincs ilyen azonosítójú típus, akkor dob egy kizárást.
    /// Az ifTypes adattagban keres, ha ifTypes üres, akkor feltölti az adatbázisból
    /// Ha __ex értéke EX_IGNORE, akkor kizárás helyett, üres stringgel tér vissza.
    static QString   ifTypeName(qlonglong __id, enum eEx __ex = EX_ERROR)    { return ifType(__id, __ex).getName(); }
    /// Visszakeresi az ifTypes konténer azon elemét, melynek az iftype_iana_id értéke megeggyezik a
    /// a paraméterben magadott értékkel, és a preferred értéke true. Ha van ilyen objektum a
    /// konténerben, akkor az első pointerével, ha nincs NULL pointerrel tér vissza.
    /// Ha a talált rekordban az iana_id_link mező nem NULL, és recursive nem false, akkor annak értékével hívja a metódust rekorzívan.
    /// A rekurzió csak egyszeres.
    /// @param _iana_id A kereset IANA ID
    /// @param recursive Ha értéke true (ez az alapértelmezés) akkor ha iana_id_link nem NULL, akkor azzal az IANA id-vel hívja önmagát rekurzívan, maximum egyszer.
    static const cIfType *fromIana(int _iana_id, bool recursive = true, bool preferdOnly = true);
    ///
    bool isLinkage() const {
        eLinkType lt = eLinkType(getId(_sIfTypeLinkType));
        return lt == LT_PTP || lt == LT_BUS || lt == LT_PATCH;
    }
    static int size() { return _ifTypes.size(); }
    static const cIfType * at(int i) { return isContIx(_ifTypes, i) ? _ifTypes.at(i) : nullptr; }
protected:
    /// Ha nincs feltöltve az ifTypes adattag , akkor feltölti az adatbázisból,
    /// Vagyis hívja a void fetchIfTypes(QSqlQuery& __q); metódust.
    static void checkIfTypes() { if (_pNull == nullptr) { QSqlQuery q = getQuery(); fetchIfTypes(q); } }
};

/* ======================================================================== */

enum eContainerValid {
    CV_NODE_PARAMS      = 0x0100,
    CV_PORTS            = 0x0200,
    CV_PORT_PARAMS      = 0x0400,
    CV_PORTS_ADDRESSES  = 0x0800,
    CV_PORT_VLANS       = 0x1000,
    CV_PATCH_BACK_SHARE = 0x2000,
    CV_ALL_PATCH        = CV_NODE_PARAMS | CV_PORTS | CV_PORT_PARAMS,
    CV_ALL_NODE         = CV_ALL_PATCH,
    CV_ALL_HOST         = CV_ALL_NODE | CV_PORTS_ADDRESSES | CV_PORT_VLANS
};


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
    explicit cNPort(no_init_& __dummy) : cRecord(), params(this, _sNul) {
        (void)__dummy;
        cNPort::descr();
    }
public:
    /// Törli a params konténert.
    virtual void clearToEnd() override;
    /// A port id indexével hívja a toEnd(int i) metódust
    virtual void toEnd() override;
    /// Ha a port id indexével hívtuk, akkor ha szükséges törli a params konténert, lásd az atEndCont() metódust.
    virtual bool toEnd(int i) override;
    /// Beszúr egy port rekordot az adatbázisba a járulékos adatokkal (rekordokkal) együtt.
    virtual bool insert(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool rewriteById(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool isContainerValid(qlonglong __mask) const override;
    virtual void setContainerValid(qlonglong __set, qlonglong __clr = 0) override;

    /// Egy port rekord beolvasása a port név és a node id alapján.
    /// @param __q Az adatbázis műveletjez haszbált objektum.
    /// @param __port_name A keresett port neve
    /// @param __node_id A hálózati elem ID, melynek a portját keressük
    /// @return Ha egy rekordot beolvasott akkor true.
    bool fetchPortByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id);
    /// Egy port rekord beolvasása/objektum feltültése a port név és a node id, ill. opcionálisan a port típus alapján.
    /// @param __q Az adatbázis műveletjez haszbált objektum.
    /// @param __port_name A keresett port neve
    /// @param __node_id A hálózati elem ID, melynek a portját keressük
    /// @return Az objektum referencia
    /// @exceptions Ha nincs ilyen port.
    cNPort& setPortByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id) {
        if (!fetchPortByName(__q, __port_name, __node_id)) {
            QString s = __port_name;
            EXCEPTION(EDATA,__node_id, s);
        }
        return *this;
    }
    /// Egy port id  lekérése a port név és a node_id alapján.
    static qlonglong getPortIdByName(QSqlQuery& __q, const QString& __port_name, qlonglong __node_id, enum eEx __ex = EX_ERROR);
    /// Egy port rekord beolvasása a port név és a node neve alapján.
    /// @param __ex Ha értéke false, és a megadott nevű node nem létezik, akkor dob egy kizárást, egyébként visszatér false-val.
    /// @return Ha egy rekordot beolvasott akkor true
    bool fetchPortByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, enum eEx __ex = EX_ERROR);
    /// Egy port rekord beolvasása a port név és a node neve alapján.
    /// Ha nem tudja beolvasni a rekordot, akkor dob egy kizárást.
    cNPort& setPortByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name) {
        if (!fetchPortByName(__q, __port_name, __node_name)) {
            QString s = QString("%1:%2").arg(__node_name,__port_name);
            EXCEPTION(EDATA, -1, s);
        }
        return *this;
    }
    /// Egy port id  lekérése a port név és a node név alapján.
    static qlonglong getPortIdByName(QSqlQuery& __q, const QString& __port_name, const QString& __node_name, enum eEx __ex = EX_ERROR);
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
    static qlonglong getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, qlonglong __node_id, enum eEx __ex = EX_ERROR);
    /// Egy port rekord beolvasása a port index és a node név alapján.
    bool fetchPortByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name, enum eEx __ex = EX_ERROR);
    /// Egy port rekord beolvasása a port index és a node_id alapján.
    /// Ha nem tudja beolvasni a rekordot, akkor dob egy kizárást.
    cNPort& setPortByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name) {
        if (!fetchPortByIndex(__q, __port_index, __node_name))
            EXCEPTION(EDATA, __port_index, __node_name);
        return *this;
    }
    /// Egy port id  lekérése a port index és a node_id alapján.
    static qlonglong getPortIdByIndex(QSqlQuery& __q, qlonglong __port_index, const QString& __node_name, enum eEx __ex = EX_ERROR);

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
    static cNPort * getPortObjById(QSqlQuery& q, qlonglong __port_id, enum eEx __ex = EX_ERROR);
    /// Allokál és beolvas egy port objektumot a megadott port_id alapján.
    /// Az allokált objektum típusa megfelel a megadott tábla OID-nek, ha a tábla az nports, pports vagy interfaces.
    /// @param __port_id A port rekord id-je (port_id)
    /// @param __tableoid A típus azonosító tábla OID
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cNPort * getPortObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __port_id, enum eEx __ex = EX_ERROR);
    /// Az objektum iftpes rekordjával tér vissza. Ha az iftype_id mező null, vagy nem egy létező iftpes rekord ID-je,
    /// akkor dob egy kizárást, vagy üres objektummal tér vissza.
    /// @param __ex Hiba esetén nem üres objektummal tér vissza, hanem dob egy kizárást, ha értéke igaz (ez az alapértelmezés).
    const cIfType& ifType(enum eEx __ex = EX_ERROR) const { return cIfType::ifType(getId(_sIfTypeId), __ex); }
    /// Az iftype hívással lekéri az iftypes rekordot, és összehasonlítja a megadott névvel, a rekord név mezőjét,
    /// Ha azonos true-val tér vissza.
    /// @param __iftypename A keresett név.
    /// @param __ex Hiba esetén dob egy kizárást, ha igaz (ez az alapértelmezés). Ha false, akkor hiba esetén false-vel tér vissza,
    ///           hacsak az __iftypename nem null.
    bool isIfType(const QString& __iftypename, enum eEx __ex = EX_ERROR) const { return ifType(__ex).getName() == __iftypename; }
    /// Feltölti az adatbázisból a params konténert.
    int fetchParams(QSqlQuery& q);
    /// Egy névvel térvissza, mely alapján a port egyedileg azonosítható.
    /// A nevet a node és a port nevéből állítja össze, szeparátor a ':' karakter
    /// Az objektumnak csak a node_id mezője és a név mezője alapján állítja elő a visszaadott értéket.
    /// @param q Az adatbázis lekérdezéshez használt query objektum.
    QString getFullName(QSqlQuery& q, enum eEx __ex = EX_ERROR) const;
    static QString getFullNameById(QSqlQuery& q, qlonglong _id);
    static int delPortByName(QSqlQuery &q, const QString &_nn, const QString &_pn, bool __pat);
    /// Port paraméterek, nincs automatikusan feltöltve
    tOwnRecords<cPortParam, cNPort>   params;
    ///
    eTristate getBoolParam(const QString &name, eEx __ex = EX_ERROR) const;
protected:
    ///
    static qlonglong _tableoid_nports;
    static qlonglong _tableoid_pports;
    static qlonglong _tableoid_interfaces;
public:
    static QString catFullName(const QString& nodeName, const QString& portName) {
        return nodeName + ":" + portName;
    }
    static qlonglong tableoid_nports()      { return _tableoid_nports; }
    static qlonglong tableoid_pports()      { return _tableoid_pports; }
    static qlonglong tableoid_interfaces()  { return _tableoid_interfaces; }
    STATICIX(cNPort, NodeId)
    STATICIX(cNPort, PortIndex)
    STATICIX(cNPort, IfTypeId)

};

EXT_ bool portLessThanByIndex(const cNPort * pp1, const cNPort * pp2);
EXT_ bool portLessThanByName(const cNPort * pp1, const cNPort * pp2);

class cPatch;

enum ePortShare {
    ES_INVALID = ENUM_INVALID,
    ES_     = 0,
    ES_A,   ES_AA, ES_AB,
    ES_B,   ES_BA, ES_BB,
    ES_C,   ES_D,
    ES_NC,
    ES_COUNT
};

/// A portshare SQL enumeráció típusból  a ePortShare típusba konverziós függvény.
/// A konverzió nem kisbetű-nagybetű érzékeny.
/// @param _n Az SQL enumerációs érték
/// @param __ex Ha értéke true, akkor ha nem lehetséges a konverzió, akkor dob egy kizárást.
/// @return Az SQL értéknek megfelelő konstan érték, vagy -1.
EXT_ int portShare(const QString& _n, enum eEx __ex = EX_ERROR);
/// A portshare SQL enumeráció típusba  a ePortShare típusból konverziós függvény.
/// @param _n Az enumerációs konstans
/// @param __ex Ha értéke true, akkor ha nem lehetséges a konverzió, akkor dob egy kizárást.
/// @return A konstansnak megfelelő SQL érték, vagy üres string, mivel ez azonos egy SQL értékkel,
///         ezért hiba detektálásra a visszaadott érték alkalmatlan.
EXT_ const QString& portShare(int _i, enum eEx __ex = EX_ERROR);

EXT_ ePortShare shareResultant(ePortShare _sh1, ePortShare _sh2);
EXT_ ePortShare shareConnect(ePortShare _sh1, ePortShare _sh2);


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

EXT_ int ifStatus(const QString& _n, enum eEx __ex = EX_ERROR);
EXT_ const QString& ifStatus(int _i, enum eEx __ex = EX_ERROR);

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
    virtual void clearToEnd() override;
    virtual void toEnd() override;
    virtual bool toEnd(int i) override;
    virtual bool insert(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool rewriteById(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual QString toString() const override;
    // A trunkMembers konténer adattaghoz hozzáad egy port indexet
    void addTrunkMember(int __ix) { trunkMembers << __ix;  }
    int updateTrunkMembers(QSqlQuery& q, enum eEx __ex = EX_ERROR);
    /// A trunkMembers adattag konstans referenciájával tér vissza.
    /// Ha a trunkMembers fel van töltve, akkor a konténer TRUNK port esetén a trunk tagjainak az indexét tartalmazza (port_index mező értéke).
    const tIntVector& getTrunkMembers() const { return trunkMembers; }
    void joinVlan(qlonglong __id, enum eVlanType __t, enum eSetType __st = ST_MANUAL);
    bool splitVlan(qlonglong __id);
    /// Beolvas egy objektumot/rekordot a MAC alapján
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett MAC
    /// @return a találatok száma, ha nulla, akkor az objektum üres lessz, egyébként az első rekorddal lessz feltöltve.
    int fetchByMac(QSqlQuery& q, const cMac& a);
    /// Beolvas egy objektumot/rekordot a IP cím alapján
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett cím
    /// @return A találatok száma, ha nulla, akkor az objektum üres lessz, egyébként az első rekorddal lessz feltöltve.
    int fetchByIp(QSqlQuery& q, const QHostAddress& a);
    /// Feltölti a vlans konténert
    int fetchVlans(QSqlQuery& q);
    /// Feltölti at addresses konténert.
    int fetchAddressess(QSqlQuery& q);
    /// Egy cím hozzáadása az interfészhez.
    /// @param __a A cím objektum (rekord)
    cIpAddress& addIpAddress(const cIpAddress& __a);
    /// Egy cím hozzáadása az interfészhez.
    /// @param __a Az ip cím.
    /// @param __t Az IP cím típusa, alapértelmezett a NULL
    /// @param __d Cím rekord megjegyzés mező, alapértelmezett a NULL
    cIpAddress& addIpAddress(const QHostAddress& __a, const QString& __t = _sNul, const QString& __d = _sNul);
    /// Egy cím hozzáadása az interfészhez.
    /// @param __a Az ip cím.
    /// @param __t Az IP cím típusa (numerikus), alapértelmezett a NULL, minden negatív érték NULL-ként van értelmezve.
    /// @param __d Cím rekord megjegyzés mező, alapértelmezett a NULL
    cIpAddress& addIpAddress(const QHostAddress& __a, int __t, const QString& __d = _sNul)
    {
        return addIpAddress(__a, __t < 0 ? _sNul : addrType(__t), __d);
    }

    /// Értékadása a hwaddress mezőnek.
    cInterface& operator=(const cMac& _m) { set(_sHwAddress, QVariant::fromValue(_m)); return *this; }

    /// VLAN hozzárendejések, nincs automatikus feltöltés
    tOwnRecords<cPortVlan, cInterface>  vlans;
    /// Ip címek listája (nincs automatikus feltöltés
    tOwnRecords<cIpAddress, cInterface> addresses;
    /// A MAC cím mező indexe
    STATICIX(cInterface, HwAddress)
    cMac mac() { return getMac(_ixHwAddress); }
// protected:
    /// Trunk port esetén a trunk tagjainak az indexe (port_index mező)
    /// Nincs automatikusan feltöltve.
    tIntVector trunkMembers;
};

/* ****************************** NODES (patchs, nodes, snmphosts ****************************** */

class cNode;

/// Node paraméter érték
class LV2SHARED_EXPORT cNodeParam : public cRecord {
    friend class cNode;
    CRECORD(cNodeParam);
public:
    /// Törli a paramType adattagot.
    virtual void    clearToEnd() override;
    /// A toEnd(int i) metódust hívja a Node paraméter (típus) rekord id mező indexével.
    virtual void 	toEnd () override;
    /// Ha megváltozik a Node param (típus) id, akkor betölti, vagy törli a megfelelp értéket a paramType adattagba.
    /// Nincs ilyen id-vel Node_params rekord (és nem NULL az id), akkor a statusban bebillenti az ES_DEFECZIVE bitet.
    virtual bool 	toEnd (int i) override;
    /// A Node paraméter nevével tér vissza
    QString typeName()   const { return paramType.getName(); }
    /// A Node paraméter típus enumerációs értékkel tér vissza
    qlonglong type()   const { return paramType.getId(_sParamTypeType); }
    /// A Node paraméter dimenzió ill. mértékegység nevével tér vissza
    QString dim()    const { return paramType.getName(__sParamTypeDim); }
    /// A Node paraméter értékére mutató referencia objektummal tér vissza.
    cRecordFieldConstRef value() const { return (*this)[_sParamValue]; }
    /// A Node paraméter értékére mutató referencia objektummal tér vissza.
    cRecordFieldRef      value()       { return (*this)[_sParamValue]; }
    /// A paraméter típusának a beállítása
    /// @param __id A paraméter típus rekord ID-je
    cNodeParam& setType(qlonglong __id)        { set(_ixParamTypeId, __id); return *this; }
    /// @param __n A paraméter típus rekord név mezőjenek az értéke
    cNodeParam& setType(const QString& __n)    { paramType.fetchByName(__n); _set(_ixParamTypeId, paramType.getId()); return *this; }
    /// Értékadás az érték mezőnek.
    cNodeParam& operator=(const QString& __v)  { setName(_sParamValue, __v); return *this; }
    /// A Node paraméter értékhez tartozó típus rekord/objektum
    cParamType  paramType;
    STATICIX(cNodeParam, ParamTypeId)
    STATICIX(cNodeParam, NodeId)
};

/// @class cShareBack
/// @brief Egy hátlapi megosztást reprezentáló objektum.
class LV2SHARED_EXPORT cShareBack {
protected:
    int     a;  ///< Az A vagy AA (hátlapi) megosztott, vagy a nem bekötött port sorszáma a ports konténerben.
    int     ab; ///< Az AB (hátlapi) megosztott port  sorszáma a ports konténerben, ha nincs a megadott megosztás, akkor negatív.
    int     b;  ///< Az B vagy BA (hátlapi) megosztott port  sorszáma a ports konténerben, ha nincs a megadott megosztás, akkor negatív.
    int     bb; ///< Az BB (hátlapi) megosztott port  sorszáma a ports konténerben, ha nincs a megadott megosztás, akkor negatív.
    /// Ha AB ill. BA megosztás is van, és értéke true, akkor azok típusa C és D
    /// Ha csak A van megadva, és értéke true, akkor a port nincs bekötve, egyébbként érdektelen.
    bool    cd;
public:
    /// Üres konstruktor.
    cShareBack() { a = ab = b = bb = -1; cd = false; }
    /// Konstruktor, az adattagok értékeivel. Nem adható meg két azonos port_index (csak ha az NULL_ID)
    cShareBack(int __a, int __b, int __ab = -1, int __bb = -1, bool __cd = false) { set(__a, __b, __ab, __bb, __cd); }
    /// Copy operátor. Mivel ellenörzi az értékeket az öres objektum nem másolható.
    cShareBack& operator=(const cShareBack& __o) { return set(__o.a, __o.b, __o.ab, __o.bb, __o.cd); }
    /// Az objektum feltöltése, két azonos index nem adható meg,
    /// az A ill AA típusú megosztású port sorszáma a ports konténerben, nem lehet negatív!
    cShareBack& set(int __a, int __b, int __ab = -1, int __bb = -1, bool __cd = false);
    /// Copy konstruktor. Nem ellenöriz.
    cShareBack(const cShareBack& __o)            { a = __o.a; ab = __o.ab; b = __o.b; bb = __o.bb; cd = __o.cd; }
    /// True, ha a két objektum a,b,ab,bb adattagjaiból bármelyik, bármelyikkel egyenlő, és nem NULL_ID.
    bool operator==(const cShareBack& __o) const;
    /// True, ha az objektum a,b,ab,bb adattagjaiból bármelyikel egyenlő __o -val, és __o nem negatív.
    bool operator==(int __i) const;
    int getA() const    { return a;  }
    int getB() const    { return b;  }
    int getAB() const   { return ab; }
    int getBB() const   { return bb; }
    /// Ha AA C D BB mehosztásról van szó, akkor true-val tér vissza
    bool isCD() const   { return cd && (ab < 0 || b < 0 || bb < 0); }
    /// Ha az objektum azt kelzi, hogy az egy magadott (A) port nincs bekötve, akkor true-val tér vissza.
    bool isNC() const   { return cd && (ab < -1 && b < 0 && bb < 0); }
};
inline uint qHash(const cShareBack& sh) { return uint(qHash(sh.getA())); }

/*!
@class cPatch
@brief A patchs tábla egy rekordját reprezentáló osztály.

Patch panel, falicsatlakozó, ill. egyébb csatlakozo ill. csatlakozó csoport objektum.
 */
class LV2SHARED_EXPORT cPatch : public cRecord {
    Q_OBJECT
    CRECORD(cPatch);
protected:
    /// Konstruktor a leszármazott osztályokhoz
    explicit cPatch(no_init_&) : cRecord(), ports(this, _sNul), params(this, _sNul), pShares(nullptr)
    {
        cPatch::descr();
        containerValid = 0;
    }
    cNPort *portSetParam(cNPort * __port, const QString& __name, const QVariant &__val, const QString& __type);
public:
    virtual void clearToEnd() override;
    virtual void toEnd() override;
    virtual bool toEnd(int i) override;
    virtual bool insert(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Egy létező az ID-vel azonosított patch objektum újra írása.
    /// A portok újra írása (ha isContainerValid(CV_PORTS) hívás értéke true) szintén az ID szerint történik.
    /// A metódus bármilyen módosítást nem tud kiírni az adatbázisba, az egymást követő írási műveletekben
    /// nem lehet név ütközés, pl. ez fog történni port nevek felcserélése esetén.
    /// @note A portok kiírásának a módja különbözik a cNode típusú objektumokhoz képest!
    virtual bool rewriteById(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool isContainerValid(qlonglong __mask) const override;
    virtual void setContainerValid(qlonglong __set, qlonglong __clr = 0) override;
    virtual void insertPort(QSqlQuery& q, int ix, const QString& _na, const QString& _no, const QString& _tag = QString());
    virtual void updateShared(QSqlQuery& q, int __a, int __ab, int __b, int __bb, bool __cd);
    /// Törli az ID mezöket, de az objektum egyébb adattartalma nem változik.
    /// Nem lesznek vérehajtva az toEnd() ill. clearToEnd(); metódusok.
    void clearIdsOnly();

    /// Kitölti a ports konténer adattagot. A node-hoz tartozó összes portot beolvassa a kontéberbe.
    /// Feltételezi, hogy a node_id mevű maző, vagyis a rekord ID ki ban töltve.
    /// @param __q Az adatbázis művelethet használt query objektum.
    /// @return A baeolvasott portok száma.
    virtual int fetchPorts(QSqlQuery& __q, int flags = 0);
    /// Feltölti az adatbázisból a params konténert.
    int fetchParams(QSqlQuery& q);
    /// Paraméter létrehozása, vagy értékének a modosítása
    void setParam(const QString& __name, const QVariant& __val, const QString &__type);

    // Csak a cPatch-ban támogatott virtualis metódusok, a cNode-ban ujraimplementált metódusok kizárást dobnak.
    /// Törli a port bekötéseket tartalmazó konténert. Amennyiben a konténer még nincs megallokálva, akkor megallokálja az üres konténert.
    virtual void clearShares();
    /// Beállít egy (hátlapi) kábel megosztást. A ports adattagot fel kell tölteni a hívás elött!
    /// @param __a    Az A vagy AA típusú megosztott, vagy nem bekötött port sorszáma a ports konténerben
    /// @param __ab   Az AB vagy C típusú megosztott port sorszáma a ports konténerben, ha értéke negatív, akkor __a egy A típusú megosztást jelöl
    /// @param __b    Az B vagy BA típusú megosztott port sorszáma a ports konténerben
    /// @param __bb   Az BB vagy D típusú megosztott port sorszáma a ports konténerben, ha értéke negatív, akkor __b egy B típusú megosztást jelöl
    /// @param __cd   Ha __ab vagy __bb nem negatív, és __cd értéke true, akkor __ab és __bb egy C ill. D típusú megosztást jelöl.
    ///               Ha __ab, __b és __bb is negatív, és __cd értéke true, akkor az __a a indexű port nincs bekütbe.
    /// @return true, ha beállította a megosztásokat a ports objektumon, és false, ha hiba történt.
    virtual bool setShare(int __a, int __ab = -1, int __b = -1, int __bb = -1, bool __cd = false);
    /// A megosztást is kiírja az adatbázisba.
    virtual bool updateShare(QSqlQuery& __q, const cShareBack& s, eEx __ex = EX_ERROR);
    /// A megosztásokat is kiírja az adatbázisba.
    /// Feltételezi, hogy a ports konténerbe az adatbázis tartalma be lett olvasva
    /// @param __q Az adatbázis művelethet használt query objektum.
    /// @param __clr Alapértelmezetten a hívás csak az eltérő bekötéseket adja hozzá az adatbázis tartamához.
    ///              Ha a paraméter értéke true, akkor elötte törli az adatbázisban a node-hoz tartozó összes eltérő bekötést.
    virtual bool updateShares(QSqlQuery& __q, bool __clr = false, enum eEx __ex = EX_ERROR);
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
    cNPort *portSetParam(const QString& __port, const QString& __name, const QVariant& __val, const QString& __type) {
        return portSetParam(getPort(__port), __name, __val, __type);
    }

    /// A sorszám/index szerint megadott port paraméter hozzáadása ill. modosítása
    /// @param __port_index A port sorszáma ill. indexe
    /// @param __par A paraméter neve
    /// @param __val A paraméter új értéke.
    cNPort *portSetParam(int __port_index, const QString& __name, const QVariant& __val, const QString& __type) {
        return portSetParam(getPort(__port_index), __name, __val, __type);
    }

    /// A sorszám/index szerint megadott portok paraméter hozzáadása ill. modosítása
    /// @param __port_index A port sorszáma ill. indexe
    /// @param __par A paraméter neve
    /// @param __val A paraméter új értékei, rendre a következő surszámú porthoz rendelve.
    cNPort *portSetParam(int __port_index, const QString& __name, const QVariantList& __val, const QString& __type);

    /// A port keresése az index mező értéke alapján.
    cNPort * getPort(int __ix, enum eEx __ex = EX_ERROR);
    /// A port keresése a port név alapján.
    cNPort * getPort(const QString& __pn, enum eEx __ex = EX_ERROR);
    /// Az ID alapján az eredeti rekord leírót adja vissza
    const cRecStaticDescr *getOriginalDescr(QSqlQuery &q, eEx __ex = EX_ERROR);
    /// Allokál és beolvas egy node objektumot a megadott node_id alapján.
    /// Az allokált objektum típusa megfelel a táblának, ha a rekor a patchs nodes, snmpdevices táblában van.
    /// @param __node_id A node rekord id-je (node_id)
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cPatch * getNodeObjById(QSqlQuery& q, qlonglong __node_id, enum eEx __ex = EX_ERROR);
    /// Allokál és beolvas egy node objektumot a megadott node_id alapján.
    /// Az allokált objektum típusa megfelel a táblának, aminek a tableoid értékét megadtuk.
    /// @param __node_id A node rekord id-je (node_id)
    /// @param __tableoid Az allokálandó objektum típusát, ill. a hozzázatozó táblát azonosító OID
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cPatch * getNodeObjById(QSqlQuery& q, qlonglong __tableoid, qlonglong __node_id, enum eEx __ex = EX_ERROR);
    /// Allokál és beolvas egy node objektumot a megadott node_name alapján.
    /// Az allokált objektum típusa megfelel a táblának, ha a rekor a patchs,  nodes, snmpdevices táblában van.
    /// @param __node_name A node rekord neve (node_name)
    /// @param __ex Ha értéke true, és nem találja a rekordot, tableoid-t akkor dob egy kizárást. Egyébként NULL-lal tér vissza
    static cPatch * getNodeObjByName(QSqlQuery& q, const QString&  __node_name, enum eEx __ex = EX_ERROR);

    /// Egy node neve alapján visszaadja az id-t
    static qlonglong getNodeIdByName(QSqlQuery& q, const QString& _n, enum eEx __ex = EX_ERROR) {
        return cPatch().getIdByName(q, _n, __ex);
    }

    /// A konténer referenciáját adja vissza, amire a pShare mutat.
    /// Ha pShare értéke NULL, akkor ezt egy leszármazottból hívtuk, ahol ez a hívás nem támogatott,
    /// ezért a metódus dob egy kizárást.
    QSet<cShareBack>& shares() {
        if (pShares == nullptr) EXCEPTION(ENOTSUPP);
        return *pShares;
    }

    bool addShare(cShareBack& s) {
        foreach (cShareBack ss, shares()) {
            if (ss == s) return false;
        }
        *pShares << s;
        return true;
    }

    /// A ports portokon törli az összes megosztást (hátlapi megosztások)
    /// Ha ports nincs feltöltve, akkor nem csinál semmit.
    static qlonglong   patchPortTypeId() { (void)cPPort(); return cPPort::_ifTypePatch; }

    /// Az objektum portjai, nincs automatikus feltöltés
    /// Ha módosítjuk az ID-t akkor törlődhet lásd az atEndCont() metódust
    tOwnRecords<cNPort, cPatch>     ports;
    /// Node paraméterek, nincs automatikusan feltöltve
    tOwnRecords<cNodeParam, cPatch> params;
    // int containerValid;

    QString  getTextParam(qlonglong _typeId, eEx __ex = EX_ERROR) const;
    qlonglong getIntParam(qlonglong _typeId, eEx __ex = EX_ERROR) const;
    eTristate getBoolParam(qlonglong _typeId, eEx __ex = EX_ERROR) const;

    void sortPortsByIndex();
    void sortPortsByName();

protected:
    /// Megosztások konténer. (csak a cPatch osztályban)
    /// Nincs automatikusan feltöltve, de a clearToEnd(); törli, ill. az atEnd() törölheti.
    /// Csak a normál bekötéstől való eltérések esetén kerül egy elem a konténerbe.
    QSet<cShareBack> * pShares;
};

/// A sablon metódus a megadott ID-vel és a típus paraméternek megfelelő típusú
/// objektumot hozza létre (new) és tölti be az adatbázisból
template<class P> inline P * getObjByIdT(QSqlQuery& q, qlonglong  __id, enum eEx __ex = EX_ERROR)
{
    P *p = new P();
    p->setId(__id);
    if (p->completion(q) != 1) {
        delete p;
        if (__ex) EXCEPTION(EDATA);
        return nullptr;
    }
    return p;
}

/* ------------------------------------------------------------------------------------------------- */
#define NIL_NODE_ID 0

/*!
@class cNode
@brief A nodes tábla egy rekordját reprezentáló osztály.

A cPatch-bol származik. Bár müködése különbözik a cPatch-tól, és az objektum így tartalmaz
néhány felesleges komponenst, de igy nem kell eltérni az adatbázisban definiált származási sortol.
 */
class LV2SHARED_EXPORT cNode : public cPatch {
    CRECORD(cNode);
    FEATURES(cNode)
protected:
    explicit cNode(no_init_& _dummy) : cPatch(_dummy) {
        _pFeatures = nullptr;
        cNode::descr();
        bDelCollisionByIp = bDelCollisionByMac = false;
    }
public:
    cNode(const QString& __name, const QString& __note);
//  cNode& operator=(const cNode& __o);
    /// Törli a child objektumokat tartalmazó konténereket, és a containerValid értékét nullára állítja.
    virtual void clearToEnd() override;
    virtual void toEnd() override;
    /// Ha i az ID indexe, akkor megvizsgálja a child objektumok konténerét (csak az első elemet!)
    /// és, ha chiuld objektumban a port_id értéke nem azonos a node objektum aktuális ID-jével,
    /// akkor törli a konténert, és a megfelelő containerValid bit(ek)et.
    virtual bool toEnd(int i) override;
    /// A cPatch::insert(QSqlQuery &__q, eEx __ex) hívása elött beállítja a
    /// node_type értékét, ha az NULL.
    virtual bool insert(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Az objektum frissítése után hívja a rewrite() metódust a
    /// paraméter ás port konténerre is.
    /// Az újra írandó objektum azonosítása név szerint történik, az objektum ID mezője figyelmenkívül lasz hagyva.
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Az objektum frissítése után hívja a rewrite() metódust a
    /// paraméter ás port konténerre is.
    /// Az újra írandó objektum azonosítása ID szerint történik.
    virtual bool rewriteById(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Kitölti a ports adattagot, hiba esetén dob egy kizárást.
    /// Alapértelmezetten ha a port típusa cInterface, akkor az IP címeket és vlan-okat is, de a port paramétereket nem tölti be!
    /// Ha megadjuk a flags paramétert, akkor a eContainerValid típusú konstans(ok)al adható meg, mely konténer adattagokat kell feltölteni.
    /// A CV_PORTS -ot nem kell megadni.
    /// @return A betöltött portok száma
    virtual int  fetchPorts(QSqlQuery& __q, int flags = 0) override;
    int fetchAllChilds(QSqlQuery& __q) { return fetchParams(__q) + fetchPorts(__q, -1); }
    /// A név alapján visszaadja a rekord ID-t, az objektum értéke nem változik.
    /// Ha a node típusban be lett állítva a host bit, akkor ha nincs találat a névre, akkor
    /// a keresett nevet kiegészíti a kereső domain nevekkel, és az így kapott nevekkel végrehajt mégegy keresést.
    /// A kereső domain nevek a sys_param táblában azok a rekordok, melyek típusának a neve "search domain".
    /// Ez utobbi esetben ha több találat van, akkor a sys_param.sys_param_name alapján rendezett első találattal tér vissza.
    virtual qlonglong getIdByName(QSqlQuery& __q, const QString& __n, enum eEx __ex = EX_ERROR) const override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual void clearShares() override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual bool setShare(int __a, int __ab = -1, int __b = -1, int __bb = -1, bool __cd = false) override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual bool updateShares(QSqlQuery& __q, bool __clr = false, enum eEx __ex = EX_ERROR) override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual cPPort *addPort(const QString& __name, const QString &__note, int __ix) override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual cPPort *addPorts(const QString& __np, int __noff, int __from, int __to, int __off) override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual void insertPort(QSqlQuery& q, int ix, const QString& _na, const QString& _no, const QString& _tag = QString()) override;
    /// Hibát dob, ebben az osztályban nem támogatott, nem értelmezett
    virtual void updateShared(QSqlQuery& q, int __a, int __ab, int __b, int __bb, bool __cd) override;
    /// A megadott eszköz szériaszámának a megadása.
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param name Az eszköz neve
    /// @param sn Széria szém
    /// @param __ex Ha értéke nem EX_IGNORE, akkor hiba esetén kizárást dob. EX_NOOP esetén (ez az alapértelmezés)
    ///         hibát dob, ha nincs ilyen nevű objektum.
    static bool setSeralNumber(QSqlQuery& q, const QString& name, const QString& sn, eEx __ex = EX_NOOP) {
        return cNode().updateFieldByName(q, name, _sSerialNumber, sn, __ex);
    }
    /// A megadott eszköz leltári számának a megadása.
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param name Az eszköz neve
    /// @param in Leltári szém
    /// @param __ex Ha értéke nem EX_IGNORE, akkor hiba esetén kizárást dob. EX_NOOP esetén (ez az alapértelmezés)
    ///         hibát dob, ha nincs ilyen nevű objektum.
    static bool setInventoryNumber(QSqlQuery& q, const QString& name, const QString& in, eEx __ex = EX_NOOP) {
        return cNode().updateFieldByName(q, name, _sInventoryNumber, in, __ex);
    }

    /// Kiírja a ports konténer tartalmát is
    virtual QString toString() const override;

    /// A portok lista kontáner bővítése egy elemmel.
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
    /// @param __np Port név minta.
    /// @param __noff Egy eltolás a név kébzéséhez
    /// @param __from Legkisebb idex
    /// @param __to Legnagyobb idex
    /// @param __off Egy eltolás a port indexhez
    /// @return Az utolsó objektum pointere
    cNPort *addPorts(const cIfType& __t, const QString& __np, int __noff, int __from, int __to, int __off);
    /// A port liasta bővítése egy sorozattal. Ha nem vesz fel egy portot sem, akkor dob egy kizárást.
    /// @param __t A port típusát definiáló típus név
    /// @param __np Port név minta.
    /// @param __noff Egy eltolás a név kébzéséhez
    /// @param __from Legkisebb idex
    /// @param __to Legnagyobb idex
    /// @param __off Egy eltolás a port indexhez
    /// @return Az utolsó objektum pointere
    cNPort *addPorts(const QString& __t, const QString& __np, int __noff, int __from, int __to, int __off)
        { return addPorts(cIfType::ifType(__t), __np, __noff, __from, __to, __off); }
    /// Hasonló mint az addPorts hívás, de a létrehozott portok típusa "sensor", továbbá kitölti az ip címet.
    /// A MAC NULL lessz, az ip cím típusa pedig "pseudo", és csak IPV4 formájú cím lehet
    cNPort *addSensors(const QString& __np, int __noff, int __from, int __to, int __off, const QHostAddress& __pip);

    /// Beolvas egy objektumot/rekordot az IP alapján. A metódus elöbb megkeresi a megadott IP címmel rendelkező
    /// iface_addrs rekordot, és az ehhez tartozó host-ot olvassa be. Ha esetleg cSnmpDevice objektummal hívjuk,
    /// és a beolvasandó rekord csak a parentben szerepel, akkor nem fog beolvasni semmit, vagyis nem lessz találat.
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett IP cím
    /// @param __ex Ha több host-nak van azonos címem akkor ha értéke nem EX_IGNORE, kizárást dob, egyébként false -val tér vissza.
    /// @return Ha van egy és csakis egy találat, ill. beolvasott rekord, akkor true, egyébként false
    int fetchByIp(QSqlQuery& q, const QHostAddress& a, enum eEx __ex = EX_ERROR);
    /// Törli a ports konténert.
    /// Beolvas egy portot, amelyhez a megadott ip cím tartozik, az egy IP cím rekorddal együtt.
    bool fetchOnePortByIp(QSqlQuery& q, const QHostAddress& a, enum eEx __ex = EX_ERROR);
    /// Beolvas egy objektumot/rekordot a MAC alapján.  Nem feltétlenül tartalmazza a beolvasott
    /// objektum a megadott címet. A metódus elöbb megkeresi a megadott MAC címmel rendelkező interfaces rekordot,
    /// és az ehhez tartozó host-ot olvassa be. Ha esetleg cSnmpDevice objektummal hívjuk, és a beolvasandó rekord
    /// csak a parentben szerepel, akkor nem fog beolvasni semmit, vagyis nem lessz találat.
    /// @param q Az adatbázisműveletekhez használt objektum
    /// @param a A keresett MAC
    /// @return a találatok száma.
    int fetchByMac(QSqlQuery& q, const cMac& a, eEx __ex = EX_WARNING);
    /// A saját cNode objektum adatait tölti be az adatbázisból.
    /// Ha meg van adva a HOSTNAME környezeti változó, akkor az ebben megadott nevű rekordot próbálja meg beolvasni.
    /// Ha nincs megadva a környezeti változó, vagy neincs ilyen nevű rekord, akkor lekérdezi a saját ip címeket,
    /// és a címek alapján próbál keresni egy host rekordot.
    /// Lásd még a lanView::testSelfName változót.
    /// @param q Az SQL müveletekhez használt query objektum.
    /// @param __ex Ha értéke nem EX_IGNORE, és nem sikerült beolvasni ill. megtalálni a rekordot, akkor dob egy kizárást.
    /// @return Ha nem találja a saját host rekordot, akkor ha __ex értéke hamis, akkor false-val tér vissza,
    ///         ha __ex értéke Hosttrue, akkor dob egy kizárást. Ha beolvasta a keresett rekordot, akkor true-val tér vissza
    bool fetchSelf(QSqlQuery& q, enum eEx __ex = EX_ERROR);
    cInterface *portAddAddress(const QString& __port, const QHostAddress& __a, const QString& __t, const QString &__d = _sNul);
    cInterface *portAddAddress(int __port_index, const QHostAddress& __a, const QString& __t, const QString &__d = _sNul);
    /// A megadott porthoz hozzáad egy VLAN összerendelést.
    cInterface *portSetVlan(const QString& __port, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st = ST_MANUAL);
    cInterface *portSetVlan(int __port_index, qlonglong __vlanId, enum eVlanType __t, enum eSetType __st = ST_MANUAL);
    ///
    cInterface *portSetVlans(const QString& __port, const QVariantList &_ids);
    cInterface *portSetVlans(int __port_index, const QVariantList &_ids);
    /// A beolvasott port és cím rekoedokból kikeresi az első címet.
    QHostAddress getIpAddress(tRecordList<cNPort>::const_iterator *ppi = nullptr) const;
    /// Ha nincsenek beolvasva a portok, akkor beolvassa azokat.
    /// A beolvasott port és cím rekoedokból kikeresi az első címet.
    QHostAddress getIpAddress(QSqlQuery& q);
    ///
    /// \brief fetchPreferedAddress
    /// \param q
    /// \param _nid node ID
    /// \param _iid Interface ID, optional. If set, the _nid is ignore
    /// \return Prefered ip adress, or NULL address
    ///
    static QHostAddress fetchPreferedAddress(QSqlQuery& q, qlonglong _nid, qlonglong _iid = NULL_ID);
    /// \brief fetchAllAddress
    /// \param q
    /// \param cont
    /// \param __id
    /// \return
    ///
    int fetchAllAddress(QSqlQuery& q, tRecordList<cIpAddress> &cont, qlonglong __id = NULL_ID) const;
    QList<QHostAddress> fetchAllIpAddress(QSqlQuery &q, qlonglong __id = NULL_ID) const;
    QList<cMac> getMacs() const;
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
    cNode& asmbWorkstation(QSqlQuery &q, const QString& __n, const tStringPair *__addr, const QString *__mac, const QString& __d, qlonglong __place = 0);
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
    cNPort& asmbHostPort(QSqlQuery& q, int ix, const QString& pt, const QString& pn, const tStringPair *ip, const QVariant *mac, const  QString& d);
    /// Egy új host vagy snmp eszköz összeállítása
    /// @param __name Az eszköz neve, vagy a "LOOKUP" string
    /// @param __port port neve/port typusa, vagy NULL, ha default.
    /// @param __addr Pointer egy string pár, az első elem az IP cím, vagy az "ARP" ill. "LOOKUP" string, ha az ip címet a MAC címből ill.
    ///            a névből kell meghatározni.A második elem az ip cím típus neve.
    /// @param __sMac Vagy a MAC stringgé konvertálva, vagy az "ARP" string, ha az IP címből kell meghatározni.
    /// @param __note node secriptorra/megjegyzés
    /// @param __ex Ha értéke nwm EX_IGNORE, akkor hiba esetén dob egy kizárást, ha false, akkor hiba esetén a ES_DEFECTIVE bitet állítja be.
    cNode& asmbNode(QSqlQuery& q, const QString& __name, const tStringPair* __port, const tStringPair *__addr, const QString *__sMac, const QString &__note = _sNul, qlonglong __place = NULL_ID, enum eEx __ex = EX_ERROR);
    ///
    bool bDelCollisionByMac;
    bool bDelCollisionByIp;
    int delCollisionByMac(QSqlQuery &__q);
    int delCollisionByIp(QSqlQuery &__q);
    static cNode * getSelfNodeObjByMac(QSqlQuery& q);
};

class cTable;

class LV2SHARED_EXPORT cSnmpDevice : public cNode {
    CRECORD(cSnmpDevice);
public:
    cSnmpDevice(const QString& __n, const QString& __d);
    /// A cPatch::insert(QSqlQuery &__q, enum eEx __ex) hívása elött beállítja a
    /// node_type értékét, ha az NULL.
    virtual bool insert(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR) override;
    /// Az SNMP verzió konstanst adja vissza (net-snmp híváshoz)
    int snmpVersion() const;
    /// SNMP lekérdezésekkel feltölti az objektumot
    bool setBySnmp(const QString& __com = _sNul, enum eEx __ex = EX_ERROR, QString *__pEs = nullptr, QHostAddress *ip = nullptr, cTable *_pTable = nullptr);
    /// Inicializálja az SNMP session-t. És próbaképpen lekéri a SNMPv2-MIB::sysDescr -t.
    /// Sorra végigveszi az eszköz IP címeit, ha nem sikerül a lekérdezés.
    /// Ha a próba lekérdezés sikertelen, és elfogytak a címek, akkor ha __ex értéke
    /// EX_IGNORE a(z utolsó) hiba koddal tér vissza, egyébként kizárást dob. Siker esetén 0-val tér vissza.
    /// @param a Lekérdezéshez hasnálható query obj., ha szükséges (nincsenek beolvasva a portok, ill. IP címek).
    /// @param snmp Az SNMP modul objektum.
    /// @param __ex Ha értéke nem EX_IGNORE, akkor hiba esetén kizárást dob.
    /// @return Az SNMP hiba kód (siker esetén 0).
    int open(QSqlQuery &q, cSnmp& snmp, enum eEx __ex = EX_ERROR, QString *pEMsg = nullptr);
    static cSnmpDevice *refresh(QSqlQuery& q, const QString& _name, enum eEx __ex = EX_ERROR, QString *pEMsg = nullptr);
    static cSnmpDevice *refresh(QSqlQuery& q, qlonglong _id, enum eEx __ex = EX_ERROR, QString *pEMsg = nullptr);
    cSnmpDevice *refresh(QSqlQuery& q, enum eEx __ex = EX_ERROR, QString *pEMsg = nullptr);
};

enum sNodeObjectType {
    NOT_INVALID,        ///< Az eredeti rekord nem a patchs tábla leszármazotja
    NOT_PATCH,          ///< Az eredeti rekord a patchs
    NOT_NODE,           ///< Az eredeti rekord a nodes
    NOT_SNMPDEVICE,     ///< Az eredeti rekord az snmpdeices
    NOT_TMSK= 0x00F,    ///< Eredeti típus maszk
    NOT_ANY = 0x010,    ///< Ez egy cRecordAny objektum
    NOT_NEQ = 0x020     ///< A rekord típusa és az objektum típusa nem azonos
};
EXT_ int nodeObjectType(QSqlQuery& q, const cRecord& o, eEx __ex = EX_ERROR);
/// Feltételes objektum allokálás, ha a típus nem egyezik a rekord eredeti típusával, vagy egy cRecordAny objektum.
/// @param po Forrás objektum pointere
/// @param pt Opcionális pointer, a típusvizsgálat eredményének (sNodeObjectType).
EXT_ cPatch * nodeToOrigin(QSqlQuery& q, const cRecord *po, int *pt = nullptr);

/* ---------------------------------------------------------------- */
/// @class cImportTemplate
/// Template vagy maktó objektum (import_template rekord).
class LV2SHARED_EXPORT cImportTemplate : public cRecord {
    CRECORD(cImportTemplate);
};

/// @class cTemplateMap
/// Import_template record cache
class LV2SHARED_EXPORT cTemplateMap : public tStringMap {
public:
    /// Konstruktor
    cTemplateMap() { EXCEPTION(ENOTSUPP); }
    /// Konstruktor
    /// @param __type Típus string, a konténerben azok a rekordok kerülnek, melyeknél a template_type mező értéke ezzel megeggyezik
    cTemplateMap(const QString& __type);
    /// Copy konstruktor
    cTemplateMap(const cTemplateMap& __o);
    /// copy operator, csak akkor másol, ha a type adattag azonos.
    /// Különböző type értékek esetén (EPROGFAIL) hibát dob
    cTemplateMap& operator=(const cTemplateMap& __o) {
        if (__o.type != type) EXCEPTION(EPROGFAIL);
        *static_cast<tStringMap *>(this) = static_cast<tStringMap>(__o);
        return *this;
    }
    /// Egy megadott nevű template lekérése, ha nincs a konténerben, akkor beolvassa az adatbázisból.
    const QString& get(QSqlQuery& __q, const QString& __name, enum eEx __ex = EX_ERROR);
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
    enum eVlanType getVlanType(enum eEx __ex = EX_ERROR){ return eVlanType(vlanType(getName(_ixVlanType), __ex)); }
    cPortVlan& setSetType(enum eSetType __e)    { setName(_ixSetType, setType(__e)); return *this; }
    cPortVlan& setSetType(const QString& __n)   { (void)setType(__n); setName(_ixSetType, __n); return *this; }
    enum eSetType getSetType(enum eEx __ex = EX_ERROR)  { return eSetType(setType(getName(_ixSetType), __ex)); }
    STATICIX(cPortVlan, PortId)
    STATICIX(cPortVlan, VlanId)
    STATICIX(cPortVlan, VlanType)
    STATICIX(cPortVlan, SetType)
};

/// @macro APPMEMO(q, m, i)
/// Egy ap_memos rekord kiírása.
/// @param q Az adatbázis művelethez használt QSqlQuery objektum.
/// @param m Az üzenet string
/// @param i Az üzenet fontossága (enum eNotifSwitch konstans)
#define APPMEMO(q, m, i) cAppMemo::memo(q, m, i, __PRETTY_FUNCTION__, __FILE__, __LINE__)

/// @macro QAPPMEMO(m, i)
/// Egy ap_memos rekord kiírása.
/// @param m Az üzenet string
/// @param i Az üzenet fontossága (enum eNotifSwitch konstans)
#define QAPPMEMO(m, i)  { QSqlQuery q = getQuery(); APPMEMO(q, m, i); }

/// @macro HEREIN(o, m, i)
/// Egy ap_memos rekord kitöltése a keletkezés hely adataival.
/// @param m Az üzenet string
/// @param i Az üzenet fontossága (enum eNotifSwitch konstans)
#define HEREIN(o, m, i)  o.herein(m, i, __PRETTY_FUNCTION__, __FILE__, __LINE__)


/// @macro HEREINW(o, m, i)
/// Ugyan az, mint a HEREIN(o,m,i) makró, de kiad egy warning típusú debug üzenetet is. lásd.: DWAR(s) makrót.
#define HEREINW(o, m, i)  { \
        QString message = m; \
        HEREIN(o, message, i); \
        DWAR() << message << endl; \
    }

/// @macro HEREINWE(o, m, i)
/// Ugyan az, mint a HEREIN(o,m,i) makró, de az export queue-be is elküldi az üzenetet,
/// log viszont nincs.
#define HEREINWE(o, m, i)  { \
        QString message = m; \
        HEREIN(o, message, i); \
        expText(i, message, true); \
    }


class LV2SHARED_EXPORT cAppMemo : public cRecord {
    CRECORD(cAppMemo);
public:
    QString getMemo() { return getName(_sMemo); }
    cAppMemo& setMemo(const QString& s) { setName(_sMemo, s); return *this; }
    cAppMemo& herein(const QString &_memo, int _imp = RS_UNKNOWN, const QString &_func_name = _sNul, const QString &_src = _sNul, int _lin = 0);
    static qlonglong memo(QSqlQuery &q, const QString &_memo, int _imp = RS_UNKNOWN, const QString &_func_name = _sNul, const QString &_src = _sNul, int _lin = 0);
};

/* */

class LV2SHARED_EXPORT cSelect : public cRecord {
    CRECORD(cSelect);
//    FEATURES(cSelect)
public:
    STATICIX(cSelect, Choice)
    cSelect& choice(QSqlQuery &q, const QString& _type, const QString& _val, eEx __ex = EX_ERROR);
    cSelect& choice(QSqlQuery &q, const QString& _type, const cMac _val, eEx __ex = EX_ERROR);

};

#endif // LV2DATA_H
