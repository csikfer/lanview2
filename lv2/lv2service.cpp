#include "lanview.h"
#include "lv2service.h"

class cInspectorThreadThread : public QThread {
    friend class cInspectorThread;
protected:
    cInspectorThreadThread(cInspectorThread *p)
        : QThread(&p->inspector)
        , pInspectorThread(p) {
        setObjectName(p->inspector.name());
        _DBGOBJ() << objectName() << endl;
    }
    cInspectorThread * pInspectorThread;
    virtual void run();
};

void cInspectorThreadThread::run()
{
    _DBGFN() << pInspectorThread->inspector.name() << endl;
    pInspectorThread->run();
    if (pInspectorThread->internalStat == IS_RUN) {
        PDEB(INFO) << QObject::tr("Start %1 event loop ...").arg(objectName()) << endl;
        exec();
        PDEB(INFO) << QObject::tr("Stop %1 event loop ...").arg(objectName()) << endl;
        pInspectorThread->on_thread_killTimer();
        pInspectorThread->inspector.timerStat = TS_STOP;
        pInspectorThread->inspector.internalStat = IS_STOPPED;
    }
}

cInspectorThread::cInspectorThread(cInspector *pp)
    : QObject()
    , inspector(*pp)
    , pThread(new cInspectorThreadThread(this))
{
    const QString name = inspector.name();
    _DBGFN() << name << endl;
    setObjectName(name + "::cInspectorThread");
    _DBGFN() << objectName() << endl;
    pThread->setObjectName(name);
    abortFlag = false;
    pLastError = nullptr;
    _DBGFNL() << name << endl;
}

cInspectorThread::~cInspectorThread()
{
    delete pThread;
}

void cInspectorThread::timerEvent(QTimerEvent * event)
{
    _DBGFN() << objectName() << endl;
    inspector.timerEvent(event);
}


void cInspectorThread::run()
{
    pDelete(pLastError);
    try {
        _DBGFN() << objectName() << " " << internalStatName(internalStat) << endl;
        if (abortFlag) {
            DWAR() << "Abort is on ..." << endl;
            return;
        }
        switch (internalStat) {
        case IS_INIT:   doInit();   break;
        case IS_DOWN:   doDown();   break;
        case IS_RUN:    doRun();    break;
        default:
            EXCEPTION(EPROGFAIL, internalStat, internalStatName(internalStat));
        }
    }
    catch(QThread *) {
        _DBGFN() << " Abort (exception) " << objectName() << endl;
        return;
    }
    CATCHS(pLastError)

    if (pLastError) {
        inspector.internalStat = IS_ERROR;
    }
    DBGFNL();
}

void cInspectorThread::doInit()
{
    _DBGFN() << inspector.name() << endl;
    inspector.threadPostInit();
    if (inspector.inspectorType & IT_SUPERIOR) {
        if (inspector.pSubordinates != nullptr) EXCEPTION(EPROGFAIL);
        inspector.pSubordinates = new QList<cInspector *>;
        inspector.setSubs(*inspector.pQuery());
    }
}

void cInspectorThread::doRun()
{
    _DBGFN() << inspector.name() << endl;
    if (inspector.isTimedType()) {
        if (inspector.passive() && (inspector.pSubordinates == nullptr || inspector.pSubordinates->isEmpty())) {
            EXCEPTION(NOTODO, 0, inspector.name());
        }
        int t = inspector.firstDelay();
        PDEB(VERBOSE) << "Start timer " << inspector.interval << QChar('/') << t << "ms in new thread " << inspector.name() << endl;
        inspector.timerId = startTimer(t);
        if (inspector.timerId <= 0) {
            EXCEPTION(EPROGFAIL, t, tr("Timer not started."));
        }
    }
    else {
        inspector.doRun(false);
        inspector.startSubs();
        inspector.internalStat = IS_STOPPED;
        return;
    }
    inspector.internalStat = IS_SUSPENDED;
    inspector.startSubs();
}

void cInspectorThread::doDown()
{
    _DBGFN() << inspector.name() << endl;
    if (inspector.timerId > 0) {
        PDEB(VERBOSE) << "Kill timer " << inspector.name() << endl;
        killTimer(inspector.timerId);
        inspector.timerId = 0;
    }
    inspector.dropSubs();
    pDelete(inspector.pQThread);
}

void cInspectorThread::chkAbort()
{
    if (abortFlag) {
        DWAR() << objectName() << " Abort exception..." << endl;
        throw (pThread);
    }
}


void cInspectorThread::do_timerEvent()
{
    QTimerEvent e(0);
    _DBGFN() << inspector.name() << endl;
    inspector.timerEvent(&e);
    _DBGFNL() << inspector.name() << endl;
}

void cInspectorThread::on_thread_killTimer()
{
    if (inspector.timerId > 0) {
        killTimer(inspector.timerId);
        inspector.timerId = -1;
    }
}

