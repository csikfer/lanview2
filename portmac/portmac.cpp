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
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2portMac::lv2portMac() : lanView()
{
    if (lastError == nullptr) {
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

const cService *cDevicePMac::pSrvSnmp = nullptr;
const cService *cDevicePMac::pSrvPMac = nullptr;
cOId    *cDevicePMac::pOId1 = nullptr;
cOId    *cDevicePMac::pOId2 = nullptr;
cOId    *cDevicePMac::pOIdx = nullptr;

cDevicePMac::cDevicePMac(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , ports(), rightMap(), foundMacs(), communityList(), hostAddress()
{
    _DBGFN() << hostService.fullName(__q, EX_IGNORE) << endl;
    // Ha nincs megadva protocol szervíz ('nil'), akkor SNMP device esetén az az SNMP lesz
    if (protoServiceId() == NIL_SERVICE_ID && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    // Csak az SNMP lekérdezés támogatott
    if (protoServiceId() != pSrvSnmp->getId()) {
        EXCEPTION(EDATA, protoServiceId(), tr("Nem megfelelő proto_service_id!"));
    }
    // Beolvassuk a portokat is
    host().fetchPorts(__q, CV_PORTS_ADDRESSES);  // VLAN nem kell
    QHostAddress a = host().getIpAddress();
    if (a.isNull()) {
        EXCEPTION(EDATA, protoServiceId(), tr("Nincs IP cím!"));
    }
    hostAddress = a.toString();
    snmpVersion = cSnmpDevice().snmpVersion();
    QStringList vlans = features().slValue(_sVLans);
    QString sCommunity = host().getName(_sCommunityRd);
    if (vlans.isEmpty()) {
        communityList << sCommunity;
    }
    else {
        for (auto s: vlans) {
            communityList << (sCommunity + "@" + s);
        }
    }
}

cDevicePMac::~cDevicePMac()
{

}

void cDevicePMac::postInit(QSqlQuery &q)
{
    DBGFN();
    cInspector::postInit(q);    // Beolvassa a MAC figyelő al szolgálltatásokat (rightmac)
}

cInspector *cDevicePMac::newSubordinate(QSqlQuery &_q, qlonglong _hsid, qlonglong _toid, cInspector *_par)
{
     cRightMac *p = new cRightMac(_q, _hsid, _toid, _par);
     if (p->flag) return p; // Ha nem volt hiba
     delete p;              // Ha valami nem OK, töröljük, a statust a konstruktor beállította.
     return nullptr;        // Ez az elem nem kerül a listába
}

int cDevicePMac::initQuery(QSqlQuery& q, QString &runMsg)
{
    int r;
    tRecordList<cNPort>::iterator   i;
    // Csinálunk a releváns portokhoz egy index táblát
    QMap<int,int>   rxix;   // Kereszt index tábla, a lekérdezésnél használlt indexekre konvertáláshoz
    for (auto sCommunity: communityList) {
        cSnmp   snmp;
        r = snmp.open(hostAddress, sCommunity, snmpVersion);
        if (r) {
            msgAppend(&runMsg, tr("%1 : SNMP open error : %2").arg(name(), snmp.emsg));
            return RS_UNREACHABLE;
        }
        int r = snmp.getXIndex(*pOIdx, rxix, true);
        if (r || (rxix.isEmpty() && communityList.size() == 1)) {
            msgAppend(&runMsg, tr("%1 : A kereszt index tábla lekérdezése sikertelen : %2").arg(name(), snmp.emsg));
            return RS_UNREACHABLE;
        }
    }
    if (rxix.isEmpty()) {
        msgAppend(&runMsg, tr("%1 : A kereszt index tábla üres.").arg(name()));
        return RS_UNREACHABLE;
    }
    for (i = host().ports.begin(); i < host().ports.end(); ++i) {
        cNPort  &np = **i;
        int ix = -1;
        if (np.descr() < cInterface::_descr_cInterface()) continue; // buta portok érdektelenek
        np.fetchParams(q);
        // Ha van "query_mac_tab" paraméter, és hamis, akkor tiltott a lekérdezés a portra
        eTristate queryFlag = np.getBoolParam(_sQueryMacTab, EX_IGNORE);
        // Ha van "suspected_uplink" paraméter, és igaz, akkor nem foglalkozunk vele (csiki-csuki elkerülése)
        eTristate suspFlag  = np.getBoolParam(_sSuspectedUplink, EX_IGNORE);
        if (queryFlag == TS_FALSE || suspFlag == TS_TRUE) {
            PDEB(INFO) << tr("Disable %1:%2 port query, suspFlag = %3, queryFlag = %4").arg(host().getName(), np.getName(), tristate2string(suspFlag), tristate2string(queryFlag)) << endl;
            continue;
        }
        QString ifTypeName = np.ifType().getName();
        if (ifTypeName == _sEthernet) {
            // Ha ez egy TRUNK tagja, akkor nem érdekes.
            if (!np.isNull(_sPortStapleId)) continue;
            // Ki kéne még hajítani az uplinkeket
            cLldpLink lldp;
            if (NULL_ID != lldp.getLinked(q, np.getId())) { // Ez egy LLDP-vel felderített link
                // Ha van "query_mac_tab" paraméter, és igaz, akkor a link ellenére (bármi legyen is) lekérdezzük
                if (queryFlag == TS_TRUE) {
                    PDEB(INFO) << tr("Force query %1 port.").arg(np.getName()) << endl;
                }
                else {
                    qlonglong linkedPortId = lldp.getId(_sPortId2);
                    // De ez uplink? Az ellenoldali node-nal lekérdezzük a címtáblát?
                    const QString sql =
                            "SELECT COUNT(*)"
                            " FROM nports AS p"
                            " JOIN host_services AS hs USING(node_id)"
                            " WHERE p.port_id = ? AND hs.service_id = ?";
                    bool ok = execSql(q, sql, linkedPortId, pSrvPMac->getId());
                    int n = 1;
                    if (ok) n = q.value(0).toInt(&ok);
                    if (!ok) EXCEPTION(EDATA, 0, sql);
                    if (n > 0) {
                        PDEB(INFO) << tr("Port %1, is uplink to %2, disabled.").arg(np.getName(), np.getFullNameById(q, linkedPortId));
                        continue;    // Uplink, kihagyjuk
                    }
                    PDEB(INFO) << tr("Port %1, is no uplink to %2.").arg(np.getName(), np.getFullNameById(q, linkedPortId));
                }
            }
            // mehet a ports konténerbe az indexe
        }
        else if (ifTypeName == _sMultiplexor) {
            // TRUNK-nal a TRUNK tagjaihoz van rendelve az uplink
            ix = -1;
            QString msg;
            // Erre a node-ra megy a link (ha marad NULL_ID, akkor nincs uplink)
            qlonglong linkedNodeId = NULL_ID;
            while (true) {
                int st = ix + 1;    // Find start index: st = 0 or next port index
                // Find trunk members
                ix = host().ports.indexOf(_sPortStapleId, np.get(_sPortId), st);
                if (ix < 0) {
                    if (st == 0 && appMemoOn) {  // Empty trunk
                        msg = tr("A %1:%2 trunk-nek nincs egyetlen tagja sem.").arg(host().getName(), np.getName());
                        APPMEMO(q, msg, RS_ON);
                    }
                    break;  // Nincs, vagy nincs több
                }
                // port ID
                qlonglong pid = host().ports[ix]->getId();
                qlonglong pid_lldp = cLldpLink().getLinked(q, pid);   // LLDP-vel felfedezett link
                qlonglong pid_log  = cLogLink(). getLinked(q, pid);   // Logikai (manuálisan rögzített) link
                // host ID
                qlonglong hid = NULL_ID;
                cNPort    lp;
                if (pid_lldp != NULL_ID && pid_log != NULL_ID && pid_lldp != pid_log) { // Antagonism.
                    msg = tr("A %1:%2 trunk %3 tagjáhozrendelt linkek ellentmondásosak: LLDP : %4; Logikai : %5")
                            .arg(host().getName(), np.getName(), host().ports[ix]->getName()
                                 , cNPort::getFullNameById(q, pid_lldp), cNPort::getFullNameById(q, pid_log));
                    APPMEMO(q, msg, RS_CRITICAL);
                    pid_log = NULL_ID;  // Az LLDP-t hisszük el.
                }
                if (pid_lldp != NULL_ID || pid_log != NULL_ID) {                        // Agreement.
                    hid = lp.setById(q, pid_lldp == NULL_ID ? pid_log : pid_lldp).getId(_sNodeId);
                }
                else {                                                                  // No information about LLDP
                    // Ha van "link_is_invisible_for_LLDP" paraméter, és igaz, akkor nem pampogunk a link hiánya miatt
                    eTristate f = host().ports[ix]->getBoolParam(_sLinkIsInvisibleForLLDP, EX_IGNORE);
                    if (f == TS_TRUE) continue;
                    msg = tr("A %1:%2 trunk %3 tagjához nincs link rendelve.")
                            .arg(host().getName(), np.getName(), host().ports[ix]->getName());
                    APPMEMO(q, msg, RS_WARNING);
                    continue;
                }
                if (linkedNodeId == NULL_ID) {  // There is no reference link (first)
                    linkedNodeId = hid;
                }
                else {
                    if (linkedNodeId != hid) {
                        msg = tr("A %1:%2 trunk %3 tagja %4 -el, előző tagja %5 -al van linkben.")
                                .arg(host().getName(), np.getName(), host().ports[ix]->getName(),
                                     cNode().getNameById(q, hid, EX_IGNORE), lp.getFullName(q, EX_IGNORE));
                        cAlarm::ticket(q, RS_CRITICAL, msg, hostServiceId());
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
        ix = int(np.getId(_sPortIndex));    // Eredeti port index
        if (rxix.contains(ix)) {            // Van ilyen a kereszt index-ben?
            ix = rxix[ix];                  // a lekérdezésben ezt az indexet fogja megkapni
            PDEB(VERBOSE) << tr("Set query %1:%2 port, inex %3 (%4).").arg(host().getName(), np.getName()).arg(ix).arg(np.getId(_sPortIndex)) << endl;
            ports.insert(ix, np.reconvert<cInterface>());
        }
 /*       else if (rxix.isEmpty()) {
            PDEB(VERBOSE) << tr("Set query %1 port, inex %2.").arg(np.getName()).arg(ix) << endl;
            ports.insert(ix, np.reconvert<cInterface>());
        } */
        else {
            PDEB(VERBOSE) << tr("No set query %1:%2 port, inex %3 not found.").arg(host().getName(), np.getName()).arg(ix) << endl;
        }
    }
    if (ports.isEmpty()) {
        msgAppend(&runMsg, tr("Nincs lekérdezendő port! A példány le lesz tiltva!"));
        hostService.setOn(_sDisabled);
        hostService.update(q, true, hostService.mask(_sDisabled));
        return RS_UNREACHABLE;
    }
    return RS_ON;
}

int cDevicePMac::run(QSqlQuery& q, QString& runMsg)
{
    _DBGFN() << QChar(' ') << name() << endl;
    enum eNotifSwitch r;
    if (ports.isEmpty()) {
        r = eNotifSwitch(initQuery(q, runMsg));
        if (r != RS_ON) return r;
    }

    QMap<cMac, int> macs;

    for (auto sCommunity: communityList) {
        cSnmp   snmp;
        int ir = snmp.open(hostAddress, sCommunity, snmpVersion);
        if (ir) {
            msgAppend(&runMsg, tr("%1 : SNMP open error : %2").arg(name(), snmp.emsg));
            return RS_UNREACHABLE;
        }
        if (RS_ON != (r = snmpQuery(snmp, *pOId1, macs, runMsg))) {
            return r;   // Az index nem feltétlenül az eredeti port index!
        }
        if (RS_ON != (r = snmpQuery(snmp, *pOId2, macs, runMsg))) {
            return r;
        }
    }
    foundMacs.clear();
    QMap<cMac, int>::iterator   i;
    for (i = macs.begin(); i != macs.end(); ++i) {  // Végigvesszük az SNMP query találatait
        const int   ix  = i.value();                // port index
        QMap<int, cInterface *>::iterator pi = ports.find(ix);
        if (pi == ports.end()) continue;            // Csak a konténerben lévő portok érdekesek
        cMacTab mt;                                 // mac_tabs record
        cMac mac = i.key();                         // MAC a címtáblában
        qlonglong pid = pi.value()->getId();        // Switch port ID
        PDEB(VERBOSE) << VDEB(pid) << _sSpace << pi.value()->getFullName(q) << " :: " << mac.toString() << endl;
        mt.setMac(_sHwAddress, mac);
        mt.setId(_sPortId, pid);
        int r = mt.replace(q);                      // Replace mac_tabs record
        switch (r) {
        case R_DISCARD:
            runMsg = msgCat(runMsg, tr("Port %1 set suspected uplink.").arg(pi.value()->getName()), "\n");
            ports.remove(ix);   // Lett neki egy "suspected_uplink" paramétere, igaz értékkel!
            break;
        case R_RESTORE:
            runMsg = msgCat(runMsg, tr("Port %1 restored mac %2.").arg(pi.value()->getName(), mac.toString()), "\n");
            foundMacs[pid] |= mac;  // Talált MAC-ek a rightmac-hoz
            break;
        default:
            foundMacs[pid] |= mac;  // Talált MAC-ek a rightmac-hoz
            break;
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

enum eNotifSwitch cDevicePMac::snmpQuery(cSnmp& snmp, const cOId& __o, QMap<cMac, int>& macs, QString& runMsg)
{
    _DBGFN() << QChar(' ') << __o.toString() << endl;
    cOId    o = __o;
    do {
        int r = snmp.getNext(o);
        if (r) {
            runMsg = msgCat(runMsg,
                    tr("SNMP error #%1 (%2). OID:%3")
                    .arg(r).arg(snmp.emsg, __o.toString()), "\n");
            return RS_CRITICAL;
        }
        o = snmp.name();
        if (!(__o < o)) break;
        bool ok;
        int pix = snmp.value().toInt(&ok);
        if (!ok) {
            runMsg = msgCat(runMsg,
                    tr("SNMP ansver, invalid index. OID:%1: %2 = %3")
                    .arg(__o.toString(), o.toString(), debVariantToString(snmp.value())), "\n");
            return RS_CRITICAL;
        }
        if (!(ports.contains(pix))) continue;
        cMac mac = snmp.name().toMac();
        if (!mac) {
            runMsg =  msgCat(runMsg,
                    tr("Az SNMP ansver, invalid MAC. OID:%1: %2 = %3")
                    .arg(__o.toString(), o.toString(), snmp.value().toString()), "\n");
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
        msg = tr("Nem támogatott szervíz : %1. Itt (pmac alatt) csak a 'rightmac' szervíz támogatott.").arg(service()->getName());
    }
    else {
        if (pPort == nullptr || 0 != pPort->chkObjType<cInterface>(EX_IGNORE)) {
            flag = false;
            if (pPort == nullptr) msg = tr("Nincs megadva port.\n");
            else                  msg = tr("A port típusa csak interface lehet.\n");
        }
        else {
            // MAC list from features
            QStringList sMacList  = features().slValue("MAC");
            foreach (QString sMac, sMacList) {
                cMac mac(sMac.simplified());
                if (mac.isValid()) rightMacs |= mac;
                else {
                    msg += tr("Helytelen MAC : %1\n").arg(sMac);
                    flag = false;
                }
            }
            // Node list from features
            QStringList sNodeList = features().slValue("node");
            cNode node;
            foreach (QString sNode, sNodeList) {
                if (node.fetchByName(q, sNode.simplified())) {
                    node.fetchPorts(q, 0);  // csak a portokat olvassuk be
                    QList<cMac> macs = node.getMacs();
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
                    rightMacs |= QSet<cMac>::fromList(macs);
#else
                    rightMacs |= QSet<cMac>(macs.begin(), macs.end());
#endif
                }
                else {
                    msg += tr("Helytelen eszköz név : %1\n").arg(sNode);
                    flag = false;
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
                flag = false;
            }
        }
    }
    if (!flag) {    // Hiba
        hostService.setState(q, _sUnreachable, msg);
    }
    else {
        dynamic_cast<cDevicePMac&>(parent()).rightMap[portId()] = this;
    }
}

void cRightMac::checkMacs(QSqlQuery& q, const QSet<cMac>& macs)
{
    QString stat, msg;
    if (macs.isEmpty()) {
        stat = _sOn;
        msg  = tr("Nincs talált MAC.");
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
            msg  = tr("Nincs engedélyezetlen MAC. Talált cím lista : [%1]").arg(slAuth.join(sep));
        }
        else {
            stat = _sCritical;
            msg  = tr("Engedélyezetlen MAC : [%1]. Talált cím lista : [%2]").arg(slUnauth.join(sep) ,slAuth.join(sep));
        }
    }
    hostService.setState(q, stat, msg);
}
