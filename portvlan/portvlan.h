#ifndef PORTSTAT_H
#define PORTSTAT_H

#include "QtCore"

#define SNMP_IS_EXISTS

#include "lanview.h"
#include "lv2service.h"
#include "lv2link.h"

#define APPNAME "portvlan"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2portVLan;

class cPortVLans : public cInspector {
public:
    cPortVLans(QSqlQuery& q, const QString& __sn);
    ~cPortVLans();
    virtual void postInit(QSqlQuery &q) override;
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid) override;
    cOId dot1qVlanCurrentEgressPorts;
    cOId dot1qVlanCurrentUntaggedPorts;
    cOId dot1qVlanStaticEgressPorts;
    cOId dot1qVlanStaticForbiddenEgressPorts;
    cOId dot1qVlanStaticUntaggedPorts;
    cOId dot1qPvid;
    cOId hpicfDot1xAuthenticator;
    cOId hpicfDot1xAuthAuthVid;
    cOId hpicfDot1xAuthUnauthVid;
    cOId hpicfDot1xAuthUnauthPeriod;
    cOId hpicfDot1xAuthClientLimit;
    cOId hpicfDot1xAuthLogoffPeriod;
    cOId hpicfDot1xAuthClientLimit2;
    cOIdVector hpicfDot1xAuthConfigTable;
    static const QString sUnauthPeriod;
    static const QString sClientLimit;
    static const QString sLogoffPeriod;
    static const QString sClientLimit2;
    QStringList headerAuthConfigTable;
};

/// @class cDevicePSt
/// Az egy lekérdezendő eszközt reprezentál
class cDevicePV : public cInspector {
public:
    /// Konstruktor
    cDevicePV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDevicePV();
    virtual void postInit(QSqlQuery& q);
    /// A lekérdezést végző virtuális metódus.
    /// @param q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    /// @param runMsg A lekérdezés szöveges eredménye
    /// @return A lekérdezés eredménye, állapot.
    virtual int run(QSqlQuery& q, QString& runMsg);
    int runSnmpStatic(QSqlQuery& q, QString &runMsg, const cPortVLans& par);
    int runSnmpDynamic(QSqlQuery& q, QString &runMsg, const cPortVLans& par);
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    /// Ha csak statikus VLAN kiosztás van, akkor értéke TS_TRUE.
    eTristate   staticOnly;
    /// A PVID értékek ignorálása az eszközre
    bool        mNoPVID;
    ///
    bool        mNoUntaggedBitmap;
    /// A PVID érték ignorálása a portra port index lista
    QList<int>  mNoPvidPorts;
    /// A bitmap-ek ki-dump-olása
    bool mDumpBitMaps;
    /// A trunk portokról nincs infó, azonos a member portok kiosztásával
    bool        mTrunkByMembers;
    bool        f_hpicfDot1xAuthenticator;
    /// A trunkökhöz tartozü member portok kiosztása
    /// trunkMembersVlanTypes[<trunk id>][<trunk member index>][<vlan id>] = <vlan type>
    QMap<qlonglong, QMap<int, QMap<int, QString> > >  trunkMembersVlanTypes;
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
    static int ixPortIndex;
    static int ixPortStapleId;
    static qlonglong multiplexorTypeId;
    /// port index bitmap kereszt referencia táblázat, ha korrigálni kellett a bitmap indexeket,
    /// mert nem azonosak a port indexel.
    /// Ha azonosak az indexek, akkor a konténer üres.
    QMap<int, int>  mIndexXref;
    /// A lekérdzés tiltása a portra port index lista
    QList<int>  mNoVlanPorts;
    QList<int>  mNoVlanPortsByPar;
    int getBitIndex(int pix) { return mIndexXref.contains(pix) ? mIndexXref[pix] : pix; }
    void trunkMap(const cNPort * p, qlonglong trunkId, int vlanId, const QString& vlanType);
    int setTrunks(QSqlQuery &q, QString &runMsg);
    void updatePortVlan(QSqlQuery &q, qlonglong portId, int vlanId, const QString& vlanType);
    int pvid(int pix) {
        int r = -1;     // Invalid VLAN ID
        if (pvidMap.contains(pix)) {
            r = pvidMap[pix];
        }
        return r;
    }

    QMap<int, QBitArray>    currentEgres;
    QMap<int, QBitArray>    currentUntagged;
    QMap<int, QBitArray>    staticEgres;
    QMap<int, QBitArray>    staticUntagged;
    QMap<int, QBitArray>    staticForbid;
    QMap<int, int>          pvidMap;
    int ctVlan, ctUnchg, ctMod, ctNew, ctIns, ctUnkn, ctRm, ctErr;
    cOIdVector hpicfDot1xAuthConfigTable;
    QStringList headerAuthConfigTable;

};


/// Lekédező APP main objektum
class lv2portVLan : public lanView {
    Q_OBJECT
public:
    lv2portVLan();
    ~lv2portVLan();
    static void staticInit(QSqlQuery *pq);
    /// Újra inicializáló metódus
    virtual void setup(eTristate _tr = TS_NULL);
};

#endif // PORTSTAT_H
