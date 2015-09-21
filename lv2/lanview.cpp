#include "lanview.h"
#include "lv2service.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   02
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

// ****************************************************************************************************************
int findArg(char __c, const char * __s, int argc, char * argv[])
{
    for (int i = 1; i < argc; ++i) {
        const char * arg = argv[i];
        size_t s = strlen(arg);
        if (s < 2) continue;    // 2 betünél kisebb kapcsoló nem lehet
        if (arg[0] == '-') {    // A kapcsoló '-'-al kezdődik
            if (arg[1] == '-') {// Ha '--' -al kezdődik
                if (s == 2) {       // a '--' az argumentumok vége
                    if (0 == strcmp(__s, __sMinusMinus)) return -1; // épp ezt kerestük
                    break;                                          // ha vége, akkor vége
                }
                if (0 == strcmp(__s, arg + 2)) {
                    return i;    // "hosszú" kapcsoló"
                }
            }
            else {              // Csak egy '-' -al kezdődik
                if (__c == arg[1]) {
                    return i;    // egy betűs kapcsoló
                }
            }
        }
    }
    return -1;
}

int findArg(const QChar& __c, const QString& __s, const QStringList &args)
{
    //PDEB(PARSEARG) << "findArg('" << __c << "', \"" << __s << "\", args)" << endl;
    for (int i = 1; i < args.count(); ++i) {
        QString arg(args[i]);
        if (arg.indexOf(_sMinusMinus)  == 0) {
            if (__s == arg.mid(2)) {
                //PDEB(PARSEARG) << QObject::trUtf8("Long switch found, return %1").arg(i) << endl;
                return i;
            }
        }
        else if (arg[0] == QChar('-') && arg.count() > 1) {
            if (QChar(__c) == arg[1]) {
                //PDEB(PARSEARG) << QObject::trUtf8("Short switch found, return %1").arg(i) << endl;
                return i;
            }
        }
    }
    // PDEB(PARSEARG) << "Program switch not found. return -1" << endl;
    return -1;
}

// ****************************************************************************************************************

#ifdef Q_OS_WIN
#define DEFDEB 0
#else
#define DEFDEB cDebug::DERROR | cDebug::WARNING | cDebug::INFO | cDebug::VERBOSE | cDebug::LV2
#endif

/// Default debug level
qlonglong lanView::debugDefault = DEFDEB;
lanView  *lanView::instance     = NULL;
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
bool      lanView::snmpNeeded   = true;
#else
bool      lanView::snmpNeeded   = false;
#endif
bool      lanView::sqlNeeded    = true;
bool      lanView::gui          = false;
const QString lanView::orgName(ORGNAME);
const QString lanView::orgDomain(ORGDOMAIN);
const short lanView::libVersionMajor = VERSION_MAJOR;
const short lanView::libVersionMinor = VERSION_MINOR;
const QString lanView::libVersion(_STR(VERSION_MAJOR) "." _STR(VERSION_MINOR));
#ifdef Q_OS_WIN
const QString lanView::homeDefault("C:\\lanview2");
#else
const QString lanView::homeDefault("/var/local/lanview2");
#endif
QString    lanView::appHelp;
QString    lanView::appName;
short      lanView::appVersionMinor;
short      lanView::appVersionMajor;
QString    lanView::appVersion;
QString    lanView::testSelfName;
eIPV4Pol   lanView::ipv4Pol = IPV4_STRICT;
eIPV6Pol   lanView::ipv6Pol = IPV6_PERMISSIVE;


