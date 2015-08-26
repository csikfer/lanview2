#include "lv2d.h"
#include "time.h"
#define VERSION_MAJOR   0
#define VERSION_MINOR   99
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main(int argc, char *argv[])
{
    cLv2QApp app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = true;

    lv2d    mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }

    int r = app.exec();

    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);

}

lv2d::lv2d()
{
    pq = NULL;
    pSelf = NULL;
    if (lastError == NULL) {
        try {
            pq = newQuery();

            sqlBegin(*pq);
            insertStart(*pq);
            sqlEnd(*pq);

            if (subsDbNotif(QString(), false)) {
                connect(pDb->driver(), SIGNAL(notification(QString)), SLOT(dbNotif(QString)));
            }

            setup();
        } CATCHS(lastError)
    }
}

lv2d::~lv2d()
{
    down();
}

bool lv2d::uSigRecv(int __i)
{
    if (__i == SIGHUP) {
        if (pSelf != NULL && pSelf->internalStat == IS_RUN) {
            PDEB(INFO) << trUtf8("Sygnal : SIGHUP; reset ...") << endl;
            reSet();
        }
        else {
            PDEB(INFO) << trUtf8("Sygnal : SIGHUP; dropped ...") << endl;
        }
        return false;
    }
    return true;
}
void lv2d::dbNotif(QString __s)
{
    if (pSelf != NULL && pSelf->internalStat != IS_RUN) return;
    PDEB(INFO) << QString(trUtf8("EVENT FROM DB : %1")).arg(__s) << endl;
    reSet();
}

void lv2d::setup()
{
    sqlBegin(*pq);
    pSelf = new cSupDaemon(*pq, appName, *this);
    pSelf->postInit(*pq);   // A Start is itt van!!!
}

void lv2d::down()
{
    if (pSelf != NULL) {
        delete pSelf;
        pSelf = NULL;
    }
}

void lv2d::reSet()
{
    pSelf->setInternalStat(IS_REINIT);
    try {
        down();
        setup();
    } CATCHS(lastError)
    if (lastError != NULL) QCoreApplication::exit(lastError->mErrorCode);
}


cSupDaemon::cSupDaemon(QSqlQuery& q, const QString& __sn, lv2d &_mo) : cInspector(q, __sn), mo(_mo)
{
    inspectorType = IT_TIMING_TIMED;
}

cSupDaemon::~cSupDaemon()
{
    ;
}

void cSupDaemon::postInit(QSqlQuery &q, const QString &)
{
    // Az időzítéssek
    interval = variantToId(get(_sNormalCheckInterval), false, -1);
    retryInt = variantToId(get(_sRetryCheckInterval),  false, interval);
    if (interval <= 0&& isTimed()) {   // Időzített időzítés nélkül !
        EXCEPTION(EDATA, interval, QObject::trUtf8("%1 időzített lekérdezés, időzítés nélkül.").arg(name()));
    }

    QSqlQuery q2 = getQuery();
    pSubordinates = new QList<cInspector *>;
    // Nem csak a host-service fán közlekedik, hanem az összes démont összeszedi, és a hierarhiát igazítja ehhez.
    QString sql = QString(
            "SELECT hs.*, h.* "
                "FROM host_services AS hs "
                 "JOIN nodes AS h USING(node_id) "
                 "JOIN services AS s USING(service_id) "
                "WHERE ((hs.node_id = %1 "                      // A saját host
                  "AND   (s.features LIKE '%:daemon=%' "      // Csak azok amik a features-ben van daemon paraméter
                   "OR   hs.features LIKE '%:daemon=%') "
                  "AND   host_service_id <> %2) "               // saját rekord nem kell
                 "OR    hs.superior_host_service_id = %2) "     // Vagy a szabályosan linkelt példányok
                  "AND NOT s.disabled AND NOT hs.disabled "     // letiltottak nem kellenek
                  "AND NOT s.deleted  AND NOT hs.deleted "      // töröltek sem kellenek
                ).arg(nodeId()).arg(hostServiceId());
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first())   EXCEPTION(NOTODO);
    do {
        cInspector *p = new cDaemon(q, this);
        if (p->hostService.getId(_sSuperiorHostServiceId) != hostServiceId()) {
            p->hostService.setId(_sSuperiorHostServiceId, hostServiceId());
            p->hostService.update(q2, false, p->hostService.mask(_sSuperiorHostServiceId));
        }
        p->postInit(q2);    // Ő is elstartol !
        *pSubordinates << p;
    } while (q.next());
    mo.runingCnt = pSubordinates->count();
    PDEB(VERBOSE) << "Start timer in defailt thread..." << endl;
    timerId = startTimer(interval);
    timerStat = TS_NORMAL;
    PDEB(VERBOSE) << name() << " timer started, interval : " << interval << "ms" << endl;
    internalStat = IS_RUN;
}

