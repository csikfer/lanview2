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
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2portVLan::lv2portVLan() : lanView()
{
    pSelfInspector = nullptr;  // Main service == APPNAME, standard init
    if (lastError == nullptr) {
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
    cInterface i;
    cDevicePV::ixPortIndex    = i.ixPortIndex();
    cDevicePV::ixPortStapleId = i.toIndex(_sPortStapleId);
    cDevicePV::multiplexorTypeId = cIfType::ifType(_sMultiplexor).getId();
}

void lv2portVLan::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::tSetup<cPortVLans>(_tr);
}


/******************************************************************************/

const QString cPortVLans::sUnauthPeriod = "UnauthPeriod";
const QString cPortVLans::sClientLimit  = "ClientLimit";
const QString cPortVLans::sLogoffPeriod = "LogoffPeriod";
const QString cPortVLans::sClientLimit2 = "ClientLimit2";

static inline void setOid(cOId& o, const QString& s)
{
    o = cOId(s);
    if (!o) EXCEPTION(ESNMP, o.status, "OID : \"" + s + "\"; " + o.emsg);
}

/// Az SNMP-n keresztül lekérdezi a VLAN beállításokat OID objektumok:
/// @code
/// dot1qVlan                           1.3.6.1.2.1.17.7.1.4
///
///   dot1qVlanCurrentTable             1.3.6.1.2.1.17.7.1.4.2
///     dot1qVlanCurrentEgressPorts     1.3.6.1.2.1.17.7.1.4.2.1.4  bitmap : assigned (tagged or untagged)
///     dot1qVlanCurrentUntaggedPorts   1.3.6.1.2.1.17.7.1.4.2.1.5  bitmap : untagged
///
///   dot1qVlanStaticTable              1.3.6.1.2.1.17.7.1.4.3
///     dot1qVlanStaticEgressPorts      1.3.6.1.2.1.17.7.1.4.3.1.2  bitmap : assigned
///     dot1qVlanForbiddenEgressPorts   1.3.6.1.2.1.17.7.1.4.3.1.3  bitmap : forbidden
///     dot1qVlanStaticUntaggedPorts    1.3.6.1.2.1.17.7.1.4.3.1.4  bitmap : untagged
///
///   dot1qPortVlanTable                1.3.6.1.2.1.17.7.1.4.5
///     dot1qPvid                       1.3.6.1.2.1.17.7.1.4.5.1.1  port PVID
///
/// dot1qVlanCurrentEgressPorts.0.<vlan id> = Port map (hex string)
/// example:
/// get mib-2.17.7.1.4.2.1.4.0.101
/// SNMPv2-SMI::mib-2.17.7.1.4.2.1.4.0.101 = Hex-STRING: 00 00 C0 00 00 00 40 00
///   vlan id = 101
///            0           1            2             3            4              5            6
///   port ix: 1234 5678  9012 3456  7890 1234  5678 9012  3456 7890  1234 5678  9012 3456  7890 1234
///   bitmap:  0000 0000  0000 0000  1100 0000  0000 0000  0000 0000  0000 0000  0100 0000  0000 0000
///      hex:     0    0     0    0     C    0     0    0     0    0     0    0     4    0     0    0
///
///   id=101 vlan is current eggres to 17,18 and 50 port.
/// @endcode
///
/// 802.1x (MIB file : hpicfOid.mib)
///
/// hpicfDot1xAuthenticator                 HP-ICF-OID::hpicfDot1xMIB.1.2
///   hpicfDot1xAuthConfigTable             HP-ICF-OID::hpicfDot1xMIB.1.2.1
///     hpicfDot1xAuthAuthVid               HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.1    VLAN ID authorized
///     hpicfDot1xAuthUnauthVid             HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.2    VLAN ID unauthorized
///     hpicfDot1xAuthUnauthPeriod          HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.3
///     hpicfDot1xAuthClientLimit           HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.4
///     hpicfDot1xAuthLogoffPeriod          HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.5
///     hpicfDot1xAuthClientLimit2          HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.6
///
/// Feaures:
///
/// - bitmap-xrefs
///   Kereszt index a VLAN bitmap-ekben, ha a bit index nem azonos a port indexével
///   pl.: HPE 1920 A NULL0 és a Vlan-interface<vlanid> interfészek kimaradnak a bitmap-ból.
///   Ezért a trunk portok indexei csúsznak minimum 2-vel.
/// - static_vlans
///   A cDevicePV::run() metódus az első hívásakor ( ha staticOnly == TS_NULL) teszteli,
///   hogy a dinamikus VLAN kiosztás (dot1qVlanCurrent... OID objektumok) lekérdezhetőek-e.
///   Ha nem lekérdezhetőek akkor a továbbiakban ezt nem kérdezi le (staticOnly = TS_TRUE).
///   Ha megadtuk a static_vlans-t és értéke true, akkor a teszt elmarad, és csak a statikus állapotot kérdezi le,
///   ha viszont static_vlans értéke false, akkor a teszt szintén elmerad, és a dinamikusakat is lekérdezi.
///   Más értékeket esetén a static_vlans -t figyelmen kívül hagyja.
///   Lásd még a eTristate str2tristate(const QString& _b, enum eEx __ex) függvényt.
///   ...
cPortVLans::cPortVLans(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{

}

cPortVLans::~cPortVLans()
{
    ;
}

void cPortVLans::postInit(QSqlQuery &q, const QString &qs)
{
    cInspector::postInit(q, qs);

    // Current VLAN settings (dynamic)
    setOid(dot1qVlanCurrentEgressPorts,             "SNMPv2-SMI::mib-2.17.7.1.4.2.1.4.0");
    setOid(dot1qVlanCurrentUntaggedPorts,           "SNMPv2-SMI::mib-2.17.7.1.4.2.1.5.0");

    setOid(dot1qVlanStaticEgressPorts,              "SNMPv2-SMI::mib-2.17.7.1.4.3.1.2");
    setOid(dot1qVlanStaticForbiddenEgressPorts,     "SNMPv2-SMI::mib-2.17.7.1.4.3.1.3");
    setOid(dot1qVlanStaticUntaggedPorts,            "SNMPv2-SMI::mib-2.17.7.1.4.3.1.4");

    setOid(dot1qPvid,                               "SNMPv2-SMI::mib-2.17.7.1.4.5.1.1");

    setOid(hpicfDot1xAuthenticator,                 "HP-ICF-OID::hpicfDot1xMIB.1.2");
    setOid(hpicfDot1xAuthAuthVid,                   "HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.1");
    setOid(hpicfDot1xAuthUnauthVid,                 "HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.2");
    setOid(hpicfDot1xAuthUnauthPeriod,              "HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.3");
    setOid(hpicfDot1xAuthClientLimit,               "HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.4");
    setOid(hpicfDot1xAuthLogoffPeriod,              "HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.5");
    setOid(hpicfDot1xAuthClientLimit2,              "HP-ICF-OID::hpicfDot1xMIB.1.2.1.1.6");
    hpicfDot1xAuthConfigTable << hpicfDot1xAuthAuthVid << hpicfDot1xAuthUnauthVid << hpicfDot1xAuthUnauthPeriod
                              << hpicfDot1xAuthClientLimit << hpicfDot1xAuthLogoffPeriod << hpicfDot1xAuthClientLimit2;
    headerAuthConfigTable << _sAuth << _sUnauth << sUnauthPeriod << sClientLimit << sLogoffPeriod << sClientLimit2;

}

cInspector * cPortVLans::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePV(q, hsid, hoid, pid);
}

