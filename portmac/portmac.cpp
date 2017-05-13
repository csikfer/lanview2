#include "portmac.h"
#include "lv2daterr.h"

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
    cDevicePMac::pSrvSnmp   = cService::service(*pq, _sSnmp);
}

/******************************************************************************/

cPortMac::cPortMac(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    cDevicePMac::pOId1 = new cOId("SNMPv2-SMI::mib-2.17.4.3.1.2");
    cDevicePMac::pOId2 = new cOId("SNMPv2-SMI::mib-2.17.7.1.2.2.1.2");
    if (!*cDevicePMac::pOId1) EXCEPTION(ESNMP, cDevicePMac::pOId1->status, cDevicePMac::pOId1->emsg);
    if (!*cDevicePMac::pOId2) EXCEPTION(ESNMP, cDevicePMac::pOId2->status, cDevicePMac::pOId2->emsg);
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

const cService *cDevicePMac::pSrvSnmp   = NULL;
cOId    *cDevicePMac::pOId1 = NULL;
cOId    *cDevicePMac::pOId2 = NULL;

cDevicePMac::cDevicePMac(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    QString msg;
    static const qlonglong suspectrdUpLinkTypeId        = cParamType().getIdByName(__q, _sSuspectedUplink);
    static const qlonglong queryMacTabTypeId            = cParamType().getIdByName(__q, _sQueryMacTab);
    static const qlonglong linkIsInvisibleForLLDPTypeId = cParamType().getIdByName(__q, _sLinkIsInvisibleForLLDP);
    // Ha nincs megadva protocol szervíz, akkor SNMP device esetén az az SNMP lessz
    if (protoService().isEmpty_() && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    // Csak az SNMP lekérdezés támogatott
    if (protoServiceId() != pSrvSnmp->getId()) {
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem megfelelő proto_service_id!"));
    }
    // Beolvassuk a portokat is
    host().fetchPorts(__q);
    // host().fetchParams(__q);
    tRecordList<cNPort>::iterator   i;
    // Csinálunk a releváns portokhoz egy index táblát
    for (i = host().ports.begin(); i < host().ports.end(); ++i) {
        cNPort  &np = **i;
        np.fetchParams(__q);
        eTristate queryFlag = np.getBoolParam(queryMacTabTypeId, EX_IGNORE);
        int ix = -1;
        if (np.descr() < cInterface::_descr_cInterface()) continue; // buta portok érdektelenek
        QString ifTypeName = np.ifType().getName();
        if (ifTypeName == _sEthernet) {
            // Ha ez egy TRUNK tagja, akkor nem érdekes.
            if (!np.isNull(_sPortStapleId)) continue;
            // Ki kéne még hajítani az uplinkeket
            if (NULL_ID != cLldpLink().getLinked(__q, np.getId())) { // Ez egy LLDP-vel felderített uplink
                // Ha van "query_mac_tab" paraméter, és igaz, akkor a link ellenére lekérdezzük
                if (queryFlag == TS_TRUE) continue;
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
                    if (st == 0) {  // Üres trunk?
                        msg = trUtf8("A %1:%2 trunk-nek nincs egyetlen tagja sem.").arg(host().getName(), np.getName());
                        APPMEMO(__q, msg, RS_WARNING);
                    }
                    break;  // Nincs, vagy nincs több
                }
                // port ID
                qlonglong pid = cLldpLink().getLinked(__q, host().ports[ix]->getId());
                // host ID
                qlonglong hid = NULL_ID;
                cNPort    lp;
                if (pid != NULL_ID) {
                    hid = lp.setById(__q, pid).getId(_sNodeId);
                }
                else {  // Nincs link ? Trunk-nél!
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
                                .arg(cNode().getNameById(__q, EX_IGNORE), lp.getFullName(__q, EX_IGNORE));
                        APPMEMO(__q, msg, RS_CRITICAL);
                    }
                }
            }
            // A Trunk-ot nem kérdezzük le, mert valószínüleg uplink, hacsak nincs "query_mac_tab = true"
            if (queryFlag == TS_TRUE) continue;
        }
        else {
            // Más típusű port nem érdekes.
            continue;
        }
        // Ha van "query_mac_tab" paraméter, és hamis, akkor tiltott a lekérdezés a portra
        // Ha van "suspected_uplink" paraméter, és igaz, akkor nem foglalkozunk vele (csiki-csuki elkerülése)
        if (queryFlag == TS_FALSE || np.getBoolParam(suspectrdUpLinkTypeId, EX_IGNORE) == TS_TRUE) continue;
        ports.insert((int)np.getId(_sPortIndex), np.reconvert<cInterface>());
    }
}

cDevicePMac::~cDevicePMac()
{
    ;
}

void cDevicePMac::postInit(QSqlQuery &q, const QString&)
{
    DBGFN();
    cInspector::postInit(q);
    if (pSubordinates != NULL) EXCEPTION(EPROGFAIL);
    snmpDev().open(q, snmp);
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

    if (RS_ON != (r = snmpQuery(*pOId1, macs, runMsg))) return r;
    if (RS_ON != (r = snmpQuery(*pOId2, macs, runMsg))) return r;

    QMap<cMac, int>::iterator   i;
    for (i = macs.begin(); i != macs.end(); ++i) {
        const int   ix  = i.value();
        QMap<int, cInterface *>::iterator pi = ports.find(ix);
        if (pi == ports.end()) continue;    // Csak a konténerben lévő portok érdekesek
        cMacTab mt;
        cMac mac = i.key();
        mt.setMac(_sHwAddress, mac);
        mt.setId(_sPortId, pi.value()->getId());
        int r = mt.replace(q);
        if (r == R_DISCARD) ports.remove(ix);   // Lett neki egy "suspected_uplink" paramétere, igaz értékkel!
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
            runMsg = trUtf8("SNMP hiba #%1 (%2). OID:%3")
                    .arg(r).arg(snmp.emsg)
                    .arg(__o.toString());
            return RS_CRITICAL;
        }
        if (!(__o < snmp.name())) break;
        o = snmp.name();
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

