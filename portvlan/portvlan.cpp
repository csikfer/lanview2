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
}

void lv2portVLan::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::tSetup<cPortVLans>(_tr);
}


/******************************************************************************/

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
cPortVLans::cPortVLans(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    // Current VLAN settings (dynamic)
    dot1qVlanCurrentEgressPorts             = cOId("SNMPv2-SMI::mib-2.17.7.1.4.2.1.4.0");
    if (!dot1qVlanCurrentEgressPorts)       EXCEPTION(ESNMP, dot1qVlanCurrentEgressPorts.status, dot1qVlanCurrentEgressPorts.emsg);
    dot1qVlanCurrentUntaggedPorts           = cOId("SNMPv2-SMI::mib-2.17.7.1.4.2.1.5.0");
    if (!dot1qVlanCurrentUntaggedPorts)     EXCEPTION(ESNMP, dot1qVlanCurrentUntaggedPorts.status, dot1qVlanCurrentUntaggedPorts.emsg);

    dot1qVlanStaticEgressPorts              = cOId("SNMPv2-SMI::mib-2.17.7.1.4.3.1.2");
    if (!dot1qVlanStaticEgressPorts)        EXCEPTION(ESNMP, dot1qVlanStaticEgressPorts.status, dot1qVlanStaticEgressPorts.emsg);
    dot1qVlanStaticForbiddenEgressPorts     = cOId("SNMPv2-SMI::mib-2.17.7.1.4.3.1.3");
    if (!dot1qVlanStaticForbiddenEgressPorts) EXCEPTION(ESNMP, dot1qVlanStaticForbiddenEgressPorts.status, dot1qVlanStaticForbiddenEgressPorts.emsg);
    dot1qVlanStaticUntaggedPorts            = cOId("SNMPv2-SMI::mib-2.17.7.1.4.3.1.4");
    if (!dot1qVlanStaticUntaggedPorts)      EXCEPTION(ESNMP, dot1qVlanStaticUntaggedPorts.status, dot1qVlanStaticUntaggedPorts.emsg);

    dot1qPvid                               = cOId("SNMPv2-SMI::mib-2.17.7.1.4.5.1.1");
    if (!dot1qPvid)                         EXCEPTION(ESNMP, dot1qPvid.status, dot1qPvid.emsg);
}

cPortVLans::~cPortVLans()
{
    ;
}

cInspector * cPortVLans::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDevicePV(q, hsid, hoid, pid);
}

/******************************************************************************/

const cService *cDevicePV::pSrvSnmp   = nullptr;

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
    staticOnly = str2tristate(features().value("static_vlans"), EX_IGNORE);
}

cDevicePV::~cDevicePV()
{
    ;
}

void cDevicePV::postInit(QSqlQuery& q, const QString& qs)
{
    cInspector::postInit(q, qs);
    node().fetchPorts(q);

    QString key;
    key = "bitmap_xrefs";
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
                    cNPort *p = node().ports.get(_sPortIndex, portIndex, EX_IGNORE);
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
    key = "no_pvid";
    mNoPVID = features().contains(key);
    qlonglong typeId = cParamType::paramType(_sBoolean).getId();
    int ixParamValue = cPortParam().toIndex(_sParamValue);
    if (!mNoPVID) {
        foreach (cNPort *p, node().ports.list()) {
            cPortParam pp;
            pp.setName(key);
            pp.setId(pp.ixPortId(), p->getId());
            pp.setType(typeId);
            bool f = pp.completion(q) && str2bool(pp.getName(ixParamValue));
            if (f) mNoPvidPorts << int(p->getId(p->ixPortIndex()));
        }
    }
}
#define _DM PDEB(VVERBOSE)
#define DM  _DM << endl;

