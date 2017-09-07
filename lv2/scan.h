#ifndef SCAN_H
#define SCAN_H

#include    "lanview.h"

#if 0   // Nincs használatban
#include "lv2xml.h"

class cSnmpDevice;

class LV2SHARED_EXPORT nmap : public processXml {
public:
    /// A hívott nmap opciók:
    enum eNmapOpts {
        PING     = 0,   ///< Csak pingelés (-sP)
        OSDETECT = 1,   ///< OS detektálás (-A)
        UDPSCAN  = 2,   ///< UDP portok scennelése (-sU)
        TCPSCAN  = 4    ///< TCP portok scennelése (-sS)
    };
    /// OS meghatározás egy eredménye (tippje)
    class os {
    public:
        QString type;   ///< OS type
        QString vendor; ///< OS vendor
        QString family; ///< OS osztály
        QString gen;    ///< OS generáció
        int     accuracy;   ///< A tipp velószínüsége % (0-100)
        os() : type(), vendor(), family(), gen() { accuracy = 0; }
        os(const os& o) : type(o.type), vendor(o.vendor), family(o.family), gen(o.gen) { accuracy = o.accuracy; }
        os& operator=(const os& o) { type = o.type; vendor = o.vendor; family = o.family; gen = o.gen; accuracy = o.accuracy; return *this; }
        void clear() { type.clear(); vendor.clear(); family.clear(); gen.clear(); accuracy = 0; }
    };
    /// MAC cím eredmény
    class mac {
    public:
        cMac    addr;   ///< MAC cím
        QString vendor; ///< A hálózati eszköz készítője (id tulajdonos)
        mac() : addr(), vendor() {;}
        mac(const mac& o) : addr(o.addr), vendor(o.vendor) {;}
        mac& operator=(const mac& o) { addr = o.addr; vendor = o.vendor; return *this; }
        void clear() { addr.clear(); vendor.clear(); }
    };
    /// Port scan egy eredménye
    class port {
    public:
        enum eProto { UNKNOWN = 0, TCP = 6, UDP = 17 }   proto; ///< protokol TCP/UDP
        int id;         ///< Port number (ID)
        QString name;   ///< Port (service) name
        QString prod;   ///< Device/service product info
        QString type;   ///< device type
        QMap<QString, QString>  script; ///< script id - script output
        port() : name(), prod(), type(), script() { id = 0; proto = UNKNOWN; }
        port(const port& o) : name(o.name), prod(o.prod), type(o.type), script() { id = o.id; proto = o.proto; }
        port& operator=(const port& o) { name = o.name; prod = o.prod; type = o.type; script = o.script; id = o.id; proto = o.proto; return *this;}
        void clear() { name.clear(); prod.clear(); type.clear(); script.clear(); id = 0; proto = UNKNOWN; }
    };

    /// Indít egy nmap lekérdezést, az ős processXml objektummal.
    /// A kiadott parancs: "sudo nmap -sU -sS -A -oX - %1"
    /// @param net A címtartomány, amire a lekérdezés vonatkozik
    /// @par opt A lekérdezés opciói ld.: eNmapOpts
    nmap(const netAddress& net, int opt = 0, QObject *parent = NULL);
    virtual ~nmap();
    /// A talált hostkok számának a lekérdezése
    /// @return Ha a lekérdezés lefutott, és az eredmény hiba nélkül fel lett dolgozva, akkor a talált hostok
    /// számával tér vissza. Egyébként -1 -el.
    /// @exception *cError Ha a feldolgozott QDomDocument objektumban nem találja a megfelelő elemet.
    int count() const;
    /// Az első host -hoz tartozó QDomElement objektumra pozicionálás.
    /// A private host adattagot állítja be az első találatra.
    /// @return A QDomElement objeltum referenciájával tér vissza. Ha nincs egy találat sem, akkor a kapott objektum isNull metódusa true-val tér vissza.
    /// @exception *cError Ha a lékérdezés még fut, ill. az eredménye még nincs feldolgozva, ill. ha hiba történt.
    const QDomElement& first();
    /// A következő host -hoz tartozó QDomElement objektumra pozicionálás.
    /// A private host adattagot állítja be a következő találatra.
    /// Ha az aktuális QDomElement üres (isNull() visszatérési értéke true), akkor nem csinál semmit. Ez amiatt is
    /// előfordulhat, mert nem hívtuk a first() metódust.
    /// @return A QDomElement objeltum referenciájával tér vissza. Ha nincs több találat, akkor a kapott objektum isNull metódusa true-val tér vissza.
    /// @exception *cError Ha a lékérdezés még fut, ill. az eredménye még nincs feldolgozva, ill. ha hiba történt.
    const QDomElement& next();
    /// A host adattagot kérdezti le.
    /// @return Az aktuális host QDomElement objektum referenciája.
    const QDomElement& hostElement() const { return host; }
    /// Az aktuális host IP címét (címeit) kérdezi le.
    /// @return Az ip címek listálya.
    netAddressList  getAddress();
    QString getName();
    mac getMAC();
    QList<os> getOs();
    QMap<int, port> getPorts(bool udp = false);
    //int xmlHost(QDomElement& host);PING_SUCCESS
private:
    QDomElement host;
};
#endif

#ifdef SNMP_IS_EXISTS

class cArp;
class cSnmpDevice;

class LV2SHARED_EXPORT cArpTable : public QMap<QHostAddress, cMac> {
public:
    cArpTable() : QMap<QHostAddress, cMac>() { ; }
    cArpTable(const cArpTable& __o) : QMap<QHostAddress, cMac>(__o) { ; }
    cArpTable& getBySnmp(cSnmp& __snmp);
    cArpTable& getByLocalProcFile(const QString& __f = QString());
    cArpTable& getBySshProcFile(const QString& __h, const QString& __f = QString(), const QString& __ru = QString());
    cArpTable& getByLocalDhcpdConf(const QString& __f = QString(), qlonglong _hid = NULL_ID);
    cArpTable& getBySshDhcpdConf(const QString& __h, const QString& __f = QString(), const QString& __ru = QString(), qlonglong _hid = NULL_ID);
    cArpTable& getFromDb(QSqlQuery& __q);
    QList<QHostAddress> operator[](const cMac& __a) const;
    cMac operator[](const QHostAddress& __a) const { return QMap<QHostAddress, cMac>::operator [](__a); }
    cArpTable& operator<<(const cArp& __arp);
    QString toString() const;
protected:
    void getByProcFile(QIODevice& __f);
    void getByDhcpdConf(QIODevice& __f, qlonglong _hid = NULL_ID);
    const QString& token(QIODevice& __f);
};

EXT_ bool setPortsBySnmp(cSnmpDevice &node, enum eEx __ex = EX_ERROR, QString *pEs = NULL);
EXT_ int setSysBySnmp(cSnmpDevice &node, enum eEx __ex = EX_ERROR, QString *pEs = NULL);

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


#else  // SNMP_IS_EXIST

class LV2SHARED_EXPORT cArpTable {
public:
    cArpTable() {
        EXCEPTION(ENOTSUPP);
    }
};


inline void scanByLldp(QSqlQuery, const cSnmpDevice&, bool _parser = false) { (void)_parser; EXCEPTION(ENOTSUPP); }

#endif // SNMP_IS_EXIST

#endif // SCAN_H