void cSupDaemon::stop(bool)
{
    if (internalStat != IS_DOWN && internalStat != IS_REINIT) setInternalStat(IS_DOWN);
    _DBGFN() << name() << endl;
    if (timerId >= 0) killTimer(timerId);
    timerId = -1;
    _DBGFNL() << name() << endl;
}

enum eNotifSwitch cSupDaemon::run(QSqlQuery &q)
{
    bool to = false;
    QList<cInspector*>::Iterator i, n = pSubordinates->end();
    for (i = pSubordinates->begin(); i != n; ++i) {
        cDaemon& d = *(cDaemon *)(*i);
        if (d.crashCnt >  d.maxCrashCnt)    continue;           // Ennek már annyi
        // if (d.crashCnt == d.oldCrashCnt)    d.crashCnt = 0;     // Ha sokáig nincs új elszállás, nullázzuk ?
        switch (d.inspectorType & IT_DAEMON_MASK) {
        case IT_DAEMON_CONTINUE:
        case IT_DAEMON_RESPAWN:
            break;      // itt nincs értelme a time out figyelésnek.
        case IT_DAEMON_POLLING:
            if (d.lastRun.elapsed() > (d.interval * 2))   to = true;
            break;
        default:
            EXCEPTION(EPROGFAIL, d.inspectorType, d.name());
        }
    }
    if (to) hostService.setState(q, _sWarning, QObject::trUtf8("Idő tullépés. Több mint kétszeres késleltetés egy lekérdezésnél"));
    else    hostService.setState(q, _sOn, _sNul);
    return RS_STAT_SETTED;
}
/* */

int cDaemon::maxCrashCnt = 5;

cDaemon::cDaemon(QSqlQuery& q,cSupDaemon *par) : cInspector(q, NULL_ID, NULL_ID, par), process(this), actLogFile()
{
    inspectorType = IT_TIMING_TIMED;   // Ez nem feltétlenül igaz, de egyedileg van kezelve a kegtöbb dolog, és így nem akad ki nem értelmezhető érték miatt
    crashCnt = 0;
    oldCrashCnt = 0;
    maxArcLog   = 4;
    maxLogSize  = 10 * 1024 * 1204; // 10MiByte
    logNull     = false;
}


cDaemon::~cDaemon()
{
    DBGFN();
    stop(false);
}



void cDaemon::postInit(QSqlQuery &q, const QString &)
{
    getCheckCmd(q);
    PDEB(INFO) << "Init " << name() << " : " << checkCmd << checkCmdArgs.join(' ') << endl;
    process.setObjectName(name());
    process.setProcessChannelMode(QProcess::MergedChannels);    // stderr + stdout ==> stdout

    if (isFeature(_sLognull)) {
        logNull = true;
    }
    else {
        QString s;
        if ((s = feature(_sLogrot)).isEmpty() == false) {
            QStringList ss = s.split(QRegExp("\\s*[,;]\\s*"));
            int i;
            bool ok;
            if (ss.size() > 0 && ((i = ss.at(0).toInt(&ok)), ok)) {
                maxLogSize = i;
                if (ss.size() > 1 && ((i = ss.at(1).toInt(&ok)), ok)) {
                    maxArcLog = i;
                }
            }
        }
        actLogFile.setFileName(lanView::getInstance()->homeDir + "/log/" +  service().getName() + ".log");
        if (!actLogFile.open(QIODevice::Append | QIODevice::WriteOnly)) {
            EXCEPTION(EFOPEN, -1, actLogFile.fileName());
        }
    }

    connect(&process, SIGNAL(error(QProcess::ProcessError)),      SLOT(procError(QProcess::ProcessError)));
    connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(procFinished(int,QProcess::ExitStatus)));
    connect(&process, SIGNAL(started()),                          SLOT(procStarted()));