// ****************************************************************************************************************
lanView::lanView()
    : QObject(), libName(LIBNAME), debFile("-"), homeDir(), binPath(), args(), lang("en"), dbThreadMap(), threadMutex()
{
    // DBGFN();
    debug       = debugDefault;
    pSet        = NULL;
    lastError   = NULL;
    pDb         = NULL;
#ifdef MUST_USIGNAL
    unixSignals = NULL;
#endif // MUST_USIGNAL
    libTranslator= NULL;
    appTranslator = NULL;
    pSelfNode = NULL;
    pSelfService = NULL;
    pSelfHostService = NULL;
    pUser = NULL;
    setSelfStateF = false;


    try {
        lang = getEnvVar("LANG");
        if (!lang.isEmpty()) {
            int i = lang.indexOf(QChar('.'));
            if (i > 0) lang = lang.mid(0,i);
        }
        cError::init(); // Hiba stringek feltöltése.
        initUserMetaTypes();

        if (testSelfName.isEmpty()) {
            testSelfName = getEnvVar("LV2TEST_SET_SELF_NAME");
        }
        // Program settings
        pSet = new QSettings(orgName, libName);
        // Beállítjuk a DEBUG/LOG paramétereket
        debug   = pSet->value(_sDebugLevel, QVariant(debugDefault)).toLongLong();
        debFile = pSet->value(_sLogFile, QVariant(_sStdErr)).toString();
        if (testSelfName.isEmpty()) {   // Csak ha még nincs beállítva
            testSelfName = pSet->value(_sLv2testSetSelfNname).toString();
        }
        homeDir = pSet->value(_sHomeDir, QVariant(homeDefault)).toString();
        ipv4Pol = (eIPV4Pol)IPV4Pol(pSet->value(_sIPV4Pol, QVariant(_sStrict)).toString());
        ipv6Pol = (eIPV6Pol)IPV6Pol(pSet->value(_sIPV6Pol, QVariant(_sPermissive)).toString());
        QDir    d(homeDir);
        if (!QDir::setCurrent(d.path())) EXCEPTION(ENODIR, -1, d.path());
        binPath = d.filePath(_sBin);
        binPath = pSet->value(_sBinDir,  QVariant(binPath)).toString();
        lang    = pSet->value(_sLang,    QVariant(lang)).toString();
        // Inicializáljuk a cDebug objektumot
        cDebug::init(debFile, debug);
        libTranslator = new QTranslator(this);
        if (libTranslator->load(libName + "_" + lang, homeDir)) {
            QCoreApplication::installTranslator(libTranslator);
        }
        else {
            delete libTranslator;
            libTranslator = NULL;
        }
        // Feldolgozzuk a program paramétereket
        args = QCoreApplication::arguments();
        parseArg();
        // cDebug változások
        if (cDebug::fName() != cDebug::fNameCnv(debFile)) cDebug::init(debFile, debug);
        else cDebug::set(debug);
        // Unix Signals
#ifdef MUST_USIGNAL
        unixSignals = new cXSignal(this);
        cXSignal::setupUnixSignalHandlers();
        connect(unixSignals, SIGNAL(qsignal(int)), this, SLOT(uSigSlot(int)));
#endif // MUST_USIGNAL
        instAppTransl();
        if (instance != NULL) EXCEPTION(EPROGFAIL,-1,QObject::trUtf8("A lanView objektumból csak egy példány lehet."))
        instance = this;
        // Kapcsolódunk az adatbázishoz, ha kell
        if (sqlNeeded) {
            openDatabase();
            // SELF:
            QSqlQuery q = getQuery();
            pSelfNode = new cNode;
            if (pSelfNode->fetchSelf(q, false)) {
                pSelfService = new cService;
                if (pSelfService->fetchByName(q, appName)) {
                    pSelfHostService = new cHostService();
                    pSelfHostService->setId(_sNodeId,    pSelfNode->getId());
                    pSelfHostService->setId(_sServiceId, pSelfService->getId());
                    if (1 == pSelfHostService->completion(q)) {
                        setSelfStateF = true;
                    }
                    else {// Nincs, vagy nem egyértelmű
                        pDelete(pSelfHostService);
                    }
                }
                else {
                    pDelete(pSelfService);
                }
            }
            else {
                pDelete(pSelfNode);
            }
        }
        // SNMP init, ha kell
        if (snmpNeeded) {
#ifdef MUST_SNMP
            netSnmp::init();
#else //  MUST_SNMP
            EXCEPTION(ENOTSUPP, -1, QObject::trUtf8("SNMP not supported."));
#endif //  MUST_SNMP
        }
        PDEB(VVERBOSE) << "End " << __PRETTY_FUNCTION__ << " try block." << endl;
    } CATCHS(lastError)
    DBGFNL();
}

