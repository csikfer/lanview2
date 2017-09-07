#ifndef PORTMAC_H
#define PORTMAC_H

#include "QtCore"

#define SNMP_IS_EXISTS
#include "lanview.h"
#include "lv2service.h"
#include "lv2link.h"

#define APPNAME "portmac"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2portMac;

/// @class cPortMac
/// A cInspector típusú (származtatott) gyökér objektum
class cPortMac : public cInspector {
public:
    /// Konstruktor
    /// @param q Az adatbázis művelethez használható QSqlQuery objektum referenciája.
    /// @param __sn A szolgáltatás neve
    cPortMac(QSqlQuery& q, const QString& __sn);
    /// Destruktor (üres).
    ~cPortMac();
    /// Az alárendelt szolgáltatás cDevicePMac objektum létrehozása. Lsd. a cDevicePMac konstruktorát.
    /// @param q Az adatbázis műveletekhez használható objektum.
    /// @param hsid host_service_id
    /// @param hoid A node objektum típusát azonosító tableoid
    /// @param A parent host_service_id
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
    virtual int run(QSqlQuery &q, QString &runMsg);
/*  LanView (PHP):
    // snmp: klient mac => port snmp index  (on same switch only default_vlan's client)
    // mib-2.dot1dBridge.dot1dTp.dot1dTpFdbTable.dot1dTpFdbEntry.dot1dTpFdbPort = mib-2.17.4.3.1.2
    // $macs : <mac[0]>.<mac[1]>.<mac[2]>.<mac[3]>.<mac[4]>.<mac[5]> => <port snmp index>
    $snmpmacs = $snmp->walk('mib-2.17.4.3.1.2');
    // snmp <vlan ix>.<client mac> => port snmp index
    $temp     = $snmp->walk('mib-2.17.7.1.2.2.1.2');
 */
};

class cRightMac;

/// @class cDevicePMac
/// Az egy lekérdezendő eszközt reprezentál
class cDevicePMac : public cInspector {
public:
    /// Konstruktor
    cDevicePMac(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDevicePMac();
    ///
    virtual void postInit(QSqlQuery &q, const QString &qs);
    virtual cInspector *newSubordinate(QSqlQuery &_q, qlonglong _hsid, qlonglong _toid, cInspector *_par);
    /// A lekérdezést végző virtuális metódus.
    /// @par q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual int run(QSqlQuery& q, QString &runMsg);
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    /// Az "snmp" szolgáltatás protokol típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
    /// A releváns port objektumok pinterei, indexelve az SNMP id-re
    QMap<int, cInterface *>         ports;
    /// Mac fehér lista ellenörzések a portokra.
    QMap<qlonglong, cRightMac *>    rightMap;
    /// Talált mac címek a protokhoz
    QMap<qlonglong, QSet<cMac> >    foundMacs;
    /// Az SNMP lekérdezés OID-i.
    static cOId    *pOId1;
    static cOId    *pOId2;
private:
    enum eNotifSwitch snmpQuery(const cOId& __o, QMap<cMac, int>& macs, QString &runMsg);
};

class cRightMac : public cInspector {
public:
    cRightMac(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    void checkMacs(QSqlQuery &q, const QSet<cMac>& macs);
    //qlonglong portId;
    QSet<cMac> rightMacs;
    static qlonglong rightMacId;
};


/// Lekédező main objektum
class lv2portMac : public lanView {
    Q_OBJECT
public:
    lv2portMac();
    ~lv2portMac();
    /// Indulás
    void setup(eTristate _tr = TS_NULL);
    static void staticInit(QSqlQuery *pq);
};


#endif // PORTMAC_H
