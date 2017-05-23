#include "scan.h"
#ifdef MUST_SCAN
#

#include "lv2data.h"
#include "srvdata.h"
#include "lv2link.h"

nmap::nmap(const netAddress& net, int opt, QObject *parent) : processXml(parent), host()
{
    QString par;
    if (!(opt & UDPSCAN))   par  = "-sP ";
    if (opt & TCPSCAN)      par  = "-sS ";
    if (opt & UDPSCAN)      par += "-sU ";
    if (opt & OSDETECT)     par += "-A ";
    start(QString("sudo nmap %1-oX - %2").arg(par, net.toString()));
}

nmap::~nmap()
{
    ;
}

int nmap::count() const
{
    int r = -1;
    if (getStat() == PROCESSED) {
        QDomElement    hosts = (*this)[_sRunStatsHosts];
        if (hosts.isNull()) EXCEPTION(EXML, -1, _sRunStatsHosts);
        r = hosts.attributeNode(_sUp).value().toInt(); // Enyi hosztot talállt
    }
    return r;
}

const QDomElement& nmap::first()
{
    if (getStat() != PROCESSED) EXCEPTION(EPROGFAIL);
    return host = (*this)[_sHost];
}

const QDomElement& nmap::next()
{
    if (getStat() != PROCESSED) EXCEPTION(EPROGFAIL);
    if (host.isNull()) return host;
    return host = host.nextSiblingElement(_sHost);
}

netAddressList  nmap::getAddress()
{
    QDomElement     dome;
    netAddressList  r;
    netAddress      a;
    for (dome = host.firstChildElement(_sAddress); !dome.isNull(); dome = dome.nextSiblingElement(_sAddress)) {
        PDEB(VVERBOSE | cDebug::ADDRESS) << "XML : address : " << dome.nodeName() << " = " << dome.nodeValue() << endl;
        QString addrType = dome.attribute(_sAddrType);
        QString addrVal  = dome.attribute(_sAddr);
        PDEB(VVERBOSE | cDebug::ADDRESS) << "XML : address attr: " << addrType << " = " << addrVal << endl;
        if      (addrType == _sIpV4) {
            a.set(addrVal);
            r << a;
        }
        else if (addrType == _sIpV6) {
            a.set(addrVal);
            r << a;
        }
    }
    return r;
}

QString nmap::getName()
{
    QDomElement     dome;
    QString r;
    dome = host.firstChildElement(_sHostNames);
    if (!dome.isNull()) {
        dome = dome.firstChildElement(_sHostName);  // Csak az elsővel foglalkozunk
        if (!dome.isNull()) {
            r = dome.attribute(_sName);
        }
    }
    return r;
}

nmap::mac nmap::getMAC()
{
    QDomElement     dome;
    nmap::mac       r;
    for (dome = host.firstChildElement(_sAddress); !dome.isNull(); dome = dome.nextSiblingElement(_sAddress)) {
        PDEB(VVERBOSE | cDebug::ADDRESS) << "XML : address : " << dome.nodeName() << " = " << dome.nodeValue() << endl;
        QString addrType = dome.attribute(_sAddrType);
        QString addrVal  = dome.attribute(_sAddr);
        PDEB(VVERBOSE | cDebug::ADDRESS) << "XML : address attr: " << addrType << " = " << addrVal << endl;
        if      (addrType == _sMac) {
            r.addr.set(addrVal);
            r.vendor = dome.attribute(_sVendor);
            return r;
        }
    }
    return r;
}

QList<nmap::os> nmap::getOs()
{
    QList<os>       r;
    QDomElement     dome = host.firstChildElement(_sOs);
    if (dome.isNull()) return r;
    nmap::os        o;
    for (dome = dome.firstChildElement(_sOsClass); !dome.isNull(); dome = dome.nextSiblingElement(_sOsClass)) {
        o.clear();
        o.type     = dome.attribute(_sType);
        o.vendor   = dome.attribute(_sVendor);
        o.family   = dome.attribute(_sOsFamily);
        o.gen      = dome.attribute(_sOsGen);
        o.accuracy = dome.attribute(_sAccuracy).toInt();
        r << o;
    }
    return r;
}

QMap<int, nmap::port> nmap::getPorts(bool udp)
{
    QMap<int, nmap::port>   r;
    nmap::port              o;
    QDomElement             dome = host.firstChildElement(_sPorts);
    if (dome.isNull()) return r;
    QString         sProt = udp ? "udp"     : "tcp";
    port::eProto    eProt = udp ? port::UDP : port::TCP;
    for (dome = dome.firstChildElement(_sPort); !dome.isNull(); dome = dome.nextSiblingElement(_sPort)) {
        if (0 != sProt.compare(dome.attribute(_sProtocol), Qt::CaseInsensitive)) continue;
        QDomElement de;
        de = dome.firstChildElement(_sState);
        // Status is open ?
        if (de.isNull() || 0 != de.attribute(_sState).compare(_sOpen, Qt::CaseInsensitive)) continue;
        o.clear();
        o.proto = eProt;
        o.id    = dome.attribute(_sPortId).toInt();
        de = dome.firstChildElement(_sService);
        if (!de.isNull()) {
            o.name = de.attribute(_sName);
            o.type = de.attribute(_sDeviceType);
            o.prod = de.attribute(_sProduct);
        }
        for (de = dome.firstChildElement(_sScript); !de.isNull(); de = de.nextSiblingElement(_sScript)) {
            QString id = de.attribute(_sId);
            QString ou = de.attribute(_sOutput);
            if (id.isNull() || ou.isNull()) EXCEPTION(EXML);
            o.script[id] = ou;
        }
        r[o.id] = o;
    }
    return r;
}

/***************************************************************************************************************/
cArpTable& cArpTable::getBySnmp(cSnmp& __snmp)
{
    DBGFN();
    cOId oid("IP-MIB::ipNetToMediaPhysAddress");
    PDEB(VVERBOSE) << "Base oid : " << oid.toNumString() << endl;
    if (__snmp.getNext(oid)) EXCEPTION(ESNMP, __snmp.status, "First getNext().")
    do {
        if (!(oid < __snmp.name())) break;
        PDEB(VVERBOSE) << "oid : " << __snmp.name().toNumString() << " = " << dump(__snmp.value().toByteArray()) << endl;
        QHostAddress addr = __snmp.name().toIPV4();
        if (addr.isNull()) {
            QString msg = QString("Invalid IP Address OID : %2").arg(__snmp.name().toString());
            DERR() << msg << endl;
            EXCEPTION(EDATA, -1, msg);
        }
        cMac mac(__snmp.value().toByteArray());
        if (mac.isEmpty()) {
            PDEB(VVERBOSE) << "Dropp NULL MAC..." << endl;
            continue;
        }
        if (!mac) {
            EXCEPTION(EDATA, -1, __snmp.value().toString());
        }
        PDEB(VVERBOSE) << "Insert : " << addr << " - " << mac << endl;
        insert(addr, mac);
    } while (!__snmp.getNext());
    return *this;
}

void cArpTable::getByProcFile(QIODevice& __f)
{
    DBGFN();
    QTextStream str(&__f);
    str.readLine(); // Fejléc
    QString line;
    while ((line = str.readLine()).isEmpty() == false) {
        QStringList fl = line.split(QRegExp("\\s+"));
        QHostAddress addr(fl[0]);
        if (addr.isNull()) {
            DWAR() << "Invalid IP address: " << fl[0] << " ARP line : '" << line << "' is dropped." << endl;
            continue;
        }
        cMac mac(fl[3]);
        if (!mac) {
            DWAR() << "Invalid MAC address: " << fl[3] << " ARP line : '" << line << "' is dropped." << endl;
            continue;
        }
        // PDEB(VVERBOSE) << line << " : " << fl[0] << "/" << fl[3] << " : " << addr.toString() << "/" << mac.toString() << endl;
        if (mac.isEmpty()) {
            // PDEB(VVERBOSE) << "Dropp NULL MAC..." << endl;
            continue;
        }
        PDEB(INFO) << "insert(" << addr.toString() << QChar(',') << mac.toString() << ")" << endl;
        insert(addr, mac);
    }
}
cArpTable& cArpTable::getByLocalProcFile(const QString& __f)
{
    _DBGFN() << "@(" << __f << QChar(',') << endl;
    QFile procFile(__f.isEmpty() ? "/proc/net/arp" : __f);
    if (!procFile.open(QIODevice::ReadOnly | QIODevice::Text)) EXCEPTION(EFOPEN, -1, procFile.fileName());
    getByProcFile(procFile);
    return *this;
}

/// Az ARP tábla lekérdezése a proc fájlrendszeren keresztül, egy távoli gépen SSH-val
/// @param __h A távoli hoszt név, vagy ip cím
/// @param __f A fájl neve a proc fájlrendszerben
/// @param __ru Opcionális, a felhasználó név a távoli gépen.
cArpTable& cArpTable::getBySshProcFile(const QString& __h, const QString& __f, const QString& __ru)
{
    //_DBGFN() << " @(" << VDEB(__h) << QChar(',') << VDEB(__f) << QChar(',') << VDEB(__ru) << QChar(')') << endl;
    QString     ru;
    if (__ru.isEmpty() == false) ru = QString(" -l %1").arg(__ru);
    QString     cmd = QString("ssh %1%2 cat ").arg(__h, ru);
    cmd += __f.isEmpty() ? "/proc/net/arp" : __f;
    QProcess    proc;
    proc.start(cmd, QIODevice::ReadOnly);
    if (false == proc.waitForStarted(  2000)
     || false == proc.waitForFinished(10000)) EXCEPTION(ETO, -1, cmd);
    getByProcFile(proc);
    return *this;
}

const QString& cArpTable::token(QIODevice& __f)
{
    static QString  tok;
    char c;
    tok.clear();
    do {
        if (!__f.getChar(&c)) return tok;
        if (c == '#') { __f.readLine(); continue; } // Komment
    } while (isspace(c));
    tok = QChar(c);
    if (strchr("()[];{}", c)) return tok;    // egy betűs tokenek
    if (c == '"') {
        tok.clear();
        while (__f.getChar(&c) && c != '"') tok += QChar(c);
        return tok;
    }
    do {
        if (!__f.getChar(&c)) return tok;
        if (c == '#') { __f.readLine(); return tok; }
        if (isspace(c)) return tok;
        if (strchr("()[];\"{}", c)) { __f.ungetChar(c); return tok; }
        tok += QChar(c);
    } while (true);
}

