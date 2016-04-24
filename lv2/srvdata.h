#ifndef SRVDATA
#define SRVDATA

#include "lv2data.h"
#include "lv2cont.h"

/// Paraméter név
EXT_ QString getParName(QString::const_iterator& i, const QString::const_iterator& e, bool __ex = true);


/// @class cIpProtocol
class LV2SHARED_EXPORT cIpProtocol : public cRecord {
    CRECORD(cIpProtocol);
};

/// @class cServiceType
/// A service objektumokat csoportosítására bevezetett elem.
/// A sevízek állapotaihoz rendelt üzenetek ősszeállításánál van szerepe.
class LV2SHARED_EXPORT cServiceType : public cRecord {
    CRECORD(cServiceType);
public:
    static qlonglong insertNew(QSqlQuery& __q, const QString& __name, const QString& __note, bool __ex = true);
};

class LV2SHARED_EXPORT cAlarmMsg  : public cRecord {
    CRECORD(cAlarmMsg);
public:
    static void replace(QSqlQuery& __q, qlonglong __stid, const QString& __stats, const QString& __shortMsg, const QString& __msg);
    static void replaces(QSqlQuery& __q, qlonglong __stid, const QStringList& __stats, const QString& __shortMsg, const QString& __msg);
};

class cHostService;

class LV2SHARED_EXPORT cService : public cRecord {
    CRECORD(cService);
    FEATURES(cService)
public:
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    const cIpProtocol protocol() { return _protocol; }
    QString protocolName() { return protocol().getName(); }
    int protocolId()  { return (int)protocol().getId(); }
protected:
    cIpProtocol             _protocol;
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
    static void clearServicesCache() { services.clear(); }
    STATICIX(cService, ixProtocolId)
};

