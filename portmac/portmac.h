#ifndef PORTMAC_H
#define PORTMAC_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "portmac"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  PORTMAC

class lv2portMac;

class cPortMac : public cInspector {
public:
    cPortMac(QSqlQuery& q, const QString& __sn);
    ~cPortMac();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
    virtual enum eNotifSwitch run(QSqlQuery &q);
/*
      // snmp: klient mac => port snmp index  (on same switch only default_vlan's client)
      // mib-2.dot1dBridge.dot1dTp.dot1dTpFdbTable.dot1dTpFdbEntry.dot1dTpFdbPort = mib-2.17.4.3.1.2
      // $macs : <mac[0]>.<mac[1]>.<mac[2]>.<mac[3]>.<mac[4]>.<mac[5]> => <port snmp index>
      $snmpmacs = $snmp->walk('mib-2.17.4.3.1.2');
      // snmp <vlan ix>.<client mac> => port snmp index
      $temp     = $snmp->walk('mib-2.17.7.1.2.2.1.2');
 */
    cOId    mOId1, mOId2;
};

/// @class cDevice
/// Az egy lekérdezendő eszközt reprezentál
class cDevice : public cInspector {
public:
    /// Konstruktor
    cDevice(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDevice();
    ///
    virtual void postInit(QSqlQuery &q, const QString &qs);
    /// A lekérdezést végző virtuális metódus.
    /// @par q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual enum eNotifSwitch run(QSqlQuery& q);
    /// SNMP objektum a lekérdezéshez
    cSnmp           snmp;
    /// Az "snmp" szolgáltatás típus. A pointert az lv2portStat konstruktora inicializálja.
    static const cService *pSrvSnmp;
};


/// Lekédező main objektum
class lv2portMac : public lanView {
    Q_OBJECT
public:
    lv2portMac();
    ~lv2portMac();
    /// Szignál kezelés
    virtual bool uSigRecv(int __i);
    /// Indulás
    void setup();
    /// Leállás, mindent lebont, hogy hívható legyen a setup()
    void down();
    /// Újra inicializáló metódus
    void reSet();
    ///
    QSqlQuery      *pq;
    /// A saját (superior) szolgáltatás és host objektumok
    cPortMac      *pSelf;
protected slots:
    /// Adatbázis "NOTIF" ill. event
    void dbNotif(QString __s);
};


#endif // PORTMAC_H
