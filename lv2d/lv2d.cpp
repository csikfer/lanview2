#include "lv2d.h"
#include "time.h"
#define VERSION_MAJOR   0
#define VERSION_MINOR   99
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main(int argc, char *argv[])
{
    cLv2QApp app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2d    mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }

    int r = mo.pSelf->inspectorType;
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
    pq = NULL;
    pSelf = NULL;
    if (lastError == NULL) {
        try {
            pq = newQuery();

            sqlBegin(*pq);
            insertStart(*pq);
            sqlEnd(*pq);

            subsDbNotif();

            setup();
        } CATCHS(lastError)
    }
}

lv2d::~lv2d()
{
    down();
}

#ifdef MUST_USIGNAL
bool lv2d::uSigRecv(int __i)
{
    if (__i == SIGHUP) {
        if (pSelf != NULL && pSelf->internalStat == IS_RUN) {
            PDEB(INFO) << trUtf8("Sygnal : SIGHUP; reset ...") << endl;
            reSet();
        }
        else {
            PDEB(INFO) << trUtf8("Sygnal : SIGHUP; dropped ...") << endl;
        }
        return false;
    }
    return true;
}
#endif
void lv2d::dbNotif(const QString& name, QSqlDriver::NotificationSource source, const QVariant &payload)
{
    lanView::dbNotif(name, source, payload);    // DEBUG
    if (pSelf != NULL && pSelf->internalStat != IS_RUN) return;
    PDEB(INFO) << trUtf8("Event from DB, call reSet()... ") << endl;
    reSet();
}

void lv2d::setup()
{
    pSelf = new cInspector(*pq, appName);
    pSelf->postInit(*pq);
    pSelf->start();
}

void lv2d::down()
{
    if (pSelf != NULL) {
        delete pSelf;
        pSelf = NULL;
    }
}

void lv2d::reSet()
{
    pSelf->setInternalStat(IS_REINIT);
    try {
        down();
        setup();
    } CATCHS(lastError)
    if (lastError != NULL) QCoreApplication::exit(lastError->mErrorCode);
}

