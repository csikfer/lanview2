
#include "scan.h"

#ifdef MUST_SCAN
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
    cInterface *pTempIf = NULL;
    QHostAddress hostAddr;
    // A gyátrói baromságok kezeléséhez kell
    QString sysdescr = node.getName(_sSysDescr);
    // Előszedjük a címet az ideiglenesen felvett/beolvasott, egy port és egy cím alapján.
    // A portot töröljük, azt majd a lekérdezés teljes adatatartalommal felveszi
    if (node.ports.size() != 1) {
        es = QObject::trUtf8("Egy. és csakis egy ideiglenes portot lehet megadni. megadott portszám :%1 ").arg(node.ports.size());
        goto setPortsBySnmp_error;
    }
    if (node.ports.first()->chkObjType<cInterface>(EX_IGNORE) < 0) {
        es = QObject::trUtf8("Az ideiglenes portonak Interface típusúnak kell lennie.");
        goto setPortsBySnmp_error;
    }
    pTempIf = node.ports.pop_back()->reconvert<cInterface>();
    if (pTempIf->addresses.size() != 1) {
        es = QObject::trUtf8("A megadott (ideiglenes) portnak egy és csakis egy címe lehet. ");
setPortsBySnmp_error:
        pDelete(pTempIf);
        if (__ex) EXCEPTION(EDATA,-1, es);
        if (pEs != NULL) *pEs += es;
        return false;
    }
    // Megvan az IP cím, amit lekérdezhetőnk
    hostAddr = pTempIf->addresses[0]->address();
    pDelete(pTempIf); // ez már nem kell.
    if (hostAddr.isNull()) {
        es = QObject::trUtf8("Nincs beállítva megfelelő IP cím.");
        goto setPortsBySnmp_error;
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
        }
    } while (0 == snmp.getNext());
    PDEB(VVERBOSE) << "*************************************************" << endl;
    PDEB(VVERBOSE) << "SNMP TABLE+ : " << endl << tab.toString() << endl;
    PDEB(VVERBOSE) << "*************************************************" << endl;
    QSqlQuery q = getQuery();
    int n = tab.rows();
    int i;
    for (i = 0; i < n; i++) {
        QHostAddress    addr(tab[_sIpAdEntAddr][i].toString());
        QString         name = tab[_sIfDescr][i].toString();
        int             type = tab[_sIfType][i].toInt(&ok);
        if (!ok) EX(EDATA, -1, QString("SNMP ifType: '%1'").arg(tab[_sIfType][i].toString()));
        // IANA típusból következtetönk az objektum típusára és iftype_name -ra
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
                cInterface *pif = dynamic_cast<cInterface *>(pPort);
                pif->addTrunkMember(o.at(0));
            } while (!snmp.getNext());
        }
    }
    _DBGFNL() << "OK, node : " << node.toString() << endl;
    return true;
}

