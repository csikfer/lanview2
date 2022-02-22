#ifndef SCAN_H
#define SCAN_H

#include    "lanview.h"

class cSnmpDevice;

#ifdef SNMP_IS_EXISTS

class cArp;

/// @class cArpTable
/// @brief MAC and IP address pairs query from different sources
class LV2SHARED_EXPORT cArpTable : public QMap<QHostAddress, cMac> {
public:
    /// Empty (default) constructor
    cArpTable() : QMap<QHostAddress, cMac>() { ; }
    /// Copy konstruktor
    cArpTable(const cArpTable& __o) : QMap<QHostAddress, cMac>(__o) { ; }
    /// SNMP-n Query by SNMP (OId : IP-MIB::ipNetToMediaPhysAddress )
    /// @param __snmp A lekérdezéshez használlt objektum, az snmp eszköz az open() metódussal meg van nyitva.
    cArpTable& getBySnmp(cSnmp& __snmp);
    /// Querying the ARP table through the proc file system on the local system
    /// @param __f File name. Opcional, default : /proc/net/arp .
    /// @param pEMsg Taget for failure messages generated during the query, optional, if not specified, will be lost with the message.
    int getByLocalProcFile(const QString& __f = QString(), QString *pEMsg = nullptr);
    /// Querying the ARP table through the proc file system on the remote system, by SSH
    /// @param __h Remote host name or IP address
    /// @param __f File name. Opcional, default : /proc/net/arp .
    /// @param __ru Optional user name
    /// @param pEMsg Taget for failure messages generated during the query, optional, if not specified, will be lost with the message.
    int getBySshProcFile(const QString& __h, const QString& __f = QString(), const QString& __ru = QString(), QString *pEMsg = nullptr);
    /// Address pairs from the DHCP configuration file based on the fixed ip address entries.
    /// @param __f File name, optional. Default file name : "/etc/dhcp3/dhcpd.conf"
    /// @param _hid host_service_id opcional
    /// @return Number of results or -1 if there was an error.
    int getByLocalDhcpdConf(const QString& __f = QString(), qlonglong _hid = NULL_ID);
    /// Address pairs from the DHCP configuration file based on the fixed ip address entries.
    /// @param __h Host address
    /// @param __f File name, optional. Default file name : "/etc/dhcp3/dhcpd.conf"
    /// @param __ru Remote user name, optional
    /// @param _hid host_service_id opcional
    /// @return Number of results or -1 if there was an error.
    int getBySshDhcpdConf(const QString& __h, const QString& __f = QString(), const QString& __ru = QString(), qlonglong _hid = NULL_ID);
    /// Address pairs based on DHCP leased
    /// @param __f File name, optional. Default file name : "/var/lib/dhcp/dhcpd.leases"
    /// @return Number of results or -1 if there was an error.
    int getByLocalDhcpdLeases(const QString& __f = QString());
    /// Address pairs based on DHCP leased
    /// @param __h Host address
    /// @param __f File name, optional. Default file name : "/var/lib/dhcp/dhcpd.leases"
    /// @param __ru Remote user name, optional
    /// @return Number of results or -1 if there was an error.
    int getBySshDhcpdLeases(const QString& __h, const QString& __f = QString(), const QString& __ru = QString());
    /// Fetch all table from db.
    cArpTable& getFromDb(QSqlQuery& __q);
    /// Get ip address list by MAC
    QList<QHostAddress> operator[](const cMac& __a) const;
    /// Get MAC by IP address
    cMac operator[](const QHostAddress& __a) const { return QMap<QHostAddress, cMac>::operator [](__a); }
    cArpTable& operator<<(const cArp& __arp);
    QString toString() const;
protected:
    int getByProcFile(QIODevice& __f, QString *pEMsg = nullptr);
    int getByDhcpdConf(QIODevice& __f, qlonglong _hid = NULL_ID);
    int getByDhcpdLease(QIODevice& __f);
    const QString& token(QIODevice& __f);
};

/// Set cSnmpDevoce (snmpdevices) object ports by SNMP query.
EXT_ bool setPortsBySnmp(cSnmpDevice &node, enum eEx __ex = EX_ERROR, QString *pEs = nullptr, QHostAddress *ip = nullptr, cTable *_pTable = nullptr);
/// Set cSnmpDevoce (snmpdevices) object fields by SNMP query.
EXT_ int setSysBySnmp(cSnmpDevice &node, enum eEx __ex = EX_ERROR, QString *pEs = nullptr, QHostAddress *ip = nullptr);

/// Reverse lookup by host address
EXT_ QString lookup(const QHostAddress& ha, enum eEx __ex = EX_ERROR);

/// Az LLDP protokol alapján felfedezi a hálózati switch-eket, és a kapcsolódásukat.
/// Az újonnan felfedezett switch-eket SNMP protokollal lekérdezi
/// lásd :cSnmpDevice::setBySnmp(const QString& __addr, const QString& __com, eEx __ex)
/// és felveszi az adatbázisba, ha még ott nem szerepelnek.
/// Az objektum neve az IP cím alapján a DNS-szerver álltal visszaadott név.
/// @param q Az adatbázis műveletekhez használlható query objektum
/// @param __dev A kiíndulási eszköz, adatbázisban rögzített, onnen feltöltött objektum.
EXT_ void scanByLldp(QSqlQuery& q, const cSnmpDevice& __dev, bool _parser = false);

/// Egy eszköz szomszédainak a felfedezése
EXT_ void lldpInfo(QSqlQuery& q, const cSnmpDevice& __dev, bool _parser);

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


inline void scanByLldp(QSqlQuery&, const cSnmpDevice&, bool _parser = false) { (void)_parser; EXCEPTION(ENOTSUPP); }
inline void lldpInfo(QSqlQuery&, const cSnmpDevice&, bool _parser = false) { (void)_parser; EXCEPTION(ENOTSUPP); }
inline void exploreByAddress(cMac, QHostAddress, cSnmpDevice&) { EXCEPTION(ENOTSUPP); }

#endif // SNMP_IS_EXIST

EXT_ cMac ip2macByArpTable(const QHostAddress& a);
EXT_ QHostAddress mac2ipByArpTable(const cMac& a);

EXT_ bool parseProcTable(const QString& _fname, cTable& table, QString *pMsg = nullptr, const QString& sFSep = QString("\\s"), const QString& sSSep = QString(), int headSize = 1);

#endif // SCAN_H
