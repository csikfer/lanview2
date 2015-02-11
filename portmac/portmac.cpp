#include "portmac.h"

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
    lanView::sqlNeeded  = true;

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

            cDevicePMac::pSrvSnmp   = &cService::service(*pq, _sSnmp);
            setup();
        } CATCHS(lastError)
    }
}

lv2portMac::~lv2portMac()
{
    down();
}

bool lv2portMac::uSigRecv(int __i)
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
void lv2portMac::dbNotif(QString __s)
{
    if (pSelf != NULL && pSelf->internalStat != IS_RUN) return;
    PDEB(INFO) << QString(trUtf8("EVENT FROM DB : %1")).arg(__s) << endl;
    reSet();
}

void lv2portMac::setup()
{
    sqlBegin(*pq);
    pSelf = new cPortMac(*pq, appName);
    pSelf->postInit(*pq);
    if (pSelf->pSubordinates == NULL || pSelf->pSubordinates->isEmpty()) EXCEPTION(NOTODO);
    sqlEnd(*pq);
    pSelf->start();
}

void lv2portMac::down()
{
    if (pSelf != NULL) delete pSelf;
    pSelf = NULL;
}

void lv2portMac::reSet()
{
    try {
        pSelf->setInternalStat(IS_REINIT);
        down();
        setup();
    } CATCHS(lastError);
    if (lastError != NULL) QCoreApplication::exit(lastError->mErrorCode);
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
enum eNotifSwitch cPortMac::run(QSqlQuery &__q)
{
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
    // Ha nincs megadva protocol szervíz, akkor SNMP device esetén az az SNMP lessz
    if (protoService().isEmpty_() && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = &hostService.getProtoService(q2);
    }
    // Csak az SNMP lekérdezés támogatott
    if (protoServiceId() != pSrvSnmp->getId()) {
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem megfelelő proto_service_id!"));
    }
    // Beolvassuk a portokat is
    host().fetchPorts(__q);
    tRecordList<cNPort>::iterator   i;
    // Csinálunk a releváns portokhoz egy index táblát
    for (i = host().ports.begin(); i < host().ports.end(); ++i) {
        cNPort  &np = **i;
        if (np.descr() < cInterface::_descr_cInterface()) continue; // buta portok érdektelenek
        QString ifTypeName = np.ifType().getName();
        if (ifTypeName == _sEthernet) {
            // Ha ez egy TRUNK tagja, akkor nem érdekes.
            if (!np.isNull(_sPortStapleId)) continue;
            // Ki kéne még hajítani az uplinkeket
            if (NULL_ID != cLldpLink().getLinked(__q, np.getId())) continue; // Ez egy LLDP-vel felderített uplink
            // mehet a ports konténerbe az indexe
        }
        else if (ifTypeName != _sMultiplexor) {
            // TRUNK-nal a TRUNK tagjaihoz van rendelve az uplink
            int ix = -1;
            bool first = true;
            // Erre a node-ra megy a link (ha marad NULL_ID, akkor nincs uplink)
            qlonglong linkedNodeId = NULL_ID;
            while (true) {
                int st = ix + 1;
                // Keressük a trunk tagjait
                ix = host().ports.indexOf(_sPortStapleId, np.get(_sPortId), ix);
                if (ix < 0) {
                    if (st == 0) {  // Üres trunk?
                        QString msg = trUtf8("A %1 trunk-nek nincs egyetlen tagja sem.").arg(np.getFullName(__q));
                        cDbErr::insertNew(__q, cDbErrType::_sNotFound, msg, np.getId(), np.tableName(), APPNAME);
                    }
                    break;  // Nincs, vagy nincs több
                }
                qlonglong pid = cLldpLink().getLinked(__q, ports[ix]->getId());
                qlonglong hid = NULL_ID;
                if (pid != NULL_ID) hid = cNode().setById(__q, pid).getId();
                if (first) {
                    first = false;
                    linkedNodeId = hid;
                }
                else {
                    if (linkedNodeId != hid) {
                        QString msg = trUtf8("A %1(%2) trunk tagjai nem azonos node-hoz csatlakoznak (%2 - %3) .")
                                .arg(np.getFullName(__q))
                                .arg(cNode().getNameById(linkedNodeId))
                                .arg(cNode().getNameById(hid));
                        cDbErr::insertNew(__q, cDbErrType::_sNotFound, msg, np.getId(), np.tableName(), APPNAME);
                    }
                }
            }
            if (linkedNodeId != NULL_ID) continue;  // uplink, nem kell
        }
        else {
            // Más típusű port nem érdekes.
            continue;
        }
        if (np.fetchParams(__q)) { // Ha van paraméter
            static  qlonglong suspectrdUpLinkTypeId = NULL_ID;
            if (suspectrdUpLinkTypeId == NULL_ID) suspectrdUpLinkTypeId = cParamType().getIdByName(__q, _sSuspectedUplink);
            int i = np.params.indexOf(_sParamTypeId, QVariant(suspectrdUpLinkTypeId));
            // Ha van "suspected_uplink" paraméter, és igaz, akkor nem foglalkozunk vele (csiki-csuki elkerülése)
            if (i >= 0 && str2bool(np.params.at(i)->getName(_sParamValue))) continue;
        }
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


enum eNotifSwitch cDevicePMac::run(QSqlQuery& q)
{
    _DBGFN() << QChar(' ') << name() << endl;
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name()));
    }
    QMap<cMac, int> macs;
    enum eNotifSwitch r;

    if (RS_ON != (r = snmpQuery(*pOId1, macs))) return r;
    if (RS_ON != (r = snmpQuery(*pOId2, macs))) return r;

    QMap<cMac, int>::iterator   i;
    for (i = macs.begin(); i != macs.end(); ++i) {
        const int   ix  = i.value();
        QMap<int, cInterface *>::iterator pi = ports.find(ix);
        if (pi == ports.end()) continue;    // Csak a konténerben lévő portok érdekesek
        cMacTab mt;
        cMac mac = i.key();
        mt.setMac(_sHwAddress, mac);
        mt.setId(_sPortId, pi.value()->getId());
        enum eReasons r = mt.replace(q);
        if (r == R_DISCARD) ports.remove(ix);   // Lett neki egy "suspected_uplink" paramétere, igaz értékkel!
    }

    DBGFNL();
    return RS_ON;
}