#define SNMPSET(s, f)   if (0 != (e = snmp.get(n = s))) { \
                            es = "cSnmp::get(" + n + ") error : " + snmp.emsg; \
                            DERR() << es << endl;\
                            node.clear(f); \
                            if (pEs != NULL) *pEs += es + " "; \
                            r = 0; /* WARNING */ \
                        } else { \
                            node.set(f, snmp.value()); \
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
    QString n;
    int e;
    SNMPSET("SNMPv2-MIB::sysDescr.0",       _sSysDescr);
    SNMPSET("SNMPv2-MIB::sysObjectID.0",    _sSysObjectId);
    SNMPSET("SNMPv2-MIB::sysContact.0",     _sSysContact);
    SNMPSET("SNMPv2-MIB::sysName.0",        _sSysName);
    SNMPSET("SNMPv2-MIB::sysLocation.0",    _sSysLocation);
    SNMPSET("SNMPv2-MIB::sysServices.0",    _sSysServices);
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
class cLldpScan {
    friend void scanByLldp(QSqlQuery q, const cSnmpDevice& __dev);
protected:
    cLldpScan(QSqlQuery& _q);
    /// LLDP felderítés
    /// @param __dev A kiindulási SNMP eszköz
    void scanByLldp(QSqlQuery &q, const cSnmpDevice& __dev);
    /// LLDP felderítés, egy eszköz, a pDev lekérdezése
    void scanByLldpDev(QSqlQuery &q);
    /// LLDP felderítés, Az SNMP lekérdezés "egy sorának" a feldolgozása
    bool scanByLldpDevRow(cSnmp &snmp, cSnmp &asnmp);
    /// Egy a MAC alapján nem talált, de felderített eszköz keresése vagy létrehozása.
    /// Ha az IP alapján megtalálja az eszközt, akkor természetesen nem hozza létre.
    /// Csak akkor hozza létre az adatbázisban nem talált eszközt, ha az SNMP-n kerestül lekérdezhető.
    /// Hiba esetén egyszerúen vissatér, csak fatális hiba esetén dob kizárást.
    /// @return Ha megtalálta, vagy megkreálta az eszközt (ill. a hozzá tartozó rekordokat), akkor true, egyébként false.
    bool createSnmpDev();
    bool updateSnmpDev();
    void updateLink();
    typedef tRecordList<cSnmpDevice> tDevList;
    tDevList    scanned;        ///< A megtalált eszközök nevei, ill. amit már nem kell lekérdezni
    tDevList    queued;         ///< A megtatlált, de még nem lekérdezett eszközök nevei
    cSnmpDevice *pDev;          ///< Az aktuális scennelt eszköz
    QSqlQuery&  q;              ///< Az adatbázis műveletekhez használt SQL Query objektum
    cMac        rMac;           ///< Aktuális távoli eszköz MAC
    cSnmpDevice rDev;           ///< Aktuális távoli device, azonos rHost-al vagy üres
    cNode       rHost;          ///< Aktuális távoli hoszt
    int         rPortIx;
    cInterface  rPort;
    int         lPortIx;        ///< Lokális port indexe
    cInterface  lPort;          ///< A lokális port objektum
    cLldpLink   lnk;            ///< Link objektum
    int         RemChassisIdSubtype;
    int         RemPortIdSubtype;
    QString     RemPortId;
    QString     RemPortDesc;
    QHostAddress RemManAddr;

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

cLldpScan::cLldpScan(QSqlQuery& _q)
    : scanned(), queued(), q(_q), rMac(), rDev(), rHost(), rPort(), lPort(), lnk(), RemPortId(), RemPortDesc()
{
    staticInit();
    pDev = NULL;
}

void cLldpScan::staticInit() {
    if (sOids.isEmpty()) {
        sOids << "LLDP-MIB::lldpRemChassisIdSubtype.0";
        oids  << cOId(sOids.last());
        sOids << "LLDP-MIB::lldpRemChassisId.0";
        oids  << cOId(sOids.last());
        sOids << "LLDP-MIB::lldpRemPortIdSubtype.0";
        oids  << cOId(sOids.last());
        sOids << "LLDP-MIB::lldpRemPortId.0";
        oids  << cOId(sOids.last());
        sOids << "LLDP-MIB::lldpRemPortDesc.0";
        oids  << cOId(sOids.last());
        sAddrOid = "LLDP-MIB::lldpRemManAddrIfId.0";
        pAddrOid  = new cOId(sAddrOid);
    }
    if (!oids[0]) EXCEPTION(ESNMP, -1, sOids.first());
}


void cLldpScan::updateLink()
{
    DBGFNL();
    lnk.clear();
    rPort.clear();
    lPort.clear();

    lPort.setId(_sPortIndex, lPortIx);
    lPort.setId(_sNodeId,    pDev->getId());
    if (lPort.completion(q) != 1) return;   // Ha nem tudjuk meghatározni a lokális portot, nem lessz link sem...

    rPort.setId(_sNodeId, rHost.getId());
    if (rPortIx > 0 && rPort.setId(_sPortIndex, rPortIx).completion(q) != 1) {
        if (rPort.setName(RemPortDesc).completion(q) != 1) {
            if (rPort.setName(RemPortId).completion(q) != 1) {
                // Név alapján nem találjuk a portot.
                rPort.clear(_sPortName);    // Ha a dev-nek csak egy portja van, akkor az lessz az
                if (rPort.completion(q) != 1) return; // Nem egy port van
            }
        }
    }
    if (lnk.isLinked(q, lPort.getId(), rPort.getId())) {
        lnk.touch(q);
        PDEB(VVERBOSE) << "Existent link : " << lnk.toString() << endl;
        return;
    }
    if (NULL_ID != lnk.getLinked(q, lPort.getId())) {
        PDEB(INFO) << "Remove old link (local port): " << lnk.toString() << endl;
        lnk.remove(q);
    }
    if (NULL_ID != lnk.getLinked(q, rPort.getId())) {
        PDEB(INFO) << "Remove old link : (remote port)" << lnk.toString() << endl;
        lnk.remove(q);
    }
    lnk.clear();
    lnk.setId(_sPortId1, lPort.getId());
    lnk.setId(_sPortId2, rPort.getId());
    lnk.insert(q);
    PDEB(INFO) << "Create new link: " << lnk.toString() << endl;
}

bool cLldpScan::createSnmpDev()
{
    DBGFN();

    QString memo;
    cNode nodeByIp;
    if (nodeByIp.fetchByIp(q, RemManAddr, EX_IGNORE)) { // Rákeresünk az eszközre a cím alapján is. Nem kéne találatnak lennie.
        memo =
                QObject::trUtf8("LLDP lekérdezés : A %1 MAC nem azonosít eszközt, de a %2 IP cím igen :\n").arg(rMac.toString()).arg(RemManAddr.toString())
              + QObject::trUtf8("Az IP cím alapján talált eszköz : \n%1\n").arg(nodeByIp.toString());
        APPMEMO(q, memo, RS_CRITICAL);
        return false;
    }
    QStringPair ip;
    QString     ma = rMac.toString();
    ip.first = RemManAddr.toString();
    ip.second= _sFixIp;
    rDev.asmbNode(q, _sLOOKUP, NULL, &ip, &ma, _sNul, NULL_ID, EX_IGNORE);
    rDev.setName(_sNodeNote, "By LLDP scan.");
    int e = 0;  // Hiba azonosító
    QString es;
    if (rDev.setBySnmp(_sNul, EX_IGNORE, &es)) {         // e == 0 : Objektum kitöltése
        e++;
        int ixNodeType = rDev.toIndex(_sNodeType);
        rDev.clearDefectivFieldBit(ixNodeType);     // Insert beállítja, nembaj ha rossz
        rDev.clear(ixNodeType);
        // Ezzel most ne szívassuk magunkat, majd talán késöbb megkeresem miér teljesül néha
//      if (rDev.isDefective() == false) {      // e == 1 : Az eddig kitöltött mezők rendben?
            e++;
            if (rDev.insert(q, EX_IGNORE)) {        // e == 2 : Sikerült létrehozni az objektumot ?
                PDEB(INFO) << "Created SNMP Device : " << rDev.toString() << endl;
                queued << rDev;                 // Ezt majd lekérdezzük
                rHost.set(rDev);
                return true;                    // O.K.
            }
//      }
    }
    memo = QObject::trUtf8("LLDP lekérdezés : %1 MAC és %2 IP című eszköz felvétele sikertelen :\n").arg(rMac.toString()).arg(RemManAddr.toString());
    switch (e) {
    case 0:     memo += QObject::trUtf8("Az SNMP lekérdezés sikertelen : ") + es + "\n";  break;
//  case 1:     memo += QObject::trUtf8("Hibásan kitöltött adatmező: %1\n").arg(rDev.defectiveFields().join(_sCommaSp));    break;
    case 2:     memo += QObject::trUtf8("Sikertelen adatbázis művelet: \n");    break;
    default:    EXCEPTION(EPROGFAIL);
    }
    memo += rDev.toString();
    APPMEMO(q, memo, RS_CRITICAL);      // LOG
    DERR() << memo << endl;
    rDev.clear();
    rHost.clear();
    return false;                       // FAILED
}

bool cLldpScan::updateSnmpDev()
{
    // rHost beolvasva...
    QString memo;
    cNode nodeByIp;
    if (!nodeByIp.fetchByIp(q, RemManAddr)) {  // Rákeresünk az eszközre a cím alapján is. Számítunk a találatra!??
        memo =
                QObject::trUtf8("LLDP lekérdezés : A %1 MAC ismert, de a %2 IP cím nem azonosít eszközt:\n").arg(rMac.toString()).arg(RemManAddr.toString())
              + QObject::trUtf8("Az MAC cím alapján talált eszköz : \n%1\n").arg(rHost.toString());
        APPMEMO(q, memo, RS_CRITICAL);
        rDev.clear();
        rHost.clear();
        return false;
    }
    if (nodeByIp != rHost) { // Ha nem azonos az nagyon gáz
        memo =
                QObject::trUtf8("LLDP lekérdezés : A talált %1 MAC és %2 IP cím nem ugyan azt az eszközt azonosítja:\n").arg(rMac.toString()).arg(RemManAddr.toString())
              + QObject::trUtf8("Az IP cím alapján talált eszköz : \n%1\n").arg(nodeByIp.toString())
              + QObject::trUtf8("Az MAC cím alapján talált eszköz : \n%1\n").arg(rHost.toString());
        APPMEMO(q, memo, RS_CRITICAL);
        rDev.clear();
        rHost.clear();
        return false;
    }
    if (rDev.tableoid() == rHost.fetchTableOId(q)) {    // Ez SNMP Device ?
        if (!rDev.fetchByMac(q, rMac)) {                // Beolvassuk a teljes rekordot
            EXCEPTION(EPROGFAIL,0, QObject::trUtf8("Fetch by %1 MAC.").arg(rMac.toString())); // Hirtelen mégsincs ???!!!
        }
        // Kell frissíteni?
        if (queued.contains(rDev.getId()) || scanned.contains(rDev.getId())) return true;
        rDev.fetchOnePortByIp(q, RemManAddr);
        rDev.containerValid = CV_PORTS | CV_PORTS_ADDRESSES;    // A VLAN-okat nem kérdezzük le.
        int e = 0;
        QString es;
        if (rDev.setBySnmp(_sNul, EX_IGNORE, &es)) {      // Objektum tulajdonságainak az aktualizálása
            ++e;
            if(rDev.rewrite(q, EX_IGNORE)) {         // Frissítjük az objektumot
                queued << rDev;                                 // Ezt majd lekérdezzük (LLDP)
                return true;
            }
        }
        memo =
                QObject::trUtf8("LLDP lekérdezés : A talált %1 MAC és %2 IP című eszköz frissítése sikertelen:\n").arg(rMac.toString()).arg(RemManAddr.toString())
              + (e == 0 ? QObject::trUtf8("Sikertelen SNMP lekérdezés.") + es : QObject::trUtf8("Az objektum kiírása sikertelen."))
              + QObject::trUtf8("\nA talált eszköz : \n%1\n").arg(rDev.toString());
        APPMEMO(q, memo, RS_WARNING);
        DWAR() << memo << endl;
        rDev.clear();
        rHost.clear();
        return false;
    }
    else rDev.clear();                                  // Ismert, de nem SNMP dev.
    return true;
}

#define NONEXTSNMPDATA(n)       if (NULL == snmp.next()) { EXCEPTION(EFOUND, n, "No SNMP data " + sOids[n]); }
bool cLldpScan::scanByLldpDevRow(cSnmp& snmp, cSnmp& asnmp)
{
    rHost.clear();
    rDev.clear();
    QString em;
    int re = RS_CRITICAL;   // Hiba, alapértelmezetten
    int r, ix;
    cOId aoid;
    RemManAddr.clear();

    if (NULL == snmp.first()) {
        EXCEPTION(EFOUND, 0, "No SNMP data " + sOids[0]); return false;
    }
    if (!(oids.first() < snmp.name())) return false;    // vége a táblázatnak

    lPortIx = snmp.name()[oids[0].size()];
    // Az eredmény első tagját már ellenőriztük.
    RemChassisIdSubtype = snmp.value().toInt(); // lldpRemChassisIdSubtype
    if (RemChassisIdSubtype != 4) return true;  // Csak a macAddress(4) típusok érdekelnek.

    NONEXTSNMPDATA(1)
    ix = (int)snmp.name()[oids[1].size()];
    if (lPortIx != ix) {
        em = "scrambled data #1 " + snmp.name().toString() + " awaited port id = " + QString::number(lPortIx);
        goto scanByLldpDevRow_error;
    }
    rMac.set(snmp.value().toByteArray());       // lldpRemChassisId
    if (!rMac) {                       // Nem OK-s MAC
        em = "Invalid MAC : ";
        goto scanByLldpDevRow_error;
    }

    NONEXTSNMPDATA(2)
    ix = (int)snmp.name()[oids[2].size()];
    if (lPortIx != ix) {
        em = "scrambled data #2 " + snmp.name().toString() + " awaited port id = " + QString::number(lPortIx);
        goto scanByLldpDevRow_error;
    }
    RemPortIdSubtype = snmp.value().toInt();    // lldpRemPortIdSubtype
    if (RemPortIdSubtype != 7   // local(7)         pl. ProCurve
     && RemPortIdSubtype != 5   // interfaceName(5) pl. HP 3Com
     && RemPortIdSubtype != 3)  // macAddress(3)    pl. HP AccesPoint
        return true;

    NONEXTSNMPDATA(3)
    ix = (int)snmp.name()[oids[3].size()];
    if (lPortIx != ix) {
        em = "scrambled data #3 " + snmp.name().toString() + " awaited port id = " + QString::number(lPortIx);
        goto scanByLldpDevRow_error;
    }
    RemPortId = snmp.value().toString();        // lldpRemPortId
    bool ok;
    rPortIx = RemPortId.toInt(&ok);
    if (!ok) rPortIx = -1;

    NONEXTSNMPDATA(4)
    ix = (int)snmp.name()[oids[4].size()];
    if (lPortIx != ix) {
        em = "scrambled data #4 " + snmp.name().toString() + " awaited port id = " + QString::number(lPortIx);
        goto scanByLldpDevRow_error;
    }
    RemPortDesc = snmp.value().toString();      // lldpRemPortDesc

    aoid = *pAddrOid + lPortIx;
    r = asnmp.getNext(aoid);
    if (r != 0 || asnmp.first() == NULL) {
        em =  "SNMP getnext(" + aoid.toString() + ") return = " + QString::number(r);
        goto scanByLldpDevRow_error;
    }
    if (!(aoid < asnmp.name())) {
        em = "No data " + aoid.toString() + "... =>> " + asnmp.name().toString() + "/" + asnmp.name().toNumString();;
        re = RS_WARNING;
        goto scanByLldpDevRow_error;
    }
    RemManAddr = asnmp.name().toIPV4();

    if (RemManAddr.isNull())  {
        em = "no IPV4 " + aoid.toString() + "... =>> " + asnmp.name().toString() + "/" + asnmp.name().toNumString();
scanByLldpDevRow_error:
        QString ln = "Local node : " + pDev->getName();
        QString line = "\nSNMP LINE : ";
        snmp.first();   line += snmp.name().toString() + ", ";
        snmp.next();    line += snmp.name().toNumString() + ", ";
        snmp.next();    line += snmp.name().toString() + " = " + snmp.value().toString() + ", ";
        snmp.next();    line += snmp.name().toNumString() + "\n";
        em = ln + line + em;
        APPMEMO(q, em, re);
        DERR() << em << endl;
        return true;    // NEXT
    }

    PDEB(VERBOSE) << "Local node : " + pDev->getName() << " SNMP row : " << VDEB(lPortIx) << VDEB(RemChassisIdSubtype) << " rMac = "
                  << rMac.toString() << VDEB(RemPortIdSubtype)  << VDEB(RemPortId) << VDEB(rPortIx) << VDEB(RemPortDesc) << endl;

    if (rHost.fetchByMac(q, rMac)) {                        // Ismert host ? (hosts, és snmpdecices táblában keressük)
        if (!updateSnmpDev()) return true;                       // Ismert host/dev
        // Frissítettük, ha még nem, ok
    }
    else {                                                  // Nem ismert host/dev
        if (!createSnmpDev()) return true;                       // nem ismert, és nem tudjuk létrehozni sem...
        // Létrehoztuk, vagy mégis megvan, ?
    }
    updateLink();
    return true;
}

void cLldpScan::scanByLldpDev(QSqlQuery& q)
{
    cSnmp snmp, asnmp;
    pDev->open(q, snmp);
    pDev->open(q, asnmp);   // másik munkamanetben kell lekérdezni a távoli eszköz címét
    // ----- Egy eszköz lekérdezése
    bool first = true;
    int  r;
    while(1) {
        /// Soron kénti lekérdezés
        if (first) {    // Első sor
            first = false;
            r = snmp.getNext(oids);
            if (r != 0) {
                DERR() << "Device : " << pDev->toString() << " SNMP getnext(" << sOids.join(_sCommaSp) << ") : " << r << endl;
                break;
            }
        }
        else {          // további sorok
            r = snmp.getNext();
            if (r != 0) {
                DERR() << "Device : " << pDev->toString() << " SNMP getnext() : " << r << endl;
                break;
            }
        }
        if (!scanByLldpDevRow(snmp, asnmp)) break;
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

void scanByLldp(QSqlQuery q, const cSnmpDevice& __dev)
{
    cLldpScan(q).scanByLldp(q, __dev);
}

#endif // MUST_SCAN
