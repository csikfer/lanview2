#ifndef LV2SERVICE_H
#define LV2SERVICE_H

#include <QTimer>

#include "lanview.h"
#include "lv2data.h"
#include "srvdata.h"
#include "vardata.h"
#include "others.h"

/*// Várakozási állapot
enum eTmStat {
    TM_WAIT    =  0,    ///< A várakozási idő még nem telt le
    TM_ELAPSED =  1,    ///< A várakozási idő letelt
    TM_TO      = -1     ///< A várakozási idő letelt, és a követkeő várakozási dőtartam túl kicsi lett
};*/


/// cInspect és leszármazottai objektum belső status
enum eInternalStat {
    IS_INIT,        ///< Internal status init
    IS_DOWN,        ///< Internal status down
    IS_RUN,         ///< Internal status runing (inited)
    IS_SUSPENDED,   ///< Felföggesztve, időzítésre várakozik
    IS_STOPPED,     ///< Lefutott
    IS_ERROR        ///< Hiba miatt leállt
};

/// Az ellenörző eljárás típusa
enum eInspectorType {
    IT_CUSTOM               = 0,        ///< Egyedi

    IT_TIMING_CUSTOM        = 0x0000,   ///< Egy szál, saját időzítés (alapértelmezés)
    IT_TIMING_TIMED         = 0x0001,   ///< Időzített, a fő szálban
    IT_TIMING_THREAD        = 0x0002,   ///< Saját szálként
    IT_TIMING_TIMEDTHREAD   = 0x0003,   ///< Időzített saját szálként
    IT_TIMING_PASSIVE       = 0x0004,   ///< Inaktív eleme a lekérdezés fának.
    IT_TIMING_POLLING       = 0x0008,   ///< Időzítés nélkül egyszer fut/szekvenciális
    IT_TIMING_MASK          = 0x000F,   ///< Maszk: ütemezés

    IT_NO_PROCESS           = 0x0000,   ///< Nem végrehajtható program hívása
    IT_PROCESS_RESPAWN      = 0x0010,   ///< A program (daemon) újrahívása, ha kilépett
    IT_PROCESS_CONTINUE     = 0x0020,   ///< A program (daemon) csak akkor lép ki, ha hiba van, vagy leállítoják
    IT_PROCESS_POLLING      = 0x0030,   ///< A programot start() indítja, lefut és kilép
    IT_PROCESS_MASK_NOTIME  = 0x0030,   ///< Maszk: Időzítés nélküli indítás
    IT_PROCESS_TIMED        = 0x0040,   ///< A programot időzítve kell indítani
    IT_PROCESS_CARRIED      = 0x0080,   ///< A hívott program beállítja a status-t
    IT_PROCESS_MASK         = 0x00F0,   ///< Maszk: önálló processz indítása

    IT_METHOD_CUSTOM        = 0x0000,   ///< Egyedi
    IT_METHOD_NAGIOS        = 0x0100,   ///< Nagios plugin
    IT_METHOD_MUNIN         = 0x0200,   ///< Munin plugin
    IT_METHOD_QPARSE        = 0x0300,   ///< Query parser
    IT_METHOD_PARSER        = 0x0400,   ///< Parser szülő objektum a query parser(ek)hez
    IT_METHOD_CARRIED       = 0x0800,   ///<
    IT_METHOD_INSPECTOR     = 0x1000,   ///< Egy LanView2 service (cInspector) APP
    IT_METHOD_MASK          = 0x1F00,

    IT_SUPERIOR             = 0x2000,   ///< Alárendelt funkciók vannak
    IT_MAIN                 = 0x4000,   ///< Fő folyamat, nincs parent
    IT_OWNER_QUERY_PARSER   = 0x8000    ///< A megallokált pQparse pointer tulajdonosa
};

