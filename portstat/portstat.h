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
    virtual int run(QSqlQuery& q, QString& runMsg);
    /// MAP az al rlinkstat szolgáltatások, port(ID) szerinti kereséséhez
    QMap<qlonglong, cInspector *>  inspectorMap;
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    ///
    void setInt(QVariant v, int ix, cInterface& iface, QBitArray& mask, QSqlQuery &q);
    /// Az "rlinkstat" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pRLinkStat;
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
    static int ixPortOStat;
    static int ixPortAStat;
    static int ixIfdescr;
    static int ixIfmtu;
    static int ixIfspeed;
    static int ixIfinoctets;
    static int ixIfinucastpkts;
    static int ixIfinnucastpkts;
    static int ixIfindiscards;
    static int ixIfinerrors;
    static int ixIfoutoctets;
    static int ixIfoutucastpkts;
    static int ixIfoutnucastpkts;
    static int ixIfoutdiscards;
    static int ixIfouterrors;
    static int ixStatLastModify;
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
