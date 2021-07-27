#ifndef SRVDATA
#define SRVDATA

#include "lv2data.h"
#include "lv2cont.h"

/// @class cServiceType
/// A service objektumokat csoportosítására bevezetett elem.
/// A sevízek állapotaihoz rendelt üzenetek ősszeállításánál van szerepe.
class LV2SHARED_EXPORT cServiceType : public cRecord {
    CRECORD(cServiceType);
public:
    static qlonglong insertNew(QSqlQuery& __q, const QString& __name, const QString& __note, bool _replace = false, enum eEx __ex = EX_ERROR);
};

class LV2SHARED_EXPORT cAlarmMsg  : public cRecord {
    CRECORD(cAlarmMsg);
public:
    enum eTextIndex {
        LTX_MESSAGE = 0,
        LTX_SHORT_MSG
    };

    static void replace(QSqlQuery& __q, qlonglong __stid, const QString& __stats, const QString& __shortMsg, const QString& __msg);
    static void replaces(QSqlQuery& __q, qlonglong __stid, const QStringList& __stats, const QString& __shortMsg, const QString& __msg);
    static void replaces(QSqlQuery& __q, const QString& __stn, const QStringList& __stats, const QString& __shortMsg, const QString& __msg)
    {
        qlonglong id = cServiceType().getIdByName(__q, __stn);
        replaces(__q, id, __stats, __shortMsg, __msg);
    }
};

#define UNMARKED_SERVICE_TYPE_ID    -1LL

class cHostService;

class LV2SHARED_EXPORT cService : public cRecord {
    CRECORD(cService);
    FEATURES(cService)
public:
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    RECACHEHED(cService, service)
    static qlonglong name2id(QSqlQuery &__q, const QString& __nm, enum eEx __ex = EX_ERROR) {
        const cService *p = service(__q, __nm, __ex);
        return p == nullptr ? NULL_ID : p->getId();
    }
    static QString id2name(QSqlQuery &__q, qlonglong __id, enum eEx __ex = EX_ERROR) {
        const cService *p = service(__q, __id, __ex);
        return p == nullptr ? _sNul : p->getName();
    }
};

#define NIL_SERVICE_ID   -1LL
#define TICKET_SERVICE_ID 0LL

enum eNoalarmType {
    NAT_OFF     =  0,   ///< A riasztás nincs letiltva
    NAT_ON,             ///< A riasztás le van tiltva
    NAT_TO,             ///< A riasztás ketiltva a megadott időpontig
    NAT_FROM,           ///< A riasztás ketiltva a megadott időponttól
    NAT_FROM_TO         ///< A riasztás ketiltva a megadott időpontok között
};

EXT_ int noalarmtype(const QString& _n, enum eEx __ex = EX_ERROR);
EXT_ const QString& noalarmtype(int _e, enum eEx __ex = EX_ERROR);



