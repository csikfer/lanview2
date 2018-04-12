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
    cOId dot1qVlanStaticEgressPorts;
    cOId dot1qVlanStaticForbiddenEgressPorts;
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
    /// A lekérdezést végző virtuális metódus.
    /// @par q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual int run(QSqlQuery& q, QString& runMsg);
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
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
