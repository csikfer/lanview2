#include "portvlan.h"
#include "lv2daterr.h"


#define VERSION_MAJOR   0
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

    lv2portVLan   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // A továbbiakban a timer ütemében történnek az események
    int r = app.exec();
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2portVLan::lv2portVLan() : lanView()
{
    pSelfInspector = nullptr;  // Main service == APPNAME, standard init
    if (lastError == nullptr) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            setup();
        } CATCHS(lastError)
    }
}

lv2portVLan::~lv2portVLan()
{
    down();
}

void lv2portVLan::staticInit(QSqlQuery *pq)
{
    cDevicePV::pSrvSnmp   = cService::service(*pq, _sSnmp);
}

void lv2portVLan::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::tSetup<cPortVLans>(_tr);
}


/******************************************************************************/

cPortVLans::cPortVLans(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    /// $oid_dot1qVlanCurrentEgressPorts   numeric object id: dot1qVlanCurrentEgressPorts
    /// dot1qVlanCurrentEgressPorts.0.<vlan id> = Port map (hex string)
    /// example:
    /// get mib-2.17.7.1.4.2.1.4.0.101
    /// SNMPv2-SMI::mib-2.17.7.1.4.2.1.4.0.101 = Hex-STRING: 00 00 C0 00 00 00 40 00
    ///   vlan id = 101
    ///            0           1            2             3            4              5            6
    ///   port ix: 1234 5678  9012 3456  7890 1234  5678 9012  3456 7890  1234 5678  9012 3456  7890 1234
    ///   bitmap:  0000 0000  0000 0000  1100 0000  0000 0000  0000 0000  0000 0000  0100 0000  0000 0000
    ///      hex:  00         00         C0         00         00         00         40         00
    ///
    ///   id=101 vlan is current eggres to 17,18 and 50 port.
    dot1qVlanCurrentEgressPorts = cOId("SNMPv2-SMI::mib-2.17.7.1.4.2.1.4.0");
    if (!dot1qVlanCurrentEgressPorts) EXCEPTION(ESNMP, dot1qVlanCurrentEgressPorts.status, dot1qVlanCurrentEgressPorts.emsg);
    /// $oid_dot1qVlanStaticEgressPorts   numeric object id: dot1qVlanStaticEgressPorts
    /// dot1qVlanStaticEgressPorts.0.<vlan id> = Port map (hex string)
    /// example: eg.: dot1qVlanCurrentEgressPorts
    dot1qVlanStaticEgressPorts = cOId("SNMPv2-SMI::mib-2.17.7.1.4.3.1.2");
    if (!dot1qVlanStaticEgressPorts) EXCEPTION(ESNMP, dot1qVlanStaticEgressPorts.status, dot1qVlanStaticEgressPorts.emsg);
    /// $oid_dot1qVlanStaticForbiddenEgressPorts   numeric object id: dot1qVlanStaticForbiddenEgressPorts
    /// dot1qVlanStaticForbiddenEgressPorts.0.<vlan id> = Port map (hex string)
    /// example: eg.: dot1qVlanCurrentEgressPorts
    dot1qVlanStaticForbiddenEgressPorts = cOId("SNMPv2-SMI::mib-2.17.7.1.4.3.1.3");
    if (!dot1qVlanStaticForbiddenEgressPorts) EXCEPTION(ESNMP, dot1qVlanStaticForbiddenEgressPorts.status, dot1qVlanStaticForbiddenEgressPorts.emsg);
    /// $oid_dot1qPvid   numeric object id: dot1qPvid
    /// dot1qPvid.<port id> = <vlan id>
    dot1qPvid = cOId("SNMPv2-SMI::mib-2.17.7.1.4.5.1.1");
    if (!dot1qPvid) EXCEPTION(ESNMP, dot1qPvid.status, dot1qPvid.emsg);
}

cPortVLans::~cPortVLans()
{
    ;
}

cInspector * cPortVLans::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePV(q, hsid, hoid, pid);
}

/******************************************************************************/

const cService *cDevicePV::pSrvSnmp   = nullptr;

cDevicePV::cDevicePV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    // Ha nincs megadva protocol szervíz ('nil'), akkor SNMP device esetén az az SNMP lesz
    if (protoServiceId() == NIL_SERVICE_ID && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    if (protoServiceId() == pSrvSnmp->getId()) {
        snmpDev().open(__q, snmp);
    }
    else {  // Csak az SNMP lekérdezés támogatott (egyenlőre)
        EXCEPTION(EDATA, protoServiceId(), QObject::tr("Nem támogatott proto_service_id!"));
    }
}

