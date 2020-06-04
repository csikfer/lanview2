#include "import.h"
#include "import_parser.h"
#include "lv2service.h"
#include "others.h"
#include <QCoreApplication>

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
    lanView::appHelp  = QObject::tr("-u|--user-id <id>           Set user id\n");
    lanView::appHelp += QObject::tr("-U|--user-name <name>       Set user name\n");
    lanView::appHelp += QObject::tr("-i|--input-file <path>      Set input file path\n");
    lanView::appHelp += QObject::tr("-I|--input-stdin            Set input file is stdin\n");
    lanView::appHelp += QObject::tr("-D|--daemon-mode            Set daemon mode\n");
    lanView::appHelp += QObject::tr("-o|--output-file <path>     Set output file path\n");
    lanView::appHelp += QObject::tr("-O|--output-stdout          Set input file is stdin\n");
}

int main (int argc, char * argv[])
{
    cLv2QApp app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;

    // Elmentjük az aktuális könyvtárt
    QString actDir = QDir::currentPath();
    lv2import   mo;
    if (mo.lastError != nullptr) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    PDEB(VVERBOSE) << "Saved original current dir : " << actDir << "; actDir : " << QDir::currentPath() << endl;

    if (mo.daemonMode) {        // Daemon mód
        // A beragadt rekordok kikukázása
        mo.abortOldRecords();
        return app.exec();
    }
    else {
        // Ha nem daemon mód van, akkor visszaállítjuk az aktuális könyvtárt
        QDir::setCurrent(actDir);
        PDEB(VVERBOSE) << "Restore act dir : " << QDir::currentPath() << endl;
        try {
            if (mo.inFileName.isEmpty()) EXCEPTION(EDATA, -1, QObject::tr("Nincs megadva forrás fájl!"));
            PDEB(VVERBOSE) << "BEGIN transaction ..." << endl;
            importParseFile(mo.inFileName);
        } catch(cError *e) {
            mo.lastError = e;
        } catch(...) {
            mo.lastError = NEWCERROR(EUNKNOWN);
        }
        cError *ipe = importGetLastError();
        if (ipe != nullptr) mo.lastError = ipe;
        if (mo.lastError) {
            PDEB(DERROR) << "**** ERROR ****\n" << mo.lastError->msg() << endl;
        }
        else {
            PDEB(DERROR) << "**** OK ****" << endl;
        }

        cDebug::end();
        return mo.lastError == nullptr ? 0 : mo.lastError->mErrorCode;
    }
}

void lv2import::abortOldRecords()
{
    QSqlQuery q = getQuery();
    QString sql =
            "UPDATE imports "
            "   SET exec_state = 'aborted',"
                "   ended = CURRENT_TIMESTAMP,"
                "   result_msg = 'Start imports server: old records aborted.'"
            " WHERE exec_state = 'wait' OR exec_state = 'execute'";
    if (!q.exec(sql)) SQLPREPERR(*pQuery, sql);
}

void lv2import::dbNotif(const QString &name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
    cImport     *pImp = nullptr;
    cExportQueue::init(true);
    lastError = nullptr;
    cError *ipe = importGetLastError(); // Töröljük a hiba objektumot, biztos ami biztos.
    if (ipe != nullptr) {  // Ennek NULL-nak kellene lennie !! Nem kezeltünk egy hibát?!
        ERROR_NESTED(ipe).exception();
    }
    try {
        PDEB(INFO) << QString(tr("DB notification : %1, %2, %3.")).arg(name).arg(int(source)).arg(debVariantToString(payload)) << endl;
        pImp = new cImport;
        pImp->setName(_sExecState, _sWait);
        pImp->fetch(*pQuery, false, pImp->mask(_sExecState), pImp->iTab(_sDateOf), 1);
        if (pImp->isEmpty_()) {
            DWAR() << tr("No waitig imports record, dropp notification.") << endl;
            return;
        }
        pImp->setName(_sExecState, _sExecute);
        pImp->set(_sStarted, QVariant(QDateTime::currentDateTime()));
        pImp->setId(_sPid, QCoreApplication::applicationPid());
        pImp->update(*pQuery, false, pImp->mask(_sExecState, _sStarted, _sPid));
        importParseText(pImp->getName(_sImportText));
    }
    CATCHS(lastError)
    ipe = importGetLastError();
    static const QBitArray ufmask = pImp->mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId) | pImp->mask( _sOutMsg);
    if (ipe != nullptr) {
        if (lastError != nullptr) {    // Többszörös hiba ??!!
            QString m = lastError->msg() + "\n" + QString(40, QChar('*')) + "\n" + ipe->msg();
            delete lastError;
            delete ipe;
            EXCEPTION(ENESTED, 0, m);
        }
        lastError = ipe;
    }
    pImp->setName(_sOutMsg, cExportQueue::toText(true));
    if (lastError == nullptr) {    // OK
        pImp->setName(_sExecState, _sOk);
        pImp->setName(_sResultMsg, _sOk);
        pImp->set(_sEnded, QVariant(QDateTime::currentDateTime()));
        pImp->clear(_sAppLogId);
        pImp->update(*pQuery, false, ufmask);
    }
    else if (pImp != nullptr) {    // Hiba, a cImport objektum létre lett hozva
        qlonglong eid = sendError(lastError);
        pImp->setName(_sExecState, _sFaile);
        pImp->setName(_sResultMsg, lastError->msg());
        pDelete(lastError);
        pImp->set(_sEnded, QVariant(QDateTime::currentDateTime()));
        pImp->setId(_sAppLogId, eid);
        pImp->update(*pQuery, false, ufmask);
    }
    else {                      // A cImport objektum létrehozása sem sikerült
        ERROR_NESTED(lastError).exception();
    }
    pDelete(pImp);
    QCoreApplication::exit(0);
}


lv2import::lv2import() : lanView()
{
    daemonMode = false;
    pQuery = nullptr;
    if (lastError != nullptr) {
        return;
    }
    pQuery = newQuery();
    int i;
    QString   userName;
    if (0 < (i = findArg('u', "user-id", args)) && (i + 1) < args.count()) {
        // ???
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('U', "user-name", args)) && (i + 1) < args.count()) {
        userName = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
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
        inFileName = '-';
        args.removeAt(i);
    }
    if (args.count() > 1) DWAR() << tr("Invalid arguments : ") << args.join(QChar(' ')) << endl;
    try {
        if (daemonMode) {
            subsDbNotif();
        }
        else {
            insertStart(*pQuery);
            if (!userName.isNull()) setUser(userName);
        }
    } CATCHS(lastError)
}

lv2import::~lv2import()
{
    DBGFN();
    pDelete(pQuery);
}

