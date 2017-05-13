#ifndef ARPD_H
#define ARPD_H

#include "QtCore"

#include "lanview.h"
#include "lv2service.h"

#define APPNAME "arpd"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  APP

class lv2ArpD;

class cArpDaemon : public cInspector {
public:
    cArpDaemon(QSqlQuery& q, const QString& __sn);
    ~cArpDaemon();
    virtual cInspector * newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid);
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
    /// @param q A lekerdezés eredményét a q objetummal írja az adatbázisba.
    virtual int run(QSqlQuery& q, QString &runMsg);
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
    /// Indulás
    virtual void setup(eTristate _tr = TS_NULL);
    static void staticInit(QSqlQuery *pq);
};

#endif // ARPD_H