lanView::~lanView()
{
    instance = NULL;    // "Kifelé" már nincs objektum
    PDEB(OBJECT) << QObject::trUtf8("delete (lanView *)%1").arg((qulonglong)this) << endl;
    // Ha volt hiba objektumunk, töröljük. Elötte kiírjuk a hibaüzenetet, ha tényleg hiba volt
    if (lastError && lastError->mErrorCode != eError::EOK) {
        PDEB(DERROR) << lastError->msg();         // A Hiba üzenet
        qlonglong eid = sendError(lastError);
        // Hibát kiírtuk, ki kell írni a staust is? (ha nem sikerült a hiba kiírása, kár a statussal próbálkozni)
        if (eid != NULL_ID && setSelfStateF) {
            if (pSelfHostService != NULL) {
                QSqlQuery   q(*pDb);
                try {
                    pSelfHostService->setState(q, _sCritical, lastError->msg());
                } catch(...) {
                    DERR() << "!!!!" << endl;
                }
            }
            else {
                DERR() << trUtf8("A pSelfHostService pointer NULL.") << endl;
            }
        }
        setSelfStateF = false;
    }
    else if (setSelfStateF) {
        if (pSelfHostService != NULL && pDb != NULL) {
            QSqlQuery   q(*pDb);
            try {
                int rs = RS_ON;
                QString n;
                if (lastError != NULL) {
                    rs = lastError->mErrorSubCode;      // Ide kéne rakni a kiírandó staust
                    n  = lastError->mErrorSubMsg;       // A megjegyzés a status-hoz
                }
                if ((rs & RS_STAT_SETTED) == 0) {        // A stusban jelezheti, hogy már megvolt a kiírás!
                    rs &= ~RS_STAT_MASK;
                    pSelfHostService->setState(q, notifSwitch(rs), n);
                }
            } catch(...) {
                DERR() << "!!!!" << endl;
            }
        }
        else {
            DERR() << trUtf8("A pSefHostService vagy a pDb pointer NULL.") << endl;
        }
    }
    pDelete(lastError);
    // Lelőjük az összes Thread-et ...? Azoknak elvileg már nem szabadna mennie, sőt a konténernek is illik üresnek lennie
    // A thread-ek adatbázis objektumait töröljük.
    PDEB(INFO) << QObject::trUtf8("Lock by threadMutex, in lanView destructor ...") << endl;
    threadMutex.lock();
    for (QMap<QString, QSqlDatabase *>::iterator i = dbThreadMap.begin(); i != dbThreadMap.end(); i++) {
        QSqlDatabase * pdb = i.value();
        if (pdb != NULL){
            if (pdb->isValid()) {
                if (pdb->isOpen()) {
                    pdb->close();
                    PDEB(SQL) << QObject::trUtf8("Close database.") << endl;
                }
            }
            delete pdb;
            DWAR() << QObject::trUtf8("pdb for %1 thread deleted by lanView destructor.").arg(i.key()) << endl;
        }
        else {
            DERR() << QObject::trUtf8("pdb is NULL, for %1 thread.").arg(i.key()) << endl;
        }
    }
    dbThreadMap.clear();
    threadMutex.unlock();
    // Töröljük az adatbázis objektumot, ha volt.
    closeDatabase();
    // Töröljük a QSettings objektumunkat
    if (pSet != NULL) {
        pSet->sync();
        delete pSet;
        pSet = NULL;
    }
#ifdef MUST_SNMP
    netSnmp::down();
#endif // MUST_SNMP
    if (appTranslator != NULL) {
        QCoreApplication::removeTranslator(appTranslator);
        delete appTranslator;
    }
    if (libTranslator != NULL) {
        QCoreApplication::removeTranslator(libTranslator);
        delete libTranslator;
    }
    DBGFNL();
}

