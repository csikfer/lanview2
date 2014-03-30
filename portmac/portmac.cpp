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
    QCoreApplication app(argc, argv);

    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = true;
    lanView::gui        = false;

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

cPortMac::cPortMac(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
    , mOId1("mib-2.17.4.3.1.2"), mOId2("mib-2.17.7.1.2.2.1.2")
{
    if (!mOId1) EXCEPTION(ESNMP, mOId1.status, mOId1.emsg);
    if (!mOId2) EXCEPTION(ESNMP, mOId2.status, mOId2.emsg);
}

cPortMac::~cPortMac()
{
    ;
}

cInspector * cPortMac::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePMac(q, hsid, hoid, pid);
}

/// Ez ugye nem csinál semmit, a "SUB"-ok dolgoznak
enum eNotifSwitch cPortMac::run(QSqlQuery &)
{
    return RS_ON;
}

/******************************************************************************/

const cService *cDevicePMac::pSrvSnmp   = NULL;


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
    /// Csak az SNMP lekérdezés támogatott (egyenlőre)
    if (protoServiceId() != pSrvSnmp->getId())
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem megfelelő proto_service_id!"));
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
    /* // portstat-ból ...
    pSubordinates = new QList<cInspector *>;
    /// A host eredeti objektum típusa
    cSnmpDevice& dev = snmpDev();
    dev.open(q, snmp);
    dev.fetchPorts(q);
    tRecordList<cNPort>::iterator i, n = dev.ports.end();
    for (i = dev.ports.begin(); i != n; ++i) {
        cNPort *np = *i;
        cInspector *pnas = NULL;
        if (np->descr() >= cInterface::_descr_cInterface()) {      // interface-nél alacsonyabb típussal nem foglalkozunk
            cInterface *p = dynamic_cast<cInterface *>(np);
            qlonglong lpid = cLogLink().getLinked(q, p->getId()); // Mivel van linkelve?
            if (lpid != NULL_ID) {  // A linkelt port ID-je
                pnas = new cInspector(this);
                pnas->service(pRLinkStat);
                pnas->pPort = cNPort::getPortObjById(q, lpid);
                pnas->pNode = cNode::getNodeObjById(q, pnas->pPort->getId(_sNodeId), false)->reconvert<cNode>();
                if (pnas->pNode == NULL) {  // Bizonyára egy HUB, nem foglakozunk vele
                    delete pnas;
                    pnas = NULL;
                }
                else {
                    cHostService& hs = pnas->hostService;
                    if (hs.fetchByIds(q, pnas->nodeId(), pnas->serviceId(), false)) { // Van host_services rekord
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
                        hs.setName(_sHostServiceNote, QObject::trUtf8("Automatikusan generálva a portstat által."));
                        hs.setName(_sNoalarmFlag, _sOn);    // Nem kérünk riasztást az automatikusan generált rekordhoz.
                        hs.insert(q);
                    }
                }
            }
        }
        *pSubordinates << pnas;
    }
    */
}

enum eNotifSwitch cDevicePMac::run(QSqlQuery& q)
{
    _DBGFN() << _sSpace << name() << endl;
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }

    /* // portstat -ból ...
    cTable      tab;    // Interface table container
    QStringList cols;   // Table col names container
    cols << _sIfIndex << _sIfAdminStatus << _sIfOperStatus << _sIfDescr;
    int r = snmp.getTable(_sIfMib, cols, tab);
    if (r) {
        EXCEPTION(ESNMP, r, QString(QObject::trUtf8("SNMP get table error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    int n = tab.rows();
    int i;
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
            PDEB(VERBOSE) << "Update port stat: " << pNode->getName() << _sColon << ifDescr << endl;
            if (!iface.update(q, false, iface.mask(_sPortOStat, _sPortAStat), iface.mask(_sNodeId, _sPortIndex), false)) {
                DERR() << "Update error : host " << pNode->getName() << " port index : " << ifIndex << endl;
            }
        }
        if ((*pSubordinates)[ix] == NULL) continue;   // Nincs szolgáltatás, kész
        // Előkapjuk a kapcsolódó szolgálltatást
        cInspector& ns   = *(*pSubordinates)[ix];
        if (opstat == _sUp) ns.hostService.setState(q, _sOn, _sNul, parentId());
        else                ns.hostService.setState(q, _sCritical, QString("%1/%2").arg(opstat).arg(adstat), parentId());
    }
    */
    DBGFNL();
    return RS_ON;
}


