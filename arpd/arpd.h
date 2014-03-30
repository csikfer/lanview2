#ifndef ARPD_H
#define ARPD_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "arpd"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  ARPD

class lv2ArpD;

class cArpDaemon : public cInspector {
public:
    cArpDaemon(QSqlQuery& q, const QString& __sn);
    ~cArpDaemon();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
    virtual enum eNotifSwitch run(QSqlQuery &q);
};

/// @class cDeviceArp
/// Az egy lekérdezendő eszközt reprezentál
class cDeviceArp : public cInspector {
public:
    /// Konstruktor
    cDeviceArp(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector *_par);
    /// Destruktor
    ~cDeviceArp();
    /// A lekérdezést végző virtuális metódus.
    /// @par q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual enum eNotifSwitch run(QSqlQuery& q);
    /// SNMP objektum a lekérdezéshez, ha kell
    cSnmp      *pSnmp;
    /// Az ARP proc fájl, vagy a dhcp.conf fájl path, ha kell
    QString    *pFileName;
    /// SSH protokol használata esetén a távoli user, ha meg van adva
    QString    *pRemoteUser;
    /// A Lehetséges prime_service ill. proto_service típusok
    static const cService   *pPSLocal;
    static const cService   *pPSSnmp;
    static const cService   *pPSSsh;
    static const cService   *pPSDhcpConf;
    static const cService   *pPSArpProc;
};


/// Lekédező APP main objektum
class lv2ArpD : public lanView {
    Q_OBJECT
public:
    lv2ArpD();
    ~lv2ArpD();
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
    cArpDaemon      *pSelf;
protected slots:
    /// Adatbázis "NOTIF" ill. event
    void dbNotif(QString __s);
};

#endif // ARPD_H
