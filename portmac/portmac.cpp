#include "portmac.h"
#include "lv2daterr.h"

#define VERSION_MAJOR   1
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

bool appMemoOn = false;

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

    lv2portMac   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // A továbbiakban a timer ütemében történnek az események
    int r = app.exec();
    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);
}

lv2portMac::lv2portMac() : lanView()
{
    if (lastError == NULL) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            appMemoOn = cSysParam::getBoolSysParam(*pQuery, "app_memo_on", false);
            setup(TS_FALSE);
        } CATCHS(lastError)
    }
}

lv2portMac::~lv2portMac()
{
    down();
}

void lv2portMac::setup(eTristate _tr)
{
    staticInit(pQuery);
    tSetup<cPortMac>(_tr);
}

void lv2portMac::staticInit(QSqlQuery *pq)
{
    cDevicePMac::pSrvSnmp = cService::service(*pq, _sSnmp);
    cDevicePMac::pSrvPMac = cService::service(*pq, "pmac");
}

/******************************************************************************/

cPortMac::cPortMac(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    cDevicePMac::pOId1 = new cOId("SNMPv2-SMI::mib-2.17.4.3.1.2");
    cDevicePMac::pOId2 = new cOId("SNMPv2-SMI::mib-2.17.7.1.2.2.1.2");
    cDevicePMac::pOIdx = new cOId("SNMPv2-SMI::mib-2.17.1.4.1.2");

    if (!*cDevicePMac::pOId1) EXCEPTION(ESNMP, cDevicePMac::pOId1->status, cDevicePMac::pOId1->emsg);
    if (!*cDevicePMac::pOId2) EXCEPTION(ESNMP, cDevicePMac::pOId2->status, cDevicePMac::pOId2->emsg);
    if (!*cDevicePMac::pOIdx) EXCEPTION(ESNMP, cDevicePMac::pOIdx->status, cDevicePMac::pOIdx->emsg);
}

cPortMac::~cPortMac()
{
    ;
}

cInspector * cPortMac::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePMac(q, hsid, hoid, pid);
}

///
int cPortMac::run(QSqlQuery &__q, QString& runMsg)
{
    (void)runMsg;
    cMacTab::refresStats(__q);
    return RS_ON;
}

/******************************************************************************/

const cService *cDevicePMac::pSrvSnmp = NULL;
const cService *cDevicePMac::pSrvPMac = NULL;
cOId    *cDevicePMac::pOId1 = NULL;
cOId    *cDevicePMac::pOId2 = NULL;
cOId    *cDevicePMac::pOIdx = NULL;

