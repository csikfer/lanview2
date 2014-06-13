
#include "lanview.h"
#include "lv2data.h"
//#include "scan.h"
#include "lv2service.h"
#include "time.h"


cInspectorThread::cInspectorThread(cInspector *pp)
    : QThread(), inspector(*pp)
{
    ;
}

void cInspectorThread::run()
{
    _DBGFN() << inspector.name() << endl;
    if (inspector.threadPrelude(*this)) {   // Timed, event loop
        PDEB(VERBOSE) << "Thread " << inspector.name() << " start timer and loop..." << endl;
        exec();
    }
    else {                                  // Continue, run
        PDEB(VERBOSE) << "Thread " << inspector.name() << " started..." << endl;
        inspector.run(*inspector.pq);
    }
    _DBGFNL() << inspector.name() << endl;
    inspector.moveToThread(qApp->thread());
}

void cInspectorThread::timerEvent(QTimerEvent * e)
{
    _DBGFN() << inspector.name() << endl;
    inspector.timerEvent(e);
    _DBGFNL() << inspector.name() << endl;
}

qlonglong cInspector::nodeOid = NULL_ID;    // "inicializálatlan"! Az első alkalommal hívott konstruktor inicializálja: initStaric().
qlonglong cInspector::sdevOid = NULL_ID;

void cInspector::preInit()
{
    inspectorType= IT_UNKNOWN;
    internalStat = IS_INIT;
    timerStat    = TS_STOP;

    pParent  = NULL;
    pNode    = NULL;
    pPort    = NULL;
    pMagicMap= NULL;
    interval = -1;
    retryInt = -1;
    timerId  = -1;
    lastRun.invalidate();
    lastElapsedTime = -1;
    pThread  = NULL;
    pq       = NULL;
    pSubordinates = NULL;
    pProtoService = NULL;
    pPrimeService = NULL;
    pService = NULL;
    initStatic();
}

cInspector::cInspector(cInspector * __par)
    : QObject(__par), hostService(), lastRun()
{
    preInit();
    pParent = __par;
}

cInspector::cInspector(QSqlQuery& q, const QString &sn)
    : QObject(NULL), hostService(), lastRun()
{
    preInit();
    self(q, sn);
    // Ha van specifikált port is
    if (! hostService.isNull(_sPortId)) pPort = cNPort::getPortObjById(q, hostService.getId(_sPortId));
    // A prime és proto service, ha van
    pPrimeService = &hostService.getPrimeService(q);
    pProtoService = &hostService.getProtoService(q);
}

cInspector::cInspector(QSqlQuery& q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *__par)
    : QObject(__par), hostService(), lastRun()
{
    preInit();
    pParent = __par;
    // Megallokáljuk a megfelelő típusú node objektumot
    if      (__tableoid == nodeOid
          || __tableoid == NULL_ID) pNode = new cNode();
    else if (__tableoid == sdevOid) pNode = new cSnmpDevice();
    else    EXCEPTION(EDATA, __tableoid, QObject::trUtf8("Invalid node tableOID."));
    // Ha a host_service_id NULL, akkor már be van olvasva a két (host_services, nodes/hosts/..) rekord !!!
    if (__host_service_id != NULL_ID) {
        QString sql = QString(
            "SELECT hs.*, n.* FROM host_services AS hs JOIN %1 AS n USING (node_id) "
                    "WHERE hs.host_service_id = %2")
                .arg(pNode->tableName()).arg(__host_service_id);
        if (!q.exec(sql)) SQLPREPERR(q, sql);
        if (!q.first()) EXCEPTION(EDATA, __host_service_id, QObject::trUtf8("host_services record not found."));
    }
    int i = 0;
    hostService.set(q, &i, hostService.cols());
    pNode->     set(q, &i, pNode->cols());
    // Minden mezőt fel kellett dolgozni
    if (i != q.record().count()) EXCEPTION(EDATA, i);
    // Előszedjük a services-t is
    QSqlQuery q2 = getQuery();
    service(q2, hostService.getId(_sServiceId));
    // Ha van specifikált port is
    if (! hostService.isNull(_sPortId)) pPort = cNPort::getPortObjById(q2, hostService.getId(_sPortId));
    // A prime és proto service, ha van
    pPrimeService = &hostService.getPrimeService(q2);
    pProtoService = &hostService.getProtoService(q2);
}

cInspector::~cInspector()
{
    QString n = name();
    _DBGFN() << n << endl;
    down();
    _DBGFNL() << n << endl;
}

