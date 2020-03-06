#include <time.h>
#include "lanview.h"
#include "lv2service.h"
#include "scan.h"

cThreadAcceptor::cThreadAcceptor(cInspectorThread *pThread)
    : QObject(nullptr), inspector(pThread->inspector)
{
    moveToThread(pThread);
    pTimer = nullptr;
}

cThreadAcceptor::~cThreadAcceptor()
{
    ;
}

void cThreadAcceptor::timer(int ms, eTimerStat tst)
{
    if (pTimer != nullptr) {
        if (tst == TS_FIRST) EXCEPTION(EPROGFAIL);
        if (!pTimer->isActive()) EXCEPTION(EPROGFAIL, 0, tr("Inactive timer"));
        pTimer->stop();
    }
    if (tst == TS_STOP) {
        pDelete(pTimer);
        return;
    }
    if (!checkThread(this)) {
        EXCEPTION(EDATA, 0, tr("Inv. thread : %1 != %2").arg(thread()->objectName(), QThread::currentThread()->objectName()));
    }
    if (pTimer == nullptr) {
        pTimer = new QTimer(this);
        connect(pTimer, SIGNAL(timeout()), this, SLOT(on_timer_timeout()));
    }
    inspector.timerId = 0;
    pTimer->start(ms);
    if (!pTimer->isActive()) {
        EXCEPTION(EPROGFAIL, 0, tr("Timer not started."));
    }
}

void cThreadAcceptor::on_timer_timeout()
{
    QTimerEvent e(0);
    _DBGFN() << inspector.name() << endl;
    inspector.timerEvent(&e);
    _DBGFNL() << inspector.name() << endl;
}

cInspectorThread::cInspectorThread(cInspector *pp)
    : QThread(pp), inspector(*pp), acceptor(this)
{
    _DBGFN() << inspector.name() << endl;
    pLastError = nullptr;
    setObjectName(inspector.name());
    DBGFNL();
}

cInspectorThread::~cInspectorThread()
{
    if (isRunning()) {
        QString msg = tr("%1 thread is run. Wait 30 sec.").arg(inspector.name());
        QAPPMEMO(msg, RS_WARNING);
        terminate();
        wait(30000);
        if (isRunning()) {
            QString msg = tr("%1 thread is run. Exit app.").arg(inspector.name());
            QAPPMEMO(msg, RS_CRITICAL);
            exit(1);
        }
    }
}

void cInspectorThread::run()
{
    pDelete(pLastError);
    try {
        _DBGFN() << inspector.name() << inspector.internalStat << endl;
        switch (inspector.internalStat) {
        case IS_INIT:   doInit();   break;
        case IS_RUN:    doRun();    break;
        case IS_DOWN:   doDown();   break;
        default:
            EXCEPTION(EPROGFAIL, inspector.internalStat, internalStatName(inspector.internalStat));
        }
    } CATCHS(pLastError)
    if (pLastError) {
        inspector.internalStat = IS_ERROR;
    }
    DBGFNL();
}

void cInspectorThread::doInit()
{
    _DBGFN() << inspector.name() << endl;
    pDelete(inspector.pq);
    inspector.pq = newQuery();
    inspector.threadPreInit();
    if (inspector.inspectorType & IT_SUPERIOR) {
        if (inspector.pSubordinates != nullptr) EXCEPTION(EPROGFAIL);
        inspector.pSubordinates = new QList<cInspector *>;
        inspector.setSubs(*inspector.pq);
    }
}

void cInspectorThread::doRun()
{
    _DBGFN() << inspector.name() << endl;
    if (inspector.isTimed()) {
        int t = inspector.firstDelay();
        PDEB(VERBOSE) << "Start timer " << inspector.interval << QChar('/') << t << "ms in new thread " << inspector.name() << endl;
        timer(t, TS_FIRST);
        inspector.timerStat = TS_FIRST;
    }
    else if (inspector.passive()) {
        if (inspector.pSubordinates == nullptr || inspector.pSubordinates->isEmpty()) EXCEPTION(EDATA);
        // start subs, and event loop
    }
    else {
        inspector.doRun(false);
        inspector.startSubs();
        inspector.internalStat = IS_STOPPED;
        return;
    }
    inspector.internalStat = IS_SUSPENDED;
    inspector.startSubs();
    PDEB(INFO) << QObject::tr("Start %1 event loop ...").arg(inspector.name()) << endl;
    exec();
    PDEB(INFO) << QObject::tr("Stopp %1 event loop ...").arg(inspector.name()) << endl;
    inspector.internalStat = IS_STOPPED;
}

void cInspectorThread::doDown()
{
    _DBGFN() << inspector.name() << endl;
    timer(0, TS_STOP);
    inspector.dropSubs();
    pDelete(inspector.pq);
}

