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

    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2portStat   mo;

    if (mo.lastError) {  // If error or end
        return mo.lastError->mErrorCode; // Error message by destructor
    }
    // The timer schedules hereafter
    int r = app.exec();
    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
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
    : cInspector(nullptr)  // Self host service
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
               QObject::trUtf8("Not supported protocol service : %1 , or device (%2) type in %3 instance of service.")
                  .arg(protoService().getName(), node().identifying(), name())    );
    }
}

cDevPortStat::~cDevPortStat()
{
    ;
}

#if 1
#define _DM PDEB(VVERBOSE)
#define DM  _DM << endl;
#else
#define _DM
#define DM
#endif

void cDevPortStat::postInit(QSqlQuery &_q, const QString&)
{
    DBGFN();
    cInspector::postInit(_q);
    if (pSubordinates != nullptr)
        EXCEPTION(EDATA, -1,
                  trUtf8("Not set the 'superior=custom' (This is also the default one)? (feature = %1) :\n%2")
                  .arg(dQuoted(hostService.getName(_sFeatures)))
                  .arg(hostService.identifying())   );
    pSubordinates = new QList<cInspector *>;
    ifMap.clear();
    // Node original type
    cSnmpDevice& dev = snmpDev();
    if (0 != dev.open(_q, snmp, EX_IGNORE)) {
        hostService.setState(_q, _sUnreachable, "SNMP Open error: " + snmp.emsg);
        EXCEPTION(EOK);
    }
    dev.fetchPorts(_q, 0);  // Fetch only ports
    tRecordList<cNPort>::iterator it, eit = dev.ports.end();
    for (it = dev.ports.begin(); it != eit; ++it) {      // for each all ports
        cNPort     *pPort    = *it;
        _DM << pPort->getName() << endl;
        // Only the interface types are interesting
        if (!(pPort->descr() >= cInterface::_descr_cInterface())) continue;
        cInterface *pInterface = dynamic_cast<cInterface *>(pPort);
        qlonglong   pid = pInterface->getId();
        DM;
        ifMap[pid] = new cPortStat(pInterface, this);
        DM;
    }
    QString msg;
    int r = queryInit(_q, msg);
    if (r == RS_ON) {
        first = false;
    }
    else {
        hostService.setState(_q, _sUnreachable, msg);
    }
    pSnmpElapsed = getServiceVar(sSnmpElapsed);
    if (pSnmpElapsed == nullptr) {
        pSnmpElapsed = new cServiceVar();
        pSnmpElapsed->setName(sSnmpElapsed);
        pSnmpElapsed->setId(cServiceVar::ixServiceVarTypeId(), cServiceVarType::srvartype(*pq, _sRuntime)->getId());
        pSnmpElapsed->setId(_sHostServiceId, hostServiceId());
        pSnmpElapsed->insert(*pq);
    }
}

int cDevPortStat::queryInit(QSqlQuery &_q, QString& msg)
{
    maxPortIndex = 0;
    if (0 != checkAvailableVars(_q)) {
        msg = trUtf8("queryInit() : checkAvailableVars() : SNMP error : ") + snmp.emsg;
        return RS_UNREACHABLE;
    }

    // Sub services those that no longer belongs to us, you must be disconnected!
    cHostService hsf;   // Flags is set true, all sub service
    hsf.setId(_sSuperiorHostServiceId, hostServiceId());    // Connected
    hsf.setBool(_sDisabled, false);                         // not disabled
    hsf.setBool(hsf.deletedIndex(),  false);                         // not deleted
    QBitArray hsfWhereBits = hsf.mask(_sSuperiorHostServiceId, _sDisabled, _sDeleted);
    hsf.mark(_q, hsfWhereBits);

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
    return RS_ON;
}

cPortStat::cPortStat(cInterface * pIf, cDevPortStat *par)
{
    parent     = par;
    pInterface = pIf;
    pRlinkStat = nullptr;
    pPortVars  = new cInspector(parent, parent->host().dup()->reconvert<cNode>(), parent->pSrvPortVars, pInterface->dup()->reconvert<cNPort>());
}

