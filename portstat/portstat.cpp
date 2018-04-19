#include "portstat.h"
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
    cDevicePSt::pPortVars  = cService::service(*pq, "portvars");
    cDevicePSt::pSrvSnmp   = cService::service(*pq, _sSnmp);
    cInterface i;
    cDevicePSt::ixPortOStat         = i.toIndex(_sPortOStat);
    cDevicePSt::ixPortAStat         = i.toIndex(_sPortAStat);
    cDevicePSt::ixIfdescr           = i.toIndex(_sIfDescr.toLower());
    /*
    cDevicePSt::ixIfmtu             = i.toIndex(_sIfMtu.toLower());
    cDevicePSt::ixIfspeed           = i.toIndex(_sIfSpeed.toLower());
    cDevicePSt::ixIfinoctets        = i.toIndex(_sIfInOctets.toLower());
    cDevicePSt::ixIfinucastpkts     = i.toIndex(_sIfInUcastPkts.toLower());
    cDevicePSt::ixIfinnucastpkts    = i.toIndex(_sIfInUcastPkts.toLower());
    cDevicePSt::ixIfindiscards      = i.toIndex(_sIfInDiscards.toLower());
    cDevicePSt::ixIfinerrors        = i.toIndex(_sIfInErrors.toLower());
    cDevicePSt::ixIfoutoctets       = i.toIndex(_sIfOutOctets.toLower());
    cDevicePSt::ixIfoutucastpkts    = i.toIndex(_sIfOutUcastPkts.toLower());
    cDevicePSt::ixIfoutnucastpkts   = i.toIndex(_sIfOutNUcastPkts.toLower());
    cDevicePSt::ixIfoutdiscards     = i.toIndex(_sIfOutDiscards.toLower());
    cDevicePSt::ixIfouterrors       = i.toIndex(_sIfOutErrors.toLower());
    */
    cDevicePSt::ixStatLastModify    = i.toIndex(_sStatLastModify);
}

void lv2portStat::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::tSetup<cPortStat>(_tr);
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
const cService *cDevicePSt::pPortVars  = NULL;
const cService *cDevicePSt::pSrvSnmp   = NULL;
int cDevicePSt::ixPortOStat = NULL_IX;
int cDevicePSt::ixPortAStat = NULL_IX;
int cDevicePSt::ixIfdescr = NULL_IX;
/*
int cDevicePSt::ixIfmtu = NULL_IX;
int cDevicePSt::ixIfspeed = NULL_IX;
int cDevicePSt::ixIfinoctets = NULL_IX;
int cDevicePSt::ixIfinucastpkts = NULL_IX;
int cDevicePSt::ixIfinnucastpkts = NULL_IX;
int cDevicePSt::ixIfindiscards = NULL_IX;
int cDevicePSt::ixIfinerrors = NULL_IX;
int cDevicePSt::ixIfoutoctets = NULL_IX;
int cDevicePSt::ixIfoutucastpkts = NULL_IX;
int cDevicePSt::ixIfoutnucastpkts = NULL_IX;
int cDevicePSt::ixIfoutdiscards = NULL_IX;
int cDevicePSt::ixIfouterrors = NULL_IX;
*/
int cDevicePSt::ixStatLastModify = NULL_IX;