void cInspectorThread::timer(int ms, eTimerStat tst)
{
    acceptor.timer(ms, tst);
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

    _DBGFN() << inspector.name() << endl;

    if ((s = inspector.feature(_sRestartMax)).size()) {
        reStartMax = s.toInt(&ok);
        if (!ok) EXCEPTION(EDATA, -1, tr("Az %1 értéke nem értelmezhető : %2").arg(_sRestartMax).arg(s));
    }
    else ok = false;
    if (!ok) {
        reStartMax = int(cSysParam::getIntegerSysParam(*inspector.pq, _sRestartMax, DEF_RESTART_MAX));
    }

    errCntClearTime = inspector.interval * 5;
    if (errCntClearTime <= 0) errCntClearTime = 600000; // 10min

    maxArcLog   = 4;
    maxLogSize  = 10 * 1024 * 1204; // 10MiByte
    logNull     = false;
    setProcessChannelMode(QProcess::MergedChannels);    // stderr + stdout ==> stdout

    bool nolog = inspector.method() != IT_METHOD_INSPECTOR  || inspector.isFeature(_sLognull);
    if (nolog) {
        logNull = true;
        maxArcLog = maxLogSize = 0;
    }
    else {
        if ((s = inspector.feature(_sLogrot)).isEmpty() == false) {
            QRegExp m("(\\d+)([kMG]?)[,;]?(\\d*)");
            if (!m.isValid()) EXCEPTION(EPROGFAIL, 0, m.pattern());
            if (!m.exactMatch(s)) EXCEPTION(EDATA, -1, tr("Az %1 értéke nem értelmezhető : %2").arg(_sLogrot).arg(s));
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
        QString fileName = lanView::getInstance()->homeDir + "/log/";
        fileName += inspector.service()->getName();
        if (inspector.pPrimeService != nullptr) fileName += '.' + inspector.pPrimeService->getName();
        if (inspector.pProtoService != nullptr) fileName += '.' + inspector.pProtoService->getName();
        fileName += ".log";
        actLogFile.setFileName(fileName);
        if (!actLogFile.open(QIODevice::Append | QIODevice::WriteOnly)) {
            EXCEPTION(EFOPEN, -1, actLogFile.fileName());
        }
    }
}

int cInspectorProcess::startProcess(int startTo, int stopTo)
{
    _DBGFN() << VDEB(startTo) << VDEB(stopTo) << endl;
    QString msg;
    if (inspector.checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
    PDEB(VVERBOSE) << "START : " << inspector.checkCmd << " " << inspector.checkCmdArgs.join(" ") << "; and wait ..." << endl;
    if (stopTo == 0) {
        PDEB(VVERBOSE) << "Set connects for path through log. ..." << endl;
        connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
        connect(this, SIGNAL(readyRead()),                         this, SLOT(processReadyRead()));
    }
    start(inspector.checkCmd, inspector.checkCmdArgs, QIODevice::ReadOnly);
    if (!waitForStarted(startTo)) {
        msg = tr("'waitForStarted(%1)' hiba : %2").arg(startTo).arg(ProcessError2Message(error()));
        inspector.hostService.setState(*inspector.pq, _sDown, msg, inspector.lastRun.elapsed());
        inspector.internalStat = IS_STOPPED;
        return -1;
    }
    switch (state()) {
    case QProcess::Running:
        if (stopTo == 0) {
            PDEB(VVERBOSE) << "Runing and continue ..." << endl;
            return 0; // RUN...
        }
        PDEB(VVERBOSE) << "Runing and wait for finished ..." << endl;
        if (!waitForFinished(stopTo)) {
            msg = tr("Started `%1` 'waitForFinished(%2)' error: '%3'.")
                    .arg(inspector.checkCmd + " " + inspector.checkCmdArgs.join(" "))
                    .arg(stopTo).arg(ProcessError2Message(error()));
            DERR() << msg << endl;
            inspector.hostService.setState(*inspector.pq, _sUnreachable, msg, inspector.lastRun.elapsed());
            inspector.internalStat = IS_STOPPED;
            return -1;
        }
        break;              // EXITED
    case QProcess::NotRunning:
        break;              // EXITED
    default:
        EXCEPTION(EPROGFAIL);
    }
    if (exitStatus() == QProcess::NormalExit) { // OK
        inspector.internalStat = inspector.inspectorType & IT_PROCESS_TIMED ? IS_SUSPENDED : IS_STOPPED;
        PDEB(VVERBOSE) << tr("A '%1' lefutott, kilépéso kód : %2").arg(inspector.checkCmd).arg(exitCode()) << endl;
        return exitCode();
    }
    // Error
    inspector.internalStat = IS_STOPPED;
    msg = tr("A '%1' program elszállt : %2").arg(inspector.checkCmd).arg(ProcessError2Message(error()));
    PDEB(VVERBOSE) << msg << endl;
    inspector.hostService.setState(*inspector.pq, _sCritical, msg, inspector.lastRun.elapsed());
    return -1;
}

void cInspectorProcess::processFinished(int _exitCode, QProcess::ExitStatus exitStatus)
{
    _DBGFN() << VDEB(_exitCode) << VDEB(exitStatus) << ", program : " << inspector.checkCmd << endl;
    if (inspector.internalStat != IS_RUN && inspector.internalStat != IS_STOPPED) {
        DERR() << tr("Invalid event, internalStat = %1").arg(internalStatName(inspector.internalStat)) << endl;
        return;
    }
    if (inspector.inspectorType & (IT_PROCESS_CONTINUE | IT_PROCESS_RESPAWN)) {   // Program indítás volt időzités nélkül
        if (inspector.inspectorType & IT_PROCESS_CONTINUE || _exitCode != 0 || exitStatus ==  QProcess::CrashExit) {
            ++reStartCnt;
            QString msg;
            if (exitStatus ==  QProcess::CrashExit) {
                msg = tr("A %1 program összeomlott.").arg(inspector.checkCmd);
            }
            else {
                msg = tr("A %1 program kilépett, exit = %2.").arg(inspector.checkCmd).arg(_exitCode);
            }
            if (reStartCnt > reStartMax) {
                inspector.hostService.setState(*inspector.pq, _sDown, msg + " Nincs újraindítás.", inspector.lastRun.elapsed());
                inspector.internalStat = IS_STOPPED;
                return;
            }
            else {
                inspector.hostService.setState(*inspector.pq, _sWarning, msg + "Újraindítási kíérlet.", inspector.lastRun.elapsed());
            }
        }
        PDEB(VERBOSE) << "ReStart : " << inspector.checkCmd << endl;
        inspector.internalStat = IS_RUN;
        startProcess(int(inspector.startTimeOut));
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
        writeRollLog(actLogFile, readAllStandardOutput(), maxLogSize, maxArcLog);
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/// "inicializálatlan"! Az első alkalommal hívott konstruktor inicializálja: cInspector::initStatic().
/// A nodes táblát azonosító OID
qlonglong cInspector::nodeOId = NULL_ID;
/// "inicializálatlan"! Az első alkalommal hívott konstruktor inicializálja: cInspector::initStatic().
/// A snmpdevices táblát azonosító OID
qlonglong cInspector::sdevOId = NULL_ID;
/// "inicializálatlan"! Az első alkalommal hívott konstruktor inicializálja: cInspector::initStatic().
/// A syscron szolgáltatást azonosító ID
qlonglong cInspector::syscronId;

void cInspector::preInit(cInspector *__par)
{
//  DBGFN();
    inspectorType= IT_CUSTOM;
    internalStat = IS_INIT;
    timerStat    = TS_STOP;

    pNode    = nullptr;
    pPort    = nullptr;
    _pFeatures= nullptr;
    interval = -1;
    retryInt = -1;
    startTimeOut =  2000;   // Default  2s
    stopTimeOut  = 10000;   // Default 10s
    timerId  = -1;
    lastRun.invalidate();
    lastElapsedTime = -1;
    pInspectorThread  = nullptr;
    pProcess = nullptr;
    pQparser = nullptr;
    pq       = newQuery();
    pSubordinates = nullptr;
    pProtoService = nullptr;
    pPrimeService = nullptr;
    pService = nullptr;
    pVars    = nullptr;
    pRunTimeVar = nullptr;
    initStatic();

    pParent = __par;
    if (pParent != nullptr) {
        setParent(pParent->useParent());
    }
//  DBGFNL();
}

void cInspector::initStatic()
{
    if (nodeOId == NULL_ID) {   // Ha még nincsenek inicializálva a static-ok
        nodeOId   = cNode().tableoid();
        sdevOId   = cSnmpDevice().tableoid();
        syscronId = cService::name2id(*lanView::getInstance()->pQuery, _sSyscron);
    }
}


cInspector::cInspector(cInspector * __par)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << " parent : " << (__par == nullptr ? "NULL" : __par->name()) << endl;
    preInit(__par);
    if (__par != nullptr || lanView::getInstance()->pSelfHostService == nullptr) {
        inspectorType = IT_TIMING_PASSIVE;
    }
    else {
        QSqlQuery q = getQuery();
        hostService.set(*(lanView::getInstance()->pSelfHostService));
        pNode    = cPatch::getNodeObjById(q, hostService.getId(_sNodeId))->reconvert<cNode>();
        qlonglong pid = hostService.getId(_sPortId);
        if (pid != NULL_ID) {
            pPort = cNPort::getPortObjById(q, pid);
        }
        pService = lanView::getInstance()->pSelfService;
        pPrimeService = hostService.getPrimeService(q);
        pProtoService = hostService.getProtoService(q);
        getInspectorType(q);
    }
}

cInspector::cInspector(cInspector * __par, cNode *pN, const cService *pS, cNPort *pP)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << " (" << (__par == nullptr ? "NULL" : __par->name()) << ", "
             << pN->getName() << ", " << pS->getName() << ", " << (pP == nullptr ? "NULL" : pP->getName())
             << ")" << endl;
    preInit(__par);
    inspectorType = IT_TIMING_PASSIVE;
    pNode    = pN;
    pService = pS;
    pPort    = pP;
    hostService.setId(_sNodeId,    pNode->getId());
    hostService.setId(_sServiceId, pService->getId());
    if (pPort != nullptr) hostService.setId(_sPortId, pPort->getId());
}

cInspector::cInspector(QSqlQuery& q, const QString &sn)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << VDEB(sn) << endl;
    preInit(nullptr);
    self(q, sn);
    // Ha van specifikált port is
    if (! hostService.isNull(_sPortId)) pPort = cNPort::getPortObjById(q, hostService.getId(_sPortId));
    // A prime és proto service, ha van
    pPrimeService = hostService.getPrimeService(q);
    pProtoService = hostService.getProtoService(q);
    getInspectorType(q);
}

cInspector::cInspector(QSqlQuery& q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *__par)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << VDEB(__host_service_id) << VDEB(__tableoid) << (__par == nullptr ? "NULL" : __par->name()) << endl;
    preInit(__par);
    // Megallokáljuk a megfelelő típusú node objektumot
    if      (__tableoid == nodeOId
          || __tableoid == NULL_ID) pNode = new cNode();
    else if (__tableoid == sdevOId) pNode = new cSnmpDevice();
    else    EXCEPTION(EDATA, __tableoid, QObject::tr("Invalid node tableOID = %1. ID = %2").arg(__tableoid).arg(__host_service_id));
    // Ha a host_service_id NULL, akkor már be van olvasva a két (host_services és nodes vagy snmpdevices) rekord !!!
    if (__host_service_id != NULL_ID) {
        QString sql = QString(
            "SELECT hs.*, n.* "
                "FROM host_services AS hs JOIN %1 AS n USING (node_id) JOIN services AS s USING(service_id) "
                "WHERE hs.host_service_id = %2 "
                  "AND NOT s.disabled AND NOT hs.disabled "   // letiltott
                  "AND NOT s.deleted  AND NOT hs.deleted "    // törölt

            ).arg(pNode->tableName()).arg(__host_service_id);
        EXECSQL(q, sql);
        if (!q.first()) EXCEPTION(EDATA, __host_service_id, QObject::tr("host_services record not found."));
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
    pPrimeService = hostService.getPrimeService(q2);
    pProtoService = hostService.getProtoService(q2);
    // features mező értelmezése
    getInspectorType(q2);
    _DBGFNL() << name() << endl;
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
        t = time(nullptr);
        srand(uint(t));
    }
    double r = i;
    r *= rand();
    r /= RAND_MAX;
    if (r < m) return m;
    return qlonglong(r);
}

