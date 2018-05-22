#ifndef PORTSTAT_H
#define PORTSTAT_H

#include "QtCore"

#define SNMP_IS_EXISTS

#include "lanview.h"
#include "lv2service.h"
#include "lv2link.h"

#define APPNAME "portstat"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2portStat;

/// @class cDevicePSt
/// Az egy lekérdezendő eszközt reprezentál
class cDevPortStat : public cInspector {
public:
    /// Konstruktor
    cDevPortStat(QSqlQuery& __q, const QString& _an);
    /// Destruktor
    ~cDevPortStat();
    ///
    virtual void postInit(QSqlQuery &_q, const QString &qs = QString());
    void varsInit(QSqlQuery &_q, cInterface *pInterface);
    /// A lekérdezést végző virtuális metódus.
    /// @par q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual int run(QSqlQuery& q, QString& runMsg);
    /// MAP az al rlinkstat szolgáltatások, port(ID) szerinti kereséséhez
    QMap<qlonglong, cInspector *>  rlinkMap;
    /// MAP az al portvars szolgáltatások, port(ID) szerinti kereséséhez
    QMap<qlonglong, cInspector *>  pVarsMap;
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    ///
    // void setInt(QVariant v, int ix, cInterface& iface, QBitArray& mask, QSqlQuery &q);
    /// Az "rlinkstat" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pRLinkStat;
    static const cService *pPortVars;
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
    static int ixPortOStat;
    static int ixPortAStat;
    static int ixIfdescr;
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