enum eNotifSwitch cDevicePMac::snmpQuery(const cOId& __o, QMap<cMac, int>& macs)
{
    _DBGFN() << QChar(' ') << __o.toString() << endl;
    cOId    o = __o;
    do {
        int r = snmp.getNext(o);
        if (r) {
            QString msg = trUtf8("A %1 node SNMP hiba. OID:%2")
                    .arg(host().getName())
                    .arg(pOId1->toString());
            DERR() << msg << ", #" << r << endl;
            cDbErr::insertNew(cDbErrType::_sRunTime, msg, r, _sNil, APPNAME);
            break;
        }
        if (!(*pOId1 < snmp.name())) break;
        o = snmp.name();
        bool ok;
        int pix = snmp.value().toInt(&ok);
        if (!ok) {
            QString msg = trUtf8("A %1 node SNMP válasz: nem értelmezhető index. OID:%2 = %3")
                    .arg(host().getName())
                    .arg(pOId1->toString())
                    .arg(debVariantToString(snmp.value()));
            DERR() << msg << endl;
            cDbErr::insertNew(cDbErrType::_sRunTime, msg, 0, _sNil, APPNAME);
            break;
        }
        if (!(ports.contains(pix))) continue;
        cMac mac = snmp.name().toMac();
        if (!mac) {
            QString msg = trUtf8("A %1 node SNMP válasz: nem értelmezhető MAC. OID:%2 = %3")
                    .arg(host().getName())
                    .arg(pOId1->toString())
                    .arg(snmp.value().toString());
            DERR() << msg << endl;
            cDbErr::insertNew(cDbErrType::_sRunTime, msg, 0, _sNil, APPNAME);
            break;
        }
        macs.insert(mac, pix); // inser or replace
        PDEB(VVERBOSE) << "#" << pix << ":" << mac.toString() << endl;
    } while(true);
    DBGFNL();
    return RS_ON;
}

