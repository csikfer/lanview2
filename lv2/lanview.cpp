#include "lanview.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
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
    PDEB(PARSEARG) << "findArg('" << __c << "', \"" << __s << "\", args)" << endl;
    for (int i = 1; i < args.count(); ++i) {
        QString arg(args[i]);
        if (arg.indexOf(_sMinusMinus)  == 0) {
            if (__s == arg.mid(2)) {
                PDEB(PARSEARG) << QObject::trUtf8("Long switch found, return %1").arg(i) << endl;
                return i;
            }
        }
        else if (arg[0] == QChar('-') && arg.count() > 1) {
            if (QChar(__c) == arg[1]) {
                PDEB(PARSEARG) << QObject::trUtf8("Short switch found, return %1").arg(i) << endl;
                return i;
            }
        }
    }
    // PDEB(PARSEARG) << "Program switch not found. return -1" << endl;
    return -1;
}

// ****************************************************************************************************************

/// Default debug level
qlonglong lanView::debugDefault = cDebug::DERROR | cDebug::WARNING | cDebug::INFO | cDebug::VERBOSE | cDebug::LV2;
lanView  *lanView::instance     = NULL;
bool      lanView::snmpNeeded   = true;
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
    instance = NULL;
    PDEB(OBJECT) << QObject::trUtf8("delete (lanView *)").arg((qulonglong)this) << endl;
    // Ha volt hiba objektumunk, töröljük. Elötte kiírjuk a hibaüzenetet, ha tényleg hiba volt
    if (lastError && lastError->mErrorCode != eError::EOK) {
        PDEB(DERROR) << lastError->msg();         // A Hiba üzenet
        sendError(lastError);
    }
    if (lastError) delete lastError;
    lastError = NULL;
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
        EXCEPTION(EOK); // Exit program
    }
    if (0 < findArg(QChar('V'), _sLibVersion, args)) {
        QTextStream QStdOut(stdout);
        QStdOut << QString(trUtf8("LanView2 lib version ")).arg(libVersion) << endl;
        EXCEPTION(EOK); // Exit program
    }
    if (0 < findArg(QChar('v'), _sVersion, args)) {
        QTextStream QStdOut(stdout);
        QStdOut << QString(trUtf8("%1 version %2")).arg(appName).arg(appVersion) << endl;
        EXCEPTION(EOK); // Exit program
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
    QString sql = "INSERT INTO app_errs"
                 "(app_name, pid, app_ver, lib_ver, thread_name, err_code, err_name, err_subcode, err_msg, errno, func_name, func_src, src_line) "
            "VALUES (?,"     "?," "?,"     "?,"     "?,"         "?,"      "?,"      "?,"         "?,"     "?,"   "?,"       "?,"      "?) "
            "RETURNING applog_id";
    PDEB(VVERBOSE) << QObject::trUtf8("sendError() : Prepare ...") << endl;
    if (! q.prepare(sql)) {
        DERR() << trUtf8("Dropp error object ... ") << endl;
        SQLPREPERRDEB(q, sql);
        return NULL_ID;
    }
    // PDEB(VVERBOSE) << "Bindings ..." << endl;
    q.bindValue( 0, appName);
    q.bindValue( 1, QCoreApplication::applicationPid());
    q.bindValue( 2, appVersion);
    q.bindValue( 3, libVersion);
    q.bindValue( 4, __t.isNull()              ? QVariant(_sNul) : QVariant(__t));
    q.bindValue( 5, pe->mErrorCode);
    q.bindValue( 6, pe->errorMsg());
    q.bindValue( 7, QVariant(pe->mErrorSubCode));
    q.bindValue( 8, pe->mErrorSubMsg.isNull() ? QVariant(_sNul) : QVariant(pe->mErrorSubMsg));
    q.bindValue( 9, pe->mErrorSysCode);
    q.bindValue(10, pe->mFuncName);
    q.bindValue(11, pe->mSrcName);
    q.bindValue(12, pe->mSrcLine);
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

void    lanView::dbNotif(QString __s)
{
    DERR() << QObject::trUtf8("Database notifycation : %1").arg(__s) << endl;
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
    if (pDb != NULL && pDb->driver()->hasFeature(QSqlDriver::EventNotifications)) {
        QString name = __n.isEmpty() ? appName : __n;
        pDb->driver()->subscribeToNotification(name);
        connect(pDb->driver(), SIGNAL(notification(QString)), SLOT(dbNotif(QString)));
        return true;
    }
    else {
        QString e = trUtf8("Database notification is nothing supported.");
        if (__ex) EXCEPTION(ENOTSUPP, -1, e);
        DERR() << e << endl;
        return false;
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