#define GETOKEN()   if ((tok = token(__f)).isEmpty()) break;
#define CHKTOK(s)   if (tok != s) continue;

/// dhcp.comf fájl tartalmának az értelmezése
/// @param __f A fájl tartalma
/// @param _hid host_service_id opcionális
void cArpTable::getByDhcpdConf(QIODevice& __f, qlonglong _hid)
{
    //DBGFN();
    static const QString _shost           = "host";
    static const QString _shardware       = "hardware";
    static const QString _sethernet       = "ethernet";
    static const QString _sfixed_address  = "fixed-address";
    static const QString _srange          = "range";
    QTextStream str(&__f);
    QString tok;
    QString _s;
    QVariant hId;
    if (_hid != NULL_ID) hId = _hid;
    while (true) {
        tok = token(__f);
        if (tok.isEmpty()) return;
        if (tok == _shost) {
            GETOKEN();  // név
            GETOKEN();  CHKTOK("{");
            GETOKEN();  CHKTOK(_shardware);
            GETOKEN();  CHKTOK(_sethernet);

            GETOKEN();  cMac    mac(tok);
            if (mac.isEmpty()) continue;

            GETOKEN();  CHKTOK(";");
            GETOKEN();  CHKTOK(_sfixed_address);
            GETOKEN();  QHostAddress addr(tok);
            if (addr.isNull()) continue;

            GETOKEN();  CHKTOK(";");
            GETOKEN();  CHKTOK("}");

            PDEB(VERBOSE) << "add : " << addr.toString() << " / " << mac.toString() << endl;

            insert(addr, mac);
        }
        else if (tok == _srange) {
            QSqlQuery q = getQuery();
            QString sb, se;
            GETOKEN();  sb = tok;   QHostAddress b(tok);
            GETOKEN();  se = tok;   QHostAddress e(tok);
            if (b.isNull() || e.isNull() || b.protocol() != e.protocol()
              || (b.protocol() == QAbstractSocket::IPv4Protocol && b.toIPv4Address() >= e.toIPv4Address())) {
                DERR() << "Dynamic range : " << sb << " > " << se << endl;
                QString hs = QObject::trUtf8("nincs megadva");
                if (_hid != NULL_ID) {
                    hs = cHostService::names(q, _hid);
                }
                QString m = QObject::trUtf8("Invalid dynamic range from %1 to %2, HostService : %3").arg(sb,se,hs);
                APPMEMO(q, m, RS_CRITICAL);
                continue;
            }
            QString r = execSqlTextFunction(q,"replace_dyn_addr_range", b.toString(), e.toString(), hId);
            PDEB(INFO) << "Dynamic range " << b.toString() << " < " << e.toString() << "repl. result: " << r << endl;
        }
    }
    return;
}

cArpTable& cArpTable::getByLocalDhcpdConf(const QString& __f, qlonglong _hid)
{
    //_DBGFN() << " @(" << __f << ")" << endl;
    QFile dhcpdConf(__f.isEmpty() ? "/etc/dhcp3/dhcpd.conf" : __f);
    if (!dhcpdConf.open(QIODevice::ReadOnly | QIODevice::Text)) EXCEPTION(EFOPEN, -1, dhcpdConf.fileName());
    getByDhcpdConf(dhcpdConf, _hid);
    return *this;
}

/// @param _hid host_service_id opcionális
cArpTable& cArpTable::getBySshDhcpdConf(const QString& __h, const QString& __f, const QString& __ru, qlonglong _hid)
{
    //_DBGFN() << " @(" << VDEB(__h) << QChar(',') << VDEB(__f) << QChar(',') << VDEB(__ru) << QChar(')') << endl;
    QString     ru;
    if (__ru.isEmpty() == false) ru = QString(" -l %1").arg(__ru);
    QString     cmd = QString("ssh %1%2 cat ").arg(__h, ru);
    cmd += __f.isEmpty() ? "/etc/dhcp3/dhcpd.conf" : __f;
    QProcess    proc;
    proc.start(cmd, QIODevice::ReadOnly);
    if (false == proc.waitForStarted(  2000)
     || false == proc.waitForFinished(10000)) EXCEPTION(ETO, -1, cmd);
    getByDhcpdConf(proc, _hid);
    return *this;
}


QList<QHostAddress> cArpTable::operator[](const cMac& __a) const
{
    QList<QHostAddress> r;
    const_iterator i;
    for (i = constBegin(); i != constEnd(); ++i) {
        if (i.value() == __a) r << i.key();
    }
    return r;
}

QString cArpTable::toString() const
{
    QString r;
    QSet<cMac> set = values().toSet();
    foreach (cMac mac, set) {
        QList<QHostAddress> al = (*this)[mac];
        r += mac.toString() + " - ";
        if (al.size() == 1) r += al[0].toString();
        else {
            r += "(";
            foreach (QHostAddress a, al) {
                r += a.toString();
                r += ",";
            }
            r.chop(1);
            r += ")";
        }
        r += ";";
    }
    r.chop(1);
    return r;
}

cArpTable& cArpTable::operator<<(const cArp& __arp)
{
    cMac            mac  = __arp;
    QHostAddress    addr = __arp;
    if (!mac) EXCEPTION(EDBDATA, -1, "Invalid MAC.");
    if (addr.isNull()) EXCEPTION(EDBDATA, -1, "Invalid IP address.");
    insert(addr, mac);
    return *this;
}

cArpTable& cArpTable::getFromDb(QSqlQuery& __q)
{
    //DBGFN();
    cArp    arp;
    if (arp.fetch(__q, false, QBitArray(1))) do {
        (*this) << arp;
    } while (arp.next(__q));
    return *this;
}

/**********************************************************************************************/

#define EX(ec,ei,_es) {\
    if (__ex) { EXCEPTION(ec, ei, _es) } \
    else {  \
        es = cError::errorMsg(eError::ec) + " : " + _es + " "; \
        DERR() << es << endl; \
        if (pEs != NULL) *pEs += es; \
    }   return  false; }


