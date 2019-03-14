#ifndef SNMPVARS_H
#define SNMPVARS_H

#include "QtCore"

#define SNMP_IS_EXISTS

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "snmpvars"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2snmpVars;

class cSnmpVars : public cInspector {
public:
    cSnmpVars(QSqlQuery& q, const QString& __sn);
    ~cSnmpVars();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
};

/// @class cDevicePSt
/// Az egy lekérdezendő eszközt reprezentál
class cDeviceSV : public cInspector {
public:
    /// Konstruktor
    cDeviceSV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDeviceSV();
    virtual void postInit(QSqlQuery &q, const QString &qs = QString());
    int queryInit(QSqlQuery &_q, QString& msg);
    /// A lekérdezést végző virtuális metódus.
    /// @param q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual int run(QSqlQuery& q, QString& runMsg);
    bool            first;
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    ///
    QStringList     varNames;
    cOIdVector      oidVector;
    QList<const cServiceVarType *> varTypes;
    tRecordList<cServiceVar> serviceVars;
};


/// Lekédező APP main objektum
class lv2snmpVars : public lanView {
    Q_OBJECT
public:
    lv2snmpVars();
    ~lv2snmpVars();
    static void staticInit(QSqlQuery *pq);
    /// Újra inicializáló metódus
    virtual void setup(eTristate _tr = TS_NULL);
};

#endif // SNMPVARS_H
