#include "lanview.h"
#include "lv2data.h"
#include "srvdata.h"
#include "lv2daterr.h"
#include "lv2user.h"
#include "lv2service.h"
#include "guidata.h"


#define VERSION_MAJOR   0
#define VERSION_MINOR   94
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR) "(" _STR(REVISION) ")"

#define DB_VERSION_MAJOR 1
#define DB_VERSION_MINOR 7

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
#define DEFDEB cDebug::DERROR | cDebug::WARNING | cDebug::INFO | cDebug::VERBOSE | cDebug::MODMASK
#endif

/// Default debug level
qlonglong lanView::debugDefault = DEFDEB;
lanView  *lanView::instance     = NULL;
#if defined(Q_OS_UNIX) || defined(Q_OS_LINUX)
bool      lanView::snmpNeeded   = true;
#else
bool      lanView::snmpNeeded   = false;
#endif
eSqlNeed  lanView::sqlNeeded    = SN_SQL_NEED;
bool      lanView::gui          = false;
const QString lanView::orgName(ORGNAME);
const QString lanView::orgDomain(ORGDOMAIN);
const short lanView::libVersionMajor = VERSION_MAJOR;
const short lanView::libVersionMinor = VERSION_MINOR;
const QString lanView::libVersion(VERSION_STR);
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
bool       lanView::setupTransactionFlag = false;

qlonglong sendError(const cError *pe, lanView *_instance)
{
    // Ha nem is volt hiba, akkor kész.
    if (pe == NULL || pe->mErrorCode == eError::EOK) {
        return NULL_ID;
    }
    // Ha van nyitott adatbázis, csinálunk egy hiba rekordot
    lanView * pInst = _instance == NULL ? lanView::instance : _instance;
    if (pInst == NULL || pInst->pDb == NULL || !pInst->pDb->isOpen()) {
        return NULL_ID;
    }
    QSqlQuery   q(*pInst->pDb);
    // sqlRollback(q, false);
    // sqlBegin(q);
    cNamedList  fields;
    fields.add(_sAppName,       lanView::appName);
    if (pInst->pSelfNode != NULL && !pInst->pSelfNode->isNullId()) fields.add(_sNodeId, pInst->pSelfNode->getId());
    fields.add(_sPid,           QCoreApplication::applicationPid());
    fields.add(_sAppVer,        lanView::appVersion);
    fields.add(_sLibVer,        lanView::libVersion);
    if (pInst->pUser != NULL && !pInst->pUser->isNullId()) fields.add(_sUserId, pInst->user().getId());
    if (pInst->pSelfService != NULL && !pInst->pSelfService->isNullId()) fields.add(_sServiceId, pInst->pSelfService->getId());
    fields.add(_sFuncName,      pe->mFuncName);
    fields.add(_sSrcName,       pe->mSrcName);
    fields.add(_sSrcLine,       pe->mSrcLine);
    fields.add("err_code",      pe->mErrorCode);
    fields.add("err_name",      pe->errorMsg());
    fields.add("err_subcode",   pe->mErrorSubCode);
    fields.add("err_syscode",   pe->mErrorSysCode);
    fields.add("err_submsg",    pe->mErrorSubMsg);
    fields.add(_sThreadName,    pe->mThreadName);
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
    if (! q.prepare(sql)) {
        return NULL_ID;
    }
    int n = fields.size();
    for (int i = 0; i < n; ++i) q.bindValue(i, fields.value(i));
    if (!q.exec()) {
        return NULL_ID;
    }
    qlonglong   eid = NULL_ID;
    if (q.first()) eid = variantToId(q.value(0));
    // sqlCommit(q);
    return eid;
}

// ****************************************************************************************************************

void settingIntParameter(QSqlQuery& q, const QString& pname)
{
    int i = cSysParam::getIntSysParam(q, pname);
    if (i > 0) {
        QString sql = QString("SET %1 TO %2").arg(pname).arg(i);
        EXECSQL(q, sql);
    }
}

