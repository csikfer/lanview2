#ifndef PORTSTAT_H
#define PORTSTAT_H

#include "QtCore"

#define SNMP_IS_EXISTS

#include "lanview.h"
#include "lv2service.h"
#include "lv2link.h"

#define APPNAME "pstat"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2portStat;
class cPortStat;

/// @class cDevicePSt
/// A lekérdezendő eszközt reprezentálja
class cDevPortStat : public cInspector {
public:
    /// Konstruktor
    cDevPortStat(QSqlQuery& __q, const QString& _an);
    /// Destruktor
    ~cDevPortStat();
    ///
    virtual void postInit(QSqlQuery &_q);
    int  queryInit(QSqlQuery &_q, QString& msg);
    int checkAvailableVars(QSqlQuery &q);
    /// Mthode of query.
    /// @par q SQL query object
    virtual int run(QSqlQuery& q, QString& runMsg);
    /// first state (query exists variables)
    bool    first;
    /// MAP port (interface) ID -> cPortStat object
    QMap<qlonglong, cPortStat *>  ifMap;
    /// SNMP object for query
    cSnmp           snmp;
    /// Maximum index for the interfaces to be query
    int             maxPortIndex;
    /// Ha egy cServiceVar objektumnál a rarefaction (ritkítás) van megadva (>1),
    /// akkor az objektumban a skeepCnt-t ezzel a számlálóval inicializáljuk úgy,
    /// hogy az adatbázis hozzáférések időben elosztva töténjenek.
    int delayCounter;
    /// Available variable names.
    QStringList     vnames;
    /// Available variable default típes
    QList<qlonglong> vTypes;
    QList<int>       vRarefactions;
    cOIdVector      snmpTabOIds;
    QStringList     snmpTabColumns;
    static const QString sSnmpElapsed;
    cInspectorVar *   pSnmpElapsed;
    /// Az "rlinkstat" szolgáltatás obj. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pRLinkStat;
    /// Az "portvars" szolgáltatás obj. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvPortVars;
    /// Az "snmp" szolgáltatás obj. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
    static int ixPortIndex;
    static int ixIfTypeId;
    static int ixPortStat;
    static int ixPortOStat;
    static int ixPortAStat;
    static int ixIfdescr;
    static int ixLastChanged;
    static int ixLastTouched;
    static int ixDelegatePortState;
};

/// A lekérdezendő portokat reprezentálja
class cPortStat {
public:
    cPortStat(cInterface *pIf, cDevPortStat *par);
    bool postInit(QSqlQuery &q);
    cDevPortStat *  parent;
    cInterface *    pInterface;
    cInspector *    pRlinkStat;
    cInspector *    pPortVars;
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
