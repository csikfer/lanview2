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
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
    cOId dot1qVlanCurrentEgressPorts;
    cOId dot1qVlanCurrentUntaggedPorts;
    cOId dot1qVlanStaticEgressPorts;
    cOId dot1qVlanStaticForbiddenEgressPorts;
    cOId dot1qVlanStaticUntaggedPorts;
    cOId dot1qPvid;
};

/// @class cDevicePSt
/// Az egy lekérdezendő eszközt reprezentál
class cDevicePV : public cInspector {
public:
    /// Konstruktor
    cDevicePV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDevicePV();
    virtual void postInit(QSqlQuery& q, const QString& qs = QString());
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
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
    /// port index bitmap kereszt referencia táblázat, ha korrigálni kellett a bitmap indexeket,
    /// mert nem azonosak a port indexel.
    /// Ha azonosak az indexek, akkor a konténer üres.
    QMap<int, int>  mIndexXref;
    int getBitIndex(int pix) { return mIndexXref.contains(pix) ? mIndexXref[pix] : pix; }

    QMap<int, QBitArray>    currentEgres;
    QMap<int, QBitArray>    currentUntagged;
    QMap<int, QBitArray>    staticEgres;
    QMap<int, QBitArray>    staticUntagged;
    QMap<int, QBitArray>    staticForbid;
    QMap<int, int>          pvidMap;
    int ctVlan, ctUnchg, ctMod, ctNew, ctIns, ctUnkn, ctRm, ctErr;
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