bool setPortsBySnmp(cSnmpDevice& node, eEx __ex, QString *pEs)
{
    QString es;
    // Előszedjük a címet
    QHostAddress hostAddr = node.getIpAddress();
    // Kell lennie legalább egy IP címnek!!
    bool    found = false;
    // A gyátrói baromságok kezeléséhez kell
    QString sysdescr = node.getName(_sSysDescr);
    // A portot töröljük, azt majd a lekérdezés teljes adatatartalommal felveszi
    node.ports.clear();
    if (hostAddr.isNull()) {
        if (__ex) EXCEPTION(EDATA,-1, es);
        if (pEs != NULL) *pEs = QObject::trUtf8("Nincs IP cím.");
        return false;
    }

    cSnmp   snmp(hostAddr.toString(), node.getName(_sCommunityRd));
    if (!snmp.isOpened()) EX(ESNMP, 0, node.getName(_sAddress));
    cTable      tab;    // Interface table container
    QStringList cols;   // Table col names container
    cols << _sIfIndex << _sIfDescr << _sIfType << _sIfMtu << _sIfSpeed << _sIfPhysAddress
         << _sIfAdminStatus << _sIfOperStatus;
    int r = snmp.getTable(_sIfMib, cols, tab);
    if (r) EX(ESNMP, r, snmp.emsg);
    PDEB(VVERBOSE) << "*************************************************" << endl;
    PDEB(VVERBOSE) << "SNMP TABLE : " << endl << tab.toString() << endl;
    PDEB(VVERBOSE) << "*************************************************" << endl;
    tab << _sIpAdEntAddr; // Add ip address (empty column) to table
    cOId oid(_sIpMib + _sIpAdEntIfIndex);
    bool ok;
    // Kitöltjük az IP cím oszlopot
    if (0 == snmp.getNext(oid)) do {
        if (NULL == snmp.first()) EX(ESNMP, -1, _sNul);
        // Az első az interface snmp indexe az OID-ben pedig az IP cím
        cOId name    = snmp.name();
        cOId oidAddr = name - oid;
        if (!oidAddr) break;    // Ha a végére értünk
        int  i    = snmp.value().toInt(&ok);
        if (!ok) EX(EDATA, -1, QString("SNMP index: %1 '%2'").arg(name.toString()).arg(snmp.value().toString()));
        QString sAddr = oidAddr.toNumString();
        QHostAddress addr(sAddr);   // ellenörzés
        if (addr.isNull()) EX(EDATA, -1, QString("%1/%2").arg(sAddr, oidAddr.toNumString()));
        if (addr != QHostAddress(QHostAddress::Any)) {  // A 0.0.0.0 címet eldobjuk
            QVariant *p = tab.find(_sIfIndex, i, _sIpAdEntAddr);
            if (!p) EX(EDATA, i, QString("Not found : %1,%2").arg(_sIfIndex, _sIpAdEntAddr));
            p->setValue(sAddr);
            found = true;   // Van IP címünk (a táblázatban)
        }
    } while (0 == snmp.getNext());
    PDEB(VVERBOSE) << "*************************************************" << endl;
    PDEB(VVERBOSE) << "SNMP TABLE+ : " << endl << tab.toString() << endl;
    PDEB(VVERBOSE) << "*************************************************" << endl;
    // Ha nincs IP címünk, az gáz
    if (!found) EX(EDATA, 0, QString("IP not found"));
    found = false;  // A tábla feldolgozása után is kell lennie!
    QSqlQuery q = getQuery();
    int n = tab.rows();
    int i;
    // Az SNMP lekérdezés eredmény táblájából legyártjuk a ports konténert
    for (i = 0; i < n; i++) {
        QHostAddress    addr(tab[_sIpAdEntAddr][i].toString());
        QString         name = tab[_sIfDescr][i].toString();
        int             type = tab[_sIfType][i].toInt(&ok);
        if (!ok) EX(EDATA, -1, QString("SNMP ifType: '%1'").arg(tab[_sIfType][i].toString()));
        // IANA típusból következtetünk az objektum típusára és iftype_name -ra
        const cIfType  *pIfType = cIfType::fromIana(type);
        if (pIfType == NULL) {
            DWAR() << "Unhandled interface type " << type << " : #" << tab[_sIfIndex][i]
                   << QChar(' ') << name << endl;
            continue;
        }
        name.prepend(pIfType->getName(_sIfNamePrefix));       // Esetleges előtag, a név ütközések elkerülésére
        QString         ifTypeName = pIfType->getName();
        cNPort *pPort = cNPort::newPortObj(*pIfType);
        if (pPort->descr().tableName() != _sInterfaces) {
            EX(EDATA, -1, QObject::trUtf8("Invalid port object type"));
        }
        cMac            mac(tab[_sIfPhysAddress][i].toByteArray());
        if (pPort->descr().tableName() == _sNPorts && mac.isValid()) {
            DWAR() << "Interface " << name << " Drop HW address " << mac.toString() << endl;
            mac.clear();
        }
        // A SonicWall használhatatlan portneveket ad vissza, lekapjuk róla a megjegyzés részt
        if (sysdescr.contains("sonicwall", Qt::CaseInsensitive)) {
            QStringList sl = name.split(QChar(' '));
            if (sl.size() > 1) {
                pPort->setNote(name);
                name = sl[0];
            }
        }
        pPort->setName(name);
        int ifIndex = tab[_sIfIndex][i].toInt();
        pPort->set(_sPortIndex, ifIndex);
        pPort->set(_sIfTypeId, pIfType->getId());
        if (mac.isValid())  pPort->set(_sHwAddress, mac.toString());
        if (!addr.isNull()) {   // Van IP címünk
            cInterface *pIf = pPort->reconvert<cInterface>();
            cIpAddress& pa = pIf->addIpAddress(addr, _sFixIp);
            pa.thisIsExternal(q);    // Ez lehet külső cím is !!
            // A paraméterként megadott címet preferáltnak vesszük
            if (addr == hostAddr) {   // Ez az
                pa.setId(_sPreferred, 0);
            }
            found = true;
        }
        PDEB(VVERBOSE) << "Insert port : " << pPort->toString() << endl;
        node.ports << pPort;
        // Trunk port hozzárendelések lekérdezése
        if (ifTypeName == _sMultiplexor) {
            cOId oid;
            oid.set("IF-MIB::ifStackStatus");
            oid << ifIndex;
            if (snmp.getNext(oid)) {
                DERR() << "cSnmp::getNext(" << oid.toString() << ") error : " << snmp.emsg << endl;
            }
            else do {
                cOId o = snmp.name() - oid;
                PDEB(VVERBOSE) << "OID : " << snmp.name().toString() << " / " << o.toNumString() << endl;
                if (o.isEmpty()) break;     // túl szaladtunk;
                if (o.size() != 1) EX(EDATA, ifIndex, o.toNumString()); // egy eleműnek kellene lennie (a keresett port indexe).
                if (o.at(0) == 0) continue; // töltelék, nem kell...
                cInterface *pif = dynamic_cast<cInterface *>(pPort);
                pif->addTrunkMember(o.at(0));
            } while (!snmp.getNext());
        }
    }
    if (!found) {   // Nincs IP címünk!!!
        for (i = 0; i < n; i++) {   // Nekifutunk mégegyszer a táblázatnak
            int             type = tab[_sIfType][i].toInt(&ok);
            const cIfType  *pIfType = cIfType::fromIana(type);
            if (pIfType == NULL) {  // Azokat keressük, amit az elöbb kihagytunk
                QHostAddress    addr(tab[_sIpAdEntAddr][i].toString());
                pIfType = &cIfType::ifType(_sVEth);  // "Kinevezzük" virtuális ethernet-nek (nem kezelt típus, mert az elöbb eldobtuk)
                QString         name = tab[_sIfDescr][i].toString();
                name.prepend(pIfType->getName(_sIfNamePrefix));       // Esetleges előtag, a név ütközések elkerülésére
                cNPort *pPort = cNPort::newPortObj(*pIfType);
                if (pPort->descr().tableName() != _sInterfaces) {
                    EX(EDATA, -1, QObject::trUtf8("Invalid port object type"));
                }
                cMac            mac(tab[_sIfPhysAddress][i].toByteArray());
                if (pPort->descr().tableName() == _sNPorts && mac.isValid()) {
                    DWAR() << "Interface " << name << " Drop HW address " << mac.toString() << endl;
                    mac.clear();
                }
                pPort->setName(name);
                int ifIndex = tab[_sIfIndex][i].toInt();
                pPort->set(_sPortIndex, ifIndex);
                pPort->set(_sIfTypeId, pIfType->getId());
                if (mac.isValid())  pPort->set(_sHwAddress, mac.toString());
                cInterface *pIf = pPort->reconvert<cInterface>();
                cIpAddress& pa = pIf->addIpAddress(addr, _sFixIp);
                pa.thisIsExternal(q);    // Ez lehet külső cím is !!
                // A paraméterként megadott címet preferáltnak vesszük
                if (addr == hostAddr) {   // Ez az
                    pa.setId(_sPreferred, 0);
                }
                PDEB(VVERBOSE) << "Insert port : " << pPort->toString() << endl;
                node.ports << pPort;
                found = true;
            }
        }
    }
    if (!found) EX(EDATA, -1, QString("IP is not found"));
    _DBGFNL() << "OK, node : " << node.toString() << endl;
    return true;
}

/// Egy SNMP érték lekérdezése, és az eredmény beállítása a megadott mezőben
/// @param node Az objektum (host)
/// @param snmp A cSnmp objektum, a lekérdezéshez (meg van nyitva)
/// @param pEs Az opcionális hiba üzenet cél string pointere
/// @param s A lekérdezendő SNMP OID objektum
/// @param f A cél mező neve
static inline bool snmpset(cSnmpDevice &node, cSnmp &snmp, QString *pEs, const QString& s, const QString& f)
{
    if (0 != snmp.get(s)) {
        QString es = "cSnmp::get(" + s + ") error : " + snmp.emsg;
        DERR() << "Node \"" << node.getName() << "\" : " << es << endl;
        node.clear(f);
        if (pEs != NULL) {
            *pEs += es + " ";
        }
        return true; /* WARNING */
    }
    else {
        node.set(f, snmp.value());
    }
    return false; /* OK */
}

int setSysBySnmp(cSnmpDevice &node, eEx __ex, QString *pEs)
{
    int r = 1;  //OK
    QString es;
    QString ma = node.getIpAddress().toString();
    if (ma.isEmpty()) {
        es = QObject::trUtf8("Hiányzó IP cím.");
        if (__ex) EXCEPTION(EDATA, -1, es);
        DERR() << es << endl;
        if (pEs != NULL) *pEs += es;
        return -1;  // ERROR
    }
    cSnmp   snmp(ma, node.getName(_sCommunityRd));

    if (!snmp.isOpened()) {
        es = QObject::trUtf8("SNMP open(%1, %2) error. ").arg(ma).arg(node.getName(_sCommunityRd));
        if (__ex) EXCEPTION(EDATA, -1, es);
        DERR() << es << endl;
        if (pEs != NULL) *pEs += es;
        return -1;  // ERROR
    }
    bool war = false;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysDescr.0",    _sSysDescr)    || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysObjectID.0", _sSysObjectId) || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysContact.0",  _sSysContact)  || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysName.0",     _sSysName)     || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysLocation.0", _sSysLocation) || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysServices.0", _sSysServices) || war;
    if (war) r = 0;
    return r;
}

/**********************************************************************************************/

QString lookup(const QHostAddress& ha, eEx __ex)
{
    QHostInfo hi = QHostInfo::fromName(ha.toString());
    QString r = hi.hostName();
    if (hi.error() != QHostInfo::NoError || ha.toString() == r) {
        if (__ex) EXCEPTION(EFOUND, (int)hi.error(), QObject::trUtf8("A hoszt név nem állapítható meg"));
        return _sNul;
    }
    return r;
}

/**********************************************************************************************/

/// @class cLldpScan
/// Az LLDP felderítést végző osztály
/// Valamit ki kell találni, mert a jelenlegi módszer nem tudja kezelni ezt az őskáoszt amit
/// az LLDP címén a tisztellt gyártók/szabványalkotók kitaláltak.
/// Vagy jobban meg kéne érteni a szarrá bonyolított rendszert.
class cLldpScan {
    friend void ::scanByLldp(QSqlQuery q, const cSnmpDevice& __dev, bool _parser);
    friend void ::lldpInfo(QSqlQuery q, const cSnmpDevice& __dev, bool _parser);
    friend int  snmpNextField(int n, cSnmp& snmp, QString& em, int& _ix);
    friend void staticAddOid(QString __cs, int i);
protected:
    struct rowData {
        cMac            cmac;   ///< ChassisId (MAC)
        cMac            pmac;   ///< PortId    (MAC)
        QString         name;   ///<
        QString         descr;  ///<
        QString         pname;  ///<
        QString         pdescr; ///<
        QHostAddress    addr;   ///<
        QString         clocal; ///< ChassisId (local)
    };
    cLldpScan(QSqlQuery& _q, bool _parser);
    /// LLDP felderítés
    /// @param __dev A kiindulási SNMP eszköz
    void scanByLldp(QSqlQuery &q, const cSnmpDevice& __dev);
    /// LLDP felderítés, egy eszköz, a pDev lekérdezése
    void scanByLldpDev(QSqlQuery &q);
    /// LLDP felderítés, Az SNMP lekérdezés "egy sorának" a feldolgozása
    void scanByLldpDevRow(QSqlQuery &q, cSnmp &snmp, int port_ix, rowData &row);
    /// Egy a MAC alapján nem talált, de felderített eszköz keresése vagy létrehozása.
    /// Ha az IP alapján megtalálja az eszközt, akkor természetesen nem hozza létre.
    /// Csak akkor hozza létre az adatbázisban nem talált eszközt, ha az SNMP-n kerestül lekérdezhető.
    /// Hiba esetén egyszerúen vissatér, csak fatális hiba esetén dob kizárást.
    /// @return Ha megtalálta, vagy megkreálta az eszközt (ill. a hozzá tartozó rekordokat), akkor true, egyébként false.
    bool updateLink(cAppMemo &em);

    bool rowProCurve(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em);
    bool rowProCurveWeb(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em);
    bool row3COM(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em);
    bool rowCisco(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em);
    bool rowHPAPC(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em);
    bool rowLinux(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em);
    bool rowEmpty(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em);

    bool setRPortFromIx(cAppMemo& em);
    bool setRPortFromMac(rowData &row, cAppMemo &em);
    int  portDescr2Ix(QSqlQuery &q, cSnmp &snmp, QHostAddress ha, const QString& pdescr, cAppMemo &em);
    bool rowTail(QSqlQuery &q, const QString &ma, const QStringPair &ip, cAppMemo &em, bool exists);
    void memo(cAppMemo em, QSqlQuery &q, int port_ix, rowData &row);

