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
    , mOId1("SNMPv2-SMI::mib-2.17.4.3.1.2")
    , mOId2("SNMPv2-SMI::mib-2.17.7.1.2.2.1.2")
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

/*
/// Ez ugye nem csinál semmit, a "SUB"-ok dolgoznak
enum eNotifSwitch cPortMac::run(QSqlQuery &)
{
    return RS_ON;
}
*/

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
    // Csak az SNMP lekérdezés támogatott
    if (protoServiceId() != pSrvSnmp->getId()) {
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem megfelelő proto_service_id!"));
    }
    // Beolvassuk a portokat is
    host().fetchPorts(__q);
    tRecordList<cNPort>::iterator   i;
    // Csinálunk a releváns portokhoz egy index táblát
    for (i = host().ports.begin(); i <= host().ports.end(); ++i) {
        cNPort  &np = **i;
        if (np.descr() < cInterface::_descr_cInterface()) continue; // buta portok érdektelenek
        QString ifTypeName = np.ifType().getName();
        if (ifTypeName == _sEthernet) {
            // Ha ez egy TRUNK tagja, akkor nem érdekes.
            if (!np.isNull(_sPortStapleId)) continue;
            // Ki kéne még hajítani az uplinkeket
            if (cLldpLink().getLinked(__q, np.getId())) continue; // Ez egy LLDP-vel felderített link
        }
        else if (ifTypeName != _sMultiplexor) {
            // TRUNK-nal a TRUNK tagjaihoz van rendelve az uplink
            int ix = 0;
            bool first = true;
            qlonglong linkedNodeId = NULL_ID;
            while (true) {
                ix = host().ports.indexOf(_sPortStapleId, np.get(_sPortId));
                if (ix < 0) break;
                qlonglong pid = cLldpLink().getLinked(__q, ports[ix]->getId());
                qlonglong hid = NULL_ID;
                if (pid != NULL_ID) hid = cNode().setById(__q, pid);
                if (first) {
                    first = false;
                    linkedNodeId = hid;
                }
                else {
                    if (linkedNodeId != hid) {
                        // WARNING !!!!
                    }
                }
            }
            if (linkedNodeId != NULL_ID) continue;
        else {
            // Más típusű port nem érdekes.
            continue;
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
    _DBGFN() << _sSpace << name() << endl;
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name()));
    }


    DBGFNL();
    return RS_ON;
}


