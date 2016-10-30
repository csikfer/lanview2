#include "portstat.h"


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

    lv2portStat   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // A továbbiakban a timer ütemében történnek az események
    int r = app.exec();
    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);
}

lv2portStat::lv2portStat() : lanView()
{
    pSelfInspector = NULL;
    if (lastError == NULL) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            setup();
        } CATCHS(lastError)
    }
}

lv2portStat::~lv2portStat()
{
    down();
}

void lv2portStat::staticInit(QSqlQuery *pq)
{
    cDevicePSt::pRLinkStat = cService::service(*pq, "rlinkstat");
    cDevicePSt::pSrvSnmp   = cService::service(*pq, _sSnmp);
}

void lv2portStat::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::setup(_tr);
}


/******************************************************************************/

cPortStat::cPortStat(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    ;
}

cPortStat::~cPortStat()
{
    ;
}

cInspector * cPortStat::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePSt(q, hsid, hoid, pid);
}

/******************************************************************************/

const cService *cDevicePSt::pRLinkStat = NULL;
const cService *cDevicePSt::pSrvSnmp   = NULL;


cDevicePSt::cDevicePSt(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    // Ha nincs megadva protocol szervíz, akkor SNMP device esetén az az SNMP lessz
    if (protoService().isEmpty_() && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    /// Csak az SNMP lekérdezés támogatott (egyenlőre)
    if (protoServiceId() != pSrvSnmp->getId())
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem megfelelő proto_service_id!"));
}

cDevicePSt::~cDevicePSt()
{
    ;
}

void cDevicePSt::postInit(QSqlQuery &q, const QString&)
{
    DBGFN();
    cInspector::postInit(q);
    if (pSubordinates != NULL) EXCEPTION(EDATA);    // Ha a superior=custom nem volt megadva
    pSubordinates = new QList<cInspector *>;
    /// A host eredeti objektum típusa
    cSnmpDevice& dev = snmpDev();
    dev.open(q, snmp);
    dev.fetchPorts(q);
    tRecordList<cNPort>::iterator i, n = dev.ports.end();
    for (i = dev.ports.begin(); i != n; ++i) {      // Végigvesszük a portokat
        cNPort *np = *i;
        cInspector *pnas = NULL;    // Al szolgáltatás,ha van
        if (np->descr() >= cInterface::_descr_cInterface()) {      // interface-nél alacsonyabb típussal nem foglalkozunk
            QString msg;
            cInterface *p = dynamic_cast<cInterface *>(np);
             // Mivel van linkelve?
            qlonglong lpid, lpid1, lpid2;     // Linkelt port ID
            lpid = cLldpLink().getLinked(q, p->getId());
            if (lpid != NULL_ID) {
                lpid1 = cLogLink().getLinked(q, p->getId());
                lpid2 = cLogLink().getLinked(q, lpid);
                if (lpid1 != NULL_ID && lpid1 != lpid) { // Ez egy ellentmondás!!!
                    msg += trUtf8("A szervíz %1 port logikai linkje %2 porthoz, ötközik az LLDP klinkkel a %3 porthoz, ezért törölkük a fizikai link első elemét. ")
                            .arg(p->getFullName(q))
                            .arg(cNPort::getFullNameById(q, lpid1))
                            .arg(cNPort::getFullNameById(q, lpid));
                    cPhsLink().unlink(q, p->getId(), LT_TERM, ES_);
                }
                if (lpid2 != NULL_ID && lpid2 != p->getId()) { // Ez egy ellentmondás!!!
                    msg += trUtf8("A kliens %1 port logikai linkje %2 porthoz, ötközik az LLDP klinkkel a %3 porthoz, ezért törölkük a fizikai link utolsó elemét. ")
                            .arg(cNPort::getFullNameById(q, lpid))
                            .arg(cNPort::getFullNameById(q, lpid2))
                            .arg(p->getFullName(q));
                    cPhsLink().unlink(q, lpid, LT_TERM, ES_);
                }
            }
            else {      // nem volt LLDP-vel felfedezett link
                lpid = cLogLink().getLinked(q, p->getId());     // Logikai link ?
                if (lpid != NULL_ID) {
                    lpid2 = cLldpLink().getLinked(q, lpid);
                    if (lpid2 != NULL_ID) {     // Elvileg lehetetlen, hogy jó legyen ha ez van az ellenkező irány meg nincs.
                        msg += trUtf8("A kliens %1 port lldp linkje %2 porthoz, ötközik a logikai klinkkel a %3 porthoz, ezért törölkük a fizikai link utolsó elemét. ")
                                .arg(cNPort::getFullNameById(q, lpid))
                                .arg(cNPort::getFullNameById(q, lpid2))
                                .arg(p->getFullName(q));
                        cPhsLink().unlink(q, lpid, LT_TERM, ES_);
                        lpid = NULL_ID;     // nem ok link, töröltük
                    }
                }
            }
            if (lpid != NULL_ID) {  // A linkelt port ID-je
                pnas = new cInspector(this);
                pnas->service(pRLinkStat);
                pnas->pPort = cNPort::getPortObjById(q, lpid);
                cRecord *pn = cNode::getNodeObjById(q, pnas->pPort->getId(_sNodeId), EX_IGNORE);
                if (pn == NULL) {  // Bizonyára egy HUB, nem foglakozunk vele
                    pDelete(pnas);
                }
                else {
                    pnas->pNode = pn->reconvert<cNode>();
                    cHostService& hs = pnas->hostService;
                    if (hs.fetchByIds(q, pnas->nodeId(), pnas->serviceId(), EX_IGNORE)) { // Van host_services rekord
                        if (hs.getId(_sSuperiorHostServiceId) != hostServiceId()) {  // máshova mutat a superior, a link szerint viszont a mienk !!!
                            hs.setId(_sSuperiorHostServiceId, hostServiceId());
                        }
                        if (hs.getId(_sPortId) != lpid) {                           // nem stimmel a port
                            hs.setId(_sPortId, lpid);
                        }
                        if (hs.isModify_()) {                                       // volt javítás, update
                            hs.update(q, true);
                        }
                    }
                    else {
                        hs.clear();
                        hs.setId(_sNodeId, pnas->nodeId());
                        hs.setId(_sServiceId, pnas->serviceId());
                        hs.setId(_sSuperiorHostServiceId, hostServiceId());
                        hs.setId(_sPortId, lpid);
                        hs.setName(_sHostServiceNote, trUtf8("Automatikusan generálva a portstat által."));
                        hs.setName(_sNoalarmFlag, _sOn);    // Nem kérünk riasztást az automatikusan generált rekordhoz.
                        hs.insert(q);
                    }
                }
            }
            if (!msg.isEmpty()) {
                cDbErr::insertNew(q, cDbErrType::_sDataWarn, msg);
            }
        }
        *pSubordinates << pnas;
    }
}

enum eNotifSwitch cDevicePSt::run(QSqlQuery& q)
{
    _DBGFN() << QChar(' ') << name() << endl;
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    cTable      tab;    // Interface table container
    QStringList cols;   // Table col names container
    cols << _sIfIndex << _sIfAdminStatus << _sIfOperStatus << _sIfDescr;
    int r = snmp.getTable(_sIfMib, cols, tab);
    if (r) {
        EXCEPTION(ESNMP, r, QString(QObject::trUtf8("SNMP get table error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    int n = tab.rows();
    int i;
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != NULL) ps->flag = false;   // A státuszt még nem állítottuk be.
    }
    for (i = 0; i < n; i++) {
        bool    ok;

        int     ifIndex = tab[_sIfIndex][i].toInt(&ok);
        if (!ok) EXCEPTION(ESNMP, -1, QString(QObject::trUtf8("ifIndex is not numeric : %1")).arg(tab[_sIfIndex][i].toString()));

        int ix = host().ports.indexOf(_sPortIndex, QVariant(ifIndex));
        if (ix < 0)
            continue;   // Nincs ilyen indexű portunk, eldobjuk

        QString ifDescr = tab[_sIfDescr][i].toString();

        QString opstat = snmpIfStatus(tab[_sIfOperStatus][i].toInt(&ok));
        if (!ok) EXCEPTION(ESNMP, -1, QString(QObject::trUtf8("ifOperStatus is not numeric : %1")).arg(tab[_sIfOperStatus][i].toString()));

        QString adstat = snmpIfStatus(tab[_sIfAdminStatus][i].toInt(&ok));
        if (!ok) EXCEPTION(ESNMP, -1, QString(QObject::trUtf8("ifAdminStatus is not numeric : %1")).arg(tab[_sIfAdminStatus][i].toString()));

        // Előkapjuk az interface objektumunkat, a konténerből, ehhez kell az indexük
        cInterface&     iface = * dynamic_cast<cInterface *>(host().ports[ix]);
        if (opstat != iface.getName(_sPortOStat) || adstat != iface.getName(_sPortAStat)) {
            iface.setName(_sPortOStat, opstat);
            iface.setName(_sPortAStat, adstat);
            PDEB(VERBOSE) << "Update port stat: " << pNode->getName() << QChar(':') << ifDescr << endl;
            if (!iface.update(q, false, iface.mask(_sPortOStat, _sPortAStat), iface.mask(_sNodeId, _sPortIndex), EX_IGNORE)) {
                DERR() << "Update error : host " << pNode->getName() << " port index : " << ifIndex << endl;
            }
        }
        if ((*pSubordinates)[ix] == NULL) continue;   // Nincs szolgáltatás, kész
        // Előkapjuk a kapcsolódó szolgálltatást
        cInspector& ns   = *(*pSubordinates)[ix];
        QString state = _sUnKnown;
        if      (opstat == _sUp)   state = _sOn;        // On
        else if (adstat == _sUp)   state = _sDown;      // Off
        else if (adstat == _sDown) state = _sWarning;   // Disabled
        ns.hostService.setState(q, state, trUtf8("if state : op:%1/adm:%2").arg(opstat).arg(adstat), parentId());
        ns.flag = true; // beállítva;
    }
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != NULL && ps->flag == false) {   // be lett állítva státusz ??? Ha még nem:
            ps->hostService.setState(q, _sUnreachable, trUtf8("No data") , parentId());
        }
    }
    DBGFNL();
    return RS_ON;
}

