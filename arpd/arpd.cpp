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
    cLv2QApp app(argc, argv);

    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;

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

            insertStart(*pq);

            subsDbNotif();

            cDeviceArp::pPSLocal   = &cService::service(*pq, _sLocal);
            cDeviceArp::pPSSnmp    = &cService::service(*pq, _sSnmp);
            cDeviceArp::pPSSsh     = &cService::service(*pq, _sSsh);
            cDeviceArp::pPSArpProc = &cService::service(*pq, "arp.proc");
            cDeviceArp::pPSDhcpConf= &cService::service(*pq, "dhcp.conf");

            setup();
        } CATCHS(lastError)
    }
}

lv2ArpD::~lv2ArpD()
{
    down();
}

void lv2ArpD::setup()
{
    pSelf = new cArpDaemon(*pq, appName);
    pSelf->postInit(*pq);
    if (pSelf->pSubordinates == NULL || pSelf->pSubordinates->isEmpty()) EXCEPTION(NOTODO);
    pSelf->start();
}

void lv2ArpD::down()
{
    if (pSelf != NULL) delete pSelf;
    pSelf = NULL;
}

void lv2ArpD::reSet()
{
    try {
        down();
        lanView::reSet();
        setup();
    } CATCHS(lastError)
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
    return new cDeviceArp(q, hsid, hoid, pid);
}

/******************************************************************************/
/// Protokol (local) azonosító objektum pointere
/// ARP lekérdezés módja lokális álomány felolvasása
const cService   *cDeviceArp::pPSLocal     = NULL;
/// Protokol (SNMP) azonosító objektum pointere
/// ARP lekérdezés módja SNMP lekérdezés
const cService   *cDeviceArp::pPSSnmp      = NULL;
/// Protokol (SSH) azonosító objektum pointere
/// ARP lekérdezés módja SSH-n keresztül egy távoli állomány felolvasása
const cService   *cDeviceArp::pPSSsh       = NULL;
/// ARP lekérdezés módja a DHCP konfig állomány felolvasása
const cService   *cDeviceArp::pPSDhcpConf  = NULL;
/// ARP lekérdezés módja a proc fájlrendszer arp állományának a felolvasása
const cService   *cDeviceArp::pPSArpProc   = NULL;


cDeviceArp::cDeviceArp(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
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
            pFileName = new QString();
            *pFileName = feature(_sFile);
        }
        if (*pPSSsh == protoService()) {
            pRemoteUser = new QString();
            *pRemoteUser = feature(_sUser);
            host().fetchPorts(*pq);
        }
    }
}

cDeviceArp::~cDeviceArp()
{
    if (pSnmp     != NULL) delete pSnmp;
    if (pFileName != NULL) delete pFileName;
}


enum eNotifSwitch cDeviceArp::run(QSqlQuery& q)
{
    _DBGFN() << QChar(' ') << name() << endl;
    int setType = ST_INVALID;
    cArpTable at;
    if (*pPSSnmp == protoService()) {
        setType = ST_QUERY;     // SNMP lekérdezés:
        at.getBySnmp(*pSnmp);
    }
    else if (*pPSDhcpConf == primeService()) {
        setType = ST_CONFIG;    // DHCP: dhcpd.conf fájl
        if    (*pPSLocal == protoService()) {
            at.getByLocalDhcpdConf(*pFileName, hostServiceId());
        }
        else if (*pPSSsh == protoService()) {
            at.getBySshDhcpdConf(host().getIpAddress().toString(), *pFileName, *pRemoteUser, hostServiceId());
        }
        else EXCEPTION(EDATA);
    }
    else if (*pPSArpProc == primeService()) {
        setType = ST_QUERY;     // proc file
        if    (*pPSLocal == protoService()) {
            at.getByLocalProcFile(*pFileName);
        }
        else if (*pPSSsh == protoService()) {
            at.getBySshProcFile(host().getIpAddress().toString(), *pFileName, *pRemoteUser);
        }
        else EXCEPTION(EDATA);
    }
    else EXCEPTION(EDATA);
    cArp::replaces(q, at, setType, hostServiceId());
    DBGFNL();
    // Ha senki nem dobott kizárást, akkor minden OK
    return RS_ON;
}