/******************************************************************************/

const cService *cDevicePV::pSrvSnmp   = nullptr;
int cDevicePV::ixPortIndex = NULL_IX;
qlonglong cDevicePV::multiplexorTypeId;
int cDevicePV::ixPortStapleId;

cDevicePV::cDevicePV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    // Ha nincs megadva protocol szervíz ('nil'), akkor SNMP device esetén az az SNMP lesz
    if (protoServiceId() == NIL_SERVICE_ID && __tableoid == cSnmpDevice().tableoid()) {
        hostService.set(_sProtoServiceId, pSrvSnmp->getId());
        QSqlQuery q2 = getQuery();
        hostService.update(q2, false, hostService.mask(_sProtoServiceId));
        pProtoService = hostService.getProtoService(q2);
    }
    if (protoServiceId() == pSrvSnmp->getId()) {
        snmpDev().open(__q, snmp);
    }
    else {  // Csak az SNMP lekérdezés támogatott (egyenlőre)
        EXCEPTION(EDATA, protoServiceId(), QObject::tr("Nem támogatott proto_service_id!"));
    }
}

cDevicePV::~cDevicePV()
{
    ;
}

void cDevicePV::postInit(QSqlQuery& q, const QString& qs)
{
    cInspector::postInit(q, qs);
    node().fetchPorts(q);

    /* Features */
    QString key, key2;
    /* ******************************************* */
    key = "static_vlans";
    /* ******************************************* */
    staticOnly = str2tristate(features().value(key), EX_IGNORE);
    /* ******************************************* */
    key = "bitmap_xrefs";
    /* ******************************************* */
    QString xref = features().value(key);
    if (!xref.isEmpty()) {
        tStringMap smap = cFeatures::value2map(xref);
        QString m;
        if (smap.isEmpty()) {
            m = tr("Az '%1' feature változó értéke ('%2') nem értelmezhető, mint kereszt index tábla.").arg(key, xref);
        }
        else {
            foreach (QString spix, smap.keys()) {
                QString sbix = smap[spix];
                bool pok, bok;
                qlonglong portIndex = spix.toInt(&pok);
                int       bitIndex  = sbix.toInt(&bok);
                if (!pok || !bok) {
                    msgAppend(&m, tr("Az '%1' feature változó ban definiált index pár nem numerikus : %2 - %3 .").arg(key, spix, sbix));
                }
                else {
                    cNPort *p = node().ports.get(ixPortIndex, portIndex, EX_IGNORE);
                    if (p == nullptr) {
                        msgAppend(&m, tr("Az '%1' feature változó ban definiált indexű port nem létezik : %2.").arg(key, spix));
                        continue;
                    }
                    mIndexXref[portIndex] = bitIndex;
                    if (!mIndexXref.contains(bitIndex)) mIndexXref[bitIndex] = -1;  // Kettős hivatkozás lenne!
                }
            }
        }
        if (!m.isEmpty()) {
            msgAppend(&m, tr("Szolgáltatás példány : %1 .").arg(name()));
            APPMEMO(q, m, RS_WARNING);
        }
    }
    qlonglong typeId = cParamType::paramType(_sBoolean).getId();
    cPortParam pp;
    int ixParamValue = pp.toIndex(_sParamValue);
    /* ******************************************* */
    key = "no_pvid";
    /* ******************************************* */
    mNoPVID = features().contains(key);
    QBitArray clrMask = ~ pp.mask(pp.ixPortId(), pp.ixParamTypeId());
    if (!mNoPVID) {
        foreach (cNPort *p, node().ports.list()) {
            pp.clear();
            pp.setName(key);
            pp.setType(typeId);
            pp.setId(pp.ixPortId(), p->getId());
            bool f = pp.completion(q) && str2bool(pp.getName(ixParamValue));
            if (f) mNoPvidPorts << int(p->getId(ixPortIndex));
        }
    }
    /* ******************************************* */
    key = "no_untagged_bitmap";
    /* ******************************************* */
    mNoUntaggedBitmap = features().contains(key);
    /* ******************************************* */
    key = "trunk_by_members";
    /* ******************************************* */
    mTrunkByMembers = features().contains(key);
    /* ******************************************* */
    key = "dump";
    /* ******************************************* */
    mDumpBitMaps = features().contains(key);
    /* Port parameters */
    /* ******************************************* */
    key  = "no_vlan";
    key2 = "802.1x";
    /* ******************************************* */
    const cPortVLans& par = dynamic_cast<const cPortVLans&>(parent());
    int r, probe = 0, maxProbe = 4;
    do {
        r = snmp.getNext(par.hpicfDot1xAuthenticator);
    } while (r != 0 && probe++ < maxProbe);
    if (r == 0) {
        snmp.first();
        hpicfDot1xAuthenticator = snmp.name() > par.hpicfDot1xAuthenticator;
        if (hpicfDot1xAuthenticator) {  // Van tábla. Mindegyik oszlop létezik?
            hpicfDot1xAuthConfigTable = par.hpicfDot1xAuthConfigTable;
            headerAuthConfigTable     = par.headerAuthConfigTable;
            probe = 0;
            do {
                r = snmp.getNext(hpicfDot1xAuthConfigTable);
            } while (r != 0 && probe++ < maxProbe);
            if (r == 0) {
                snmp.first();
                int i, n = headerAuthConfigTable.size();
                for (i = 0; i < n; ++i) {
                    const cOId&     oib = hpicfDot1xAuthConfigTable[i]; // Column base OID
                    const cOId      oia = snmp.name();                  // Cell OID
                    if (!(oib < oia)) {
                        // missing column
                        hpicfDot1xAuthConfigTable.remove(i);
                        headerAuthConfigTable.removeAt(i);
                        --i; --n;
                    }
                    snmp.next();
                }
                hpicfDot1xAuthenticator = headerAuthConfigTable.contains(_sAuth) && headerAuthConfigTable.contains(_sUnauth);
            }
            else {
                hpicfDot1xAuthenticator = false;
            }
        }
    }
    foreach (cNPort *p, node().ports.list()) {
        pp.clear();
        pp.setName(key);
        pp.setType(typeId);
        pp.setId(pp.ixPortId(), p->getId());
        bool f = pp.completion(q) && str2bool(pp.getName(ixParamValue));
        if (f) mNoVlanPortsByPar << int(p->getId(ixPortIndex));
        else if (!hpicfDot1xAuthenticator) {    // Ha lekérdezzük a 802.1x, akkor nincs statikus tiltás a 802.1x paraméterrel.
            pp.clear();
            pp.setName(key2);
            pp.setType(typeId);
            pp.setId(pp.ixPortId(), p->getId());
            f = pp.completion(q) && str2bool(pp.getName(ixParamValue));
            if (f) mNoVlanPortsByPar << int(p->getId(ixPortIndex));
        }
    }
}
#define _DM PDEB(VVERBOSE)
#define DM  _DM << endl;

