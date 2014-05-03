#ifndef LV2SERVICE_H
#define LV2SERVICE_H

#include "lv2_global.h"
#include "lanview.h"
#include "lv2data.h"
#include "others.h"

/*// Várakozási állapot
enum eTmStat {
    TM_WAIT    =  0,    ///< A várakozási idő még nem telt le
    TM_ELAPSED =  1,    ///< A várakozási idő letelt
    TM_TO      = -1     ///< A várakozási idő letelt, és a követkeő várakozási dőtartam túl kicsi lett
};*/

/// Lekérdezés eredmonye.
/// Ugyan ez az enumerációs típus reprezentálja az adatbázisban a notifswitch nevű enumerációs típust is,
/// de ott az RS_STAT_MASK és az aszt megelőző elemek nem használhatóak, ill nincs megfelelőjük.
/// Az enumerációs (numerikus) értékek, és az őket reprezentáló adatbázis beli string értékek között a
/// notifSwitch() függvény pár konvertál.
enum eNotifSwitch {
    RS_INVALID     =   -1,  ///< Csak hibajelzésre szolgál
    RS_STAT_SETTED = 0x80,  ///< A status beállítása megtörtént, mask nem valódi status érték, henem egy flag
    RS_SET_RETRY   = 0x40,  ///< Az időzítést normálból->retry-be kel váltani.
    RS_STAT_MASK   = 0x0f,  ///< A valódi státusz maszkja
    RS_ON          =    0,  ///< Az aktuális futási eredmény 'on'
    RS_RECOVERED,           ///< Az aktuális futási eredmény 'recovered'
    RS_WARNING,             ///< Az aktuális futási eredmény 'warning'
    RS_CRITICAL,            ///< Az aktuális futási eredmény 'critical'
    RS_UNREACHABLE,         ///< Az aktuális futási eredmény 'unreachable'
    RS_DOWN,                ///< Az aktuális futási eredmény 'down'
    RS_FLAPPING,            ///< Az aktuális futási eredmény 'flapping'
    RS_UNKNOWN              ///< Az aktuális futási eredmény 'unknown'
};

/// A lekérdezés eredményét reprezebtáló enumerációs értéket konvertálja az adatbázisban tárolt string formára.
/// RS_STAT_SETTED érték konverzió elütt ki lessz maszkolva. Ha a konverzió nem lehetséges, akkor dob egy kizárást.
EXT_ const QString& notifSwitch(int _ns, bool __ex = true);
/// A lekérdezés eredményét reprezebtáló enumerációs érték adatbázisban tárolt string formáráját konvertálja
/// numerikus értékké.
EXT_ int notifSwitch(const QString& _nm, bool __ex = true);


/// cInspect és leszármazottai objektum belső status
enum eInternalStat {
    IS_INIT,    ///< Internal status init
    IS_DOWN,    ///< Internal status down
    IS_REINIT,  ///< Internal status reinit
    IS_RUN      ///< Internal status runing (inited)
};

/// Az ellenörző eéjárás típusa
enum eInspectorType {
    IT_UNKNOWN = -1,
    IT_TIMED   =  1,    ///< Időzített, a fő szálban
    IT_THREAD,          ///< Időzített saját szálként
    IT_CONTINUE,        ///< Belső időzítés, futó saját szál
    IT_PASSIVE          ///< a superior lekérdezés eredményéhez kapcsolódik
};

EXT_ const QString& inspectorType(enum eInspectorType __t, bool __ex = true);
EXT_ eInspectorType inspectorType(const QString& __n, bool __ex = true);

/// Az időzítés típusa
enum eTimerStat {
    TS_STOP,    ///< nincs időzítés
    TS_NORMAL,  ///< normál időzítés (normal_interval)
    TS_RETRY    ///< Hiba miatt gyorsabb időzítés (retry_interval)
};

/// Belső status érték konvertálása stringgé, nem értelmezhető érték esetén dob egy kizárást
EXT_ const QString& internalStatName(eInternalStat is);

class cInspector;