#define MIN_VLAN_ID 1
#define MAX_VLAN_ID 4097

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
    if (!snmp.isOpened()) {
        EXCEPTION(ESNMP,-1, QString(QObject::tr("SNMP open error : %1 in %2").arg(snmp.emsg).arg(name())));
    }
    switch (staticOnly) {
    case TS_NULL:   // Test
        if (0 != (r = snmp.getBitMaps(par.dot1qVlanCurrentEgressPorts,   currentEgres))
         || 0 != (r = snmp.getBitMaps(par.dot1qVlanCurrentUntaggedPorts, currentUntagged))) {
            runMsg = tr("SNMP '%1' error (test) : #%2/%3, '%4'.")
                    .arg(snmp.name().toString())
                    .arg(snmp.status).arg(r)
                    .arg(snmp.emsg);
            rs = RS_UNREACHABLE;
        }
        else {
            bool ze = currentEgres.isEmpty();
            bool zu = currentUntagged.isEmpty();
            if (ze != zu) {
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
    DBGFNL();
    return rs;
}

int cDevicePV::runSnmpStatic(QSqlQuery& q, QString &runMsg, const cPortVLans& par)
{
    // SNMP query
    int r = 0;
    if (r != 0
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticEgressPorts,          staticEgres))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticForbiddenEgressPorts, staticForbid))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticUntaggedPorts,        staticUntagged))
     || (mNoPVID == false
      && 0 != (r = snmp.getXIndex( par.dot1qPvid, pvidMap)))) {
        runMsg = tr("SNMP '%1' error (query, dynamic): #%2/%3, '%4'.")
                .arg(snmp.name().toString())
                .arg(snmp.status).arg(r)
                .arg(snmp.emsg);
        return RS_UNREACHABLE;
    }
    // Az eszköz portjaihoz tartozó vlan kapcsoló rekordok: mind jelöletlen
    static const QString sql = "UPDATE port_vlans SET flag = false WHERE port_id IN (SELECT port_id FROM nports WHERE node_id = ?)";
    execSql(q, sql, nodeId());
    //
    QMap<int, int>  untaggedSettedMap;
    int rs = RS_ON;
    for (int vid = MIN_VLAN_ID; vid < MAX_VLAN_ID; ++vid) {
        if (staticEgres.contains(vid) || staticForbid.contains(vid)) {    // Van ilyen VLAN ?
            ++ctVlan;   // Számláló. Talált VLAN-ok száma
            tRecordList<cNPort>::iterator i, n = node().ports.end();
            QString vstat;
            for (i = node().ports.begin(); i != n; ++i) {
                int pix = (*i)->getId(_sPortIndex);     // port index
                bool noPvid = mNoPVID || mNoPvidPorts.contains(pix);
                if (!noPvid && !pvidMap.contains(pix)) continue;   // Ha nincs PVID, akkor ez nem VLAN-t támogató port
                int bix = getBitIndex(pix);             // index a bitmap-ban / elvileg azonos, gyakorlatilag meg nem mindíg
                bool isUntagged = maps2bool(staticUntagged, vid, bix);
                if (!noPvid) {
                    bool isPVID = pvidMap[pix] == vid;
                    if (isPVID != isUntagged) {
                        msgAppend(&runMsg,
                                  tr("A PVID és az Untagged port ellentmondása  (static)! Port : %1; PVID = %2; %3 untagged : %4")
                                    .arg((*i)->getName()).arg(pvidMap[pix]).arg(vid).arg(langBool(isUntagged)));
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
            }
        }
    }
    // törlendő jelöletlen rekordok
    bool ok;
    ctRm = execSqlIntFunction(q, &ok, "rm_unmarked_port_vlan", node().getId());
    msgAppend(&runMsg,
              tr("%1 VLAN (static), %2 változatlan, %3 új (%4 új VLAN), %5 változás, %6 törölva, %7 ismeretlen; %8 hiba.")
                .arg(ctVlan).arg(ctUnchg).arg(ctIns + ctNew).arg(ctNew).arg(ctMod)
                .arg(ok ? QString::number(ctRm) : "?")
                .arg(ctUnkn).arg(ctErr));
    return rs;

}

int cDevicePV::runSnmpDynamic(QSqlQuery& q, QString &runMsg, const cPortVLans& par)
{
    // SNMP query
    int r = 0;
    if (currentEgres.isEmpty() || currentUntagged.isEmpty()) {
        r = snmp.getBitMaps(par.dot1qVlanCurrentEgressPorts,   currentEgres);
        if (r == 0) r = snmp.getBitMaps(par.dot1qVlanCurrentUntaggedPorts, currentUntagged);
    }
    if (r != 0
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticEgressPorts,          staticEgres))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticForbiddenEgressPorts, staticForbid))
     || 0 != (r = snmp.getBitMaps(par.dot1qVlanStaticUntaggedPorts,        staticUntagged))
     || 0 != (r = snmp.getXIndex( par.dot1qPvid, pvidMap))) {
        runMsg = tr("SNMP '%1' error (query, dynamic): #%2/%3, '%4'.")
                .arg(snmp.name().toString())
                .arg(snmp.status).arg(r)
                .arg(snmp.emsg);
        return RS_UNREACHABLE;
    }
    // Az eszköz portjaihoz tartozó vlan kapcsoló rekordok: mind jelöletlen
    static const QString sql = "UPDATE port_vlans SET flag = false WHERE port_id IN (SELECT port_id FROM nports WHERE node_id = ?)";
    execSql(q, sql, nodeId());
    //
    QMap<int, int>  untaggedSettedMap;
    int rs = RS_ON;
    for (int vid = MIN_VLAN_ID; vid < MAX_VLAN_ID; ++vid) {
        if (currentEgres.contains(vid) || staticEgres.contains(vid) || staticForbid.contains(vid)) {    // Van ilyen VLAN ?
            ++ctVlan;   // Számláló. Talált VLAN-ok száma
            tRecordList<cNPort>::iterator i, n = node().ports.end();
            QString vstat;
            for (i = node().ports.begin(); i != n; ++i) {
                int pix = (*i)->getId(_sPortIndex);     // port index
                bool noPvid = mNoPVID || mNoPvidPorts.contains(pix);
                if (!noPvid && !pvidMap.contains(pix)) continue;   // Ha nincs PVID, akkor ez nem VLAN-t támogató port
                int bix = getBitIndex(pix);             // index a bitmap-ban / elvileg azonos, gyakorlatilag meg nem mindíg
                bool isUntagged = maps2bool(staticUntagged, vid, bix) || maps2bool(currentUntagged, vid, bix);
                if (!noPvid) {
                    bool isPVID = pvidMap[pix] == vid;
                    if (isPVID != isUntagged) {
                        msgAppend(&runMsg,
                                  tr("A PVID és az Untagged port ellentmondása (dynamic)! Port : %1; PVID = %2; %3 untagged : %4")
                                    .arg((*i)->getName()).arg(pvidMap[pix]).arg(vid).arg(langBool(isUntagged)));
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
                else    continue;
                QString r = execSqlTextFunction(q, "update_port_vlan", (*i)->getId(), vid, vstat);
                switch (reasons(r, EX_IGNORE)) {
                case R_UNCHANGE:    ++ctUnchg;  break;  // Nem volt változás
                case R_NEW:         ++ctNew;    break;  // Új rekord
                case R_INSERT:      ++ctIns;    break;  // Új rekord, és uj VLAN rekord is fel lett véve
                case R_MODIFY:      ++ctMod;    break;  // Modosítva
                default:            ++ctUnkn;   break;  // ??
                }
            }
        }
    }
    // törlendő jelöletlen rekordok
    bool ok;
    ctRm = execSqlIntFunction(q, &ok, "rm_unmarked_port_vlan", node().getId());
    msgAppend(&runMsg,
              tr("%1 VLAN (dynamic), %2 változatlan, %3 új (%4 új VLAN), %5 változás, %6 törölva, %7 ismeretlen; %8 hiba.")
                .arg(ctVlan).arg(ctUnchg).arg(ctIns + ctNew).arg(ctNew).arg(ctMod)
                .arg(ok ? QString::number(ctRm) : "?")
                .arg(ctUnkn).arg(ctErr));
    return rs;
}