bool lanView::openDatabase(bool __ex)
{
    closeDatabase();
    PDEB(VERBOSE) << "Open database ..." << endl;
    pDb = new  QSqlDatabase(QSqlDatabase::addDatabase(_sQPSql));
    if (!pDb->isValid()) SQLOERR(*pDb);
    pDb->setHostName(pSet->value(_sSqlHost).toString());
    pDb->setPort(pSet->value(_sSqlPort).toUInt());
    pDb->setUserName(pSet->value(_sSqlUser).toString());
    pDb->setPassword(pSet->value(_sSqlPass).toString());
    pDb->setDatabaseName(pSet->value(_sDbName).toString());
    bool r = pDb->open();
    if (!r && __ex) // SQLOERR(*pDb);
    {
        QSqlError le = (*pDb).lastError();
        _sql_err_deb_(le, __FILE__, __LINE__, __PRETTY_FUNCTION__);
        EXCEPTION(ESQLOPEN, le.number(), le.text());
    }
    if (!r) closeDatabase();
    return r;
}

void lanView::closeDatabase()
{
    if (pDb != NULL){
        if (pDb->isValid()) {
            if (pDb->isOpen()) {
                pDb->close();
                PDEB(SQL) << QObject::trUtf8("Close database.") << endl;
            }
            delete pDb;
            pDb = NULL;
            QSqlDatabase::removeDatabase(_sQPSql);
            PDEB(SQL) << QObject::trUtf8("Remove database object.") << endl;
        }
        else {
            delete pDb;
            pDb = NULL;
        }
        PDEB(SQL) << QObject::trUtf8("pDb deleted.") << endl;
    }
}

void lanView::parseArg(void)
{
    DBGFN();
    int i;
    if (ONDB(PARSEARG)) {
        cDebug::cout() << head << QObject::trUtf8("arguments :");
        foreach (QString arg, args) {
             cDebug::cout() << QChar(' ') << arg;
        }
        cDebug::cout() << endl;
    }
    if (0 < findArg(QChar('h'), _sHelp, args)) {
        QTextStream QStdOut(stdout);
        if (appHelp.isEmpty() == false) QStdOut << appHelp << endl;
        QStdOut << trUtf8("-d|--debug-level <level>    Set debug level") << endl;
        QStdOut << trUtf8("-L|--log-file <file name>   Set log file name") << endl;
        QStdOut << trUtf8("-V|--lib-version            Print lib version") << endl;
        QStdOut << trUtf8("-S|--test-self-name         Test option") << endl;
        QStdOut << trUtf8("-h|--help                   Print help") << endl;
        EXCEPTION(EOK, RS_STAT_SETTED); // Exit program
    }
    if (0 < findArg(QChar('V'), _sLibVersion, args)) {
        QTextStream QStdOut(stdout);
        QStdOut << QString(trUtf8("LanView2 lib version ")).arg(libVersion) << endl;
        EXCEPTION(EOK, RS_STAT_SETTED); // Exit program
    }
    if (0 < findArg(QChar('v'), _sVersion, args)) {
        QTextStream QStdOut(stdout);
        QStdOut << QString(trUtf8("%1 version %2")).arg(appName).arg(appVersion) << endl;
        EXCEPTION(EOK, RS_STAT_SETTED); // Exit program
    }
    if (0 < (i = findArg(QChar('d'), _sDebugLevel, args))
     && (i + 1) < args.count()) {
        bool ok;
        debug = args[i + 1].toLongLong(&ok, 0);
        if (!ok) DERR() << trUtf8("Invalid numeric argument parameter : %1 %2").arg(args[i]).arg(args[i + 1])  << endl;
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg(QChar('L'), _sLogFile, args))
     && (i + 1) < args.count()) {
        debFile = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg(QChar('S'), _sLv2testSetSelfNname, args))
     && (i + 1) < args.count()) {
        testSelfName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    DBGFNL();
}