#define MIN_VLAN_ID 1
#define MAX_VLAN_ID 4095

static QString dumpMaps(const QString& head, const QMap<int, QBitArray>& maps)
{
    QString dump = head + "\n";
    QList<int> vlans = maps.keys();
    std::sort(vlans.begin(), vlans.end());
    foreach (int vlan, vlans) {
        QString s = QString::number(vlan).rightJustified(4) + " :";
        const QBitArray& ba = maps[vlan];
        int i, n = ba.size();
        for (i = 0; i < n; ++i) {
            if (i % 8 == 0) s += _sSpace;
            if (i % 4 == 0) s += _sSpace;
            s += QChar(ba.at(i) ? '1' : '0');
        }
        dump += s +"\n";
    }
    dump.chop(1);   // Az utolsó '\n'
    return dump;
}

static QString dumpPVID(const QString& head, const QMap<int, int>& map)
{
    QString dump = head + "\n";
    QList<int> pixs = map.keys();
    std::sort(pixs.begin(), pixs.end());
    dump += "Port : ";
    foreach (int pix, pixs) {
        dump += QString::number(pix).rightJustified(4);
    }
    dump += "\n";
    dump += "VLan : ";
    foreach (int pix, pixs) {
        dump += QString::number(map[pix]).rightJustified(4);
    }
    return dump;
}