/// Az időzítés típusa ill. állapota
enum eTimerStat {
    TS_STOP,    ///< nincs időzítés (vagy még nem elindított)
    TS_NORMAL,  ///< normál időzítés (normal_interval)
    TS_RETRY,   ///< Hiba miatt gyorsabb időzítés (retry_interval)
    TS_FIRST    ///< Első alkalom (egy a normal_interval -nal rövidebb véletlenszerű időzítés)
};

/// Belső status érték konvertálása stringgé, nem értelmezhető érték esetén dob egy kizárást
EXT_ QString internalStatName(eInternalStat is);

enum eNagiosPluginRetcode {
    NR_OK       = 0,
    NR_WARNING  = 1,
    NR_CRITICAL = 2,
    NR_UNKNOWN  = 3
};

class cInspector;

/// @class cInspectorThread
/// Időzített thread
class LV2SHARED_EXPORT cInspectorThread : public QThread {
    friend class cInspector;
    Q_OBJECT
public:
    cInspectorThread(cInspector * pp);
    cInspector& inspector;
    cError     *pLastError;
protected:
    virtual void run();
    virtual void doInit();
    virtual void doDown();
    virtual void doRun();
    QTimer      *pTimer;
protected slots:
    virtual void timerEvent();
};

class LV2SHARED_EXPORT cInspectorProcess : public QProcess {
    Q_OBJECT
public:
    cInspectorProcess(cInspector *pp);
    /// A inspector objektumban meghatározott checkCmd paramcsot elindítja,
    /// megvárja, míg elindul. Ezután, ha a sync igaz, akkor megvárja míg kilép.
    /// Ha viszint sync hamis, akkor csatlakoztatja a processFinished(), és
    /// processReadyRead() slot-okat.
    /// Hiba esetén dob egy kizárást.
    /// @param startTo Maximális várakozási dő a parancs indulására millisec-ben, alapértelmezetten 5 másodperc.
    /// @param sync Ha értéke true, akkor megvárja, amíg kilép a hívott program, ha false, akkor a slot-okat cstlakoztatja, és kilép.
    /// @param stopTo Maximális várakozási dő a parancs lefutására millisec-ben, alapértelmezetten 30 másodperc. Ha sync értéke false, akkor érdektelen.
    virtual int startProcess(bool conn = false, int startTo = 5000, int stopTo = 0);
    cInspector& inspector;
protected slots:
    virtual void processFinished(int _exitCode, QProcess::ExitStatus exitStatus);
    virtual void processReadyRead();
protected:
    int         reStartCnt;         ///< Hiba számláló
    int         reStartMax;         ///< Maximális megengedett hiba szám
    int         errCntClearTime;    ///< Hiba számláló törlése, ha nincs hiba ennyi msec ideig.
    int         maxArcLog;
    int         maxLogSize;
    int         logNull;
    QFile       actLogFile;
};

/// @class cInspector
/// Egy szolgáltatás példány adatai és időzítése, kezelése
/// Az osztály közvetlenül is használható, nem klasszikus értelemben vett bázis osztály,
/// mivel minden virtuális fügvénye definiált, és ezek közül csak néhányat kell(het) fellüldefiniálni
/// a feladattól föggően.
class LV2SHARED_EXPORT cInspector : public QObject {
    friend class cInspectorThread;
    Q_OBJECT
public:
    /// Üres konstruktor
    cInspector(cInspector *_par = NULL);
    /// Az objektumot mint saját szolgálltatás tölti fel
    /// @param q Az adatbázis művelethez használható QSqlQuery objektum referenciája.
    /// @param sn A szolgáltatás neve
    cInspector(QSqlQuery &q, const QString& sn);
    /// Az objektumot a megadott ID-k alapján tölti fel
    /// Ha a megadott host_servce_id értéke NULL (NULL_ID), akkor a szükséges mezők már le lettek kérdezve (first() metódust is meg lett hívva),
    /// azokat a __q objektumból kell kinyerni, a rekord sorrend : host_services, nodes (ill. a tableoid-vel definiállt leszármazott).
    /// @param q Az adatbázis művelethez használható QSqlQuery objektum referenciája, ill. a szükséges adatokat beolvasó lekérdezés eredménye.
    /// @param __host_service_id host_services rekord ID, vagy NULL, ha a szükséges mezőket már lekérdeztők a __q -val.
    /// @param __tableoid A node rekord tábla OID-je (node  tényleges típusát azonosítja), alapértelmezett (NULL_ID esetén) a hosts tábla.
    /// @param __par A parent, vagy NULL
    cInspector(QSqlQuery& q, qlonglong __host_service_id = NULL_ID, qlonglong __tableoid = NULL_ID, cInspector * __par = NULL);
    /// Destruktor
    virtual ~cInspector();