/// @class cInspectorThread
/// Időzített thread
class LV2SHARED_EXPORT cInspectorThread : public QThread {
    Q_OBJECT
public:
    cInspectorThread(cInspector * pp);
    virtual void timerEvent(QTimerEvent * e);
    cInspector& inspector;
protected:
    virtual void run();
};

/// @class cInspector
/// Egy szolgáltatás példány adatai és időzítése
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

    /// Belső státusz beállítása. Virtuális metódus, a bázis osztály esetén az objektum belső státuszát vagyis az intStat adattagot állítja a megadott értékre.
    virtual void setInternalStat(enum eInternalStat is);
    /// Az objektumhoz időzítéséhez tartozó metódus.
    /// Ha az internalStat értéke nem IS_RUN, akkor nem csinál semmit.
    /// Ha a szolgáltatás nem időzített, ill. az állapota alapján nem kéne óra eseménynek bekövetkeznie, akkor kizárást dob.
    /// Futó (IS_RUN), és időzített szolgáltatás esetén egy try blokkban meghívja a run() virtuális metódust.
    /// A rum metódus által visszaadott érték, vagy az esetleges hiba alapján beállítja az adatbázisban a szolgáltatáspéldány állapotát,
    /// valamint állít az időzítésen, ha ez szükséges (normal/retry időzítés kezelése)
    virtual void timerEvent(QTimerEvent * );
    /// A szolgáltatáshoz tartozó tevékenységet végrehajtó virtuális metódus.
    /// A alap objektumban a metódus nem csinál semmit (egy debug üzenet feltéteées kiírásán túl), csak visszatér egy RS_ON értékkel.
    /// @return A szolgáltatás állpota, ill. a tevékenység eredménye.
    virtual enum eNotifSwitch run(QSqlQuery& q);
    /// Futás időzítés indítása
    virtual void start();
    /// Futás/időzítés leállítása, ha nem futott, és __ex = true, akkor dob egy kizárást.
    virtual void stop(bool __ex = true);
    /// Egy alárendelt szolgáltatás objektum létrehozása. Az alapértelmezett metódus egy NULL pointert ad vissza.
    /// Egy alternatív lehetőség a subordinates konténer elemeinek a feltöltésére, ezért nem tisztán virtuális.
    /// @param q Az adatbázis műveletekhez használható objektum.
    /// @param hsid host_service_id
    /// @param hoid A node objektum típusát azonosító tableoid
    /// @param A parent host_service_id
    virtual cInspector *newSubordinate(QSqlQuery& q, qlonglong hsid, qlonglong hoid = NULL_ID, cInspector *pid = NULL);
    /// A QThread objektum ill. az abból származtatott objektum allokálása. Az alap metódus egy QThread objektumot allokál.
    virtual QThread *newThread();
    /// Feltölti a subordinates konténert. Hiba esetén dob egy kizárást, de ha nincs mivel feltölteni a subordinatest, az nem hiba.
    /// Hasonló a setSubsT() template metódushoz, csak itt az objektum típusa a newSubordinate() virtuáéis metódus által meghatározott.
    /// @param q az adabázis művelethez használlható objektum.
    /// @param qs Opcionális query string, A stringben a %1 karakter a hostServiceId-vel helyettesítődik.
    /// @param qsp Opcíonális query string, ha rekurzyvan meghívásra kerül a metódus superior=per paraméter miatt. A stringben a %1 az aktuális hostServiceId-vel helyettesítődik.
    /// @param mp Kereső minta a services.properties mezőre (LIKE) opcionális
    virtual void setSubs(QSqlQuery& q, const QString& qs = _sNul);
    /// A pHost, pService és hostService adattagok feltöltése után az inicializálás befejezése
    /// @param Superior tulajdonság esetén az alárendeltek beolvasásához használt objektum, a setSubs-nak adja át
    /// @param Szinté az opcionális alárendeltek beolvasásáoz egy opcionális query string, a setSubs második paramétere.
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());

    /// hasonló a cRecord get(const QString& __n) metódusához. A mezőt elöszőr a hostService adattagban keresi, ha viszont az NULL,
    /// akkor aservices adattagból olvassa be.
    /// @param __n A mező név
    /// @return A mező értéke.
    QVariant get(const QString& __n) const;
    /// A services és a host_services rekordban a atrubutes nezőt vágja szát, és az elemeket elhelyezi a pMagicMap pointer által mutatott konténerbe.
    /// Ha pMagicMap egy NULL pointer, akkor a művelet elött megallokálja a konténert, ha nem NULL, akkor pedig törli a konténer tartalmát.
    tMagicMap& splitMagic(bool __ex = true);
    /// Visszaadja a pMagicMap által mutatott konténer referenciáját. Ha pMagicMap értéke NULL, akkor hívja a splitMagic() metódust, ami megallokálja
    /// és feltölti a konténert.
    tMagicMap& magicMap(bool __ex = true)                               { if (pMagicMap == NULL) splitMagic(__ex); return *pMagicMap; }
    /// A megadott kulcs alapján visszaadja a magicMap konténerből a paraméter értéket a név alapján. Ha a konténer nincs megallokálva, akkor megallokálja
    /// és feltölti.
    /// @return ha a kulcshoz tartozik megadott néven paraméter, akkor az értéket adja vissza, vagy üres stringet.
    QString magicParam(const QString& __nm, bool __ex = true) { return ::magicParam(__nm, magicMap(__ex)); }
    /// Magadot kulcsal egy paraméter keresése.
    /// @return találat esetén true.
    bool findMagic(const QString &_nm, bool __ex = true)      { return ::findMagic(_nm, magicMap(__ex)); }
    /// Saját adatok beállítása. Hiba esetén dob egy kizárást.
    /// A pNode adattag egy cNode objektumra fog mutatni, ami a sajátgép adatait fogja tartalmazni, feltéve, hogy az adatbázis ezt tartalmazza.
    /// Akkor is cNode lessz az adattípus, ha a sajátgép történetesen egy SNMP eszközként szerepel az adatbázisban.
    /// A saját géphez természetesen bejegyezve kell lennie a megadott nevű szolgáltatásnak.
    /// @param __q Az adatbázis művelethez használható QSqlQuery objektum referenciája
    /// @param __sn Szolgálltatás neve
    void self(QSqlQuery& q, const QString& __sn);
    /// A belső statuszt konvertálja stringgé.
    const QString& internalStatName() { return ::internalStatName(internalStat); }

    // Adattagok
    /// Objektum típus
    enum eInspectorType inspectorType;
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
    tMagicMap          *pMagicMap;
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
    QThread            *pThread;
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
protected:
    /// A lekérdezés típusát azonosító services rekord objektum pointere, vagy NULL ha ismeretlen, vagy még nincs beállítva.
    /// Nem kell/szabad felszabadítani a pointert!
    const cService     *pService;
