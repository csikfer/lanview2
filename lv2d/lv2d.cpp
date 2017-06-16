#include "lv2d.h"
#include "time.h"
#include <stdio.h>
#define VERSION_MAJOR   0
#define VERSION_MINOR   99
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

#define COREDUMP 1

const QString& setAppHelp()
{
    return _sNul;
}

int main(int argc, char *argv[])
{
    printf("Start lv2d...\n");
    cLv2QApp app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2d    mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
#if COREDUMP
    static const QString cmd = "ulimit -c 1000000";
    int rr = QProcess::execute(cmd);
    PDEB(INFO) << QString("Exec : \"%1\", return : %2").arg(cmd).arg(rr) << endl;
#endif

    int r = mo.pSelfInspector->inspectorType;
    r = r & (IT_TIMING_POLLING | IT_PROCESS_POLLING);
    if (r) {
        PDEB(INFO) << QObject::trUtf8("Nothing start event loop, exit OK.") << endl;
        exit(0);
    }

    PDEB(INFO) << QObject::trUtf8("Start event loop ...") << endl;
    r = app.exec();

    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);
}

lv2d::lv2d()
{
    pSelfInspector = NULL;
    if (lastError == NULL) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            setup(TS_FALSE);
        } CATCHS(lastError)
    }
}

lv2d::~lv2d()
{
    down();
}

void lv2d::setup(eTristate _tr)
{

    tSetup<cSysInspector>(_tr);
}
