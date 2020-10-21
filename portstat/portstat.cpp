#include "portstat.h"
#include "lv2daterr.h"


#define VERSION_MAJOR   2
#define VERSION_MINOR   00
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
    cLv2QApp app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2portStat   mo;

    if (mo.lastError) {  // If error or end
        return mo.lastError->mErrorCode; // Error message by destructor
    }
    // The timer schedules hereafter
    int r = app.exec();
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2portStat::lv2portStat() : lanView()
{
    pSelfInspector = nullptr;
    // Self node (pSelfNode) now is not the questioner node, it's the target node, if set host_service_id set command line.
    if (lastError == nullptr) {
        try {
            if (pSelfNode == nullptr) {
                EXCEPTION(EDATA);
            }
            pDelete(pSelfNode);
            pSelfNode = new cNode();
            pSelfNode->fetchSelf(*pQuery);  // questioner
            // insertStart(*pQuery);
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
    cDevPortStat::pRLinkStat = cService::service(*pq, "rlinkstat");     // sub service by name
    cDevPortStat::pSrvPortVars  = cService::service(*pq, "portvars");      // sub service by name
    cDevPortStat::pSrvSnmp   = cService::service(*pq, _sSnmp);          // Protocol bay name
    cInterface i;   // Field indexs in the interface object
    cDevPortStat::ixPortIndex   = i.toIndex(_sPortIndex);
    cDevPortStat::ixIfTypeId    = i.toIndex(_sIfTypeId);
    cDevPortStat::ixPortStat    = i.toIndex(_sPortStat);
    cDevPortStat::ixPortOStat   = i.toIndex(_sPortOStat);
    cDevPortStat::ixPortAStat   = i.toIndex(_sPortAStat);
    cDevPortStat::ixIfdescr     = i.toIndex(_sIfDescr.toLower());
    cDevPortStat::ixLastChanged = i.toIndex(_sLastChanged);
    cDevPortStat::ixLastTouched = i.toIndex(_sLastTouched);
    cDevPortStat::ixDelegatePortState = cServiceVar().toIndex(_sDelegatePortState);
}

void lv2portStat::setup(eTristate _tr)
{
    staticInit(pQuery);
    // Create cDevPortStat object (main inspector object) and schedule query
    lanView::tSetup<cDevPortStat>(_tr);
}


/******************************************************************************/

const cService *cDevPortStat::pRLinkStat = nullptr;
const cService *cDevPortStat::pSrvPortVars  = nullptr;
const cService *cDevPortStat::pSrvSnmp   = nullptr;
int cDevPortStat::ixPortIndex   = NULL_IX;
int cDevPortStat::ixIfTypeId    = NULL_IX;
int cDevPortStat::ixPortStat    = NULL_IX;
int cDevPortStat::ixPortOStat   = NULL_IX;
int cDevPortStat::ixPortAStat   = NULL_IX;
int cDevPortStat::ixIfdescr     = NULL_IX;
int cDevPortStat::ixLastChanged = NULL_IX;
int cDevPortStat::ixLastTouched = NULL_IX;
int cDevPortStat::ixDelegatePortState = NULL_IX;

const QString cDevPortStat::sSnmpElapsed = "snmp_elapsed";

cDevPortStat::cDevPortStat(QSqlQuery& __q, const QString &_an)
    : cInspector(nullptr)  // Is not self host service !!
    , snmp()
{
    first = true;
    (void)_an;
    if (hostService.isNull() || pNode == nullptr || pService == nullptr) {
        EXCEPTION(EDATA);
    }

    // If not set protocol service ('nil'), and device type is snmpdevice then set protocol service is SNMP.
    qlonglong psid = protoServiceId();
    bool isSnmpDev = 0 == pNode->chkObjType<cSnmpDevice>(EX_IGNORE);
    if (psid == NIL_SERVICE_ID && isSnmpDev) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        hostService.update(__q, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(__q);
    }
    // Only SNMP query supported (for the time being)
    if (protoServiceId() != pSrvSnmp->getId() || !isSnmpDev) {
        EXCEPTION(EDATA, protoServiceId(),
               QObject::tr("Not supported protocol service : %1 , or device (%2) type in %3 instance of service.")
                  .arg(protoService().getName(), node().identifying(), name())    );
    }
}

cDevPortStat::~cDevPortStat()
{

}

void cDevPortStat::postInit(QSqlQuery &_q)
{
    DBGFN();
    cInspector::postInit(_q);
    if (pSubordinates != nullptr) {
        EXCEPTION(EDATA, -1,
                  tr("Not set the 'superior=custom' (This is also the default one)? (feature = %1) :\n%2")
                  .arg(dQuoted(hostService.getName(_sFeatures)))
                  .arg(hostService.identifying())   );
    }
    pSubordinates = new QList<cInspector *>;
    ifMap.clear();
    // Node original type
    cSnmpDevice& dev = snmpDev();
    if (0 != dev.open(_q, snmp, EX_IGNORE)) {
        hostService.setState(_q, _sUnreachable, "SNMP Open error: " + snmp.emsg, 0);
        EXCEPTION(EOK);
    }
    dev.fetchPorts(_q, 0);  // Fetch only ports
    tRecordList<cNPort>::iterator it, eit = dev.ports.end();
    for (it = dev.ports.begin(); it != eit; ++it) {      // for each all ports
        cNPort     *pPort    = *it;
        // Only the interface types are interesting
        if (!(pPort->descr() >= cInterface::_descr_cInterface())) continue;
        cInterface *pInterface = dynamic_cast<cInterface *>(pPort);
        qlonglong   pid = pInterface->getId();
        ifMap[pid] = new cPortStat(pInterface, this);
    }
    QString msg;
    int r = queryInit(_q, msg);
    if (r == RS_ON) {
        first = false;
    }
    else {
        hostService.setState(_q, _sUnreachable, msg, 0);
    }
    pSnmpElapsed = getInspectorVar(sSnmpElapsed);
    if (pSnmpElapsed == nullptr) {
        pSnmpElapsed = new cInspectorVar(*pQuery(), this, sSnmpElapsed, cServiceVarType::srvartype(*pQuery(), _sRuntime)->getId());
        pSnmpElapsed->postInit(*pQuery());
    }
}

int cDevPortStat::queryInit(QSqlQuery &_q, QString& msg)
{
    DBGFN();
    maxPortIndex = 0;
    if (0 != checkAvailableVars(_q)) {
        msg = tr("queryInit() : checkAvailableVars() : SNMP error : ") + snmp.emsg;
        DWAR() << msg << endl;
        return RS_UNREACHABLE;
    }

    // Sub services those that no longer belongs to us, you must be disconnected!
    cHostService hsf;   // Flags is set true, all sub service
    hsf.setId(_sSuperiorHostServiceId, hostServiceId());    // Connected
    QBitArray hsfWhereBits = hsf.mask(_sSuperiorHostServiceId);
    int marked = hsf.mark(_q, hsfWhereBits);
    PDEB(VERBOSE) << QString("Marked %1 record(s).").arg(marked) << endl;

    delayCounter = 0;
    foreach (qlonglong pid, ifMap.keys()) {
        if (ifMap[pid]->postInit(_q)) {
            int portIndex = int(ifMap[pid]->pInterface->getId(ixPortIndex));
            if (maxPortIndex < portIndex) maxPortIndex = portIndex;
        }
        else {
            delete ifMap.take(pid);     // It does not do anything, so we throw it away
        }
    }

    int removed = hsf.removeMarked(_q, hsfWhereBits); // Delete unnecessary sub service
    PDEB(INFO) << QString("Removed %1 record(s).").arg(removed) << endl;
    return RS_ON;
}

cPortStat::cPortStat(cInterface * pIf, cDevPortStat *par)
{
    parent     = par;
    pInterface = pIf;
    pRlinkStat = nullptr;
    pPortVars  = nullptr;
}

/// Port lekérdezések inicialozálása.
/// A portvars és rlinkstat al szolgáltatások cIspector objektumainak a létrehozása (ha szükséges)
bool cPortStat::postInit(QSqlQuery& q)
{
    bool isWanted = true;
    pInterface->_toReadBack = RB_NO;
 // portvars *********************************************
    pPortVars  = new cInspector(parent, parent->host().dup()->reconvert<cNode>(), parent->pSrvPortVars, pInterface->dup()->reconvert<cNPort>());
    // host service
    cHostService& vhs = pPortVars->hostService;
    // Record exists ?
    int n = vhs.completion(q);      // Fetch
    if (n  > 1) EXCEPTION(AMBIGUOUS, n, pPortVars->name());  // Incredible, multiple record
    if (n == 0) {                       // Not found, create if port type is ethernet
        if (cIfType::ifTypeId(_sEthernet) == pInterface->getId(_sIfTypeId)) {
            vhs.setId(_sServiceId, parent->pSrvPortVars->getId());
            vhs.setId(_sNodeId,    parent->host().getId());
            vhs.setId(_sPortId,    pInterface->getId());
            vhs.setId(_sSuperiorHostServiceId, parent->hostServiceId());
            vhs.setId(_sPrimeServiceId, NIL_SERVICE_ID);
            vhs.setId(_sProtoServiceId, NIL_SERVICE_ID);
            vhs.setName(_sNoalarmFlag, _sOn);    // Default: alarm disaled
            vhs._toReadBack = RB_ID;
            vhs.insert(q);
            vhs._toReadBack = RB_YES;
        }
        else {
            pDelete(pPortVars);         // Not found record, and not create.
        }
    }
    else {
        int ix = vhs.toIndex(_sSuperiorHostServiceId);
        if (vhs.getBool(_sDisabled)) {  // port_vars service is disabled ?
            vhs.clearState(q);
            pDelete(pPortVars);
        }
        else if (parent->hostServiceId() != vhs.getId(ix)) {
            // Not ours !!!???
            QBitArray m = vhs.clearStateFields(TS_FALSE);
            vhs.setId(ix, parent->hostServiceId());
            m.setBit(ix);
            vhs._toReadBack = RB_NO_ONCE;
            vhs.update(q, false, m);
        }
        else {  // OK
            vhs.mark(q, vhs.primaryKey(), false); // clear flag
        }
    }
    if (pPortVars != nullptr) {
        pPortVars->features();  // nem volt postInit()!, pMergedFeature == nullptr
        // service variables
        pPortVars->pVars = pPortVars->fetchVars(q);  // Fetch variables
        if (pPortVars->pVars == nullptr) {
            pPortVars->pVars = new QList<cInspectorVar *>;
        }
        else {
            foreach (cInspectorVar *p, *pPortVars->pVars) {
                p->pSrvVar->setFlag();
            }
        }
        bool resetRarefaction = cSysParam::getBoolSysParam(q, "reset_rarefaction", false);
        n = parent->vnames.size();
        if (n != parent->vTypes.size() || n != parent->vRarefactions.size()) EXCEPTION(EPROGFAIL);
        for (int i = 0; i < n; ++i) {   // Names of required variables
            const QString& name = parent->vnames.at(i);
            cInspectorVar *pVar = pPortVars->getInspectorVar(name, EX_IGNORE);
            if (pVar == nullptr) {      // If not exists, create
                pVar = new cInspectorVar(q, pPortVars, name, parent->vTypes.at(i));
                pVar->pSrvVar->setId(cServiceVar::ixRarefaction(), parent->vRarefactions.at(i));
                pVar->postInit(q);
                *(pPortVars->pVars) << pVar;
            }
            else {
                if (pVar->pSrvVar->getBool(_sDisabled)) {    // If disabled then dropp for list
                    if (pVar->pSrvVar->getId(cServiceVar::ixVarState()) != RS_UNKNOWN) {
                        pVar->pSrvVar->setName(cServiceVar::ixVarState(), _sUnknown);
                        pVar->pSrvVar->update(q, false, pVar->pSrvVar->mask(cServiceVar::ixVarState()));
                    }
                    pPortVars->pVars->removeAll(pVar);
                    delete pVar;
                    continue;
                }
                else if (resetRarefaction) {
                    if (pVar->pSrvVar->getId(cServiceVar::ixRarefaction()) != parent->vRarefactions.at(i)) {
                        pVar->pSrvVar->setId(cServiceVar::ixRarefaction(), parent->vRarefactions.at(i));
                        pVar->pSrvVar->update(q, false, pVar->pSrvVar->mask(cServiceVar::ixRarefaction()), pVar->pSrvVar->primaryKey());
                        pVar->postInit(q);
                    }
                }
            }
            pVar->pSrvVar->setFlag(false);   // unmarked
            pVar->initSkeepCnt(parent->delayCounter);
        }
        n = pPortVars->pVars->size();
        foreach (cInspectorVar *pVar, *pPortVars->pVars) {
            if (pVar->pSrvVar->getBool(_sFlag)) {
                pVar->pSrvVar->remove(q);           // delete from database
                pPortVars->pVars->removeAll(pVar);  // delete from list,
                delete pVar;                        // and delete object from memory
            }
        }
        if (pPortVars->pVars->isEmpty()) {  // If no variable, drop inspector object
            pDelete(pPortVars);
        }
    }
    if (pPortVars == nullptr) {
        isWanted = false;
        if (pInterface->getId(cDevPortStat::ixPortStat) != RS_UNKNOWN) {
            pInterface->setId(cDevPortStat::ixPortStat, RS_UNKNOWN);
            pInterface->update(q, false, pInterface->mask(cDevPortStat::ixPortStat));
        }
    }
    else {
        *parent->pSubordinates << pPortVars;
    }
 // portvars end

 // rlinkstat *****************************************************************
    if (!cIfType::ifType(pInterface->getId(cDevPortStat::ixIfTypeId)).isLinkage()) return isWanted;
    QString wMsg;   // Message if something is suspicious
    // By the time it is connected (log_links, lldp_links -> rlinkstat)
    qlonglong lpid; // Linked port ID
    eLinkResult lr = getLinkedPort(q, pInterface->getId(), lpid, wMsg, true);
    if (lr == LINK_CONFLICT || lr == LINK_RCONFLICT) {
        APPMEMO(q, wMsg, RS_CRITICAL);
    }
    if (lpid == NULL_ID) return isWanted;  // No likely link
    // Linked object
    cNPort *p = cNPort::getPortObjById(q, lpid);      // port ID --> node
    pRlinkStat = new cInspector(parent, cNode::getNodeObjById(q, p->getId(_sNodeId))->reconvert<cNode>(), parent->pRLinkStat, p);
    cHostService& lhs = pRlinkStat->hostService;
    // host_services record ?
    n = lhs.completion(q);
    // Only one record may be
    if (n  > 1) EXCEPTION(EDATA, n, parent->hostService.identifying(false));
    if (n == 1) {   // Found exists record, check and repair if necessary
        QBitArray sets(lhs.cols(), false);
        if (lhs.getBool(_sDisabled) || lhs.getBool(_sDeleted)) {
            // If it is deleted or disabled, we will proceed further
            pDelete(pRlinkStat);
            return isWanted;
        }
        // If another service sub service, then adopt!
        qlonglong hsid  = parent->hostServiceId();
        qlonglong shsid = lhs.getId(_sSuperiorHostServiceId);
        if (shsid != hsid) {
            int rs;
            QString msg;
            if (wMsg.isEmpty() == false || wMsg.endsWith("\n") == false) wMsg += "\n";
            if (shsid == NULL_ID) {
                wMsg += QObject::tr("Set superior service : NULL -> %1")
                        .arg(parent->name());
                msg = wMsg;
                rs = RS_RECOVERED;  // Superior service is recovered
            }
            else {
                wMsg += QObject::tr("Change superior service : %1 -> %2")
                        .arg(cHostService::fullName(q, shsid, EX_IGNORE))
                        .arg(parent->name());
                msg = wMsg + QObject::tr("\nLast service (%1) state : ").arg(lhs.fullName(q, EX_IGNORE))
                    + lhs.getName(_sHostServiceState) + " ("
                    + lhs.getName(_sHardState) + ", "
                    + lhs.getName(_sSoftState) + ") last changed : "
                    + lhs.view(q, _sLastChanged) + " / "
                    + lhs.view(q, _sLastTouched);
                // Reverse logic, if it was good, is the more serious
                rs = lhs.getId(_sHostServiceState) > RS_WARNING ? RS_WARNING : RS_CRITICAL;
            }
            APPMEMO(q, msg, rs);
            lhs.setId(_sSuperiorHostServiceId, hsid);
            sets.setBit(lhs.toIndex(_sSuperiorHostServiceId));  // need update
        }
        if (!wMsg.isEmpty()) {
            QString s = lhs.getNote();
            if (!s.contains(wMsg)) {
                s += " " + wMsg;
                lhs.setNote(s);
                sets.setBit(lhs.toIndex(_sHostServiceNote));    // need update
            }
        }
        lhs.setBool(_sFlag, false);  // Ours, clear flag
        sets.setBit(lhs.toIndex(_sFlag));
        lhs.update(q, true, sets);
    }
    else {          // Not found, create
        lhs.clear();
        lhs.setId(_sNodeId, pRlinkStat->nodeId());
        lhs.setId(_sServiceId, pRlinkStat->serviceId());
        lhs.setId(_sSuperiorHostServiceId, parent->hostServiceId());
        lhs.setId(_sPortId, lpid);
        // lhs.setBool(_sFlag, false); = DEFAULT
        QString note =
                QObject::tr("Automatically generated by portstat (%1) : \n")
                                .arg(parent->hostService.identifying());
        lhs.setNote(note + wMsg);
        // Default: no alarm
        lhs.setName(_sNoalarmFlag, _sOn);
        lhs.insert(q);
    }
    *parent->pSubordinates << pRlinkStat;
 // rlinkstat end
    return true;
}

static bool changeStringField(QString s, int ix, cInterface& iface, QBitArray& mask)
{
    if (s != iface.getName(ix)) {
        iface.setName(ix, s);
        mask.setBit(ix);
        return true;
    }
    return false;
}

int cDevPortStat::checkAvailableVars(QSqlQuery& q)
{
    vnames.clear();
    vTypes.clear();
    vRarefactions.clear();
    snmpTabColumns.clear();
    snmpTabOIds.clear();
    // Variable names (all)
    vnames << _sIfMtu << _sIfSpeed
           << _sIfInOctets  << _sIfInUcastPkts  << _sIfInNUcastPkts  << _sIfInDiscards  << _sIfInErrors
           << _sIfOutOctets << _sIfOutUcastPkts << _sIfOutNUcastPkts << _sIfOutDiscards << _sIfOutErrors;
    // Variable (default) type names (all)
    cServiceVarType st;
    qlonglong ifBytesId = st.getIdByName(q, "ifbytes");
    qlonglong ifPacksId = st.getIdByName(q, "ifpacks");
    vTypes << st.getIdByName(q, _sIfMtu.toLower()) << st.getIdByName(q, _sIfSpeed.toLower())
           << ifBytesId << ifPacksId << ifPacksId << ifPacksId << ifPacksId
           << ifBytesId << ifPacksId << ifPacksId << ifPacksId << ifPacksId;
    vRarefactions << 20 << 20       // MTU, speed : It rarely changes
                  <<  1  << 2 << 2 << 10 << 10
                  <<  1  << 2 << 2 << 10 << 10;
    snmpTabColumns << _sIfIndex << _sIfAdminStatus << _sIfOperStatus << _sIfDescr;
    const int important = snmpTabColumns.size();
    snmpTabColumns << vnames;
    int r = snmp.columns2oids(_sIfMib, snmpTabColumns, snmpTabOIds);
    if (r) return r;
    QBitArray bits;
    r = snmp.checkTableColumns(snmpTabOIds, bits);
    if (r) return r;
    if (bits.count(false)) {    // Number Unavailable parameters
        int n = bits.size();
        for (int i = 0, j = 0; i < n; ++i, ++j) {
            bool accessible = bits[i];
            if (!accessible) {
                if (i < important) return -1;   // These can not be inaccessible.
                snmpTabColumns.removeAt(j);     // Dropp parameter
                snmpTabOIds.removeAt(j);
                vnames.removeAt(j - important);
                vTypes.removeAt(j - important);
                vRarefactions.removeAt(j - important);
                --j;
            }
        }
    }
    return 0;   // OK
}


int cDevPortStat::run(QSqlQuery& q, QString &runMsg)
{
    if (first) {
        if (RS_ON != queryInit(q, runMsg)) {
            return RS_UNREACHABLE;
        }
        first = false;
    }
    enum eNotifSwitch rs = RS_ON;   // State of pstat
    _DBGFN() << QChar(' ') << name() << endl;
    qlonglong parid = parentId(EX_IGNORE);
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::tr("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    cTable      tab;    // Interface table container
    QElapsedTimer timer;
    timer.start();
    int r = snmp.getTable(snmpTabOIds, snmpTabColumns, tab, oid(maxPortIndex));
    int state;
    pSnmpElapsed->setValue(q, QVariant(timer.elapsed()), state);
    if (r) {
        runMsg += tr("SNMP get table error : %1 in %2; host : %3, from %4 parent service.\n")
                .arg(snmp.emsg)
                .arg(name())
                .arg(lanView::getInstance()->selfNode().getName())
                .arg(parid == NULL_ID ? "NULL" : cHostService::fullName(q, parid, EX_IGNORE));
        return RS_UNREACHABLE;
    }
    int n = tab.rows();
    int i;
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != nullptr) ps->flag = false;   // Marked; status is not set
    }
    for (i = 0; i < n; i++) {
        bool    ok;
        QString ifDescr = tab[_sIfDescr][i].toString();
        int     ifIndex = tab[_sIfIndex][i].toInt(&ok);
        if (!ok) {
            rs = RS_CRITICAL;
            QVariant v = tab[_sIfIndex][i];
            runMsg += tr("ifIndex (%1) is not numeric : %2, in %3; host : %4, from %5 parent service.\n")
                    .arg(ifDescr)
                    .arg(debVariantToString(v))
                    .arg(name())
                    .arg(lanView::getInstance()->selfNode().getName())
                    .arg(parid == NULL_ID ? "NULL" : cHostService::fullName(q, parid, EX_IGNORE));
            continue;
        }

        int ix = host().ports.indexOf(ixPortIndex, QVariant(ifIndex));  // Find port in ports container
        if (ix < 0)
            continue;   // Not Found
        // Get interface object
        cInterface&     iface = *host().ports[ix]->reconvert<cInterface>();
        // Find cPortStat object
        QMap<qlonglong, cPortStat *>::iterator ifi = ifMap.find(iface.getId());
        if (ifi == ifMap.end())
            continue;   // Not found
        cPortStat& portstat = **ifi;
        QBitArray bitsSet = iface.mask(ixLastTouched);              // bits: Field is changed
        iface.set(ixLastTouched, QDateTime::currentDateTime());     // utolsó status állítás ideje, most.
        iface.set(ixLastChanged, QDateTime::currentDateTime());     // Last changed, if changed any state.
        QString eMsg;

        int    _opstat = tab[_sIfOperStatus][i].toInt(&ok);
        QString opstat = snmpIfStatus(_opstat, EX_IGNORE);
        if (!ok || opstat.isEmpty()) {
            eMsg = QObject::tr("Port %1: ifOperStatus is not numeric, or invalid : %2").arg(iface.getName(), tab[_sIfOperStatus][i].toString());
            opstat = _sUnknown;
        }

        int    _adstat = tab[_sIfAdminStatus][i].toInt(&ok);
        QString adstat = snmpIfStatus(_adstat, EX_IGNORE);
        if (!ok) {
            QString msg = tr("Port %1: ifAdminStatus is not numeric, or invalid : %2").arg(iface.getName(), tab[_sIfAdminStatus][i].toString());
            eMsg = msgCat(eMsg, msg, "\n");
            adstat = _sUnknown;
        }
        bool change;
        change = changeStringField(ifDescr, ixIfdescr,   iface, bitsSet);
        change = changeStringField(opstat,  ixPortOStat, iface, bitsSet) || change;
        change = changeStringField(adstat,  ixPortAStat, iface, bitsSet) || change;
        bitsSet.setBit(ixLastTouched, change);

        // rlinkstat if exists
        if (portstat.pRlinkStat != nullptr) {
            cInspector& insp = *portstat.pRlinkStat;
            QString state = _sUnknown;
            if      (_opstat == IF_UP)   state = _sOn;        // On
            else if (_adstat == IF_UP)   state = _sDown;      // Off
            else if (_adstat == IF_DOWN) state = _sWarning;   // Disabled
            QString msg = tr("interface %1:%2 state : op:%3/adm:%4").
                    arg(node().getName(),iface.getName()).arg(opstat, adstat);
            eMsg = msgCat(eMsg, msg, "\n");
            if (nullptr == insp.hostService.setState(q, state, eMsg, parid)) { // törölték!
                if (1 != pSubordinates->removeAll(portstat.pRlinkStat)) {
                    EXCEPTION(EPROGFAIL);
                }
                pDelete(portstat.pRlinkStat);
            }
            insp.flag = true; // State is set
        }

        // Ha van kapcsolódó portvars al szolgáltatás:
        if (portstat.pPortVars != nullptr) {
            cInspector& insp = *portstat.pPortVars;
            int sstate = RS_ON;         // delegated state to service state
            int pstate = RS_INVALID;    // delegated state to port state
            int rs;
            QString vMsg, msg;
            if (iface.getBool(_sDelegateOperStat)  && _opstat != IF_UP) {
                msg = tr("Port operate status is not 'UP'.");
                msgAppend(&vMsg, msg);
                sstate = RS_CRITICAL;
            }
            if (iface.getBool(_sDelegateAdminStat) && _adstat != IF_UP) {
                msg = tr("Port admin status is not 'UP'.");
                msgAppend(&vMsg, msg);
                sstate = RS_CRITICAL;
            }
            qlonglong raw;
            foreach (QString vname, vnames) {   // foreach variable names
                cInspectorVar *psv = insp.getInspectorVar(vname);   // Get variable obj.
                if (psv == nullptr) continue;                   // not found, next
                raw = tab[vname][i].toLongLong();               // Get raw value from SNMP query
                rs = psv->setValue(q, raw, sstate, TS_NULL);
                if (psv->pSrvVar->getBool(ixDelegatePortState) && rs > pstate) pstate = rs;
                msg = psv->pSrvVar->getName(_sStateMsg);
                QString val = psv->pSrvVar->getName(_sServiceVarValue);
                QString srs = notifSwitch(rs, EX_IGNORE);
                QString vst = psv->pSrvVar->getName(_sVarState);
                if (msg.isEmpty() && srs.isEmpty()) {   // skipping (rarefaction)
                    msg = tr("Var %1 ( = '%2'/'%3') is skipped.").arg(vname, val, vst);
                }
                else {
                    if (srs.isEmpty()) srs = _sUnKnown;
                    msg = tr("Var '%1' = '%2'/'%3' >> '%4' : '%5'").arg(vname, val, vst, srs, msg);
                }
                msgAppend(&vMsg, msg);
            }
            insp.hostService.setState(q, notifSwitch(sstate), vMsg, NULL_ID, parid);
            insp.flag = true; // State is set
            if (pstate != RS_INVALID && changeStringField(notifSwitch(pstate), ixPortStat, iface, bitsSet)) {
                bitsSet.setBit(ixLastChanged);
            }
        }
        // Write interface states
        int un = iface.update(q, false, bitsSet);
        if (un != 1) {
            QString msg = tr("Az update visszatérési érték %1 hiba, Service : %2, interface : %3")
                    .arg(un)
                    .arg(iface.identifying(false))
                    .arg(hostService.fullName(q, EX_IGNORE));
            APPMEMO(q, msg, RS_CRITICAL);
        }
    }
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != nullptr && ps->flag == false) {   // be lett állítva státusz ??? Ha még nem:
            ps->hostService.setState(q, _sUnknown, tr("Not set by query"), NULL_ID, parid);
        }
    }
    DBGFNL();
    return rs;
}

