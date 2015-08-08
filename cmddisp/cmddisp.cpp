#include "cmddisp.h"

#define VERSION_MAJOR   1
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
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

         /* sqlBegin(*pq);
            insertStart(*pq);
            sqlEnd(*pq);
         */

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
    pSelf = new cCmdDispatcher(*pq, appName);
    QString sql = "UPDATE host_services "
            "SET superior_host_service_id = ? "
            "WHERE prime_service_id = ?";
    execSql(*pq, sql, pSelf->hostServiceId(), pSelf->serviceId());
    pSelf->postInit(*pq);
    if (pSelf->pSubordinates == NULL || pSelf->pSubordinates->isEmpty()) EXCEPTION(NOTODO);
    sqlEnd(*pq);
    pSelf->start();
}


/******************************************************************************/

cCmdDispatcher::cCmdDispatcher(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    ;
}

cCmdDispatcher::~cCmdDispatcher()
{
    ;
}

cInspector * cCmdDispatcher::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cCmdDispItem(q, hsid, hoid, pid);
}


cCmdDispItem::cCmdDispItem(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
{
    ;
}

cCmdDispItem::~cCmdDispItem()
{
    ;
}