qlonglong lanView::sendError(const cError *pe, const QString& __t)
{
    _DBGFN() << "Thread : " << __t << endl;
    // Ha nem is volt hiba, akkor kész.
    if (pe == NULL || pe->mErrorCode == eError::EOK) {
        _DBGFNL() << "No error." << endl;
        return NULL_ID;
    }
    PDEB(VVERBOSE) << QObject::trUtf8("sendError() : %1").arg(pe->msg()) << endl;
    // Ha van nyitott adatbázis, csinálunk egy hiba rekordot
    if (instance == NULL || instance->pDb == NULL || !instance->pDb->isOpen()) {
        DWAR() << trUtf8("Dropp error object, no database object or not open.") << endl;
        return NULL_ID;
    }
    QSqlQuery   q(*instance->pDb);
    // sqlRollback(q, false);
    // sqlBegin(q);
    cNamedList  fields;
    fields.add(_sAppName,       appName);
    if (instance->pSelfNode != NULL && !selfNode().isNullId()) fields.add(_sNodeId, selfNode().getId());
    fields.add(_sPid,           QCoreApplication::applicationPid());
    fields.add("app_ver",       appVersion);
    fields.add("lib_ver",       libVersion);
    fields.add("func_name",     pe->mFuncName);
    fields.add("src_name",      pe->mSrcName);
    fields.add("src_line",      pe->mSrcLine);
    fields.add("err_code",      pe->mErrorCode);
    fields.add("err_name",      pe->errorMsg());
    fields.add("err_subcode",   pe->mErrorSubCode);
    fields.add("err_syscode",   pe->mErrorSysCode);
    fields.add("err_submsg",    pe->mErrorSubMsg);
    fields.add("thread_name",   pe->mThreadName);
    fields.add("sql_err_num",   pe->mSqlErrNum);
    fields.add("sql_err_type",  pe->mSqlErrType);
    fields.add("sql_driver_text",pe->mSqlErrDrText);
    fields.add("sql_db_text",   pe->mSqlErrDbText);
    fields.add("sql_query",     pe->mSqlQuery);
    fields.add("sql_bounds",    pe->mSqlBounds);
    fields.add("data_line",     pe->mDataLine);
    fields.add("data_pos",      pe->mDataPos);
    fields.add("data_msg",      pe->mDataMsg);
    fields.add("data_name",     pe->mDataName);

    QString sql ="INSERT INTO app_errs (" + fields.names().join(", ") + ") VALUES (" + fields.quotes() + ") RETURNING applog_id";
    PDEB(VVERBOSE) << QObject::trUtf8("sendError() : Prepare ...") << endl;
    if (! q.prepare(sql)) {
        DERR() << trUtf8("Dropp error object ... ") << endl;
        SQLPREPERRDEB(q, sql);
        return NULL_ID;
    }
    // PDEB(VVERBOSE) << "Bindings ..." << endl;
    int n = fields.size();
    for (int i = 0; i < n; ++i) q.bindValue(i, fields.value(i));
    // PDEB(VVERBOSE) << "Exec ..." << endl;
    if (!q.exec()) {
        DERR() << trUtf8("Dropp error object ... ") << endl;
        SQLQUERYERRDEB(q);
        return NULL_ID;
    }
    qlonglong   eid = NULL_ID;
    if (q.first()) eid = variantToId(q.value(0));
    DBGFNL();
    // sqlEnd(q);
    return eid;
}

void lanView::instAppTransl()
{
    appTranslator = new QTranslator(this);
    if (appTranslator->load(langFileName(appName, lang), homeDir)) {
        QCoreApplication::installTranslator(appTranslator);
    }
    else {
        DERR() << QObject::trUtf8("Application language file not loaded : %1/%2").arg(appName).arg(lang) << endl;
        delete appTranslator;
        appTranslator = NULL;
    }
}

bool lanView::uSigRecv(int __i)
{
    return (__i == __i);    // Hogy örüljön a fordító, használom az __i-t
}

void lanView::uSigSlot(int __i)
{
    if (uSigRecv(__i)) switch (__i) {
        case SIGINT:
            PDEB(INFO) << QObject::trUtf8("Signal SIGINT, call exit.") << endl;;
            QCoreApplication::exit(0);
            break;
        default:
            PDEB(WARNING) << QObject::trUtf8("Signal #%1 ignored.").arg(__i) << endl;
            break;
    }
}