void cInspector::down()
{
    _DBGFN() << name() << endl;
    drop(EX_IGNORE);
    if (pInspectorThread != nullptr) {
        pInspectorThread->start();  // Indítás az IS_DOWN állapottal timert leállítja, QSqlQuerry objektumo(ka)t törli
        if (!pInspectorThread->wait(stopTimeOut)) pInspectorThread->terminate();
        dropThreadDb(pInspectorThread->objectName(), EX_IGNORE);
        pDelete( pInspectorThread);
    }
    pDelete(pProcess);
    if (inspectorType & IT_METHOD_PARSER) {
        PDEB(VVERBOSE) << tr("%1: Free QParser : %2").arg(name()).arg(qlonglong(pQparser), 0, 16) << endl;
        pDelete(pQparser);
    }
    else pQparser = nullptr;
    pDelete(pq);
    pDelete(pPort);
    pDelete(pNode);
    pDelete(_pFeatures);
    pDelete(pVars);
    inspectorType = IT_CUSTOM;
}

void cInspector::dropSubs()
{
    if (pSubordinates != nullptr) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            cInspector *p = *i;
            if (p != nullptr) delete p;
        }
        delete pSubordinates;
        pSubordinates = nullptr;
    }
}

cInspector *cInspector::newSubordinate(QSqlQuery& _q, qlonglong _hsid, qlonglong _toid, cInspector * _par)
{
    return new cInspector(_q, _hsid, _toid, _par);
}

cInspectorThread *cInspector::newThread()
{
    _DBGFN() << name() << endl;
    cInspectorThread *p = nullptr;
    p = new cInspectorThread(this);
    p->setObjectName(name());
    return p;
}

cInspectorProcess *cInspector::newProcess()
{
    _DBGFN() << name() << endl;
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
    if (pSubordinates    != nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("%1 pSubordinates pointer nem NULL!").arg(name()));
    if (pInspectorThread != nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("%1 pThread pointer nem NULL!").arg(name()));
    if (pVars            != nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("%1 pVars pointer nem NULL!").arg(name()));
    qlonglong tpid = qlonglong(get(_sTimePeriodId));
    switch (tpid) {
    case NULL_ID:               // Not Specified
    case ALWAYS_TIMEPERIOD_ID:  // The "always", no exclusion, not read.
        break;
    case NEVER_TIMEPERIOD_ID:   // Never run
        EXCEPTION(NOTODO);
//      break;
    default:
        timeperiod.setById(q);
        break;
    }
    pVars = fetchVars(q);    // Változók, ha vannak
    // Ha még nincs meg a típus
    if (inspectorType == IT_CUSTOM) getInspectorType(q);
    // Ha meg van adva az ellenörző parancs, akkor a QProcess objektum létrehozása
    if (!checkCmd.isEmpty()) {
        pProcess = newProcess();
    }
    // Az időzítéssek, ha kellenek
    interval = int(variantToId(get(_sNormalCheckInterval), EX_IGNORE, -1));
    retryInt = int(variantToId(get(_sRetryCheckInterval),  EX_IGNORE, interval));
    if (interval <=  0 && isTimed()) {   // Időzített időzítés nélkül !
        EXCEPTION(EDATA, interval, QObject::tr("%1 időzített lekérdezés, időzítés nélkül.").arg(name()));
    }
    // Saját szál ?
    if (isThread()) {
        pInspectorThread = newThread();     //
        pInspectorThread->start();          // Init, Init után leáll
        if (!pInspectorThread->wait(startTimeOut)) {   // Az init még szinkron, megvárjuk a végét

            EXCEPTION(ETO, 0, tr("%1 thread init.").arg(name()));
        }
        if (pInspectorThread->pLastError != nullptr) {
            cError *pe = pInspectorThread->pLastError;
            pInspectorThread->pLastError = nullptr;
            pe->exception();
        }
    }
    // Van superior. (Thread-nél ezt a thread-ben kell)
    else if (inspectorType & IT_SUPERIOR) {
        // process-nél a mi dolgunk ?
        bool f = pProcess == nullptr;
        f = f || !(inspectorType & (IT_METHOD_INSPECTOR));
        if (f) {
            pSubordinates = new QList<cInspector *>;
            setSubs(q, qs);
        }
    }
}

void cInspector::threadPreInit()
{
    return;
}

void cInspector::dbNotif(const QString& cmd)
{
    if (0 == cmd.compare(_sTick, Qt::CaseInsensitive)) {
        timerEvent(nullptr);
        return;
    }
    // Still missing: reset, pause ...
}

void cInspector::setSubs(QSqlQuery& q, const QString& qs)
{
    _DBGFN() << name() << VDEB(qs) << endl;
    if (pSubordinates == nullptr) EXCEPTION(EPROGFAIL, -1, name());
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
    EXECSQL(q, sql);
    if (q.first()) do {
        qlonglong       hsid = variantToId(q.value(0));  // host_service_id      A szervíz rekord amit be kell olvasni
        qlonglong       hoid = variantToId(q.value(1));  // node tableoid        A node típusa
        cInspector *p  = nullptr;
        cError     *pe = nullptr;
        try {
            p = newSubordinate(q2, hsid, hoid, this);
            if (p != nullptr) {
                p->postInit(q2);  // ??
            }
        } CATCHS(pe);
        if (pe != nullptr) {
            ok = false;
            if (pe->mErrorCode != eError::EOK) {
                cHostService hs;
                QSqlQuery q3 = getQuery();
                if (hs.fetchById(q3, hsid)) hs.setState(q3, _sCritical, pe->msg(), 0, hostServiceId(), false);
            }
            pDelete(p);
            delete pe;
        }
        if (p != nullptr) {
            *pSubordinates << p;
        }
    } while (q.next());
    // Ha nem vettünk fel egy elemet sem, és lett volna, de hiba miatt nem vettük fel, akkor kizárást dobunk.
    if (!ok && pSubordinates->size() == 0) {
        EXCEPTION(EDATA, 0, tr("Due to the initialization errors of the subservice, all subservices have been dropped."));
    }
}

tOwnRecords<cServiceVar, cHostService> *cInspector::fetchVars(QSqlQuery& q)
{
    tOwnRecords<cServiceVar, cHostService> *p = new tOwnRecords<cServiceVar, cHostService>(&hostService);
    int n = p->fetch(q);
    if (0 < n) {
        for (int i = 0; i < n; ++i) {
            p->at(i)->varType(q);
        }
        return p;
    }
    delete p;
    return nullptr;
}


cRecordFieldConstRef cInspector::get(const QString& __n) const
{
    cRecordFieldConstRef r = hostService[__n];
    if (r.isNull())                             r = (*service())    [__n];
    if (r.isNull() && pPrimeService != nullptr) r = (*pPrimeService)[__n];
    if (r.isNull() && pProtoService != nullptr) r = (*pProtoService)[__n];
    return r;
}

cFeatures& cInspector::splitFeature(eEx __ex)
{
    int ixFeatures = cService::_descr_cService().toIndex(_sFeatures);

    if (_pFeatures  == nullptr) _pFeatures = new cFeatures();
    else                        _pFeatures->clear();

    if (pProtoService != nullptr) _pFeatures->split(pProtoService->getName(ixFeatures), true, __ex);
    if (pPrimeService != nullptr) _pFeatures->split(pPrimeService->getName(ixFeatures), true, __ex);
    _pFeatures->split(service()-> getName(ixFeatures), true, __ex);
    _pFeatures->split(hostService.getName(_sFeatures), true, __ex);
    PDEB(VVERBOSE) << name() << " features : " << _pFeatures->join() << endl;
    return *_pFeatures;
}