class LV2SHARED_EXPORT cHostService : public cRecord {
    template <class S> friend void _SplitMagicT(S& o, bool __ex);
    template <class S> friend void _Magic2PropT(S& o);
    CRECORD(cHostService);
    FEATURES(cHostService)
public:
    cHostService& operator=(const cHostService& __o);
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual int replace(QSqlQuery &__q, bool __ex = true);
    /// Státusz beállítása. A  set_service_stat() PL/pSQL függvényt hívja.
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @param __st A szolgáltatással kapcsolatos művelet eredménye. Nem az új status, azt a set_service_stat()
    ///             határozza majd meg a mostani eredmény és az előélet alapján.
    /// @param __note Egy megjegyzés, ami, bele kerül a log rekordba, ha van változás, ill. ha keletkezik üzenet,
    ///             akkor abba is.
    /// @param __did Daemon host_service_id, opcionális
    /// @return Az objektum referenciája, aktualizálva az aktuális rekord értékkel.
    /// @exception Bármilyen hiba esetén dob egy kizázást cError * -al.
    cHostService& setState(QSqlQuery& __q, const QString& __st, const QString& __note = QString(), qlonglong __did = NULL_ID);
    /// A hálózati elem, és a szolgáltatás típus neve alapján olvassa be egy rekordot.
    /// Ha több rekord is létezik, akkor az első kerül beolvasásra.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) neve
    /// @param __sn A szolgáltatás típus (services) neve
    /// @param __ex Ha nincs ilyen rekord, vagy több van, és értéke true, akkor dob egy kizárást.
    /// @return A megadott nevekkel azonosított szolgáltatáspéldányok száma.
    int fetchByNames(QSqlQuery& q, const QString& __hn, const QString& __sn, bool __ex = true);
    /// A hálózati elem, port és a szolgáltatás típus neve alapján olvassa be egy rekordot.
    /// Ha több rekord is létezik, akkor az első kerül beolvasásra.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) neve
    /// @param __sn A szolgáltatás típus (services) neve
    /// @param __pn A szolgáltatáspéldányhoz rendelt port neve, az üres string esetén nincs definiálva a példányhoz port.
    ///             Fugyelem az üres string olyan rekordot jelent amelyben a port_id éeréke NULL!
    /// @param __ex Ha nincs ilyen rekord, vagy több van, és értéke true, akkor dob egy kizárást.
    /// @return A megadott nevekkel azonosított szolgáltatáspéldányok száma.
    int fetchByNames(QSqlQuery& q, const QString &__hn, const QString& __sn, const QString& __pn, bool __ex = NULL);
    /// A hálózati elem, port,  szolgáltatás típus valamit a proto és prome szolgáltatás típus nevek alapján olvassa be egy rekordot.
    /// Ha több rekord is létezik, akkor az első kerül beolvasásra.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) neve
    /// @param __sn A szolgáltatás típus (services) neve
    /// @param __pn A szolgáltatáspéldányhoz rendelt port neve, az üres string esetén nincs definiálva a példányhoz port.
    ///             Fugyelem az üres string olyan rekordot jelent amelyben a port_id éeréke NULL!
    /// @param __pron A szolgáltatáspéldányhoz rendelt protocol szolgáltatás neve, az üres string esetén nincs definiálva a példányhoz potokol.
    /// @param __prin A szolgáltatáspéldányhoz rendelt prime szolgáltatás neve, az üres string esetén nincs definiálva a példányhoz rime sz..
    /// @param __ex Ha nincs ilyen rekord, vagy több van, és értéke true, akkor dob egy kizárást.
    /// @return A megadott nevekkel azonosított szolgáltatáspéldányok száma.
    int fetchByNames(QSqlQuery& q, const QString& __hn, const QString& __sn, const QString& __pn, const QString& __pron, const QString& __prin, bool __ex = NULL);
    /// A hálózati elem, és a szolgáltatás típus név minták alapján olvassa be az első rekordot.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) neve
    /// @param __sn A szolgáltatás típus (services) neve
    /// @param __ex Ha nincs egy ilyen rekord sem, és értéke true, akkor dob egy kizárást.
    /// @return A megadott név mintákkal azonosított szolgáltatáspéldányok száma.
    int fetchFirstByNamePatterns(QSqlQuery& q, const QString& __hn, const QString& __sn, bool __ex = true);
    /// A hálózati elem, és a szolgáltatás típus ID-k alapján olvas be egy rekordot
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hid A hálózati elem (nodes) ID
    /// @param __sid A szolgáltatás típus (services) ID
    /// @param __ex Ha nincs ilyen rekord, vagy több van, és értéke true, akkor dob egy kizárást.
    /// @return true, ha van egy és csakis egy ilyen rekord.
    bool fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, bool __ex = true);
    /// A megadott nevek alapján türli a megadott rekord(ok)at
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __nn A node neve, vagy egy minta
    /// @param __sn A szolgálltatás típus neve, vagy egy minta
    /// @param __npat Ha értéke true, akkor az __nn paraméter egy minta, egyébként egy node neve.
    /// @param __spat Ha értéke true, akkor az __sn paraméter egy minta, egyébként egy szervíz típus neve.
    /// @return A törölt rekordok számával tér vissza.
    int delByNames(QSqlQuery& q, const QString& __nn, const QString& __sn, bool __npat = false, bool __spat = false);
    /// Beolvassa a saját host és host_service rekordokat. Az __sn egy beolvasott servces rekordot kell tartalmazzon.
    /// A beolvasott host a saját host rekord lessz.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __h A kitöltendő host objektum. Eredeti objektum típusa cNode vagy cSnmpDevice lehet.
    /// @param __s A szervíz rekordot reprezentáló kitöltött objektum, a metódus csak az ID mezőt használja.
    /// @param __ex Ha értéke true (ill nem adjuk meg), akkor hiba esetén, ill. ha nem létezik valamelyik keresett objektum, akkor dob egy kizárást.
    /// @return Ha sikerült beolvasni a rekordokat, akkor true, ha nem és __ex értéke true, akkor false.
    bool fetchSelf(QSqlQuery& q, cNode& __h, const cService& __s, bool __ex = true);
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
    static QString names(QSqlQuery& q, qlonglong __id);
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
};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cAlarm  : public cRecord {
    CRECORD(cAlarm);
public:
};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cOui  : public cRecord {
    CRECORD(cOui);
public:
    virtual int replace(QSqlQuery& __q, bool __ex = false);
};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cMacTab  : public cRecord {
    CRECORD(cMacTab);
public:
    /// Inzertálja, vagy modosítja az ip cím, mint kulcs alapján a rekordot.
    /// A funkciót egy PGPLSQL fúggvény (replace_mactab) valósítja meg.
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @return A peplace_arp függvény vissatérési értéke. Ld.: enum eReasons
    virtual int replace(QSqlQuery& __q, bool __ex = false);
    static int refresStats(QSqlQuery& __q);
    STATICIX(cMacTab, ixPortId)
    STATICIX(cMacTab, ixHwAddress)
    STATICIX(cMacTab, ixSetType)
    STATICIX(cMacTab, ixMacTabState)
};