    typedef tRecordList<cSnmpDevice> tDevList;
    bool        parser;         ///< A parser-ből lett hivva?
    tDevList    scanned;        ///< A megtalált eszközök nevei, ill. amit már nem kell lekérdezni
    tDevList    queued;         ///< A megtatlált, de még nem lekérdezett eszközök nevei
    cSnmpDevice *pDev;          ///< Az aktuális scennelt eszköz
    QSqlQuery&  q;              ///< Az adatbázis műveletekhez használt SQL Query objektum
    cMac        rMac;           ///< Aktuális távoli eszköz MAC
    cSnmpDevice rDev;           ///< Aktuális távoli device, azonos rHost-al vagy üres
    cNode       rHost;          ///< Aktuális távoli hoszt
    int         rPortIx;
    cInterface  rPort;          ///< Akruélis távoli port
    cInterface  rMacPort;       ///< Aktuális távoli port a távoli MAC címhez
    cInterface  rIpPort;        ///< Aktuális távoli port a távoli IP  címhez
    int         lPortIx;        ///< Lokális port indexe
    cInterface  lPort;          ///< A lokális port objektum
    cLldpLink   lnk;            ///< Link objektum
    QMap<int, rowData>  rRows;
    QString choice;

    static QStringList  sOids;      ///< Az SNMP lekérdezésnél használt kiindulási objektum név lista
    static cOIdVector   oids;       ///< Az SNMP lekérdezésnél használt kiindulási objektum lista
    static QString      sAddrOid;   ///< Távoli eszköz címe objektum név
    static cOId        *pAddrOid;   ///< Távoli eszköz címe objektum, külön kell lekérdezni, hiányozhat
    static void staticInit();       ///< Az oids és oidss statikus konténerek inicializálása/feltöltése
};

QStringList  cLldpScan::sOids;
cOIdVector   cLldpScan::oids;
QString      cLldpScan::sAddrOid;
cOId        *cLldpScan::pAddrOid = NULL;

cLldpScan::cLldpScan(QSqlQuery& _q, bool _parser)
    : scanned(), queued(), q(_q), rMac(), rDev(), rHost(), rPort(), lPort(), lnk()
{
    parser = _parser;
    staticInit();
    pDev = NULL;
}

#define LLDP_REM_CID_TYPE   0
#define LLDP_REM_CID        1
#define LLDP_REM_PID_TYPE   2
#define LLDP_REM_PID        3
#define LLDP_REM_PDSC       4
#define LLDP_REM_SDSC       5
#define LLDP_REM_SNAME      6

inline void staticAddOid(QString __s, int i)
{
    if (cLldpScan::sOids.size() != i || cLldpScan::oids.size() != i) EXCEPTION(EPROGFAIL);
    cLldpScan::sOids << __s;
    cLldpScan::oids  << cOId(cLldpScan::sOids.last());
    if (!cLldpScan::oids.last()) EXCEPTION(EOID, -1, cLldpScan::sOids.last());
}

void cLldpScan::staticInit() {
    // One might ask directly to the table, but the exact rows identifiers are also needed.
    if (sOids.isEmpty()) {
        static const QString m = "LLDP-MIB::";
        staticAddOid(m + "lldpRemChassisIdSubtype", LLDP_REM_CID_TYPE);
        staticAddOid(m + "lldpRemChassisId",        LLDP_REM_CID);
        staticAddOid(m + "lldpRemPortIdSubtype",    LLDP_REM_PID_TYPE);
        staticAddOid(m + "lldpRemPortId",           LLDP_REM_PID);
        staticAddOid(m + "lldpRemPortDesc",         LLDP_REM_PDSC);
        staticAddOid(m + "lldpRemSysDesc",          LLDP_REM_SDSC);
        staticAddOid(m + "lldpRemSysName",          LLDP_REM_SNAME);
        sAddrOid =   m + "lldpRemManAddrIfId";
        pAddrOid = new cOId(sAddrOid);
        if (!*pAddrOid) EXCEPTION(EOID, -1, sAddrOid);
    }
}

EXT_ bool isBreakImportParser(bool _except);

bool cLldpScan::updateLink(cAppMemo &em)
{
    DBGFNL();

    lnk.clear();
    qlonglong lId = lPort.getId();
    qlonglong rId = rPort.getId();
    int r;

    if (lId == NULL_ID || rId == NULL_ID) {
        QString es = QObject::trUtf8("LLDP link update failed.");
        if (lId == NULL_ID) es += QObject::trUtf8(" Local port ID is NULL.");
        if (rId == NULL_ID) es += QObject::trUtf8(" Remote port ID is NULL.");
        HEREIN(em, es, RS_WARNING);
        return false;
    }

    if (lnk.isLinked(q, lId, rId)) {
        r = lnk.touch(q);
        switch (r) {
        case 0:
            HEREIN(em, QObject::trUtf8("Az LLDP link időbéllyegének a modosítása sikertelen."), RS_WARNING);
            break;
        case 1:
            return true;
        default:
            HEREIN(em, QObject::trUtf8("Az LLDP link igőbéllyegének a modosítása. Értelmezhetetlen eredmény : %1").arg(r), RS_WARNING);
            break;
        }
        return false;
    }

    if (NULL_ID != lnk.getLinked(q, lId)) {
        PDEB(INFO) << "Remove old link (local port): " << lnk.toString() << endl;
        r = lnk.remove(q);
        switch (r) {
        case 0:
            HEREIN(em, QObject::trUtf8("Régi LLDP link (L) törlése sikertelen."), RS_WARNING);
            return false;
        case 1:
            break;
        default:
            HEREIN(em, QObject::trUtf8("Régi LLDP link (L) törlése. Értelmezhetetlen eredmény : %1").arg(r), RS_WARNING);
            return false;
        }
    }
    if (NULL_ID != lnk.getLinked(q, rId)) {
        PDEB(INFO) << "Remove old link : (remote port)" << lnk.toString() << endl;
         r = lnk.remove(q);
         switch (r) {
         case 0:
             HEREIN(em, QObject::trUtf8("Régi LLDP link (R) törlése sikertelen."), RS_WARNING);
             return false;
         case 1:
             break;
         default:
             HEREIN(em, QObject::trUtf8("Régi LLDP link (R) törlése. Értelmezhetetlen eredmény : %1").arg(r), RS_WARNING);
             return false;
         }
    }
    lnk.clear();
    lnk.setId(_sPortId1, lId);
    lnk.setId(_sPortId2, rId);
    if (lnk.insert(q)) {
        PDEB(INFO) << "Create new link: " << lnk.toString() << endl;
        return true;
    }
    HEREIN(em, QObject::trUtf8("Az új LLDP link kiírása sikertelen."), RS_WARNING);
    return false;
}

void cLldpScan::memo(cAppMemo em, QSqlQuery& q, int port_ix, rowData& row)
{
    QString s;
    s = QObject::trUtf8("LLDP scan, beolvasott sor %1:#%2 :").arg(pDev->getName()).arg(port_ix);
    s += "\nchoice = " + quotedString(choice);
    s += "\nname = " + quotedString(row.name);
    s += "\naddr = " + row.addr.toString();
    s += "\ndescription = " + quotedString(row.descr);
    s += "\nport name = " + quotedString(row.pname);
    s += "\nport descr. = " + quotedString(row.pdescr);
    s += "\nc. MAC = " + (row.cmac.isValid() ? row.cmac.toString() : "NULL");
    s += "\np. MAC = " + (row.pmac.isValid() ? row.cmac.toString() : "NULL");
    s += "\n" + em.getMemo();
    em.setMemo(s);
    em.insert(q);
}

// DEBUG
#define LLDP_WRITE_CSV  1