void cInspectorThread::do_reset_sub(cInspector *psi, bool _start)
{
    inspector.do_reset_sub(psi, _start);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static QString _sRestartMax = "restart_max";
#define DEF_RESTART_MAX 5

cInspectorProcess::cInspectorProcess(cInspector *pp)
    : QProcess(pp)
    , inspector(*pp)
{
    setObjectName(inspector.name() + "::cInspectorProcess");
    _DBGFN() << objectName() << endl;

    bool ok = false;
    QString s;

    reStartCnt = 0;
    lastElapsed = NULL_ID;
    bProcessFinished = bProcessReadyRead = false;
    internalStat = IS_INIT;


    if ((s = inspector.feature(_sRestartMax)).size()) {
        reStartMax = s.toInt(&ok);
        if (!ok) EXCEPTION(EDATA, -1, tr("Az %1 értéke nem értelmezhető : %2").arg(_sRestartMax, s));
    }
    if (!ok) {
        reStartMax = int(cSysParam::getIntegerSysParam(*inspector.pQuery(), _sRestartMax, DEF_RESTART_MAX));
    }

    errCntClearTime = inspector.interval * 5;
    if (errCntClearTime <= 0) errCntClearTime = 600000; // 10min

    maxArcLog   = 4;
    maxLogSize  = 10 * 1024 * 1204; // 10MiByte
    logNull     = false;
    isAsync = 0 != (inspector.inspectorType & IT_PROCESS_ASYNC);
    if (isAsync) {
        // stderr --> log
        // stdout --> text output
        setCurrentReadChannel(QProcess::StandardError); // Log
    }
    else {
        // stderr + stdout ==> stdout
        setProcessChannelMode(QProcess::MergedChannels);
    }

    bool nolog = inspector.methodType() != IT_METHOD_INSPECTOR  || inspector.isFeature(_sLognull);
    if (nolog) {
        logNull = true;
        maxArcLog = maxLogSize = 0;
    }
    else {
        if ((s = inspector.feature(_sLogrot)).isEmpty() == false) {
            static const QRegularExpression re(ANCHORED("(\\d+)([kMG]?)[,;]?(\\d*)"));
            QRegularExpressionMatch ma;
            if (!re.isValid()) EXCEPTION(EPROGFAIL, 0, re.pattern());
            if (!(ma = re.match(s)).hasMatch()) EXCEPTION(EDATA, -1, tr("Az %1 értéke nem értelmezhető : %2").arg(_sLogrot, s));
            s = ma.captured(1);   // $1
            maxLogSize = s.toInt(&ok);
            if (!ok) EXCEPTION(EPROGFAIL, 0, s);
            s = ma.captured(2);   // $2
            if (s.size()) switch (s[0].toLatin1()) {
            case 'k':   maxLogSize *= 1024;                 break;
            case 'M':   maxLogSize *= 1024 * 1024;          break;
            case 'G':   maxLogSize *= 1024 * 1024 * 1024;   break;
            default:    EXCEPTION(EPROGFAIL, 0, s);
            }
            s = ma.captured(3);   // $3
            if (s.size()) {
                maxArcLog = s.toInt(&ok);
                if (!ok) EXCEPTION(EPROGFAIL, 0, s);
            }
        }
        QString fileName = lanView::getInstance()->homeDir + "/log";    // LOG DIR
        if (!QDir().mkpath(fileName)) { // Check dir, or create
            EXCEPTION(ENODIR, -1, fileName);
        }
        fileName += '/';
        fileName += inspector.node().getName() + '.';
        fileName += inspector.service()->getName();
        if (inspector.pPrimeService != nullptr) fileName += '.' + inspector.pPrimeService->getName();
        if (inspector.pProtoService != nullptr) fileName += '.' + inspector.pProtoService->getName();
        fileName += "_" + QString::number(inspector.hostServiceId());
        fileName += ".log";
        actLogFile.setFileName(fileName);
        bool f;
        if (actLogFile.exists()) f = actLogFile.open(QIODevice::Append | QIODevice::WriteOnly);
        else                     f = actLogFile.open(QIODevice::WriteOnly);
        if (!f) {
            EXCEPTION(EFOPEN, -1, actLogFile.fileName());
        }
    }
}

enum eStartProcessResult {
    SPR_ERROR_START = -1,
    SPR_ERROR_STOP  = -2,
    SPR_SKIP_START  = -3,
    SPR_CRASH       = -4,
    SPR_NO_WAIT     = 0x80000,
};

int cInspectorProcess::startProcess(int startTo, int stopTo)
{
    _DBGFN() << VDEB(startTo) << VDEB(stopTo) << endl;
    QString msg;
    if (inspector.checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
    bool isRuning = state() == QProcess::Running;
    if (isRuning) {
        msg = tr("Service %1 : Process %2 already runing, no start.").arg(inspector.name(), inspector.checkCmd + " " + inspector.checkCmdArgs.join(" "));
        APPMEMO(*inspector.pQuery(), msg, RS_WARNING);
        return SPR_SKIP_START;
    }
    PDEB(VERBOSE) << "START program : " << inspector.checkCmd << " " << inspector.checkCmdArgs.join(" ")
                   << (isAsync ? "; Asyncron (no wait for exit)" :
                                (stopTo == 0) ? "; no wait for exit" : "; Syncron (wait for exit)")
                   << endl;
    if (isAsync || stopTo == 0) {  // No wait for terminate, asyncron call
        if (!bProcessFinished) {
            // connect(this, SIGNAL(finished(int, ExitStatus)), this, SLOT(processFinished(int, ExitStatus))); // Ez nem működik !?
            bProcessFinished = connect(this, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &cInspectorProcess::processFinished);
        }
        if (!bProcessReadyRead) {
            PDEB(VVERBOSE) << "Set connects for path through log. ..." << endl;
            // bProcessReadyRead = connect(this, SIGNAL(readyRead()), this, SLOT(processReadyRead()));
            bProcessReadyRead = connect(this, &QProcess::readyRead, this, &cInspectorProcess::processReadyRead);
        }
        if (!bProcessFinished || !bProcessReadyRead) {
            msg = inspector.name() + " : ";
            if (!bProcessFinished)  msg = msgCat(msg, tr("Connection failed to finished() signal."));
            if (!bProcessReadyRead) msg = msgCat(msg, tr("Connection failed to readyRead() signal."));
            EXCEPTION(EPROGFAIL,0, msg);
        }
    }
    lastStart.invalidate();
    lastStart.start();
    start(inspector.checkCmd, inspector.checkCmdArgs, QIODevice::ReadOnly);
    internalStat = IS_RUN;
    if (!waitForStarted(startTo)) {
        kill();
        msg = tr("'waitForStarted(%1)' hiba : %2").arg(startTo).arg(ProcessError2Message(error()));
        inspector.setState(*inspector.pQuery(), _sUnreachable, msg);
        internalStat = IS_ERROR;
        return SPR_ERROR_START;
    }
    switch (state()) {
    case QProcess::Running:
        if (isAsync || stopTo == 0) {
            PDEB(VVERBOSE) << "Runing and continue ..." << endl;
            return SPR_NO_WAIT;
        }
        PDEB(VVERBOSE) << "Program runing, wait for finished ..." << endl;
        if (!waitForFinished(stopTo)) {
            msg = tr("Started `%1` 'waitForFinished(%2)' error: '%3'.")
                    .arg(inspector.checkCmd + " " + inspector.checkCmdArgs.join(" "))
                    .arg(stopTo).arg(ProcessError2Message(error()));
            DERR() << msg << endl;
            inspector.setState(*inspector.pQuery(), _sUnreachable, msg);
            kill();
            if (QProcess::Running == state()) waitForFinished(10000);
            internalStat = IS_ERROR;
            return SPR_ERROR_STOP;
        }
        internalStat = IS_SUSPENDED;
        break;              // EXITED
    case QProcess::NotRunning:
        internalStat = IS_SUSPENDED;
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
    inspector.internalStat = IS_ERROR;
    msg = tr("A '%1' program elszállt : %2").arg(inspector.checkCmd, ProcessError2Message(error()));
    PDEB(VVERBOSE) << msg << endl;
    inspector.setState(*inspector.pQuery(), _sCritical, msg);
    return SPR_CRASH;
}

#define RESTART_FAILURE -1

void cInspectorProcess::processFinished(int _exitCode, QProcess::ExitStatus _exitStatus)
{
    int exitStatus = _exitStatus;
    _DBGFN() << VDEB(_exitCode) << VDEB(exitStatus) << ", program : " << inspector.checkCmd << endl;
    if (inspector.internalStat != IS_RUN && inspector.internalStat != IS_STOPPED) {
        DERR() << tr("Invalid event, internalStat = %1").arg(internalStatName(inspector.internalStat)) << endl;
        return;
    }
    lastElapsed = lastStart.isValid() ? lastStart.elapsed() : NULL_ID;
    if (isAsync) {  // Vártunk, hogy befejezze.
        if (internalStat == IS_RUN) {
            internalStat = IS_SUSPENDED;
            setCurrentReadChannel(QProcess::StandardOutput);
            QString tn = "process_" + toSqlName(inspector.name());
            QString statMsg;
            int  retStat = RS_UNREACHABLE;  // A lekérdezés alapértelmezett státusza
            bool statIsSet    = false;  // A statusz beállítva
            bool statSetRetry = false;  // Időzítés modosítása retry-re
            cError *pe = nullptr;
            bool isTransaction =  0 != (inspector.inspectorType & IT_AUTO_TRANSACTION);
            try {
                if (isTransaction) sqlBegin(*inspector.pQuery(), tn);
                if (exitStatus == NormalExit) {
                    retStat = inspector.parse(exitCode(), *this, statMsg);
                    statIsSet    = retStat & RS_STAT_SETTED;
                    statSetRetry = retStat & RS_SET_RETRY;
                    retStat      = (retStat & RS_STAT_MASK);
                }
                else {
                    statMsg = tr("Called program is crashed.");
                    statSetRetry = true;
                }
            } CATCHS(pe);
            qlonglong elapsed = inspector.lastRun.elapsed();
            if (pe != nullptr) {
                if (isTransaction) sqlRollback(*inspector.pQuery(), tn);  // Hiba volt, inkább visszacsináljuk az egészet.
                // A hibárol LOG az adatbázisba, amugy töröljük a hibát
                lanView  *plv = lanView::getInstance();
                qlonglong id = sendError(pe);
                if (plv->lastError == pe) plv->lastError = nullptr;
                pDelete(pe);
                statMsg = msgCat(statMsg, tr("Hiba, ld.: app_errs.applog_id = %1").arg(id));
                inspector.setState(*inspector.pQuery(), _sUnreachable, statMsg, inspector.parentId(EX_IGNORE));
                inspector.internalStat = IS_STOPPED;
            }
            else {
                if (isTransaction) sqlCommit(*inspector.pQuery(), tn);
                if (retStat < RS_WARNING
                 && ((inspector.interval > 0 && inspector.lastRun.hasExpired(inspector.interval)))) { // Ha ugyan nem volt hiba, de sokat tököltünk
                    statMsg = msgCat(statMsg, tr("Időtúllépés, futási idö %1 ezred másodperc").arg(elapsed));
                    inspector.setState(*inspector.pQuery(), notifSwitch(RS_WARNING), statMsg, inspector.parentId(EX_IGNORE));
                }
                else if (!statIsSet) {   // Ha még nem volt status állítás
                    statMsg = msgCat(statMsg, tr("Futási idő %1 ezred másodperc").arg(elapsed));
                    inspector.setState(*inspector.pQuery(), notifSwitch(retStat), statMsg, inspector.parentId(EX_IGNORE));
                }
            }
            // normal/retry intervallum kezelése
            inspector.manageInterval(statSetRetry, int(elapsed));
        }
        else {
            DWAR() << inspector.name() << " internal stat is : "
                   << internalStatName(internalStat) << " / " << internalStatName(inspector.internalStat) << endl;
        }
    }
    else if (inspector.inspectorType & (IT_PROCESS_CONTINUE | IT_PROCESS_RESPAWN)) {   // Program indítás volt időzités nélkül
        QString msg = "?";
        while (true) {
            if (lastElapsed > errCntClearTime) {
                lastElapsed = reStartCnt = 0;
                if (errCntClearTime <= 0) EXCEPTION(EPROGFAIL);
            }
            if (exitStatus != RESTART_FAILURE) {
                processReadyRead();
                switch (exitStatus) {
                case QProcess::CrashExit:
                    msg = tr("A %1 program összeomlott.").arg(inspector.checkCmd);
                    break;
                case QProcess::NormalExit:
                    msg = tr("A %1 program kilépett, exit = %2.").arg(inspector.checkCmd).arg(_exitCode);
                    break;
                }
            }
            if (!inspector.hostService.fetchById(*inspector.pQuery()) || inspector.hostService.getBool(_sDisabled)) {     // reread, enabled?
                if (!inspector.hostService.isNull()) {
                    inspector.setState(*inspector.pQuery(), _sDown, msg + tr(" Nincs újraindítás. Letíltva."));
                }
                inspector.internalStat = IS_STOPPED;
                break;
            }
            if (inspector.inspectorType & IT_PROCESS_CONTINUE || _exitCode != 0 || exitStatus ==  QProcess::CrashExit) {
                ++reStartCnt;
                if (reStartCnt > reStartMax) {
                    msg += tr(" Nincs újraindítás. Túl sok (%1 > %2) úgraindítási kísérlet.").arg(reStartCnt).arg(reStartMax);
                    inspector.setState(*inspector.pQuery(), _sDown, msg);
                    inspector.internalStat = IS_STOPPED;
                    PDEB(INFO) << msg << endl;
                    break;
                }
                else {
                    inspector.setState(*inspector.pQuery(), _sWarning, msg + tr("Újraindítási kíérlet (#%1).").arg(reStartCnt));
                    PDEB(INFO) << QString("ReStart %1 (< %2) : ").arg(reStartCnt).arg(reStartMax) << inspector.checkCmd << endl;
                    inspector.internalStat = IS_RUN;
                    int r = startProcess(int(inspector.startTimeOut), 0);
                    if (r == 0) break;
                    msg = tr("A %1 program újraindítása sikertelen : #%2").arg(inspector.checkCmd).arg(r);
                    exitStatus = RESTART_FAILURE;
                }
            }
        }
    }
    else {  // ?! Nem szabadna itt lennünk.
        EXCEPTION(EPROGFAIL, inspector.inspectorType, inspector.name());
    }
    close();
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

/// Minimum interval 10s
#define MINIMUM_INERVAL 10000

void cInspector::_init(cInspector *__par)
{
//  DBGFN();
    inspectorType= IT_CUSTOM;
    internalStat = IS_INIT;
    timerStat    = TS_STOP;

    pNode    = nullptr;
    pPort    = nullptr;
    pMergedFeatures= nullptr;
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
    pQMain   = nullptr;
    pQThread = nullptr;
    pSubordinates = nullptr;
    pProtoService = nullptr;
    pPrimeService = nullptr;
    pService = nullptr;
    pVars    = nullptr;
    pRunTimeVar = nullptr;
    initStatic();

    pParent = __par;
    if (pParent != nullptr) {
        setParent(pParent->usedAsParent());
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
    _init(__par);
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
    }
    setObjectName(name());
}


cInspector::cInspector(cInspector * __par, cNode *pN, const cService *pS, cNPort *pP)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << " (" << (__par == nullptr ? "NULL" : __par->name()) << ", "
             << pN->getName() << ", " << pS->getName() << ", " << (pP == nullptr ? "NULL" : pP->getName())
             << ")" << endl;
    _init(__par);
    inspectorType = IT_TIMING_PASSIVE;
    pNode    = pN;
    pService = pS;
    pPort    = pP;
    hostService.setId(_sNodeId,    pNode->getId());
    hostService.setId(_sServiceId, pService->getId());
    if (pPort != nullptr) hostService.setId(_sPortId, pPort->getId());
    setObjectName(name());
}


cInspector::cInspector(QSqlQuery& q, const QString &sn)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << VDEB(sn) << endl;
    _init(nullptr);
    self(q, sn);
    // Ha van specifikált port is
    if (! hostService.isNull(_sPortId)) pPort = cNPort::getPortObjById(q, hostService.getId(_sPortId));
    // A prime és proto service, ha van
    pPrimeService = hostService.getPrimeService(q);
    pProtoService = hostService.getProtoService(q);
    setObjectName(name());
}

cInspector::cInspector(QSqlQuery& q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *__par)
    : QObject(nullptr), hostService(), lastRun()
{
    _DBGFN() << VDEB(__host_service_id) << VDEB(__tableoid) << "; parent : " << (__par == nullptr ? "NULL" : __par->name()) << endl;
    _init(__par);
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
    setObjectName(name());
    _DBGFNL() << name() << endl;
}

cInspector::~cInspector()
{
    const QString myName = name();
    internalStat = IS_DOWN;
    PDEB(INFO) << myName << " state is DOWN" << endl;

    stop();

    dropSubs();

    // Ha ez a root objektum, akkor
    // Ha volt hiba objektumunk, töröljük. Elötte kiírjuk a hibaüzenetet, ha tényleg hiba volt
    if (pParent == nullptr) { // root object ?
        lanView *lanView = lanView::getInstance();
        cError *lastError = lanView->lastError;
        if (lanView->setSelfStateF && lastError != nullptr) {
            if (lastError->mErrorCode != eError::EOK) { // Error
                try {
                    setState(*pQuery(), _sCritical, lastError->msg(), NULL_ID, false);
                } catch(...) {
                    DERR() << tr("!!!! Failed write %1 exit error state.").arg(myName) << endl;
                }
            }
            else {
                try {
                    int rs = RS_DOWN;
                    QString n;
                    rs = int(lastError->mErrorSubCode); // Ide kéne rakni a kiírandó statust
                    n  = lastError->mErrorSubMsg;       // A megjegyzés a status-hoz
                    if ((rs & RS_STAT_SETTED) == 0) {   // Jelezheti, hogy már megvolt a kiírás!
                        setState(*pQuery(), notifSwitch(rs), n, NULL_ID, false);
                    }
                } catch(...) {
                    DERR() << tr("!!!! Failed write %1 exit state.").arg(myName) << endl;
                }
            }
        }
    }


    PDEB(INFO) << QChar(' ') << myName << " internalStat = " << internalStatName() << endl;

    if (isThreadType() && pInspectorThread != nullptr) {
        bool notTerm;
        int cnt = 0;
        ulong to = llMin(10000UL, stopTimeOut);  // minimum 10s
        while ((notTerm = pInspectorThread->pThread->isRunning())) {
            if (++cnt <= 2) {
                pInspectorThread->abortFlag = true;
                QString msg = "Send stop doRun() " + pInspectorThread->objectName() + " thread...";
                if (cnt == 1) { PDEB(VERBOSE) << msg << endl; }
                else          { DWAR()        << msg << endl; }
                pInspectorThread->pThread->quit();
            }
            else {
                DERR() << "Send treminate : doRun() " << pInspectorThread->objectName() << " thread..." << endl;
                if (cnt > 3) break;     // Just two tries
                pInspectorThread->pThread->terminate();
            }
            pInspectorThread->pThread->wait(to);
        }
        if (notTerm) {
            DERR() << "Thread " << pInspectorThread->objectName() << " is nothing stop doRun(), nothing start thread doDown() ." << endl;
        }
        else {
            PDEB(INFO) << "Thread " << pInspectorThread->objectName() << " is not running doRun(), start thread doDown() ." << endl;
            pInspectorThread->abortFlag = false;
            emitStartThread(IS_DOWN);  // Indítás az IS_DOWN állapottal timert leállítja, QSqlQuerry objektumo(ka)t törli
            cnt = 0;
            while ((notTerm = pInspectorThread->pThread->isRunning())) {
                pInspectorThread->pThread->wait(to);
                if (++cnt <= 2) {
                    pInspectorThread->abortFlag = true;
                    QString msg = "Send stop doDown() " + pInspectorThread->objectName() + " thread...";
                    if (cnt == 1) { PDEB(VERBOSE) << msg << endl; }
                    else          { DWAR()        << msg << endl; }
                    pInspectorThread->pThread->quit();
                }
                else {
                    DERR() << "Send treminate : doDown() " << pInspectorThread->objectName() << " thread..." << endl;
                    if (cnt > 3) break;     // Just 3 tries
                    pInspectorThread->pThread->terminate();
                }
            }
            if (notTerm) {
                DERR() << "Thread " << pInspectorThread->objectName() << " is nothing stop doDown()." << endl;
            }
        }
        dropThreadDb(pInspectorThread->pThread->objectName(), EX_IGNORE);
        pDelete(pInspectorThread);
    }

    if (timerId > 0) {
        PDEB(VERBOSE) << "Kill timer " << name() << endl;
        killTimer(timerId);
        timerId = -1;
        timerStat = TS_STOP;
    }

    if (inspectorType & IT_METHOD_PARSER && pQparser != nullptr) {
        cError *pe = nullptr;
        pQparser->post(pe);
        if (pe != nullptr) DERR() << pe->msg() << endl;
        PDEB(VVERBOSE) << tr("%1: Free QParser : %2").arg(name()).arg(qlonglong(pQparser),0 , 16) << endl;
        pDelete(pQparser);
    }
    else pQparser = nullptr;

    pDelete(pProcess);
    pDelete(pQMain);
    pDelete(pQThread);
    pDelete(pPort);
    pDelete(pNode);
    pDelete(pMergedFeatures);
    if (pVars != nullptr) {
        while (!pVars->isEmpty()) delete pVars->takeFirst();
        delete pVars;
        pVars = nullptr;
    }

    if (pParent == nullptr && lanView::appStat == IS_RUN) {
        QCoreApplication::exit(0);
    }
}

void cInspector::dropSubs()
{
    if (pSubordinates != nullptr) {
        QList<cInspector*>::Iterator i, n = pSubordinates->end();
        for (i = pSubordinates->begin(); i != n; ++i) {
            threadChkAbort();
            cInspector *p = *i;
            if (p != nullptr) delete p;
        }
        delete pSubordinates;
        pSubordinates = nullptr;
    }
}

cInspector *cInspector::newSubordinate(QSqlQuery& _q, qlonglong _hsid, qlonglong _toid, cInspector * _par)
{
    threadChkAbort();
    return new cInspector(_q, _hsid, _toid, _par);
}

cInspectorProcess *cInspector::newProcess()
{
    _DBGFN() << name() << endl;
    cInspectorProcess *p = new cInspectorProcess(this);
    p->setObjectName(name());
    return p;
}

void cInspector::postInit(QSqlQuery& q)
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
        EXCEPTION(NOTODO, 0, tr("It never runs."));
//      break;
    default:
        timeperiod.setById(q, tpid);
        break;
    }
    // Az időzítéssek, ha vannak
    interval = int(variantToId(get(_sNormalCheckInterval), EX_IGNORE, -1));         // Default : -1
    retryInt = int(variantToId(get(_sRetryCheckInterval),  EX_IGNORE, interval));
    // Típus
    getInspectorType(q);
    if (interval <=  0 && isTimedType()) {   // Időzített időzítés nélkül ?!
        EXCEPTION(EDATA, interval, QObject::tr("%1 időzített lekérdezés, időzítés nélkül.").arg(name()));
    }
    // Ha meg van adva az ellenörző parancs, és nem a parseré, akkor a QProcess objektum létrehozása
    if (0 == (inspectorType & IT_METHOD_PARSE_CMD) && !checkCmd.isEmpty()) {
        pProcess = newProcess();
    }
    // Saját szál ?
    if (isThreadType()) {
        pInspectorThread = newThread(); //
        threadInit(pInspectorThread);
        emitStartThread(IS_INIT);           // Init, Init után leáll
        if (!pInspectorThread->pThread->wait(startTimeOut)) {   // Az init még szinkron, megvárjuk a végét
            pInspectorThread->abortFlag = true;
            EXCEPTION(ETO, 0, tr("%1 thread init.").arg(name()));
        }
        if (pInspectorThread->pLastError != nullptr) {  // Nem sikerült az init
            cError *pe = pInspectorThread->pLastError;
            pInspectorThread->pLastError = nullptr;
            pe->exception();
        }
    }
    // Ha superior. (Thread-nél ezt a thread-ben kell elintézni)
    else if (inspectorType & IT_SUPERIOR) {
        // process-nél a mi dolgunk ?
        bool f = pProcess == nullptr;
        f = f || !(inspectorType & (IT_METHOD_INSPECTOR));
        if (f) {
            pSubordinates = new QList<cInspector *>;
            setSubs(q);
        }
    }
    pVars = fetchVars(q);    // Változók, ha vannak
    variablesPostInit(q);
}