class LV2SHARED_EXPORT cHostService : public cRecord {
    template <class S> friend void _SplitMagicT(S& o, enum eEx __ex);
    template <class S> friend void _Magic2PropT(S& o);
    CRECORD(cHostService);
    FEATURES(cHostService)
public:
    cHostService(QSqlQuery& q, const QString& __h, const QString& __p, const QString& __s, const QString& __n);
    cHostService& operator=(const cHostService& __o);
    virtual void toEnd();
    virtual bool toEnd(int i);
    virtual void clearToEnd();
    virtual int replace(QSqlQuery &__q, enum eEx __ex = EX_ERROR);
    /// A port_id mező kitöltése a port név alapján. A metódus számít rá, hogy a node_id mező már ki van töltve.
    void setPort(QSqlQuery& q, const QString& __p) { setId(_sPortId, cNPort().getPortIdByName(q, __p, getId(_sNodeId))); }
    /// Státusz beállítása. A  set_service_stat() PL/pSQL függvényt hívja.
    /// Csak a disabled, host_service_state és hard_state mezőket olvassa vissza
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @param __st A szolgáltatással kapcsolatos művelet eredménye. Nem az új status, azt a set_service_stat()
    ///             határozza majd meg a mostani eredmény és az előélet alapján.
    /// @param __note Egy megjegyzés, ami, bele kerül a log rekordba, ha van változás, ill. ha keletkezik üzenet,
    ///             akkor abba is.
    /// @param __did Daemon host_service_id, opcionális
    /// @param _resetIfDeleted Ha nem találja a rekordot, akkor kiad egy lanView::getInstance()->reSet(); hívást, ha a deleted mező true lessz úgyszintén.
    /// @return Az objektum pointere, aktualizálva a visszaolvasott mező értékkel.
    ///         Ha (már) nem létezik a megadott szolgáltzatás példány, akkor nem dob kizárást, hanem a 'deleted' mezőt true-ra állítja,
    ///         és nullptr-el tér vissza.
    /// @exception Bármilyen egyéb hiba esetén dob egy kizázást cError * -al.
    cHostService *setState(QSqlQuery& __q, const QString& __st, const QString& __note = QString(), qlonglong __did = NULL_ID, bool _resetIfDeleted = true, bool forced = false);
    /// Törli az aktuális rekord (ID alapján) állapot mezőit.
    /// Az adatbázisból nem olvassa vissza a rekordok tartalmát.
    cHostService& clearState(QSqlQuery& __q);
    /// Az objektum állapot mezőit törli/alaphelyzetbe állítja (a memóriában)
    /// @param flag Ha értéke nem TS_NULL, akkor beállítja a flag mezőt is
    /// @return A modosított mezők maszk-ja.
    QBitArray clearStateFields(eTristate flag = TS_NULL);
    /// A hálózati elem, és a szolgáltatás típus neve alapján olvassa be egy rekordot.
    /// Ha több rekord is létezik, akkor az első kerül beolvasásra.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) név minta (LIKE)
    /// @param __sn A szolgáltatás típus (services) minta (LIKE)
    /// @param __ex Ha nincs ilyen rekord, és értéke nem EX_IGNORE, vagy tőbb van és értéke EX_WARNING, akkor kizárást generál.
    /// @return A megadott nevekkel azonosított szolgáltatáspéldányok száma.
    int fetchByNames(QSqlQuery& q, const QString& __hn, const QString& __sn, enum eEx __ex = EX_ERROR);
    /// A hálózati elem, port és a szolgáltatás típus neve alapján olvassa be egy rekordot.
    /// Ha több rekord is létezik, akkor az első kerül beolvasásra.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) név minta (LIKE)
    /// @param __sn A szolgáltatás típus (services) minta (LIKE)
    /// @param __pn A szolgáltatáspéldányhoz rendelt port név minta (LIKE), az üres string esetén nincs definiálva a példányhoz port.
    ///             Fugyelem az üres string olyan rekordot jelent amelyben a port_id éeréke NULL!
    /// @param __ex Ha nincs ilyen rekord, és értéke nem EX_IGNORE, vagy tőbb van és értéke EX_WARNING, akkor kizárást generál.
    /// @return A megadott nevekkel azonosított szolgáltatáspéldányok száma.
    int fetchByNames(QSqlQuery& q, const QString &__hn, const QString& __sn, const QString& __pn, enum eEx __ex = EX_WARNING);
    /// A hálózati elem, port,  szolgáltatás típus valamit a proto és prome szolgáltatás típus nevek alapján olvassa be egy rekordot.
    /// Ha több rekord is létezik, akkor az első kerül beolvasásra.
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hn A hálózati elem (nodes) név minta (LIKE)
    /// @param __sn A szolgáltatás típus (services) minta (LIKE)
    /// @param __pn A szolgáltatáspéldányhoz rendelt port név minta (LIKE), az üres string esetén nincs definiálva a példányhoz port.
    ///             Fugyelem az üres string olyan rekordot jelent amelyben a port_id éeréke NULL!
    /// @param __pron A szolgáltatáspéldányhoz rendelt protocol szolgáltatás név minta, az üres string esetén nincs "nil".
    /// @param __prin A szolgáltatáspéldányhoz rendelt prime szolgáltatás név minta, az üres string esetén nincs "nil".
    /// @param __ex Ha nincs ilyen rekord, és értéke nem EX_IGNORE, vagy tőbb van és értéke EX_WARNING, akkor kizárást generál.
    /// @return A megadott nevekkel azonosított szolgáltatáspéldányok száma.
    int fetchByNames(QSqlQuery& q, const QString& __hn, const QString& __sn, const QString& __pn, const QString& __pron, const QString& __prin, enum eEx __ex = EX_WARNING);
    /// A hálózati elem, és a szolgáltatás típus ID-k alapján olvas be egy rekordot
    /// @param q Az adatbázis művelethez használlt objektum.
    /// @param __hid A hálózati elem (nodes) ID
    /// @param __sid A szolgáltatás típus (services) ID
    /// @param __ex Ha nincs ilyen rekord, vagy több van, és értéke true, akkor dob egy kizárást.
    /// @return true, ha van egy és csakis egy ilyen rekord.
    bool fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, enum eEx __ex = EX_ERROR);
    bool fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, qlonglong __pid, eEx __ex = EX_ERROR);
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
    bool fetchSelf(QSqlQuery& q, cNode& __h, const cService& __s, enum eEx __ex = EX_ERROR);
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
    /// Rekord azonosító nevekből képez egy stringet: node[:port].szolgáltatás alakban.
    /// Az eredmény stringet az ID alapján kérdezi le az adatbázisból. Csak az ID mezőnek kell kitöltve lennie,
    /// az objektum többi mezőjének az értéke érdektelen az eredmény szempontjából.
    QString fullName(QSqlQuery& q, eEx __ex = EX_ERROR) const;
    /// Rekord azonosító nevekből képez egy stringet: node[:port].szolgáltatés alakban.
    /// Az eredmény stringet a megadott ID alapján kérdezi le az adatbázisból.
    static QString fullName(QSqlQuery& q, qlonglong __id, eEx __ex = EX_ERROR);
    /// A prime_service_id mező álltal hivatkozott cService objektummal tér vissza.
    /// Ha a mező ártéke NULL, akkor egy NULL pointerrel tér vissza.
    /// @param __q Az esetleges SQL lekérdezéshez használlt objektum (ha már be van olvasva a keresett objektum, akkor nem fordul az adatbázishoz)
    /// @param __ex Hiba esetén vagy, ha az id nem NULL, de mégsem találja az objektumot, akkor nem NULL pointerrel tér vissza, hanem dob egy kizárást,
    ///             ha __ex értéke nem EX_IGNORE. Ha __ex értéke EX_WARNING és a mező érték NULL, szintén kizárást dob.
    /// @return A keresett objektum pointere, vagy NULL pointer.
    /// @note A prime_service_id mező nem lehet NULL az adatbázisban. Az alapértelmezett érték a 'nil' nevű szolgáltatásra mutat.
    const cService *getPrimeService(QSqlQuery& __q, enum eEx __ex = EX_ERROR)
    {
        qlonglong id = getId(_sPrimeServiceId);
        if (id == NULL_ID) {
            if (__ex == EX_WARNING) EXCEPTION(EDATA);
            return nullptr;
        }
        return cService::service(__q, id, __ex);
    }
    /// A proto_service_id mező álltal hivatkozott cService objektummal tér vissza.
    /// Ha a mező ártéke NULL, akkor egy NULL pointerrel tér vissza.
    /// @param __q Az esetleges SQL lekérdezéshez használlt objektum (ha már be van olvasva a keresett objektum, akkor nem fordul az adatbázishoz)
    /// @param __ex Hiba esetén vagy, ha az id nem NULL, de mégsem találja az objektumot, akkor nem NULL pointerrel tér vissza, hanem dob egy kizárást,
    ///             ha __ex értéke nem EX_IGNORE. Ha __ex értéke EX_WARNING és a mező érték NULL, szintén kizárást dob.
    /// @return A keresett objektum pointere, vagy NULL pointer.
    /// @note A proto_service_id mező nem lehet NULL az adatbázisban. Az alapértelmezett érték a 'nil' nevű szolgáltatásra mutat.
    const cService *getProtoService(QSqlQuery& __q, enum eEx __ex = EX_ERROR)
    {
        qlonglong id = getId(_sProtoServiceId);
        if (id == NULL_ID) {
            if (__ex == EX_WARNING) EXCEPTION(EDATA);
            return nullptr;
        }
        return cService::service(__q, id, __ex);
    }
    ///
    static qlonglong ticketId(QSqlQuery& q, eEx __ex = EX_ERROR) {
        cHostService o;
        if (o.fetchByNames(q, _sNil, _sTicket, __ex) == 1) return o.getId();
        return NULL_ID;
    }