void cLldpScan::scanByLldpDevRow(QSqlQuery& q, cSnmp& snmp, int port_ix, rowData& row)
{
    _DBGFN() << " ROW : " << row.name << "(" << row.addr.toString() << "):" << row.pname << " / "
             << row.cmac.toString() << ":" << row.pmac.toString()
             << row.descr << " : " << row.pdescr << endl;
    bool f, r = false;
    int  e = 0, i, portIdSubType;
    QString portDescr, portId;
    QStringList soids;
    cAppMemo em;
    cPatch *pp = NULL, *pp2 = NULL;
    cSelect sel;
    QHostAddress a;
    cMac        portMac;
    cNPort *plp = NULL;
    QString mm;

    rDev.clear();
    rDev.bDelCollisionByIp = rDev.bDelCollisionByMac = true;    // Ütköző objektumok törlése
    rHost.clear();
    rHost.bDelCollisionByIp = rHost.bDelCollisionByMac = true;  // Ütköző objektumok törlése
    lPort.clear();
    rPort.clear();
    rPortIx = -1;

    // A típus szerinti elágazáshoz
    sel.choice(q, "lldp.descr", row.descr);
    choice = sel.getName(cSelect::ixChoice());
    lPortIx   = -1;

    // Lokális port objektum Az LLDP-ben nem biztos, hogy azonos a portok indexelése (gratula annak aki ezt kitalálta)
    // És az sem biztos, hogy a portDescr mező az interfész descriptort tartalmazza, ez a 3COM esetén az portId-ben van (brávó-brávó, gondolom létezik több elcseszett variáció is)
    soids <<  QString("LLDP-MIB::lldpLocPortDesc.%1")     .arg(port_ix);
    soids <<  QString("LLDP-MIB::lldpLocPortIdSubtype.%1").arg(port_ix);
    soids <<  QString("LLDP-MIB::lldpLocPortId.%1")       .arg(port_ix);

    a = pDev->getIpAddress();
    snmp.open(a.toString(), pDev->getName(_sCommunityRd), pDev->getId(_sSnmpVer));
    e = snmp.get(soids);
    if  (e) {
        HEREIN(em, QObject::trUtf8("A %1 LLDP indexű lokális port sikertelen azonosítás: snmp error #%2.").arg(port_ix).arg(e), RS_WARNING);
        goto scanByLldpDevRow_error;
    }
    snmp.first();
    portDescr       = snmp.value().toString();
    snmp.next();
    portIdSubType   = snmp.value().toInt();
    snmp.next();
    switch (portIdSubType) {
    case 1: // interfaceAlias
    case 2: // portComponent
    case 4: // networkAddress
    case 6: // agentCircuitId
    default:
        portMac.clear();
        portId.clear();
        break;
    case 7: // local
        lPortIx = snmp.value().toInt(&f);
        if (!f) lPortIx = -3;
        break;
    case 3: // macAddress
        portMac.set(snmp.value());
        break;
    case 5: // interfaceName :? ifDescr
        portId  = snmp.value().toString();
        break;
    }

    if (lPortIx >= 0) {
        cNPort *pnp = pDev->ports.get(_sPortIndex, lPortIx, EX_IGNORE);
        if (pnp == NULL && 0 != pnp->chkObjType<cInterface>(EX_IGNORE)) {
            lPortIx = -3;   // nem ok
        }
        else {
            plp = pnp;
        }
    }
    else for (i = 0; i < pDev->ports.size(); ++i) {
        cNPort *pnp = pDev->ports.at(i);
        if (0 != pnp->chkObjType<cInterface>(EX_IGNORE)) continue;
        cInterface *pif = dynamic_cast<cInterface *>(pnp);
        QString name = pif->getName();
        cMac    pmac = pif->getMac(_sHwAddress);
        r = name == portDescr;
        r = r || (portId.isEmpty() == false && portId  == name);
        r = r || (portMac.isValid()         && portMac == pmac);
        if (r) {
            if (lPortIx < 0) {  // Volt találat
                lPortIx = pif->getId(_sPortIndex);
                plp = pnp;
            }
            else {              // Több találat nem lehet !!!
                lPortIx = -2;
                break;
            }
        }
    }
    if (lPortIx < 0) {
        switch (lPortIx) {
        case -1:
            HEREIN(em, QObject::trUtf8("A %1 LLDP indexű lokális port sikertelen azonosítás, nincs találat. portDescr = %2, portId = %3, portMac = %4.").arg(port_ix).arg(portDescr, portId, portMac.toString()), RS_WARNING);
            break;
        case -2:
            HEREIN(em, QObject::trUtf8("A %1 LLDP indexű lokális port sikertelen azonosítás, nem egyértelmű. portDescr = %2, portId = %3, portMac = %4.").arg(port_ix).arg(portDescr, portId, portMac.toString()), RS_WARNING);
            break;
        case -3:
            HEREIN(em, QObject::trUtf8("A %1 LLDP indexű lokális port sikertelen azonosítás, a lokális %2 index nem értelmezhető, vagy hihető").arg(port_ix).arg(snmp.value().toString()), RS_WARNING);
            break;
        }
        goto scanByLldpDevRow_error;
    }
    lPort.clone(*plp);

    // A távoli eszköz címe megvan?
    if (row.addr.isNull()) {
        if (0 == choice.compare(_sEmpty, Qt::CaseInsensitive)) {
            r = rowEmpty(q, snmp, row, em);
            r = r && updateLink(em);
            if (r) return;
        }
        else {
            HEREIN(em, QObject::trUtf8("Hiányzik, vagy nem értelmezhető a távoli eszköz IP címe."), RS_WARNING);
        }
        goto scanByLldpDevRow_error;
    }
    // Rákeresünk az adatbázisban a címek alapján
    rIpPort.fetchByIp(q, row.addr);
    f =      (row.cmac.isValid() && 0 < rMacPort.fetchByMac(q, row.cmac));
    f = f || (row.pmac.isValid() && 0 < rMacPort.fetchByMac(q, row.pmac));
    if (f || !rIpPort.isEmpty()) {  // Exist
        qlonglong nid;
        qlonglong nidByIp  = rIpPort.getId(_sNodeId);
        qlonglong nidByMac = rMacPort.getId(_sNodeId);
        if (nidByIp == nidByMac) {      // Ugyan azt találtuk a címek alapján, öröm boldogság!
            nid = nidByMac;
            pp  = cPatch::getNodeObjById(q, nid, EX_ERROR);
        }
        else if (nidByIp == NULL_ID) {  // Nem találtuk IP alapján
            nid = nidByMac;
            pp = cPatch::getNodeObjById(q, nid, EX_ERROR);
            HEREIN(em, QObject::trUtf8("A %1 indexű lokális porthoz tartozó eszköz MAC alapján :\n%2\nIp cím alapján nics találat")
                   .arg(port_ix).arg(pp->identifying()), RS_WARNING);
            memo(em, q, port_ix, row);
        }
        else if (nidByMac == NULL_ID) { // Nem találtuk a cím alapján
            nid = nidByIp;
            pp = cPatch::getNodeObjById(q, nid, EX_ERROR);
            if (choice.compare(_s3Com, Qt::CaseInsensitive) != 0) { // 3Com-nal egy fiktív MAC-ot ad, amire nem lessz találat.
                HEREIN(em, QObject::trUtf8("A %1 indexű lokális porthoz tartozó eszköz IP cím alapján :\n%2\nMAC alapján nics találat")
                       .arg(port_ix).arg(pp->identifying()), RS_WARNING);
                memo(em, q, port_ix, row);
            }
        }
        else {                          // Két különböző eszköz lett azonosítva
            nid = nidByMac;     // Ezt hisszük el.
            pp  = cPatch::getNodeObjById(q, nid, EX_ERROR);
            pp2 = cPatch::getNodeObjById(q, nidByIp, EX_ERROR);
            rIpPort.fetchAddressess(q);
            {
                int aix = cIpAddress().toIndex(_sAddress);
                tOwnRecords<cIpAddress, cInterface>::const_iterator it = rIpPort.addresses.constBegin();
                for (; it < rIpPort.addresses.constEnd(); it++) {
                    cIpAddress *pip = *it;
                    if (pip->isNull(aix)) continue;
                    const netAddress na = pip->get(aix).value<netAddress>();
                    if (na.addr() == row.addr) {
                        pip->setId(_sIpAddressType, AT_DYNAMIC);
                        pip->clear(aix);
                        pip->update(q, false);
                        rIpPort.clear();
                        break;
                    }
                }
            }
            mm = QObject::trUtf8(
                       "A %1 indexű lokális porthoz tartozó eszköz MAC alapján :\n%2\n"
                       "Nem egyezik az Ip cím alapján talált eszközzel :\n%3\n"
                       )
                   .arg(port_ix).arg(pp->identifying()).arg(port_ix).arg(pp2->identifying());
            pDelete(pp2);
            if (rIpPort.isNull()) { // töröltük
                mm += QObject::trUtf8("Az Ip alapján talált eszközön az IP cím törölve lessz!");
                HEREIN(em, mm, RS_WARNING);
                memo(em, q, port_ix, row);
            }
            else {
                mm += QObject::trUtf8("Az Ip alapján talált eszközön az IP cím törlése sikertelen!");
                HEREIN(em, mm, RS_CRITICAL);
                goto scanByLldpDevRow_error;
            }
        }
        // Exist, and SNMP device
        if (0 == pp->chkObjType<cSnmpDevice>(EX_IGNORE)) {
            rDev.clone(*pp);
            rDev.fetchPorts(q);
            rDev.fetchParams(q);
            rHost.clone(rDev);
        }
        // Exist, and not node, patch ???!!!
        else if (0 != pp->chkObjType<cNode>(EX_IGNORE)) {
            HEREIN(em, QObject::trUtf8("A %1 indexű lokális porthoz tartozó eszköz típusa patch !?.").arg(port_ix), RS_WARNING);
            goto scanByLldpDevRow_error;
        }
        // Exist, and node.
        else {
            rHost.clone(*pp);
            rHost.fetchPorts(q);    // Interface esetén a címek és vlan-okat is beolvassa
            rHost.fetchParams(q);
        }    }

    // *** TEST ***
#if LLDP_WRITE_CSV
    {
        QFile f("lldp_test_rows.csv");
        f.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream strm(&f);
        strm << quotedString(pDev->getName()) << ";" << port_ix << ";"
             << choice << ";"
             << quotedString(row.name)  << ";" << row.addr.toString() << ";" << quotedString(row.descr) << ";"
             << quotedString(row.pname) << ";" << quotedString(row.pdescr) << ";"
             << row.cmac.toString() << ";" << row.pmac.toString()
             << endl;
        f.close();
    }
#endif

    if      (0 == choice.compare(_sProCurve,    Qt::CaseInsensitive)) r = rowProCurve(q, snmp, row, em);
    else if (0 == choice.compare(_sProCurveWeb, Qt::CaseInsensitive)) r = rowProCurveWeb(q, snmp, row, em);
    else if (0 == choice.compare(_s3Com,        Qt::CaseInsensitive)) r = row3COM(q, snmp, row, em);
    else if (0 == choice.compare(_sCisco,       Qt::CaseInsensitive)) r = rowCisco(q, snmp, row, em);
    else if (0 == choice.compare(_sHPAPC,       Qt::CaseInsensitive)) r = rowHPAPC(q, snmp, row, em);
    else if (0 == choice.compare(_sLinux,       Qt::CaseInsensitive)) r = rowLinux(q, snmp, row, em);
    else {
        HEREIN(em, QObject::trUtf8("A távoli eszköz '%1' descriptora alapján az adatfeldogozás módja nem ismert.").arg(row.descr), RS_WARNING);
        r = false;
    }
    if (!r) goto scanByLldpDevRow_error;
    if (!updateLink(em)) goto scanByLldpDevRow_error;
    return;
scanByLldpDevRow_error:
    memo(em, q, port_ix, row);
    pDelete(pp);
    pDelete(pp2);
    return;
}

inline int snmpNextField(int n, cSnmp& snmp, QString& em, int& _ix)
{
    const netsnmp_variable_list *p;
    p = (n == 0) ? snmp.first() : snmp.next();
    if (NULL == p) {
        em = QObject::trUtf8("No SNMP data #%1, OID:%2; %3").arg(n).arg(cLldpScan::sOids[n], snmp.name().toString());
        return -1;
    }
    if (!(cLldpScan::oids[n] < snmp.name())) {
        if (n == 0) return  0;  // End off table
        else {
            em = QObject::trUtf8("Truncated row #%1, OID: %2  awaited port id = %3").arg(n).arg(snmp.name().toString()).arg(_ix);
            return -1;
        }
    }
    int ix = (int)snmp.name()[cLldpScan::oids[n].size() +1];
    if (n == 0) _ix = ix;   // First column set row id (port index)
    else if (_ix != ix) {   // Check row id
        em = QObject::trUtf8("scrambled data #%1, OID: %2  awaited port id = %3").arg(n).arg(snmp.name().toString()).arg(_ix);
        return -1;
    }
    return 1;   //OK
}

