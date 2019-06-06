#include "arpd.h"
#include "scan.h"


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
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2ArpD::lv2ArpD() : lanView()
{
    pSelfInspector = nullptr;
    if (lastError == nullptr) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            setup(TS_FALSE);
        } CATCHS(lastError)
    }
}

lv2ArpD::~lv2ArpD()
{
    down();
}

void lv2ArpD::staticInit(QSqlQuery *pq)
{
    cDeviceArp::pPSLocal   = cService::service(*pq, _sLocal);
    cDeviceArp::pPSSnmp    = cService::service(*pq, _sSnmp);
    cDeviceArp::pPSSsh     = cService::service(*pq, _sSsh);
    cDeviceArp::pPSArpProc = cService::service(*pq, "arp_proc");
    cDeviceArp::pPSDhcpConf= cService::service(*pq, "dhcp_conf");
    cDeviceArp::pPSDhcpLeases= cService::service(*pq, "dhcp_leases");
}



void lv2ArpD::setup(eTristate _tr)
{
    staticInit(pQuery);
    tSetup<cArpDaemon>(_tr);
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
const cService   *cDeviceArp::pPSLocal     = nullptr;
/// Protokol (SNMP) azonosító objektum pointere
/// ARP lekérdezés módja SNMP lekérdezés
const cService   *cDeviceArp::pPSSnmp      = nullptr;
/// Protokol (SSH) azonosító objektum pointere
/// ARP lekérdezés módja SSH-n keresztül egy távoli állomány felolvasása
const cService   *cDeviceArp::pPSSsh       = nullptr;
/// ARP mode : read DHCPD config
const cService   *cDeviceArp::pPSDhcpConf  = nullptr;
/// ARP mode : read DHCPD config
const cService   *cDeviceArp::pPSDhcpLeases  = nullptr;
/// ARP mode : read /proc/arp file
const cService   *cDeviceArp::pPSArpProc   = nullptr;


cDeviceArp::cDeviceArp(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
{
    pSnmp = nullptr;
    pFileName = nullptr;
    pRemoteUser = nullptr;
    if (*pPSSnmp == protoService()) {
        pSnmp = new cSnmp();
        snmpDev().open(__q, *pSnmp);
    }
    else {
        if (*pPSDhcpConf == primeService() || *pPSArpProc == primeService() || *pPSDhcpLeases == primeService()) {
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
    if (pSnmp     != nullptr) delete pSnmp;
    if (pFileName != nullptr) delete pFileName;
}


int cDeviceArp::run(QSqlQuery& q, QString& runMsg)
{
    _DBGFN() << QChar(' ') << name() << endl;
    int setType = ENUM_INVALID;
    cArpTable at;
    int r = 1;
    if (*pPSSnmp == protoService()) {
        setType = ST_QUERY;     // SNMP lekérdezés:
        at.getBySnmp(*pSnmp);
    }
    else if (*pPSDhcpConf == primeService()) {
        setType = ST_CONFIG;    // DHCP: dhcpd.conf fájl
        if    (*pPSLocal == protoService()) {
            r = at.getByLocalDhcpdConf(*pFileName, hostServiceId());
        }
        else if (*pPSSsh == protoService()) {
            r = at.getBySshDhcpdConf(host().getIpAddress().toString(), *pFileName, *pRemoteUser, hostServiceId());
        }
        else EXCEPTION(EDATA);
    }
    else if (*pPSDhcpLeases == primeService()) {
        setType = ST_QUERY;    // DHCP: dhcpd.leases fájl
        if    (*pPSLocal == protoService()) {
            r = at.getByLocalDhcpdLeases(*pFileName);
        }
        else if (*pPSSsh == protoService()) {
            r = at.getBySshDhcpdLeases(host().getIpAddress().toString(), *pFileName, *pRemoteUser);
        }
        else EXCEPTION(EDATA);
    }
    else if (*pPSArpProc == primeService()) {
        setType = ST_QUERY;     // proc file
        if    (*pPSLocal == protoService()) {
            r = at.getByLocalProcFile(*pFileName, &runMsg);
        }
        else if (*pPSSsh == protoService()) {
            r = at.getBySshProcFile(host().getIpAddress().toString(), *pFileName, *pRemoteUser, &runMsg);
        }
        else EXCEPTION(EDATA);
    }
    else EXCEPTION(EDATA);
    cArp::replaces(q, at, setType, hostServiceId());
    DBGFNL();
    // Ha senki nem dobott kizárást ...
    if (r <  0) return RS_CRITICAL;
    if (r == 0) return RS_WARNING;
    return RS_ON;
}


