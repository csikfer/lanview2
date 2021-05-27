#include "import.h"
#include "import_parser.h"
#include "lv2service.h"
#include "others.h"
#include <QCoreApplication>
#include <iostream>

#define VERSION_MAJOR   0
#define VERSION_MINOR   05
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp += QObject::tr("-i|--input-file <path>      Set input file path\n");
    lanView::appHelp += QObject::tr("-I|--input-stdin            Set input file is stdin\n");
    lanView::appHelp += QObject::tr("-D|--daemon-mode            Set daemon mode (default, if set -R)\n");
    lanView::appHelp += QObject::tr("-o|--output-file <path>     Set output file path\n");
    lanView::appHelp += QObject::tr("-O|--output-stdout          Set output file is stdin\n");
    lanView::appHelp += QObject::tr("-e|--exec <command>         Execute <command>\n");
    lanView::appHelp += QObject::tr("If set -R and/or -D, then ignore -i, -I and -e.\n");
    lanView::appHelp += QObject::tr("If set -e, then ignore -o and -O.\n");
}

QString     lv2import::actDir;

int main (int argc, char * argv[])
{
    cLv2QApp app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;
    lv2import::actDir = QDir::currentPath();

    lv2import   mo;
    if (mo.lastError != nullptr) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    if (mo.daemonMode) {        // Daemon mód
        // A beragadt rekordok kikukázása
        PDEB(VERBOSE) << "Start event loop..." << endl;
        return app.exec();
    }
    else {                      // executed
        // Ha nem daemon mód van, akkor visszaállítjuk az aktuális könyvtárt
        return 0;
    }
}

lv2import::lv2import() : lanView()
  , inFileName("-"), outFileName("-")
{
    daemonMode = forcedSelfHostService; // Ha az -R megvan adva, akkor nem kell a -D opció.
    pQuery = nullptr;
    if (lastError != nullptr) {
        return;
    }
    pQuery = newQuery();
    int i;
    // Command options :
    if (0 < (i = findArg('i', "input-file", args)) && (i + 1) < args.count()) {
        inFileName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('I', "input-stdin", args))) {
        inFileName = '-';
        args.removeAt(i);
    }
    if (0 < (i = findArg('D', "daemon-mode", args))) {
        args.removeAt(i);;
        PDEB(INFO) << tr("Set daemon mode.") << endl;
        daemonMode = true;
    }
    if (0 < (i = findArg('o', "output-file", args)) && (i + 1) < args.count()) {
        outFileName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('O', "output-stdout", args))) {
        outFileName = '-';
        args.removeAt(i);
    }
    if (0 < (i = findArg('e', "exec", args)) && (i + 1) < args.count()) {
        sExecStr = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (args.count() > 1) DWAR() << tr("Invalid arguments : ") << args.join(QChar(' ')) << endl;
    try {
        if (daemonMode) {
            subsDbNotif();
        }
    } CATCHS(lastError)
    if (daemonMode) {
        initDaemon();
    }
    else {
        QDir::setCurrent(actDir);
        execute();
    }
}

lv2import::~lv2import()
{
    DBGFN();
    pDelete(pQuery);
}

void lv2import::initDaemon()
{
    if (pSelfHostService != nullptr && pSelfService != nullptr && pSelfService->getName() == _sImport) {
        pSelfInspector = new cImportInspector();
        pSelfInspector->postInit(*pQuery);
        pSelfInspector->start();
        abortOldRecords(*pQuery);
    }
    else {
        EXCEPTION(ECONTEXT);
    }
}

void lv2import::abortOldRecords(QSqlQuery& q)
{
    static const QString sql =
            "UPDATE imports "
            "   SET exec_state = 'aborted',"
                "   ended = CURRENT_TIMESTAMP,"
                "   result_msg = '(Re)Start imports server: old records aborted.'"
            " WHERE target_id = ? AND (exec_state = 'wait' OR exec_state = 'execute')";
    execSql(q, sql, pSelfHostService->get(pSelfHostService->idIndex()));
}