qlonglong cInspector::rnd(qlonglong i, qlonglong m)
{
    static time_t t = 0;
    if (t == 0) {
        t = time(NULL);
        srand((unsigned int)t);
    }
    double r = i;
    r *= rand();
    r /= RAND_MAX;
    if (r < m) return m;
    return (qlonglong)r;
}

void cInspector::down()
{
    stop(false);
    if (internalStat != IS_DOWN) {
        setInternalStat(IS_DOWN);
    }
    dropSubs();
    if (pThread   != NULL) {
        dropThreadDb(pThread->objectName(), false);
        delete pThread;
        pThread = NULL;
    }
    if (pq        != NULL) { delete pq;         pq = NULL; }
    if (pPort     != NULL) { delete pPort;      pPort = NULL; }
    if (pNode     != NULL) { delete pNode;      pNode = NULL; }
    if (pMagicMap != NULL) { delete pMagicMap;  pMagicMap = NULL; }
}

void cInspector::dropSubs()
{
    if (pSubordinates != NULL) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            cInspector *p = *i;
            if (p != NULL) delete p;
        }
        delete pSubordinates;
        pSubordinates = NULL;
    }
}

void cInspector::setInternalStat(enum eInternalStat is)
{
    if (pSubordinates != NULL) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            cInspector *p = *i;
            if (p != NULL) p->setInternalStat(is);
        }
    }
    internalStat = is;
}

cInspector *cInspector::newSubordinate(QSqlQuery&, qlonglong, qlonglong, cInspector *)
{
    DWAR() << "Called default dummy methode." << endl;
    return NULL;
}

QThread *cInspector::newThread()
{
    DBGFN();
    QThread *p = new cInspectorThread(this);
    p->setObjectName(name());
    return p;
}

static inline QString ssi(const QString& s1, const QString& s2, qlonglong n)
{
    QString r = s2.isEmpty() ? s1 : s2;
    if (r.contains(QString("%1"))) {
        return r.arg(n, 0, 10);
    }
    return r;
}

void cInspector::postInit(QSqlQuery& q, const QString& qs)
{
    _DBGFN() << name() << endl;
    if (pSubordinates != NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("A pSubordinates pointer nem NULL!"));
    if (pThread       != NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("A pThread pointer nem NULL!"));
    // Az objektum típusa, a IT_TIMED az alapértelmezett
    QString s = magicParam(_sInspector);
    inspectorType = s.isEmpty() ? IT_TIMED : ::inspectorType(s);
    PDEB(VERBOSE) << name() << " inspectorType = " << ::inspectorType(inspectorType) << endl;
    // Az időzítéssek, ha kellenek
    interval = variantToId(get(_sNormalCheckInterval), false, -1);
    retryInt = variantToId(get(_sRetryCheckInterval),  false, interval);
    if (interval > 0) { // Átváltás sec -> msec, ha van mit
        interval *= 1000;
        retryInt *= 1000;
    }
    else if (isTimed()) {   // Időzített időzítés nélkül !
        EXCEPTION(EDATA, interval, QObject::trUtf8("Időzített lekérdezés, időzítés nélkül."));
    }
    // Saját szál ?
    if (isThread()) {
        pThread = newThread();      //
    }
    // Van superior, de nem custom vagy bármi más, Thread-nál a thread-ban kell
    else if (findMagic(_sSuperior) && magicParam(_sSuperior).isEmpty()) {
        pSubordinates = new QList<cInspector *>;
        setSubs(q, qs);
    }
}

void cInspector::setSubs(QSqlQuery& q, const QString& qs)
{
    if (pSubordinates == NULL) EXCEPTION(EPROGFAIL);
    QSqlQuery q2 = getQuery();
    static QString sql =
            "SELECT hs.host_service_id, h.tableoid"
            " FROM host_services AS hs JOIN nodes AS h USING(node_id) "
            " WHERE hs.superior_host_service_id = %1";
    sql = ssi(sql, qs, hostServiceId());
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (q.first()) do {
        qlonglong       hsid = variantToId(q.value(0));  // host_service_id      A szervíz rekord amit be kell olvasni
        qlonglong       hoid = variantToId(q.value(1));  // node tableoid        A node típusa
        cInspector *p = newSubordinate(q2, hsid, hoid, this);
        if (p != NULL) {
            p->postInit(q2);  // ??
            *pSubordinates << p;
        }
    } while (q.next());
}

