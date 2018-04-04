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
    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
    exit(mo.lastError == NULL ? r : mo.lastError->mErrorCode);
}

lv2portVLan::lv2portVLan() : lanView()
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
    lanView::tSetup<cPortVLan>(_tr);
}


/******************************************************************************/

cPortVLan::cPortVLan(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    ;
}

cPortVLan::~cPortVLan()
{
    ;
}

cInspector * cPortVLan::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePV(q, hsid, hoid, pid);
}

/******************************************************************************/

const cService *cDevicePV::pSrvSnmp   = NULL;

cDevicePV::cDevicePV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    // Ha nincs megadva protocol szervíz ('nil'), akkor SNMP device esetén az az SNMP lessz
    if (protoServiceId() == cService::nilId && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    /// Csak az SNMP lekérdezés támogatott (egyenlőre)
    if (protoServiceId() != pSrvSnmp->getId())
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem megfelelő proto_service_id!"));
}

cDevicePV::~cDevicePV()
{
    ;
}

#define _DM PDEB(VVERBOSE)
#define DM  _DM << endl;

void cDevicePV::postInit(QSqlQuery &_q, const QString&)
{
    DBGFN();
    cInspector::postInit(_q);
    if (pSubordinates != NULL)
        EXCEPTION(EDATA, -1,
                  trUtf8("A 'superior=custom'' nem volt megadva? (feature = %1) :\n%2")
                  .arg(dQuoted(hostService.getName(_sFeatures)))
                  .arg(hostService.identifying())   );
    pSubordinates = new QList<cInspector *>;
    // Azokat az al szolgáltatásokat, ami (már) nem tartozik hozzánk, le kell választani!
    cHostService hsf;   // A flag biteket true-ba tesszük
    hsf.setId(_sSuperiorHostServiceId, hostServiceId());    // Ha a mi al szolgáltatásunk
    hsf.setBool(_sDisabled, false);                         // nincs letiltva
    hsf.setBool(_sDeleted,  false);                         // vagy törölve
    hsf.setBool(_sFlag, true);
    QBitArray hsfWhereBits = hsf.mask(_sSuperiorHostServiceId, _sDisabled, _sDeleted);
    hsf.update(_q, false, hsf.mask(_sFlag), hsfWhereBits, EX_ERROR);
    // A host eredeti objektum típusa
    cSnmpDevice& dev = snmpDev();
    if (0 != dev.open(_q, snmp, EX_IGNORE)) {
        hostService.setState(_q, _sUnreachable, "SNMP Open error: " + snmp.emsg);
        EXCEPTION(EOK);
    }
    dev.fetchPorts(_q, 0);  // Csak a portok beolvasása. Címek, vlan-ok és paraméterek nem.
    tRecordList<cNPort>::iterator it, eit = dev.ports.end();
    for (it = dev.ports.begin(); it != eit; ++it) {      // Végigvesszük a portokat
        cNPort     *pPort    = *it;
        _DM << pPort->getName() << endl;
        // interface-nél alacsonyabb típussal nem foglalkozunk
        if (!(pPort->descr() >= cInterface::_descr_cInterface())) continue;
        // .....
    }
    DM;
    hsfWhereBits.setBit(hsf.toIndex(_sFlag));   // Ha maradt a true flag, akkor leválasztjuk
    hsf.clear(_sSuperiorHostServiceId);
    QString msg = trUtf8("Leválasztva %1 ről.").arg(hostService.names(_q));
    hsf.setNote(msg);
    int un = hsf.update(_q, false, hsf.mask(_sSuperiorHostServiceId, _sHostServiceNote), hsfWhereBits, EX_ERROR);
    if (un > 0) {
        msg += trUtf8(" %1 services.").arg(un);
        APPMEMO(_q, msg, RS_WARNING);
    }
    DM;
}

int cDevicePV::run(QSqlQuery& q, QString &runMsg)
{
    enum eNotifSwitch rs = RS_ON;
    _DBGFN() << QChar(' ') << name() << endl;
    // qlonglong parid = parentId(EX_IGNORE);
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    // ....
    (void)q;
    (void)runMsg;
    // ....
    DBGFNL();
    return rs;
}