void dbOpenPost(QSqlQuery& q, int n)
{
    QString nn = lanView::appName;
    if (n > 0) {
        setLanguage(q, lanView::getInstance()->langId);
        if (lanView::getInstance()->pUser != NULL) {
            if (!q.exec(QString("SELECT set_user_id(%1)").arg(lanView::getInstance()->pUser->getId()))) SQLQUERYERR(q);
        }
        nn += "_" + QString::number(n);
    }
    EXECSQL(q, QString("SET application_name TO '%1'").arg(nn));
    settingIntParameter(q, "lock_timeout");
    settingIntParameter(q, "statement_timeout");
}

lanView::lanView()
    : QObject(), libName(LIBNAME), debFile("-"), homeDir(), binPath(), args(), dbThreadMap(), threadMutex()
{
    // DBGFN();
    debug       = debugDefault;
    pSet        = NULL;
    lastError   = NULL;
    nonFatal    = NULL;
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
    pSelfInspector = NULL;
    selfHostServiceId = NULL_ID;
    pQuery = NULL;

    try {
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
        if (!QDir::setCurrent(d.path())) {
            // Ha GUI, és nem volt megadva könyvtár, akkor az user home könyvtár lessz a home
            if (gui) {
                if (pSet->value(_sHomeDir).isNull()) {
                    d = QDir::homePath();
                    homeDir = d.path();
                }
                else {
                    // You should not crashed, rather setup
                    nonFatal = NEWCERROR(ENODIR, 0, d.path());
                }
            }
            else {
                EXCEPTION(ENODIR, -1, d.path());
            }
        }
        binPath = d.filePath(_sBin);
        binPath = pSet->value(_sBinDir,  QVariant(binPath)).toString();
        // Inicializáljuk a cDebug objektumot
        cDebug::init(debFile, debug);
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
        if (instance != NULL) EXCEPTION(EPROGFAIL,-1,QObject::trUtf8("A lanView objektumból csak egy példány lehet."))
        instance = this;
        // Connect database, if need
        if (sqlNeeded != SN_NO_SQL) {
            if (openDatabase(bool2ex(sqlNeeded == SN_SQL_NEED))) {
                pQuery = newQuery();
                dbOpenPost(*pQuery);
                // A reasons típus nem tartozik olyan táblához amihez osztály is van definiálva, külön csekkoljuk
                cColEnumType::checkEnum(*pQuery, "reasons", reasons, reasons);
                setSelfObjects();
            }
        }
        // Language
        bool lok;
        langId = pSet->value(_sLangId).toInt(&lok);
        if (!lok) langId = NULL_IX; // int !
        if (lok && pQuery != NULL) {
            setLanguage(*pQuery, langId);
            sLang = getLanguage(*pQuery, langId);
        }
        if (sLang.isEmpty()) {
            sLang = getEnvVar("LANG");
            if (!sLang.isEmpty()) {
                int i = sLang.indexOf(QChar('.'));
                if (i > 0) sLang = sLang.mid(0,i);
            }
        }
        if (!lok && pQuery != NULL && !sLang.isEmpty()) {
            QStringList sl = sLang.split("_");
            if (sl.size() < 2) sl << _sNul;
            langId = setLanguage(*pQuery, sl.first(), sl.at(1));
        }
        if (!sLang.isEmpty()) {
            libTranslator = new QTranslator(this);
            if (libTranslator->load(libName + "_" + sLang, homeDir)
             || libTranslator->load(libName + "_" + sLang, binPath)) {
                QCoreApplication::installTranslator(libTranslator);
            }
            else {
                delete libTranslator;
                libTranslator = NULL;
            }
        }
        instAppTransl();
        // SNMP init, ha kell
        if (snmpNeeded) {
#ifdef SNMP_IS_EXISTS
            netSnmp::init();
#else //  SNMP_IS_EXISTS
            EXCEPTION(ENOTSUPP, -1, QObject::trUtf8("SNMP not supported."));
#endif //  SNMP_IS_EXISTS
        }
        PDEB(VVERBOSE) << "End " << __PRETTY_FUNCTION__ << " try block." << endl;
    } CATCHS(lastError)
    DBGFNL();
}

