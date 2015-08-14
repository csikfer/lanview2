#include "cmddisp.h"

#define VERSION_MAJOR   0
#define VERSION_MINOR   02
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}
/*
#ifdef Q_OS_WIN
#include <Windows.h>
void Console()
{
    AllocConsole();
    FILE *pFileCon = NULL;
    pFileCon = freopen("CONOUT$", "w", stdout);

    COORD coordInfo;
    coordInfo.X = 130;
    coordInfo.Y = 9000;

    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coordInfo);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),ENABLE_QUICK_EDIT_MODE| ENABLE_EXTENDED_FLAGS);

    fprintf(pFileCon, "Print to *pFileCon\n");
    printf("Print to stdout\n");
}
#endif
*/
int main (int argc, char * argv[])
{
/*
#ifdef Q_OS_WIN
    Console();
#endif
*/
    cLv2QApp app(argc, argv);

    SETAPP();
    lanView::sqlNeeded  = true;

    lv2CmdDisp   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    exit(0);
}

lv2CmdDisp::lv2CmdDisp() : lanView()
{
    pq    = NULL;
    pSelf = NULL;
    if (lastError == NULL) {
        try {
            pq = newQuery();
            pSelf = new cCmdDispatcher(*pq, APPNAME);
            if (pSelf->inspectorType & IT_TIMING_TIMEDTHREAD) {
                sqlBegin(*pq);
                insertStart(*pq);
                sqlEnd(*pq);
            }
            setup();
        } CATCHS(lastError)
    }
}

lv2CmdDisp::~lv2CmdDisp()
{
    pDelete(pSelf);
}

void lv2CmdDisp::setup()
{
    sqlBegin(*pq);
    pSelf->postInit(*pq);
    if (pSelf->pSubordinates == NULL || pSelf->pSubordinates->isEmpty()) EXCEPTION(NOTODO);
    sqlEnd(*pq);
    pSelf->start();
}


/******************************************************************************/

cCmdDispatcher::cCmdDispatcher(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    _DBGOBJ() << name() << endl;
}

cCmdDispatcher::~cCmdDispatcher()
{
    _DBGOBJ() << name() << endl;
}

cInspector * cCmdDispatcher::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cCmdDispItem(q, hsid, hoid, pid);
}


/* -----------------------------------------------------------------------------*/

cCmdDispItem::cCmdDispItem(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
{
    _DBGOBJ() << name() << endl;
}

cCmdDispItem::~cCmdDispItem()
{
    _DBGOBJ() << name() << endl;
}

cInspector * cCmdDispItem::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cCmdDispSubItem(q, hsid, hoid, pid);
}

/* -----------------------------------------------------------------------------*/

cCmdDispSubItem::cCmdDispSubItem(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
{
    _DBGOBJ() << name() << endl;
}

cCmdDispSubItem::~cCmdDispSubItem()
{
    ;
}
