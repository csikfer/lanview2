#ifndef PORTSTAT_H
#define PORTSTAT_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"
#include "lv2link.h"

#define APPNAME "portstat"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2portStat;

class cPortStat : public cInspector {
public:
    cPortStat(QSqlQuery& q, const QString& __sn);
    ~cPortStat();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
};

/// @class cDevicePSt
/// Az egy lekérdezendő eszközt reprezentál
class cDevicePSt : public cInspector {
public:
    /// Konstruktor
    cDevicePSt(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDevicePSt();
    ///
    virtual void postInit(QSqlQuery &q, const QString &qs);
    /// A lekérdezést végző virtuális metódus.
    /// @par q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual enum eNotifSwitch run(QSqlQuery& q);
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    /// Az "rlinkstat" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pRLinkStat;
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
};


/// Lekédező APP main objektum
class lv2portStat : public lanView {
    Q_OBJECT
public:
    lv2portStat();
    ~lv2portStat();
    static void staticInit(QSqlQuery *pq);
    /// Újra inicializáló metódus
    virtual void setup(eTristate _tr = TS_NULL);
};



#endif // PORTSTAT_H