QVariant cInspector::get(const QString& __n) const
{
    QVariant r = hostService.get(__n);
    if (r.isNull()) r = service().get(__n);
    return r;
}

tMagicMap& cInspector::splitMagic(bool __ex)
{
    if (pMagicMap == NULL) pMagicMap = new tMagicMap();
    (*pMagicMap)  = service().magicMap(__ex);
    tMagicMap& mm = hostService.magicMap(__ex);
    for (tMagicMapConstIteraeor i = mm.constBegin(); i != mm.constEnd(); ++i) {
        (*pMagicMap)[i.key()] = i.value();
    }
    return *pMagicMap;
}

void cInspector::self(QSqlQuery& q, const QString& __sn)
{
    // Elöszőr mindent kiürítünk, mert jobb a békesség
    pService = NULL;
    if (pNode    != NULL) { delete pNode;     pNode = NULL; }
    if (pPort    != NULL) { delete pPort;     pPort = NULL; }
    if (pMagicMap!= NULL) { delete pMagicMap; pMagicMap = NULL; }
    hostService.clear();
    // Előkotorjuk a szolgáltatás típus rekordot.
    pService = &cService::service(q, __sn);
    // Jelenleg feltételezzük, hogy a node az egy host
    pNode = new cNode();
    pNode->fetchSelf(q);
    // És a host_services rekordot is előszedjük.
    hostService.fetchByIds(q, pNode->getId(), service().getId());
}

void cInspector::timerEvent(QTimerEvent *)
{
    if (internalStat != IS_RUN) {
        PDEB(VERBOSE) << __PRETTY_FUNCTION__ << " skip " << name() << ", internalStat = " << internalStatName() << endl;
        return;
    }
    _DBGFN() << " Run: " << hostService.getId() << _sSpace
             << host().getName()   << _sABraB << host().getId()    << _sABraE << _sColon
             << service().getName()<< _sABraB << service().getId() << _sABraE << _sCommaSp
             << "Thread: " << (isMainThread() ? "Main" : objectName()) <<  endl;
    if (!isTimed()) EXCEPTION(EPROGFAIL, (int)inspectorType, name());
    if (isThread() && isMainThread()) EXCEPTION(EPROGFAIL, (int)inspectorType, name());
    if (timerStat == TS_FIRST) toNormalInterval();  // Ha az első esetleg túl rövid, ne legyen felesleges event.
    enum eNotifSwitch retStat = RS_UNKNOWN;
    bool statIsSet    = false;
    bool statSetRetry = false;
    cError * lastError = NULL;
    if (pq == NULL) pq = newQuery();
    // Tesszük a dolgunkat bármi legyen is az, egy tranzakció lessz.
    try {
        sqlBegin(*pq);
        if (lastRun.isValid()) {
            lastElapsedTime = lastRun.restart();
        }
        else {
            lastElapsedTime = 0;
            lastRun.start();
        }
        retStat      = run(*pq);
        statIsSet    = retStat & RS_STAT_SETTED;
        statSetRetry = retStat & RS_SET_RETRY;
        retStat      = (enum eNotifSwitch)(retStat & RS_STAT_MASK);
    }
    catch (cError * e)  {   lastError = e;                  }
    catch (...)         {   lastError = NEWCERROR(EUNKNOWN);}
    // Ha hívtuk a run metódust, és dobott egy hátast
    if (lastError != NULL) {
        sqlRollback(*pq, false);  // Hiba volt, inkább visszacsináljuk az egészet.
        // A hibárol LOG az adatbázisba, amugy töröljük a hibát
        lanView  *plv = lanView::getInstance();
        qlonglong id = plv->sendError(lastError);
        delete lastError;
        if (plv->lastError == lastError) plv->lastError = NULL;
        lastError = NULL;
        QString msg = QString(QObject::trUtf8("Hiba, ld.: app_errs.applog_id = %1")).arg(id);
        sqlBegin(*pq);
        hostService.setState(*pq, _sUnknown, msg, parentId(false));
    }
    // Ha ugyan nem volt hiba, de sokat tököltünk
    else if (retStat < RS_WARNING && (lastRun.hasExpired(interval) || lastElapsedTime > ((interval*3)/2))) {
        QString msg = QString(QObject::trUtf8("Idő tullépés, futási idö %1 ezred másodperc")).arg(lastRun.elapsed());
        hostService.setState(*pq, notifSwitch(RS_WARNING), msg, parentId(false));
    }
    else {  // A lekérdezés O.K.
        if (!statIsSet) {   // Ha nem volt status állítás
            QString msg = QString(QObject::trUtf8("Futási idő %1 ezred másodperc")).arg(lastRun.elapsed());
            hostService.setState(*pq, notifSwitch(retStat), msg, parentId(false));
        }
    }
    // normal/retry intervallum kezelése
    if (interval != retryInt) { // csak akkor ha különböző
        int hard = notifSwitch(hostService.getName(_sHardState));
        int soft = notifSwitch(hostService.getName(_sSoftState));
        // Ha a soft status rosszabb mint a hard status, és nem csak warning, akkor retry interval
        if (statSetRetry || (soft > RS_WARNING && hard < soft)) {
            if (timerStat == TS_NORMAL) toRetryInterval();
        }
        else {
            if (timerStat == TS_RETRY) toNormalInterval();
        }
    }
    sqlEnd(*pq);
    DBGFNL();
}

