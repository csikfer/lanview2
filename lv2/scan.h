#ifndef SCAN_H
#define SCAN_H

#include    "lanview.h"

#ifdef SNMP_IS_EXISTS

class cArp;
class cSnmpDevice;

class LV2SHARED_EXPORT cArpTable : public QMap<QHostAddress, cMac> {
public:
    cArpTable() : QMap<QHostAddress, cMac>() { ; }
    cArpTable(const cArpTable& __o) : QMap<QHostAddress, cMac>(__o) { ; }
    cArpTable& getBySnmp(cSnmp& __snmp);
    int getByLocalProcFile(const QString& __f = QString());
    int getBySshProcFile(const QString& __h, const QString& __f = QString(), const QString& __ru = QString());
    int getByLocalDhcpdConf(const QString& __f = QString(), qlonglong _hid = NULL_ID);
    int getBySshDhcpdConf(const QString& __h, const QString& __f = QString(), const QString& __ru = QString(), qlonglong _hid = NULL_ID);
    cArpTable& getFromDb(QSqlQuery& __q);
    QList<QHostAddress> operator[](const cMac& __a) const;
    cMac operator[](const QHostAddress& __a) const { return QMap<QHostAddress, cMac>::operator [](__a); }
    cArpTable& operator<<(const cArp& __arp);
    QString toString() const;
protected:
    int getByProcFile(QIODevice& __f);
    int getByDhcpdConf(QIODevice& __f, qlonglong _hid = NULL_ID);
    const QString& token(QIODevice& __f);
};

EXT_ bool setPortsBySnmp(cSnmpDevice &node, enum eEx __ex = EX_ERROR, QString *pEs = NULL, QHostAddress *ip = NULL);
EXT_ int setSysBySnmp(cSnmpDevice &node, enum eEx __ex = EX_ERROR, QString *pEs = NULL, QHostAddress *ip = NULL);

EXT_ QString lookup(const QHostAddress& ha, enum eEx __ex = EX_ERROR);

/// Az LLDP protokol alapján felfedezi a hálózati switch-eket, és a kapcsolódásukat.
/// Az újonnan felfedezett switch-eket SNMP protokollal lekérdezi
/// lásd :cSnmpDevice::setBySnmp(const QString& __addr, const QString& __com, eEx __ex)
/// és felveszi az adatbázisba, ha még ott nem szerepelnek.
/// Az objektum neve az IP cím alapján a DNS-szerver álltal visszaadott név.
/// @param q Az adatbázis műveletekhez használlható query objektum
/// @param __dev A kiíndulási eszköz, adatbázisban rögzített, onnen feltöltött objektum.
EXT_ void scanByLldp(QSqlQuery q, const cSnmpDevice& __dev, bool _parser = false);

EXT_ void lldpInfo(QSqlQuery q, const cSnmpDevice& __dev, bool _parser);

/// Egy eszköz helyének a közvetlen lekérdezése a switch-ek címtáblái alapján
/// @param _mac A keresett eszköz MAC-je
/// @param _ip A keresett eszköz IP címe.
/// @param _start A kiíndulási switch objektum.
EXT_ void exploreByAddress(cMac _mac, QHostAddress _ip, cSnmpDevice& _start);

#else  // SNMP_IS_EXIST

class LV2SHARED_EXPORT cArpTable {
public:
    cArpTable() {
        EXCEPTION(ENOTSUPP);
    }
};


inline void scanByLldp(QSqlQuery, const cSnmpDevice&, bool _parser = false) { (void)_parser; EXCEPTION(ENOTSUPP); }

inline void exploreByAddress(cMac, QHostAddress, cSnmpDevice&) { EXCEPTION(ENOTSUPP); }

#endif // SNMP_IS_EXIST

#endif // SCAN_H
