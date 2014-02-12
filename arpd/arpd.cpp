#include "arpd.h"

#define VERSION_MAJOR   1
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
    QCoreApplication app(argc, argv);

    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = true;
    lanView::gui        = false;

    lv2ArpD   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // A továbbiakban a timer ütemében történnek az események
    int r = app.exec();
    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);
}

lv2ArpD::lv2ArpD() : lanView()
{
    pq    = NULL;
    pSelf = NULL;
    if (lastError == NULL) {
        try {
            pq = newQuery();

            sqlBegin(*pq);
            insertStart(*pq);
            sqlEnd(*pq);

            if (subsDbNotif(QString(), false)) {
                connect(pDb->driver(), SIGNAL(notification(QString)), SLOT(dbNotif(QString)));
            }

            cDevice::pPSLocal   = &cService::service(*pq, _sLocal);
            cDevice::pPSSnmp    = &cService::service(*pq, _sSnmp);
            cDevice::pPSSsh     = &cService::service(*pq, _sSsh);
            cDevice::pPSArpProc = &cService::service(*pq, "arp.proc");
            cDevice::pPSDhcpConf= &cService::service(*pq, "dhcp.conf");

            setup();
        } catch(cError * e) {
            lastError = e;
        } catch(...) {
            lastError = NEWCERROR(EUNKNOWN);
        }
    }
}

lv2ArpD::~lv2ArpD()
{
    down();
}

bool lv2ArpD::uSigRecv(int __i)
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
void lv2ArpD::dbNotif(QString __s)
{
    if (pSelf != NULL && pSelf->internalStat != IS_RUN) return;
    PDEB(INFO) << QString(trUtf8("EVENT FROM DB : %1")).arg(__s) << endl;
    reSet();
}

void lv2ArpD::setup()
{
    sqlBegin(*pq);
    pSelf = new cArpDaemon(*pq, appName);
    pSelf->postInit(*pq);
    if (pSelf->pSubordinates == NULL || pSelf->pSubordinates->isEmpty()) EXCEPTION(NOTODO);
    sqlEnd(*pq);
    pSelf->start();
}

void lv2ArpD::down()
{
    if (pSelf != NULL) delete pSelf;
    pSelf = NULL;
}

void lv2ArpD::reSet()
{
    pSelf->setInternalStat(IS_REINIT);
    try {
        down();
        setup();
    } catch(cError * e) {
        lastError = e;
    } catch(...) {
        lastError = NEWCERROR(EUNKNOWN);
    }
    if (lastError != NULL) QCoreApplication::exit(lastError->mErrorCode);
}

/******************************************************************************/

cArpDaemon::cArpDaemon(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    ;
}

cArpDaemon::~cArpDaemon()
{
    ;
}

cInspector * cArpDaemon::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevice(q, hsid, hoid, pid);
}

enum eNotifSwitch cArpDaemon::run(QSqlQuery &)
{
    return RS_ON;
}

/******************************************************************************/

const cService   *cDevice::pPSLocal     = NULL;
const cService   *cDevice::pPSSnmp      = NULL;
const cService   *cDevice::pPSSsh       = NULL;
const cService   *cDevice::pPSDhcpConf  = NULL;
const cService   *cDevice::pPSArpProc   = NULL;


cDevice::cDevice(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
{
    pSnmp = NULL;
    pFileName = NULL;
    pRemoteUser = NULL;
    if (*pPSSnmp == protoService()) {
        pSnmp = new cSnmp();
        snmpDev().open(__q, *pSnmp);
    }
    else {
        if (*pPSDhcpConf == primeService() || *pPSArpProc == primeService()) {
            static const QString _sFile("file");
            pFileName = new QString();
            *pFileName = hostService.magicParam(_sFile);
            if (pFileName->isEmpty()) *pFileName = primeService().magicParam(_sFile);
        }
        if (*pPSSsh == protoService()) {
            static const QString _sUser("user");
            pRemoteUser = new QString();
            *pRemoteUser = hostService.magicParam(_sUser);
            if (pRemoteUser->isEmpty()) *pRemoteUser = protoService().magicParam(_sUser);
        }
    }
}

cDevice::~cDevice()
{
    if (pSnmp     != NULL) delete pSnmp;
    if (pFileName != NULL) delete pFileName;
}


enum eNotifSwitch cDevice::run(QSqlQuery& q)
{
    _DBGFN() << _sSpace << name() << endl;
    cArpTable at;
    // SNMP lekérdezés:
    if (*pPSSnmp == protoService()) {
        at.getBySnmp(*pSnmp);
    }
    else if (*pPSDhcpConf == primeService()) {
        if    (*pPSLocal == protoService()) {
            at.getByLocalDhcpdConf(*pFileName);
        }
        else if (*pPSSsh == protoService()) {
            at.getBySshDhcpdConf(host().getIpAddress().toString(), *pFileName, *pRemoteUser);
        }
        else EXCEPTION(EDATA);
    }
    else if (*pPSArpProc == primeService()) {
        if    (*pPSLocal == protoService()) {
            at.getByLocalProcFile(*pFileName);
        }
        else if (*pPSSsh == protoService()) {
            at.getBySshProcFile(host().getIpAddress().toString(), *pFileName, *pRemoteUser);
        }
        else EXCEPTION(EDATA);
    }
    else EXCEPTION(EDATA);
    cArp::replaces(q, at);
    DBGFNL();
    return RS_ON;
}