enum eNotifSwitch cInspector::run(QSqlQuery&)
{
    DBGFN();
    return RS_ON;
}

bool cInspector::threadPrelude(QThread &)
{
    _DBGFN() << name() << endl;
    if (pq == NULL) pq = newQuery();
    if (findMagic(_sSuperior) && magicParam(_sSuperior).isEmpty()) {
        pSubordinates = new QList<cInspector *>;
        setSubs(*pq);
    }
    bool r = isTimed();
    // Időzített
    if (r) {
        qlonglong t = rnd(interval);
        PDEB(VERBOSE) << "Start timer " << interval << _sSlash << t << "ms in new thread..." << endl;
        timerStat = TS_FIRST;
        timerId = startTimer(t);
    }
    startSubs();
    internalStat = IS_RUN;
    return r;
}

void cInspector::start()
{
    _DBGFN() << _sSpace << name() << " internalStat = " << internalStatName() << endl;
    if (internalStat != IS_INIT && internalStat != IS_REINIT)
        EXCEPTION(EDATA, (int)internalStat, QObject::trUtf8("Nem megfelelő belső állapot"));
    if (timerId != -1)
        EXCEPTION(EDATA, timerId, QObject::trUtf8("óra újra inicializálása."));
    if (isThread()) {
        if (pThread == NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("A pThread egy NULL pointer."));
        if (pThread->isRunning()) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("A szál már el lett idítva."));
        if (pq != NULL) {
            delete pq;
            pq = NULL;
        }
        setParent(NULL);            // Nem lehet parentje
        moveToThread(pThread);
        if (thread() != pThread) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("A QObject::moveToThread() hívás sikertelen."));
        pThread->start();
        PDEB(VERBOSE) << name() << " thread started." << endl;
        _DBGFNL() << _sSpace << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    if (isTimed()) {
        qlonglong t = rnd(interval);
        PDEB(VERBOSE) << "Start timer " << interval << _sSlash << t << "ms in defailt thread..." << endl;
        timerId = startTimer(t);
        timerStat = TS_FIRST;
    }
    startSubs();
    internalStat = IS_RUN;
    _DBGFNL() << _sSpace << name() << " internalStat = " << internalStatName() << endl;
}

void cInspector::startSubs()
{
    _DBGFN() << name() << endl;
    if (pSubordinates != NULL) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            cInspector *p = *i;
            if (p != NULL) {
                if (p->needStart()) p->start();
                else p->setInternalStat(IS_RUN);
            }
        }
    }
}

void cInspector::stop(bool __ex)
{
    _DBGFN() << name() << endl;
    if (internalStat != IS_DOWN && internalStat != IS_REINIT) setInternalStat(IS_DOWN);
    if (isTimed()) {
        if (timerId == -1) {
            QString m = QObject::trUtf8("El sem indított óra leállítása.");
            if (__ex) EXCEPTION(EDATA, -1, m);
            DWAR() << m << endl;
        }
        else {
            killTimer(timerId);
            timerId = -1;
        }
        timerStat = TS_STOP;
    }
    if (isThread()) {
        if (pThread == NULL) {
            QString m = QObject::trUtf8("A pThread egy NULL pointer.");
            if(__ex) EXCEPTION(EPROGFAIL, -1, m);
            DWAR() << m << endl;
        }
        else if (pThread->isRunning()) {
            PDEB(VVERBOSE) << "Stop " << pThread->objectName() << " thread..." << endl;
            pThread->quit();
            pThread->wait(10000);
            if (pThread->isRunning()) {
                DWAR() << "Thread " << pThread->objectName() << " quit time out" << endl;
                pThread->terminate();
            }
            else {
                DWAR() << "Thread " << pThread->objectName() << " is not running." << endl;
            }
        }
    }
    _DBGFNL() << name() << endl;
}

