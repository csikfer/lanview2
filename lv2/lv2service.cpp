
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
    inspector. pq = newQuery();
    if (inspector.threadPrelude(*this)) {   // Timed, event loop
        PDEB(VERBOSE) << "Thread " << inspector.name() << " start timer and loop..." << endl;
        exec();
    }
    else {                                  // Continue, run
        PDEB(VERBOSE) << "Thread " << inspector.name() << " started..." << endl;
        inspector.run(*inspector.pq);   // ??
    }
    pDelete(inspector.pq);
    inspector.moveToThread(qApp->thread());
    _DBGFNL() << inspector.name() << endl;
    inspector.internalStat = IS_STOPPED;
}

void cInspectorThread::timerEvent(QTimerEvent * e)
{
    _DBGFN() << inspector.name() << endl;
    inspector.timerEvent(e);
    _DBGFNL() << inspector.name() << endl;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static QString _sRestartMax = "restart_max";
#define DEF_RESTART_MAX 5

cInspectorProcess::cInspectorProcess(cInspector *pp)
    : QProcess(pp)
    , inspector(*pp)
{
    bool ok;
    QString s;

    reStartCnt = 0;

    if ((s = inspector.feature(_sRestartMax)).size()) {
        reStartMax = s.toInt(&ok);
        if (!ok) EXCEPTION(EDATA, -1, trUtf8("Az %1 értéke nem értelmezhető : %2").arg(_sRestartMax).arg(s));
    }
    else ok = false;
    if (!ok) {
        reStartMax = cSysParam::getIntSysParam(*inspector.pq, _sRestartMax, DEF_RESTART_MAX);
    }

    errCntClearTime = inspector.interval * 5;
    if (errCntClearTime <= 0) errCntClearTime = 600000; // 10min

    maxArcLog   = 4;
    maxLogSize  = 10 * 1024 * 1204; // 10MiByte
    logNull     = false;
    setProcessChannelMode(QProcess::MergedChannels);    // stderr + stdout ==> stdout

    if (inspector.isFeature(_sLognull)) {
        logNull = true;
        maxArcLog = maxLogSize = 0;
    }
    else {
        if ((s = inspector.feature(_sLogrot)).isEmpty() == false) {
            QRegExp m("(\\d+)([kMG]?)[,;]?(\\d*)");
            if (!m.isValid()) EXCEPTION(EPROGFAIL, 0, m.pattern());
            if (!m.exactMatch(s)) EXCEPTION(EDATA, -1, trUtf8("Az %1 értéke nem értelmezhető : %2").arg(_sLogrot).arg(s));
            s = m.cap(1);   // $1
            maxLogSize = s.toInt(&ok);
            if (!ok) EXCEPTION(EPROGFAIL, 0, s);
            s = m.cap(2);   // $2
            if (s.size()) switch (s[0].toLatin1()) {
            case 'k':   maxLogSize *= 1024;                 break;
            case 'M':   maxLogSize *= 1024 * 1024;          break;
            case 'G':   maxLogSize *= 1024 * 1024 * 1024;   break;
            default:    EXCEPTION(EPROGFAIL, 0, s);
            }
            s = m.cap(3);   // $3
            if (s.size()) {
                maxArcLog = s.toInt(&ok);
                if (!ok) EXCEPTION(EPROGFAIL, 0, s);
            }
        }
        actLogFile.setFileName(lanView::getInstance()->homeDir + "/log/" +  inspector.service().getName() + ".log");
        if (!actLogFile.open(QIODevice::Append | QIODevice::WriteOnly)) {
            EXCEPTION(EFOPEN, -1, actLogFile.fileName());
        }
    }
}

int cInspectorProcess::startProcess(bool conn, int startTo, int stopTo)
{
    _DBGFN() << VDEBBOOL(conn) << VDEB(startTo) << VDEB(stopTo) << endl;
    QString msg;
    if (inspector.checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
    PDEB(VVERBOSE) << "START : " << inspector.checkCmd << "; and wait ..." << endl;
    if (conn) {
        PDEB(VVERBOSE) << "connect .." << endl;
        connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
        connect(this, SIGNAL(readyRead()),                         this, SLOT(processReadyRead()));
    }
    start(inspector.checkCmd, inspector.checkCmdArgs, QIODevice::ReadOnly);
    if (!waitForStarted(startTo)) {
        msg = trUtf8("'waitForStarted()' hiba : %1").arg(ProcessError2Message(error()));
        inspector.hostService.setState(*inspector.pq, _sDown, msg);
        return -1;
    }
    switch (state()) {
    case QProcess::Running:
        PDEB(VVERBOSE) << "Runing..." << endl;
        if (conn) return 0; // RUN...
        if (stopTo) {
            if (!waitForFinished(stopTo)) {
                msg = trUtf8("'waitForFinished()' hiba: '%1'.").arg(ProcessError2Message(error()));
                DERR() << msg << endl;
                inspector.hostService.setState(*inspector.pq, _sUnknown, msg);
                return -1;
            }
            break;          // EXITED
        }
        else return 0;      // RUN...
        break;
    case QProcess::NotRunning:
        break;              // EXITED
    default:
        EXCEPTION(EPROGFAIL);
    }
    inspector.internalStat = IS_STOPPED;
    if (exitStatus() == QProcess::NormalExit) {
        PDEB(VVERBOSE) << "A '" << inspector.checkCmd << "' lefutott, kilépéso kód : " << exitCode() << endl;
        return exitCode();
    }
    msg = trUtf8("A '%1' program elszállt : %2").arg(inspector.checkCmd).arg(ProcessError2Message(error()));
    inspector.hostService.setState(*inspector.pq, _sCritical, msg);
    return -1;
}

void cInspectorProcess::processFinished(int _exitCode, QProcess::ExitStatus exitStatus)
{
    _DBGFN() << VDEB(_exitCode) << VDEB(exitStatus) << endl;
    if (inspector.internalStat != IS_RUN) return;
    if (inspector.inspectorType & (IT_PROCESS_CONTINUE | IT_PROCESS_RESPAWN)) {   // Program indítás volt időzités nélkül
        if (inspector.inspectorType & IT_PROCESS_CONTINUE || _exitCode != 0 || exitStatus ==  QProcess::CrashExit) {
            ++reStartCnt;
            QString msg;
            if (exitStatus ==  QProcess::CrashExit) msg = trUtf8("A program összeomlott.");
            else                                    msg = trUtf8("A program kilépett, exit = %1.").arg(_exitCode);
            if (reStartCnt > reStartMax) {
                inspector.hostService.setState(*inspector.pq, _sDown, msg + " Nincs újraindítás.");
                inspector.internalStat = IS_STOPPED;
                return;
            }
            else {
                inspector.hostService.setState(*inspector.pq, _sWarning, msg + "Újraindítási kíérlet.");
            }
        }
        PDEB(VERBOSE) << "ReStart : " << inspector.checkCmd << endl;
        inspector.internalStat = IS_RUN;
        startProcess();
    }
    else {  // ?! Nem szabadna itt lennünk.
        EXCEPTION(EPROGFAIL, inspector.inspectorType, inspector.name());
    }
}

void cInspectorProcess::processReadyRead()
{
    // DBGFN();
    if (logNull) {
        (void)readAllStandardOutput();
    }
    else {
        if (!actLogFile.isOpen()) {
            EXCEPTION(EPROGFAIL);
        }
        actLogFile.write(readAllStandardOutput());
        qint64 pos = actLogFile.pos();
        if (maxLogSize < pos) {
            PDEB(VVERBOSE) << "LogRot : " << maxLogSize << " < " << pos << endl;
            actLogFile.close();
            QString     old;
            QString     pre;
            for (int i = maxArcLog; i > 1; --i) {
                old = actLogFile.fileName() + QChar('.') + QString::number(i);
                (void)QFile::remove(old);
                pre = actLogFile.fileName() + QChar('.') + QString::number(i -1);
                bool r = QFile::rename(pre, old);
                PDEB(VVERBOSE) << "Rename " << pre << " to " << old << " Result : " << DBOOL(r) << endl;
            }
            old = actLogFile.fileName() + ".1";
            (void)QFile::remove(old);
            pre = actLogFile.fileName();
            if (!QFile::rename(pre, old)) {
                DERR() << "log file rename " << pre << " to " << old << " error." << endl;
            }
            else {
                PDEB(VVERBOSE) << "Rename " << pre << " to " << old << " O.K." << endl;
            }
            if (!actLogFile.open(QIODevice::Append | QIODevice::WriteOnly)) {
                EXCEPTION(EFOPEN, -1, actLogFile.fileName());
            }
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// "inicializálatlan"! Az első alkalommal hívott konstruktor inicializálja: initStatic().
qlonglong cInspector::nodeOid = NULL_ID;
qlonglong cInspector::sdevOid = NULL_ID;

void cInspector::preInit()
{
    inspectorType= IT_CUSTOM;
    internalStat = IS_INIT;
    timerStat    = TS_STOP;

    pParent  = NULL;
    pNode    = NULL;
    pPort    = NULL;
    _pFeatures= NULL;
    interval = -1;
    retryInt = -1;
    timerId  = -1;
    lastRun.invalidate();
    lastElapsedTime = -1;
    pThread  = NULL;
    pProcess = NULL;
    pQparser = NULL;
    pq       = newQuery();
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
    inspectorType = IT_TIMING_PASSIVE;
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
    getInspectorType(q);
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
            "SELECT hs.*, n.* "
                "FROM host_services AS hs JOIN %1 AS n USING (node_id) JOIN services AS s USING(service_id) "
                "WHERE hs.host_service_id = %2 "
                  "AND NOT s.disabled AND NOT hs.disabled "   // letiltott
                  "AND NOT s.deleted  AND NOT hs.deleted "    // törölt

            ).arg(pNode->tableName()).arg(__host_service_id);
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
    // features mező értelmezése
    getInspectorType(q2);
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
    pDelete(pProcess);
    if (inspectorType & IT_OWNER_QUERY_PARSER) {
        pDelete(pQparser);
        inspectorType &= ~IT_OWNER_QUERY_PARSER;
    }
    else pQparser = NULL;
    pDelete(pq);
    pDelete(pPort);
    pDelete(pNode);
    pDelete(_pFeatures);
    inspectorType = IT_CUSTOM;
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

cInspector *cInspector::newSubordinate(QSqlQuery& _q, qlonglong _hsid, qlonglong _toid, cInspector * _par)
{
    return new cInspector(_q, _hsid, _toid, _par);
}

cInspectorThread *cInspector::newThread()
{
    DBGFN();
    cInspectorThread *p = new cInspectorThread(this);
    p->setObjectName(name());
    return p;
}

cInspectorProcess *cInspector::newProcess()
{
    DBGFN();
    cInspectorProcess *p = new cInspectorProcess(this);
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
    if (pSubordinates != NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 pSubordinates pointer nem NULL!").arg(name()));
    if (pThread       != NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 pThread pointer nem NULL!").arg(name()));
    // Ha még nincs meg a típus
    if (inspectorType == IT_CUSTOM) getInspectorType(q);
    // Ha meg van adva az ellenörző parancs, akkor a QProcess objektum létrehozása
    if (!checkCmd.isEmpty()) {
        pProcess = newProcess();
    }
    // Az időzítéssek, ha kellenek
    interval = variantToId(get(_sNormalCheckInterval), false, -1);
    retryInt = variantToId(get(_sRetryCheckInterval),  false, interval);
    if (interval <=  0 && isTimed()) {   // Időzített időzítés nélkül !
        EXCEPTION(EDATA, interval, QObject::trUtf8("%1 időzített lekérdezés, időzítés nélkül.").arg(name()));
    }
    // Saját szál ?
    if (isThread()) {
        pThread = newThread();      //
    }
    // Van superior. (Thread-nél a thread-ben kell.)
    else if (inspectorType & IT_SUPERIOR) {
        pSubordinates = new QList<cInspector *>;
        setSubs(q, qs);
    }
}

void cInspector::setSubs(QSqlQuery& q, const QString& qs)
{
    if (pSubordinates == NULL) EXCEPTION(EPROGFAIL, -1, name());
    bool ok = true;
    QSqlQuery q2 = getQuery();
    QString sql =
            "SELECT hs.host_service_id, h.tableoid "
             "FROM host_services AS hs JOIN nodes AS h USING(node_id) JOIN services AS s USING(service_id) "
             "WHERE hs.superior_host_service_id = %1 "
               "AND NOT s.disabled AND NOT hs.disabled "   // letiltottak nem kellenek
               "AND NOT s.deleted  AND NOT hs.deleted "    // töröltek sem kellenek
            ;
    sql = ssi(sql, qs, hostServiceId());
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (q.first()) do {
        qlonglong       hsid = variantToId(q.value(0));  // host_service_id      A szervíz rekord amit be kell olvasni
        qlonglong       hoid = variantToId(q.value(1));  // node tableoid        A node típusa
        cInspector *p  = NULL;
        cError     *pe = NULL;
        try {
            p = newSubordinate(q2, hsid, hoid, this);
            if (p != NULL) {
                p->postInit(q2);  // ??
            }
        } CATCHS(pe);
        if (pe != NULL) {
            if (pe->mErrorCode == eError::EDATA) {  // Nem fatális hiba
                ok = false;
                cHostService hs;
                QSqlQuery q3 = getQuery();
                if (hs.fetchById(q3, hsid)) hs.setState(q3, _sCritical, pe->msg(), hostServiceId());
                pDelete(p);
                delete pe;
            }
            else {
                pDelete(p);
                pe->exception();
            }
        }
        if (p != NULL) {
            *pSubordinates << p;
        }
    } while (q.next());
    // Ha nem vettünk fel egy elemet sem, és lett volna, de hiba miatt nem vettük fel, akkor kizárást dobunk.
    if (!ok && pSubordinates->size() == 0) {
        EXCEPTION(EDATA);
    }
}

cRecordFieldConstRef cInspector::get(const QString& __n) const
{
    cRecordFieldConstRef r = hostService[__n];
    if (r.isNull()) r = service()[__n];
    if (r.isNull() && pPrimeService != NULL) r = (*pPrimeService)[__n];
    if (r.isNull() && pProtoService != NULL) r = (*pProtoService)[__n];
    return r;
}

cFeatures& cInspector::splitFeature(bool __ex)
{
    int ixFeatures = cService::_descr_cService().toIndex(_sFeatures);

    if (_pFeatures  == NULL) _pFeatures = new cFeatures();
    else                     _pFeatures->clear();

    if (pProtoService != NULL) _pFeatures->split(pProtoService->getName(ixFeatures), __ex);
    if (pPrimeService != NULL) _pFeatures->split(pPrimeService->getName(ixFeatures), __ex);
    _pFeatures->split(service().  getName(ixFeatures), __ex);
    _pFeatures->split(hostService.getName(_sFeatures), __ex);
    return *_pFeatures;
}

int cInspector::getInspectorTiming(const QString& value)
{
    if (value.isEmpty()) return 0;
    int r = 0;
    if (value.contains(_sTimed,   Qt::CaseInsensitive)) r |= IT_TIMING_TIMED;
    if (value.contains(_sThread,  Qt::CaseInsensitive)) r |= IT_TIMING_THREAD;
    if (value.contains(_sPassive, Qt::CaseInsensitive)) r |= IT_TIMING_PASSIVE;
    if (value.contains(_sPolling, Qt::CaseInsensitive)) r |= IT_TIMING_POLLING;
    switch (r) {
    case IT_TIMING_CUSTOM:
    case IT_TIMING_TIMED:
    case IT_TIMING_THREAD:
    case IT_TIMING_TIMED | IT_TIMING_THREAD:
    case IT_TIMING_PASSIVE:
    case IT_TIMING_POLLING:
        break;  // O.K.
    default:
        EXCEPTION(EDATA, r, trUtf8("Invalid feature in %1 timing = %2").arg(name()).arg(value));
    }
    return r;
}

int cInspector::getInspectorProcess(const QString &value)
{
    int r = 0;
    if (value.contains(_sRespawn,  Qt::CaseInsensitive)) r |= IT_PROCESS_RESPAWN;
    if (value.contains(_sContinue, Qt::CaseInsensitive)) r |= IT_PROCESS_CONTINUE;
    if (value.contains(_sPolling,  Qt::CaseInsensitive)) r |= IT_PROCESS_POLLING;
    if (value.contains(_sTimed,    Qt::CaseInsensitive)) r |= IT_PROCESS_TIMED;
    if (value.contains(_sCarried,  Qt::CaseInsensitive)) r |= IT_PROCESS_CARRIED;
    switch (r) {
    case IT_PROCESS_RESPAWN:
    case IT_PROCESS_CONTINUE:
    case IT_PROCESS_POLLING:
    case IT_PROCESS_TIMED:
    case IT_PROCESS_CARRIED:
    case IT_PROCESS_POLLING | IT_PROCESS_CARRIED:
    case IT_PROCESS_TIMED   | IT_PROCESS_CARRIED:
        break;  // O.K.
    case IT_NO_PROCESS:
    default:
        EXCEPTION(EDATA, r, trUtf8("Invalid feature in %1 process = %2").arg(name()).arg(value));
    }
    return r;
}

int cInspector::getInspectorMethod(const QString &value)
{
    if (value.isEmpty()) return 0;
    int r = 0;
    if (value.contains(_sNagios,   Qt::CaseInsensitive)) inspectorType |= IT_METHOD_NAGIOS;
    if (value.contains(_sMunin,    Qt::CaseInsensitive)) inspectorType |= IT_METHOD_MUNIN;
    if (value.contains(_sCarried,  Qt::CaseInsensitive)) inspectorType |= IT_METHOD_CARRIED;
    switch (r) {
    case IT_METHOD_CUSTOM:
    case IT_METHOD_NAGIOS:
    case IT_METHOD_MUNIN:
    case IT_METHOD_CARRIED:
        break;    // O.K.
    default:
        EXCEPTION(EDATA, r, trUtf8("Invalid feature in %1 method = %2").arg(name()).arg(value));
    }
    return r;
}

int cInspector::getInspectorType(QSqlQuery& q)
{
    inspectorType = 0;
    if (pParent == NULL) inspectorType |= IT_MAIN;
    int r = getCheckCmd(q);
    switch (r) {
    case  0:        // Nincs program hívás
        if (isFeature(_sSuperior)) inspectorType |= IT_SUPERIOR;
        inspectorType |= getInspectorTiming(feature(_sTiming));
        inspectorType |= getInspectorMethod(feature(_sMethod));
        break;
    case  1:        // Program hívása, a hívó applikációban
        r = getInspectorProcess(feature(_sProcess));
        inspectorType |= r;
        // Check...
        r &= ~IT_PROCESS_CARRIED;   // ellenörzés szempontjából érdektelen
        r |= getInspectorTiming(feature(_sTiming));
        r &= ~IT_TIMING_THREAD;     // ellenörzés szempontjából érdektelen
        switch (r) {
        case IT_CUSTOM:
        case IT_PROCESS_POLLING  | IT_TIMING_TIMED:
        case IT_PROCESS_TIMED    | IT_TIMING_TIMED:
            EXCEPTION(EDATA, r, trUtf8("Invalid feature in %1 process = %2, timing = %3").arg(name()).arg(feature(_sProcess)).arg(feature(_sTiming)));
        }
        r = getInspectorMethod(feature(_sMethod));
        inspectorType |= r;
        break;
    case -1:        // Program hívása, a hívótt applikációban
        if (isFeature(_sSuperior)) inspectorType |= IT_SUPERIOR;
        inspectorType |= getInspectorTiming(feature(_sTiming));
        r = getInspectorMethod(feature(_sMethod));
        inspectorType |= r;
        switch (r) {
        case IT_METHOD_CUSTOM:
        case IT_METHOD_CARRIED:
            break;
        default:
            EXCEPTION(EDATA, r, trUtf8("Invalid feature in %1 method = %2").arg(name()).arg(feature(_sMethod)));
        }
        break;
    default: EXCEPTION(EPROGFAIL, r);
    }
    return inspectorType;
}


void cInspector::self(QSqlQuery& q, const QString& __sn)
{
    // Elöszőr mindent kiürítünk, mert jobb a békesség
    pService = NULL;
    pDelete(pNode);
    pDelete(pPort);
    pDelete(_pFeatures);
    hostService.clear();
    // Előkotorjuk a szolgáltatás típus rekordot.
    pService = &cService::service(q, __sn);
    // Jelenleg feltételezzük, hogy a node az egy host
    pNode = lanView::getInstance()->selfNode().dup()->reconvert<cNode>();
    pNode->fetchSelf(q);
    // És a host_services rekordot is előszedjük.
    hostService.fetchByIds(q, pNode->getId(), service().getId());
}

int cInspector::getCheckCmd(QSqlQuery& q)
{
    QString val;
    int ixCheckCmd = cService::_descr_cService().toIndex(_sCheckCmd);

    checkCmdArgs.clear();
    if (pProtoService != NULL) checkCmd = pProtoService->getName(ixCheckCmd);
    if (pPrimeService != NULL) {
        val = pPrimeService->getName(ixCheckCmd);
        if (!val.isEmpty()) {
            if (val == "!") checkCmd.clear();
            else            checkCmd = val;
        }
    }
    val = service().getName(ixCheckCmd);
    if (!val.isEmpty()) {
        if (val == "!") checkCmd.clear();
        else            checkCmd = val;
    }
    val = hostService.getName(_sCheckCmd);
    if (!val.isEmpty()) {
        if (val == "!") checkCmd.clear();
        else            checkCmd = val;
    }
    checkCmd = checkCmd.trimmed();
    if (checkCmd.isEmpty()) return 0;

    QString arg;
    QString::const_iterator i = checkCmd.constBegin();
    char separator = 0;
    bool prevSpace = false;
    while (i != checkCmd.constEnd()) {
        char c = i->toLatin1();
        QChar qc = *i;
        ++i;
        if (separator == 0 && qc.isSpace()) {
            if (prevSpace) continue;
            prevSpace = true;
            checkCmdArgs << arg;
            arg.clear();
            continue;
        }
        prevSpace = false;
        if (separator != 0 && c == separator) {
            separator = 0;
            continue;
        }
        if (c == '"' || c == '\'') {
            separator = c;
            continue;
        }
        if (c == '$') {
            if (i->toLatin1() == '$') {
                ++i;
            }
            else {
                QString vname;
                vname = getParName(i, checkCmd.constEnd());
                arg += getParValue(q, vname);
                continue;
            }
        }
        arg += qc;
    }
    checkCmdArgs << arg;                // Utolsó paraméter
    checkCmd = checkCmdArgs.first();    // Parancs név
    checkCmdArgs.pop_front();           // Argumentumok

#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX))
    enum Qt::CaseSensitivity cs = Qt::CaseSensitive;
#else
    enum Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#endif
    QString myBase = QFileInfo(QCoreApplication::arguments().at(0)).baseName();
    QString ccBase = QFileInfo(checkCmd).baseName();
    if (0 == myBase.compare(ccBase, cs)) {
        // Önmagunk hívása nem jó ötéet
        checkCmd.clear();
        checkCmdArgs.clear();
        return -1;
    }

#ifdef Q_OS_WIN
    if (!checkCmd.contains(QChar('.'))) checkCmd += ".exe";
#endif
    QFileInfo   fcmd(checkCmd);
    if (fcmd.isExecutable()) {
        ; // OK
    }
    else {    // A saját nevén (ha full path) ill. az aktuális mappában nincs.
        QString pathEnv = qgetenv("PATH");
        QStringList path;
#ifdef Q_OS_WIN
        QChar   sep(';');
#else
        QChar   sep(':');
#endif
        path << lanView::getInstance()->binPath.split(sep) << pathEnv.split(sep);    // saját path, és a PATH -ban lévő path-ok
        foreach (QString dir, path) {
            QDir d(dir);
            fcmd.setFile(d, checkCmd);
            if (fcmd.isExecutable()) break;      // megtaláltuk
        }
        if (!fcmd.isExecutable()) EXCEPTION(ENOTFILE, -1, trUtf8("Ismeretlrn %1 parancs a %2 -ben").arg(checkCmd).arg(name()));  // nem volt a path-on sem
    }
    checkCmd = fcmd.absoluteFilePath();
    _DBGFNL() << "checkCmd = " << quotedString(checkCmd) <<
                 (checkCmdArgs.isEmpty() ? " ARGS()"
                                         : " ARGS(\"" + checkCmdArgs.join("\", \"") + "\")")
              << endl;
    return 1;
}

void cInspector::timerEvent(QTimerEvent *)
{
    if (internalStat != IS_RUN) {
        PDEB(VERBOSE) << __PRETTY_FUNCTION__ << " skip " << name() << ", internalStat = " << internalStatName() << endl;
        return;
    }
    _DBGFN() << " Run: " << hostService.getId() << QChar(' ')
             << host().getName()   << QChar('(') << host().getId()    << QChar(')') << QChar(',')
             << service().getName()<< QChar('(') << service().getId() << QChar(')') << _sCommaSp
             << "Thread: " << (isMainThread() ? "Main" : objectName()) <<  endl;
    if (inspectorType & IT_TIMING_PASSIVE) {
        if (pSubordinates == NULL) EXCEPTION(EPROGFAIL);    //?!
        int n = 0;  // Hány alárendelt fut még?
        foreach (cInspector * pSub, *pSubordinates) {
            if (pSub != NULL && pSub->internalStat == IS_RUN) ++n;
        }
        if (!n) EXCEPTION(NOTODO, 1);
        hostService.touch(*pq, _sLastTouched);
/*	// Teszt, a NOTIFY -nek így sincs semmi hatása...
        QStringList nl = getSqlDb()->driver()->subscribedToNotifications();
        if (nl.size()) {
            QString sql  = "LISTEN " + nl[0];
            QSqlQuery q = getQuery();
            if (!q.exec(sql)) SQLPREPERR(q, sql);
            PDEB(VVERBOSE) << "subscribedToNotifications : " << getSqlDb()->driver()->subscribedToNotifications().join(", ") << endl;
        }*/
        return;
    }
    if (!isTimed()) EXCEPTION(EPROGFAIL, (int)inspectorType, name());
    if (isThread() && isMainThread()) EXCEPTION(EPROGFAIL, (int)inspectorType, name());
    bool statSetRetry = toRun(true);
    if (timerStat == TS_FIRST) toNormalInterval();  // Ha az első esetleg túl rövid, ne legyen felesleges event.
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
    DBGFNL();
}

bool cInspector::toRun(bool __timed)
{
    _DBGFN() << name() << (__timed ? _sTimed : _sNul) << endl;
    enum eNotifSwitch retStat = RS_UNKNOWN;     // A lekérdezés státusza
    bool statIsSet    = false;                  // A statusz beállítva
    bool statSetRetry = false;                  // Időzítés modosítása
    cError * lastError = NULL;                  // Hiba leíró
    if (lastRun.isValid()) {
        lastElapsedTime = lastRun.restart();
    }
    else {
        lastElapsedTime = 0;
        lastRun.start();
    }
    // Tesszük a dolgunkat bármi legyen is az, egy tranzakció lessz.
    try {
        sqlBegin(*pq);
        retStat      = run(*pq);
        statIsSet    = retStat & RS_STAT_SETTED;
        statSetRetry = retStat & RS_SET_RETRY;
        retStat      = (enum eNotifSwitch)(retStat & RS_STAT_MASK);
    } CATCHS(lastError);
    if (lastError != NULL) {    // Ha hívtuk a run metódust, és dobott egy hátast
        sqlRollback(*pq);  // Hiba volt, inkább visszacsináljuk az egészet.
        if (pProcess != NULL && QProcess::NotRunning != pProcess->state()) {
            pProcess->kill();
        }
        // A hibárol LOG az adatbázisba, amugy töröljük a hibát
        lanView  *plv = lanView::getInstance();
        qlonglong id = lanView::sendError(lastError);
        if (plv->lastError == lastError) plv->lastError = NULL;
        pDelete(lastError);
        QString msg = QString(QObject::trUtf8("Hiba, ld.: app_errs.applog_id = %1")).arg(id);
        sqlBegin(*pq);
        hostService.setState(*pq, _sUnknown, msg, parentId(false));
    }
    // Ha ugyan nem volt hiba, de sokat tököltünk
    else if (retStat < RS_WARNING
             && ((interval > 0 && lastRun.hasExpired(interval))
                 || (__timed   && lastElapsedTime > ((interval*3)/2)))) {
        QString msg = QString(QObject::trUtf8("Idő tullépés, futási idö %1 ezred másodperc")).arg(lastRun.elapsed());
        hostService.setState(*pq, notifSwitch(RS_WARNING), msg, parentId(false));
    }
    else if (!statIsSet) {   // Ha nem volt status állítás
        QString msg = QString(QObject::trUtf8("Futási idő %1 ezred másodperc")).arg(lastRun.elapsed());
        hostService.setState(*pq, notifSwitch(retStat), msg, parentId(false));
    }
    sqlEnd(*pq);
    return statSetRetry;
}

enum eNotifSwitch cInspector::run(QSqlQuery& q)
{
    _DBGFN() << name() << endl;
    (void)q;
    if (pProcess != NULL) {
        if (checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
        PDEB(VERBOSE) << "Run : " << checkCmd << endl;
        int ec = pProcess->startProcess(true);
        return parse(ec, *pProcess);
    }
    return RS_UNKNOWN;
}

enum eNotifSwitch cInspector::parse(int _ec, QIODevice& text)
{
    _DBGFN() <<  name() << endl;
    switch (inspectorType & IT_METHOD_MASK) {
    case IT_METHOD_CUSTOM:
    case IT_METHOD_CARRIED: return RS_STAT_SETTED;
    case IT_METHOD_MUNIN:   return parse_munin(_ec, text);
    case IT_METHOD_NAGIOS:  return parse_nagios(_ec, text);
    case IT_METHOD_QPARSE:  return parse_qparse(_ec, text);
    default:
        EXCEPTION(ENOTSUPP, inspectorType, name());
    }
    return RS_INVALID;
}

enum eNotifSwitch cInspector::parse_munin(int _ec, QIODevice &text)
{
    (void)_ec;
    (void)text;
    EXCEPTION(ENOTSUPP);    // majd...
    return RS_INVALID;
}

enum eNotifSwitch cInspector::parse_nagios(int _ec, QIODevice &text)
{
    (void)_ec;
    (void)text;
    EXCEPTION(ENOTSUPP);    // majd...
    return RS_INVALID;
}

enum eNotifSwitch cInspector::parse_qparse(int _ec, QIODevice& text)
{
    (void)_ec;
    QString comment = feature(_sComment);
    // Ha a parser egy másik szálban fut, keresük ki indította el
    if (pQparser == NULL) {
        for (cInspector *pPar = pParent; pPar != NULL; pPar = pPar->pParent) {
            pQparser = pPar->pQparser;
            if (pQparser != NULL) break;     // Megtaláltuk
        }
        if (pQparser == NULL) {
            pQparser = new cQueryParser;
            inspectorType |= IT_OWNER_QUERY_PARSER;
            int r = pQparser->load(*pq, serviceId(), true, false);
            if (R_NOTFOUND == r && NULL != pPrimeService) r = pQparser->load(*pq, primeServiceId(), true, false);
            if (R_NOTFOUND == r && NULL != pProtoService) r = pQparser->load(*pq, protoServiceId(), true, false);
            if (R_NOTFOUND == r) EXCEPTION(EFOUND);
        }
    }
    pQparser->setInspector(this);   // A kliens beállítása...
    QString t;
    bool ok = false;
    while (false == (t = QString::fromUtf8(text.readLine())).isEmpty()) {
        t = t.simplified();
        if (t.isEmpty()) continue;      // üres
        if (!comment.isEmpty() && 0 == t.indexOf(comment)) continue;
        cError *pe = NULL;
        int r = pQparser->parse(t, pe);
        ok = ok || r == REASON_OK;      // Ha semmire sem volt találat
        if (r == R_NOTFOUND) {
            continue;  // Nincs minta a sorra, nem foglalkozunk vele
        }
        if (r != REASON_OK) {
            if (pe == NULL) {
                DERR() << _sUnKnown << VDEB(r) << endl;
                EXCEPTION(EUNKNOWN, r, name());
            }
            DERR() << pe->msg() << endl;
            pe->exception();
        }
    }
    return ok ? RS_ON : RS_INVALID;
}

bool cInspector::threadPrelude(QThread &)
{
    _DBGFN() << name() << endl;
    if (inspectorType & IT_SUPERIOR) {
        pSubordinates = new QList<cInspector *>;
        setSubs(*pq);
    }
    bool r = isTimed();
    // Időzített
    if (r) {
        qlonglong t = rnd(interval);
        PDEB(VERBOSE) << "Start timer " << interval << QChar('/') << t << "ms in new thread " << name() << endl;
        timerStat = TS_FIRST;
        timerId = startTimer(t);
    }
//  internalStat = IS_RUN;
    startSubs();
    return r;
}

void cInspector::start()
{
    _DBGFN() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
    if (internalStat != IS_INIT && internalStat != IS_REINIT)
        EXCEPTION(EDATA, (int)internalStat, QObject::trUtf8("%1 nem megfelelő belső állapot").arg(name()));
    internalStat = IS_RUN;
    if (timerId != -1)
        EXCEPTION(EDATA, timerId, QObject::trUtf8("%1 óra újra inicializálása.").arg(name()));
    if (pQparser != NULL)
        EXCEPTION(EPROGFAIL, -1, trUtf8("A %1-ben a parser objektum nem létezhet a start elött!").arg(name()));
    if (inspectorType & IT_METHOD_PARSER) {
        pQparser = new cQueryParser();
        inspectorType |= IT_OWNER_QUERY_PARSER;
        int r = pQparser->load(*pq, serviceId(), true);
        if (R_NOTFOUND == r && NULL != pPrimeService) r = pQparser->load(*pq, primeServiceId(), true);
        if (R_NOTFOUND == r && NULL != pProtoService) r = pQparser->load(*pq, protoServiceId(), true);
        if (R_NOTFOUND == r) EXCEPTION(EFOUND);
        cError *pe = NULL;
        pQparser->setInspector(this);
        pQparser->prep(pe);
        if (NULL != pe) pe->exception();
    }
    // ......................
    if (isThread()) {   // Saját szál?
        if (pThread == NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 pThread egy NULL pointer.").arg(name()));
        if (pThread->isRunning()) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 szál már el lett idítva.").arg(name()));
        if (pq != NULL) {
            delete pq;
            pq = NULL;
        }
        setParent(NULL);            // Nem lehet parentje, mert a moveToThread(); vinné azt is!
        moveToThread(pThread);
        if (thread() != pThread) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 QObject::moveToThread() hívás sikertelen.").arg(name()));
        pDelete(pq);
        pThread->start();
        PDEB(VERBOSE) << name() << " thread started." << endl;
        _DBGFNL() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    if (inspectorType & (IT_PROCESS_CONTINUE | IT_PROCESS_RESPAWN)) {   // Program indítás időzités nélkül
        if (checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
        PDEB(VERBOSE) << "Start : " << checkCmd << endl;
        pProcess->startProcess(true);
        return;
    }
    if (isTimed()) {
        qlonglong t = rnd(interval);
        PDEB(VERBOSE) << "Start timer " << interval << QChar('/') << t << "ms in defailt thread " << name() << endl;
        timerId = startTimer(t);
        timerStat = TS_FIRST;
        startSubs();
        internalStat = IS_RUN;
        return;
    }
    if (inspectorType & IT_SUPERIOR && timing() == IT_TIMING_PASSIVE) {
        if (interval > 0) {
            timerId   = startTimer(interval);
            timerStat = TS_NORMAL;
        }
        startSubs();
        return;
    }
    if (needStart()) {
        (void)toRun(false);
        startSubs();
        if (inspectorType & IT_TIMING_POLLING) stop();
        return;
    }
    _DBGFNL() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
}

void cInspector::startSubs()
{
    _DBGFN() << name() << endl;
    if (pSubordinates != NULL) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            cInspector *p = *i;
            if (p != NULL) {
                if (p->needStart()) {
                    p->start();
                }
                else {
                    p->setInternalStat(IS_RUN);
                }
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
            QString m = QObject::trUtf8("El sem indított %1 óra leállítása.").arg(name());
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
            QString m = QObject::trUtf8("%1 pThread egy NULL pointer.").arg(name());
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
    if (pQparser != NULL) {
        cError *pe = NULL;
        pQparser->post(pe);
        if (pe != NULL) DERR() << pe->msg() << endl;
        pDelete(pQparser);
    }
    _DBGFNL() << name() << endl;
}

void cInspector::toRetryInterval()
{
    DBGFN();
    if (timerStat == TS_RETRY) return;
    timerStat = TS_RETRY;
    if (timerId < 0) EXCEPTION(EPROGFAIL, -1, name());
    killTimer(timerId);
    timerId = startTimer(retryInt);
}

void cInspector::toNormalInterval()
{
    DBGFN();
    if (timerStat == TS_NORMAL) return;
    timerStat = TS_NORMAL;
    if (timerId < 0) EXCEPTION(EPROGFAIL, -1, name());
    killTimer(timerId);
    timerId = startTimer(interval);
}

QString cInspector::name() const
{
    QString r = "(";
    r += typeid(*this).name();
    r += "):";
    static const QString    qq("??");
    if (pNode != NULL && !node().isNull(node().nameIndex())) r += node().getName();
    else                                                     r += qq;
    r +=  QChar(':');
    if (pPort != NULL) r += nPort().getName() + QChar(':');
    if (pService != NULL) r += service().getName();
    else                  r += qq;
    if (pPrimeService != NULL || pProtoService != NULL) {
        r += "(";
        if (pPrimeService != NULL) r += pPrimeService->getName();
        r += ":";
        if (pProtoService != NULL) r += pProtoService->getName();
        r += ")";
    }
    r += QString("[%1]").arg(hostServiceId());
    return r;
}

QString cInspector::getParValue(QSqlQuery& q, const QString& name, bool *pOk) const
{
    if (pOk != NULL) *pOk = true;
    QStringList sl = name.split(QChar('.'));
    if (sl.size() > 1) {
        if (sl.size() > 2) {
            if (pOk == NULL) EXCEPTION(EDATA, sl.size(), name);
            *pOk = false;
            return QString();
        }
        QString on = sl[0];
        QString fn = sl[1];
        if (0 == on.compare("host_service",   Qt::CaseInsensitive)) return hostService.getName(fn);
        if (0 == on.compare("hostservice",    Qt::CaseInsensitive)) return hostService.getName(fn);
        if (0 == on.compare("service",        Qt::CaseInsensitive)) return service().getName(fn);
        if (0 == on.compare(_sNode,           Qt::CaseInsensitive)) return node().getName(fn);
        if (0 == on.compare(_sHost,           Qt::CaseInsensitive)) return node().getName(fn);
        if (0 == on.compare(_sInterface,      Qt::CaseInsensitive)) return nPort().getName(fn);
        if (pOk == NULL) EXCEPTION(EDATA, 2, name);
        *pOk = false;
        return QString();
    }
    else {
        if (0 == name.compare("host_service", Qt::CaseInsensitive)) return hostService.getName();
        if (0 == name.compare("hostservice",  Qt::CaseInsensitive)) return hostService.getName();
        if (0 == name.compare("service",      Qt::CaseInsensitive)) return service().getName();
        if (0 == name.compare(_sNode,         Qt::CaseInsensitive)) return node().getName();
        if (0 == name.compare(_sHost,         Qt::CaseInsensitive)) return node().getName();
        if (0 == name.compare(_sInterface,    Qt::CaseInsensitive)) return nPort().getName();
        if (0 == name.compare("selfName",     Qt::CaseInsensitive)) {
            if (lanView::testSelfName.isEmpty()) {
                return lanView::selfNode().getName();
            }
            return lanView::testSelfName;
        }
        if (0 == name.compare("S",            Qt::CaseSensitive)) {
            if (lanView::testSelfName.isEmpty()) return _sNul;
            return "-S " + lanView::testSelfName;
        }
        if (0 == name.compare(_sAddress,      Qt::CaseInsensitive)) return host().getIpAddress().toString();
        if (0 == name.compare(_sProtocol,     Qt::CaseInsensitive)) return protoService().getName();
        if (0 == name.compare("ipproto",      Qt::CaseInsensitive)) {
            qlonglong pid = service().getId(_sProtocolId);
            if (pid == NULL_ID) pid = protoService().getId(_sProtocolId);
            if (pid == NULL_ID) {
                if (pOk == NULL) EXCEPTION(EDATA, 0, name);
                *pOk = false;
                return QString();
            }
            return cService().getNameById(q, pid);
        }
        if (0 == name.compare(_sPort,         Qt::CaseInsensitive)) {
            qlonglong pn = service().getId(_sPort);
            if (pn == NULL_ID) pn = protoService().getId(_sPort);
            if (pn == NULL_ID) {
                if (pOk == NULL) EXCEPTION(EDATA, 0, name);
                *pOk = false;
                return QString();
            }
            return QString::number(pn);
        }
        if (0 == name.compare("host_service_id",Qt::CaseInsensitive))return QString::number(hostServiceId());
        if (0 == name.compare("host_id",        Qt::CaseInsensitive))return QString::number(nodeId());
        if (0 == name.compare("service_id",     Qt::CaseInsensitive))return QString::number(serviceId());

        if (pOk == NULL) EXCEPTION(EDATA, 1, name);
        *pOk = false;
        return QString();
    }
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