    /// Az objektumhoz időzítéséhez tartozó metódus. (külön szál esetén is ez a metódus kerül meghívásra).
    /// Ha az internalStat értéke nem IS_SUSPENDED vagy IS_STOPPED, akkor csak letesz egy app_memos rekordot..
    /// Ha a szolgáltatás nem időzített, ill. az állapota alapján nem kéne óra eseménynek bekövetkeznie, akkor kizárást dob.
    /// Időzített szolgáltatás esetén meghívja a doRun() virtuális metódust.
    /// A doRum metódus által visszaadott állpot érték alapján állít az időzítésen, ha ez szükséges (normal/retry időzítés kezelése)
    virtual void timerEvent(QTimerEvent * );
    /// Végrehajtja a run() virtuális metódust. A visszatérési érték és futási idő alapján állítja a szolgáltatás
    /// példány állpotát. A run() metüdust egy try blokba, és SQL tranzakció blokkban hívja.
    virtual bool doRun(bool __timed);
    /// A szolgáltatáshoz tartozó tevékenységet végrehajtó virtuális metódus.
    /// A alap objektumban a metódus ha pProcess = NULL, akkor nem csinál semmit (egy debug üzenet feltételes kiírásán túl),
    /// csak visszatér egy RS_UNKNOWN értékkel.
    /// Ha pProcess pointer nem NULL, akkor végrehajtja a megadott parancsot, és az eredménnyel hívja a parse() metódust.
    /// @return A szolgáltatás állpota, ill. a tevékenység eredménye.
    virtual int run(QSqlQuery& q, QString &runMsg);
    /// Szöveg (parancs kimenet) értelmezése.
    /// Ha meg van adva kölső ellenörző program, akkor az alapértelmezett run() metódus hívja a végrehajtott parancs kimenetével.
    virtual enum eNotifSwitch parse(int _ec, QIODevice &text);
    /// Futás időzítés indítása
    virtual void start();
    /// Futás/időzítés leállítása, ha időzített, de nem volt időzítés, és __ex = EX_ERROR, akkor dob egy kizárást.
    /// Csak az aktuális objektumot stoppolja le, a sub objektumokat törli: dropSubs() -al.
    virtual void drop(enum eEx __ex = EX_ERROR);
    /// Egy alárendelt szolgáltatás objektum létrehozása. Alapértelmezetten egy cInspector objektumot hoz létre.
    /// Az alapőértelmezett setSubs() metódus hívja a gyerek objektumok létrehozásához, ha azt akarjuk,
    /// hogy ijenkkor egy cIspector leszármazott jöjjön létre, akkor a metódus fellüldefiniálandü.
    /// @param _hsid host_service_id
    /// @param _toid A node objektum típusát azonosító tableoid
    /// @param _par A parent objekrum
    virtual cInspector *newSubordinate(QSqlQuery& q, qlonglong _hsid, qlonglong _toid = NULL_ID, cInspector *_par = NULL);
    /// A QThread objektum ill. az abból származtatott objektum allokálása. Az alap metódus egy cInspectorThread objektumot allokál.
    /// Amennyiben a szervíz a 'syscron', akkor a megallokált objektum a cSysCronThread (Win: not supported).
    virtual cInspectorThread *newThread();
    virtual cInspectorProcess *newProcess();
    /// Feltölti a subordinates konténert. Hiba esetén dob egy kizárást, de ha nincs mivel feltölteni a subordinatest, az nem hiba.
    /// At új objektum típusa a newSubordinate() virtuális metódus által meghatározott.
    /// @param q az adabázis művelethez használlható objektum.
    /// @param qs Opcionális query string, A stringben a %1 karakter a hostServiceId-vel helyettesítődik.
    virtual void setSubs(QSqlQuery& q, const QString& qs = _sNul);
    /// A pHost, pService és hostService adattagok feltöltése után az inicializálás befejezése
    /// Logikailag a konstruktor része lenne, de a virtuális metódusok miatt külön kell hívni a konstruktor után.
    /// @param q Superior tulajdonság esetén az alárendeltek beolvasásához használt objektum, a setSubs-nak adja át
    /// @param qs Szintén az opcionális alárendeltek beolvasásáoz egy opcionális query string, a setSubs második paramétere.
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    /// A thread inicializáló rutinjában meghívorr metódus, az objektum egyedi initje
    /// Alapértelmezetten egy üres (azonnal visszatér) metódus.
    virtual void threadPreInit();
    /// Beolvassa a szolgáltatás példányhoz tartozó változókat
    tOwnRecords<cServiceVar, cHostService> *fechVars(QSqlQuery& q);
    /// hasonló a cRecord get(const QString& __n) metódusához. A mezőt elöszőr a hostService adattagban keresi, ha viszont az NULL,
    /// akkor aservices adattagból olvassa be, majd a prime és végül a proto szervíz rekordbol (ha van).
    /// @param __n A mező név
    /// @return A mező értéke.
    cRecordFieldConstRef get(const QString& __n) const;
    /// A services és a host_services rekordban a features nezőt vágja szét, és az elemeket elhelyezi a pFeatures pointer által mutatott konténerbe.
    /// Ha pFeatures egy NULL pointer, akkor a művelet elött megallokálja a konténert, ha nem NULL, akkor pedig törli a konténer tartalmát.
    /// A feldolgozás sorrendje: proto, prime, servece, host_service, lásd még a cFeatures osztály split() metódusát
    /// Az egymás utáni feldolgozott features mezők eredményét összefésüli, egy ismételten megadott paraméter érték az előzőt fellülírja,
    /// a '!' érték pedig törli.
    cFeatures& splitFeature(enum eEx __ex = EX_ERROR);
    /// Visszaadja a pMagicMap által mutatott konténer referenciáját. Ha pMagicMap értéke NULL, akkor hívja a splitMagic() metódust, ami megallokálja
    /// és feltölti a konténert.
    cFeatures& features(enum eEx __ex = EX_ERROR)   { if (_pFeatures == NULL) splitFeature(__ex); return *_pFeatures; }
    /// A megadott kulcs alapján visszaadja a magicMap konténerből a paraméter értéket a név alapján. Ha a konténer nincs megallokálva, akkor megallokálja
    /// és feltölti.
    /// @return Egy string, a paraméter érték, ha nincs ilyen paraméter, akkor a NULL string, ha viszont nincs paraméternek értéke, akkor egy üres string
    QString feature(const QString& __nm, enum eEx __ex = EX_ERROR) { return features(__ex).value(__nm); }
    bool isFeature( const QString& __nm, enum eEx __ex = EX_ERROR) { return features(__ex).contains(__nm); }
    ///
    int getInspectorType(QSqlQuery &q);
    /// Saját adatok beállítása. Hiba esetén dob egy kizárást.
    /// A pNode adattag egy cNode objektumra fog mutatni, ami a sajátgép adatait fogja tartalmazni, feltéve, hogy az adatbázis ezt tartalmazza.
    /// Akkor is cNode lessz az adattípus, ha a sajátgép történetesen egy SNMP eszközként szerepel az adatbázisban.
    /// A saját géphez természetesen bejegyezve kell lennie a megadott nevű szolgáltatásnak.
    /// Teszt opció: A tesztelés megkönnyítése érdekében
    /// @param __q Az adatbázis művelethez használható QSqlQuery objektum referenciája
    /// @param __sn Szolgálltatás neve (lsd.: teszt opció fentebb!)
    void self(QSqlQuery& q, const QString& __sn);
    /// A belső statuszt konvertálja stringgé.
    QString internalStatName() { return ::internalStatName(internalStat); }
    /// A parancs string, és behelyettesítéseinek a végrahajtása.
    /// A parancs path a checkCmd adattagba, a paraméterei pedig a checkCmdArgs konténerbe kerülnek.
    /// @return 0, ha nincs parancs string, 1, ha van és checkCmd beállítva, -1 ha a parancs az éppen futó process
    virtual int getCheckCmd(QSqlQuery &q);
    // Adattagok
    /// Objektum típus
    int inspectorType;
    /// Belső status
    enum eInternalStat  internalStat;
    /// Az időzítés statusa
    enum eTimerStat     timerStat;
    /// A parent szolgáltatás pointere, vagy NULL
    cInspector         *pParent;
    /// A konkrét lekérdzést azonosító host_services rekord objektum
    cHostService        hostService;
    /// A node rekord objektum pointere (node, host, snmpdevice)
    cNode              *pNode;
    /// A port objektum pointere, ha meg van adva a hos_services rekordban, egyébként NULL
    cNPort             *pPort;
    /// magicMap konténer, vagy null pointer, ha még nincs feltöltve
    cFeatures        *_pFeatures;
    /// Két lekérdezés közötti idő intervallum ezred másodpercben normal_check_interval
    qint64              interval;
    /// Hiba esetén az időzítés
    qint64              retryInt;
    /// Az időzítő QTimer azonosítója
    int                 timerId;
    /// Objektum a futási idő mérésére
    QElapsedTimer       lastRun;
    /// Az előző két run metódus hívás között eltelt idő ms-ben
    qlonglong           lastElapsedTime;
    /// Ha ő egy thread, akkor a QThread objektumra mutat, egyébként NULL
    cInspectorThread   *pInspectorThread;
    /// Opcionális parancs
    QString             checkCmd;
    /// Opcionális parancs argumentumok
    QStringList         checkCmdArgs;
    /// Parancs objektum, vagy NULL
    cInspectorProcess  *pProcess;
    /// Parancs kimenetet értelmező objektum, ha van
    cQueryParser       *pQparser;
    /// Adatbázis műveletekhez használt objektum
    QSqlQuery          *pq;
    /// Ha superior szolgálltatásról van szó, akkor az alárendeltek listájára mutató pointer, egyébként NULL.
    QList<cInspector*> *pSubordinates;
    /// Ha van proto_service_id, akkor a protocol service objektumra mutat, egyébként NULL.
    /// Nem kell/szabad felszabadítani a pointert!
    const cService     *pProtoService;
    /// Ha van prime_service_id, akkor a prime service objektumra mutat, egyébként NULL.
    /// Nem kell/szabad felszabadítani a pointert!
    const cService     *pPrimeService;
    /// Szabad felhasználású flag
    bool    flag;
    /// Változók, ha vannak, vagy NULL.
    tOwnRecords<cServiceVar, cHostService>    *pVars;
protected:
    /// A lekérdezés típusát azonosító services rekord objektum pointere, vagy NULL ha ismeretlen, vagy még nincs beállítva.
    /// Nem kell/szabad felszabadítani a pointert!
    const cService     *pService;
    int getInspectorTiming(const QString &value);
    int getInspectorProcess(const QString &value);
    int getInspectorMethod(const QString &value);
    enum eNotifSwitch parse_munin(int _ec, QIODevice &text);
    enum eNotifSwitch parse_nagios(int _ec, QIODevice &text);
    enum eNotifSwitch parse_qparse(int _ec, QIODevice &text);
public:
    /// A szolgáltatás cService objektumára mutató referenciával tér vissza
    /// @param __ex Ha értéke true, és nem ismert a szolgáltatás objektum (pService értéke NULL) akkor dob egy kizárást
    /// @return A szolgáltatás objektum referenciája, ha __ex értéke false, és nem ismert a szolgáltatés, akkor egy üres objektum őpintere.
    const cService *service(enum eEx __ex = EX_ERROR) const {
        if (pService == NULL) {
            if (__ex) EXCEPTION(EDATA);
            return NULL;
        }
        return pService;
    }
    /// A megadott nevű szolgáltatás cService objektumára mutató referenciával tér vissza, továbbá beállítja az objektumra a pService pointert.
    /// Ha egy üres paraméterrel tért vissza, akkor a pService értéke NULL lessz, és nem az üres objektumra fog mutatni.
    /// @param __q Ha szükség van adatbázis műveletre, akkor a művelethez használható objektum referenciája.
    /// @param __sn A szolgáltatás név
    /// @param __ex Ha értéke true, és nem ismert a megadott nevű szolgáltatás objektum (pService értéke NULL) akkor dob egy kizárást
    /// @return A szolgáltatás objektum referenciája, ha __ex értéke false, és nem ismert a szolgáltatés, akkor egy üres objektum pointere.
    const cService *service(QSqlQuery& __q, const QString& __sn, enum eEx __ex = EX_ERROR) {
        pService = cService::service(__q, __sn, __ex);
        return service(__ex);
    }
    /// A megadott azonosítójú szolgáltatás cService objektumára mutató referenciával tér vissza, továbbá beállítja az objektumra a pService pointert.
    /// Ha egy üres paraméterrel tért vissza, akkor a pService értéke NULL lessz, és nem az üres objektumra fog mutatni.
    /// @param __q Ha szükség van adatbázis műveletre, akkor a művelethez használható objektum referenciája.
    /// @param __sn A szolgáltatás ID
    /// @param __ex Ha értéke true, és nem ismert a megadott nevű szolgáltatás objektum (pService értéke NULL) akkor dob egy kizárást
    /// @return A szolgáltatás objektum referenciája, ha __ex értéke false, és nem ismert a szolgáltatés, akkor egy üres objektum pointere.
    const cService *service(QSqlQuery& __q, qlonglong __id, enum eEx __ex = EX_ERROR) {
        pService = cService::service(__q, __id, __ex);
        return service(__ex);
    }
    /// A szolgáltatás cService objektumára mutató pointert állítja be.
    /// A pointer a cIspector tulajdonába kerúl, a destruktora felszabadítja!
    /// @param _ps A szolgálltatás típus objektumra mutató pointer
    /// @return A szolgáltatás objektum referenciája.
    const cService& service(const cService * _ps)  { return *(pService = _ps); }
    ///
    const cNode& node() const           { if (pNode == NULL) EXCEPTION(EDATA); return *pNode; }
    cNode& node()                       { if (pNode == NULL) EXCEPTION(EDATA); return *pNode; }
    const cNode& host() const           { if (pNode == NULL || !(pNode->descr() >= cNode().descr()) ) EXCEPTION(EDATA); return *(dynamic_cast<const cNode*>(pNode)); }
    cNode& host()                       { if (pNode == NULL || !(pNode->descr() >= cNode().descr()) ) EXCEPTION(EDATA); return *(dynamic_cast<cNode*>(pNode)); }
    const cSnmpDevice& snmpDev() const  { if (pNode == NULL || !(pNode->descr() >= cSnmpDevice().descr()) ) EXCEPTION(EDATA); return *(dynamic_cast<const cSnmpDevice*>(pNode)); }
    cSnmpDevice& snmpDev()              { if (pNode == NULL || !(pNode->descr() >= cSnmpDevice().descr()) ) EXCEPTION(EDATA); return *(dynamic_cast<cSnmpDevice*>(pNode)); }
    const cInspector& parent() const{if (pParent == NULL) EXCEPTION(EDATA); return *pParent; }
    cInspector& parent()                { if (pParent == NULL) EXCEPTION(EDATA); return *pParent; }
    template <class T> static qlonglong getIdT(const T *p, enum eEx __ex = EX_ERROR) {
        if (p == NULL) {
            if (__ex) EXCEPTION(EDATA);
            return NULL_ID;
        }
        return p->getId();
    }
    const cNPort& nPort() const         { if (pPort == NULL) EXCEPTION(EDATA); return *pPort; }
    cNPort& nPort()                     { if (pPort == NULL) EXCEPTION(EDATA); return *pPort; }
    const cInterface& interface() const { if (pPort == NULL) EXCEPTION(EDATA); return *pPort->creconvert<cInterface>(); }
    cInterface& interface()             { if (pPort == NULL) EXCEPTION(EDATA); return *pPort->reconvert<cInterface>(); }
    qlonglong nodeId(enum eEx __ex = EX_ERROR) const     { return getIdT<cNode>(pNode, __ex); }
    qlonglong serviceId(enum eEx __ex = EX_ERROR) const  { return getIdT<cService>(pService, __ex); }
    qlonglong hostServiceId() const              { return hostService.getId(); }
    qlonglong parentId(enum eEx __ex = EX_ERROR) const {
        if (pParent == NULL) {
            if (__ex) EXCEPTION(EDATA);
            return NULL_ID;
        }
        return pParent->hostServiceId();
    }
    qlonglong portId() const            { return nPort().getId(); }
    const cService& primeService() const{ if (pPrimeService == NULL) EXCEPTION(EDATA); return *pPrimeService; }
    qlonglong primeServiceId() const    { return primeService().getId(); }
    QString primeServiceName() const    { return primeService().getName(); }
    const cService& protoService() const{ if (pProtoService == NULL) EXCEPTION(EDATA); return *pProtoService; }
    qlonglong protoServiceId() const    { return protoService().getId(); }
    QString protoServiceName() const    { return protoService().getName(); }
    QString name() const;
    QString getParValue(QSqlQuery &q, const QString& name, bool *pOk = NULL);

