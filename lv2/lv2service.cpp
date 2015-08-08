
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
        inspector.run(*inspector.pq);   // ??
    }
    inspector.moveToThread(qApp->thread());
    _DBGFNL() << inspector.name() << endl;
}

void cInspectorThread::timerEvent(QTimerEvent * e)
{
    _DBGFN() << inspector.name() << endl;
    inspector.timerEvent(e);
    _DBGFNL() << inspector.name() << endl;
}

// "inicializálatlan"! Az első alkalommal hívott konstruktor inicializálja: initStaric().
qlonglong cInspector::nodeOid = NULL_ID;
qlonglong cInspector::sdevOid = NULL_ID;

void cInspector::preInit()
{
    inspectorType= IT_UNKNOWN;
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
    setInspectorType();
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
    setInspectorType();
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
    pDelete(pQparser);
    pDelete(pq);
    pDelete(pPort);
    pDelete(pNode);
    pDelete(_pFeatures);
    inspectorType = IT_INVALID;
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
    DWAR() << "Called " << name() << " default dummy methode." << endl;
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
    if (pSubordinates != NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 pSubordinates pointer nem NULL!").arg(name()));
    if (pThread       != NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 pThread pointer nem NULL!").arg(name()));
    // Ha meg van adva az ellenörző parancs, akkor a QProcess objektum létrejozása
    if (!getCheckCmd(q).isEmpty()) {
        pProcess = new QProcess(this);
    }
    if (inspectorType == IT_INVALID) setInspectorType();
    PDEB(VERBOSE) << name() << " inspectorType = " << ::inspectorType(inspectorType) << endl;
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
    // Van superior, de nem custom vagy bármi más, Thread-nál a thread-ban kell
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
    static QString sql =
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
    else                       _pFeatures->clear();

    if (pProtoService != NULL) _pFeatures->split(pProtoService->getName(ixFeatures), __ex);
    if (pPrimeService != NULL) _pFeatures->split(pPrimeService->getName(ixFeatures), __ex);
    _pFeatures->split(service().  getName(ixFeatures), __ex);
    _pFeatures->split(hostService.getName(_sFeatures), __ex);
    return *_pFeatures;
}

int cInspector::setInspectorType()
{
    QString value;
    inspectorType = 0;
    value = features().value(_sTiming);
    if      (     value.isEmpty())                                inspectorType |= IT_TIMING_TIMED;
    else if (0 == value.compare(_sTimed,    Qt::CaseInsensitive)) inspectorType |= IT_TIMING_TIMED;
    else if (0 == value.compare(_sThread,   Qt::CaseInsensitive)) inspectorType |= IT_TIMING_THREAD;
    else if (0 == value.compare(_sPassive,  Qt::CaseInsensitive)) inspectorType |= IT_TIMING_PASSIVE;
    else if (0 == value.compare(_sPolling,  Qt::CaseInsensitive)) inspectorType |= IT_TIMING_POLLING;
//  else if (0 == value.compare(_sCustom,   Qt::CaseInsensitive)) inspectorType |= IT_TIMING_CUSTOM;
    else {
        if (value.contains(_sTimed, Qt::CaseInsensitive) && value.contains(_sThread, Qt::CaseInsensitive))
            inspectorType |= IT_TIMING_TIMEDTHREAD;
    }

    value = features().value(_sDaemon);
    if      (0 == value.compare(_sRespawn,  Qt::CaseInsensitive)) inspectorType |= IT_DAEMON_RESPAWN;
    else if (0 == value.compare(_sContinue, Qt::CaseInsensitive)) inspectorType |= IT_DAEMON_CONTINUE;
    else if (0 == value.compare(_sPolling,  Qt::CaseInsensitive)) inspectorType |= IT_DAEMON_POLLING;

    if (isFeature(_sSuperior))                                    inspectorType |= IT_SUPERIOR;

    value = features().value(_sMethod);
    if      (0 == value.compare(_sNagios,   Qt::CaseInsensitive)) inspectorType |= IT_METHOD_NAGIOS;
    else if (0 == value.compare(_sMunin,    Qt::CaseInsensitive)) inspectorType |= IT_METHOD_MUNIN;
    else if (0 == value.compare(_sCarried,  Qt::CaseInsensitive)) inspectorType |= IT_METHOD_CARRIED;
//  else if (0 == value.compare(_sCustom,   Qt::CaseInsensitive)) inspectorType |= IT_METHOD_CUSTOM;
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
    pNode = new cNode();
    pNode->fetchSelf(q);
    // És a host_services rekordot is előszedjük.
    hostService.fetchByIds(q, pNode->getId(), service().getId());
}

QString& cInspector::getCheckCmd(QSqlQuery& q)
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
    if (checkCmd.isEmpty()) return checkCmd;

    QString arg;
    QString::const_iterator i = checkCmd.constBegin();
    char separator = 0;
    bool prevSpace = false;
    while (i < checkCmd.end()) {
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
        return checkCmd;
    }

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
    return checkCmd;
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
    if (pq == NULL) pq = newQuery();
    // Tesszük a dolgunkat bármi legyen is az, egy tranzakció lessz.
    try {
        sqlBegin(*pq);
        retStat      = run(*pq);
        statIsSet    = retStat & RS_STAT_SETTED;
        statSetRetry = retStat & RS_SET_RETRY;
        retStat      = (enum eNotifSwitch)(retStat & RS_STAT_MASK);
    } CATCHS(lastError);
    if (lastError != NULL) {    // Ha hívtuk a run metódust, és dobott egy hátast
        sqlRollback(*pq, false);  // Hiba volt, inkább visszacsináljuk az egészet.
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
    DBGFN();
    (void)q;
    if (pProcess != NULL) {
        if (checkCmd.isEmpty()) EXCEPTION(EPROGFAIL);
        pProcess->start(checkCmd, checkCmdArgs, QIODevice::ReadOnly);
        if (!pProcess->waitForStarted()) {
            EXCEPTION(ETO, pProcess->error(), ProcessError2Message(pProcess->error()));
        }
        if (pProcess->waitForFinished()) {
            EXCEPTION(ETO, pProcess->error(), ProcessError2Message(pProcess->error()));
        }
        QByteArray out = pProcess->readAll();
        return parser(pProcess->exitCode(), out);
    }
    return RS_ON;
}

enum eNotifSwitch cInspector::parser(int _ec, QByteArray& text)
{
    (void)_ec; // !!!!!!!!!!!!!!!!!!!!!!! Ezzel is kellene kezdeni valamit ?
    if (0 == _sInferior.compare(feature(_sQparser), Qt::CaseInsensitive)) {
        cQueryParser *pQP = NULL;
        for (cInspector *pPar = pParent; pPar != NULL; pPar = pPar->pParent) {
            pQP = pPar->pQparser;
            if (pQP != NULL) break;
        }
        if (pQP == NULL) EXCEPTION(EDATA, -1, name());
        QString t(text);
        cError *pe;
        int r = pQP->parse(t, pe);
        if (r != REASON_OK) pe->exception();
        return RS_ON;
    }
    else {
        EXCEPTION(ENOTSUPP);
        return RS_CRITICAL;
    }
}

bool cInspector::threadPrelude(QThread &)
{
    _DBGFN() << name() << endl;
    if (pq == NULL) pq = newQuery();
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
    startSubs();
    internalStat = IS_RUN;
    return r;
}

void cInspector::start()
{
    _DBGFN() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
    if (internalStat != IS_INIT && internalStat != IS_REINIT)
        EXCEPTION(EDATA, (int)internalStat, QObject::trUtf8("%1 nem megfelelő belső állapot").arg(name()));
    if (timerId != -1)
        EXCEPTION(EDATA, timerId, QObject::trUtf8("%1 óra újra inicializálása.").arg(name()));
    if (pQparser != NULL)
        EXCEPTION(EPROGFAIL, -1, trUtf8("A %1-ben a parser objektum nem létezhet a start elött!").arg(name()));
    if (_sManager.compare(feature(_sQparser), Qt::CaseInsensitive)) {
        pQparser = new cQueryParser();
        int r = pQparser->load(*pq, serviceId(), true);
        if (R_NOTFOUND == r && NULL != pPrimeService) r = pQparser->load(*pq, primeServiceId(), true);
        if (R_NOTFOUND == r && NULL != pProtoService) r = pQparser->load(*pq, protoServiceId(), true);
        if (R_NOTFOUND == r) EXCEPTION(EFOUND);
        cError *pe = NULL;
        pQparser->prep(pe);
        if (NULL != pe) pe->exception();
    }
    if (isThread()) {
        if (pThread == NULL) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 pThread egy NULL pointer.").arg(name()));
        if (pThread->isRunning()) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 szál már el lett idítva.").arg(name()));
        if (pq != NULL) {
            delete pq;
            pq = NULL;
        }
        setParent(NULL);            // Nem lehet parentje, mert a moveToThread(); vinné azt is!
        moveToThread(pThread);
        if (thread() != pThread) EXCEPTION(EPROGFAIL, -1, QObject::trUtf8("%1 QObject::moveToThread() hívás sikertelen.").arg(name()));
        pThread->start();
        PDEB(VERBOSE) << name() << " thread started." << endl;
        _DBGFNL() << QChar(' ') << name() << " internalStat = " << internalStatName() << endl;
        return;
    }
    if (isTimed()) {
        qlonglong t = rnd(interval);
        PDEB(VERBOSE) << "Start timer " << interval << QChar('/') << t << "ms in defailt thread " << name() << endl;
        timerId = startTimer(t);
        timerStat = TS_FIRST;
        startSubs();
        internalStat = IS_RUN;
    }
    else if (needStart()) {
        (void)toRun(false);
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
    QString r;
    static const QString    qq("??");
    if (pNode != NULL && !node().isNull(node().nameIndex())) r = node().getName();
    else                                                     r = qq;
    r +=  QChar(':');
    if (pPort != NULL) r += nPort().getName() + QChar(':');
    if (pService != NULL) r += service().getName();
    else                  r += qq;
    // A prime és proto services-t még esetleg ki kéne írni...
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

QString inspectorType(int __t)
{
    QString r;
    bool ok = true;
    switch (__t & IT_TIMING_MASK) {
    case IT_TIMING_CUSTOM:      r = _sTiming + "=" + _sCustom;                      break;
    case IT_TIMING_TIMED:       r = _sTiming + "=" + _sTimed;                       break;
    case IT_TIMING_THREAD:      r = _sTiming + "=" + _sThread;                      break;
    case IT_TIMING_TIMEDTHREAD: r = _sTiming + "=" + _sTimed + _sCommaSp + _sThread;break;
    case IT_TIMING_PASSIVE:     r = _sTiming + "=" + _sPassive;                     break;
    case IT_DAEMON_POLLING:     r = _sTiming + "=" + _sPolling;                     break;
    default:                    ok = false;                                         break;
    }
    switch (__t & IT_DAEMON_MASK) {
    case IT_NO_DAEMON:                                                              break;
    case IT_DAEMON_RESPAWN:     r += ":" + _sDaemon + "=" + _sRespawn;              break;
    case IT_DAEMON_CONTINUE:    r += ":" + _sDaemon + "=" + _sContinue;             break;
    case IT_DAEMON_POLLING:     r += ":" + _sDaemon + "=" + _sPolling;              break;
    default:                    ok = false;                                         break;
    }
    if (__t & IT_SUPERIOR)      r += ":" + _sSuperior;
    switch (__t & IT_METHOD_MASK) {
    case IT_METHOD_CUSTOM:                                                          break;
    case IT_METHOD_NAGIOS:      r += ":" + _sMethod + "=" + _sNagios;               break;
    case IT_METHOD_MUNIN:       r += ":" + _sMethod + "=" + _sMunin;                break;
    case IT_METHOD_CARRIED:     r += ":" + _sMethod + "=" + _sCarried;              break;
    }
    if (ok) return r;
    return _sUnknown + "(" + r + ")";
}