int cInspector::getInspectorTiming(const QString& value)
{
    if (value.isEmpty()) return 0;
    int r = 0; // IT_TIMING_CUSTOM
    QStringList vl = value.split(QRegExp("\\s*,\\s*"));
    if (vl.contains(_sTimed,   Qt::CaseInsensitive)) r |= IT_TIMING_TIMED;
    if (vl.contains(_sThread,  Qt::CaseInsensitive)) r |= IT_TIMING_THREAD;
    if (vl.contains(_sPassive, Qt::CaseInsensitive)) r |= IT_TIMING_PASSIVE;
    if (vl.contains(_sPolling, Qt::CaseInsensitive)) r |= IT_TIMING_POLLING;
    switch (r) {
    case IT_TIMING_CUSTOM:
    case IT_TIMING_TIMED:
    case IT_TIMING_THREAD:
    case IT_TIMING_TIMED | IT_TIMING_THREAD:
    case IT_TIMING_PASSIVE:
    case IT_TIMING_PASSIVE | IT_TIMING_THREAD:
    case IT_TIMING_POLLING:
        break;  // O.K.
    default: {
        QSqlQuery q = getQuery();
        EXCEPTION(EDATA, r, tr("Invalid 'timing'\n") + typeErrMsg(q));
      }
    }
    PDEB(VVERBOSE) << name() << VDEB(value) << " timing = " << r << endl;
    return r;
}

int cInspector::getInspectorProcess(const QString &value)
{
    int r = 0;
    if (!value.isEmpty()) {
        QStringList vl = value.split(QRegExp("\\s*,\\s*"));
        if (vl.contains(_sRespawn,  Qt::CaseInsensitive)) r |= IT_PROCESS_RESPAWN;
        if (vl.contains(_sContinue, Qt::CaseInsensitive)) r |= IT_PROCESS_CONTINUE;
        if (vl.contains(_sPolling,  Qt::CaseInsensitive)) r |= IT_PROCESS_POLLING;
        if (vl.contains(_sTimed,    Qt::CaseInsensitive)) r |= IT_PROCESS_TIMED;
        if (vl.contains(_sCarried,  Qt::CaseInsensitive)) r |= IT_PROCESS_CARRIED;
    }
    switch (r) {
    case IT_PROCESS_RESPAWN:
    case IT_PROCESS_CONTINUE:
    case IT_PROCESS_POLLING:
    case IT_PROCESS_TIMED:
    case IT_PROCESS_CARRIED:
    case IT_PROCESS_POLLING | IT_PROCESS_CARRIED:
    case IT_PROCESS_TIMED   | IT_PROCESS_CARRIED:
        r |= getInspectorMethod(feature(_sMethod));
        break;  // O.K.
    case IT_NO_PROCESS:
        // Van megadva parancs, de nincs megadva process időzítés
        // A metódustol függően még jó lehet
    {
        int met = getInspectorMethod(feature(_sMethod));
        switch (met) {
        // ezeknél nem is kell (lehet) időzítés
        case IT_METHOD_NAGIOS:
        case IT_METHOD_NAGIOS | IT_METHOD_QPARSE:
        case IT_METHOD_SAVE_TEXT:
//      case IT_METHOD_MUNIN:
        case IT_METHOD_QPARSE:
        case IT_METHOD_QPARSE | IT_METHOD_PARSER:
        case IT_METHOD_QPARSE | IT_METHOD_CARRIED:
        case IT_METHOD_QPARSE | IT_METHOD_PARSER | IT_METHOD_CARRIED:
            return r | met;
        default: {
            // Nem OK
            QSqlQuery q = getQuery();
            EXCEPTION(EDATA, met, tr("Invalid 'process', 'method'\n") + typeErrMsg(q));
          }
        }
    }
    default: {
        // Nem OK
        QSqlQuery q = getQuery();
        EXCEPTION(EDATA, r, tr("Invalid 'process''\n") + typeErrMsg(q));
      }
    }
    PDEB(VVERBOSE) << name() << VDEB(value) << " process = " << r << endl;
    return r;
}

int cInspector::getInspectorMethod(const QString &value)
{
    if (value.isEmpty()) return 0;
    int r = 0;  // IT_METHOD_CUSTOM
    QStringList vl = value.split(QRegExp("\\s*,\\s*"));
    if (vl.contains(_sNagios,   Qt::CaseInsensitive)) r |= IT_METHOD_NAGIOS;
    if (vl.contains(_sText,     Qt::CaseInsensitive)) r |= IT_METHOD_SAVE_TEXT;
//  if (vl.contains(_sMunin,    Qt::CaseInsensitive)) r |= IT_METHOD_MUNIN;
    if (vl.contains(_sCarried,  Qt::CaseInsensitive)) r |= IT_METHOD_CARRIED;
    if (vl.contains(_sQparse,   Qt::CaseInsensitive)) r |= IT_METHOD_QPARSE;
    if (vl.contains(_sParser,   Qt::CaseInsensitive)) r |= IT_METHOD_PARSER;
    if (vl.contains(_sInspector,Qt::CaseInsensitive)) r |= IT_METHOD_INSPECTOR;
    switch (r) {
    case IT_METHOD_CUSTOM:
    case IT_METHOD_NAGIOS:
    case IT_METHOD_NAGIOS | IT_METHOD_QPARSE:
    case IT_METHOD_SAVE_TEXT:
//  case IT_METHOD_MUNIN:
    case IT_METHOD_CARRIED:
    case IT_METHOD_QPARSE:
    case IT_METHOD_PARSER:
    case IT_METHOD_QPARSE | IT_METHOD_PARSER:
    case IT_METHOD_QPARSE | IT_METHOD_CARRIED:
    case IT_METHOD_QPARSE | IT_METHOD_PARSER | IT_METHOD_CARRIED:
    case IT_METHOD_INSPECTOR:
        break;    // O.K.
    default: {
        QSqlQuery q = getQuery();
        EXCEPTION(EDATA, r, tr("Invalid 'method'") + typeErrMsg(q));
      }
    }
    PDEB(VVERBOSE) << name() << VDEB(value) << " method = " << r << endl;
    return r;
}

int cInspector::getInspectorType(QSqlQuery& q)
{
    _DBGFN() << name() << " " << endl;
    // Nem típus, de itt előkotorjuk az esetleges Time out értékeket is, ha vannak
    // "stopTimeOut"  Program indítás, millisec-ben
    // "startTimeOut" vagy "timeOut" program futás, vagy más folyamatra szán idő szintén millisec-ben
    bool ok;
    quint64 to;
    QString sto, stod;
    stod = feature(_sTimeout);
    sto  = feature("startTimeout");
    if (sto.isEmpty()) sto = stod;
    if (!sto.isEmpty()) {
        to = sto.toULongLong(&ok);
        if (ok) startTimeOut = to;
    }
    sto  = feature("stopTimeout");
    if (sto.isEmpty()) sto = stod;
    if (!sto.isEmpty()) {
        to = sto.toULongLong(&ok);
        if (ok) stopTimeOut = to;
    }
    PDEB(VERBOSE) << VDEB(startTimeOut) << VDEB(stopTimeOut) << endl;
    if (startTimeOut <= 0 || stopTimeOut <= 0) {
        EXCEPTION(EDATA, 0,
                  tr("Invalid time out value in %1. startTimeOut = %2, stopTimeOut = %3")
                  .arg(name()).arg(startTimeOut).arg(stopTimeOut));
    }
    // Típus:
    inspectorType = 0;
    if (isFeature("auto_transaction")) inspectorType |= IT_AUTO_TRANSACTION;
    if (isFeature(_sSuperior))         inspectorType |= IT_SUPERIOR;
    if (pParent == nullptr)            inspectorType |= IT_MAIN;
    int r = getCheckCmd(q);
    switch (r) {
    case  0:        // Nincs program hívás
        PDEB(VERBOSE) << tr("Nincs program hívás") << endl;
        inspectorType |= getInspectorTiming(feature(_sTiming));
        inspectorType |= getInspectorMethod(feature(_sMethod));
        if ((method() & (/* IT_METHOD_MUNIN | */ IT_METHOD_NAGIOS | IT_METHOD_INSPECTOR | IT_METHOD_SAVE_TEXT)) != 0) {
            EXCEPTION(EDATA, inspectorType, tr("Nem értelmezhető inspectorType érték (#0) :\n") + typeErrMsg(q));
        }
        break;
    case  1:        // Program hívása, a hívó applikációban
    PDEB(VERBOSE) << tr("A '%1' program hívása").arg(checkCmd + " " + checkCmdArgs.join(" ")) << endl;
        r = getInspectorProcess(feature(_sProcess));
        inspectorType |= r;
        if ((r & IT_PROCESS_MASK) == IT_NO_PROCESS) {
            int timing = getInspectorTiming(feature(_sTiming));
            if (!timing && inspectorType & (IT_METHOD_NAGIOS | IT_METHOD_SAVE_TEXT /*| IT_METHOD_MUNIN */))
                timing = IT_PROCESS_TIMED;
            inspectorType |= timing;
            break;
        }
        // Check...
        r &= ~IT_PROCESS_CARRIED;   // ellenörzés szempontjából érdektelen
        r |= getInspectorTiming(feature(_sTiming));
        r &= ~IT_TIMING_THREAD;     // ellenörzés szempontjából érdektelen
        switch (r) {
        case IT_CUSTOM:
        case IT_PROCESS_POLLING  | IT_TIMING_TIMED:
        case IT_PROCESS_TIMED    | IT_TIMING_TIMED:
            EXCEPTION(EDATA, inspectorType,
                tr("Nem értelmezhető inspectorType érték (#1) :\n") + typeErrMsg(q));
        }
        break;
    case -1:        // Van Check Cmd, de éppen a hívot app mi vagyunk
        PDEB(VERBOSE) << tr("A hívott alprogramban...") << endl;
        inspectorType |= getInspectorTiming(feature(_sTiming));
        r = getInspectorMethod(feature(_sMethod));
        inspectorType |= r;
        switch (r) {
        case IT_METHOD_INSPECTOR:
        case IT_METHOD_CARRIED:
            break;
        default:
            EXCEPTION(EDATA, inspectorType,
                tr("Nem értelmezhető inspectorType érték (#1) :\n") + typeErrMsg(q));
        }
        break;
    default: EXCEPTION(EPROGFAIL, r, name());
    }
    _DBGFNL() << name() << VDEB(inspectorType) << endl;
    return inspectorType;
}