void cInspector::variablesPostInit(QSqlQuery& q)
{
    if (variablesListFeature(q, _sVariables)) {
        variablesListCreateOrCheck(q);
    }
}

bool cInspector::variablesListFeature(QSqlQuery &q, const QString& _name)
{
    (void)q;
    QString sVars = feature(_name);
    QStringList varNames;
    QString   tName, vName;
    if (!sVars.isEmpty()) {
        varsListMap = cFeatures::value2listMap(sVars);
        varNames = varsListMap.keys();
        foreach (vName, varNames) {
            threadChkAbort();
            QStringList& sl = varsListMap[vName];
            if (sl.size() != 2) {
                EXCEPTION(EDATA, 0, QObject::tr("Invalid '%1' : '%2'").arg(_name, sVars));
            }
            varsFeatureMap[vName] = sl.at(0);
            tName = sl.at(1);
            if (nullptr == cServiceVarType::srvartype(q, tName, EX_IGNORE)) {
                EXCEPTION(EDATA, 0, QObject::tr("Invalid '%1' : '%2'; Missing '%3' type.").arg(_name, sVars, tName));
            }
        }
    }
    return !varNames.isEmpty();
}

void cInspector::variablesListCreateOrCheck(QSqlQuery &q)
{
    for (QMap<QString, QStringList>::const_iterator i = varsListMap.constBegin(); i != varsListMap.constEnd(); ++i) {
        const QString& vName = i.key();
        threadChkAbort();
        variablePostInitCreateOrCheck(q, vName);
    }
}

void cInspector::variablePostInitCreateOrCheck(QSqlQuery &q, const QString& _name)
{
    const QString& tName = varsListMap[_name].at(1);
    qlonglong stid = cServiceVarType::srvartype(q, tName)->getId();
    cInspectorVar *pVar = getInspectorVar(_name);
    if (pVar == nullptr) {  // If missing, then create
        pVar = new cInspectorVar(q, this, _name, stid);
        pVar->postInit(q);
        vars() << pVar;
    }
}

void cInspector::dbNotif(const QString& cmd)
{
    // Thread esetén ez necces lehet !!!!
    bool ok = false;
    if (0 == cmd.compare(_sTick, Qt::CaseInsensitive)) {
        if (pInspectorThread != nullptr) {
            ok = QMetaObject::invokeMethod(pInspectorThread, "do_timerEvent", Qt::AutoConnection);
        }
        else {
            ok = QMetaObject::invokeMethod(this, "do_timerEvent", Qt::AutoConnection);
        }
        if (!ok) {
            DERR() << "Invoke timerEvent() is unsuccesful." << endl;
        }
        return;
    }
    bool reStartFlg = 0 == cmd.compare(_sReset, Qt::CaseInsensitive);
    if (reStartFlg || 0 == cmd.compare(_sExit,  Qt::CaseInsensitive)) {
        if (pParent == nullptr) {
            DERR() << QString("Invalid ReSet notification to %1 main inspector.").arg(name()) << endl;
        }
        else if (pParent->pInspectorThread != nullptr) {
            ok = QMetaObject::invokeMethod(pParent->pInspectorThread, "do_reset_sub",  Q_ARG(cInspector*, this), Q_ARG(bool, reStartFlg));
        }
        else {
            ok = QMetaObject::invokeMethod(pParent, "do_reset_sub",  Q_ARG(cInspector*, this), Q_ARG(bool, reStartFlg));
        }
        if (!ok) {
            DERR() << "Invoke do_reset_sub() is unsuccesful." << endl;
        }
        return;
    }
}