class cArpTable;
class LV2SHARED_EXPORT cArp : public cRecord {
    CRECORD(cArp);
protected:
    static int _ixIpAddress;
    static int _ixHwAddress;
    static int _ixSetType;
    static int _ixHostServiceId;
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
    /// A funkciót egy PGPLSQL fúggvény (replace_arp) valósítja meg.
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @return A replace_arp függvény vissatérési értéke. Ld.: enum eReasons
    virtual int replace(QSqlQuery& __q, bool __ex = true);
    /// Inzertálja, vagy morosítja az ip cím, mint kulcs alapján a rekordokat
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @param __t A módosításokat tartalmazó konténer
    /// @return a kiírt, vagy módosított rekordok száma
    static int replaces(QSqlQuery& __q, const cArpTable& __t, int setType = ST_QUERY, qlonglong hsid = NULL_ID);
    static QList<QHostAddress> mac2ips(QSqlQuery& __q, const cMac& __m);
    static QHostAddress mac2ip(QSqlQuery& __q, const cMac& __m, bool __ex = true);
    static cMac ip2mac(QSqlQuery& __q, const QHostAddress& __a, bool __ex = true);
    static int checkExpired(QSqlQuery& __q);
};

/* ---------------------------------------------------------------- */

class LV2SHARED_EXPORT cDynAddrRange : public cRecord {
    CRECORD(cDynAddrRange);
public:
    static eReasons replace(QSqlQuery &q, const QHostAddress& b, const QHostAddress& e, qlonglong dhcpSrvId, bool exclude, qlonglong subnetId = NULL_ID);
};

/* ---------------------------------------------------------------- */
enum eExecState {
    ES_INVALID = -1,
    ES_WAIT    =  0,
    ES_EXECUTE,
    ES_OK,
    ES_FAILE,
    ES_ABORTED
};

EXT_ int execState(const QString& _n, bool __ex = true);
EXT_ const QString& execState(int _e, bool __ex = true);

class LV2SHARED_EXPORT cImport : public cRecord {
    CRECORD(cImport);
};


/* ---------------------------------------------------------------- */
class cInspector;
class cImportParseThread;

class LV2SHARED_EXPORT cQueryParser : public cRecord {
    CRECORD(cQueryParser);
public:
    static void _insert(QSqlQuery& q, qlonglong _sid, const QString& _ty, bool _cs, const QString& _re, const QString& _cmd, const QString& _not, qlonglong _seq);
    void setInspector(cInspector *pInsp);
    int prep(cError *&pe);
    int parse(QString src, cError *&pe);
    int post(cError *&pe);
    /// Beolvassa az összes megadott _sid -hez tartozó rekordot, és létrehozza, és kitölti a pListCmd és pListRExp kontlnereket,
    /// valamit a pPrepCmd és pPostCmd stringeket.
    /// Ha a thread értéke true, akkor kltrejozza, és elindítja a pParserThread -et is.
    /// @param force Akkor is beolvassa a rekordokat, ha a konténerek léteznek, és _sid nem változott.
    int load(QSqlQuery& q, qlonglong _sid = NULL_ID, bool force = true, bool thread = true);
    int delByServiceName(QSqlQuery &q, const QString &__n, bool __pat);
protected:
    QString getParValue(const QString& name, const QStringList &args);
    QString substitutions(const QString& _cmd, const QStringList& args);
    int execute(cError *&pe, const QString& _cmd, const QStringList& args = QStringList());
    QStringList         *pListCmd;
    QList<QRegExp>      *pListRExp;
    QString             *pPrepCmd;
    QString             *pPostCmd;
    cInspector          *pInspector;
    cImportParseThread  *pParserThread;
};

#endif // SRVDATA