static QString dumpTable(const cTable& t)
{
    QString dump;
    foreach (QString colName, t.keys()) {
        dump += colName + " :\t";
        foreach (QVariant v, t[colName]) {
            dump += debVariantToString(v) + _sCommaSp;
        }
        dump.chop(_sCommaSp.size());
        dump += "\n";
    }
    return dump;
}

inline static bool maps2bool(const QMap<int, QBitArray>& maps, int mapix, int bitix) {
    if (bitix <= 0) return false;   // A valós index: 1,2...
    if (!maps.contains(mapix)) return false;
    const QBitArray& ba = maps[mapix];
    if (ba.size() < bitix) return false;
    return ba[bitix -1];
}

int cDevicePV::run(QSqlQuery& q, QString &runMsg)
{
    int rs = RS_ON;
    int r;
    const cPortVLans& par = dynamic_cast<const cPortVLans&>(parent());
    ctVlan = ctUnchg = ctMod = ctNew = ctIns = ctUnkn = ctRm = ctErr = 0;
    currentEgres.clear();
    currentUntagged.clear();
    staticEgres.clear();
    staticUntagged.clear();
    staticForbid.clear();
    pvidMap.clear();
    trunkMembersVlanTypes.clear();
    mNoVlanPorts = mNoVlanPortsByPar;
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::tr("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    // Az eszköz portjaihoz tartozó vlan kapcsoló rekordok: mind jelöletlen
    static const QString sql = "UPDATE port_vlans SET flag = false WHERE port_id IN (SELECT port_id FROM nports WHERE node_id = ?)";
    execSql(q, sql, nodeId());
    if (hpicfDot1xAuthenticator) {
        cTable table;
        r = snmp.getTable(hpicfDot1xAuthConfigTable, headerAuthConfigTable, table);
        if (0 != r) {
            runMsg = tr("SNMP '%1' error (auth) : #%2/%3, '%4'.")
                    .arg(snmp.name().toString())
                    .arg(snmp.status).arg(r)
                    .arg(snmp.emsg);
            rs = RS_UNREACHABLE;
            DBGFNL();
            return rs;
        }
        if (mDumpBitMaps) {
            msgAppend(&runMsg, dumpTable(table));
        }
        int rows = table.rows();
        for (int row = 0; row < rows; ++row) {
            int portIndex = table[cSnmp::sSnmpTableIndex][row].toInt();
            int authVId   = table[_sAuth]  [row].toInt();
            if (authVId <= 0) continue;
            int unauthVId = table[_sUnauth][row].toInt();
            qlonglong pid = host().ports.get(ixPortIndex, portIndex)->getId();
            updatePortVlan(q, pid, authVId,   _sAuth);
            updatePortVlan(q, pid, unauthVId, _sUnauth);
            mNoVlanPorts << portIndex;
            // ...
        }
    }
    if (mDumpBitMaps) {
        QString s = "mNoVlanPorts : ";
        foreach (int ix, mNoVlanPorts) s += QString::number(ix) + _sCommaSp;
        s += "\n";
        msgAppend(&runMsg, s);
    }
    switch (staticOnly) {
    case TS_NULL:   // Test
        r = snmp.getBitMaps(par.dot1qVlanCurrentEgressPorts,   currentEgres);
        if (r == 0 && !mNoUntaggedBitmap) r = snmp.getBitMaps(par.dot1qVlanCurrentUntaggedPorts, currentUntagged);
        if (0 != r) {
            runMsg = tr("SNMP '%1' error (test) : #%2/%3, '%4'.")
                    .arg(snmp.name().toString())
                    .arg(snmp.status).arg(r)
                    .arg(snmp.emsg);
            rs = RS_UNREACHABLE;
        }
        else {
            bool ze = currentEgres.isEmpty();
            bool zu = currentUntagged.isEmpty();
            if (!mNoUntaggedBitmap && ze != zu) {
                static const QString nz = tr("elérhető"), z = tr("nem elérhető");
                runMsg = tr("Teszt : Nem értelmezhető SNMP válasz : dot1qVlanCurrentEgressPorts %1; dot1qVlanCurrentUntaggedPorts %2.")
                        .arg(ze ? z : nz, zu ? z :nz);
                rs = RS_UNREACHABLE;
            }
            else if (ze) {
                staticOnly = TS_TRUE;
                rs = runSnmpStatic(q, runMsg, par);
            }
            else {
                staticOnly = TS_FALSE;
                rs = runSnmpDynamic(q, runMsg, par);
            }
        }
        break;
    case TS_TRUE:   // Static only
        rs = runSnmpStatic(q, runMsg, par);
        break;
    case TS_FALSE:  // Dinamic
        rs = runSnmpDynamic(q, runMsg, par);
        break;
    }
    if (rs < RS_UNREACHABLE) {
        // törlendő jelöletlen rekordok
        bool ok;
        ctRm = execSqlIntFunction(q, &ok, "rm_unmarked_port_vlan", node().getId());
        msgAppend(&runMsg,
                  tr("%1 VLAN (dynamic), %2 változatlan, %3 új (%4 új VLAN), %5 változás, %6 törölva, %7 ismeretlen; %8 hiba.")
                    .arg(ctVlan).arg(ctUnchg).arg(ctIns + ctNew).arg(ctNew).arg(ctMod)
                    .arg(ok ? QString::number(ctRm) : "?")
                    .arg(ctUnkn).arg(ctErr));
    }
    DBGFNL();
    return rs;
}

void cDevicePV::trunkMap(const cNPort * p, qlonglong trunkId, int vlanId, const QString& vlanType)
{
        int memberIndex = int(p->getId(ixPortIndex));
        trunkMembersVlanTypes[trunkId][memberIndex][vlanId] = vlanType;
}

static QString dumpVlanTypes(QMap<int, QString> vlanTypes)
{
    QString r;
    QList<int> vids = vlanTypes.keys();
    std::sort(vids.begin(), vids.end());
    foreach (int vid, vids) {
        r += QString("%1(%2), ").arg(vid).arg(vlanTypes[vid]);
    }
    r.chop(2);
    return r;
}

int cDevicePV::setTrunks(QSqlQuery& q, QString& runMsg)
{
    int rs = RS_ON;
    foreach (qlonglong trunkId, trunkMembersVlanTypes.keys()) {
        QMap<int, QMap<int, QString> >& membersMap = trunkMembersVlanTypes[trunkId];
        QList<int>  memberIndexs = membersMap.keys();
        int firstMemberIndex = memberIndexs.takeFirst();
        QMap<int, QString> vlanTypesMap = membersMap[firstMemberIndex];
        foreach (int memberIndex, memberIndexs) {
            if (vlanTypesMap != membersMap[memberIndex]) {
                msgAppend(&runMsg,
                          tr("A %1 trunk port %2 és %3 tag portjának a VLAN kiosztása nem azonos %4 <> %5")
                            .arg(node().ports.get(trunkId)->getName())
                            .arg(node().ports.get(ixPortIndex, firstMemberIndex)->getName())
                            .arg(node().ports.get(ixPortIndex, memberIndex)->getName())
                            .arg(dumpVlanTypes(vlanTypesMap))
                            .arg(dumpVlanTypes(membersMap[memberIndex])));
                rs = RS_WARNING;
            }
        }
        foreach (int vlanId, vlanTypesMap.keys()) {
            const QString& vtype = vlanTypesMap[vlanId];
            updatePortVlan(q, trunkId, vlanId, vtype);
        }
    }
    return rs;
}

void cDevicePV::updatePortVlan(QSqlQuery& q, qlonglong portId, int vlanId, const QString& vlanType)
{
    QString r = execSqlTextFunction(q, "update_port_vlan", portId, vlanId, vlanType);
    switch (reasons(r, EX_IGNORE)) {
    case R_UNCHANGE:    ++ctUnchg;  break;  // Nem volt változás
    case R_NEW:         ++ctNew;    break;  // Új rekord
    case R_INSERT:      ++ctIns;    break;  // Új rekord, és uj VLAN rekord is fel lett véve
    case R_MODIFY:      ++ctMod;    break;  // Modosítva
    default:            ++ctUnkn;   break;  // ??
    }
}

int cDevicePV::runSnmpStatic(QSqlQuery& q, QString &runMsg, const cPortVLans& par)
{
    // SNMP query
    int r = 0;
    if (r != 0
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticEgressPorts,          staticEgres))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticForbiddenEgressPorts, staticForbid))
     || (mNoUntaggedBitmap == false
      && 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticUntaggedPorts,        staticUntagged)))
     || (mNoPVID == false
      && 0 != (r = snmp.getXIndex( par.dot1qPvid, pvidMap)))) {
        runMsg = tr("SNMP '%1' error (query, dynamic): #%2/%3, '%4'.")
                .arg(snmp.name().toString())
                .arg(snmp.status).arg(r)
                .arg(snmp.emsg);
        return RS_UNREACHABLE;
    }
    if (mDumpBitMaps) {
        msgAppend(&runMsg, dumpMaps("dot1qVlanStaticEgressPorts",           staticEgres));
        msgAppend(&runMsg, dumpMaps("dot1qVlanStaticForbiddenEgressPorts",  staticForbid));
        if (!mNoUntaggedBitmap) msgAppend(&runMsg, dumpMaps("dot1qVlanStaticUntaggedPorts", staticUntagged));
        if (!mNoPVID)           msgAppend(&runMsg, dumpPVID("dot1qPvid",                    pvidMap));
    }
    //
    QMap<int, int>  untaggedSettedMap;
    int rs = RS_ON;
    for (int vid = MIN_VLAN_ID; vid < MAX_VLAN_ID; ++vid) {
        if (staticEgres.contains(vid) || staticForbid.contains(vid)) {    // Van ilyen VLAN ?
            ++ctVlan;   // Számláló. Talált VLAN-ok száma
            tRecordList<cNPort>::iterator i, n = node().ports.end();
            QString vstat;
            for (i = node().ports.begin(); i != n; ++i) {
                int pix = (*i)->getId(ixPortIndex);     // port index
                if (mNoVlanPorts.contains(pix)) continue;
                qlonglong trunkId = (*i)->getId(ixPortStapleId);
                if (mTrunkByMembers) {
                    if ((*i)->getId((*i)->ixIfTypeId()) == multiplexorTypeId) {
                        if (trunkId != NULL_ID) {
                            QString m = tr("Trunk-ölt trunk port ? %1 < %2")
                                    .arg((*i)->getName(), (*i)->getNameById(q, trunkId));
                            APPMEMO(q, m, RS_CRITICAL);
                        }
                        continue;                       // Skeep trunk port
                    }
                }
                else {
                    if (trunkId != NULL_ID) continue;   // Skeep Trunk member ?
                }
                bool noPvid = mNoPVID || mNoPvidPorts.contains(pix);
                if (!noPvid && !pvidMap.contains(pix)) continue;   // Ha nincs PVID, akkor ez nem VLAN-t támogató port
                int bix = getBitIndex(pix);             // index a bitmap-ban / elvileg azonos, gyakorlatilag meg nem mindíg
                bool isUntagged = maps2bool(staticUntagged, vid, bix);
                bool isPVID     = pvid(pix) == vid;
                if (mNoUntaggedBitmap) {
                    isUntagged = isPVID;
                }
                else if (!noPvid) {
                    if (isPVID != isUntagged) {
                        msgAppend(&runMsg,
                                  tr("A PVID és az Untagged port ellentmondása  (static)! Port : %1; PVID = %2; %3 untagged : %4")
                                    .arg((*i)->getName()).arg(pvid(pix)).arg(vid).arg(langBool(isUntagged)));
                        rs = RS_WARNING;
                        ctErr++;
                    }
                }
                if (isUntagged) {
                    if (untaggedSettedMap.contains(pix)) {
                        msgAppend(&runMsg,
                                  tr("A %1 porthoz több untagged VLAN van kiosztva : %2 és %3 is")
                                    .arg((*i)->getName()).arg(vid).arg(untaggedSettedMap[pix]));
                        rs = RS_WARNING;
                        ctErr++;
                    }
                    else {
                        untaggedSettedMap[pix] = vid;
                    }
                    vstat = _sUntagged;
                }
                else if (maps2bool(staticForbid,  vid, bix)) vstat = _sForbidden;
                else if (maps2bool(staticEgres,   vid, bix)) vstat = _sTagged;
                else    continue;
                QString r = execSqlTextFunction(q, "update_port_vlan", (*i)->getId(), vid, vstat);
                switch (reasons(r, EX_IGNORE)) {
                case R_UNCHANGE:    ++ctUnchg;  break;  // Nem volt változás
                case R_NEW:         ++ctNew;    break;  // Új rekord
                case R_INSERT:      ++ctIns;    break;  // Új rekord, és uj VLAN rekord is fel lett véve
                case R_MODIFY:      ++ctMod;    break;  // Modosítva
                default:            ++ctUnkn;   break;  // ??
                }
                if (trunkId != NULL_ID) trunkMap(*i, trunkId, vid, vstat);
            }
        }
    }
    int _rs = setTrunks(q, runMsg);
    rs = std::max(rs, _rs);
    return rs;
}