#define NEXTSNMPDATA(n) { \
                            int rr = snmpNextField(n, snmp, em, index); \
                            if      (rr ==  0) break; \
                            else if (rr == -1) goto scanByLldpDev_error; \
                        }

inline int snmpGetNext(cSnmp& snmp, bool first, cOIdVector& oids)
{
    if (first) {    // First row
        return snmp.getNext(oids);
    }
    else {          // Next row
        return snmp.getNext();
    }
}

void cLldpScan::scanByLldpDev(QSqlQuery& q)
{
    int         RemChassisIdSubtype;
    int         RemPortIdSubtype;
    QVariant    RemPortId;
    QString     RemPortDesc;
    QString     RemSysDesc;
    QString     RemSysName;
    QVariant    RemChasisId;
    QString     em, s;
    cMac        mac;
    int r, index;
    cSnmp snmp;
    isBreakImportParser(parser);
    pDev->fetchPorts(q);
    pDev->open(q, snmp);
    // ----- Egy eszköz lekérdezése
    rRows.clear();
    bool first = true;
    while(1) {
        // SNMP Get Next
        r = snmpGetNext(snmp, first, oids);
        if (r) {
            em = QObject::trUtf8("SNMP get-%1 (%2) error #%3, '%4'")
                      .arg(first ? "first" : "next")
                      .arg(sOids.join(_sCommaSp))
                      .arg(r).arg(snmp.emsg);
            goto scanByLldpDev_error;
        }
        first = false;
        // Interpret ...
        // Unfortunately, each manufacturer is sending different information.
        // The clear numeric identifier of a remote port no information.
        // Interpretation of the data was based on the available tools:
        // ProCurve switch, HPE (3Com) switch, HP Access Point, Cisco router switch.
        NEXTSNMPDATA(LLDP_REM_CID_TYPE);        // first : set index : local port index
        RemChassisIdSubtype = snmp.value().toInt(); // lldpRemChassisIdSubtype
        NEXTSNMPDATA(LLDP_REM_CID);
        RemChasisId = snmp.value();                 // lldpRemChassisId
        NEXTSNMPDATA(LLDP_REM_PID_TYPE);
        RemPortIdSubtype = snmp.value().toInt();    // lldpRemPortIdSubtype
        NEXTSNMPDATA(LLDP_REM_PID);
        RemPortId = snmp.value();                   // lldpRemPortId
        NEXTSNMPDATA(LLDP_REM_PDSC);
        RemPortDesc = snmp.value().toString();      // lldpRemPortDesc
        NEXTSNMPDATA(LLDP_REM_SDSC);
        RemSysDesc  = snmp.value().toString();      // lldpRemSysDesc
        NEXTSNMPDATA(LLDP_REM_SNAME);
        RemSysName  = snmp.value().toString();      // lldpRemSysName

#if LLDP_WRITE_CSV
        {
            const QString sep = ";";
            QFile f("lldp_test_gets.csv");
            f.open(QIODevice::WriteOnly | QIODevice::Append);
            QTextStream strm(&f);
            strm << quotedString(pDev->getName()) << sep << index << sep
                 << RemChassisIdSubtype << sep
                 << quotedString(RemChasisId.toString()) << sep << dump(RemChasisId.toByteArray()) << sep
                 << RemPortIdSubtype << sep
                 << quotedString(RemPortId.toString()) << sep << dump(RemPortId.toByteArray()) << sep
                 << quotedString(RemPortDesc) << sep
                 << quotedString(RemSysDesc) << sep
                 << quotedString(RemSysName) << endl;
            f.close();
        }
#endif

        if (!RemSysName.isEmpty()) {
            if (!rRows[index].name.isEmpty() && rRows[index].name != RemSysName) {
                s = QObject::trUtf8("LLDP:\nA %1 eszköz %2 indexű portján a távoli eszköz neve nem egyértelmű : %3, %4")
                        .arg(pDev->getName()).arg(index).arg(rRows[index].name, RemSysName);
                APPMEMO(q, s, RS_WARNING);
            }
            rRows[index].name = RemSysName;
        }

        if (!RemSysDesc.isEmpty()) {
            if (rRows[index].descr.isEmpty()) rRows[index].descr  =       RemSysDesc;
            else                              rRows[index].descr += " " + RemSysDesc;
        }

        if (!RemPortDesc.isEmpty()) {
            if (!rRows[index].pdescr.isEmpty() && rRows[index].pdescr != RemPortDesc) {
                s = QObject::trUtf8("LLDP:\nA %1 eszköz %2 indexű portján a távoli eszköz port descripzor nem egyértelmű : %3, %4")
                        .arg(pDev->getName()).arg(index).arg(rRows[index].pdescr, RemPortDesc);
                APPMEMO(q, s, RS_WARNING);
            }
            rRows[index].pdescr = RemPortDesc;
        }

        /* - */
        switch (RemChassisIdSubtype) {
        case 1: // chassisComponent
        case 2: // interfaceAlias
        case 3: // portComponent
            break;  // ?
        case 4: // macAddress
            mac.set(RemChasisId.toByteArray());
            if (mac) rRows[index].cmac = mac;
            break;
        case 5: // networkAddress
            break;  // ?
        case 6: // interfaceName    Just guessing. :(
            if (rRows[index].pname.isEmpty()) {
                s = RemChasisId.toString();
                if (!s.isEmpty()) rRows[index].pname = s;
            }
            break;
        case 7: // local
            s = RemChasisId.toString();
            if (!s.isEmpty()) {
                rRows[index].clocal   = s;
            }
            break;
        }
        /* - */
        switch (RemPortIdSubtype) {
        case 1: // interfaceAlias
        case 2: // portComponent
            break;  // ?
        case 3: // macAddress
            mac.set(RemPortId.toByteArray());
            if (mac) rRows[index].pmac = mac;
            break;
        case 4: // networkAddress
            break;  // ?
        case 6: // agentCircuitId
            break;  // ?
        case 5: // interfaceName
        case 7: // local
            s = RemPortId.toString();
            if (!s.isEmpty()) rRows[index].pname = s;
            break;
        }
        /* - */
        continue;
scanByLldpDev_error:
        {
            QList<QHostAddress> al = pDev->fetchAllIpAddress(q);
            QString sa;
            if (al.isEmpty()) sa = QObject::trUtf8("Nincs IP cím");
            else {
                foreach (QHostAddress a, al) {
                    sa += a.toString() + ", ";
                }
                sa.chop(2);
            }
            QString ln = QObject::trUtf8("\nLocal node %1 (%2)").arg(pDev->getName()).arg(sa);
            QString line = "\nSNMP LINE : ";
            snmp.first();   line += snmp.name().toString() + ", ";
            snmp.next();    line += snmp.name().toNumString() + ", ";
            snmp.next();    line += snmp.name().toString() + " = " + snmp.value().toString() + ", ";
            snmp.next();    line += snmp.name().toNumString() + "\n";
            em += ln + line;
            APPMEMO(q, em, RS_CRITICAL);
            DERR() << em << endl;
        }
        return;
    }
    /* IP */
    static cOIdVector aoi;
    cOId o;
    if (aoi.isEmpty()) aoi << *pAddrOid;
    first = true;
    while (1) {
        r = snmpGetNext(snmp, first, aoi);
        if (r) {
            em = QObject::trUtf8("SNMP get - %1(%2) error #%3, '4')")
                      .arg(pDev->toString())
                      .arg(first ? "first" : "next")
                      .arg(sAddrOid)
                      .arg(r);
            if (!first) {
                PDEB(WARNING) << pDev->getName() << " : " << em << endl;
                break;
            }
            goto scanByLldpDev_errorA;
        }
        first = false;
        /* - */
        if (NULL == snmp.first()) {
            em = QObject::trUtf8("No SNMP data, OID:%1; %2").arg(sAddrOid, snmp.name().toString());
            goto scanByLldpDev_errorA;
        }
        if (!(*pAddrOid < snmp.name())) {
            break;  // End
        }
        /* - */
        o = snmp.name();
        o -= *pAddrOid;
        if (o.size() < 2) continue;
        index = (int)o[1];
        // IPV4 !!
        if (o.size() == 9 && o[3] == 1 && o[4] == 4) {   // ?.index.?.1.4.IPV4
            QHostAddress a = o.toIPV4();
            if (!a.isNull()) rRows[index].addr = a;
        }
        // IPV6 ????
        continue;
scanByLldpDev_errorA:
        {
            QList<QHostAddress> al = pDev->fetchAllIpAddress(q);
            QString sa;
            if (al.isEmpty()) sa = QObject::trUtf8("Nincs IP cím");
            else {
                foreach (QHostAddress a, al) {
                    sa += a.toString() + ", ";
                }
                sa.chop(2);
            }
            QString ln = QObject::trUtf8("\nLocal node %1 (%2)").arg(pDev->getName()).arg(sa);
            em += ln;
            APPMEMO(q, em, RS_CRITICAL);
            DERR() << em << endl;
        }
        return;
    }
    /* ************************************************ */
    if (rRows.isEmpty()) return;
    foreach (index, rRows.keys()) {
        scanByLldpDevRow(q, snmp, index, rRows[index]);
    }
}

void cLldpScan::scanByLldp(QSqlQuery& q, const cSnmpDevice& __dev)
{
    queued.clear();
    scanned.clear();
    queued << __dev;
    while (queued.size() > 0) {     // Amíg van mit lekérdezni
        pDev = queued.pop_back();
        if (!scanned.contains(pDev->getId())) { // Ha már lekérdeztuk, akkor eldobjuk
            scanByLldpDev(q);        // Lekérdezés
            scanned << pDev;        // Lekérdezve
        }
        else delete pDev;
    }
}

bool cLldpScan::setRPortFromIx(cAppMemo &em)
{
    cNPort *pp = rDev.ports.get(_sPortIndex, rPortIx, EX_IGNORE);
    if (pp == NULL) {
        HEREIN(em, QObject::trUtf8("Nincs %1 indexű távoli port.").arg(rPortIx), RS_WARNING);
        return false;
    }
    if (pp->chkObjType<cInterface>(EX_IGNORE)) {
        HEREIN(em, QObject::trUtf8("A %1 indexű távoli port %2 típusa nem megfelelő.").arg(rPortIx).arg(typeid(*pp).name()), RS_WARNING);
        return false;
    }
    rPort.clone(*pp);
    if (rPort.getId() == NULL_ID) {
        EXCEPTION(EPROGFAIL);
    }
    return true;
}

