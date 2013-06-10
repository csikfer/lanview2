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
    QCoreApplication app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = true;
    lanView::gui        = false;

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
        } catch(cError * e) {
            lastError = e;
        } catch(...) {
            lastError = NEWCERROR(EUNKNOWN);
        }
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
    } catch(cError * e) {
        lastError = e;
    } catch(...) {
        lastError = NEWCERROR(EUNKNOWN);
    }
    if (lastError != NULL) QCoreApplication::exit(lastError->mErrorCode);
}


cSupDaemon::cSupDaemon(QSqlQuery& q, const QString& __sn, lv2d &_mo) : cInspector(q, __sn), mo(_mo)
{
    inspectorType = IT_TIMED;
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
    if (interval > 0) { // Átváltás sec -> msec, ha van mit
        interval *= 1000;
        retryInt *= 1000;
    }
    else if (isTimed()) {   // Időzített időzítés nélkül !
        EXCEPTION(EDATA, interval, QObject::trUtf8("Időzített lekérdezés, időzítés nélkül."));
    }

    QSqlQuery q2 = getQuery();
    pSubordinates = new QList<cInspector *>;
    // Nem a host-service fán közlekedik, hanem az összes démont összeszedi, és a hierarhiát igazítja ehhez.
    QString sql = QString(
            "SELECT host_services.*, hosts.* "
                "FROM host_services JOIN hosts USING(node_id) JOIN services USING(service_id) "
                "  WHERE host_services.node_id = %1 AND services.properties LIKE '%:daemon=%' AND host_service_id <> %2"
                ).arg(nodeId()).arg(hostServiceId());
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first()) EXCEPTION(NOTODO);
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
        switch (d.daemonType) {
        case DT_CONTINUE:
        case DT_RESPAWN:
            break;      // itt nincs értelme a time out figyelésnek.
        case DT_POLLING:
            if (d.lastRun.elapsed() > (d.interval * 2))   to = true;
            break;
        default:
            EXCEPTION(EPROGFAIL, (int)d.daemonType);
        }
    }
    if (to) hostService.setState(q, _sWarning, QObject::trUtf8("Idő tullépés. Több mint kétszeres késleltetés egy lekérdezésnél"));
    else    hostService.setState(q, _sOn, _sNul);
    return RS_STAT_SETTED;
}
/* */

int cDaemon::maxCrashCnt = 5;

cDaemon::cDaemon(QSqlQuery& q,cSupDaemon *par) : cInspector(q, NULL_ID, NULL_ID, par), cmd(), process(this), actLogFile()
{
    inspectorType = IT_TIMED;   // Ez nem feltétlenül igaz, de egyedileg van kezelve a kegtöbb dolog, és így nem akad ki nem értelmezhető érték miatt
    crashCnt = 0;
}


cDaemon::~cDaemon()
{
    DBGFN();
    stop(false);
}



void cDaemon::postInit(QSqlQuery &, const QString &)
{
    QString s = magicParam(_sDaemon);
    if      (s.compare(_sRespawn, Qt::CaseInsensitive)) daemonType = DT_RESPAWN;
    else if (s.compare(_sPolling, Qt::CaseInsensitive)) daemonType = DT_POLLING;
    else                                                daemonType = DT_CONTINUE;
    getCmd();
    PDEB(INFO) << "Init " << name() << " : " << cmd << endl;
    process.setObjectName(name());
    process.setProcessChannelMode(QProcess::MergedChannels);    // stderr + stdout ==> stdout

    actLogFile.setFileName(lanView::getInstance()->homeDir + "/log/" +  service().getName() + ".log");
    if (!actLogFile.open(QIODevice::Append | QIODevice::WriteOnly)) {
        EXCEPTION(EFOPEN, -1, actLogFile.fileName());
    }

    connect(&process, SIGNAL(error(QProcess::ProcessError)),      SLOT(procError(QProcess::ProcessError)));
    connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(procFinished(int,QProcess::ExitStatus)));
    connect(&process, SIGNAL(started()),                          SLOT(procStarted()));
//  connect(&process, SIGNAL(readyReadStandardError()),           SLOT(procReadyStdErr()));
    connect(&process, SIGNAL(readyReadStandardOutput()),          SLOT(procReadyStdOut()));
    internalStat = IS_RUN;
    if (daemonType == DT_CONTINUE || daemonType == DT_RESPAWN) process.start(cmd);
}

static QString getParName(QString::Iterator& i, const QString::Iterator& e)
{
    QString r;
    ++i;
    if (i == e) return r;
    if (*i == QChar('{')) {
        for (++i; i != e && *i != QChar('}'); ++i) r += *i;
        if (i == e) return QString();
        ++i;
        return r;
    }
    for (; i != e; ++i) {
        QChar c = *i;
        if (c.isLetterOrNumber() || c == QChar('.') || c == QChar('_')) {
            r += c;
        }
        break;
    }
    return r;
}