void    lanView::dbNotif(const QString& name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
    if (ONDB(INFO)) {
        QString src;
        switch (source) {
        case QSqlDriver::SelfSource:    src = "Self";       break;
        case QSqlDriver::OtherSource:   src = "Other";      break;
        case QSqlDriver::UnknownSource:
        default:                        src = "Unknown";    break;
        }
        cDebug::cout() << QObject::trUtf8("Database notifycation : %1, source %2, payload :").arg(name).arg(src) << debVariantToString(payload) << endl;
    }
}

void lanView::insertStart(QSqlQuery& q)
{
    DBGFN();
    /* if (!q.exec(QString("SELECT error('Start', %1, '%2')").arg(QCoreApplication::applicationPid()).arg(appName)))
        SQLQUERYERR(q);*/
    cDbErr::insertNew(q, cDbErrType::_sStart, appName, QCoreApplication::applicationPid(), _sNil, _sNil);
}

void lanView::insertReStart(QSqlQuery& q) {
    DBGFN();
    /*if (!q.exec(QString("SELECT error('ReStart', %1, '%2')").arg(QCoreApplication::applicationPid()).arg(appName)))
        SQLQUERYERR(q);*/
    cDbErr::insertNew(q, cDbErrType::_sReStart, appName, QCoreApplication::applicationPid(), _sNil, _sNil);
}

bool lanView::subsDbNotif(const QString& __n, bool __ex)
{
    static bool first = true;
    QString e;
    if (pDb != NULL && pDb->driver()->hasFeature(QSqlDriver::EventNotifications)) {
        QString name = __n.isEmpty() ? appName : __n;
        if (pDb->driver()->subscribeToNotification(name)) {
            PDEB(INFO) << "Subscribed NOTIF " << name << endl;
            if (first) {
                PDEB(VVERBOSE) << "Connect to notification(QString)" << endl;
                connect(pDb->driver(),
                        SIGNAL(notification(const QString&, QSqlDriver::NotificationSource, const QVariant&)),
                               SLOT(dbNotif(const QString&, QSqlDriver::NotificationSource, const QVariant&)));
                first = false;
            }
            return true;
        }
        e = trUtf8("Database notification subscribe is unsuccessful.");
    }
    else {
    	e = trUtf8("Database notification is nothing supported.");
    }
    if (__ex) EXCEPTION(ENOTSUPP, -1, e);
    DERR() << e << endl;
    return false;
}

const cUser *lanView::setUser(const QString& un, const QString& pw, bool __ex)
{
    if (instance == NULL) EXCEPTION(EPROGFAIL);
    QSqlQuery q = getQuery();
    if (instance->pUser != NULL) pDelete(instance->pUser);
    instance->pUser = new cUser();
    if (!instance->pUser->checkPasswordAndFetch(q, pw, un)) {
        pDelete(instance->pUser);
        if (__ex)  EXCEPTION(EFOUND,-1,trUtf8("Ismeretlen felhasználó, vagy nem megfelelő jelszó"));
        return NULL;
    }
    if (!q.exec(QString("SELECT set_user_id(%1)").arg(instance->pUser->getId()))) SQLQUERYERR(q);
    return instance->pUser;
}
const cUser *lanView::setUser(const QString& un, bool __ex)
{
    if (instance == NULL) EXCEPTION(EPROGFAIL);
    QSqlQuery q = getQuery();
    if (instance->pUser != NULL) pDelete(instance->pUser);
    instance->pUser = new cUser();
    if (!instance->pUser->fetchByName(q, un)) {
        pDelete(instance->pUser);
        if (__ex)  EXCEPTION(EFOUND,-1,trUtf8("Ismeretlen felhasználó : %1").arg(un));
        return NULL;
    }
    if (q.exec(QString("SELECT set_user_id(%1)").arg(instance->pUser->getId()))) SQLQUERYERR(q);
    return instance->pUser;
}