protected:
    static int setStateMaxTry;
    QString sFulName;
private:
    // setState
    static tIntVector readBackFieldIxs;    // Visszaolvasandó (változó) mezők indexei
    static QString readBackFields;
};

/* ---------------------------------------------------------------- */
enum eUserEventType {
    UE_NOTICE,
    UE_VIEW,
    UE_ACKNOWLEDGE,
    UE_SENDMESSAGE,
    UE_SENDMAIL
};

enum eUserEventState {
    UE_NECESSARY,
    UE_HAPPENED,
    UE_DROPPED
};

EXT_ int userEventType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& userEventType(int _i, eEx __ex = EX_ERROR);

EXT_ int userEventState(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& userEventState(int _i, eEx __ex = EX_ERROR);

class LV2SHARED_EXPORT cUserEvent  : public cRecord {
    CRECORD(cUserEvent);
public:
    static qlonglong insert(QSqlQuery &q, qlonglong _uid, qlonglong _aid, eUserEventType _et);
    static qlonglong insertHappened(QSqlQuery &q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString& _m = QString());
    static bool happened(QSqlQuery& q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString& _m = QString());
    static bool dropped(QSqlQuery& q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString& _m = QString());
};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cAlarm  : public cRecord {
    CRECORD(cAlarm);
public:
    static QString htmlText(QSqlQuery &q, qlonglong _id);
    static void ticket(QSqlQuery &_q, eNotifSwitch _st, const QString& _msg, qlonglong _did = NULL_ID, qlonglong _said = NULL_ID, eNotifSwitch _fst = RS_INVALID, eNotifSwitch _lst = RS_INVALID);

};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cOui  : public cRecord {
    CRECORD(cOui);
public:
    virtual int replace(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    bool fetchByMac(QSqlQuery& q, const cMac& __mac);
};

/* ---------------------------------------------------------------- */
class LV2SHARED_EXPORT cMacTab  : public cRecord {
    CRECORD(cMacTab);
public:
    /// Inzertálja, vagy modosítja az ip cím, mint kulcs alapján a rekordot.
    /// A funkciót egy PGPLSQL fúggvény (replace_mactab) valósítja meg.
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @return A peplace_arp függvény vissatérési értéke. Ld.: enum eReasons
    virtual int replace(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    static int refresStats(QSqlQuery& __q);
    STATICIX(cMacTab, PortId)
    STATICIX(cMacTab, HwAddress)
    STATICIX(cMacTab, SetType)
    STATICIX(cMacTab, MacTabState)
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
    virtual int replace(QSqlQuery& __q, enum eEx __ex = EX_ERROR);
    /// Inzertálja, vagy morosítja az ip cím, mint kulcs alapján a rekordokat
    /// @param __q Az adatbázis művelethez használt objektum.
    /// @param __t A módosításokat tartalmazó konténer
    /// @return a kiírt, vagy módosított rekordok száma
    static int replaces(QSqlQuery& __q, const cArpTable& __t, int setType = ST_QUERY, qlonglong hsid = NULL_ID);
    static int mac2arps(QSqlQuery& __q, const cMac& __m, tRecordList<cArp>& arps);
    static QList<QHostAddress> mac2ips(QSqlQuery& __q, const cMac& __m);
    static QHostAddress mac2ip(QSqlQuery& __q, const cMac& __m, enum eEx __ex = EX_ERROR);
    static cMac ip2mac(QSqlQuery& __q, const QHostAddress& __a, enum eEx __ex = EX_ERROR);
    static int checkExpired(QSqlQuery& __q);
};

/* ---------------------------------------------------------------- */

class LV2SHARED_EXPORT cDynAddrRange : public cRecord {
    CRECORD(cDynAddrRange);
public:
    static eReasons replace(QSqlQuery &q, const QHostAddress& b, const QHostAddress& e, qlonglong dhcpSrvId, bool exclude, qlonglong subnetId = NULL_ID);
    static int isDynamic(QSqlQuery &q, const QHostAddress& a);
};

/* ---------------------------------------------------------------- */
enum eExecState {
    ES_WAIT    =  0,
    ES_EXECUTE,
    ES_OK,
    ES_FAILE,
    ES_ABORTED
};

EXT_ int execState(const QString& _n, enum eEx __ex = EX_ERROR);
EXT_ const QString& execState(int _e, enum eEx __ex = EX_ERROR);

class LV2SHARED_EXPORT cImport : public cRecord {
    CRECORD(cImport);
};


/* ---------------------------------------------------------------- */
class cInspector;
class cImportParseThread;

enum eParseType {
    PT_PREP, PT_PARSE, PT_POST
};

const QString& parseType(int e, eEx __ex = EX_ERROR);
int parseType(const QString& s, eEx __ex = EX_ERROR);

enum eRegexpAttr {
    RA_CASESENSITIVE, RA_EXACTMATCH, RA_LOOP
};

const QString& regexpAttr(int e, eEx __ex = EX_ERROR);
int regexpAttr(const QString& s, eEx __ex = EX_ERROR);

class LV2SHARED_EXPORT cQueryParser : public cRecord {
    Q_OBJECT
    CRECORD(cQueryParser);
public:
    /// Egy rekord beszúrása
    /// @param q
    /// @param _sid A szolgáltatás azonosító (ID)
    /// @param _ty Típus neve ('prep', 'parse', 'post')
    /// @param _ra A regExp attributumai
    /// @param _re A reguláris kifejezés
    /// @param _cmd Parancs
    /// @param _seq Sorrend vagy prioritás
    static void _insert(QSqlQuery& q, qlonglong _sid, const QString& _ty, qlonglong _ra, const QString& _re, const QString& _cmd, const QString& _not, qlonglong _seq);
    /// Az inspektor objektum beállítása a query parser végrehajtásához, ha az egy inspektor objektum alapján kerül végrehajtásra.
    void setInspector(cInspector *pInsp);
    /// Csak fordítás, az interpretert nem hívja. Interaktív módú végrehajtásnál. A kimeneti szöveget a getText() adja vissza.
    /// A konténer a behelyettesítésekhez egy változó listát ad, mivel ilyenkor pInspector értéke NULL, így az azon keresztüli behelyettesítések nem elérhetőek.
    void setMaps(tStringMap *pVM);
    /// A prep típusú rekord végrehajtása (inspector mód) vagy pufferelése (interaktív mód), ha van
    int prep(cError *&pe);
    /// A szövek keresése a mintákban, és találat esetén a hozzá tartozó parancs végrehajtása (inspector mód) vagy pufferelése (interaktív mód).
    int captureAndParse(QString src, cError *&pe);
    /// A post típusú rekord végrehajtása (inspector mód) vagy pufferelése (interaktív mód), ha van
    int post(cError *&pe);
    /// Egy közvetlen parancs végrehajtása a parser thread-en. A query_parser rekordoktol független funkció.
    cError *execParserCmd(const QString &_cmd, cInspector *pInspector);
    /// Beolvassa az összes megadott _sid -hez tartozó rekordot, és létrehozza, és kitölti a pListCmd és pListRExp konténereket,
    /// valamit a pPrepCmd és pPostCmd stringeket.
    /// Ha a thread értéke true, akkor beolvassa a prep és post parancsokat is.
    /// @param force Akkor is beolvassa a rekordokat, ha a konténerek léteznek, és _sid nem változott.
    int load(QSqlQuery& q, qlonglong _sid = NULL_ID, bool force = true, bool thread = true);
    int delByServiceName(QSqlQuery &q, const QString &__n, bool __pat);
    /// Közvetett végrehajtás esetén a végrehalytandó parancsok szövegét adja vissza.
    /// Közvetett végrehajtás esetén pText nem lehet NULL, és pParserThread -nek az értéke NULL.
    QString getCommands() { CHKNULL(pCommands); return *pCommands; }
    cQueryParser *newChild(cInspector * _isp);
    const QStringList& debugLines() const { return _debugLines; }
    const QString&     exportText() const { return _exportText; }
protected:
    QString getParValue(const QString& name, const QStringList &args);
    QString substitutions(const QString& _cmd, const QStringList& args);
    int execute(cError *&pe, const QString& _cmd, const QStringList& args = QStringList());
    QStringList         *pListCmd;          ///< interpreter parancsok listája
    QList<QRegExp>      *pListRExp;         ///< Reguláris kifelyezések listája (sorrend azonos mint a pListCmd-ben)
    QList<qlonglong>    *pListReAttr;       ///< Reguláris kifelyezések attributumainak listája (sorrend azonos mint a pListCmd-ben)
    QString             *pPrepCmd;          ///< prepare (indító) parancs, ha van, egyébként NULL
    QString             *pPostCmd;          ///< post (záró) parancs, ha van, egyébként NULL
    cInspector          *pInspector;        ///< Tulajdonos pointere, ha egy szálban fut az interpreter
    cImportParseThread  *pParserThread;     ///< Az interpreter szál pointere, vagy NULL
    tStringMap          *pVarMap;           ///< Változó lista, ha az interpreter nem egy szálban fut, és csak közvetlen végrehajtás van.
    QString             *pCommands;         ///< Nem közvetlen végrahajtás esetén a végrehalytandó parancsok ide kerülnek, egyébként null.
    QStringList         _debugLines;
    QString             _exportText;
protected slots:
    void debugLineReady();
};

#endif // SRVDATA

