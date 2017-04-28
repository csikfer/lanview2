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
    lanView::appHelp  = QObject::trUtf8("-u|--user-id <id>           Set user id\n");
    lanView::appHelp += QObject::trUtf8("-U|--user-name <name>       Set user name\n");
    lanView::appHelp += QObject::trUtf8("-i|--input-file <path>      Set input file path\n");
    lanView::appHelp += QObject::trUtf8("-I|--input-stdin            Set input file is stdin\n");
    lanView::appHelp += QObject::trUtf8("-D|--daemon-mode            Set daemon mode\n");
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
    if (mo.lastError != NULL) {
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
            if (mo.fileNm.isEmpty()) EXCEPTION(EDATA, -1, QObject::trUtf8("Nincs megadva forrás fájl!"));
            PDEB(VVERBOSE) << "BEGIN transaction ..." << endl;
            importParseFile(mo.fileNm);
        } catch(cError *e) {
            mo.lastError = e;
        } catch(...) {
            mo.lastError = NEWCERROR(EUNKNOWN);
        }
        cError *ipe = importGetLastError();
        if (ipe != NULL) mo.lastError = ipe;
        if (mo.lastError) {
            PDEB(DERROR) << "**** ERROR ****\n" << mo.lastError->msg() << endl;
        }
        else {
            PDEB(DERROR) << "**** OK ****" << endl;
        }

        cDebug::end();
        return mo.lastError == NULL ? 0 : mo.lastError->mErrorCode;
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
    cImport    *pImp = NULL;
    try {
        PDEB(INFO) << QString(trUtf8("DB notification : %1, %2, %3.")).arg(name).arg((int)source).arg(debVariantToString(payload)) << endl;
        pImp = new cImport;
        pImp->setName(_sExecState, _sWait);
        pImp->fetch(*pQuery, false, pImp->mask(_sExecState), pImp->iTab(_sDateOf), 1);
        if (pImp->isEmpty_()) {
            DWAR() << trUtf8("No waitig imports record, dropp notification.") << endl;
            return;
        }
        pImp->setName(_sExecState, _sExecute);
        pImp->set(_sStarted, QVariant(QDateTime::currentDateTime()));
        pImp->setId(_sPid, QCoreApplication::applicationPid());
        pImp->update(*pQuery, false, pImp->mask(_sExecState, _sStarted, _sPid));
        importParseText(pImp->getName(_sImportText));
    }
    CATCHS(lastError)
    cError *ipe = importGetLastError();
    if (ipe != NULL) lastError = ipe;
    if (lastError == NULL) {
        pImp->setName(_sExecState, _sOk);
        pImp->setName(_sResultMsg, _sOk);
        pImp->set(_sEnded, QVariant(QDateTime::currentDateTime()));
        pImp->clear(_sAppLogId);
        pImp->update(*pQuery, false, pImp->mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId));
    }
    else if (pImp != NULL) {
        qlonglong eid = sendError(lastError);
        pImp->setName(_sExecState, _sFaile);
        pImp->setName(_sResultMsg, lastError->msg());
        delete lastError;
        lastError = NULL;
        pImp->set(_sEnded, QVariant(QDateTime::currentDateTime()));
        pImp->setId(_sAppLogId, eid);
        pImp->update(*pQuery, false, pImp->mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId));
    }
    else {
        EXCEPTION(EPROGFAIL);
    }
    pDelete(pImp);
    QCoreApplication::exit(0);
}


lv2import::lv2import() : lanView(), fileNm(), in()
{
    daemonMode = false;
    if (lastError != NULL) {
        return;
    }

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
        fileNm = args[i + 1];
        args.removeAt(i);
        args.removeAt(i);
    }
    if (0 < (i = findArg('I', "input-stdin", args))) {
        fileNm = '-';
        args.removeAt(i);
    }
    if (0 < (i = findArg('D', "daemon-mode", args))) {
        args.removeAt(i);;
        PDEB(INFO) << trUtf8("Set daemon mode.") << endl;
        daemonMode = true;
    }
    if (args.count() > 1) DWAR() << trUtf8("Invalid arguments : ") << args.join(QChar(' ')) << endl;
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
}