void cInspector::setSubs(QSqlQuery& q)
{
    _DBGFN() << name() << endl;
    if (pSubordinates == nullptr) EXCEPTION(EPROGFAIL, -1, name());
    bool ok = true;
    QSqlQuery q2 = getQuery();
    static const QString _sql =
            "SELECT hs.host_service_id, h.tableoid "
             "FROM host_services AS hs JOIN nodes AS h USING(node_id) JOIN services AS s USING(service_id) "
             "WHERE hs.superior_host_service_id = %1 "
               "AND NOT s.disabled AND NOT hs.disabled "   // letiltottak nem kellenek
               "AND NOT s.deleted  AND NOT hs.deleted "    // töröltek sem kellenek
            ;
    QString sql = _sql.arg(hostServiceId());
    EXECSQL(q, sql)
    if (q.first()) do {
        threadChkAbort();
        qlonglong       hsid = variantToId(q.value(0));  // host_service_id      A szervíz rekord amit be kell olvasni
        qlonglong       hoid = variantToId(q.value(1));  // node tableoid        A node típusa
        cInspector *p  = nullptr;
        cError     *pe = nullptr;
        try {
            p = newSubordinate(q2, hsid, hoid, this);
            if (p != nullptr) {
                p->postInit(q2);  // ??
            }
        } CATCHS(pe)
        if (pe != nullptr) {
            ok = false;
            if (pe->mErrorCode != eError::EOK) {
                cHostService hs;
                QSqlQuery q3 = getQuery();
                if (hs.fetchById(q3, hsid)) {
                    hs.setState(q3, _sCritical, pe->msg(), hostServiceId(), false);
                }
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

QList<cInspectorVar *> * cInspector::fetchVars(QSqlQuery& _q)
{
    static const QString sql =
            "SELECT service_var_id"
            " FROM service_vars"
            " WHERE host_service_id = ?";
    QList<qlonglong> ids;
    QList<cInspectorVar *> *pl = nullptr;
    if (execSql(_q, sql, hostServiceId()) && getListFromQuery(_q, ids)) {
        pl = new QList<cInspectorVar *>;
        foreach (qlonglong id, ids) {
            threadChkAbort();
            cInspectorVar * pv = new cInspectorVar(_q, id, this);
            pv->postInit(_q);
            *pl << pv;
        }
    }
    return pl;
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

    if (pMergedFeatures  == nullptr) pMergedFeatures = new cFeatures();
    else                        pMergedFeatures->clear();

    pMergedFeatures->split(service()-> getName(ixFeatures), true, __ex);
    if (pPrimeService != nullptr) pMergedFeatures->split(pPrimeService->getName(ixFeatures), true, __ex);
    if (pProtoService != nullptr) pMergedFeatures->split(pProtoService->getName(ixFeatures), true, __ex);
    pMergedFeatures->split(hostService.getName(_sFeatures), true, __ex);
    PDEB(VVERBOSE) << name() << " features : " << pMergedFeatures->join() << endl;
    return *pMergedFeatures;
}

#define CONT_ONE(e, c)  \
    n = ::removeAll(vl, e);                \
    if (n  > 1 ) goto getFunc_error_label; \
    if (n == 1)  { r |= c; on = true; }

#define CONT_ONE_COL(e, c, col)  \
    n = ::removeAll(vl, e);                                  \
    if (n > 1 || (n == 1 && col)) goto getFunc_error_label;  \
    if (n == 1) { r |= c;     on = true; }

#define CONT_ONE_ONE(e, c)  CONT_ONE_COL(e, c, on)

#define getFunc_error_label getInspectorTiming_error

int cInspector::getInspectorTiming(const QString& value)
{
    int r = 0; // IT_TIMING_CUSTOM
    QStringList vl = value.split(QRegularExpression("\\s*,\\s*"), Q_SKIPEMPTYPARTS);
    if (vl.isEmpty()) return r;
    bool on = false;
    int n;
    if (vl.contains(_sCustom, Qt::CaseInsensitive)) {
        if (vl.size() == 1) return r;
        goto getFunc_error_label;
    }
    if (interval > 0 && !vl.contains(_sTimed, Qt::CaseInsensitive) && vl.contains(_sPassive, Qt::CaseInsensitive)) {
        vl << _sTimed;
    }
    CONT_ONE_ONE(_sTimed,   IT_TIMING_TIMED)
    CONT_ONE_ONE(_sPolling, IT_TIMING_POLLING)
    CONT_ONE(_sPassive, IT_TIMING_PASSIVE)
    if (!on) goto getFunc_error_label;
    CONT_ONE(_sThread,  IT_TIMING_THREAD)
    if (vl.isEmpty()) {
        PDEB(VVERBOSE) << name() << VDEB(value) << QString(" timing = %1").arg(r,0,16) << endl;
        return r;
    }
getFunc_error_label:
    QString msg;
    msg = tr("Hibás %1 paraméter a feature mezőben : %2").arg(_sTiming, value);
    EXCEPTION(EDATA,0,msg);
}

#undef getFunc_error_label
#define getFunc_error_label getInspectorProcess_error

int cInspector::getInspectorProcess(const QString &value)
{
    static const QString sAsync = "async";
    int r = getInspectorMethod(feature(_sMethod));
    int n;
    bool on = false;
    QStringList vl;
    vl = value.split(QRegularExpression("\\s*,\\s*"), Q_SKIPEMPTYPARTS);
    if (!vl.isEmpty()) {
        CONT_ONE_ONE(_sPolling,  IT_PROCESS_POLLING)
        CONT_ONE_ONE(_sTimed,    IT_PROCESS_TIMED)
        CONT_ONE_ONE(_sRespawn,  IT_PROCESS_RESPAWN)
        CONT_ONE_ONE(_sContinue, IT_PROCESS_CONTINUE)
        CONT_ONE(    _sCarried,  IT_METHOD_CARRIED)
        CONT_ONE(    sAsync,     IT_PROCESS_ASYNC)
    }
    if (!on) {
        n = getInspectorTiming(feature(_sTiming));
        switch (n) {
        case IT_TIMING_TIMED:   r |= IT_PROCESS_TIMED;      on = true;  break;
        case IT_TIMING_POLLING: r |= IT_PROCESS_POLLING;    on = true;  break;
        }
    }
    if (!on || !vl.isEmpty()) goto getFunc_error_label;
    if (r & IT_PROCESS_ASYNC) {
        if (r & IT_PROCESS_MASK_NOTIME) goto getFunc_error_label;
        if (!(r & IT_PROCESS_TIMED)) r |= IT_PROCESS_TIMED;
    }
    if (vl.isEmpty()) {
        PDEB(VVERBOSE) << name() << VDEB(value) << " process = " << r << endl;
        return r;
    }
getFunc_error_label:
    QString msg;
    msg = tr("Hibás %1 paraméter a feature mezőben : %2").arg(_sProcess, value);
    EXCEPTION(EDATA,0,msg);
}

#undef getFunc_error_label
#define getFunc_error_label getInspectorMethod_error

int cInspector::getInspectorMethod(const QString &value)
{
    int r = 0;  // IT_METHOD_CUSTOM
    if (value.isEmpty()) return r;
    QStringList vl = value.split(QRegularExpression("\\s*,\\s*"));
    bool on = false;
    int n;
    if (vl.contains(_sCustom,   Qt::CaseInsensitive)) {
        if (vl.size() == 1) return r;
        goto getFunc_error_label;
    }
    CONT_ONE(_sQparse,   IT_METHOD_QPARSE)
    CONT_ONE(_sParser,   IT_METHOD_PARSER)
    on = false;
    CONT_ONE_ONE(_sInspector,IT_METHOD_INSPECTOR)
    CONT_ONE(_sSave,     IT_METHOD_SAVE_TEXT)
    CONT_ONE(_sText,     IT_METHOD_PARSE_TEXT)
    CONT_ONE(_sCarried,  IT_METHOD_CARRIED)
    CONT_ONE_COL(_sNagios,   IT_METHOD_NAGIOS, r & (IT_METHOD_CARRIED))
    CONT_ONE_ONE(_sJson,     IT_METHOD_JSON)
    if (vl.isEmpty()) {
        PDEB(VERBOSE) << name() << VDEB(value) << QString(" method = %1").arg(r,0,16) << endl;
        return r;
    }
getFunc_error_label:
    QString msg;
    msg = tr("Hibás %1 paraméter a feature mezőben : %2").arg(_sMethod, value);
    EXCEPTION(EDATA,0,msg);
}



void cInspector::getTimeouts(ulong defaultStart, ulong defaultStop)
{

    startTimeOut = getTimeout(features(), _sStartTimeout, defaultStart);
    stopTimeOut  = getTimeout(features(), _sStopTimeout,  defaultStop);
    PDEB(VERBOSE) << VDEB(startTimeOut) << VDEB(stopTimeOut) << endl;
    if (startTimeOut <= 0 || stopTimeOut <= 0) {
        EXCEPTION(EDATA, 0,
                  tr("Invalid time out value in %1. startTimeOut = %2, stopTimeOut = %3")
                  .arg(name()).arg(startTimeOut).arg(stopTimeOut));
    }

}

enum eCmdType {
    CT_NO_COMMAND,
    CT_SHELL,
    CT_PARSER,
    CT_CALLED

};

int cInspector::getInspectorType(QSqlQuery& q)
{
    _DBGFN() << name() << " " << endl;
    // Nem típus, de itt előkotorjuk az esetleges Time out értékeket is, ha vannak
    // "stopTimeOut"  Program indítás, millisec-ben
    // "startTimeOut" vagy "timeOut" program futás, vagy más folyamatra szán idő szintén millisec-ben
    getTimeouts(2000, 10000);   // Default timeouts : start 2s, stop 10s
    // Típus:
    inspectorType = 0;
    if (isFeature("auto_transaction")) inspectorType |= IT_AUTO_TRANSACTION;
    if (isFeature(_sSuperior))         inspectorType |= IT_SUPERIOR;
    if (pParent == nullptr) {
        PDEB(VERBOSE) << "Parent is NULL" << endl;
        inspectorType |= IT_MAIN;        // Csak a rész fa gyökere !!!
    }
    else {
        PDEB(VERBOSE) << "Parent is " << pParent->name() << endl;
    }
    int r = getCheckCmd(q);
    switch (r) {
    case CT_NO_COMMAND:        // Nincs program hívás
        PDEB(VERBOSE) << tr("Nincs program hívás") << endl;
        inspectorType |= getInspectorTiming(feature(_sTiming));
        inspectorType |= getInspectorMethod(feature(_sMethod));
        if ((inspectorType & IT_METHOD_MASK_PROCESS) != 0) {
            EXCEPTION(EDATA, inspectorType, tr("Nem értelmezhető inspectorType érték (#0) :\n") + typeErrMsg(q));
        }
        break;
    case CT_SHELL:        // Calling a program, we are the caller
        PDEB(VERBOSE) << tr("A '%1' program hívása").arg(checkCmd + " " + checkCmdArgs.join(" ")) << endl;
        r = getInspectorProcess(feature(_sProcess));
        inspectorType |= r;
        // Check...
        r |= getInspectorTiming(feature(_sTiming));
        switch (r) {
        case IT_CUSTOM:
        case IT_PROCESS_POLLING  | IT_TIMING_TIMED:
            EXCEPTION(EDATA, inspectorType,
                tr("Nem értelmezhető inspectorType érték (#1) :\n") + typeErrMsg(q));
        }
        break;
    case CT_CALLED:        // We have Check Cmd, but we are in the program to be called
        PDEB(VERBOSE) << tr("We're in the called program.") << endl;
        inspectorType |= getInspectorTiming(feature(_sTiming));
        r = getInspectorMethod(feature(_sMethod));
        inspectorType |= r;
        checkCmd.clear();   // nem hívunk programot, már abban vagyunk.
        switch (r) {
        case IT_METHOD_INSPECTOR:
        case IT_METHOD_CARRIED:
            break;
        default:
            EXCEPTION(EDATA, inspectorType,
                tr("Nem értelmezhető inspectorType érték (#2) :\n") + typeErrMsg(q));
        }
        break;
    case CT_PARSER:
        PDEB(VERBOSE) << tr("A '%1' végrehajytása a parserrel").arg(checkCmd) << endl;
        r = getInspectorProcess(feature(_sProcess));
        if (r & (IT_PROCESS_MASK_NOTIME)) {
            EXCEPTION(EDATA, inspectorType,
                tr("Nem értelmezhető inspectorType érték (#3) :\n") + typeErrMsg(q));
        }
        inspectorType |= r | IT_METHOD_PARSE_CMD;
        break;
    default:
        EXCEPTION(EPROGFAIL, r, name());
        break;
    }

    if (isThreadType()) {   // stopTimeout
        getTimeouts(30000, 15000);  // Default timeouts : start 30s, stop 15s
    }

    _DBGFNL() << name() << VDEBHEX(inspectorType) << endl;
    return inspectorType;
}

void cInspector::self(QSqlQuery& q, const QString& __sn)
{
    // Elöszőr mindent kiürítünk, mert jobb a békesség
    pService = nullptr;
    pDelete(pNode);
    pDelete(pPort);
    pDelete(pMergedFeatures);
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
    static const int ixCheckCmd = cService::_descr_cService().toIndex(_sCheckCmd);

    checkCmd.clear();
    checkCmdArgs.clear();
    // Get check_cmd
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
    if (checkCmd.isEmpty()) {
        PDEB(VERBOSE) << "Check command is empty ..." << endl;
        return CT_NO_COMMAND;
    }
    if (inspectorType & IT_MAIN) {      // Rész fa gyökere, már egy hívott szolgáltatás
        PDEB(VERBOSE) << name() << " : root of the part tree." << endl;
        return CT_CALLED;
    }

    // exec by pareser? First character eq. '&' ?
    eCmdType r = CT_SHELL;
    if (checkCmd.startsWith(QChar('&'))) {
        checkCmd.remove(0,1);   // Remove '&' marker
        r = CT_PARSER;
    }

    QString checkCmdSubs;   // Command substituted
    checkCmdSubs = substitute(q, this, checkCmd,
        [] (const QString& key, QSqlQuery& q, void *_p)
            {
                QString r;
                cInspector *p = static_cast<cInspector *>(_p);
                r = p->getParValue(q, key);
                return r;
            });
    if (r == CT_PARSER) {
        checkCmd = checkCmdSubs;
        return r;
    }
    // Split args
    checkCmdArgs = splitArgs(checkCmdSubs);
    QString ccBase = QFileInfo(checkCmdArgs.first()).baseName();

    if (pProtoService != nullptr && protoService().getName() == _sSsh) {
        QString sUser = feature(_sUser);
        QString sHost = node().getIpAddress(q).toString();
        if (!sUser.isEmpty()) sHost = sUser + "@" + sHost;
        checkCmdArgs.prepend(sHost);
        checkCmdArgs.prepend(_sSsh);
    }
    checkCmd = checkCmdArgs.takeFirst();    // Split command and arguments

    QString myBase = QFileInfo(QCoreApplication::arguments().at(0)).baseName();
#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX))
    enum Qt::CaseSensitivity cs = Qt::CaseSensitive;
#else
    enum Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#endif
    if (0 == myBase.compare(ccBase, cs)) {
        if (pService->getName() != _sLv2d) {   // Az lv2d hívhatja önmagát
            // Önmagunk hívása lenne!
            EXCEPTION(ECONTEXT, 0, name());
        }
    }
    bool isFullPath;
#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX))
    isFullPath = checkCmd.startsWith(QChar('/'));
#else
    QRegularExpression fpp("[a-z,A-Z]:\\\\");
    isFullPath = 0 == checkCmd.indexOf(fpp);
#endif
    if (isFullPath) {
        QFileInfo   fcmd(checkCmd);
        if (!fcmd.isExecutable()) {
            EXCEPTION(ENOTFILE, -1, tr("Ismeretlen %1 parancs a %2 -ben").arg(checkCmd, name()));
        }
    }
    else {
#ifdef Q_OS_WIN
        if (!checkCmd.endsWith(".exe", Qt::CaseInsensitive)) checkCmd += ".exe";
#endif
        QFileInfo   fcmd(checkCmd);
        if (!fcmd.isExecutable()) { // A saját nevén ill. az aktuális mappában nincs.
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
            if (!fcmd.isExecutable()) EXCEPTION(ENOTFILE, -1, tr("Ismeretlen %1 parancs a %2 -ben").arg(checkCmd, name()));  // nem volt a path-on sem
        }
        checkCmd = fcmd.absoluteFilePath();
    }
    _DBGFNL() << "checkCmd = " << quotedString(checkCmd) <<
                 (checkCmdArgs.isEmpty() ? " ARGS()"
                                         : " ARGS(\"" + checkCmdArgs.join("\", \"") + "\")")
              << endl;
    return r;
}

void cInspector::timerEvent(QTimerEvent *)
{
    _DBGFN() << objectName() << endl;
    // Check state
    if (internalStat == IS_DOWN || internalStat == IS_STOPPED) {
        _DBGFNL() << objectName() << "Skip, stat is" << internalStatName() << endl;
        return;
    }
    if (internalStat != IS_SUSPENDED && internalStat != IS_OMITTED) {
        QString msg = tr("%1  skip %2, internalStat = %3").arg( __PRETTY_FUNCTION__, name(), internalStatName());
        PDEB(WARNING) << msg << endl;
        if (internalStat != IS_INIT) {      // Ha dbNotif-al leállította, és utána kiad egy tick-et, akkor ez van. Debug ok, de nem kell memo.
            APPMEMO(*pQuery(), msg, RS_WARNING);
        }
        return;
    }
    // timeperiods ?
    if (!timeperiod._isEmptyRec()) {
        if (!timeperiod.isOnTime(*pQuery())) {
            QDateTime now = QDateTime::currentDateTime();
            int t = int(now.msecsTo(timeperiod.nextOnTime(*pQuery(), now)));
            internalStat = IS_OMITTED;
            timerStat    = TS_OMMIT;
            PDEB(VERBOSE) << objectName() << " set ommited. " << VDEB(t) << endl;
            QObject *po = stopTimer();
            if (po != nullptr) timerId = po->startTimer(t);
            if (0 == timerId) EXCEPTION(EPROGFAIL, retryInt, tr("Timer %1 not restarted.").arg(po == nullptr ? _sNULL : po->objectName()));
            _DBGFNL() << QString("Set omitted, wait %1 mSec").arg(t) << endl;
            return;
        }
    }

    internalStat = IS_RUN;
    if (inspectorType & IT_TIMING_PASSIVE) {
        if (pSubordinates != nullptr) {
            int n = 0;  // Hány alárendelt fut még?
            int maxState = RS_UNKNOWN;
            int minState = RS_ON;
            foreach (cInspector * pSub, *pSubordinates) {
                if (pSub != nullptr
                 && (pSub->internalStat == IS_RUN           // Éppen fut
                  || pSub->internalStat == IS_SUSPENDED     // várakozik: timer
                  || pSub->internalStat == IS_OMITTED)) {   // várakozik: timeperiod
                    ++n;
                    int state = int(pSub->hostService.getId(_sHostServiceState));
                    if (minState < state) minState = state;
                    if (maxState > state) maxState = state;
                }
            }
            if (!n) {
                PDEB(INFO) << tr("Passive sevice down : no any subservice.") << endl;
                deleteLater();
                return;
            }
            QString msg = tr("Runing sub services : %1/%2; states : %3 - %4")
                    .arg(n).arg(pSubordinates->size())
                    .arg(notifSwitch(maxState), notifSwitch(minState));
            PDEB(INFO) << tr("Passive sevice touch : ") << msg << endl;
            hostService.setState(*pQuery(), _sOn, msg);
            manageInterval(false);
        }
        internalStat = IS_SUSPENDED;
        return;
    }
    if (!isTimedType()) EXCEPTION(EPROGFAIL, inspectorType, name());
    if (isThreadType() && isMainThread()) EXCEPTION(EPROGFAIL, inspectorType, name());
    bool statSetRetry = doRun(true);
    if (pProcess == nullptr || !pProcess->isAsync) {
        // normal/retry intervallum kezelése
        manageInterval(statSetRetry);
    }
    DBGFNL();
}

void cInspector::manageInterval(bool retry, int offset)
{
    _DBGFN() << VDEB(retry) << VDEB(offset) << " timerStat = " << ::timerStat(timerStat) << endl;
    if (interval != retryInt) { // csak akkor ha különböző, akkor kell váltani
        int hard = notifSwitch(hostService.getName(_sHardState));
        int soft = notifSwitch(hostService.getName(_sSoftState));
        // Ha a soft status rosszabb mint a hard status, és nem RS_ON vagy RS_RECOVERED, akkor retry interval
        if (retry || (soft >= RS_WARNING && hard < soft)) {
            if (timerStat != TS_RETRY) {
                toRetryInterval(offset);
            }
        }
        else {
            if (timerStat != TS_NORMAL) {
                toNormalInterval(offset);
            }
        }
    }
    else if (timerStat == TS_FIRST) {
        toNormalInterval(offset);  // Ha az első esetleg túl rövid, ne legyen felesleges event.
    }
    if (internalStat == IS_RUN) internalStat = IS_SUSPENDED;
    _DBGFNL() << " timerStat = " << ::timerStat(timerStat) << endl;
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
    // Tesszük a dolgunkat bármi legyen is az. Ha engedélyezett, akkor tranzakcióban.
    QString tn = toSqlName(name());
    bool isTransaction = 0 != (inspectorType & IT_AUTO_TRANSACTION);
    try {
        if (isTransaction) sqlBegin(*pQuery(), tn);
        retStat      = run(*pQuery(), statMsg);
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
        if (isTransaction) sqlRollback(*pQuery(), tn);  // Hiba volt, inkább visszacsináljuk az egészet.
        if (pProcess != nullptr && QProcess::NotRunning != pProcess->state()) {
            pProcess->kill();
        }
        // A hibárol LOG az adatbázisba, amugy töröljük a hibát
        lanView  *plv = lanView::getInstance();
        qlonglong id = sendError(lastError);
        if (plv->lastError == lastError) plv->lastError = nullptr;
        pDelete(lastError);
        statMsg = msgCat(statMsg, tr("Hiba, ld.: app_errs.applog_id = %1").arg(id));
        internalStat = IS_STOPPED;
        try {
            setState(*pQuery(), _sUnreachable, statMsg, parentId(EX_IGNORE));
        }  catch (...) {
            PDEB(DERROR) << name() << " : " << statMsg << endl;
            internalStat = IS_ERROR;
        }
    }
    else {
        if (isTransaction) sqlCommit(*pQuery(), tn);
        if (retStat < RS_WARNING
         && ((interval > 0 && lastRun.hasExpired(interval)))) { // Ha ugyan nem volt hiba, de sokat tököltünk
            statMsg = msgCat(statMsg, tr("Időtúllépés, futási idö %1 ezred másodperc").arg(elapsed));
            setState(*pQuery(), notifSwitch(RS_WARNING), statMsg, parentId(EX_IGNORE));
        }
        else if (!statIsSet) {   // Ha még nem volt status állítás
            statMsg = msgCat(statMsg, tr("Futási idő %1 ezred másodperc").arg(elapsed));
            setState(*pQuery(), notifSwitch(retStat), statMsg, parentId(EX_IGNORE));
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
        if (ec < 0) {   // A parancs futtatása sikertelen
            return RS_STAT_SETTED;    // Already sended: RS_CRITICAL, or is running
        }

        return parse(ec, *pProcess, runMsg);
    }
    else if (inspectorType & IT_METHOD_PARSE_CMD) {
        return parse_cmd(q, runMsg);
    }
    else {
        runMsg = tr("Incomplete cInspector (%1) object. No activity specified.").arg(name());
        PDEB(VVERBOSE) << runMsg << endl;
        internalStat = isTimedType() ? IS_SUSPENDED : IS_STOPPED;
    }
    return RS_UNREACHABLE;
}

int cInspector::parse_cmd(QSqlQuery& q, QString &runMsg)
{
    (void)q;
    if (pQparser == nullptr) {
        for (cInspector *pPar = pParent; pPar != nullptr; pPar = pPar->pParent) {
            pQparser = pPar->pQparser;
            if (pQparser != nullptr) break;     // Megtaláltuk
        }
    }
    if (pQparser == nullptr) {
        runMsg = tr(
                    "A '%1' parancs nem hajtható végre.\n"
                    "A %2 szolgáltatás példány kontexusa hibás\n"
                    "Nem qparse típus, és nincs qparse típusú szülő sem!"
                         ).arg(checkCmd, name());
        return RS_UNREACHABLE;
    }
    cError *pe = pQparser->execParserCmd(checkCmd, this);
    runMsg = pQparser->exportText();
    if (pe == nullptr) {
        return RS_ON;
    }
    else {
        msgAppend(&runMsg, tr("********* DEBUG *********"));
        foreach (QString s, pQparser->debugLines()) {
            msgAppend(&runMsg, s);
        }
        msgAppend(&runMsg, tr("********* ERROR *********"));
        msgAppend(&runMsg, pe->msg());
        return RS_UNREACHABLE;
    }
}

int cInspector::parse(int _ec, QIODevice& _text, QString& runMsg)
{
    _DBGFN() <<  name() << endl;
    int r = RS_INVALID;
    if (inspectorType & IT_METHOD_CARRIED) {
        r = RS_STAT_SETTED;
        if (_ec != 0) r |= RS_SET_RETRY;
        return r;
    }
    QByteArray bytes = _text.readAll();
    QString text = QString::fromUtf8(bytes);
    PDEB(VVERBOSE) << "Parsing text : \n" << text << endl;
    int method = inspectorType & IT_METHOD_MASK;
    if (method == 0) {  // Nincs előírás
        msgAppend(&runMsg, tr("Return code %1").arg(_ec));
        msgAppend(&runMsg, text);
        r = _ec == 0 ? RS_ON : RS_CRITICAL;
    }
    else {
        if (method & IT_METHOD_SAVE_TEXT) {
            r = save_text(_ec, text);
        }
        if (method & IT_METHOD_PARSE_TEXT) {
            r = parse_text(_ec, text, runMsg);
        }
        switch (method & IT_METHODE_TEXT_DATA) {
        case IT_METHOD_NAGIOS:  r = parse_nagios(_ec, text, runMsg);    break;
        case IT_METHOD_JSON:    r = parse_json(_ec, bytes, runMsg);     break;
        }
        if (method & IT_METHOD_QPARSE) r = parse_qparse(_ec, text);
    }
    _DBGFNL() << r << endl;
    return r;
}

enum eNotifSwitch cInspector::parse_nagios(int _ec, QString &text, QString& runMsg)
{
    _DBGFN() <<  name() << endl;
    eNotifSwitch r = RS_DOWN;
    switch (_ec) {
    case NR_OK:         r = RS_ON;           break;
    case NR_WARNING:    r = RS_WARNING;      break;
    case NR_CRITICAL:   r = RS_CRITICAL;     break;
    case NR_UNKNOWN:    r = RS_UNREACHABLE;  break;
    }
    msgAppend(&runMsg, text);
    return r;
}

inline QJsonValue jsonDocToVal(const QJsonDocument& d)
{
    QJsonValue r;
    if      (d.isArray()) {
        QJsonArray a = d.array();
        r = QJsonValue(a);
    }
    else if (d.isObject()) {
        QJsonObject o = d.object();
        r = QJsonValue(o);
    }
    return r;
}

static inline void jsonDeb(cDebug::eMask m, const QString& key, QJsonValue& jv)
{
    if (cDebug::pDeb(m | __DMOD__)) {
        if (jv.type() == QJsonValue::Null) {
            cDebug::cout() << "JSON : " << quoted(key) << " = " << endl;
        }
        else {
            cDebug::cout() << "JSON : " << quoted(key) << " = " <<  debVariantToString(jv.toVariant()) << endl;
        }
    }
}

QJsonValue findJsonValue(const QJsonValue& o, const QString& key)
{
    QJsonValue r;
    if (o.isObject()) return o.toObject()[key];
    else if (o.isArray()) {
        QJsonArray ja = o.toArray();
        QJsonArray::iterator i, e = ja.end();
        for (i = ja.begin(); i < e; ++i) {
            QJsonValue v = *i;
            r = v.toObject()[key];
            if (!r.isNull()) break;
        }
    }
    jsonDeb(cDebug::INFO, key, r);
    return r;
}

static QJsonValue searchJsonValue(const QJsonValue& o, const QString& keys)
{
    QJsonValue r = o;
    QStringList ksl = keys.split('.', Q_SKIPEMPTYPARTS);
    foreach (QString key, ksl) {
        key = key.simplified();
        if (key.isEmpty()) continue;
        r = findJsonValue(r, key);
        if (r.isNull()) break;
    }
    return r;
}

enum eNotifSwitch cInspector::parse_json(int _ec, const QByteArray &text, QString &runMsg)
{
    _DBGFN() <<  name() << endl;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(text, &err);
    int st = _ec == 0 ? RS_ON : RS_WARNING;
    if (err.error != 0) {
        msgAppend(&runMsg, tr("JSON text :"));
        msgAppend(&runMsg, QString::fromUtf8(text));
        msgAppend(&runMsg, tr("Error #%1; %2").arg(err.error).arg(err.errorString()));
        return RS_CRITICAL;
    }
    QJsonValue jvBase = jsonDocToVal(doc);
    QString keys = feature("json_base");
    jvBase = searchJsonValue(jvBase, keys);
    for (tStringMap::const_iterator i = varsFeatureMap.constBegin(); i != varsFeatureMap.constEnd(); ++i) {
        const QString& vname = i.key();
        cInspectorVar& sv = *getInspectorVar(vname, EX_ERROR);
        keys = i.value();
        QJsonValue jv = searchJsonValue(jvBase, keys);
        QVariant v = jv.toVariant();
        sv.setValue(*pQuery(), v, st);
    }
    return eNotifSwitch(st);
}

enum eNotifSwitch cInspector::save_text(int _ec, const QString &text)
{
    _DBGFN() <<  name() << endl;
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
    pParam->replace(*pQuery());
    return _ec == 0 ? RS_ON : RS_WARNING;
}

enum eNotifSwitch cInspector::parse_text(int _ec, QString &text, QString &runMsg)
{
    _DBGFN() <<  name() << endl;
    int r = RS_ON;
    if (_ec != 0) {
        msgAppend(&runMsg, tr("Command return : %1.").arg(_ec));
        r = RS_WARNING;
    }
    if (isFeature("list")) {
        return parse_list(r, text, runMsg);
    }
    if (isFeature(_sTagged)) {
        return parse_tagged(r, text, runMsg);
    }
    msgAppend(&runMsg, tr("Parse text : Type is not defined."));
    return RS_UNREACHABLE;
}

enum eNotifSwitch cInspector::parse_list(int r, QString &text, QString &runMsg)
{
    _DBGFN() <<  name() << endl;
    QStringList values = text2list(text);
    if (values.isEmpty()) {
        msgAppend(&runMsg, tr("Parse text list: There is no value."));
        return RS_UNREACHABLE;
    }
    bool ok;
    QMapIterator<QString, QString> i(varsFeatureMap);
    while (i.hasNext()) {      i.next();
        const QString& vname = i.key();
        const QString& sIx   = i.value();
        int ix = sIx.toInt(&ok);
        if (!ok) {
            msgAppend(&runMsg, tr("Invalid variable index : %1.").arg(sIx));
            r = RS_UNREACHABLE;
            break;
        }
        --ix;   // 1,2,3,... --> 0,1,2,...
        if (!isContIx(values, ix)) {
            msgAppend(&runMsg, tr("Missing data, indexed : %1.").arg(ix));
            if (r == RS_ON) r = RS_WARNING;
            continue;
        }
        cInspectorVar& sv = *getInspectorVar(vname, EX_ERROR);
        sv.setValue(*pQuery(), values[ix], r);
    }
    return eNotifSwitch(r);

}

enum eNotifSwitch cInspector::parse_tagged(int r, QString &text, QString &runMsg)
{
    _DBGFN() <<  name() << endl;
    QStringList values = text2list(text);
    if (values.isEmpty()) {
        msgAppend(&runMsg, tr("Parse tagged text : There is no value."));
        return RS_UNREACHABLE;
    }
    tStringMap vmap;
    QString tagSeparator;
    if ((tagSeparator = feature("tag_separator")).isEmpty() == false) {
        foreach (QString s, values) {
            QStringList sl = s.split(tagSeparator);
            if (sl.size() < 2) continue;
            vmap[sl.first()] = sl.at(1);
        }
    }
    else if ((tagSeparator = feature("tag_regexp")).isEmpty() == false) {
        QRegularExpression tagRexp(tagSeparator);
        QRegularExpressionMatch ma;
        foreach (QString s, values) {
            int ix = s.indexOf(tagRexp, 0, &ma);
            if (ix < 0) continue;
            QStringList sl = ma.capturedTexts();
            if (sl.size() != 3) {
                msgAppend(&runMsg, tr("Invalid regular expression in tag_regexp : %1.").arg(tagSeparator));
                return RS_UNREACHABLE;
            }
            vmap[sl.at(1)] = sl.at(2);
        }
    }
    for (tStringMap::const_iterator i = varsFeatureMap.constBegin(); i != varsFeatureMap.constEnd(); ++i) {
        const QString& vname = i.key();
        QString sRef = i.value();       // Value name in query string
        if (sRef.isEmpty()) sRef = vname;
        tStringMap::iterator j = vmap.find(sRef);
        if (j == vmap.end()) {
            msgAppend(&runMsg, tr("Missing data : %1 (%2).").arg(vname, sRef));
            if (r == RS_ON) r = RS_WARNING;
            continue;
        }
        cInspectorVar& sv = *getInspectorVar(vname, EX_ERROR);
        sv.setValue(*pQuery(), j.value(), r);
    }
    return eNotifSwitch(r);
}

QStringList cInspector::text2list(QString &text)
{
    QString sSeparator = feature("separator");
    QString sRegexp;
    QStringList values;
    if (!sSeparator.isEmpty()) {
        values = text.split(sSeparator, Q_SKIPEMPTYPARTS);
    }
    else if (!(sRegexp = feature("regexp_separator")).isEmpty()) {
        values = text.split(QRegularExpression(sRegexp), Q_SKIPEMPTYPARTS);
    }
    else if (!(sRegexp = feature(_sRegexp)).isEmpty()) {
        QRegularExpression re(sRegexp);
        QRegularExpressionMatch ma;
        if (text.indexOf(re, 0, &ma) >= 0) {
            values = ma.capturedTexts().mid(1);
        }
    }
    else {
        QString line;
        QTextStream strm(&text, QIODevice::ReadOnly);
        while ((line = strm.readLine()).isEmpty() == false) values << line;
    }
    return values;
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
            if ((inspectorType & IT_METHODE_TEXT_DATA) == IT_METHOD_NAGIOS) {
                // If nagios plugin, then drop qparse, if not found parser. No warning.
                inspectorType &= ~IT_METHOD_QPARSE;
                return RS_DOWN;
            }
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
    QRegularExpression sep("[\r\n]+");
    QStringList sl = text.split(sep);
    foreach (QString t, sl) {
        t = t.simplified();
        if (t.isEmpty()) continue;      // Empty line
        if (!comment.isEmpty() && 0 == t.indexOf(comment)) continue;    // Comment line
        cError *pe = nullptr;
        int r = pQparser->captureAndParse(t, pe);
        // Match?
        if (r == R_NOTFOUND) {
            PDEB(VVERBOSE) << "DROP : " << quotedString(t) << endl;
            continue;  // No match, drop line
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

void cInspector::start()
{
    PDEB(INFO) << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
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
        int r = pQparser->load(*pQuery(), serviceId(), true);
        if (R_NOTFOUND == r && nullptr != pPrimeService) r = pQparser->load(*pQuery(), primeServiceId(), true);
        if (R_NOTFOUND == r && nullptr != pProtoService)     pQparser->load(*pQuery(), protoServiceId(), true);
        cError *pe = nullptr;
        pQparser->setInspector(this);
        pQparser->prep(pe);
        if (nullptr != pe) pe->exception();
    }
    // Thread
    if (isThreadType()) {   // Saját szál?
        if (pInspectorThread == nullptr) EXCEPTION(EPROGFAIL, -1, QObject::tr("%1 pThread egy NULL pointer.").arg(name()));
        internalStat = IS_RUN;
        emitStartThread(IS_RUN);
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
    QObject *po = isThreadType() ? static_cast<QObject *>(pInspectorThread) : static_cast<QObject *>(this);
    if (isTimedType()) {
        pRunTimeVar = getInspectorVar(_sRuntime);
        if (pRunTimeVar == nullptr) {
            pRunTimeVar = new cInspectorVar(*pQuery(), this, _sRuntime, cServiceVarType::srvartype(*pQuery(), _sRuntime)->getId());
            pRunTimeVar->postInit(*pQuery());
            vars() << pRunTimeVar;
        }
        internalStat = IS_SUSPENDED;
        startSubs();
        qlonglong t = firstDelay();
        PDEB(VERBOSE) << QString("Start timer %1 ").arg(t) << name() << " / " << po->objectName() << endl;
        timerId = po->startTimer(int(t));
        if (0 == timerId) EXCEPTION(EPROGFAIL, interval, tr("Timer not started."));
        _DBGFNL() << " (timed) " << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    // Passive
    if (passive()) {
        if (inspectorType & IT_SUPERIOR && timingType() == IT_TIMING_PASSIVE) {
            PDEB(VERBOSE) << "Start (passive)" << name() << " timer " << interval << "ms"
                          << " object thread : " << thread()->objectName() << endl;
            PDEB(VERBOSE) << QString("Start timer %1 ").arg(interval) << name() << " / " << po->objectName() << endl;
            timerId   = po->startTimer(interval);
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
    if (inspectorType & IT_TIMING_POLLING) deleteLater();
    _DBGFNL() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
    return;
}

void cInspector::stop()
{
    PDEB(VERBOSE) << name() << endl;
    if (timerId > 0) {
        stopTimer();
    }
    if (pInspectorThread != nullptr) {
        pInspectorThread->pThread->quit();
        pInspectorThread->abortFlag = true;
        pInspectorThread->pThread->wait(stopTimeOut);
    }
    else if (pProcess != nullptr) {
        pProcess->terminate();
        if (!pProcess->waitForFinished(stopTimeOut)) {
            pProcess->kill();
        }
    }
    else if (pSubordinates != nullptr) {
        foreach (cInspector *pSub, *pSubordinates) {
            if (pSub != nullptr) pSub->stop();
        }
    }
    internalStat = IS_STOPPED;
}

QObject * cInspector::stopTimer()
{
    // A timer tulajdonosa
    QObject *po = (pInspectorThread != nullptr) ? static_cast<QObject *>(pInspectorThread) : static_cast<QObject *>(this);
    // a megfelelő szálban vagyunk ?
    if (isCurrentThread(po)) {
        if (timerId > 0) {
            po->killTimer(timerId);
            timerId = -1;
        }
        return po;
    }
    /* --------------------------------------------------------------------------------------------- */
    // Ha nem fut a timer, akkor vége, nem kell a szál pointere sem.
    if (timerId <= 0) return nullptr;
    bool ok = false;
    eTristate invoke = TS_NULL;
    // Nem azonos a szál, az inspector thread-é a timer, nem kéne abban a szálban lenni, mert akkor valami nem stimmel.
    // Ha nem fut a thread, akkor feleslegesen strapáljuk magunkat.
    if (pInspectorThread != nullptr) {
        if (!isCurrentThread(pInspectorThread) && pInspectorThread->pThread->isRunning()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            ok = QMetaObject::invokeMethod(pInspectorThread, &cInspectorThread::on_thread_killTimer);
#else
            ok = QMetaObject::invokeMethod(pInspectorThread, "on_thread_killTimer", Qt::QueuedConnection);
#endif
            invoke = bool2ts(ok);
        }
    }
    // Mélyebben van a thread, már abbol indított inspector, és a gyökérből próbálljuk leállítani.
    else {
        PDEB(VVERBOSE) << "Find ..." << endl;
        cInspector *pp = pParent;
        while (pp != nullptr) {
            // Ha megvan a legközelebbi thread ős
            if (pp->pInspectorThread != nullptr) {
                // Az inspector ebben a szálban van, és fut a szál?
                if (pp->pInspectorThread->pThread == thread() && pp->pInspectorThread->pThread->isRunning()) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
                    ok = QMetaObject::invokeMethod(this, &cInspector::do_killTimer);
#else
                    ok = QMetaObject::invokeMethod(this, "do_killTimer", Qt::QueuedConnection);
#endif
                    invoke = bool2ts(ok);
                }
                break;
            }
            pp = pp->pParent;
        }
    }
    switch (invoke) {
    case TS_TRUE: {
        // Ha elküldtük a jelet a thread-nek, és sikerült.
        // Várunk max 5 másodpercet, 200msec-enként, a timerId törlésére
        int cnt = 25;
        QEventLoop e;
        do {
            QTimer::singleShot(200, &e, &QEventLoop::quit);
            e.exec();
        } while (timerId > 0 && --cnt);
        if (timerId > 0) {
            DERR() << "Invoked on_thread_killTimer(), timer is not stopped." << endl;
        }
      } break;
    case TS_FALSE:
        DERR() << "Nothing timer : inspector thread : " << (pInspectorThread == nullptr ? _sNULL : pInspectorThread->objectName())
               << "; current thread : " << QThread::currentThread()->objectName() << endl;
    case TS_NULL:
        break;
    }
    return nullptr;
}

int cInspector::firstDelay()
{
    int t;
    QDateTime last;
    t = getTimeout(features(), "delay", 0);
    if (t <= 0) {    // Ha nincs magadva késleltetés
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
    if (!timeperiod._isEmptyRec()) {   // timeperiod ?
        QSqlQuery q = getQuery();
        QDateTime now  = QDateTime::currentDateTime();
        QDateTime next = now.addMSecs(t);
        if (!timeperiod.isOnTime(q, next)) {
            t = now.msecsTo(timeperiod.nextOnTime(q, next));
        }
    }
    if (t < MINIMUM_INERVAL) t = MINIMUM_INERVAL;
    timerStat = TS_FIRST;
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
            threadChkAbort();
            cInspector *p = *i;
            if (p != nullptr) {
                p->start();
            }
        }
    }
}

void cInspector::setState(QSqlQuery& __q, const QString& __st, const QString& __note, qlonglong __did, bool _resetIfDeleted)
{
    hostService.setState(__q, __st, __note, __did, _resetIfDeleted);
    if (pRunTimeVar != nullptr) {
        int dumy;
        pRunTimeVar->setValue(__q, QVariant(lastRun.elapsed()), dumy);
    }
}

void cInspector::toRetryInterval(int offset)
{
    _DBGFN() << VDEB(offset) << endl;
    if (timerStat == TS_RETRY) return;
    timerStat = TS_RETRY;
    int t = retryInt;
    if (offset < retryInt && (offset * 10) > retryInt) {
        timerStat = TS_FIRST;
        t -= offset;
    }
    if (t < MINIMUM_INERVAL) t = MINIMUM_INERVAL;
    QObject *po = stopTimer();
    if (po != nullptr) timerId = po->startTimer(t);
    if (0 >= timerId) EXCEPTION(EPROGFAIL, retryInt, tr("Timer not started."));
}

void cInspector::toNormalInterval(int offset)
{
    _DBGFN() << VDEB(offset) << endl;
    if (timerStat == TS_NORMAL) return;
    timerStat = TS_NORMAL;
    int t = interval;
    if (offset < interval && (offset * 10) > interval) {
        timerStat = TS_FIRST;
        t -= offset;
    }
    if (t < MINIMUM_INERVAL) t = MINIMUM_INERVAL;
    QObject *po = stopTimer();
    if (po != nullptr) timerId = po->startTimer(t);
    if (0 == timerId) EXCEPTION(EPROGFAIL, interval, tr("Timer not started."));
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

cInspectorVar *cInspector::getInspectorVar(const QString& _name, eEx __ex)
{
    cInspectorVar *r = nullptr;
    if (pVars != nullptr && !pVars->isEmpty()) {
        foreach (cInspectorVar *p, *pVars) {
            if (p->pSrvVar->getName() == _name) {
                r = p;
                break;
            }
        }
    }
    if (r == nullptr && __ex != EX_IGNORE) {
        EXCEPTION(EDATA, 0, tr("Variable %1 not found in %2 ,").arg(_name, name()));
    }
    return r;
}

QSqlQuery *cInspector::pQuery()
{
    if (thread() == QThread::currentThread()) {
        if (pQMain == nullptr) pQMain = newQuery();
        return pQMain;
    }
    else if (pInspectorThread != nullptr && pInspectorThread->thread() == QThread::currentThread()) {
        if (pQThread == nullptr) pQThread = newQuery();
        return pQThread;
    }
    EXCEPTION(EPROGFAIL, 0, "Unknown thread : " + QThread::currentThread()->objectName());
}

/* ---------------------------- Thread ------------------------------ */

cInspectorThread *cInspector::newThread()
{
    _DBGFN() << name() << endl;
    cInspectorThread *p = nullptr;
    p = new cInspectorThread(this);
    return p;
}

void cInspector::threadInit(cInspectorThread *p)
{

    QThread *pThread = p->pThread;
    p->moveToThread(pThread);
    if (p->thread() != pThread) {
        EXCEPTION(EPROGFAIL, hostServiceId(), objectName() + " : move to thread");
    }
    if (!connect(pThread, &QThread::finished, this, &cInspector::on_thread_finished)) {
        EXCEPTION(EPROGFAIL, hostServiceId(), objectName() + " : connect finished()");
    }
}

void cInspector::emitStartThread(int s)
{
    if (pInspectorThread == nullptr || pInspectorThread->pThread == nullptr) EXCEPTION(EPROGFAIL);
    QThread *pThread = pInspectorThread->pThread;
    if (pThread->isRunning()) {
        EXCEPTION(EPROGFAIL, s, name() + " Thread is already runing.");
    }
    pInspectorThread->internalStat = eInternalStat(s);
    pThread->start();
    QEventLoop loop(this);
    QTimer::singleShot(200, &loop, SLOT(quit()));
    loop.exec();
}

void cInspector::threadPostInit()
{
    return;
}

void cInspector::threadChkAbort()
{
    if (pInspectorThread != nullptr) {
        if (isCurrentThread(pInspectorThread)) {
            pInspectorThread->chkAbort();
        }
    }
}

void cInspector::do_killTimer()
{
    if (timerId > 0) {
        killTimer(timerId);
        timerId = -1;
    }
}

void cInspector::on_thread_finished()
{
    _DBGFN() << name() << endl;
    cDebug::flushAll();
    if (pInspectorThread->internalStat == IS_RUN) {
        if (internalStat == IS_ERROR) {
            PDEB(VERBOSE) << "down : " << name() << endl;
            deleteLater();
        }
    }
}

void cInspector::do_timerEvent()
{
    QTimerEvent e(0);
    _DBGFN() << name() << endl;
    timerEvent(&e);
    _DBGFNL() << name() << endl;
}

void cInspector::do_reset_sub(cInspector *psi, bool _start)
{
    _DBGFN() << name() << " ==> " << psi->name() << endl;
    int ix;
    if (pSubordinates != nullptr && (ix = pSubordinates->indexOf(psi)) >= 0) {
        PDEB(INFO) << "ReSet : " << psi->name() << endl;
        qlonglong hsid = psi->hostServiceId();
        qlonglong toid = psi->host().tableoid();
        QSqlQuery& q = *pQuery();
        if (!_start) {
            // Set state immediate down
            psi->hostService.setState(q, _sDown, tr("Stopped."), NULL_ID, false, true);
        }
        (*pSubordinates)[ix] = nullptr;
        delete psi;
        cInspector *pi = newSubordinate(q, hsid, toid, this);
        if (pi != nullptr) {
            if (_start) {
                pi->postInit(*pQuery());
                pi->start();
            }
            (*pSubordinates)[ix] = pi;
        }
    }
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
    case IS_EXIT:       return _sExit;
    case IS_RESTART:    return "restart";
    }
    return QString("Invalid(%1)").arg(is, 0, 16);
}

QString timerStat(int ts)
{
    switch (ts) {
    case TS_STOP:   return "stop";
    case TS_NORMAL: return "normal";
    case TS_RETRY:  return "retry";
    case TS_FIRST:  return "first";
    case TS_OMMIT:  return "ommit";
    }
    return QString("Invalid(%1)").arg(ts, 0, 16);
}
/* *************************************************************************************** */

QBitArray cInspectorVar::updateMask;
QBitArray cInspectorVar::readBackMask;
QString cInspectorVar::sInvalidValue;
QString cInspectorVar::sNotCredible;
QString cInspectorVar::sFirstValue;
QString cInspectorVar::sRawIsNull;

cInspectorVar::cInspectorVar(QSqlQuery& _q, qlonglong _id, cInspector * pParent)
{
    _init();
    pInspector = pParent;
    pSrvVar    = cServiceVar::getVarObjectById(_q, _id);
    // Csak a releváns mezők visszaolvasása az update hívásnál
    pSrvVar->_toReadBack   = RB_MASK;
    pSrvVar->_readBackMask = readBackMask;
}

cInspectorVar::cInspectorVar(QSqlQuery& _q, cInspector * pParent, const QString& __name, qlonglong _stid, eEx __ex)
{
    _init();
    pInspector = pParent;
    qlonglong hsid = pInspector->hostServiceId();
    pSrvVar = cServiceVar::getVarObjectByName(_q, __name, hsid, EX_IGNORE);
    if (pSrvVar != nullptr) {
        qlonglong stid = pSrvVar->getId(_sServiceVarTypeId);
        if (_stid != NULL_ID && stid != _stid) {
            if (__ex != EX_IGNORE) EXCEPTION(EDATA, 0, QObject::tr("Different type ID (%1 - %2), for %1 variable.").arg(stid).arg(_stid).arg(__name));
            pDelete(pSrvVar);
        }
    }
    else {  // Not found. Create by postInit()
        if (_stid == NULL_ID) {
            if (__ex != EX_IGNORE) EXCEPTION(EDATA, 0, QObject::tr("Missing type ID, for %1 variable.").arg(__name));
            pDelete(pSrvVar);
        }
        else {
            // A post init kreálja meg a rekordot.
            // Automatikusan mindíg a cServiceVar jön létre, később konvertálható.
            pSrvVar = new cServiceVar;
            pSrvVar->setName(__name);
            pSrvVar->setId(_sHostServiceId, hsid);
            pSrvVar->setId(_sServiceVarTypeId, _stid);
        }
    }
    if (pSrvVar != nullptr) {
        // Csak a releváns mezők visszaolvasása az update hívásnál
        pSrvVar->_toReadBack   = RB_MASK;
        pSrvVar->_readBackMask = readBackMask;
    }
}

cInspectorVar::~cInspectorVar()
{
    pDelete(pSrvVar);
    pDelete(pEnumVals);
    pDelete(pMergedFeatures);
    pDelete(pTypeMergedFeatures);
    pDelete(pVarType);
}

void cInspectorVar::_init()
{
    pMergedFeatures = nullptr;
    pTypeMergedFeatures = nullptr;
    pEnumVals = nullptr;
    pVarType = nullptr;
    lastCount = 0;
    skeepCnt = 0;
    pSrvVar  = nullptr;
    pVarType = nullptr;
    if (updateMask.isEmpty()) {
        cServiceVar o;
        updateMask = o.mask(o.ixLastTime(), o.ixRawValue())
                   | o.mask(o.ixVarState(), o.ixServiceVarValue(), o.ixStateMsg());
        readBackMask = o.mask(o.ixLastTime()) | o.mask(_sDisabled);
        sInvalidValue = QObject::tr("Value is invalid : %1");
        sNotCredible  = QObject::tr("Data is not credible.");
        sFirstValue   = QObject::tr("First value, no data.");
        sRawIsNull    = QObject::tr("Raw value is NULL");
    }
}

bool cInspectorVar::postInit(QSqlQuery& _q)
{
    if (pSrvVar == nullptr) {
        EXCEPTION(EPROGFAIL, 0, QObject::tr("pSrvVar is NULL."));
    }
    if (pInspector == nullptr) {
        EXCEPTION(EPROGFAIL, 0, QObject::tr("pInspector is NULL."));
    }
    if (pInspector->pMergedFeatures == nullptr) {
        EXCEPTION(EPROGFAIL, 0, QObject::tr("pInspector->pMergedFeatures is NULL. Invalid '%1' cInspectror object.").arg(pInspector->name()));
    }
    if (pSrvVar->isNullId()) {   // Exists ?
        pSrvVar->_toReadBack = RB_YES;
        if (!pSrvVar->insert(_q)) { // Create
            EXCEPTION(EPROGFAIL, 0, QObject::tr("Failed to insert service variable . Service name : %1, variable : %2.").arg(pInspector->name(), pSrvVar->getName()));
        }
        pSrvVar->_toReadBack = RB_MASK;
    }
    pDelete(pVarType);
    pVarType = new cServiceVarType(pSrvVar->varType(_q));

    // feature, merge
    pDelete(pMergedFeatures);
    pDelete(pTypeMergedFeatures);
    pMergedFeatures     = new cFeatures(pSrvVar->features());
    pTypeMergedFeatures = new cFeatures(pVarType->features());
    pMergedFeatures    ->merge(*pInspector->pMergedFeatures, "service_var." + pSrvVar->getName());
    pTypeMergedFeatures->merge(            *pMergedFeatures, "service_var_type");
    PDEB(VVERBOSE) << "InspectorVar mergedFeatures : "     << pMergedFeatures->join()     << endl;
    PDEB(VVERBOSE) << "InspectorVar typeMergedFeatures : " << pTypeMergedFeatures->join() << endl;
    // Modify pVarType by features
    QList<QString> fns = {  // A modosítható mezők nevei
        _sPlausibilityType, _sPlausibilityParam1, _sPlausibilityParam2, _sPlausibilityInverse,
        _sWarningType,      _sWarningParam1,      _sWarningParam2,     _sWarningInverse,
        _sCriticalType,     _sCriticalParam1,     _sCriticalParam2,    _sCriticalInverse
    };
    for (QString& key: fns) {
        QMap<QString, QString>::const_iterator i = pTypeMergedFeatures->constFind(key);
        if (i != pTypeMergedFeatures->constEnd()) {
            PDEB(VVERBOSE) << pInspector->name() << '/' << pSrvVar->getName() << '/' << pVarType->getName() << '.' << key <<  " Modify to " << i.value() << endl;
            pVarType->set(key, i.value());
        }
    }
    //
    heartbeat = pInspector->hostService.getId(_sHeartbeatTime);
    if (heartbeat <= 0) heartbeat = pInspector->interval * 2;
    if (heartbeat <= 0) {
        const cInspector *pParInsp = pInspector->pParent;
        // Parent is not NULL, and cInspector object is not temporary
        if (pParInsp != nullptr && pParInsp != pInspector) {
            heartbeat = pParInsp->hostService.getId(_sHeartbeatTime);
            if (heartbeat <= 0) heartbeat = pParInsp->interval * 2;
        }
    }
    bool ok;
    rarefaction = feature(_sRarefaction).toInt(&ok);
    if (!ok || rarefaction <= 0) rarefaction = int(pSrvVar->getId(pSrvVar->ixRarefaction()));
    if (heartbeat > 0 && rarefaction > 1) heartbeat *= rarefaction;
    skeepCnt = rarefaction / 2;
    return true;
}


int cInspectorVar::setValue(QSqlQuery& q, const QVariant& _rawVal, int& state)
{
    if (skeep()) return ENUM_INVALID;
    // Type of raw data
    cParamType cptRaw = pSrvVar->rawDataType(q);
    eParamType ptRaw = eParamType(cptRaw.getId(cParamType::ixParamTypeType()));
    eTristate rawChanged = preSetValue(q, ptRaw, _rawVal, state);
    if (rawChanged == TS_NULL) return RS_UNREACHABLE;
    bool ok = true;
    switch (ptRaw) {
    case PT_INTEGER: {
        qlonglong rwi = _rawVal.toLongLong(&ok);
        if (ok) return setValue(q, rwi, state, rawChanged);
    }   break;
    case PT_REAL: {
        double    rwd = _rawVal.toDouble(&ok);
        if (ok) return setValue(q, rwd, state, rawChanged);
    }   break;
    case PT_INTERVAL: {
        qlonglong rwi;
        if (variantIsInteger(_rawVal)) rwi = _rawVal.toLongLong(&ok);
        else                           rwi = parseTimeInterval(_rawVal.toString(), &ok);
        if (ok) return setValue(q, rwi, state, rawChanged);
    }   break;
    case PT_TEXT:
        if (nullptr != enumVals()) {   // ENUM?
            qlonglong i = enumVals()->indexOf(_rawVal.toString());
            return updateEnumVar(q, i, state);
        }
        break;
    default:
        break;
    }
    if (!ok) {
        addMsg(sInvalidValue.arg(debVariantToString(_rawVal)));
        return noValue(q, state);
    }
    int rs;
    if (rawChanged == TS_TRUE) {
        eParamType ptVal = eParamType(pSrvVar->dataType(q).getId(cParamType::ixParamTypeType()));
        rs = postSetValue(q, ptVal, _rawVal, RS_ON, state);
    }
    else {
        pSrvVar->touch(q);
        rs = int(pSrvVar->getId(pSrvVar->ixVarState()));
    }
    return rs;
}

int cInspectorVar::setValue(QSqlQuery& q, double val, int& state, eTristate rawChg)
{
    if (skeep()) return ENUM_INVALID;
    int rpt = int(pSrvVar->rawDataType(q).getId(_sParamTypeType));
    // Check raw type
    if (rpt == PT_INTEGER || rpt == PT_INTERVAL) {
        return setValue(q, qlonglong(val + 0.5), state, rawChg);
    }
    if (rpt != PT_REAL) {    // supported types
        addMsg(QObject::tr("Invalid context : Non numeric raw data type %1.").arg(paramTypeType(rpt)));
        noValue(q, state);
        return RS_UNREACHABLE;
    }
    eTristate changed = TS_NULL;
    switch (rawChg) {
    case TS_NULL:
        changed = preSetValue(q, rpt, QVariant(val), state);
        if (changed == TS_NULL) return RS_UNREACHABLE;
        break;
    case TS_TRUE:
    case TS_FALSE:
        changed = rawChg;
        break;
    }
    // Var preprocessing methode
    qlonglong svt = pVarType->getId(_sServiceVarType);
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DCOUNTER:
        return setCounter(q, qlonglong(val + 0.5), int(svt), state);
    case SVT_DERIVE:
    case SVT_DDERIVE:
         return setDerive(q, val, state);
    }
    /* Ez több bajt okoz, nem jó ötlet
    int rs = int(pSrvVar->getId(_sVarState));
    if (changed == TS_FALSE && rs != RS_UNKNOWN) {
        pSrvVar->touch(q);
        return rs;
    }
    */
    int rs = RS_INVALID;
    if (svt == SVT_ABSOLUTE) {
        if (val < 0) {
            val = - val;
        }
    }
    QString rpn = feature(_sRpn);
    if (!rpn.isEmpty()) {
        QString err;
        PDEB(VVERBOSE) << VDEB(val) << VDEB(rpn) << endl;
        if (!rpn_calc(val, rpn, err)) {
            addMsg(QObject::tr("RPN error : %1").arg(err));
            rs = RS_UNKNOWN;
            postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
            return rs;
        }
        PDEB(VVERBOSE) << VDEB(val) << endl;
    }
    int vpt = int(pSrvVar->dataType(q).getId(_sParamTypeType));
    switch (svt) {
    case SVT_ABSOLUTE:
    case NULL_ID:
    case SVT_GAUGE: {
        switch (vpt) {
        case PT_INTEGER:    rs = updateVar(q, qlonglong(val + 0.5), state);   break;
        case PT_REAL:       rs = updateVar(q, val,                  state);   break;
        case PT_INTERVAL:   rs = updateIntervalVar(q, qlonglong(val), state); break;
        default:
            addMsg(QObject::tr("Unsupported service Variable data type : %1").arg(paramTypeType(rpt)));
            rs = RS_UNKNOWN;
            postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
            return rs;
        }
    }   break;
    default:
        addMsg(QObject::tr("Unsupported service Variable type : %1").arg(serviceVarType(int(svt))));
        rs = RS_UNKNOWN;
        postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
    }
    return rs;
}

int cInspectorVar::setValue(QSqlQuery& q, qlonglong val, int &state, eTristate rawChg)
{
    if (skeep()) return ENUM_INVALID;
    int rpt = int(pSrvVar->rawDataType(q).getId(_sParamTypeType));
    // Check raw type
    if (rpt == PT_REAL) {
        return setValue(q, double(val), state, rawChg);
    }
    if (rpt != PT_INTEGER && rpt != PT_INTERVAL) {    // supported types
        addMsg(QObject::tr("Invalid context : Non numeric raw data type %1.").arg(paramTypeType(rpt)));
        noValue(q, state);
        return RS_UNREACHABLE;
    }
    eTristate changed = TS_NULL;
    switch (rawChg) {
    case TS_NULL:
        changed = preSetValue(q, rpt, QVariant(val), state);
        if (changed == TS_NULL) return RS_UNREACHABLE;
        break;
    case TS_TRUE:
    case TS_FALSE:
        changed = rawChg;
        break;
    }
    // Var preprocessing methode
    qlonglong svt = pVarType->getId(_sServiceVarType);
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DCOUNTER:
    case SVT_DERIVE:
    case SVT_DDERIVE:
         return setCounter(q, val, int(svt), state);
    }
    /* Ez több bajt okoz, nem jó ötlet
    int rs = int(pSrvVar->getId(_sVarState));
    if (changed == TS_FALSE && rs != RS_UNKNOWN) {
        pSrvVar->touch(q);
        return rs;
    }
    */
    int rs = RS_INVALID;
    double d = val;
    if (svt == SVT_ABSOLUTE) {
        if (val < 0) {
            val = - val;
            d   = - d;
        }
    }
    QString rpn = feature(_sRpn);
    if (!rpn.isEmpty()){
        QString err;
        if (!rpn_calc(d, rpn, err)) {
            addMsg(QObject::tr("RPN error : %1").arg(err));
            rs = RS_UNKNOWN;
            postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
            return rs;
        }
        val = qlonglong(d + 0.5);
    }
    const cParamType& pt = pSrvVar->dataType(q);
    int vpt = int(pt.getId(cParamType::ixParamTypeType()));
//    int vpt = int(dataType(q).getId(_sParamTypeType));
    switch (svt) {
    case SVT_ABSOLUTE:
    case NULL_ID:
    case SVT_GAUGE: {
        switch (vpt) {
        case PT_INTEGER:    return updateVar(q, val,         state);
        case PT_REAL:       return updateVar(q, d,           state);
        case PT_INTERVAL:   return updateIntervalVar(q, val, state);
        case PT_TEXT:
            if (nullptr != enumVals()) {   // ENUM ?
                return updateEnumVar(q, val, state);
            }
        }
        addMsg(QObject::tr("Invalid context. Unsupported service Variable data type : %1").arg(paramTypeType(rpt)));
        rs = RS_UNKNOWN;
        postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
    }   break;
    default:
        addMsg(QObject::tr("Unsupported service Variable type : %1").arg(serviceVarType(int(svt))));
        rs = RS_UNKNOWN;
        postSetValue(q, ENUM_INVALID, QVariant(), rs, state);
    }
    return rs;
}

int cInspectorVar::setUnreachable(QSqlQuery q, const QString& msg)
{
    static QBitArray aSet = pSrvVar->mask(pSrvVar->ixVarState(), pSrvVar->ixLastTime(), pSrvVar->ixStateMsg());
    pSrvVar->setName(pSrvVar->ixVarState(), _sUnreachable);
    pSrvVar->setName(pSrvVar->ixLastTime(), _sNOW);
    pSrvVar->setName(pSrvVar->ixStateMsg(), msg);
    return pSrvVar->update(q, false, aSet);
}

const QStringList * cInspectorVar::enumVals()
{
    if (pEnumVals == nullptr) {
        pEnumVals = new QStringList(cFeatures::value2list(feature(_sEnumeration)));
    }
    return pEnumVals;
}


int cInspectorVar::setValue(QSqlQuery& q, cInspector *pInsp, const QString& _name, const QVariant& val, int& state)
{
    int rs;
    int r;
    cInspectorVar *p = pInsp->getInspectorVar(_name);
    r = p->setValue(q, val, rs);
    if (p->pSrvVar->getBool(_sDelegateServiceState) && state < rs) state = rs;
    if (rs > state) state = rs;
    return r;
}


int cInspectorVar::setValues(QSqlQuery& q, qlonglong hsid, const QStringList& _names, const QVariantList& vals)
{
    cInspector insp(q, hsid);
    insp.pParent = &insp;       // Indicates a temporary object. If pParent is NULL, the destructor calls the exit () method.
    insp.splitFeature();
    int n = _names.size();
    n = llMin(n, vals.size());
    int r = RS_ON, st, dummy = RS_UNKNOWN;
    for (int i = 0; i < n; ++i) {
        cInspectorVar var(q, &insp, _names.at(i));
        var.postInit(q);
        st = var.setValue(q, vals.at(i), dummy);
        r = llMax(r, st);
    }
    return r;
}

int cInspectorVar::setValues(QSqlQuery& q, cInspector *pInsp, const QStringList& _names, const QVariantList& vals, int& state)
{
    int r = RS_ON;
    int n = _names.size();
    n = llMin(n, vals.size());
    for (int i = 0; i < n; ++i) {
        int rr = setValue(q, pInsp, _names.at(i), vals.at(i), state);
        if (r < rr) r = rr;
    }
    return r;
}

eTristate cInspectorVar::preSetValue(QSqlQuery& q, int ptRaw, const QVariant& rawVal, int& state)
{
    now = QDateTime::currentDateTime();
    pSrvVar->clear(pSrvVar->ixStateMsg());
    if (rawVal.isNull()) {
        addMsg(QObject::tr("Raw data is NULL."));
        noValue(q, state);
        return TS_NULL;
    }
    bool ok;
    QString s = cParamType::paramToString(eParamType(ptRaw), rawVal, EX_IGNORE, &ok);
    if (!ok) {
        addMsg(QObject::tr("Conversion of raw data (%1) failed.").arg(debVariantToString(ptRaw)));
        postSetValue(q, ENUM_INVALID, QVariant(), RS_UNREACHABLE, state);
        return TS_NULL;
    }
    // a változásra nem kellene vizsgálni, csak a NULL-ra,
    bool changed = s.compare(pSrvVar->getName(pSrvVar->ixRawValue()));
    if (changed) pSrvVar->setName(pSrvVar->ixRawValue(), s);
    lastLast = pSrvVar->get(pSrvVar->ixLastTime()).toDateTime();
    pSrvVar->set(pSrvVar->ixLastTime(), now);
    return bool2ts(changed); // Raw value is changed ?
}

bool cInspectorVar::postSetValue(QSqlQuery& q, int ptVal, const QVariant& val, int rs, int& state)
{
    if (ptVal == ENUM_INVALID || val.isNull()) {
        pSrvVar->clear(pSrvVar->ixServiceVarValue());
    }
    else {
        bool ok = true;
        pSrvVar->setName(pSrvVar->ixServiceVarValue(), cParamType::paramToString(eParamType(ptVal), val, EX_IGNORE, &ok));
        if (!ok) {
            rs = RS_UNREACHABLE;
            addMsg(QObject::tr("Conversion of target data (%1) failed.").arg(debVariantToString(ptVal)));
        }
    }
    if (pSrvVar->getBool(_sDelegateServiceState) && state < rs) state = rs;
    pSrvVar->setId(pSrvVar->ixVarState(), rs);
    return 1 == pSrvVar->update(q, false, updateMask);
}

int cInspectorVar::setCounter(QSqlQuery& q, qlonglong val, int svt, int &state)
{
    if (!lastTime.isValid()) {
        lastCount = val;
        lastTime  = now;
        addMsg(sFirstValue);
        return noValue(q, state, ENUM_INVALID);
    }
    qlonglong delta = 0;
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DERIVE:    // A számláló tulcsordulás (32 bit)
        if (lastCount > val) {
            addMsg(QObject::tr("A számláló túlcsordult."));
            delta = val + 0x100000000LL - lastCount;
            if (delta > 0x100000000LL) {
                lastCount = val;
                lastTime  = now;
                addMsg(QObject::tr("Többszörös túlcsordulás?"));
                return noValue(q, state);
            }
            break;
        }
        delta = val - lastCount;
        break;
    case SVT_DDERIVE:   //
    case SVT_DCOUNTER:
        delta = val - lastCount;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    lastCount = val;
    double dt = double(lastTime.msecsTo(now)) / 1000;
    lastTime = now;
    double dVal = double(delta) / dt;
    return updateVar(q, dVal, state);
}

int cInspectorVar::setDerive(QSqlQuery &q, double val, int& state)
{
    QDateTime now = QDateTime::currentDateTime();
    if (!lastTime.isValid()) {
        lastValue = val;
        lastTime  = now;
        addMsg(sFirstValue);
        return noValue(q, state, ENUM_INVALID);
    }
    double delta = val - lastValue;
    lastValue = val;
    double dt = double(lastTime.msecsTo(now)) / 1000;
    lastTime = now;
    double dVal = delta / double(dt);
    return updateVar(q, dVal, state);
}


int cInspectorVar::updateIntervalVar(QSqlQuery& q, qlonglong val, int &state)
{
    int rs = RS_ON;
    if (TS_TRUE != checkIntervalValue(val, pVarType->getId(_sPlausibilityType), pVarType->getName(_sPlausibilityParam1), pVarType->getName(_sPlausibilityParam2), pVarType->getBool(_sPlausibilityInverse), true)) {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    if (TS_TRUE == checkIntervalValue(val, pVarType->getId(_sCriticalType), pVarType->getName(_sCriticalParam1), pVarType->getName(_sCriticalParam2), pVarType->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkIntervalValue(val, pVarType->getId(_sWarningType), pVarType->getName(_sWarningParam1), pVarType->getName(_sWarningParam2), pVarType->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    postSetValue(q, PT_INTERVAL, val, rs, state);
    return rs;
}

int cInspectorVar::updateVar(QSqlQuery& q, qlonglong val, int &state)
{
    if (TS_TRUE != checkIntValue(val, pVarType->getId(_sPlausibilityType), pVarType->getName(_sPlausibilityParam1), pVarType->getName(_sPlausibilityParam2), pVarType->getBool(_sPlausibilityInverse), true)) {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkIntValue(val, pVarType->getId(_sCriticalType), pVarType->getName(_sCriticalParam1), pVarType->getName(_sCriticalParam2), pVarType->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkIntValue(val, pVarType->getId(_sWarningType), pVarType->getName(_sWarningParam1), pVarType->getName(_sWarningParam2), pVarType->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    postSetValue(q, PT_INTEGER, val, rs, state);
    return rs;
}

int cInspectorVar::updateVar(QSqlQuery& q, double val, int& state)
{
    if (TS_TRUE != checkRealValue(val, pVarType->getId(_sPlausibilityType), pVarType->getName(_sPlausibilityParam1), pVarType->getName(_sPlausibilityParam2), pVarType->getBool(_sPlausibilityInverse), true)) {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkRealValue(val, pVarType->getId(_sCriticalType), pVarType->getName(_sCriticalParam1), pVarType->getName(_sCriticalParam2), pVarType->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkRealValue(val, pVarType->getId(_sWarningType), pVarType->getName(_sWarningParam1), pVarType->getName(_sWarningParam2), pVarType->getBool(_sWarningInverse))) {
        rs = RS_WARNING;
    }
    postSetValue(q, PT_REAL, val, rs, state);
    return rs;
}

int cInspectorVar::updateEnumVar(QSqlQuery& q, qlonglong i, int& state)
{
    int ix = int(i);
    if (pEnumVals == nullptr) EXCEPTION(EPROGFAIL);
    const QStringList& evals = *pEnumVals;
    if (!isContIx(evals, i) || evals[ix] == "!") {
        addMsg(sNotCredible);
        return noValue(q, state);
    }
    int rs = RS_ON;
    if (TS_TRUE == checkEnumValue(ix, evals, pVarType->getId(_sCriticalType), pVarType->getName(_sCriticalParam1), pVarType->getName(_sCriticalParam2), pVarType->getBool(_sCriticalInverse))) {
        rs = RS_CRITICAL;
    }
    else if (TS_TRUE == checkEnumValue(ix, evals, pVarType->getId(_sWarningType), pVarType->getName(_sWarningParam1), pVarType->getName(_sWarningParam2), pVarType->getBool(_sWarningInverse))) {
        rs = RS_CRITICAL;
    }
    postSetValue(q, PT_TEXT, evals[ix], rs, state);
    return rs;
}


int cInspectorVar::noValue(QSqlQuery& q, int &state, int _st)
{
    if (heartbeat > 0 && lastLast.isValid()) { // Ha van türelmi idő, volt előző érték
        qlonglong dt  = lastLast.msecsTo(QDateTime::currentDateTime());
        if (heartbeat < dt) {
            _st = RS_UNREACHABLE;
            pSrvVar->setId(pSrvVar->ixVarState(), _st);
            pSrvVar->clear(pSrvVar->ixServiceVarValue());
            QString msg = pSrvVar->getName(pSrvVar->ixStateMsg());
            msgAppend(&msg, QObject::tr("Time out (%1 > %2).").arg(intervalToStr(dt), intervalToStr(heartbeat)));
            pSrvVar->setName(pSrvVar->ixStateMsg(), msg);
            pSrvVar->update(q, false, updateMask);
            if (pSrvVar->getBool(_sDelegateServiceState)) state = _st;
        }
    }
    // Ha nincs adat, és türelmi idő nem járt le (nincs türelmi idő, első alkalom) akkor nem csinálunk semmit
    return _st;
}

eTristate cInspectorVar::checkIntValue(qlonglong val, qlonglong ft, const QString &_p1, const QString &_p2, bool _inverse, bool ifNo)
{
    if (ft == NULL_ID || ft == FT_NO) return bool2ts(ifNo);
    bool ok1 = true, ok2 = true;
    qlonglong p1 = 0, p2 = 0;
    eTristate r = TS_NULL;
    // required parameters
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1, EX_IGNORE);
        if (r != TS_NULL) p1 = r;
        else              ok1 = false;
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toLongLong(&ok1);
        break;
    case FT_INTERVAL:
        p1 = _p1.toLongLong(&ok1);
        p2 = _p2.toLongLong(&ok2);
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(QObject::tr("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(QObject::tr("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_BOOLEAN:
            r = (val == 0) == (p1 == 0) ? TS_TRUE : TS_FALSE;
            break;
        case FT_EQUAL:
            r = (val == p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_LITLE:
            r = (val  < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (val  > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(QObject::tr("Invalid filter %1(%2) for integer type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;
}

eTristate cInspectorVar::checkRealValue(double val, qlonglong ft, const QString &_p1, const QString &_p2, bool _inverse, bool ifNo)
{
    if (ft == NULL_ID || ft == FT_NO) return bool2ts(ifNo);
    bool ok1 = true, ok2 = true;
    eTristate r = TS_NULL;
    double p1 = 0.0;
    double p2 = 0.0;
    // required parameters
    switch (ft) {
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toDouble(&ok1);
        break;
    case FT_INTERVAL:
        p1 = _p1.toDouble(&ok1);
        p2 = _p2.toDouble(&ok2);
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(QObject::tr("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(QObject::tr("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_LITLE:
            r = (val < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (val > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(QObject::tr("Invalid filter %1(%2) for real numeric type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;
}

eTristate cInspectorVar::checkIntervalValue(qlonglong val, qlonglong ft, const QString& _p1, const QString& _p2, bool _inverse, bool ifNo)
{
    if (ft == NULL_ID || ft == FT_NO) return bool2ts(ifNo);
    bool ok1 = true, ok2 = true;
    qlonglong p1 = 0, p2 = 0;
    eTristate r = TS_NULL;
    // required parameters
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1, EX_IGNORE);
        if (r != TS_NULL) p1 = r;
        else              ok1 = false;
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toLongLong(&ok1);
        if (!ok1) {
            p1 = parseTimeInterval(_p1, &ok1);
        }
        break;
    case FT_INTERVAL:
        p1 = _p1.toLongLong(&ok1);
        p2 = _p2.toLongLong(&ok2);
        if (!ok1) {
            p1 = parseTimeInterval(_p1, &ok1);
        }
        if (!ok2) {
            p2 = parseTimeInterval(_p2, &ok2);
        }
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(QObject::tr("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(QObject::tr("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_BOOLEAN:
            r = (val == 0) == (p1 == 0) ? TS_TRUE : TS_FALSE;
            break;
        case FT_EQUAL:
            r = (val == p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_LITLE:
            r = (val  < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (val  > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (val > p1 && val < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(QObject::tr("Invalid filter %1(%2) for interval type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;
}

eTristate cInspectorVar::checkEnumValue(int ix, const QStringList& evals, qlonglong ft, const QString& _p1, const QString& _p2, bool _inverse)
{
    if (ft == NULL_ID || ft == FT_NO) return TS_FALSE;
    bool ok1 = true, ok2 = true;
    int p1 = 0, p2 = 0;
    eTristate r = TS_NULL;
    // required parameters
    switch (ft) {
    case FT_BOOLEAN:
        r = str2tristate(_p1, EX_IGNORE);
        if (r != TS_NULL) p1 = r;
        else              ok1 = false;
        break;
    case FT_EQUAL:
    case FT_LITLE:
    case FT_BIG:
        p1 = _p1.toInt(&ok1);
        if (!ok1) {
            p1 = evals.indexOf(_p1);
            ok1 = p1 >= 0;
        }
        break;
    case FT_INTERVAL:
        p1 = _p1.toInt(&ok1);
        p2 = _p2.toInt(&ok2);
        if (!ok1) {
            p1 = evals.indexOf(_p1);
            ok1 = p1 >= 0;
        }
        if (!ok2) {
            p2 = evals.indexOf(_p2);
            ok2 = p2 >= 0;
        }
        break;
    }
    // Check parameter(s)
    if (!ok1) {
        addMsg(QObject::tr("Invalid param1 data %1 for %2 filter.").arg(_p1, filterType(int(ft))));
    }
    if (!ok2) {
        addMsg(QObject::tr("Invalid param2 data %1 for %2 filter.").arg(_p2, filterType(int(ft))));
    }
    // Check rules, if all parameters is OK.
    if (ok1 && ok2) {
        switch (ft) {
        case FT_BOOLEAN:
            r = (ix == 0) == (p1 == 0) ? TS_TRUE : TS_FALSE;
            break;
        case FT_EQUAL:
            r = (ix == p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_LITLE:
            r = (ix  < p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_BIG:
            r = (ix  > p1) ? TS_TRUE : TS_FALSE;
            break;
        case FT_INTERVAL:
            r = (ix > p1 && ix < p2) ? TS_TRUE : TS_FALSE;
            break;
        default:
            addMsg(QObject::tr("Invalid filter %1(%2) for interval type.").arg(filterType(int(ft), EX_IGNORE)).arg(ft));
            break;
        }
    }
    return _inverse ? inverse(r) : r;

}

bool   cInspectorVar::skeep()
{
    static const int ixDisabled = pSrvVar->toIndex(_sDisabled);
    if (pSrvVar->getBool(ixDisabled)) {
        PDEB(VERBOSE) << "Disabled " << pInspector->name() << " : " << pSrvVar->getName() << endl;
        return true;
    }

    if (pSrvVar->getId(pSrvVar->ixVarState()) >= RS_UNREACHABLE) return false;

    int svt = int(pVarType->getId(_sServiceVarType));
    switch (svt) {
    case SVT_COUNTER:
    case SVT_DCOUNTER:
    case SVT_DERIVE:
    case SVT_DDERIVE:
         if (!lastTime.isValid()) return false;
    }

    if (!lastTime.isValid()) lastTime = pSrvVar->get(pSrvVar->ixLastTime()).toDateTime();
    if (!lastTime.isValid()) return false;
    QDateTime now = QDateTime::currentDateTime();
    qlonglong dt = lastTime.msecsTo(now);
    if (dt > (heartbeat / 2)) return false;

    --skeepCnt;
    bool r = skeepCnt > 0;
    if (!r) {
        skeepCnt = rarefaction;
    }
    return r;
}

bool cInspectorVar::initSkeepCnt(int& delayCnt)
{
    if (rarefaction <= 1) return false;
    skeepCnt = (delayCnt % rarefaction) + 1;
    ++delayCnt;
    return true;
}

/// Reverse Polish Notation
bool cInspectorVar::rpn_calc(double& _v, const QString &_expr, QString& st)
{
    QStack<double>  stack;
    stack << _v;
    _v = 0.0;
    static const QRegularExpression sep(QString("\\s+"));
    QStringList tokens = _expr.split(sep);
    while (!tokens.isEmpty()) {
        QString token = tokens.takeFirst();
        bool ok;
        _v = token.toDouble(&ok);
        if (ok) {   // numeric
            stack.push(_v);
            continue;
        }
        int n = stack.size();
        if (token.size() == 1) {    // Character token
            if (n < 2) {
                st = QObject::tr("A binary operator %1 expects two parameters.").arg(token);
                return false;
            }
            // Binary operator:
            _v = stack.pop();
            static QString  ctokens = "+-*/";
            switch (ctokens.indexOf(token)) {
            case 0: stack.top() += _v;  break;
            case 1: stack.top() -= _v;  break;
            case 2: stack.top() *= _v;  break;
            case 3: stack.top() /= _v;  break;
            default:
                st = QObject::tr("One character token %1 unknown.").arg(token);
                return false;
            }
            continue;
        }
        if (token.startsWith(QChar('$'))) {     // parameter
            QString key = token.mid(1);
            if (key.isEmpty()) {
                st = QObject::tr("Empty feature name %1.").arg(key);
                return false;
            }
            QString val = (*pMergedFeatures)[key];
            if (key.contains(QChar('.'))) {
                QStringList keys = key.split(QChar('.'));
                QString key1 = keys.takeFirst();
                QString key2 = keys.join(QChar('.'));
                QString v;
                if      (0 == key1.compare(_sInspector,    Qt::CaseInsensitive)) v = (*pInspector->pMergedFeatures)[key2];
                else if (0 == key1.compare("host_service", Qt::CaseInsensitive)) v = pInspector->hostService.feature(key2);
                else if (0 == key1.compare(_sHost,         Qt::CaseInsensitive)) v = pInspector->host().feature(key2);
                else if (0 == key1.compare(_sService,      Qt::CaseInsensitive)) v = pInspector->service()->feature(key2);
                if (!v.isEmpty()) val = v;
            }
            if (val.isEmpty()) {
                st = QObject::tr("Unknown feature name %1.").arg(key);
                return false;
            }
            QStringList sl = val.split(sep);
            while (!sl.isEmpty()) tokens.prepend(sl.takeLast());
            continue;
        }
        enum eTokens { T_NULL, UT_NEG, UT_DROP, UT_DUP, UT_NOP, BT_SWAP };
#define UNARYTOK(s)     { #s, UT_##s, 1 }
#define BINARYTOK(s)    { #s, BT_##s, 2 }
        struct tTokens { QString name; int key; int ops; }
                       tokens[] = {
            UNARYTOK(NEG), UNARYTOK(DROP), UNARYTOK(DUP), UNARYTOK(NOP),
            BINARYTOK(SWAP),
            { "",  T_NULL, 0 }
        };
        tTokens *pTok;
        for (pTok = tokens; pTok->key != T_NULL; ++pTok) {
            if (pTok->name.compare(token, Qt::CaseInsensitive)) {
                break;
            }
        }
        if (pTok->key != T_NULL) {
            st = QObject::tr("Invalid tokan %1.").arg(token);
            return false;
        }
        if (pTok->ops < n) {
            st = QObject::tr("There is not enough operand (%1 instead of just %2) for the %3 token.").arg(pTok->ops).arg(n).arg(token);
            return false;
        }
        switch (pTok->key) {
        case UT_NEG:    stack.top() = - stack.top();    break;
        case UT_DROP:   stack.pop();                    break;
        case UT_DUP:    stack.push(stack.top());        break;
        case BT_SWAP:   _v = stack.pop(); std::swap(_v, stack.top()); stack.push(_v);    break;
        case UT_NOP:    break;
        }
    }
    if (stack.isEmpty()) {
        st = QObject::tr("No return value.");
        return false;
    }
    _v = stack.top();
    return true;
}