QStringList * lanView::getTransactioMapAndCondLock()
{
    if (isMainThread()) {
        return &getInstance()->mainTrasactions;
    }
    else {
        getInstance()->threadMutex.lock();
        QMap<QString, QStringList>&  map = lanView::getInstance()->trasactionsThreadMap;
        QString thread = currentThreadName();
        QMap<QString, QStringList>::iterator i = map.find(thread);
        if (i == map.end()) {
            lanView::getInstance()->threadMutex.unlock();
            EXCEPTION(EPROGFAIL, 0, thread);
        }
        return &i.value();
    }
}

static void rollback_all(const QString& n, QSqlDatabase * pdb, QStringList& l)
{
    if (!l.isEmpty()) {
        DWAR() << QObject::trUtf8("Rolback %1 all transactions : %2").arg(n,l.join(", ")) << endl;
        QSqlQuery q(*pdb);
        q.exec("ROLLBACK");
        l.clear();
    }
}

lanView::~lanView()
{
    instance = NULL;    // "Kifelé" már nincs objektum
    PDEB(OBJECT) << QObject::trUtf8("delete (lanView *)%1").arg((qulonglong)this) << endl;
    // fő szál tranzakciói (nem kéne lennie, ha mégis, akkor rolback mindegyikre)
    rollback_all("main", pDb, mainTrasactions);
    // Ha volt hiba objektumunk, töröljük. Elötte kiírjuk a hibaüzenetet, ha tényleg hiba volt
    if (lastError != NULL && lastError->mErrorCode != eError::EOK) {    // Hiba volt
        PDEB(DERROR) << lastError->msg() << endl;         // A Hiba üzenet
        qlonglong eid = sendError(lastError, this);
        // Hibát kiírtuk, ki kell írni a staust is? (ha nem sikerült a hiba kiírása, kár a statussal próbálkozni)
        if (eid != NULL_ID && setSelfStateF) {
            if (pSelfHostService != NULL && pQuery != NULL) {
                try {
                    pSelfHostService->setState(*pQuery, _sCritical, lastError->msg());
                } catch(...) {
                    DERR() << "!!!!" << endl;
                }
            }
            else {
                if (pSelfHostService == NULL) DERR() << trUtf8("A pSelfHostService pointer értéke NULL.") << endl;
                if (pQuery           == NULL) DERR() << trUtf8("A pQuery pointer értéke NULL.") << endl;
            }
        }
        setSelfStateF = false;
    }
    else if (setSelfStateF) {
        if (pSelfHostService != NULL && pQuery != NULL) {
            try {
                int rs = RS_ON;
                QString n;
                if (lastError != NULL) {
                    rs = lastError->mErrorSubCode;      // Ide kéne rakni a kiírandó staust
                    n  = lastError->mErrorSubMsg;       // A megjegyzés a status-hoz
                }
                if ((rs & RS_STAT_SETTED) == 0) {        // Jelezheti, hogy már megvolt a kiírás!
                    pSelfHostService->setState(*pQuery, notifSwitch(rs), n);
                }
            } catch(...) {
                DERR() << "!!!!" << endl;
            }
        }
        else {
            DERR() << trUtf8("A pSefHostService vagy a pQuery pointer értéke NULL.") << endl;
        }
    }
    pDelete(lastError);
    int en = cError::errCount();
    if (en) {   // Ennek illene üresnek lennie
        DERR() << trUtf8("A hiba objektum konténer nem üres! %1 db kezeletlen objektum.").arg(en) << endl;
        do {
            cError *pe = cError::errorList.first();
            sendError(pe, this);
            delete pe;
        } while (cError::errCount());
    }
    // Lelőjük az összes Thread-et ...? Azoknak elvileg már nem szabadna mennie, sőt a konténernek is illik üresnek lennie
    // A thread-ek adatbázis objektumait töröljük.
    PDEB(INFO) << QObject::trUtf8("Lock by threadMutex, in lanView destructor ...") << endl;
    threadMutex.lock();
    for (QMap<QString, QSqlDatabase *>::iterator i = dbThreadMap.begin(); i != dbThreadMap.end(); i++) {
        QSqlDatabase * pdb = i.value();
        if (pdb != NULL){
            if (pdb->isValid()) {
                if (pdb->isOpen()) {
                    rollback_all(i.key(), pdb, trasactionsThreadMap[i.key()]);
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
    trasactionsThreadMap.clear();
    threadMutex.unlock();
    // Töröljük az adatbázis objektumot, ha volt.
    pDelete(pQuery);
    closeDatabase();
    // Töröljük a QSettings objektumunkat
    if (pSet != NULL) {
        pSet->sync();
        delete pSet;
        pSet = NULL;
    }
#ifdef SNMP_IS_EXISTS
    netSnmp::down();
#endif // SNMP_IS_EXISTS
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

bool checkDbVersion(QSqlQuery& q, QString& msg)
{
    // A cSysParam objektumot nem használhatjuk !!
    bool ok1, ok2;
    int vmajor = (int)execSqlIntFunction(q, &ok1, "get_int_sys_param", "version_major");
    int vminor = (int)execSqlIntFunction(q, &ok2, "get_int_sys_param", "version_minor");
    if (!(ok1 && ok2)) {
        msg = QObject::trUtf8("The database version numbers wrong format or missing.");
        DERR() << msg << endl;
        return false;
    }
    if (DB_VERSION_MAJOR != vmajor || DB_VERSION_MINOR != vminor) {
        msg = QObject::trUtf8("Incorrect database version.\n"
                              "The expected version is %1.%2\n"
                              "The current database version is %3.%4")
                .arg(DB_VERSION_MAJOR).arg(DB_VERSION_MINOR).arg(vmajor).arg(vminor);
        DERR() << msg << endl;
        return false;
    }
    return true;
}

bool lanView::openDatabase(eEx __ex)
{
    closeDatabase();
    PDEB(VERBOSE) << "Open database ..." << endl;
    pDb = new  QSqlDatabase(QSqlDatabase::addDatabase(_sQPSql));
    if (!pDb->isValid()) SQLOERR(*pDb);
    pDb->setHostName(pSet->value(_sSqlHost).toString());
    pDb->setPort(pSet->value(_sSqlPort).toUInt());
    pDb->setUserName(scramble(pSet->value(_sSqlUser).toString()));
    pDb->setPassword(scramble(pSet->value(_sSqlPass).toString()));
    pDb->setDatabaseName(pSet->value(_sDbName).toString());
    bool r = pDb->open();
    cError *pe;
    if (r) {
        QSqlQuery q(*pDb);
        QString msg;
        r = checkDbVersion(q, msg);
        if (!r) {
            pe = NEWCERROR(ESQLOPEN, 0, msg);
            if (__ex == EX_IGNORE) nonFatal = pe;
            else pe->exception();
            closeDatabase();
        }
    }
    else {
        QSqlError le = (*pDb).lastError();
        _sql_err_deb_(le, __FILE__, __LINE__, __PRETTY_FUNCTION__);
        pe = NEWCERROR(ESQLOPEN, le.number(), le.text());
        if (__ex == EX_IGNORE) nonFatal = pe;
        else pe->exception();
    }
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

void lanView::setSelfObjects()
{
    if (pDb != NULL && pDb->isOpen()) {
        if (!isMainThread()) EXCEPTION(EPROGFAIL);
        pSelfNode = new cNode;
        if (selfHostServiceId != NULL_ID) {
            pSelfHostService = new cHostService();
            pSelfHostService->setById(*pQuery, selfHostServiceId);
            pSelfNode->setById(*pQuery, pSelfHostService->getId(_sNodeId));
            pSelfService = cService::service(*pQuery, pSelfHostService->getId(_sServiceId));
            setSelfStateF = true;
        }
        else if (pSelfNode->fetchSelf(*pQuery, EX_IGNORE)) {
            pSelfService = cService::service(*pQuery, appName, EX_IGNORE);
            if (pSelfService != NULL) {
                pSelfHostService = new cHostService();
                pSelfHostService->setId(_sNodeId,    pSelfNode->getId());
                pSelfHostService->setId(_sServiceId, pSelfService->getId());
                if (1 == pSelfHostService->completion(*pQuery)) {
                    setSelfStateF = true;
                }
                else {// Nincs, vagy nem egyértelmű
                    pDelete(pSelfHostService);
                }
            }
        }
        else {
            pDelete(pSelfNode);
        }
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
        QStdOut << trUtf8("-R|--host-service-id        Root host-service id") << endl;
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
    if (0 < (i = findArg(QChar('R'), QString(_sHostServiceId).replace(QChar('_'), QChar('-')), args))
     && (i + 1) < args.count()) {
        bool ok;
        selfHostServiceId = args[i + 1].toLongLong(&ok, 0);
        if (!ok) DERR() << trUtf8("Invalid numeric argument parameter : %1 %2").arg(args[i]).arg(args[i + 1])  << endl;
        args.removeAt(i);
        args.removeAt(i);
    }
    DBGFNL();
}

void lanView::instAppTransl()
{
    if (sLang.isEmpty()) {
        appTranslator = NULL;
        DERR() << QObject::trUtf8("Application language file not loaded, language string id is empty.") << endl;
        return;
    }
    appTranslator = new QTranslator(this);
    if (appTranslator->load(langFileName(appName, sLang), homeDir)
     || appTranslator->load(langFileName(appName, sLang), binPath)) {
        QCoreApplication::installTranslator(appTranslator);
    }
    else {
        DERR() << QObject::trUtf8("Application language file not loaded : %1/%2").arg(appName).arg(sLang) << endl;
        delete appTranslator;
        appTranslator = NULL;
    }
}

#ifdef MUST_USIGNAL
bool lanView::uSigRecv(int __i)
{
    if (__i == SIGHUP) {
        PDEB(INFO) << trUtf8("Esemény : SIGHUP; reset ...") << endl;
        reSet();
        return false;
    }
    return true;
}
#endif

void lanView::reSet()
{
    try {
        down();
        resetCacheData();
        setSelfObjects();
        setup();
    } CATCHS(lastError)
    if (lastError != NULL) QCoreApplication::exit(lastError->mErrorCode);
}

void lanView::setup(eTristate _tr)
{
    tSetup<cInspector>(_tr);
}

void lanView::down()
{
    pDelete(pSelfNode);
    pSelfService = NULL;    // cache!
    pDelete(pSelfHostService);
    pDelete(pSelfInspector);
}

#ifdef MUST_USIGNAL
void lanView::uSigSlot(int __i)
{
    if (uSigRecv(__i)) switch (__i) {
        case SIGINT:
            PDEB(INFO) << QObject::trUtf8("Signal SIGINT, call exit.") << endl;
            QCoreApplication::exit(0);
            break;
        default:
            PDEB(WARNING) << QObject::trUtf8("Signal #%1 ignored.").arg(__i) << endl;
            break;
    }
}
#endif

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
        cDebug::cout() << HEAD() << QObject::trUtf8("Database notifycation : %1, source %2, payload :").arg(name).arg(src) << debVariantToString(payload) << endl;
    }
    QString sPayload = payload.toString();
    if (0 == appName.compare(name,     Qt::CaseInsensitive)
     && 0 == _sReset.compare(sPayload, Qt::CaseInsensitive)) {
        PDEB(INFO) << trUtf8("Esemény : NOTIFY %1  %2; reset ...").arg(name, sPayload) << endl;
        reSet();
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

bool lanView::subsDbNotif(const QString& __n, eEx __ex)
{
    static bool first = true;
    QString e;
    if (pDb != NULL && pDb->driver()->hasFeature(QSqlDriver::EventNotifications)) {
        QString name = __n.isEmpty() ? appName : __n;
        if (!first && pDb->driver()->subscribedToNotifications().contains(name)) {
            PDEB(INFO) << "Already subscribed NOTIF " << name << endl;
            return true;
        }
        PDEB(INFO) << "Subscribe NOTIF " << name << endl;
        if (pDb->driver()->subscribeToNotification(name)) {
            if (first) {
                PDEB(VVERBOSE) << "Connect to notification(QString)" << endl;
                connect(pDb->driver(),
                        SIGNAL(notification(const QString&, QSqlDriver::NotificationSource, const QVariant&)),
                               SLOT(dbNotif(const QString&, QSqlDriver::NotificationSource, const QVariant&)));
                first = false;
            }
            return true;    // O.K.
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

const cUser *lanView::setUser(const QString& un, const QString& pw, eEx __ex)
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
const cUser *lanView::setUser(const QString& un, eEx __ex)
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

const cUser *lanView::setUser(qlonglong uid, eEx __ex)
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

const cUser *lanView::setUser(cUser *__pUser)
{
    if (instance == NULL) EXCEPTION(EPROGFAIL);
    if (!(__pUser->_stat < 0 || __pUser->_stat & ES_COMPLETE)) EXCEPTION(EDATA,__pUser->_stat, "Invalid __pUser object :" + __pUser->toString());
    if (instance->pUser != NULL) pDelete(instance->pUser);
    instance->pUser = __pUser;
    return instance->pUser;
}


const cUser& lanView::user()
{
    if (instance == NULL || instance->pUser == NULL) EXCEPTION(EPROGFAIL);
    return *instance->pUser;
}

qlonglong lanView::getUserId(eEx __ex)
{
    if (instance == NULL) EXCEPTION(EPROGFAIL);
    if (instance->pUser == NULL) {
        if (__ex != EX_IGNORE) EXCEPTION(EFOUND);
        return NULL_ID;
    }
    return instance->pUser->getId();
}

bool lanView::isAuthorized(enum ePrivilegeLevel pl)
{
    enum ePrivilegeLevel act = user().privilegeLevel();
    return act  >= pl;
}

bool lanView::isAuthorized(const QString& pl)
{
    return isAuthorized((enum ePrivilegeLevel)privilegeLevel(pl));
}



void lanView::resetCacheData()
{
    if (!exist()) EXCEPTION(EPROGFAIL);
    QSqlQuery q = getQuery();
    cIfType::fetchIfTypes(q);
    cService::resetCacheData();
    cTableShape::resetCacheData();
    if (instance->pUser != NULL) {
        instance->pUser->setById(q);
        instance->pUser->getRights(q);
    }
}

const QString& IPV4Pol(int e, eEx __ex)
{
    switch(e) {
    case IPV4_IGNORED:      return _sIgnored;
    case IPV4_PERMISSIVE:   return _sPermissive;
    case IPV4_STRICT:       return _sStrict;
    }
    if (__ex) EXCEPTION(EDATA);
    return _sNul;
}

int IPV4Pol(const QString& n, eEx __ex)
{
    if (n.compare(_sIgnored, Qt::CaseInsensitive)) return IPV4_IGNORED;
    if (n.compare(_sPermissive, Qt::CaseInsensitive)) return IPV4_PERMISSIVE;
    if (n.compare(_sStrict, Qt::CaseInsensitive)) return IPV4_STRICT;
    if (__ex) EXCEPTION(EDATA);
    return IPV4_UNKNOWN;
}

const QString& IPV6Pol(int e, eEx __ex)
{
    switch(e) {
    case IPV6_IGNORED:      return _sIgnored;
    case IPV6_PERMISSIVE:   return _sPermissive;
    case IPV6_STRICT:       return _sStrict;
    }
    if (__ex) EXCEPTION(EDATA);
    return _sNul;
}

int IPV6Pol(const QString& n, eEx __ex)
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

QString scramble(const QString& _s)
{
    QString r;
    qsrand(32572345U);
    foreach (QChar c, _s) {
        ushort u = c.unicode();
        u ^= (ushort)qrand();
        r += QChar(u);
    }
    return r;
}

enum eImportParserStat importParserStat = IPS_READY;

bool callFromInterpreter()
{
    switch (importParserStat) {
    case IPS_READY:                         // Nem fut az interpreter
        return false;
    case IPS_RUN:                           // A fő szálban fut az interpreter
        if (isMainThread()) return true;    // Ez a fő szál
        return false;                       // De ez nem a fő szál
    case IPS_THREAD:                        // Egy thread-ban fut az interpreter
        if (isMainThread()) return false;   // De ez a fő szál
        if (QThread::currentThread()->objectName() == _sImportParser) return true;
        return false;                       // Ez egy másik szál
    default:
        EXCEPTION(EPROGFAIL);
        return false;
    }
}

#define IMPQUEUEMAXWAIT 1000

cExportQueue *cExportQueue::_pForParser = NULL;
QMutex cExportQueue::staticMutex;
QMap<QString, cExportQueue *>    cExportQueue::intMap;

cExportQueue *cExportQueue::_pInstanceByThread()
{
    QString tn = currentThreadName();
    QMap<QString, cExportQueue *>::iterator it = intMap.find(tn);
    if (it == intMap.end()) return NULL;
    return it.value();
}

cExportQueue *cExportQueue::pInstance()
{
    cExportQueue *r = NULL;
    if (!staticMutex.tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
    if (callFromInterpreter()) {
        if (_pForParser == NULL) _pForParser = new cExportQueue;
        r = _pForParser;
    }
    else {
        r = _pInstanceByThread();
    }
    staticMutex.unlock();
    return r;
}

cExportQueue::cExportQueue()
    : QObject(), QQueue<QString>(), QMutex()
{
    ;
}

cExportQueue::~cExportQueue()
{
    ;
}

/// Ha nem az iterpreterből lett hivva a metódus nem csinál semmit, lásd callFromInterpreter() -t.
/// Egyébként egy sor vagy paragrafus elhelyezése a queue-ban.
/// A szöveg elhelyezése után kivált egy ready() szignált.
/// A queue-hoz való hozzáféréshez egy belső mutex objektumot használ,
/// időtullépés esetén kizárást dob.
void cExportQueue::push(const QString &s)
{
    cExportQueue *r = pInstance();
    if (r == NULL) return;
    if (!r->tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
    r->enqueue(s);
    r->unlock();
    r->ready();
}

void cExportQueue::push(const QStringList &sl)
{
    cExportQueue *r = pInstance();
    if (r == NULL) return;
    foreach (QString s, sl) {
        if (!r->tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
        r->enqueue(s);
        r->unlock();
        r->ready();
    }
}

/// Kivesz egy sort vagy paragrafust a queue-ból.
/// Ha a queue üres, akkor egy null stringgel tér vissza.
/// A queue-hoz való hozzáféréshez egy belső mutex objektumot használ,
/// időtullépés esetén kizárást dob.
QString cExportQueue::pop()
{
    if (!tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
    QString s;
    if (!isEmpty()) s = dequeue();
    unlock();
    return s;
}

/// Kiüríti a queue-t, az eredmény stringben a queue elemei közötti szeparátor a soremelés karakter.
/// A queue-hoz való hozzáféréshez egy belső mutex objektumot használ,
/// időtullépés esetén kizárást dob.
/// ...
QString cExportQueue::toText(bool fromInterpret, bool _clr)
{
    cExportQueue *p;
    if (!staticMutex.tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
    if (fromInterpret) {
        p = _pForParser;
    }
    else {
        p = _pInstanceByThread();
    }
    staticMutex.unlock();
    QString s;
    if (p != NULL) {
        if (!p->tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
        s = p->join("\n");
        if (!fromInterpret && _clr) {
            QString tn = currentThreadName();
            p->unlock();
            delete p;
            intMap.remove(tn);
            return s;
        }
        else {
            p->clear();
        }
    }
    p->unlock();
    return s;
}

cExportQueue *cExportQueue::init(bool fromInterpret)
{
    cExportQueue *p;
    if (!staticMutex.tryLock(IMPQUEUEMAXWAIT)) EXCEPTION(ETO);
    if (fromInterpret) {
        if (_pForParser != NULL) {
            delete _pForParser;
        }
        _pForParser = p = new cExportQueue;
    }
    else {
        QThread *th = QThread::currentThread();
        QString  tn = threadName(th);
        QMap<QString, cExportQueue *>::iterator it = intMap.find(tn);
        if (it == intMap.end()) {
            intMap[tn] = p = new cExportQueue;
        }
        else {
            delete it.value();
            it.value() = p = new cExportQueue;
        }
        connect(th, SIGNAL(destroyed(QObject*)), p, SLOT(destroy_mq(QObject*)));
    }
    staticMutex.unlock();
    return p;
}

void cExportQueue::destroy_mq(QObject *p)
{
    QString tn = threadName((QThread *)p);
    QMap<QString, cExportQueue *>::iterator it = intMap.find(tn);
    if (it == intMap.end()) {
        // Ettöl a sortol megkergül a fordító :-O
        // DERR() << trUtf8("Object (thread) name %1 not found").arg(tn) << endl;
    }
    else {
        delete it.value();
        intMap.remove(tn);
    }
}