public:
    /// A szolgáltatás cService objektumára mutató referenciával tér vissza
    /// @param __ex Ha értéke true, és nem ismert a szolgáltatás objektum (pService értéke NULL) akkor dob egy kizárást
    /// @return A szolgáltatás objektum referenciája, ha __ex értéke false, és nem ismert a szolgáltatés, akkor egy üres objektum őpintere.
    const cService& service(bool __ex = true) const {
        if (pService == NULL) {
            if (__ex) EXCEPTION(EDATA);
            return cService::_nul();
        }
        return *pService;
    }
    /// A megadott nevű szolgáltatás cService objektumára mutató referenciával tér vissza, továbbá beállítja az objektumra a pService pointert.
    /// Ha egy üres paraméterrel tért vissza, akkor a pService értéke NULL lessz, és nem az üres objektumra fog mutatni.
    /// @param __q Ha szükség van adatbázis műveletre, akkor a művelethez használható objektum referenciája.
    /// @param __sn A szolgáltatás név
    /// @param __ex Ha értéke true, és nem ismert a megadott nevű szolgáltatás objektum (pService értéke NULL) akkor dob egy kizárást
    /// @return A szolgáltatás objektum referenciája, ha __ex értéke false, és nem ismert a szolgáltatés, akkor egy üres objektum pointere.
    const cService& service(QSqlQuery& __q, const QString& __sn, bool __ex = true) {
        pService = &cService::service(__q, __sn, __ex);
        if (__ex == false && pService->isNull()) pService = NULL;
        return service();
    }
    /// A megadott azonosítójú szolgáltatás cService objektumára mutató referenciával tér vissza, továbbá beállítja az objektumra a pService pointert.
    /// Ha egy üres paraméterrel tért vissza, akkor a pService értéke NULL lessz, és nem az üres objektumra fog mutatni.
    /// @param __q Ha szükség van adatbázis műveletre, akkor a művelethez használható objektum referenciája.
    /// @param __sn A szolgáltatás ID
    /// @param __ex Ha értéke true, és nem ismert a megadott nevű szolgáltatás objektum (pService értéke NULL) akkor dob egy kizárást
    /// @return A szolgáltatás objektum referenciája, ha __ex értéke false, és nem ismert a szolgáltatés, akkor egy üres objektum pointere.
    const cService& service(QSqlQuery& __q, qlonglong __id, bool __ex = true) {
        pService = &cService::service(__q, __id, __ex);
        if (__ex == false && pService->isNull()) pService = NULL;
        return service();
    }
    /// A szolgáltatás cService objektumára mutató pointert állítja be
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
    cInspector& parent()           { if (pParent == NULL) EXCEPTION(EDATA); return *pParent; }
    template <class T> static qlonglong getIdT(const T *p, bool __ex) {
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
    qlonglong nodeId(bool __ex = true) const     { return getIdT<cNode>(pNode, __ex); }
    qlonglong serviceId(bool __ex = true) const  { return getIdT<cService>(pService, __ex); }
    qlonglong hostServiceId() const              { return hostService.getId(); }
    qlonglong parentId(bool __ex = true) const {
        if (pParent == NULL) {
            if (__ex) EXCEPTION(EDATA);
            return NULL_ID;
        }
        return pParent->hostServiceId();
    }
    qlonglong portId() const            { return nPort().getId(); }
    const cService& primeService()      { if (pPrimeService == NULL) EXCEPTION(EPROGFAIL); return *pPrimeService; }
    qlonglong primeServiceId()          { return primeService().getId(); }
    QString primeServiceName()          { return primeService().getName(); }
    const cService& protoService()      { if (pProtoService == NULL) EXCEPTION(EPROGFAIL); return *pProtoService; }
    qlonglong protoServiceId()          { return protoService().getId(); }
    QString protoServiceName()          { return protoService().getName(); }
    QString name();

    /// Ha az objektum időzített.
    bool isTimed() const { return inspectorType == IT_TIMED  || inspectorType == IT_THREAD; }
    /// Ha az objektum önálló szálon fut
    bool isThread() const { return inspectorType == IT_THREAD || inspectorType == IT_CONTINUE; }
    /// Ha a lekérdezést el kell indítani / nem passzív
    bool needStart() const { return inspectorType == IT_TIMED  || inspectorType == IT_THREAD || inspectorType == IT_CONTINUE; }
    /// A statikus adattagokat (tableoid-k) inicializálja, ha ez még nem történt meg (értékük NULL_ID).
    /// A tableoid értékek csak a main objektum (lnaview2) létrehozása után kérdezhetőek le, miután már meg lett nyitva az adatbázis.
    static void initStatic() {
        if (nodeOid == NULL_ID) {   // Ha még nincsenek inicializálva az OID-k
            nodeOid = cNode().tableoid();
            sdevOid = cSnmpDevice().tableoid();
        }
    }
    /// A lehetséges node típusok tableoid értékei
    static qlonglong nodeOid;
    static qlonglong sdevOid;
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
    /// A thread run() metódusa hívja.
    /// Ha időzített a thread esetén indít egy timer-t, és hívja a thread exec() metódust.
    ///
    virtual bool threadPrelude(QThread& t);
private:
    void preInit();
};

#endif // LV2SERVICE_H