bool cPortStat::postInit(QSqlQuery& q)
{
    bool isWanted = true;
    pInterface->_toReadBack = RB_NO;
 // portvars *********************************************
    // host service
    cHostService& vhs = pPortVars->hostService;
    // Record exists ?
    int n = vhs.completion(q);
    if (n  > 1) EXCEPTION(AMBIGUOUS, n, pPortVars->name());
    if (n == 0) {   // Not found, create...
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
        if (parent->hostServiceId() != vhs.getId(_sSuperiorHostServiceId)) {
            // Not ours !!!???
            vhs.setId(_sSuperiorHostServiceId, parent->hostServiceId());
            vhs.setFlag(false);
            vhs._toReadBack = RB_NO_ONCE;
            vhs.update(q, false, vhs.mask(_sSuperiorHostServiceId, _sFlag));
        }
        else {  // OK
            vhs.mark(q, vhs.primaryKey(), false); // clear flag
        }
        if (vhs.getBool(_sDisabled)) {  // port_vars service is disabled ?
            pDelete(pPortVars);
        }
    }
    DM;
    if (pPortVars != nullptr) {
        // service variables
        pPortVars->pVars = pPortVars->fetchVars(q);  // Fetch variables
        DM;
        if (pPortVars->pVars == nullptr) {
            pPortVars->pVars = new tOwnRecords<cServiceVar, cHostService>(&(pPortVars->hostService));
        }
        else {
            pPortVars->pVars->sets(_sFlag, QVariant(true));    // marked (only memory)
        }
        DM;
        bool resetRarefaction = cSysParam::getBoolSysParam(q, "reset_rarefaction", false);
        n = parent->vnames.size();
        if (n != parent->vTypes.size() || n != parent->vRarefactions.size()) EXCEPTION(EPROGFAIL);
        for (int i = 0; i < n; ++i) {   // Names of required variables
            const QString& name = parent->vnames.at(i);
            cServiceVar *pVar = pPortVars->pVars->get(name, EX_IGNORE);
            if (pVar == nullptr) {      // If not exists, create
                pVar = new cServiceVar;
                pVar->setName(name);
                pVar->setId(cServiceVar::ixServiceVarTypeId(), parent->vTypes.at(i));
                pVar->setId(_sHostServiceId, pPortVars->hostServiceId());
                pVar->setId(cServiceVar::ixRarefaction(), parent->vRarefactions.at(i));
                pVar->insert(q);
                *(pPortVars->pVars) << pVar;
                pVar->_toReadBack = RB_NO;
            }
            else {
                pVar->_toReadBack = RB_NO;
                if (pVar->getBool(_sDisabled)) {    // If disabled then dropp for list
                    if (pVar->getId(cServiceVar::ixVarState()) != RS_UNKNOWN) {
                        pVar->setName(cServiceVar::ixVarState(), _sUnknown);
                        pVar->_toReadBack = RB_NO;
                        pVar->update(q, false, pVar->mask(cServiceVar::ixVarState()));
                    }
                    int ix = pPortVars->pVars->indexOf(pVar->getId());
                    if (ix < 0) EXCEPTION(EPROGFAIL);
                    pPortVars->pVars->removeAt(ix);
                    delete pVar;
                    continue;
                }
                else if (resetRarefaction) {
                    if (pVar->getId(cServiceVar::ixRarefaction()) != parent->vRarefactions.at(i)) {
                        pVar->setId(cServiceVar::ixRarefaction(), parent->vRarefactions.at(i));
                        pVar->update(q, false, pVar->mask(cServiceVar::ixRarefaction()), pVar->primaryKey());
                    }
                }
            }
            pVar->setOff(_sFlag);   // unmarked
            pVar->initSkeepCnt(parent->delayCounter);
        }
        n = pPortVars->pVars->size();
        for (int i = 0; i < n; ++i) {   // Find, and delete marked
            cServiceVar *pVar = pPortVars->pVars->at(i);
            if (pVar->getBool(_sFlag)) {
                pVar->remove(q);                    // delete from database
                delete pPortVars->pVars->takeAt(i); // delete from list, and delete object from memory
                --i;
            }
        }
        if (pPortVars->pVars->isEmpty()) {
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
    DM;
    if (!cIfType::ifType(pInterface->getId(cDevPortStat::ixIfTypeId)).isLinkage()) return isWanted;
    QString wMsg;   // Message if something is suspicious
    // By the time it is connected (log_links, lldp_links -> rlinkstat)
    qlonglong lpid; // Linked port ID
    eLinkResult lr = getLinkedPort(q, pInterface->getId(), lpid, wMsg, true);
    if (lr == LINK_CONFLICT || lr == LINK_RCONFLICT) {
        APPMEMO(q, wMsg, RS_CRITICAL);
    }
    if (lpid == NULL_ID) return isWanted;  // No likely link
    DM;
    // Linked object
    cNPort *p = cNPort::getPortObjById(q, lpid);      // port ID --> node
    pRlinkStat = new cInspector(parent, cNode::getNodeObjById(q, p->getId(_sNodeId))->reconvert<cNode>(), parent->pRLinkStat, p);
    cHostService& lhs = pRlinkStat->hostService;
    DM;
    // host_services record ?
    n = lhs.completion(q);
    // Only one record may be
    if (n  > 1) EXCEPTION(EDATA, n, parent->hostService.identifying(false));
    DM;
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
                wMsg += QObject::trUtf8("Set superior service : NULL -> %1")
                        .arg(parent->name());
                msg = wMsg;
                rs = RS_RECOVERED;  // Superior service is recovered
            }
            else {
                wMsg += QObject::trUtf8("Change superior service : %1 -> %2")
                        .arg(cHostService::fullName(q, shsid, EX_IGNORE))
                        .arg(parent->name());
                msg = wMsg + QObject::trUtf8("\nLast service (%1) state : ").arg(lhs.fullName(q, EX_IGNORE))
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
        lhs.setBool(_sFlag, false);
        QString note =
                QObject::trUtf8("Automatically generated by portstat (%1) : \n")
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
        EXCEPTION(ESNMP,-1, QString(QObject::trUtf8("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    cTable      tab;    // Interface table container
    QElapsedTimer timer;
    timer.start();
    int r = snmp.getTable(snmpTabOIds, snmpTabColumns, tab, oid(maxPortIndex));
    int state;
    pSnmpElapsed->setValue(q, QVariant(timer.elapsed()), state);
    if (r) {
        runMsg += trUtf8("SNMP get table error : %1 in %2; host : %3, from %4 parent service.\n")
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
            runMsg += trUtf8("ifIndex (%1) is not numeric : %2, in %3; host : %4, from %5 parent service.\n")
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
        cInterface&     iface = * dynamic_cast<cInterface *>(host().ports[ix]);
        // Find cPortStat object
        QMap<qlonglong, cPortStat *>::iterator ifi = ifMap.find(iface.getId());
        if (ifi == ifMap.end())
            continue;   // Not found
        cPortStat& portstat = **ifi;
        QBitArray bitsSet(iface.cols(), false);     // bits: Field is changed
        iface.set(ixLastTouched, QDateTime::currentDateTime());  // utolsó status állítás ideje, most.
        bitsSet.setBit(ixLastTouched);
        changeStringField(ifDescr, ixIfdescr, iface, bitsSet);

        QString eMsg;

        int    _opstat = tab[_sIfOperStatus][i].toInt(&ok);
        QString opstat = snmpIfStatus(_opstat, EX_IGNORE);
        if (!ok || opstat.isEmpty()) {
            eMsg = QObject::trUtf8("ifOperStatus is not numeric, or invalid : %1").arg(tab[_sIfOperStatus][i].toString());
            opstat = _sUnknown;
        }
        changeStringField(opstat,  ixPortOStat, iface, bitsSet);

        int    _adstat = tab[_sIfAdminStatus][i].toInt(&ok);
        QString adstat = snmpIfStatus(_adstat, EX_IGNORE);
        if (!ok) {
            if (!eMsg.isEmpty()) eMsg += "\n";
            eMsg += QObject::trUtf8("ifAdminStatus is not numeric, or invalid : %1").arg(tab[_sIfAdminStatus][i].toString());
            adstat = _sUnknown;
        }
        changeStringField(adstat,  ixPortAStat, iface, bitsSet);

        // rlinkstat if exists
        if (portstat.pRlinkStat != nullptr) {
            cInspector& insp = *portstat.pRlinkStat;
            QString state = _sUnKnown;
            if      (_opstat == IF_UP)   state = _sOn;        // On
            else if (_adstat == IF_UP)   state = _sDown;      // Off
            else if (_adstat == IF_DOWN) state = _sWarning;   // Disabled
            QString msg = trUtf8("interface state : op:%1/adm:%2").arg(opstat).arg(adstat);
            if (!eMsg.isEmpty()) msg += "\n" + eMsg;
            insp.hostService.setState(q, state, msg, parid);
            insp.flag = true; // State is set
        }

        // Ha van kapcsolódó portvars al szolgáltatás:
        if (portstat.pPortVars != nullptr) {
            cInspector& insp = *portstat.pPortVars;
            int sstate = RS_ON;         // delegated state to service state
            int pstate = RS_INVALID;    // delegated state to port state
            int rs;
            qlonglong raw;
            QString stMsg;
            foreach (QString vname, vnames) {   // foreac variable names
                cServiceVar *psv = insp.getServiceVar(vname);   // Get variable obj.
                if (psv == nullptr) continue;                   // not found, next
                raw = tab[vname][i].toLongLong();               // Get raw value from SNMP query
                rs = psv->setValue(q, raw, sstate, TS_NULL);
                if (psv->getBool(ixDelegatePortState) && rs > pstate) pstate = rs;
            }
            insp.hostService.setState(q, notifSwitch(sstate), stMsg, parid);
            insp.flag = true; // State is set
            if (pstate != RS_INVALID && changeStringField(notifSwitch(pstate), ixPortStat, iface, bitsSet)) {
                iface.set(ixLastChanged, QDateTime::currentDateTime());  // utolsó status állítás ideje, most.
                bitsSet.setBit(ixLastChanged);
            }
        }
        // Write interface states
        int un = iface.update(q, false, bitsSet);
        if (un != 1) {
            QString msg = trUtf8("Az update visszatérési érték %1 hiba, Service : %2, interface : %3")
                    .arg(un)
                    .arg(iface.identifying(false))
                    .arg(hostService.fullName(q, EX_IGNORE));
            APPMEMO(q, msg, RS_CRITICAL);
        }
    }
    foreach (cInspector *ps, *pSubordinates) {
        if (ps != nullptr && ps->flag == false) {   // be lett állítva státusz ??? Ha még nem:
            ps->hostService.setState(q, _sUnknown, trUtf8("Not set by query") , parid);
        }
    }
    DBGFNL();
    return rs;
}