void cInspector::self(QSqlQuery& q, const QString& __sn)
{
    // Elöszőr mindent kiürítünk, mert jobb a békesség
    pService = nullptr;
    pDelete(pNode);
    pDelete(pPort);
    pDelete(_pFeatures);
    hostService.clear();
    // Ha már beolvastuk..
    if (lanView::getInstance()->pSelfHostService != nullptr
     && lanView::getInstance()->pSelfService     != nullptr) {
        pService = &lanView::selfService();
        pNode    =  lanView::selfNode().dup()->reconvert<cNode>();
        hostService.clone(lanView::selfHostService());
    }
    else {
        // Előkotorjuk a szolgáltatás típus rekordot.
        pService = cService::service(q, __sn);
        // Jelenleg feltételezzük, hogy a node az egy host
        pNode = lanView::selfNode().dup()->reconvert<cNode>();   // ??!
        pNode->fetchSelf(q);
        // És a host_services rekordot is előszedjük.
        hostService.fetchByIds(q, pNode->getId(), service()->getId());
    }
}

int cInspector::getCheckCmd(QSqlQuery& q)
{
    QString val;
    int ixCheckCmd = cService::_descr_cService().toIndex(_sCheckCmd);

    checkCmdArgs.clear();
    if (pProtoService != nullptr) checkCmd = pProtoService->getName(ixCheckCmd);
    if (pPrimeService != nullptr) {
        val = pPrimeService->getName(ixCheckCmd);
        if (!val.isEmpty()) {
            if (val == "!") checkCmd.clear();
            else            checkCmd = val;
        }
    }
    val = service()->getName(ixCheckCmd);
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
    QString checkCmdSubs;   // Command substituted
    // Substitute
    while (i != checkCmd.constEnd()) {
        char c = i->toLatin1();
        QChar qc = *i;
        ++i;
        if (separator != 0 && c == separator) {
            separator = 0;
            checkCmdSubs += qc;
            continue;
        }
        if (c == '\'') {
            separator = c;
            checkCmdSubs += qc;
            continue;
        }
        if (c == '\\') {
            if (i != checkCmd.constEnd()) {
                checkCmdSubs += qc;
                checkCmdSubs += *i;
                ++i;
            }
            continue;
        }
        if (c == '$') {
            if (i->toLatin1() == '$') {
                ++i;
            }
            else {
                QString vname;
                vname = getParName(i, checkCmd.constEnd());
                checkCmdSubs += getParValue(q, vname);
                continue;
            }
        }
        checkCmdSubs += qc;
    }
    // Split args
    bool prevSpace = false;
    i = checkCmdSubs.constBegin();
    while (i != checkCmdSubs.constEnd()) {
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
        if (c == '\\') {
            if (i != checkCmdSubs.constEnd()) {
                arg += *i;
                ++i;
            }
            continue;
        }
        arg += qc;
    }
    checkCmdArgs << arg;                // Utolsó paraméter
    if (protoService().getName() == _sSsh) {
        checkCmdArgs.prepend(node().getIpAddress(q).toString());
        checkCmdArgs.prepend(_sSsh);
    }
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
        // Önmagunk hívása nem jó ötlet
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
        if (!fcmd.isExecutable()) EXCEPTION(ENOTFILE, -1, tr("Ismeretlen %1 parancs a %2 -ben").arg(checkCmd).arg(name()));  // nem volt a path-on sem
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
    // Check state
    if (internalStat != IS_SUSPENDED && internalStat != IS_STOPPED && internalStat != IS_OMITTED) {
        QString msg = tr("%1  skip %2, internalStat = %3").arg( __PRETTY_FUNCTION__, name(), internalStatName());
        PDEB(WARNING) << msg << endl;
        APPMEMO(*pq, msg, RS_WARNING);
        return;
    }
    // timeperiods ?
    if (!timeperiod._isEmpty()) {
        if (!timeperiod.isOnTime(*pq)) {
            QDateTime now = QDateTime::currentDateTime();
            int t = int(now.msecsTo(timeperiod.nextOnTime(*pq, now)));
            internalStat = IS_OMITTED;
            timerStat    = TS_OMMIT;
            if (isThread()) {
                if (pInspectorThread == nullptr)
                    EXCEPTION(EPROGFAIL,0, tr("pInspectorThread is NULL"));
                pInspectorThread->timer(t, timerStat);
            }
            else {
                killTimer(timerId);
                timerId = startTimer(t);
                if (0 == timerId) EXCEPTION(EPROGFAIL, retryInt, tr("Timer not started."));
            }
            return;
        }
    }

    internalStat = IS_RUN;
    _DBGFN() << " Run: " << hostService.getId() << QChar(' ')
             << host().getName()    << QChar('(') << host().getId()    << QChar(')') << QChar(',')
             << service()->getName()<< QChar('(') << service()->getId() << QChar(')') << _sCommaSp
             << "Thread: " << (isMainThread() ? "Main" : objectName()) <<  endl;
    if (inspectorType & IT_TIMING_PASSIVE) {
        if (pSubordinates != nullptr) {
            int n = 0;  // Hány alárendelt fut még?
            int maxState = RS_UNKNOWN;
            int minState = RS_ON;
            foreach (cInspector * pSub, *pSubordinates) {
                if (pSub != nullptr
                 && (pSub->internalStat == IS_RUN           // Éppen fut
                  || pSub->internalStat == IS_SUSPENDED     // Lefutott, várakozik
                  || pSub->internalStat == IS_STOPPED)) {   // Leállt, várakozik
                    ++n;
                    int state = int(pSub->hostService.getId(_sHostServiceState));
                    if (minState < state) minState = state;
                    if (maxState > state) maxState = state;
                }
            }
            if (!n) EXCEPTION(NOTODO, 1, tr("No runing any (%1) sub services. Abort.").arg(n));
            QString msg = tr("Runing sub services : %1/%2; states : %3 - %4")
                    .arg(n).arg(pSubordinates->size())
                    .arg(notifSwitch(maxState), notifSwitch(minState));
            hostService.setState(*pq, _sOn, msg);
        }
        internalStat = IS_SUSPENDED;
        return;
    }
    if (!isTimed()) EXCEPTION(EPROGFAIL, inspectorType, name());
    if (isThread() && isMainThread()) EXCEPTION(EPROGFAIL, inspectorType, name());
    bool statSetRetry = doRun(true);
    // normal/retry intervallum kezelése
    if (interval != retryInt) { // csak akkor ha különböző
        int hard = notifSwitch(hostService.getName(_sHardState));
        int soft = notifSwitch(hostService.getName(_sSoftState));
        // Ha a soft status rosszabb mint a hard status, és nem RS_ON vagy RS_RECOVERED, akkor retry interval
        if (statSetRetry || (soft >= RS_WARNING && hard < soft)) {
            if (timerStat != TS_RETRY) {
                toRetryInterval();
            }
        }
        else {
            if (timerStat != TS_NORMAL) {
                toNormalInterval();
            }
        }
    }
    else if (timerStat == TS_FIRST) {
        toNormalInterval();  // Ha az első esetleg túl rövid, ne legyen felesleges event.
    }
    if (internalStat == IS_RUN) internalStat = IS_SUSPENDED;
    DBGFNL();
}