cDevicePMac::cDevicePMac(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    _DBGFN() << hostService.fullName(__q, EX_IGNORE) << endl;
    QString msg;
    static const qlonglong suspectrdUpLinkTypeId        = cParamType().getIdByName(__q, _sSuspectedUplink);
    static const qlonglong queryMacTabTypeId            = cParamType().getIdByName(__q, _sQueryMacTab);
    static const qlonglong linkIsInvisibleForLLDPTypeId = cParamType().getIdByName(__q, _sLinkIsInvisibleForLLDP);
    // Ha nincs megadva protocol szervíz ('nil'), akkor SNMP device esetén az az SNMP lessz
    if (protoServiceId() == NIL_SERVICE_ID && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    // Csak az SNMP lekérdezés támogatott
    if (protoServiceId() != pSrvSnmp->getId()) {
        EXCEPTION(EDATA, protoServiceId(), trUtf8("Nem megfelelő proto_service_id!"));
    }
    // Beolvassuk a portokat is
    host().fetchPorts(__q, 0);  // IP cím VLAN nem kell
    // host().fetchParams(__q);
    tRecordList<cNPort>::iterator   i;
    // Csinálunk a releváns portokhoz egy index táblát
    QMap<int,int>   rxix;   // Kereszt index tábla, a lekérdezésnél használlt indexekre konvertáláshoz
    snmpDev().open(__q, snmp);
    int r = snmp.getXIndex(*pOIdx, rxix, true);
    if (r) {
        EXCEPTION(ESNMP, r, trUtf8("A kereszt index tábla lekérdezése sikertelen : %1").arg(name()));
    }
    for (i = host().ports.begin(); i < host().ports.end(); ++i) {
        cNPort  &np = **i;
        int ix = -1;
        if (np.descr() < cInterface::_descr_cInterface()) continue; // buta portok érdektelenek
        np.fetchParams(__q);
        // Ha van "query_mac_tab" paraméter, és hamis, akkor tiltott a lekérdezés a portra
        eTristate queryFlag = np.getBoolParam(queryMacTabTypeId, EX_IGNORE);
        // Ha van "suspected_uplink" paraméter, és igaz, akkor nem foglalkozunk vele (csiki-csuki elkerülése)
        eTristate suspFlag  = np.getBoolParam(suspectrdUpLinkTypeId, EX_IGNORE);
        if (queryFlag == TS_FALSE || suspFlag == TS_TRUE) {
            PDEB(VERBOSE) << trUtf8("Disable %1 port query, suspFlag = %2, queryFlag = %3").arg(np.getName()).arg((int)suspFlag).arg((int)queryFlag) << endl; 
            continue;
        }
        QString ifTypeName = np.ifType().getName();
        if (ifTypeName == _sEthernet) {
            // Ha ez egy TRUNK tagja, akkor nem érdekes.
            if (!np.isNull(_sPortStapleId)) continue;
            // Ki kéne még hajítani az uplinkeket
            cLldpLink lldp;
            if (NULL_ID != lldp.getLinked(__q, np.getId())) { // Ez egy LLDP-vel felderített link
                // Ha van "query_mac_tab" paraméter, és igaz, akkor a link ellenére (bármi legyen is) lekérdezzük
                if (queryFlag == TS_TRUE) {
                    PDEB(VERBOSE) << trUtf8("Force query %1 port.").arg(np.getName()) << endl;
                }
                else {
                    // De ez uplink? Az ellenoldali node-nal lekérdezzük a címtáblát?
                    const QString sql =
                            "SELECT COUNT(*)"
                            " FROM nports AS p"
                            " JOIN host_services USING(node_id)"
                            " WHERE p.port_id = ? AND service_id = ?";
                    bool ok = execSql(__q, sql, lldp.get(_sPortId2), pSrvPMac->getId());
                    int n;
                    if (ok) n = __q.value(0).toInt(&ok);
                    if (!ok) EXCEPTION(EDATA, 0, sql);
                    if (n > 0) continue;    // Uplink, kihagyjuk
                }
            }
            // mehet a ports konténerbe az indexe
        }
        else if (ifTypeName == _sMultiplexor) {
            // TRUNK-nal a TRUNK tagjaihoz van rendelve az uplink
            ix = -1;
            // Erre a node-ra megy a link (ha marad NULL_ID, akkor nincs uplink)
            qlonglong linkedNodeId = NULL_ID;
            while (true) {
                int st = ix + 1;    // Find start index: st = 0 or next port index
                // Find trunk members
                ix = host().ports.indexOf(_sPortStapleId, np.get(_sPortId), st);
                if (ix < 0) {
                    if (st == 0 && appMemoOn) {  // Empty trunk
                        msg = trUtf8("A %1:%2 trunk-nek nincs egyetlen tagja sem.").arg(host().getName(), np.getName());
                        APPMEMO(__q, msg, RS_ON);
                    }
                    break;  // Nincs, vagy nincs több
                }
                // port ID
                qlonglong pid = host().ports[ix]->getId();
                qlonglong pid_lldp = cLldpLink().getLinked(__q, pid);   // LLDP-vel felfedezett link
                qlonglong pid_log  = cLogLink(). getLinked(__q, pid);   // Logikai (manuálisan rögzített) link
                // host ID
                qlonglong hid = NULL_ID;
                cNPort    lp;
                if (pid_lldp != NULL_ID && pid_log != NULL_ID && pid_lldp != pid_log) { // Antagonism.
                    msg = trUtf8("A %1:%2 trunk %3 tagjáhozrendelt linkek ellentmondásosak: LLDP : %4; Logikai : %5")
                            .arg(host().getName(), np.getName(), host().ports[ix]->getName()
                                 , cNPort::getFullNameById(__q, pid_lldp), cNPort::getFullNameById(__q, pid_log));
                    APPMEMO(__q, msg, RS_CRITICAL);
                    pid_log = NULL_ID;  // Az LLDP-t hisszük el.
                }
                if (pid_lldp != NULL_ID || pid_log != NULL_ID) {                        // Agreement.
                    hid = lp.setById(__q, pid_lldp == NULL_ID ? pid_log : pid_lldp).getId(_sNodeId);
                }
                else {                                                                  // No information about LLDP
                    // Ha van "link_is_invisible_for_LLDP" paraméter, és igaz, akkor nem pampogunk a link hiánya miatt
                    eTristate f = host().ports[ix]->getBoolParam(linkIsInvisibleForLLDPTypeId, EX_IGNORE);
                    if (f == TS_TRUE) continue;
                    msg = trUtf8("A %1:%2 trunk %3 tagjához nincs link rendelve.")
                            .arg(host().getName(), np.getName(), host().ports[ix]->getName());
                    APPMEMO(__q, msg, RS_WARNING);
                    continue;
                }
                if (linkedNodeId == NULL_ID) {  // There is no reference link (first)
                    linkedNodeId = hid;
                }
                else {
                    if (linkedNodeId != hid) {
                        msg = trUtf8("A %1:%2 trunk %3 tagja %4 -el, előző tagja %5 -al van linkben.")
                                .arg(host().getName(), np.getName(), host().ports[ix]->getName())
                                .arg(cNode().getNameById(__q, hid, EX_IGNORE), lp.getFullName(__q, EX_IGNORE));
                        cAlarm::ticket(__q, RS_CRITICAL, msg, hostServiceId());
                    }
                }
            }
            // A Trunk-ot nem kérdezzük le, mert valószínüleg uplink, hacsak nincs "query_mac_tab = true"
            if (queryFlag != TS_TRUE) continue;
        }
        else {
            // Más típusú port nem érdekes.
            continue;
        }
        ix = (int)np.getId(_sPortIndex);    // Eredeti port index
        if (rxix.contains(ix)) {            // Van ilyen a kereszt index-ben?
            ix = rxix[ix];                  // a lekérdezésben ezt az indexet fogja megkapni
            PDEB(VERBOSE) << trUtf8("Set query %1 port, inex %2 (%3).").arg(np.getName()).arg(ix).arg(np.getId(_sPortIndex)) << endl;
            ports.insert(ix, np.reconvert<cInterface>());
        }
        else {
            PDEB(VERBOSE) << trUtf8("No set query %1 port, inex %2 not found.").arg(np.getName()).arg(ix) << endl;
        }
    }
}

cDevicePMac::~cDevicePMac()
{
    ;
}

void cDevicePMac::postInit(QSqlQuery &q, const QString&)
{
    DBGFN();
    cInspector::postInit(q);    // Beolvassa a MAC figyelő al szolgálltatásokat (rightmac)
    // snmpDev().open(q, snmp);
}

cInspector *cDevicePMac::newSubordinate(QSqlQuery &_q, qlonglong _hsid, qlonglong _toid, cInspector *_par)
{
     cRightMac *p = new cRightMac(_q, _hsid, _toid, _par);
     if (p->flag) return p; // Ha nem volt hiba
     delete p;              // Ha valami nem OK, töröljük, a statust a konstruktor beállította.
     return NULL;           // Ez az elem nem kerül a listába
}

int cDevicePMac::run(QSqlQuery& q, QString& runMsg)
{
    (void)runMsg;
    _DBGFN() << QChar(' ') << name() << endl;
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name()));
    }
    QMap<cMac, int> macs;
    enum eNotifSwitch r;

    if (RS_ON != (r = snmpQuery(*pOId1, macs, runMsg))) return r;   // Az index nem feltétlenül az eredeti port index!
    if (RS_ON != (r = snmpQuery(*pOId2, macs, runMsg))) return r;

    foundMacs.clear();
    QMap<cMac, int>::iterator   i;
    for (i = macs.begin(); i != macs.end(); ++i) {
        const int   ix  = i.value();
        QMap<int, cInterface *>::iterator pi = ports.find(ix);
        if (pi == ports.end()) continue;    // Csak a konténerben lévő portok érdekesek
        cMacTab mt;
        cMac mac = i.key();
        qlonglong pid = pi.value()->getId();
        mt.setMac(_sHwAddress, mac);
        mt.setId(_sPortId, pid);
        int r = mt.replace(q);
        if (r == R_DISCARD) ports.remove(ix);   // Lett neki egy "suspected_uplink" paramétere, igaz értékkel!
        else {
            foundMacs[pid] |= mac;  // Talált MAC-ek a rightmac-hoz
        }
    }
    QMap<qlonglong, cRightMac *>::iterator j;
    for (j = rightMap.begin(); j != rightMap.end(); ++j) {
        qlonglong pid = j.key();
        j.value()->checkMacs(q, foundMacs[pid]);
    }

    DBGFNL();
    return RS_ON;
}