bool cLldpScan::setRPortFromMac(rowData &row, cAppMemo &em)
{
    if (0 == rHost.ports.size() && 0 == rHost.fetchPorts(q)) {
        HEREIN(em, QObject::trUtf8("Talált, és regisztrált '%1'-nek nincs egy portja sem.").arg(row.name), RS_WARNING);
        return false;
    }
    for (int i = 0; i < rHost.ports.size(); ++i) {
        cNPort *pp = rHost.ports.at(i);
        if (!pp->isIfType(_sEthernet)) continue;
        if (pp->chkObjType<cInterface>(EX_IGNORE)) continue;
        cInterface *pi = pp->reconvert<cInterface>();
        if (pi->mac() == row.cmac || pi->mac() == row.pmac) {
            rPort.clone(*pi);
            return true;
        }
    }
    HEREIN(em, QObject::trUtf8("Talált, és regisztrált '%1'-nek nincs megfelelő portja %2 címmel.").arg(row.name, row.cmac.toString()), RS_WARNING);
    return false;
}

int cLldpScan::portDescr2Ix(QSqlQuery &q, cSnmp &snmp, QHostAddress ha, const QString& pdescr, cAppMemo& em)
{
    _DBGFN() << "@(,," << ha.toString() << ", " << pdescr << ",)" << endl;
    (void)q;
    if (pdescr.isEmpty()) {
        HEREIN(em, QObject::trUtf8("Hiányzik a port azonosító név."), RS_CRITICAL);
        DWAR() << em.getMemo() << endl;
        return -1;
    }
    int r = snmp.open(ha.toString());
    if (r) {
        HEREIN(em, QObject::trUtf8("Port index keresése. Az SNMP megnyitás sikertelen #%1").arg(r), RS_WARNING);
        DWAR() << em.getMemo() << endl;
        return -1;
    }
    cOId oid(_sIfMib + _sIfDescr);
    if (oid.isEmpty()) EXCEPTION(ESNMP,0,_sIfMib + _sIfDescr);
    r = snmp.getNext(oid);
    if (r) {
        HEREIN(em, QObject::trUtf8("Port index keresése. Sikertelen SNMP (first) lekérdezés #%1").arg(r), RS_WARNING);
        DWAR() << em.getMemo() << endl;
        return -1;
    }
    do {
        if (NULL == snmp.first()) {
            DWAR() << em.getMemo() << endl;
            HEREIN(em, QObject::trUtf8("Port index keresése. No SNMP data"), RS_WARNING);
            return -1;
        }
        cOId o = snmp.name();
        PDEB(VVERBOSE) << o.toString() << " = " << debVariantToString(snmp.value()) << endl;
        if (!(oid < o)) {
            HEREIN(em, QObject::trUtf8("Port index keresése. Nincs %1 nevű port.").arg(pdescr), RS_WARNING);
            DWAR() << em.getMemo() << endl;
            return -1;
        }
        /* - */
        o -= oid;
        if (o.size() >= 1) {
            r = (int)o[0];
            QString n = snmp.value().toString();
            PDEB(VVERBOSE) << "#" << r << "; compare : " << quotedString(n) << " == " << quotedString(pdescr) << endl;
            if (n == pdescr) return r;
        }
        else {
            DWAR() << "Invalid OID size : " << o.toString() << " , host IP : " << ha.toString() << endl;
        }
        r = snmp.getNext();
        if (r) {
            HEREIN(em, QObject::trUtf8("Port index keresése. Sikertelen SNMP (next) lekérdezés #%1").arg(r), RS_WARNING);
            DWAR() << em.getMemo() << endl;
            return -1;
        }
    } while (true);
}

bool cLldpScan::rowTail(QSqlQuery &q, const QString& ma, const QStringPair& ip, cAppMemo& em, bool exists)
{
    if (!exists) {
        rDev.asmbNode(q, _sLOOKUP, NULL, &ip, &ma, _sNul, NULL_ID, EX_IGNORE);
        rDev.setNote(QObject::trUtf8("By LLDP scan."));
    }
    QString es;
    if (!rDev.setBySnmp(_sNul, EX_IGNORE, &es)) {
        HEREIN(em, QObject::trUtf8("A %1 MAC és %2 IP című eszköz SNMP felderítése sikertelen :\n%3\n").arg(ma, ip.first, es), RS_WARNING);
        return false;
    }

    cError *pe = NULL;
    if (exists) {
        rDev.containerValid = CV_PORTS | CV_PORTS_ADDRESSES;
        pe = rDev.tryRewrite(q, true);
    }
    else {
        pe = rDev.tryInsert(q);
    }
    if (pe != NULL) {
        QString es = exists ? QObject::trUtf8("Modosítása") : QObject::trUtf8("Mentése");
        HEREIN(em, QObject::trUtf8("A %1 MAC és %2 IP című SNMP felderített eszköz \n%3\n %4 sikertelen :\n%5\n")
                .arg(ma, ip.first, rDev.toString(), es, pe->msg()), RS_WARNING);
        pDelete(pe);
        return false;
    }
    rHost.clone(rDev);
    queued << rDev;
    return setRPortFromIx(em);;
}

/// ProCurwe switch management
bool cLldpScan::rowProCurve(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em)
{
    bool ok;
    bool exists;
    QStringPair ip;
    QString     ma;
    (void)snmp;

    if (!row.cmac.isValid()) {
        HEREIN(em, QObject::trUtf8("Hiányzik az eszköz azonosító MAC cím."), RS_WARNING);
        return false;
    }
    rPortIx = row.pname.toInt(&ok);         // A port név = index
    if (!ok) {
        rPortIx = row.pdescr.toInt(&ok);    // vagy port descr = index (pl.: 1820-Gx)
        if (!ok) {
            HEREIN(em, QObject::trUtf8("A '%1' nem értelmezhető port indexként.").arg(row.pname), RS_WARNING);
            return false;
        }
    }
    exists = !rHost.isEmpty();
    if (exists) {
        qlonglong id = rDev.getId();
        if (id == NULL_ID) {
            HEREIN(em, QObject::trUtf8("A távoli IP-hez tartozó node létezik, de nem egy SNMP eszköz."), RS_WARNING);
            return false;
        }
        // Frissítettük már ?
        if (queued.contains(id) || scanned.contains(id)) return setRPortFromIx(em);
        // Update
    }
    else {
        rDev.setId(_sNodeType, enum2set(NT_HOST, NT_SWITCH, NT_SNMP));
    }
    // update or new
    ma = row.cmac.toString();
    ip.first  = row.addr.toString();
    ip.second = _sFixIp;
    return rowTail(q, ma, ip, em, exists);
}

/// ProCurwe switch WEB management
bool cLldpScan::rowProCurveWeb(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em)
{
    bool exists;
    QStringPair ip;
    QString     ma;
    cNPort     *pp;

    if (!row.cmac.isValid()) {
        HEREIN(em, QObject::trUtf8("Hiányzik az eszköz azonosító MAC cím."), RS_WARNING);
        return false;
    }
    if (row.pname.isEmpty()) {
        HEREIN(em, QObject::trUtf8("Hiányzik a távoli port azonosító név cím."), RS_WARNING);
        return false;
    }
    exists = !rHost.isEmpty();
    if (exists) {
        if (rDev.isEmpty()) {
            HEREIN(em, QObject::trUtf8("A távoli IP-hez tartozó node létezik, de nem egy SNMP eszköz."), RS_WARNING);
            return false;
        }
        pp = rHost.ports.get(_sPortName, row.pdescr, EX_IGNORE);
        if (pp == NULL) {
            HEREIN(em, QObject::trUtf8("Nincs %1 nevű távoli port.").arg(row.pname), RS_WARNING);
            return false;
        }
        rPortIx = pp->getId(_sPortIndex);
        // Frissítettük már ?
        if (queued.contains(rDev.getId()) || scanned.contains(rDev.getId())) return setRPortFromIx(em);
    }
    else {
        rPortIx = portDescr2Ix(q, snmp, row.addr, row.pdescr, em);
        if (rPortIx < 0) return false;
        rDev.setId(_sNodeType, enum2set(NT_HOST, NT_SWITCH, NT_SNMP));
    }
    ma = row.cmac.toString();
    ip.first  = row.addr.toString();
    ip.second = _sFixIp;
    return rowTail(q, ma, ip, em, exists);
}

bool cLldpScan::row3COM(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em)
{
    bool exists;
    QStringPair ip;
    QString     ma;
    cNPort     *pp;

    if (!row.cmac.isValid()) {
        HEREIN(em, QObject::trUtf8("Hiányzik az eszköz azonosító MAC cím."), RS_WARNING);
        return false;
    }
    if (row.pname.isEmpty()) {
        HEREIN(em, QObject::trUtf8("Hiányzik a távoli port azonosító név."), RS_WARNING);
        return false;
    }
    exists = !rHost.isEmpty();
    if (exists) {
        if (rDev.isEmpty()) {
            HEREIN(em, QObject::trUtf8("A távoli IP-hez tartozó node létezik, de nem egy SNMP eszköz."), RS_WARNING);
            return false;
        }
        pp = rHost.ports.get(_sPortName, row.pname, EX_IGNORE);
        if (pp == NULL) {
            HEREIN(em, QObject::trUtf8("Nincs %1 nevű távoli port.").arg(row.pname), RS_WARNING);
            return false;
        }
        rPortIx = pp->getId(_sPortIndex);
        // Frissítettük már ?
        if (queued.contains(rDev.getId()) || scanned.contains(rDev.getId())) return setRPortFromIx(em);
        // Nincs újra felderítés, mért hiányozni fognak a trunk adatok !!!
        queued << rDev;
        return setRPortFromIx(em);
    }
    else {
        rPortIx = -1;
        if (!row.pdescr.isEmpty()) {
            rPortIx = portDescr2Ix(q, snmp, row.addr, row.pdescr, em);
        }
        if (rPortIx < 0 && !row.pname.isEmpty()) {
            rPortIx = portDescr2Ix(q, snmp, row.addr, row.pname, em);
        }
        // Lehetne keresni a MAC alapján is, ha van ???
        if (rPortIx < 0) {
            return false;
        }
        rDev.setId(_sNodeType, enum2set(NT_HOST, NT_SWITCH, NT_SNMP));
    }
    ma = _sARP; // row.cmac.toString(); // az eszköz MAC nem jelenik meg sehol, men azonos a port MAC-al
    ip.first  = row.addr.toString();
    ip.second = _sFixIp;
    return rowTail(q, ma, ip, em, exists);
}