bool cInspector::doRun(bool __timed)
{
    _DBGFN() << name() << (__timed ? _sTimed : _sNul) << endl;
    int  retStat = RS_UNREACHABLE;  // A lekérdezés alapértelmezett státusza
    bool statIsSet    = false;  // A statusz beállítva
    bool statSetRetry = false;  // Időzítés modosítása retry-re
    cError * lastError = nullptr;  // Hiba leíró
    if (lastRun.isValid()) {
        lastElapsedTime = lastRun.restart();
    }
    else {
        lastElapsedTime = 0;
        lastRun.start();
    }
    QString statMsg;
    // Tesszük a dolgunkat bármi legyen is az, egy tranzakció lessz, hacsak le nem tiltották
    QString tn = toSqlName(name());
    try {
        if (inspectorType & IT_AUTO_TRANSACTION) sqlBegin(*pq, tn);
        retStat      = run(*pq, statMsg);
        statIsSet    = retStat & RS_STAT_SETTED;
        statSetRetry = retStat & RS_SET_RETRY;
        retStat      = (retStat & RS_STAT_MASK);
    } CATCHS(lastError);
    qlonglong elapsed = lastRun.elapsed();
    // Ha többet csúszott az időzítés mint 50%
    if (__timed  && lastElapsedTime > ((interval*3)/2)) {
        // Ha a státusz már rögzítve, és nincs egyéb hiba, ez nem fog megjelenni sehol
        statMsg = msgCat(statMsg, tr("Az időzítés csúszott: %1 > %2").arg(lastElapsedTime).arg(interval));
    }
    if (lastError != nullptr) {   // Ha hívtuk a run metódust, és dobott egy hátast
        if (inspectorType & IT_AUTO_TRANSACTION) sqlRollback(*pq, tn);  // Hiba volt, inkább visszacsináljuk az egészet.
        if (pProcess != nullptr && QProcess::NotRunning != pProcess->state()) {
            pProcess->kill();
        }
        // A hibárol LOG az adatbázisba, amugy töröljük a hibát
        lanView  *plv = lanView::getInstance();
        qlonglong id = sendError(lastError);
        if (plv->lastError == lastError) plv->lastError = nullptr;
        pDelete(lastError);
        statMsg = msgCat(statMsg, tr("Hiba, ld.: app_errs.applog_id = %1").arg(id));
        hostService.setState(*pq, _sUnreachable, statMsg, elapsed, parentId(EX_IGNORE));
        internalStat = IS_STOPPED;
    }
    else {
        if (inspectorType & IT_AUTO_TRANSACTION) sqlCommit(*pq, tn);
        if (retStat < RS_WARNING
         && ((interval > 0 && lastRun.hasExpired(interval)))) { // Ha ugyan nem volt hiba, de sokat tököltünk
            statMsg = msgCat(statMsg, tr("Időtúllépés, futási idö %1 ezred másodperc").arg(elapsed));
            hostService.setState(*pq, notifSwitch(RS_WARNING), statMsg, elapsed, parentId(EX_IGNORE));
        }
        else if (!statIsSet) {   // Ha még nem volt status állítás
            statMsg = msgCat(statMsg, tr("Futási idő %1 ezred másodperc").arg(elapsed));
            hostService.setState(*pq, notifSwitch(retStat), statMsg, elapsed, parentId(EX_IGNORE));
        }
    }
    _DBGFNL() << name() << endl;
    return statSetRetry;
}