//  connect(&process, SIGNAL(readyReadStandardError()),           SLOT(procReadyStdErr()));
    connect(&process, SIGNAL(readyReadStandardOutput()),          SLOT(procReadyStdOut()));
    internalStat = IS_RUN;
    if ((inspectorType & IT_DAEMON_MASK) == IT_DAEMON_CONTINUE || (inspectorType & IT_DAEMON_MASK) == IT_DAEMON_RESPAWN) process.start(checkCmd, checkCmdArgs);
}

eNotifSwitch cDaemon::run(QSqlQuery& q)
{
    DBGFN();
    if (crashCnt > maxCrashCnt) return RS_DOWN;
    switch (inspectorType & IT_DAEMON_MASK) {
    case IT_DAEMON_POLLING:
        if (process.state() == QProcess::NotRunning) {
            process.start(checkCmd, checkCmdArgs);
        }
        else {
            hostService.setState(q,
                                 _sWarning,
                                 QString(QObject::trUtf8("Idő tullépés a %1 parancs futásánál.")).arg(checkCmd),
                                 ((lv2d *)lanView::getInstance())->pSelf->hostServiceId());
        }
        break;
    case IT_DAEMON_CONTINUE:
    case IT_DAEMON_RESPAWN:
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    DBGFNL();
    return RS_STAT_SETTED;
}

void cDaemon::stop(bool)
{
    _DBGFN() << name() << endl;
    if (internalStat != IS_DOWN && internalStat != IS_REINIT) setInternalStat(IS_DOWN);
    if (timerId >= 0) killTimer(timerId);
    timerId = -1;
    if (process.state() != QProcess::NotRunning) {
        PDEB(INFO) << "Terminate process " << name() << endl;
        process.terminate();
        if (!process.waitForFinished(10000)) {
            DWAR() << "Kill process " << name() << endl;
            process.kill();
        }
    }
    _DBGFNL() << name() << endl;
}

void cDaemon::procError(QProcess::ProcessError error)
{
    QString msg = QString(trUtf8("Service %1, Command : '%2'; error : %3")).arg(service().getName()).arg(checkCmd).arg(ProcessError2Message(error));
    DERR() << msg << endl;
    INSERROR(EPROCERR, (int)error, msg);
}

void cDaemon::procFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    _DBGFN() << name() << " @(" << exitCode << ", " << (exitStatus ? "normal exit" : "crashed") << ")" << endl;
    if (internalStat != IS_RUN) return;
    if (exitStatus ==  QProcess::CrashExit || (inspectorType & IT_DAEMON_MASK) == IT_DAEMON_CONTINUE) {
        crashCnt++;
        QString msg = QString(QObject::trUtf8("Service %1 : ")).arg(service().getName());
        if (exitStatus ==  QProcess::CrashExit) {
            msg += QString(QObject::trUtf8("A %1 program elszállt")).arg(quotedString(checkCmd));
        }
        else {
            msg += QString(QObject::trUtf8("A %1 program kilépett. Kilépési kód %2.")).arg(quotedString(checkCmd)).arg(exitCode);
        }
        if (crashCnt > maxCrashCnt) msg += QObject::trUtf8(" Leállítva!");
        DERR() << msg << endl;
        INSERROR(EPROCCRE, exitCode, msg);
        QSqlQuery *pq = ((lv2d *)lanView::getInstance())->pq;
        if (crashCnt > maxCrashCnt) {
            hostService.setState(*pq, _sUnknown, msg);
        }
        else {
            hostService.setState(*pq, _sWarning, msg);
            int& runingCnt = ((lv2d *)lanView::getInstance())->runingCnt;
            --runingCnt;
            if (runingCnt == 0) EXCEPTION(NOTODO, -1, QObject::trUtf8("Az összes hívott szolgáltatás elszállt!"))
            return;
        }
    }
    else {
        crashCnt = 0;
    }
    if (internalStat != IS_RUN) switch (inspectorType & IT_DAEMON_MASK) {
    case IT_DAEMON_CONTINUE:
    case IT_DAEMON_RESPAWN:
        PDEB(INFO) << "Restart process " << name() << endl;
        process.start(checkCmd, checkCmdArgs);
        break;
    case IT_DAEMON_POLLING:
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
}

void cDaemon::procStarted()
{
    PDEB(INFO) << "Started : " << service().getName() << endl;
}

void cDaemon::procReadyStdOut()
{
    if (logNull) {
        return;
    }
    actLogFile.write(process.readAllStandardOutput());
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