enum eNotifSwitch cDevicePMac::snmpQuery(const cOId& __o, QMap<cMac, int>& macs, QString& runMsg)
{
    _DBGFN() << QChar(' ') << __o.toString() << endl;
    cOId    o = __o;
    do {
        int r = snmp.getNext(o);
        if (r) {
            runMsg = msgCat(runMsg,
                    trUtf8("SNMP hiba #%1 (%2). OID:%3")
                    .arg(r).arg(snmp.emsg)
                    .arg(__o.toString()), "\n");
            return RS_CRITICAL;
        }
        o = snmp.name();
        if (!(__o < o)) break;
        bool ok;
        int pix = snmp.value().toInt(&ok);
        if (!ok) {
            runMsg = msgCat(runMsg,
                    trUtf8("Az SNMP válaszban: nem értelmezhető index. OID:%1: %2 = %3")
                    .arg(__o.toString()).arg(o.toString())
                    .arg(debVariantToString(snmp.value())), "\n");
            return RS_CRITICAL;
        }
        if (!(ports.contains(pix))) continue;
        cMac mac = snmp.name().toMac();
        if (!mac) {
            runMsg =  msgCat(runMsg,
                    trUtf8("Az SNMP válaszban: nem értelmezhető MAC. OID:%1: %2 = %3")
                    .arg(__o.toString()).arg(o.toString())
                    .arg(snmp.value().toString()), "\n");
            continue;       // előfordul
        }
        macs.insert(mac, pix); // insert or replace
        PDEB(VVERBOSE) << "#" << pix << ":" << mac.toString() << endl;
    } while(true);
    DBGFNL();
    return RS_ON;
}