QString cDaemon::getParValue(const QString& n)
{
    QStringList sl = n.split(QChar('.'));
    QString m;
    if (sl.size() > 1) {
        if (sl.size() > 2) EXCEPTION(EDATA, 1, n);
        QString m = sl.first();
        if (m == _sHostServices) return hostService.getName(sl[1]);
        if (m == _sServices)     return service().getName(sl[1]);
        if (m == _sNodes)        return pNode->getName(sl[1]);
        EXCEPTION(EDATA, 2, n);
    }
    else {
        if (hostService.isIndex(n)) return hostService.getName(n);
        if (  service().isIndex(n)) return   service().getName(n);
        if (     pNode->isIndex(n)) return      pNode->getName(n);
        if (n == "parent_id")       return QString::number(((lv2d *)lanView::getInstance())->pSelf->hostServiceId());
        EXCEPTION(EDATA, 3, n);
    }
    return QString();
}

void cDaemon::getCmd()
{
    DBGFN();
    QChar   c('$');
    cmd = get(_sCheckCmd).toString();
    QString::Iterator i;
    if (cmd.contains(c)) {
        QString s = cmd;
        cmd.clear();
        for (i = s.begin(); i != s.end(); ++i) {
            if (*i != c) {
                cmd += *i;
                continue;
            }
            QString n = getParName(i, s.end());
            cmd += getParValue(n);
        }
    }
    if (cmd.isEmpty()) EXCEPTION(EDATA, -1, trUtf8("Insufficient command"));
    QString cmdFile;
    for (i = cmd.begin(); i != cmd.end() && !i->isSpace(); ++i) cmdFile += *i;
    if (i == cmd.end()) cmd.clear();    // maradék a paraméterek
    else                cmd.mid(i - cmd.begin());
    QFileInfo   f(cmdFile);
    if (f.isExecutable()) {
 //       if (f.path().isEmpty()) {
 //           QDir    d(lanView::getInstance()->homeDir);
 //           f.setFile(d, cmdFile);
 //       }
    }
    else {    // A saját nevén (ha full path) ill. az aktiv directoriban nincs.
        QString pathEnv(qgetenv("PATH"));
        QStringList path;
#ifdef Q_OS_WIN
        QChar   sep(';');
#else
        QChar   sep(':');
#endif
        path << lanView::getInstance()->binPath.split(sep) << pathEnv.split(sep);    // saját path, és a PATH -ban lévő path-ok
        foreach (QString dir, path) {
            QDir d(dir);
            f.setFile(d, cmdFile);
            if (f.isExecutable()) break;      // megtaláltuk
        }
        if (!f.isExecutable()) EXCEPTION(ENOTFILE, -1, cmdFile);  // nem volt a path-on
    }
    cmdFile = f.absoluteFilePath();
    cmd = cmdFile + cmd;
    _DBGFNL() << "cmd = " << quotedString(cmd) << endl;
}

eNotifSwitch cDaemon::run(QSqlQuery& q)
{
    DBGFN();
    if (crashCnt > maxCrashCnt) return RS_DOWN;
    switch (daemonType) {
    case DT_POLLING:
        if (process.state() == QProcess::NotRunning) {
            process.start(cmd);
        }
        else {
            hostService.setState(q,
                                 _sWarning,
                                 QString(QObject::trUtf8("Idő tullépés a %1 parancs futásánál.")).arg(cmd),
                                 ((lv2d *)lanView::getInstance())->pSelf->hostServiceId());
        }
        break;
    case DT_CONTINUE:
    case DT_RESPAWN:
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
    QString msg = QString(trUtf8("Service %1, Command : '%2'; error : %3")).arg(service().getName()).arg(cmd).arg(ProcessError2Message(error));
    DERR() << msg << endl;
    INSERROR(EPROCERR, (int)error, msg);
}

void cDaemon::procFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    _DBGFN() << name() << " @(" << exitCode << ", " << (exitStatus ? "normal exit" : "crashed") << ")" << endl;
    if (internalStat != IS_RUN) return;
    if (exitStatus ==  QProcess::CrashExit || daemonType == DT_CONTINUE) {
        crashCnt++;
        QString msg = QString(QObject::trUtf8("Service %1 : ")).arg(service().getName());
        if (exitStatus ==  QProcess::CrashExit) {
            msg += QString(QObject::trUtf8("A %1 program elszállt")).arg(quotedString(cmd));
        }
        else {
            msg += QString(QObject::trUtf8("A %1 program kilépett. Kilépési kód %2.")).arg(quotedString(cmd)).arg(exitCode);
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
    if (internalStat != IS_RUN) switch (daemonType) {
    case DT_CONTINUE:
    case DT_RESPAWN:
        PDEB(INFO) << "Restart process " << name() << endl;
        process.start(cmd);
        break;
    case DT_POLLING:
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
    actLogFile.write(process.readAllStandardOutput());
    qint64 pos = actLogFile.pos();
    if (logRot && maxLogSize < pos) {
        PDEB(VVERBOSE) << "LogRot : " << maxLogSize << " < " << pos << endl;
        actLogFile.close();
        QString     old;
        QString     pre;
        for (int i = maxArcLog; i > 1; --i) {
            old = actLogFile.fileName() + _sPoint + QString::number(i);
            pre = actLogFile.fileName() + _sPoint + QString::number(i -1);
            bool r = QFile::rename(pre, old);
            PDEB(VVERBOSE) << "Rename " << pre << " to " << old << " Result : " << DBOOL(r) << endl;
        }
        old = actLogFile.fileName() + ".1";
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