void lv2import::fetchAndExec()
{
    debugStream *pDS = cDebug::getInstance()->pCout();
    cImport     *pImp = nullptr;
    cExportQueue::init(true);
    lastError = nullptr;
    cError *ipe = importGetLastError(); // Töröljük a hiba objektumot, biztos ami biztos.
    if (ipe != nullptr) {  // Ennek NULL-nak kellene lennie !! Nem kezeltünk egy hibát?!
        ERROR_NESTED(ipe).exception();
    }
    try {
        pImp = new cImport;
        pImp->setName(_sExecState, _sWait);
        pImp->setId(_sTargetId, pSelfInspector->hostServiceId());
        pImp->fetch(*pQuery, false, pImp->mask(_sTargetId, _sExecState), pImp->iTab(_sDateOf), 1);
        if (pImp->isEmptyRec_()) {
            PDEB(INFO) << tr("No waitig imports record, dropp notification.") << endl;
            return;
        }
        pImp->_toReadBack = RB_NO;
        pImp->setName(_sExecState, _sExecute);
        pImp->set(_sStarted, _sNOW);
        pImp->setId(_sPid, QCoreApplication::applicationPid());
        pImp->update(*pQuery, false, pImp->mask(_sExecState, _sStarted, _sPid));
        sDebugLines.clear();
        if (!connect(pDS, SIGNAL(readyDebugLine()), this, SLOT(debugLine()))) {
            EXCEPTION(EPROGFAIL);
        }
        importParseText(pImp->getName(_sImportText));
    }
    CATCHS(lastError)
    ipe = importGetLastError();
    static const QBitArray ufmask = pImp->mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId) | pImp->mask( _sOutMsg, _sExpMsg);
    if (ipe != nullptr) {
        if (lastError != nullptr) {    // Többszörös hiba ??!!
            QString m = sDebugLines
                    + QString(40, QChar('-')) + "\n"
                    + lastError->msg() + "\n"
                    + QString(40, QChar('*')) + "\n"
                    + ipe->msg();
            delete lastError;
            delete ipe;
            EXCEPTION(ENESTED, 0, m);
        }
        lastError = ipe;
    }
    pImp->setName(_sExpMsg, cExportQueue::toText(true));
    pImp->setName(_sOutMsg, sDebugLines);
    if (lastError == nullptr) {    // OK
        pImp->setName(_sExecState, _sOk);
        pImp->setName(_sResultMsg, _sOk);
        pImp->set(_sEnded, _sNOW);
        pImp->clear(_sAppLogId);
        pImp->update(*pQuery, false, ufmask);
    }
    else if (pImp != nullptr) {    // Hiba, a cImport objektum létre lett hozva
        qlonglong eid = sendError(lastError);
        pImp->setName(_sExecState, _sFaile);
        pImp->setName(_sResultMsg, lastError->msg());
        pDelete(lastError);
        pImp->set(_sEnded, _sNOW);
        pImp->setId(_sAppLogId, eid);
        pImp->update(*pQuery, false, ufmask);
    }
    else {                      // A cImport objektum létrehozása sem sikerült
        ERROR_NESTED(lastError).exception();
    }
    pDelete(pImp);
    QCoreApplication::exit(0);
}

void lv2import::execute()
{
    PDEB(VVERBOSE) << "Current dir : " << actDir << endl;
    QDir::setCurrent(actDir);
    if (parentIsLv2d()) {  // Az lv2d hívta meg
        setUser(UID_SYSTEM);
    }
    else {
        QString domain, user;
        cUser *pUser = getOsUser(domain, user);
        if (pUser == nullptr) {
            EXCEPTION(EPERMISSION, 0 , tr("Authentication failed (%1@%2).").arg(user, domain));
        }
        if (pUser->privilegeLevel() < PL_ADMIN) {
            EXCEPTION(EPERMISSION, 0 , tr("You do not have sufficient privileges (%1@%2 : %3).").arg(user, domain, pUser->getName()));
        }
        setUser(pUser->getId());
        delete pUser;
    }
    QString out;
    try {
        if (inFileName.isEmpty() && sExecStr.isEmpty()) EXCEPTION(EDATA, -1, QObject::tr("Nincs megadva forrás fáj, vagy végrehajtandó parancs!"));
        PDEB(VVERBOSE) << "Begin parser ..." << endl;
        cExportQueue::init(true);
        if (sExecStr.isEmpty()) importParseFile(inFileName);
        else                    importParseText(sExecStr);
        out = cExportQueue::toText(true);
    } catch(cError *e) {
        lastError = e;
    } catch(...) {
        lastError = NEWCERROR(EUNKNOWN);
    }
    cError *ipe = importGetLastError();
    if (ipe != nullptr) lastError = ipe;
    if (outFileName.isEmpty()) {
        PDEB(INFO) << "Parser output : \n" << out << endl;
    }
    else if (outFileName == "-") {
        std::cout << out.toStdString() << std::endl;
    }
    else {
        QFile of(outFileName);
        if (of.open(QIODevice::WriteOnly)) {
            of.write(out.toUtf8());
            of.close();
        }
        else {
            DERR() << QString("File %1 open error : %2 ").arg(outFileName, of.errorString());
        }
    }
    if (lastError) {
        PDEB(DERROR) << "**** ERROR ****\n" << lastError->msg() << endl;
    }
    else {
        PDEB(DERROR) << "**** OK ****" << endl;
    }
}

void lv2import::dbNotif(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
    PDEB(INFO) << QString(tr("DB notification : %1, %2, %3.")).arg(name).arg(int(source)).arg(debVariantToString(payload)) << endl;
    if (name == _sImport) {
        bool ok;
        qlonglong id = payload.toLongLong(&ok);
        if (ok && pSelfHostService->getId() == id) {
            PDEB(VERBOSE) << "CALL fetchAndExec() by dbNotif(...)" << endl;
            fetchAndExec();
        }
    }
}

void lv2import::debugLine()
{
    QString s = cDebug::getInstance()->dequeue();
    QRegExp  re("^([\\da-f]{8})\\s(.+)$");
    if (re.exactMatch(s)) {
        bool ok;
        QString sm = re.cap(1);
        qulonglong m = sm.toULongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL);
        if (m & (cDebug::INFO | cDebug::WARNING | cDebug::DERROR)) {
            s = re.cap(2).trimmed();
            sDebugLines += s + "\n";
        }
    }
}

int cImportInspector::run(QSqlQuery&, QString &)
{
    return RS_ON;
}