cDevicePV::~cDevicePV()
{
    ;
}

#define _DM PDEB(VVERBOSE)
#define DM  _DM << endl;

#define MIN_VLAN_ID 1
#define MAX_VLAN_ID 4097

inline static bool maps2bool(const QMap<int, QBitArray>& maps, int mapix, int bitix) {
    if (bitix <= 0) return false;   // A valós index: 1,2...
    if (!maps.contains(mapix)) return false;
    const QBitArray& ba = maps[mapix];
    if (ba.size() < bitix) return false;
    return ba[bitix -1];
}

int cDevicePV::run(QSqlQuery& q, QString &runMsg)
{
    enum eNotifSwitch rs = RS_ON;
    _DBGFN() << QChar(' ') << name() << endl;
    // qlonglong parid = parentId(EX_IGNORE);
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::tr("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    {   // Az eszköz portjaihoz tartozó vlan kapcsoló rekordok: mind jelöletlen
        static const QString sql = "UPDATE port_vlans SET flag = false WHERE port_id IN (SELECT port_id FROM nports WHERE node_id = ?)";
        execSql(q, sql, nodeId());
    }
    QMap<int, QBitArray>    currentMaps;
    QMap<int, QBitArray>    staticMaps;
    QMap<int, QBitArray>    forbidMaps;
    QMap<int, int>          pvidMap;
    const cPortVLans& par = dynamic_cast<const cPortVLans&>(parent());
    int r;
    if (0 != (r = snmp.getBitMaps(par.dot1qVlanCurrentEgressPorts,         currentMaps))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticEgressPorts,          staticMaps))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticForbiddenEgressPorts, forbidMaps))
     || 0 != (r = snmp.getXIndex( par.dot1qPvid, pvidMap))) {
        runMsg = tr("SNMP '%1' error : #%2/%3, '%4'.")
                .arg(snmp.name().toString())
                .arg(snmp.status).arg(r)
                .arg(snmp.emsg);
        rs = RS_UNREACHABLE;
    }
    else {
        int ctVlan = 0, ctUnchg = 0, ctMod = 0, ctNew = 0, ctIns = 0, ctUnkn = 0, ctRm = 0;
        for (int vid = MIN_VLAN_ID; vid < MAX_VLAN_ID; ++vid) {
            if (currentMaps.contains(vid) || staticMaps.contains(vid)) {    // Van ilyen VLAN ?
                ++ctVlan;   // Számláló. Talált VLAN-ok száma
                if (node().ports.isEmpty()) node().fetchPorts(q);
                tRecordList<cNPort>::iterator i, n = node().ports.end();
                QString vstat;
                for (i = node().ports.begin(); i != n; ++i) {
                    int pix = (*i)->getId(_sPortIndex);
                    if      (pvidMap.contains(pix) && pvidMap[pix] == vid)  vstat = _sUntagged;
                    else if (maps2bool(staticMaps,  vid, pix))              vstat = _sTagged;
                    else if (maps2bool(currentMaps, vid, pix))              vstat = _sTagged;
                    else if (maps2bool(forbidMaps,  vid, pix))              vstat = _sForbidden;
                    else    continue;
                    QString r = execSqlTextFunction(q, "update_port_vlan", (*i)->getId(), vid, vstat);
                    switch (reasons(r, EX_IGNORE)) {
                    case R_UNCHANGE:    ++ctUnchg;  break;  // Nem volt változás
                    case R_NEW:         ++ctNew;    break;  // Új rekord
                    case R_INSERT:      ++ctIns;    break;  // Új rekord, és uj VLAN rekord is fel lett véve
                    case R_MODIFY:      ++ctMod;    break;  // Modosítva
                    default:            ++ctUnkn;   break;  // ??
                    }
                }
            }
        }
        bool ok;
        // törlendő jelöletlen rekordok
        ctRm = execSqlIntFunction(q, &ok, "rm_unmarked_port_vlan", node().getId());
        runMsg = tr("%1 VLAN, %2 változatlan, %3 új (%4 új VLAN), %5 változás, %6 törölva, %7 ismeretlen/hiba.")
                 .arg(ctVlan).arg(ctUnchg).arg(ctIns + ctNew).arg(ctNew).arg(ctMod).arg(ctRm).arg(ctUnkn);
    }
    DBGFNL();
    return rs;
}