const cUser *lanView::setUser(qlonglong uid, bool __ex)
{
    if (instance == NULL) EXCEPTION(EPROGFAIL);
    QSqlQuery q = getQuery();
    if (instance->pUser != NULL) pDelete(instance->pUser);
    instance->pUser = new cUser();
    if (!instance->pUser->fetchById(q,uid)) {
        pDelete(instance->pUser);
        if (__ex)  EXCEPTION(EFOUND, uid, trUtf8("Ismeretlen felhasználó azonosító"));
        return NULL;
    }
    if (!q.exec(QString("SELECT set_user_id(%1)").arg(uid))) SQLQUERYERR(q);
    instance->pUser->getRights(q);
    return instance->pUser;
}

const cUser& lanView::user()
{
    if (instance == NULL || instance->pUser == NULL) EXCEPTION(EPROGFAIL);
    return *instance->pUser;
}

void lanView::resetCacheData()
{
    if (!exist()) EXCEPTION(EPROGFAIL);
    QSqlQuery q = getQuery();
    cIfType::fetchIfTypes(q);
    cService::clearServicesCache();
    if (instance->pUser != NULL) {
        instance->pUser->setById(q);
        instance->pUser->getRights(q);
    }
}

const QString& IPV4Pol(int e, bool __ex)
{
    switch(e) {
    case IPV4_IGNORED:      return _sIgnored;
    case IPV4_PERMISSIVE:   return _sPermissive;
    case IPV4_STRICT:       return _sStrict;
    }
    if (__ex) EXCEPTION(EDATA);
    return _sNul;
}

int IPV4Pol(const QString& n, bool __ex)
{
    if (n.compare(_sIgnored, Qt::CaseInsensitive)) return IPV4_IGNORED;
    if (n.compare(_sPermissive, Qt::CaseInsensitive)) return IPV4_PERMISSIVE;
    if (n.compare(_sStrict, Qt::CaseInsensitive)) return IPV4_STRICT;
    if (__ex) EXCEPTION(EDATA);
    return IPV4_UNKNOWN;
}

const QString& IPV6Pol(int e, bool __ex)
{
    switch(e) {
    case IPV6_IGNORED:      return _sIgnored;
    case IPV6_PERMISSIVE:   return _sPermissive;
    case IPV6_STRICT:       return _sStrict;
    }
    if (__ex) EXCEPTION(EDATA);
    return _sNul;
}

int IPV6Pol(const QString& n, bool __ex)
{
    if (n.compare(_sIgnored, Qt::CaseInsensitive)) return IPV6_IGNORED;
    if (n.compare(_sPermissive, Qt::CaseInsensitive)) return IPV6_PERMISSIVE;
    if (n.compare(_sStrict, Qt::CaseInsensitive)) return IPV6_STRICT;
    if (__ex) EXCEPTION(EDATA);
    return IPV6_UNKNOWN;
}

cLv2QApp::cLv2QApp(int& argc, char ** argv) : QCoreApplication(argc, argv)
{
    lanView::gui = false;
}


cLv2QApp::~cLv2QApp()
{
    ;
}

bool cLv2QApp::notify(QObject * receiver, QEvent * event)
{
    static cError *lastError = NULL;
    try {
        return QCoreApplication::notify(receiver, event);
    }
    catch(no_init_&) { // Már letiltottuk a cError dobálást
        PDEB(VERBOSE) << "Dropped cError..." << endl;
        return false;
    }
    CATCHS(lastError)
/*    PDEB(DERROR) << "Error in "
                 << __PRETTY_FUNCTION__
                 << endl;
    PDEB(DERROR) << "Receiver : "
                 << receiver->objectName()
                 << "::"
                 << typeid(*receiver).name()
                 << endl;
    PDEB(DERROR) << "Event : "
                 << typeid(*event).name()
                 << endl; Ettől kiakad :-O ! */
    cError::mDropAll = true;                    // A továbbiakban nem *cError-al dobja a hibákat, hanem no_init_ -el
    lanView::getInstance()->lastError = lastError;
    DERR() << lastError->msg() << endl;
    QCoreApplication::exit(lastError->mErrorCode);  // kilépünk.
    return false;
}