    ///
    int timing() const { return inspectorType & IT_TIMING_MASK; }
    int process() const{ return inspectorType & IT_PROCESS_MASK; }
    int method() const { return inspectorType & IT_METHOD_MASK; }
    /// Ha az objektum időzített.
    bool isTimed() const { return inspectorType & (IT_TIMING_TIMED | IT_PROCESS_TIMED) ; }
    /// Ha az objektum önálló szálon fut
    bool isThread() const { return inspectorType & IT_TIMING_THREAD; }
    /// Ha a lekérdezést passzív
    bool passive() const { return inspectorType & IT_TIMING_PASSIVE; }
    /// A statikus adattagokat (tableoid-k) inicializálja, ha ez még nem történt meg (értékük NULL_ID).
    /// A tableoid értékek csak a main objektum (lnaview2) létrehozása után kérdezhetőek le, miután már meg lett nyitva az adatbázis.
    static void initStatic();
    // A lehetséges node típusok tableoid értékei
    // Erre ki kéne találni valamit, hogy ne lehessen elrontani
    static qlonglong nodeOId;
    static qlonglong sdevOId;
    static qlonglong syscronId;
protected:
    /// Az időzítés módosítása
    void toRetryInterval();
    /// Az időzítés módosítása
    void toNormalInterval();
    /// Törli a pSubordinates pointert, és a konténer elemeit, az összes pointert felszabadítja
    void dropSubs();
    ///
    void down();
    ///
    void startSubs();
private:
    /// Alaphelyzetbe állítja az adattagokat (a konstruktorokhoz)
    void preInit(cInspector *__par);
    /// Típus hiba üzenethez azonosító adatok
    QString typeErrMsg(QSqlQuery &q);
public:
    static qlonglong rnd(qlonglong i, qlonglong m = 1000);
};

#endif // LV2SERVICE_H
