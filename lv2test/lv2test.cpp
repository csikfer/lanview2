#include "lv2test.h"
#include "lv2service.h"
#include "others.h"
#include <QCoreApplication>

#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

void setAppHelp()
{
}

int main(int argc, char *argv[])
{
    cLv2QApp app(argc, argv);
    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2test   mo;
    if (mo.lastError != NULL) {
        _DBGFNL() << mo.lastError->mErrorCode << endl;
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    try {
/*        cRecordAny a1;
        cRecordAny a2;
        cNode       n;
        cSnmpDevice s;
        a1 = n;
        a2 = s;
        n.clone(a1);
        s.clone(a2);
        n.clone(a2);
        n.clone(n);
        n.clone(s);
        s.clone(a1);*/
        QString exp;
        QSqlQuery q = getQuery();

/*        cUser u;
        if (u.fetch(*mo.pq, false, QBitArray(1))) {
            do {
                exp = u.objectExport(q, 1);
                PDEB(INFO) << exp << endl;
            } while (u.next(*mo.pq));
        }


        cEnumVal oev;
        if (oev.fetch(*mo.pq, false, QBitArray(1))) {
            do {
                exp = oev.objectExport(q, 1);
                PDEB(INFO) << exp << endl;
            } while (oev.next(*mo.pq));
        }*/
        cTableShape ts;
        ts.setByName(q, "nodes");
        exp = ts.objectExport(q);
        PDEB(INFO) << exp << endl;

    } CATCHS(mo.lastError)
    return mo.lastError == NULL ? 0 : mo.lastError->mErrorCode;
}

lv2test::lv2test() : lanView()
{
    if (lastError != NULL) {
        pq     = NULL;
        return;
    }
    try {
        pq = newQuery();
    } CATCHS(lastError)
}

lv2test::~lv2test()
{
    DBGFN();
    if (pq     != NULL) {
        PDEB(VVERBOSE) << "delete pq ..." << endl;
        delete pq;
    }
    DBGFNL();
}
void lv2test::dbNotif(QString __s)
{
    _DBGFN() << __s;
}
