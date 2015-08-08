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
    lanView::sqlNeeded  = true;

    // Elmentjük az aktuális könyvtárt
    QString actDir = QDir::currentPath();
    lv2import   mo;
    if (mo.lastError != NULL) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    PDEB(VVERBOSE) << "Saved original current dir : " << actDir << "; actDir : " << QDir::currentPath() << endl;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
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
            sqlBegin(*mo.pq);
            importParseFile(mo.fileNm);
        } catch(cError *e) {
            mo.lastError = e;
        } catch(...) {
            mo.lastError = NEWCERROR(EUNKNOWN);
        }
        cError *ipe = importGetLastError();
        if (ipe != NULL) mo.lastError = ipe;
        if (mo.lastError) {
            sqlRollback(*mo.pq);
            PDEB(DERROR) << "**** ERROR ****\n" << mo.lastError->msg() << endl;
        }
        else {
            sqlEnd(*mo.pq);
            PDEB(DERROR) << "**** OK ****" << endl;
        }

        cDebug::end();
        return mo.lastError == NULL ? 0 : mo.lastError->mErrorCode;
    }
}

void lv2import::abortOldRecords()
{
    QString sql =
            "UPDATE imports "
            "   SET exec_state = 'aborted',"
                "   ended = CURRENT_TIMESTAMP,"
                "   result_msg = 'Start imports server: old records aborted.'"
            " WHERE exec_state = 'wait' AND exec_state = 'execute'";
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
    pq->finish();
}

void lv2import::dbNotif(QString __s)
{
    cImport     imp;
    try {
        PDEB(INFO) << QString(trUtf8("DB notification : %1.")).arg(__s) << endl;
        imp.setName(_sExecState, _sWait);
        imp.fetch(*pq, false, imp.mask(_sExecState), imp.iTab(_sDateOf), 1);
        if (imp.isEmpty_()) {
            DWAR() << trUtf8("No waitig imports record, dropp notification.") << endl;
            return;
        }
        sqlBegin(*pq);
        imp.setName(_sExecState, _sExecute);
        imp.set(_sStarted, QVariant(QDateTime::currentDateTime()));
        imp.setId(_sPid, QCoreApplication::applicationPid());
        imp.update(*pq, false, imp.mask(_sExecState, _sStarted, _sPid));
        sqlEnd(*pq);
        sqlBegin(*pq);
        importParseText(imp.getName(_sImportText));
    }
    CATCHS(lastError)
    cError *ipe = importGetLastError();
    if (ipe != NULL) lastError = ipe;
    if (lastError == NULL) {
        imp.setName(_sExecState, _sOk);
        imp.setName(_sResultMsg, _sOk);
        imp.set(_sEnded, QVariant(QDateTime::currentDateTime()));
        imp.clear(_sAppLogId);
        imp.update(*pq, false, imp.mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId));
        sqlEnd(*pq);
    }
    else {
        sqlRollback(*pq);
        sqlBegin(*pq);
        qlonglong eid = sendError(lastError);
        imp.setName(_sExecState, _sFaile);
        imp.setName(_sResultMsg, lastError->msg());
        delete lastError;
        lastError = NULL;
        imp.set(_sEnded, QVariant(QDateTime::currentDateTime()));
        imp.setId(_sAppLogId, eid);
        imp.update(*pq, false, imp.mask(_sExecState, _sResultMsg, _sEnded, _sAppLogId));
        sqlEnd(*pq);
    }
    QCoreApplication::exit(0);
}


lv2import::lv2import() : lanView(), fileNm(), in()
{
    daemonMode = false;
    if (lastError != NULL) {
        pq     = NULL;
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
        subsDbNotif();
    }
    if (args.count() > 1) DWAR() << trUtf8("Invalid arguments : ") << args.join(QChar(' ')) << endl;
    try {
        pq = newQuery();
        if (daemonMode) return;
        insertStart(*pq);
        if (!userName.isNull()) setUser(userName);
    } CATCHS(lastError)
}

lv2import::~lv2import()
{
    DBGFN();
    if (pq     != NULL) {
        PDEB(VVERBOSE) << "delete pq ..." << endl;
        delete pq;
    }
    DBGFNL();
}