int cDevicePV::runSnmpDynamic(QSqlQuery& q, QString &runMsg, const cPortVLans& par)
{
    // SNMP query
    int r = 0;
    if (currentEgres.isEmpty() || currentUntagged.isEmpty()) {
        r = snmp.getBitMaps(par.dot1qVlanCurrentEgressPorts,   currentEgres);
        if (!mNoUntaggedBitmap && r == 0) r = snmp.getBitMaps(par.dot1qVlanCurrentUntaggedPorts, currentUntagged);
    }
    if (r == 0) r = snmp.getBitMaps(par.dot1qVlanStaticForbiddenEgressPorts, staticForbid);
    if (!mNoPVID && r == 0) r = snmp.getXIndex( par.dot1qPvid, pvidMap);
    if (r != 0) {
        runMsg = tr("SNMP '%1' error (query, dynamic): #%2/%3, '%4'.")
                .arg(snmp.name().toString())
                .arg(snmp.status).arg(r)
                .arg(snmp.emsg);
        return RS_UNREACHABLE;
    }
    if (mDumpBitMaps) {
        msgAppend(&runMsg, dumpMaps("dot1qVlanCurrentEgressPorts",          currentEgres));
        msgAppend(&runMsg, dumpMaps("dot1qVlanStaticForbiddenEgressPorts",  staticForbid));
        if (!mNoUntaggedBitmap) msgAppend(&runMsg, dumpMaps("dot1qVlanCurrentUntaggedPorts", currentUntagged));
        if (!mNoPVID)           msgAppend(&runMsg, dumpPVID("dot1qPvid",                     pvidMap));
    }
    //
    QMap<int, int>  untaggedSettedMap;
    int rs = RS_ON;
    for (int vid = MIN_VLAN_ID; vid < MAX_VLAN_ID; ++vid) {
        if (currentEgres.contains(vid) || currentUntagged.contains(vid) || staticForbid.contains(vid)) {    // Van ilyen VLAN ?
            ++ctVlan;   // Számláló. Talált VLAN-ok száma
            tRecordList<cNPort>::iterator i, n = node().ports.end();
            QString vstat;
            for (i = node().ports.begin(); i != n; ++i) {
                int pix = (*i)->getId(ixPortIndex);     // port index
                if (mNoVlanPorts.contains(pix)) continue;
                qlonglong trunkId = (*i)->getId(ixPortStapleId);
                if (mTrunkByMembers) {
                    if ((*i)->getId((*i)->ixIfTypeId()) == multiplexorTypeId) {
                        if (trunkId != NULL_ID) {
                            QString m = tr("Trunk-ölt trunk port ? %1 < %2")
                                    .arg((*i)->getName(), (*i)->getNameById(q, trunkId));
                            APPMEMO(q, m, RS_CRITICAL);
                        }
                        continue;                       // Skeep trunk port
                    }
                }
                else {
                    if (trunkId != NULL_ID) continue;   // Skeep Trunk member ?
                }
                bool noPvid = mNoPVID || mNoPvidPorts.contains(pix);    // Ignore PVID ?
                if (!noPvid && !pvidMap.contains(pix)) continue;        // Ha nincs PVID, akkor ez nem VLAN-t támogató port
                int bix = getBitIndex(pix);  // index a bitmap-ban / elvileg azonos, gyakorlatilag meg nem mindíg (bitmap_xrefs)
                bool isUntagged = maps2bool(currentUntagged, vid, bix);;
                bool isPVID     = pvid(pix) == vid;
                if (mNoUntaggedBitmap) {
                    isUntagged = isPVID;
                }
                else if (!noPvid) {
                    if (isPVID != isUntagged) {
                        msgAppend(&runMsg,
                                  tr("A PVID és az Untagged port ellentmondása (dynamic)! Port : %1; PVID = %2; %3 untagged : %4")
                                    .arg((*i)->getName()).arg(pvid(pix)).arg(vid).arg(langBool(isUntagged)));
                        rs = RS_WARNING;
                        ctErr++;
                    }
                }
                if (isUntagged) {
                    if (untaggedSettedMap.contains(pix)) {
                        msgAppend(&runMsg,
                                  tr("A %1 porthoz több untagged VLAN van kiosztva : %2 és %3 is")
                                    .arg((*i)->getName()).arg(vid).arg(untaggedSettedMap[pix]));
                        rs = RS_WARNING;
                        ctErr++;
                    }
                    else {
                        untaggedSettedMap[pix] = vid;
                    }
                    vstat = _sUntagged;
                }
                else if (maps2bool(staticForbid,  vid, bix)) vstat = _sForbidden;
                else if (maps2bool(currentEgres, vid, bix))  vstat = _sAutoTagged;
                else                                         vstat = _sAuto;
                QString r = execSqlTextFunction(q, "update_port_vlan", (*i)->getId(), vid, vstat);
                switch (reasons(r, EX_IGNORE)) {
                case R_UNCHANGE:    ++ctUnchg;  break;  // Nem volt változás
                case R_NEW:         ++ctNew;    break;  // Új rekord
                case R_INSERT:      ++ctIns;    break;  // Új rekord, és uj VLAN rekord is fel lett véve
                case R_MODIFY:      ++ctMod;    break;  // Modosítva
                default:            ++ctUnkn;   break;  // ??
                }
                if (trunkId != NULL_ID) trunkMap(*i, trunkId, vid, vstat);
            }
        }
    }
    int _rs = setTrunks(q, runMsg);
    rs = std::max(rs, _rs);
    return rs;
}