int cInspector::run(QSqlQuery& q, QString& runMsg)
{
    _DBGFN() << name() << endl;
    (void)q;
    if (pProcess != nullptr) {
        if (checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
        PDEB(VERBOSE) << "Run : " << checkCmd << " " << checkCmdArgs.join(" ") << endl;
        int ec = pProcess->startProcess(int(startTimeOut), int(stopTimeOut));
        if (ec == -1) return RS_STAT_SETTED;    // Already sended: RS_CRITICAL
        return parse(ec, *pProcess);
    }
    else {
        runMsg = tr("Incomplete cInspector (%1) object. No activity specified.").arg(name());
        PDEB(VVERBOSE) << runMsg << endl;
        internalStat = isTimed() ? IS_SUSPENDED : IS_STOPPED;
    }
    return RS_UNREACHABLE;
}

enum eNotifSwitch cInspector::parse(int _ec, QIODevice& _text)
{
    _DBGFN() <<  name() << endl;
    enum eNotifSwitch r = RS_INVALID;
    int method = inspectorType & IT_METHOD_MASK;
    switch (method) {
//  case IT_METHOD_CUSTOM:
    case IT_METHOD_CARRIED:
        return RS_STAT_SETTED;
//  case IT_METHOD_MUNIN:   EXCEPTION(EPROGFAIL);   break;
    case IT_METHOD_NAGIOS:
    case IT_METHOD_NAGIOS | IT_METHOD_QPARSE:
    case IT_METHOD_SAVE_TEXT:
    case IT_METHOD_QPARSE | IT_METHOD_CARRIED:
    case IT_METHOD_QPARSE:
        break;
    default:
        EXCEPTION(ENOTSUPP, inspectorType, name());
    }
    QString text = QString::fromUtf8(_text.readAll());
    switch (method) {
//  case IT_METHOD_MUNIN:   EXCEPTION(EPROGFAIL);   break;
    case IT_METHOD_NAGIOS:
    case IT_METHOD_NAGIOS | IT_METHOD_QPARSE:
        r = parse_nagios(_ec, text);
        break;
    case IT_METHOD_SAVE_TEXT:
        r = save_text(_ec, text);
        break;
    case IT_METHOD_QPARSE | IT_METHOD_CARRIED:
    case IT_METHOD_QPARSE:
        r = parse_qparse(_ec, text);
        break;
    }
    return r;
}

enum eNotifSwitch cInspector::parse_nagios(int _ec, const QString &text)
{
    int r = RS_DOWN, rr;
    switch (_ec) {
    case NR_OK:         r = RS_ON;           break;
    case NR_WARNING:    r = RS_WARNING;      break;
    case NR_CRITICAL:   r = RS_CRITICAL;     break;
    case NR_UNKNOWN:    r = RS_UNREACHABLE;  break;
    }
    QString note = text;
    if (inspectorType & IT_METHOD_QPARSE) switch (r) {
    case RS_ON:
    case RS_WARNING:
    case RS_CRITICAL:
        rr = parse_qparse(_ec, text);
        note += tr("\nQuery pareser result : %1 (#%2).").arg(notifSwitch(rr, EX_IGNORE)).arg(rr);
        break;
    default:
        note += tr("\nQuery pareser has been left out.");
    }
    hostService.setState(*pq, notifSwitch(r), note, lastRun.elapsed());
    return RS_STAT_SETTED;
}

enum eNotifSwitch cInspector::save_text(int _ec, const QString &text)
{
    cRecord *pParam;
    if (pPort != nullptr) {
        pParam = new cPortParam;
        pParam->setId(_sPortId, portId());
    }
    else {
        pParam = new cNodeParam;
        pParam->setId(_sNodeId, nodeId());
    }
    QString n = feature(_sName);
    if (n.isEmpty()) n = service()->getName();
    pParam->setName(n);
    pParam->setId(_sParamTypeId, cParamType::paramType(_sText).getId());
    pParam->setName(_sParamValue, text);
    pParam->setNote(tr("Service %1, command (%2) return code #%3")
                    .arg(name(), (QStringList(checkCmd) += checkCmdArgs).join(_sSpace)).arg(_ec));
    pParam->replace(*pq);
    return _ec == 0 ? RS_ON : RS_WARNING;
}

enum eNotifSwitch cInspector::parse_qparse(int _ec, const QString &text)
{
    _DBGFN() << name() << endl;
    (void)_ec;
    QString comment = feature(_sComment);
    // Ha a parsert nem mi indítottuk, keressük meg, valamelyik parent lessz az indító
    if (pQparser == nullptr) {
        for (cInspector *pPar = pParent; pPar != nullptr; pPar = pPar->pParent) {
            pQparser = pPar->pQparser;
            if (pQparser != nullptr) break;     // Megtaláltuk
        }
        if (pQparser == nullptr) {
            QString msg = tr(
                        "A '%1' parancs nem hajtható végre.\n"
                        "A %2 szolgáltatás példány kontexusa hibás\n"
                        "Nem qparse típus, és nincs qparse típusú szülő sem!"
                             ).arg(text, name());
            EXCEPTION(EFOUND,0,msg);
        }
        pQparser = pQparser->newChild(this);
    }
    pQparser->setInspector(this);   // A kliens beállítása...
    bool ok = false;
    // A text soronkénti feldolgozása
    QRegExp sep("[\r\n]+");
    QStringList sl = text.split(sep);
    foreach (QString t, sl) {
        t = t.simplified();
        if (t.isEmpty()) continue;      // üres
        if (!comment.isEmpty() && 0 == t.indexOf(comment)) continue;    // Van komment jel. és az első karakter az
        cError *pe = nullptr;
        int r = pQparser->parse(t, pe);
        // Ha semmire sem volt találat
        if (r == R_NOTFOUND) {
            PDEB(VVERBOSE) << "DROP : " << quotedString(t) << endl;
            continue;  // Nincs minta a sorra, nem foglalkozunk vele
        }
        // Találat, és hiba
        if (r != REASON_OK) {
            if (pe == nullptr) {
                DERR() << _sUnKnown << VDEB(r) << endl;
                EXCEPTION(EUNKNOWN, r, name());
            }
            DERR() << pe->msg() << endl;
            pe->exception();
        }
        ok = ok || r == REASON_OK;
        PDEB(VVERBOSE) << "MATCH : " << quotedString(t) << endl;
    }
    _DBGFNL() << name() << " : " << (ok ? "OK" : "INVALID") << endl;
    if (ok) return (inspectorType & IT_METHOD_CARRIED) ? RS_STAT_SETTED : RS_ON;
    return RS_INVALID;
}
/*
enum eNotifSwitch cInspector::munin(QSqlQuery& q, QString& runMsg)
{
    runMsg = tr("Nem támogatott (még).");
    (void)q;
    return RS_INVALID;
}
*/

void cInspector::start()
{
    _DBGFN() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
    // Check
    if (internalStat != IS_INIT) {
        EXCEPTION(EDATA, internalStat, QObject::tr("%1 nem megfelelő belső állapot").arg(name()));
    }
    if (timerId != -1)
        EXCEPTION(EDATA, timerId, QObject::tr("%1 óra újra inicializálása.").arg(name()));
    if (pQparser != nullptr)
        EXCEPTION(EPROGFAIL, -1, tr("A %1-ben a parser objektum nem létezhet a start elött!").arg(name()));
    // pre start
    if (inspectorType & IT_METHOD_PARSER) {
        internalStat = IS_RUN;
        pQparser = new cQueryParser();
        PDEB(VVERBOSE) << tr("%1: Alloc QParser : %2").arg(name()).arg(qlonglong(pQparser),0,16) << endl;
        int r = pQparser->load(*pq, serviceId(), true);
        if (R_NOTFOUND == r && nullptr != pPrimeService) r = pQparser->load(*pq, primeServiceId(), true);
        if (R_NOTFOUND == r && nullptr != pProtoService) r = pQparser->load(*pq, protoServiceId(), true);
        if (R_NOTFOUND == r) inspectorType |= IT_PURE_PARSER;
        cError *pe = nullptr;
        pQparser->setInspector(this);
        pQparser->prep(pe);
        if (nullptr != pe) pe->exception();
    }
    // Thread
    if (isThread()) {   // Saját szál?
        if (pInspectorThread == nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("%1 pThread egy NULL pointer.").arg(name()));
        internalStat = IS_RUN;
        pInspectorThread->start();
        _DBGFNL() << " (thread) " << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    // Process
    if (inspectorType & IT_PROCESS_MASK_NOTIME) {   // Program indítás időzités nélkül
        if (checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
        internalStat = IS_RUN;
        PDEB(VERBOSE) << "Start : " << checkCmd << endl;
        pProcess->startProcess(int(startTimeOut));
        _DBGFNL() << " (process) " << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    // Start timer
    if (isTimed()) {
        pRunTimeVar = getServiceVar(_sRuntime);
        if (pRunTimeVar == nullptr) {
            pRunTimeVar = new cServiceVar();
            pRunTimeVar->setName(_sRuntime);
            pRunTimeVar->setId(cServiceVar::ixServiceVarTypeId(), cServiceVarType::srvartype(*pq, _sRuntime)->getId());
            pRunTimeVar->setId(_sHostServiceId, hostServiceId());
            pRunTimeVar->insert(*pq);
        }
        internalStat = IS_SUSPENDED;
        qlonglong t = firstDelay();
        timerId = startTimer(int(t));
        if (0 == timerId) EXCEPTION(EPROGFAIL, interval, tr("Timer not started."));
        timerStat = TS_FIRST;
        startSubs();
        _DBGFNL() << " (timed) " << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    // Passive
    if (passive()) {
        if (inspectorType & IT_SUPERIOR && timing() == IT_TIMING_PASSIVE) {
            PDEB(VERBOSE) << "Start (passive)" << name() << " timer " << interval << "ms"
                          << " object thread : " << thread()->objectName() << endl;
            timerId   = startTimer(interval);
            if (0 == timerId) EXCEPTION(EPROGFAIL, interval, tr("Timer not started."));
            timerStat = TS_NORMAL;
        }
        internalStat = IS_SUSPENDED;  // Csinálja a semmit
    }
    else {
        internalStat = IS_RUN;
        (void)doRun(false);
        internalStat = IS_STOPPED;
    }
    startSubs();
    if (inspectorType & IT_TIMING_POLLING) drop();
    _DBGFNL() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
    return;
}

int cInspector::firstDelay()
{
    qint64 t;
    QDateTime last;
    bool ok;
    t = feature("delay").toLongLong(&ok);
    if (!ok || t <= 0) {    // Ha nincs magadva késleltetés
        if (!hostService.isNull(_sLastTouched)) {
            last = hostService.get(_sLastTouched).toDateTime();
            qint64 ms = last.msecsTo(QDateTime::currentDateTime());
            if (ms < interval) t = interval - ms;
            else               t = int(rnd(retryInt));
        }
        else {
            t = rnd(interval);
        }
    }
    if (t < 1000) t = 1000;    // min 1 sec
    if (!timeperiod._isEmpty()) {   // timeperiod ?
        QSqlQuery q = getQuery();
        QDateTime now  = QDateTime::currentDateTime();
        QDateTime next = now.addMSecs(t);
        if (!timeperiod.isOnTime(q, next)) {
            t = now.msecsTo(timeperiod.nextOnTime(q, next));
        }
    }
    PDEB(VERBOSE) << "Start " << name() << " timer " << interval << QChar('/') << t << "ms, Last time = " << last.toString()
                  << " object thread : " << thread()->objectName() << endl;
    return int(t);
}

void cInspector::startSubs()
{
    _DBGFN() << name() << endl;
    if (pSubordinates != nullptr) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            cInspector *p = *i;
            if (p != nullptr) {
                p->start();
            }
        }
    }
}

void cInspector::drop(eEx __ex)
{
    _DBGFN() << name() << endl;

    internalStat = IS_DOWN;
    if (isThread()) {
        if (pInspectorThread == nullptr) {
            QString m = QObject::tr("%1 pThread egy NULL pointer.").arg(name());
            if(__ex != EX_IGNORE) EXCEPTION(EPROGFAIL, -1, m);
            DWAR() << m << endl;
        }
        else if (pInspectorThread->isRunning()) {
            PDEB(VVERBOSE) << "Stop " << pInspectorThread->objectName() << " thread..." << endl;
            pInspectorThread->quit();
            pInspectorThread->wait(stopTimeOut);
            if (pInspectorThread->isRunning()) {
                DWAR() << "Thread " << pInspectorThread->objectName() << " quit time out" << endl;
                pInspectorThread->terminate();
            }
            else {
                DWAR() << "Thread " << pInspectorThread->objectName() << " is not running." << endl;
            }
        }
    }
    if (isTimed()) {
        if (timerId <= 0) {
            if (!isThread()) {
                if(__ex != EX_IGNORE) EXCEPTION(EPROGFAIL);
            }
            else if (pInspectorThread != nullptr) {
                pInspectorThread->start();  // status down-ban, ráadjuk a vezérlést, hogy le tudjon állni
                pInspectorThread->wait(stopTimeOut);
            }
        }
        else if (timerId > 0) {
            killTimer(timerId);
            timerId = -1;
        }
        else if (__ex != EX_IGNORE) {
            QString m = QObject::tr("El sem indított %1 óra leállítása.").arg(name());
            if(__ex != EX_IGNORE) EXCEPTION(EDATA, -1, m);
            DWAR() << m << endl;
        }
        timerStat = TS_STOP;
    }
    if (inspectorType & IT_METHOD_PARSER) {
        if (pQparser == nullptr) {
            if(__ex != EX_IGNORE) EXCEPTION(EPROGFAIL, 0, name());
        }
        else {
            cError *pe = nullptr;
            pQparser->post(pe);
            if (pe != nullptr) DERR() << pe->msg() << endl;
            PDEB(VVERBOSE) << tr("%1: Free QParser : %2").arg(name()).arg(qlonglong(pQparser),0 , 16) << endl;
            pDelete(pQparser);
            if (pSubordinates != nullptr) {
                // Az gyerkőcöknél is törölni kell, feltételezzük, hogy 1*-es a mélység, és csak ez az egy parser van a rész fában.
                foreach (cInspector *pi, *pSubordinates) {
                    pi->pQparser = nullptr;
                }
            }
        }
    }
    if (!thread()) dropSubs();
    _DBGFNL() << name() << endl;
}

void cInspector::toRetryInterval()
{
    DBGFN();
    if (timerStat == TS_RETRY) return;
    timerStat = TS_RETRY;
    if (timerId < 0) EXCEPTION(EPROGFAIL, -1, name());
    if (isThread()) {
        if (pInspectorThread == nullptr)
            EXCEPTION(EPROGFAIL,0, tr("pInspectorThread is NULL"));
        pInspectorThread->timer(retryInt, timerStat);
    }
    else {
        killTimer(timerId);
        timerId = startTimer(retryInt);
        if (0 == timerId) EXCEPTION(EPROGFAIL, retryInt, tr("Timer not started."));
    }
}

void cInspector::toNormalInterval()
{
    DBGFN();
    if (timerStat == TS_NORMAL) return;
    timerStat = TS_NORMAL;
    if (timerId < 0) EXCEPTION(EPROGFAIL, -1, name());
    if (isThread()) {
        if (pInspectorThread == nullptr)
            EXCEPTION(EPROGFAIL,0, tr("pInspectorThread is NULL"));
        pInspectorThread->timer(interval, timerStat);
    }
    else {
        killTimer(timerId);
        timerId = startTimer(interval);
        if (0 == timerId) EXCEPTION(EPROGFAIL, interval, tr("Timer not started."));
    }
}

QString cInspector::name() const
{
    QString r = "(";
    r += typeid(*this).name();
    r += "):";
    static const QString    qq("??");
    if (pNode != nullptr && !node().isNull(node().nameIndex())) r += node().getName();
    else                                                     r += qq;
    r +=  QChar(':');
    if (pPort != nullptr) r += nPort().getName() + QChar(':');
    if (pService != nullptr) r += service()->getName();
    else                  r += qq;
    if (pPrimeService != nullptr || pProtoService != nullptr) {
        r += "(";
        if (pPrimeService != nullptr) r += pPrimeService->getName();
        r += ":";
        if (pProtoService != nullptr) r += pProtoService->getName();
        r += ")";
    }
    r += QString("[%1]").arg(hostServiceId());
    return r;
}

QString cInspector::getParValue(QSqlQuery& q, const QString& name, bool *pOk)
{
    static const QString _sHostservice = "hostservice";
    static const QString _sHostService = "host_service";
    if (pOk != nullptr) *pOk = true;
    QString v;
    v = feature(name, EX_IGNORE);                   // feature parameter ?
    if (!v.isEmpty()) return v;
    v = cSysParam::getTextSysParam(q, name, _sNul); // System parameter ?
    if (!v.isEmpty()) return v;
    QStringList sl = name.split(QChar('.'));
    if (sl.size() > 1) {
        if (0 == sl.first().compare("parent", Qt::CaseInsensitive)) {
            if (pParent == nullptr) {
                if (pOk == nullptr) EXCEPTION(EDATA, -1, name);
                *pOk = false;
                return QString();
            }
            sl.pop_front();
            QString pnm = sl.join('.');
            return pParent->getParValue(q, pnm, pOk);
        }
        if (sl.size() > 2) {
            if (pOk == nullptr) EXCEPTION(EDATA, sl.size(), name);
            *pOk = false;
            return QString();
        }
        QString on = sl[0];
        QString fn = sl[1];
        if (0 == on.compare(_sHostService,    Qt::CaseInsensitive)) return hostService.getName(fn);
        if (0 == on.compare(_sHostservice,    Qt::CaseInsensitive)) return hostService.getName(fn);
        if (0 == on.compare(_sService,        Qt::CaseInsensitive)) return service()->getName(fn);
        if (0 == on.compare(_sNode,           Qt::CaseInsensitive)) return node().getName(fn);
        if (0 == on.compare(_sHost,           Qt::CaseInsensitive)) return node().getName(fn);
        if (0 == on.compare(_sInterface,      Qt::CaseInsensitive)) return nPort().getName(fn);
        if (pOk == nullptr) EXCEPTION(EDATA, 2, name);
        *pOk = false;
        return QString();
    }
    else {
        if (0 == name.compare(_sHomeDir,      Qt::CaseInsensitive)) return lanView::getInstance()->homeDir;
        if (0 == name.compare(_sBinDir,       Qt::CaseInsensitive)) return lanView::getInstance()->binPath;
        if (0 == name.compare(_sHostService,  Qt::CaseInsensitive)) return hostService.getName();
        if (0 == name.compare(_sHostservice,  Qt::CaseInsensitive)) return hostService.getName();
        if (0 == name.compare(_sService,      Qt::CaseInsensitive)) return service()->getName();
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
        if (0 == name.compare(_sAddress,      Qt::CaseInsensitive)) {
            return cNode::fetchPreferedAddress(q, nodeId(), hostService.getId(_sPortId)).toString();
        }
        if (0 == name.compare(_sProtocol,     Qt::CaseInsensitive)) return protoService().getName();
        if (0 == name.compare(_sPort,         Qt::CaseInsensitive)) {
            qlonglong pn = service()->getId(_sPort);
            if (pn == NULL_ID) pn = protoService().getId(_sPort);
            if (pn == NULL_ID) {
                if (pOk == nullptr) EXCEPTION(EDATA, 0, name);
                *pOk = false;
                return QString();
            }
            return QString::number(pn);
        }
        if (0 == name.compare(_sHostServiceId,Qt::CaseInsensitive))return QString::number(hostServiceId());
        if (0 == name.compare(_sNodeId,       Qt::CaseInsensitive))return QString::number(nodeId());
        if (0 == name.compare(_sServiceId,    Qt::CaseInsensitive))return QString::number(serviceId());

        v = getEnvVar(name.toStdString().c_str());      // Environment variable ?
        if (!v.isEmpty()) return v;

        if (pOk == nullptr) EXCEPTION(EDATA, 1, name);
        *pOk = false;
        return QString();
    }
}

QString cInspector::typeErrMsg(QSqlQuery& q)
{
    return tr(
           "Szervíz példány : %1\n"
           "process = '%2', timing = '%3', method = '%4'\n"
           "'features' mezők: %5 / %6 / %7 / %8.")
             .arg(name())
             .arg(feature(_sProcess)).arg(feature(_sTiming)).arg(feature(_sMethod))
             .arg(hostService.view(q, _sFeatures))
             .arg(service()->view(q, _sFeatures))
             .arg(pProtoService == nullptr ? cColStaticDescr::rNul : protoService().view(q, _sFeatures))
             .arg(pPrimeService == nullptr ? cColStaticDescr::rNul : primeService().view(q, _sFeatures));
}

cServiceVar *cInspector::getServiceVar(const QString& name)
{

    if (pVars == nullptr || pVars->isEmpty()) return nullptr;
    return pVars->get(name, EX_IGNORE);
}

/* ********************************************************************************** */

QString internalStatName(eInternalStat is)
{
    switch (is) {
    case IS_INIT:       return _sInit;
    case IS_DOWN:       return _sDown;
    case IS_RUN:        return _sRun;
    case IS_SUSPENDED:  return "suspended";
    case IS_OMITTED:    return "ommited";
    case IS_STOPPED:    return _sStopped;
    case IS_ERROR:      return _sError;
    }
    return QString("Invalid(%1)").arg(is, 0, 16);
}