void cInspector::toRetryInterval()
{
    DBGFN();
    if (timerStat == TS_RETRY) return;
    timerStat = TS_RETRY;
    if (timerId < 0) EXCEPTION(EPROGFAIL);
    killTimer(timerId);
    timerId = startTimer(retryInt);
}

void cInspector::toNormalInterval()
{
    DBGFN();
    if (timerStat == TS_NORMAL) return;
    timerStat = TS_NORMAL;
    if (timerId < 0) EXCEPTION(EPROGFAIL);
    killTimer(timerId);
    timerId = startTimer(interval);
}

QString cInspector::name()
{
    QString r;
    static const QString    qq("??");
    if (pNode != NULL && !node().isNull(node().nameIndex())) r = node().getName();
    else                                                     r = qq;
    r +=  _sColon;
    if (pPort != NULL) r += nPort().getName() + _sColon;
    if (pService != NULL) r += service().getName();
    else                  r += qq;
    return r;
}

/* ********************************************************************************** */

const QString& notifSwitch(int _ns, bool __ex)
{
    _ns = (enum eNotifSwitch)(_ns & ~RS_STAT_SETTED);
    switch (_ns) {
    case RS_ON:             return _sOn;
    case RS_RECOVERED:      return _sRecovered;
    case RS_WARNING:        return _sWarning;
    case RS_CRITICAL:       return _sCritical;
    case RS_DOWN:           return _sDown;
    case RS_UNREACHABLE:    return _sUnreachable;
    case RS_FLAPPING:       return _sFlapping;
    case RS_UNKNOWN:        return _sUnknown;
    default:                if (__ex)EXCEPTION(EDATA, (int)_ns);
    }
    return _sNul;
}

int notifSwitch(const QString& _nm, bool __ex)
{
    if (0 == _nm.compare(_sOn,         Qt::CaseInsensitive)) return RS_ON;
    if (0 == _nm.compare(_sRecovered,  Qt::CaseInsensitive)) return RS_RECOVERED;
    if (0 == _nm.compare(_sWarning,    Qt::CaseInsensitive)) return RS_WARNING;
    if (0 == _nm.compare(_sCritical,   Qt::CaseInsensitive)) return RS_CRITICAL;
    if (0 == _nm.compare(_sDown,       Qt::CaseInsensitive)) return RS_DOWN;
    if (0 == _nm.compare(_sUnreachable,Qt::CaseInsensitive)) return RS_UNREACHABLE;
    if (0 == _nm.compare(_sFlapping,   Qt::CaseInsensitive)) return RS_FLAPPING;
    if (0 == _nm.compare(_sUnknown,    Qt::CaseInsensitive)) return RS_UNKNOWN;
    if (0 ==__ex) EXCEPTION(EDATA, -1, _nm);
    return RS_INVALID;
}

const QString& internalStatName(eInternalStat is)
{
    switch (is) {
    case IS_INIT:   return _sInit;
    case IS_DOWN:   return _sDown;
    case IS_REINIT: return _sReInit;
    case IS_RUN:    return _sRun;
    default:        EXCEPTION(EPROGFAIL, (int)is, QObject::trUtf8("Ismeretlen enumerációs érték."));
    }
    return _sNul;   // Hogy ne legyen warningunk.
}

const QString& inspectorType(enum eInspectorType __t, bool __ex)
{
    switch (__t) {
    case IT_TIMED:      return _sTimed;
    case IT_THREAD:     return _sThread;
    case IT_CONTINUE:   return _sContinue;
    case IT_PASSIVE:    return _sPassive;
    default:            break;
    }
    if (__ex) EXCEPTION(EDATA, (int)__t, QObject::trUtf8("Invalid eInspectorType érték."));
    return _sNul;
}

eInspectorType inspectorType(const QString& __n, bool __ex)
{
    if (0 == __n.compare(_sTimed,    Qt::CaseInsensitive)) return IT_TIMED;
    if (0 == __n.compare(_sThread,   Qt::CaseInsensitive)) return IT_THREAD;
    if (0 == __n.compare(_sContinue, Qt::CaseInsensitive)) return IT_CONTINUE;
    if (0 == __n.compare(_sPassive,  Qt::CaseInsensitive)) return IT_PASSIVE;
    if (__ex) EXCEPTION(EDATA, -1, __n);
    return IT_UNKNOWN;
}