qlonglong cRightMac::rightMacId = NULL_ID;

cRightMac::cRightMac(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
{
    flag = true;   // Most hibajelzésre használjuk
    QSqlQuery q = getQuery();
    if (rightMacId == NULL_ID) {
        rightMacId = cService::service(q, "rightmac")->getId();
    }
    QString msg;
    if (rightMacId != serviceId()) {
        flag = false;   // Nem lehet más szolgáltatás, hibás konfig !
        msg = trUtf8("Nem támogatott szervíz : %1. Itt csak a 'rightmac' szervíz támogatott.").arg(service()->getName());
    }
    else {
        if (pPort == NULL || 0 != pPort->chkObjType<cInterface>(EX_IGNORE)) {
            flag = false;
            if (pPort == NULL) msg = trUtf8("Nincs megadva port.\n");
            else               msg = trUtf8("A port típusa csak interface lehet.\n");
        }
        else {
            QString sMacList  = feature("MAC");
            QString sNodeList = feature("node");
            const QString sep = ",";
            if (!sMacList.isEmpty()) {
                foreach (QString sMac, sMacList.split(sep)) {
                    cMac mac(sMac.simplified());
                    if (mac.isValid()) rightMacs |= mac;
                    else {
                        msg += trUtf8("Helytelen MAC : %1\n").arg(sMac);
                        flag = false;
                    }
                }
            }
            cNode node;
            if (!sNodeList.isEmpty()) {
                foreach (QString sNode, sNodeList.split(sep)) {
                    if (node.fetchByName(q, sNode.simplified())) {
                        node.fetchPorts(q, 0);  // csak a portokat olvassuk be
                        rightMacs |= node.getMacs().toSet();
                    }
                    else {
                        msg += trUtf8("Helytelen eszköz név : %1\n").arg(sNode);
                        flag = false;
                    }
                }
            }
            // Van logikai (end to end) link ?
            qlonglong lpid = cLogLink().getLinked(q, portId());
            if (lpid != NULL_ID) {
                cNPort *p = cNPort::getPortObjById(q, lpid);
                if (0 == p->chkObjType<cInterface>(EX_IGNORE) && !p->isNull(_sHwAddress)) {
                    rightMacs |= p->getMac(_sHwAddress);
                }
            }
            if (flag && rightMacs.isEmpty()) {
                msg += ("A MAC fehér lista üres.");
            }
        }
    }
    if (!flag) {    // Hiba
        hostService.setState(q, _sUnreachable, msg);
    }
    else {
        ((cDevicePMac&)parent()).rightMap[portId()] = this;
    }
}

void cRightMac::checkMacs(QSqlQuery& q, const QSet<cMac>& macs)
{
    QString stat, msg;
    if (macs.isEmpty()) {
        stat = _sOn;
        msg  = trUtf8("Nincs talált MAC.");
    }
    else {
        static const QString sep = ", ";
        QStringList slAuth, slUnauth;
        QSet<cMac> unauth = macs - rightMacs;
        QSet<cMac> auth   = macs & rightMacs;
        foreach (cMac mac, auth) {
            slAuth << mac.toString();
        }
        foreach (cMac mac, unauth) {
            slUnauth << mac.toString();
        }
        if (unauth.isEmpty()) {
            stat = _sOn;
            msg  = trUtf8("Nincs engedélyezetlen MAC. Talált cím lista : %1").arg(slAuth.join(sep));
        }
        else {
            stat = _sCritical;
            msg  = trUtf8("Engedélyezetlen MAC : %1. Talált cím lista : %2").arg(slUnauth.join(sep) ,slAuth.join(sep));
        }
    }
    hostService.setState(q, stat, msg);
}