cDevicePSt::cDevicePSt(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
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

cDevicePSt::~cDevicePSt()
{
    ;
}

#define _DM PDEB(VVERBOSE)
#define DM  _DM << endl;

void cDevicePSt::postInit(QSqlQuery &_q, const QString&)
{
    DBGFN();
    cInspector::postInit(_q);
    if (pSubordinates != NULL)
        EXCEPTION(EDATA, -1,
                  trUtf8("A 'superior=custom'' nem volt megadva? (feature = %1) :\n%2")
                  .arg(dQuoted(hostService.getName(_sFeatures)))
                  .arg(hostService.identifying())   );
    pSubordinates = new QList<cInspector *>;
    rlinkMap.clear();
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
        // interface-nél alacsonyabb típussal nem foglalkozunk, nincs állpota
        if (!(pPort->descr() >= cInterface::_descr_cInterface())) continue;
        // Interface, van/kell statusz
        cInterface *pInterface = dynamic_cast<cInterface *>(pPort);
        qlonglong   pid = pInterface->getId();
        DM;
        varsInit(_q, pInterface);
        DM;
        cInspector *pRLnkSt  = NULL;    // Al szolgáltatás, ha van. Csak rlinkstat lehet/támogatott (és portvars, amit az elöbb kezeltünk)
        QString wMsg;   // Gyanús dolgok
         // Mivel van linkelve?
        qlonglong lldp_pid;     // Linkelt port by LLDP
        qlonglong logl_pid;     // Linkelt port by log_links table
        cLldpLink lldp;
        cLogLink  logl;
        lldp_pid = lldp.getLinked(_q, pid);  // Az LLDP szerint ?
        logl_pid = logl.getLinked(_q, pid);  // A logikai linkek szerint
        DM;
        if (lldp_pid != NULL_ID && logl_pid != NULL_ID && lldp_pid != logl_pid) { // Caveat
            // A probléma leírása :
            wMsg = trUtf8("A %1 port logikai linkje ütközik az LLDP linkkel."
                          " Szolgáltatás példány : %2\n")
                    .arg(pInterface->getFullName(_q))
                    .arg(hostService.names(_q));
            wMsg += trUtf8("LLDP    : ") + lldp.show(true) + "\n";
            wMsg += trUtf8("Logikai : ") + logl.show(true) + "\n";
            wMsg += trUtf8("A fizikai linkek lánca :\n");
            wMsg += logl.showChain().join("\n");
            APPMEMO(_q, wMsg, RS_CRITICAL);
            lldp_pid = logl_pid = NULL_ID;   // Nem hisszük el egyiket sem
        }
        else if (logl_pid != NULL_ID && lldp_pid != NULL_ID && lldp_pid == logl_pid) { // Fine
            wMsg = trUtf8("A log és LLDP link azonos.");
        }
        else if (lldp_pid == NULL_ID && logl_pid != NULL_ID) {
            lldp_pid = logl_pid;     // Ha nincs az LLDP szerint, akkor elhisszük a logikait
            wMsg = trUtf8("Nincs LLDP link, csak logikai.");
        }
        else if (lldp_pid != NULL_ID && logl_pid == NULL_ID) {
            wMsg = trUtf8("Nincs logikai link, csak LLDP.");
        }
        // Likey linked port ID is lldp_pid
        if (lldp_pid == NULL_ID) continue;  // No likely link
        DM;
        // Passzív objektum,
        cNPort *p = cNPort::getPortObjById(_q, lldp_pid);      // port, .. node
        pRLnkSt = new cInspector(this, cNode::getNodeObjById(_q, p->getId(_sNodeId))->reconvert<cNode>(), pRLinkStat, p);
        cHostService& hs = pRLnkSt->hostService;
        DM;
        // Van host_services rekord ?
        int n = hs.completion(_q);
        // Több rekord nem lehet(ne), ha töröljük mindet, aminek nem nil a proto és prime szolg.-a,
        // akkor meg kell szünnie a redundanciának.
        if (n  > 1) {
            QSqlQuery qq = getQuery();
            cHostService hsc = hs;
            hs.clear();
            do {
                if (hsc.getId(_sPrimeServiceId) != cService::nilId || hsc.getId(_sProtoServiceId) != cService::nilId) {
                    QString msg = trUtf8("Hibás host_service objektum : %1\n"
                                     "A rekord törlő gazda (superior) szolgáltatás : %2"
                                     ).arg(hsc.names(_q), hostService.names(_q));
                    hsc.remove(qq);
                    APPMEMO(qq, msg, RS_WARNING);
                    --n;
                }
                else {
                    hs = hsc;
                }
            } while (hsc.next(_q));
        }
        if (n  > 1) EXCEPTION(EPROGFAIL, n); // Incredible
        DM;
        if (n == 1) {   // Van, és (most már) egyedi, ha kell javítjuk
            QBitArray sets(hostService.cols(), false);
            if (hs.getBool(_sDisabled) || hs.getBool(_sDeleted)) {
                // Ha disabled, vagy deleted, akkor nem bántjuk, nem foglakozunk vele
                delete pRLnkSt;
                continue;
            }
            // máshova mutat a superior, a link szerint viszont a mienk !!!
            qlonglong hsid  = hostServiceId();
            qlonglong shsid = hs.getId(_sSuperiorHostServiceId);
            if (shsid != hsid) {
                int rs;
                QString msg;
                if (wMsg.isEmpty() == false || wMsg.endsWith("\n") == false) wMsg += "\n";
                if (shsid == NULL_ID) {
                    wMsg += trUtf8("Gazda szolgáltatás (superior) beállítása : NULL -> %1")
                            .arg(hostService.names(_q));
                    msg = wMsg;
                    rs = RS_RECOVERED;
                }
                else {
                    wMsg += trUtf8("Gazda szolgáltatás (superior) csere : %1 -> %2")
                            .arg(cHostService::names(_q, shsid))
                            .arg(hostService.names(_q));
                    msg = wMsg + trUtf8("\nAktuális service (%1) státusz : ").arg(hs.names(_q))
                        + hs.getName(_sHostServiceState) + " ("
                        + hs.getName(_sHardState) + ", "
                        + hs.getName(_sSoftState) + ") utolsó modosítás : "
                        + hs.view(_q, _sLastChanged) + " / "
                        + hs.view(_q, _sLastTouched);
                    // Fordított logika, ha ok a státusz akkor igazán gáz a váltás
                    rs = hs.getId(_sHostServiceState) > RS_WARNING ? RS_WARNING : RS_CRITICAL;
                }
                APPMEMO(_q, msg, rs);
                hs.setId(_sSuperiorHostServiceId, hsid);
                sets.setBit(hs.toIndex(_sSuperiorHostServiceId));
            }
            if (!wMsg.isEmpty()) {
                QString s = hs.getNote();
                if (!s.contains(wMsg)) {
                    s += " " + wMsg;
                    hs.setNote(s);
                    sets.setBit(hs.toIndex(_sHostServiceNote));
                }
            }
            hs.setBool(_sFlag, false);  // Mienk, töröljük a flag-et
            sets.setBit(hs.toIndex(_sFlag));
            hs.update(_q, true, sets);
        }
        else {          // Nincs (már vagy még) létrehozzuk
            hs.clear();
            hs.setId(_sNodeId, pRLnkSt->nodeId());
            hs.setId(_sServiceId, pRLnkSt->serviceId());
            hs.setId(_sSuperiorHostServiceId, hostServiceId());
            hs.setId(_sPortId, lldp_pid);
            hs.setBool(_sFlag, false);
            QString note =
                    trUtf8("Automatikusan generálva a portstat (%1) által. ")
                    .arg(hostService.identifying());
            hs.setNote(note + wMsg);
            // Nem kérünk riasztást az automatikusan generált rekordhoz.
            hs.setName(_sNoalarmFlag, _sOn);
            hs.insert(_q);
        }
        *pSubordinates << pRLnkSt;
        rlinkMap[pid] = pRLnkSt;
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

void cDevicePSt::varsInit(QSqlQuery &_q, cInterface *pInterface)
{
    qlonglong pid = pInterface->getId();
    // Passzív objektum, hostService adattag csak részben inicializálva
    cInspector *pPVars = new cInspector(this, host().dup()->reconvert<cNode>(), pPortVars, pInterface->dup()->reconvert<cNPort>());
    cHostService& hs = pPVars->hostService;
    // Van a porthoz portvars host_services rekord ?
    int n = hs.completion(_q);
    if (n  > 1) EXCEPTION(AMBIGUOUS, n, name());
    if (n == 0) {   // Nincs létrehozzuk...
        hs.setId(_sServiceId, pPortVars->getId());
        hs.setId(_sNodeId,    host().getId());
        hs.setId(_sPortId,    pid);
        hs.setId(_sSuperiorHostServiceId, hostServiceId());
        hs.setId(_sPrimeServiceId, cService::nilId);
        hs.setId(_sProtoServiceId, cService::nilId);
        hs.setName(_sNoalarmFlag, _sOn);    // Tiltott alarm
        hs.insert(_q);
    }
    else if (hostServiceId() != hs.getId(_sSuperiorHostServiceId)) {
        // Nem a mienk!!!???
        hs.setId(_sSuperiorHostServiceId, hostServiceId());
        hs.update(_q, false, hs.mask(_sSuperiorHostServiceId));
    }
    DM;
    pPVars->pVars = pPVars->fetchVars(_q);  // A változók beolvasása
    DM;
    if (pPVars->pVars == NULL) {
        DM;
        pPVars->pVars = new tOwnRecords<cServiceVar, cHostService>(&(pPVars->hostService));
    }
    DM;
    QStringList vnames, vtypes;
    // Ezek a változó navek kellenek
    vnames << _sIfMtu << _sIfSpeed
           << _sIfInOctets  << _sIfInUcastPkts  << _sIfInNUcastPkts  << _sIfInDiscards  << _sIfInErrors
           << _sIfOutOctets << _sIfOutUcastPkts << _sIfOutNUcastPkts << _sIfOutDiscards << _sIfOutErrors;
    // Ezek a default típus nevek, ha létre kell hozni
    vtypes << _sIfMtu.toLower() << _sIfSpeed.toLower()
           << "ifbytes" << "ifpacks" << "ifpacks" << "ifpacks" << "ifpacks"
           << "ifbytes" << "ifpacks" << "ifpacks" << "ifpacks" << "ifpacks";
    n = vnames.size();
    if (n != vtypes.size()) EXCEPTION(EPROGFAIL);
    DM;
    for (int i = 0; i < n; ++i) {
        const QString& name = vnames.at(i);
        cServiceVar *pVar = pPVars->pVars->get(name, EX_IGNORE);
        if (pVar == NULL) {
            pVar = new cServiceVar;
            pVar->setName(name);
            pVar->setId(_sServiceVarTypeId, cServiceVarType().getIdByName(_q, vtypes.at(i)));
            pVar->setId(_sHostServiceId, pPVars->hostServiceId());
            pVar->insert(_q);
            *(pPVars->pVars) << pVar;
        }
    }
    pVarsMap[pid] = pPVars;
    *pSubordinates << pPVars;
}

static void setString(QString s, int ix, cInterface& iface, QBitArray& mask)
{
    if (s != iface.getName(ix)) {
        iface.setName(ix, s);
        mask.setBit(ix);
    }
}

int cDevicePSt::run(QSqlQuery& q, QString &runMsg)
{
    enum eNotifSwitch rs = RS_ON;
    _DBGFN() << QChar(' ') << name() << endl;
    qlonglong parid = parentId(EX_IGNORE);
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    cTable      tab;    // Interface table container
    QStringList cols;   // Table col names container
    cols << _sIfIndex << _sIfAdminStatus << _sIfOperStatus << _sIfDescr << _sIfMtu <<_sIfSpeed
         << _sIfInOctets << _sIfInUcastPkts << _sIfInNUcastPkts << _sIfInDiscards << _sIfInErrors
         << _sIfOutOctets << _sIfOutUcastPkts << _sIfOutNUcastPkts << _sIfOutDiscards << _sIfOutErrors;

    int r = snmp.getTable(_sIfMib, cols, tab);
    if (r) {
        runMsg = trUtf8("SNMP get table error : %1 in %2 host, from %3 parent service.")
                .arg(snmp.emsg)
                .arg(name())
                .arg(parid == NULL_ID ? "NULL" : cHostService::names(q, parid));
        return RS_UNREACHABLE;
    }
    int n = tab.rows();
    int i;
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != NULL) ps->flag = false;   // A státuszt még nem állítottuk be.
    }
    for (i = 0; i < n; i++) {
        bool    ok;

        QString ifDescr = tab[_sIfDescr][i].toString();
        int     ifIndex = tab[_sIfIndex][i].toInt(&ok);
        if (!ok) {
            rs = RS_CRITICAL;
            QVariant v = tab[_sIfIndex][i];
            runMsg += trUtf8("ifIndex (%1) is not numeric : %2, in %3 host, from %4 parent service. ")
                    .arg(ifDescr)
                    .arg(debVariantToString(v))
                    .arg(name())
                    .arg(parid == NULL_ID ? "NULL" : cHostService::names(q, parid));
        }

        int ix = host().ports.indexOf(_sPortIndex, QVariant(ifIndex));
        if (ix < 0)
            continue;   // Nincs ilyen indexű portunk, eldobjuk
        // Előkapjuk az interface objektumunkat, a konténerből, ehhez kell az indexük
        cInterface&     iface = * dynamic_cast<cInterface *>(host().ports[ix]);

        QString eMsg;

        QString opstat = snmpIfStatus(tab[_sIfOperStatus][i].toInt(&ok));
        if (!ok) {
            eMsg = QObject::trUtf8("ifOperStatus is not numeric : %1").arg(tab[_sIfOperStatus][i].toString());
            opstat = _sUnknown;
        }

        QString adstat = snmpIfStatus(tab[_sIfAdminStatus][i].toInt(&ok));
        if (!ok) {
            if (!eMsg.isEmpty()) eMsg += "\n";
            eMsg += QObject::trUtf8("ifAdminStatus is not numeric : %1").arg(tab[_sIfAdminStatus][i].toString());
            adstat = _sUnknown;
        }

        // Ha van kapcsolódó rlinkstat al szolgáltatás:
        QMap<qlonglong, cInspector *>::iterator it = rlinkMap.find(iface.getId());
        if (it != rlinkMap.end()) {
            // Előkapjuk a kapcsolódó szolgálltatást
            cInspector& ns   = **it;
            QString state = _sUnKnown;
            if      (opstat == _sUp)   state = _sOn;        // On
            else if (adstat == _sUp)   state = _sDown;      // Off
            else if (adstat == _sDown) state = _sWarning;   // Disabled
            QString msg = trUtf8("if state : op:%1/adm:%2").arg(opstat).arg(adstat);
            if (!eMsg.isEmpty()) msg += "\n" + eMsg;
            ns.hostService.setState(q, state, msg, parid);
            ns.flag = true; // beállítva;
        }

        // Interface állapot:
        QBitArray bitsSet(iface.cols(), false);     // modosítandó mezők mask
        iface.set(ixStatLastModify, QDateTime::currentDateTime());  // utolsó status állítás ideje, most.
        bitsSet.setBit(ixStatLastModify);
        setString(ifDescr, ixIfdescr,   iface, bitsSet);
        setString(opstat,  ixPortOStat, iface, bitsSet);
        setString(adstat,  ixPortAStat, iface, bitsSet);
        PDEB(VERBOSE) << "Update port stat: " << pNode->getName() << QChar(':') << ifDescr << endl;
        int un = iface.update(q, false, bitsSet);
        if (un != 1) {
            QString msg = trUtf8("Az update visszatérési érték %1 hiba, Service : %2, interface : %3")
                    .arg(un)
                    .arg(iface.identifying(false))
                    .arg(hostService.names(q));
            APPMEMO(q, msg, RS_CRITICAL);
        }
        // Ha van kapcsolódó portvars al szolgáltatás:
        it = pVarsMap.find(iface.getId());
        if (it != pVarsMap.end()) {
            // Előkapjuk a kapcsolódó szolgálltatást
            cInspector& ns  = **it;
            int state = RS_ON;
            ns.setServiceVar(q, _sIfMtu,          tab[_sIfMtu          ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfSpeed,        tab[_sIfSpeed        ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfInOctets,     tab[_sIfInOctets     ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfInUcastPkts,  tab[_sIfInUcastPkts  ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfInNUcastPkts, tab[_sIfInNUcastPkts ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfInDiscards,   tab[_sIfInDiscards   ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfInErrors,     tab[_sIfInErrors     ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfOutOctets,    tab[_sIfOutOctets    ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfOutUcastPkts, tab[_sIfOutUcastPkts ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfOutNUcastPkts,tab[_sIfOutNUcastPkts][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfOutDiscards,  tab[_sIfOutDiscards  ][i].toULongLong(), state);
            ns.setServiceVar(q, _sIfOutErrors,    tab[_sIfOutErrors    ][i].toULongLong(), state);
            ns.hostService.setState(q, notifSwitch(state), _sNul, parid);
            ns.flag = true; // beállítva;
        }
        else {
            PDEB(INFO) << trUtf8("No variables : %1/%2").arg(name(), ifDescr) << endl;
        }
    }
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != NULL && ps->flag == false) {   // be lett állítva státusz ??? Ha még nem:
            ps->hostService.setState(q, _sUnreachable, trUtf8("No data") , parid);
        }
    }
    DBGFNL();
    return rs;
}