bool cLldpScan::rowCisco(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em)
{
    bool exists;
    QStringPair ip;
    QString     ma;
    cNPort     *pp;

    // MAC nincs
    if (row.pname.isEmpty()) {
        HEREIN(em, QObject::trUtf8("Hiányzik a távoli port azonosító név."), RS_WARNING);
        return false;
    }
    exists = !rHost.isEmpty();
    if (exists) {
        if (rDev.isEmpty()) {
            HEREIN(em, QObject::trUtf8("A távoli IP-hez tartozó node létezik, de nem egy SNMP eszköz."), RS_WARNING);
            return false;
        }
        pp = rHost.ports.get(_sPortName, row.pname, EX_IGNORE);
        if (pp == NULL) {
            HEREIN(em, QObject::trUtf8("Nincs %1 nevű távoli port.").arg(row.pname), RS_WARNING);
            return false;
        }
        rPortIx = pp->getId(_sPortIndex);
        // Frissítettük már ?
        if (queued.contains(rDev.getId()) || scanned.contains(rDev.getId())) return setRPortFromIx(em);
        // Nincs újra felderítés, mért hiányozni fognak a trunk adatok !!!
        queued << rDev;
        return setRPortFromIx(em);
    }
    else {
        QString name = row.pname;
        rPortIx = portDescr2Ix(q, snmp, row.addr, name, em);
        if (rPortIx < 0) return false;
    }
    ma = _sARP;
    ip.first  = row.addr.toString();
    ip.second = _sFixIp;
    return rowTail(q, ma, ip, em, exists);
}

bool cLldpScan::rowHPAPC(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em)
{
    (void)snmp;
    if (!(row.pmac.isValid() && row.cmac.isValid())) {
        HEREIN(em, QObject::trUtf8("Hiányzik a távoli port vagy eszköz azonosító."), RS_WARNING);
        return false;
    }
    bool exists = !rHost.isEmpty();
    if (exists) {
        if (rHost.getName().compare(row.name, Qt::CaseInsensitive) == 0 || rHost.getName().startsWith(row.name + ".")) {
            return setRPortFromMac(row, em);
        }
        HEREIN(em, QObject::trUtf8("Név ütjözés. Talált '%1', azonos címmekkel bejegyzett '%2'. Az eszköz át lessz nevezve.").arg(row.name, rHost.getName()), RS_WARNING);
        rHost.setName(row.name);
        rHost.update(q, false, _bit(rHost.nameIndex()));
        return setRPortFromMac(row, em);
    }

    QString n = row.name;
    if (n.isEmpty()) n = "node-" + row.cmac.toString().remove(char(':'));
    rHost.clear();
    rDev.clear();

    if (rHost.fetchByName(n)) { // Név szerint sincs ?
        HEREIN(em, QObject::trUtf8("Név ütjözés. A '%1', létezik, nem a  '%2' címmel.").arg(n, row.addr.toString()), RS_WARNING);
        return false;
    }

    rHost.setName(n);
    rHost.setName(_sNodeNote, row.descr);
    rHost.addPort(_sEthernet, row.pdescr, row.pname, 1);
    cInterface *pi = rHost.ports.first()->reconvert<cInterface>();
    *pi = row.cmac;
    pi->addIpAddress(row.addr, cDynAddrRange::isDynamic(q, row.addr), "By LLDP");
    rHost.setId(_sNodeType, enum2set(NT_HOST, NT_AP));
    cError *pe = rHost.tryInsert(q);
    if (pe != NULL) {
        HEREIN(em, QObject::trUtf8("A %1 MAC és %2 IP című felderített eszköz \n%3\n Kiírása sikertelen :\n%4\n")
                .arg(row.pmac, row.addr.toString(), rHost.toString(), pe->msg()), RS_WARNING);
        pDelete(pe);
        return false;
    }
    if (row.clocal.isEmpty() == false) {   // SN
        QString oldSn = execSqlTextFunction(q, "get_str_node_param", rHost.getId(), _sSerialNumber);
        if (oldSn.isEmpty()) {
            execSqlFunction(q, "set_str_node_param", rHost.getId(), row.clocal, _sSerialNumber);
        }
    }
    rPort.clone(*pi);
    return true;
}

bool cLldpScan::rowLinux(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em)
{
    cNPort     *pp;
    (void)snmp;
    if (!(row.pmac.isValid() && row.cmac.isValid())) {
        HEREIN(em, QObject::trUtf8("Hiányzik a távoli port vagy eszköz azonosító MAC."), RS_WARNING);
        return false;
    }
    bool exists = !rHost.isEmpty();
    if (exists) {
        if (rHost.getName().compare(row.name, Qt::CaseInsensitive) == 0 || rHost.getName().startsWith(row.name + ".")) {
            pp = rHost.ports.get(_sPortName, row.pdescr, EX_IGNORE);
            if (pp == NULL) {
                HEREIN(em, QObject::trUtf8("Nincs %1 nevű távoli port.").arg(row.pdescr), RS_WARNING);
                return false;
            }
            rPortIx = pp->getId(_sPortIndex);
            // A MAC alapján nem biztos, hogy azonosítható
            if (pp->chkObjType<cInterface>(EX_IGNORE)) {
                HEREIN(em, QObject::trUtf8("A %1 indexű távoli port %2 típusa nem megfelelő.").arg(rPortIx).arg(typeid(*pp).name()), RS_WARNING);
                return false;
            }
            rPort.clone(*pp->reconvert<cInterface>());
            return true;
        }
        HEREIN(em, QObject::trUtf8("Név ütjözés. Talált '%1', azonos címmel bejegyzett '%2'").arg(row.name, rHost.getName()), RS_WARNING);
        return false;
    }

    QString n = row.name;
    if (n.isEmpty()) n = "linux-" + row.cmac.toString().remove(char(':'));
    rHost.clear();
    rDev.clear();

    rHost.setName(n);
    rHost.setName(_sNodeNote, row.descr);
    rHost.addPort(_sEthernet, row.pdescr, row.pname, 1);
    cInterface *pi = rHost.ports.first()->reconvert<cInterface>();
    *pi = row.cmac;
    rDev.setId(_sNodeType, enum2set(NT_HOST));
    pi->addIpAddress(row.addr, cDynAddrRange::isDynamic(q, row.addr), "By LLDP");
    cError *pe = rHost.tryInsert(q);
    if (pe != NULL) {
        HEREIN(em, QObject::trUtf8("A %1 MAC és %2 IP című felderített eszköz \n%3\n Kiírása sikertelen :\n%4\n")
                .arg(row.pmac, row.addr.toString(), rHost.toString(), pe->msg()), RS_WARNING);
        pDelete(pe);
        return false;
    }
    rPort.clone(*pi);
    return true;
}

/// Csak egy MAC címunk van, se IP, se típus azonosító
/// Windows??
bool cLldpScan::rowEmpty(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em)
{
    (void)snmp;
    cMac mac = row.pmac;
    if (!mac) mac = row.cmac;
    if (!mac)  {
        HEREIN(em, QObject::trUtf8("Hiányzik a távoli port vagy eszköz azonosító MAC."), RS_WARNING);
        return false;
    }
    // Keresünk egy portot az egy szem MAC alapján
    int r = rPort.fetchByMac(q, mac);
    if (r == 1) {   // eggyetlen találat a jó válasz
        rHost.fetchById(rPort.getId(_sNodeId));
        rDev.clear();
        return setRPortFromMac(row, em);
    }
    if (r > 1)  {
        HEREIN(em, QObject::trUtf8("A %1 MAC nem azonosít egyértelmüen egy portot.").arg(mac.toString()), RS_WARNING);
        return false;
    }
    // Nincs ilyen node, keresünk egy IP címet az ARP lekérdezésekben
    cArp arp;
    QList<QHostAddress> al = arp.mac2ips(q, mac);
    r = al.size();
    if (r != 1) {   // Itt is csak egy találat a jó
        if (r) {    // egynél több
            QString s;
            foreach (QHostAddress a, al) {
                s += a.toString() + ", ";
            }
            s.chop(2);
            HEREIN(em, QObject::trUtf8("A %1 MAC címhez nem azonosítható egyértelmüen IP cím (%2).").arg(mac.toString(),s), RS_WARNING);
        }
        else {  // nincs találat
            HEREIN(em, QObject::trUtf8("A %1 MAC címhez nem azonosítható IP cím.").arg(mac.toString()), RS_WARNING);
        }
        return false;
    }
    // Van egy MAC, egy IP. Keresünk hozzá egy nevet.
    QHostAddress a = al.first();
    QHostInfo hi = QHostInfo::fromName(a.toString());
    QString name = hi.hostName();
    if (name == a.toString()) name = "host-" + mac.toString();

    rHost.setName(name);
    QString note = QObject::trUtf8("LLDP auto %1").arg(QDateTime::currentDateTime().toString());
    rHost.setNote(note);
    rHost.setId(_sNodeType, enum2set(NT_HOST, NT_WORKSTATION));
    rHost.addPort(_sEthernet, _sEthernet, _sEthernet, 1);
    cInterface *pi = rHost.ports.first()->reconvert<cInterface>();
    *pi = row.cmac;
    rDev.setId(_sNodeType, enum2set(NT_HOST));
    pi->addIpAddress(a, cDynAddrRange::isDynamic(q, a), note);
    cError *pe = rHost.tryInsert(q);
    if (pe != NULL) {
        HEREIN(em, QObject::trUtf8("A %1 MAC és %2 IP című felderített eszköz \n%3\n Kiírása sikertelen :\n%4\n")
                .arg(row.pmac, a.toString(), rHost.toString(), pe->msg()), RS_WARNING);
        pDelete(pe);
        return false;
    }
    rPort.clone(*pi);
    return true;
}

void scanByLldp(QSqlQuery q, const cSnmpDevice& __dev, bool _parser)
{
    cLldpScan(q, _parser).scanByLldp(q, __dev);
}

void lldpInfo(QSqlQuery q, const cSnmpDevice& __dev, bool _parser)
{
    cLldpScan o(q, _parser);
    cSnmpDevice *pDev = dynamic_cast<cSnmpDevice *>(__dev.dup());
    if (pDev->ports.isEmpty()) pDev->fetchPorts(q);
    o.pDev = pDev;
    o.scanByLldpDev(q);
    delete pDev;
}

#else
#error "Invalid projects file ?"
#endif // MUST_SCAN

