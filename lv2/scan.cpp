#include "scan.h"

#include "lv2data.h"
#include "srvdata.h"
#include "lv2link.h"
#include "report.h"

/***************************************************************************************************************/
#ifdef SNMP_IS_EXISTS

cArpTable& cArpTable::getBySnmp(cSnmp& __snmp)
{
    DBGFN();
    cOId oid("IP-MIB::ipNetToMediaPhysAddress");
    PDEB(VVERBOSE) << "Base oid : " << oid.toNumString() << endl;
    if (__snmp.getNext(oid)) EXCEPTION(ESNMP, __snmp.status, "First getNext().");
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

int cArpTable::getByLocalProcFile(const QString& __f, QString *pEMsg)
{
    _DBGFN() << "@(" << __f << QChar(',') << endl;
    QFile procFile(__f.isEmpty() ? "/proc/net/arp" : __f);
    if (!procFile.open(QIODevice::ReadOnly | QIODevice::Text)) EXCEPTION(EFOPEN, -1, procFile.fileName());
    return getByProcFile(procFile, pEMsg);
}

int cArpTable::getBySshProcFile(const QString& __h, const QString& __f, const QString& __ru, QString *pEMsg)
{
    //_DBGFN() << " @(" << VDEB(__h) << QChar(',') << VDEB(__f) << QChar(',') << VDEB(__ru) << QChar(')') << endl;
    QString cmd = "ssh";
    QStringList args;
    args << __h;
    if (__ru.isEmpty() == false) args << "-l" << __ru;
    args << "cat";
    args << (__f.isEmpty() ? "/proc/net/arp" : __f);
    QProcess    proc;
    QString     msg;
    int r = startProcessAndWait(proc, cmd, args, &msg);
    if (r != 0) EXCEPTION(EPROCERR, r, msgCat(proc.readAll(), msg, "\n"));
    return getByProcFile(proc, pEMsg);
}

int cArpTable::getByLocalDhcpdConf(const QString& __f, qlonglong _hid)
{
    //_DBGFN() << " @(" << __f << ")" << endl;
    QFile dhcpdConf(__f.isEmpty() ? "/etc/dhcp3/dhcpd.conf" : __f);
    if (!dhcpdConf.open(QIODevice::ReadOnly | QIODevice::Text)) EXCEPTION(EFOPEN, -1, dhcpdConf.fileName());
    return getByDhcpdConf(dhcpdConf, _hid);
}

int cArpTable::getBySshDhcpdConf(const QString& __h, const QString& __f, const QString& __ru, qlonglong _hid)
{
    //_DBGFN() << " @(" << VDEB(__h) << QChar(',') << VDEB(__f) << QChar(',') << VDEB(__ru) << QChar(')') << endl;
    QString cmd = "ssh";
    QStringList args;
    args << __h;
    if (__ru.isEmpty() == false) args << "-l" << __ru;
    args << "cat";
    args <<(__f.isEmpty() ? "/etc/dhcp3/dhcpd.conf" : __f);
    QProcess    proc;
    QString     msg;
    int r = startProcessAndWait(proc, cmd, args, &msg);
    if (r != 0) EXCEPTION(EPROCERR, r, msgCat(proc.readAll(), msg, "\n"));
    return getByDhcpdConf(proc, _hid);
}

int cArpTable::getByLocalDhcpdLeases(const QString& __f)
{
    //_DBGFN() << " @(" << __f << ")" << endl;
    QFile dhcpdLease(__f.isEmpty() ? "/var/lib/dhcp/dhcpd.leases" : __f);
    if (!dhcpdLease.open(QIODevice::ReadOnly | QIODevice::Text)) EXCEPTION(EFOPEN, -1, dhcpdLease.fileName());
    return getByDhcpdLease(dhcpdLease);
}
int cArpTable::getBySshDhcpdLeases(const QString& __h, const QString& __f, const QString& __ru)
{
    //_DBGFN() << " @(" << VDEB(__h) << QChar(',') << VDEB(__f) << QChar(',') << VDEB(__ru) << QChar(')') << endl;
    QString cmd = "ssh";
    QStringList args;
    args << __h;
    if (__ru.isEmpty() == false) args << "-l" << __ru;
    args << "cat";
    args << (__f.isEmpty() ? "/var/lib/dhcp/dhcpd.leases" : __f);
    QProcess    proc;
    QString     msg;
    int r = startProcessAndWait(proc, cmd, args, &msg);
    if (r != 0) EXCEPTION(EPROCERR, r, msgCat(proc.readAll(), msg, "\n"));
    return getByDhcpdLease(proc);
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

// protected

int cArpTable::getByProcFile(QIODevice& __f, QString *pEMsg)
{
    DBGFN();
    QTextStream str(&__f);
    str.readLine(); // Drop header
    QString line;
    int r = 0;
    while ((line = str.readLine()).isEmpty() == false) {
        QStringList fl = line.split(QRegExp("\\s+"));
        QHostAddress addr(fl[0]);
        QString em;
        if (addr.isNull()) {
            em = QObject::tr("Invalid IP address: %1 ARP #%2 line : '%3'.").arg(fl[0]).arg(r).arg(line);
            DWAR() << em << endl;
            if (pEMsg != nullptr) *pEMsg = em;
            return -1;
        }
        cMac mac(fl[3]);
        if (mac.isEmpty()) {
            // PDEB(VVERBOSE) << "Dropp NULL MAC..." << endl;
            continue;
        }
        if (!mac) {
            em = QObject::tr("Invalid MAC address: %1 ARP #%2 line : '%3'.").arg(fl[3]).arg(r).arg(line);
            DWAR() << em << endl;
            if (pEMsg != nullptr) *pEMsg = em;
            return -1;
        }
        // PDEB(VVERBOSE) << line << " : " << fl[0] << "/" << fl[3] << " : " << addr.toString() << "/" << mac.toString() << endl;
        PDEB(INFO) << "insert(" << addr.toString() << QChar(',') << mac.toString() << ")" << endl;
        insert(addr, mac);
        ++r;
    }
    if (pEMsg != nullptr) *pEMsg = QObject::tr("Pcessed %1 line(s).").arg(r);
    return r;
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

/// Parse dhcp.comf file
/// @param __f A fájl tartalma
/// @param _hid host_service_id opcionális
int cArpTable::getByDhcpdConf(QIODevice& __f, qlonglong _hid)
{
    //DBGFN();
    static const QString _shost           = "host";
    static const QString _shardware       = "hardware";
    static const QString _sethernet       = "ethernet";
    static const QString _sfixed_address  = "fixed-address";
    static const QString _srange          = "range";
    QString tok;
    QVariant hId;
    if (_hid != NULL_ID) hId = _hid;
    int r = 0;
    while (true) {
        tok = token(__f);
        if (tok.isEmpty()) return r;
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
            ++r;
        }
        else if (tok == _srange) {
            QSqlQuery q = getQuery();
            QString sb, se;
            GETOKEN();  sb = tok;   QHostAddress b(tok);
            GETOKEN();  se = tok;   QHostAddress e(tok);
            if (b.isNull() || e.isNull() || b.protocol() != e.protocol()
              || (b.protocol() == QAbstractSocket::IPv4Protocol && b.toIPv4Address() >= e.toIPv4Address())) {
                DERR() << "Dynamic range : " << sb << " > " << se << endl;
                QString m = QObject::tr("Invalid dynamic range from %1 to %2, HostService : %3").arg(sb, se, cHostService::fullName(q, _hid, EX_IGNORE));
                APPMEMO(q, m, RS_CRITICAL);
                continue;
            }
            QString s = execSqlTextFunction(q,"replace_dyn_addr_range", b.toString(), e.toString(), hId);
            PDEB(INFO) << "Dynamic range " << b.toString() << " < " << e.toString() << "repl. result: " << s << endl;
            ++r;
        }
    }
    return -1;
}

int cArpTable::getByDhcpdLease(QIODevice& __f)
{
    int r = 0;
    while (!__f.atEnd()) {
        QString line = QString(__f.readLine()).trimmed();
        if (line.isEmpty()) continue;
        QRegExp firstLine("lease\\s+(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s*\\{");   // Is Begin Block ?
        QHostAddress addr;
        cMac         mac;
        if (!firstLine.exactMatch(line)) {
            PDEB(WARNING) << "Dropped line : " << quotedString(line) << endl;
            continue;
        }
        addr.setAddress(firstLine.cap(1));
        if (addr.isNull()) {
            PDEB(WARNING) << "Invalid IP address, line : "  << quotedString(line) << endl;
            continue;
        }
        eTristate active = TS_NULL;
        while (!__f.atEnd()) {
            QString line = QString(__f.readLine()).trimmed();
            if (line.isEmpty()) continue;
            if (line == "}") break;                             // Is En Of Block
            if (active == TS_FALSE) continue;                   // Inactive : Scan End Of Block
            if (active == TS_TRUE && mac.isValid()) continue;   // Ready    : Scan End Of Block
            QRegExp stateLine("binding\\s+state\\s+(\\w+)\\s*;");   // State ?
            if (stateLine.exactMatch(line)) {
                if (stateLine.cap(1) == QString("active")) {
                    active = TS_TRUE;
                }
                else {
                    active = TS_FALSE;
                }
            }
            QRegExp macLine("hardware\\s+ethernet\\s+([a-fA-F\\d:]+)\\s*;");    // MAC ?
            if (macLine.exactMatch(line)) {
                mac.set(macLine.cap(1));
                if (mac.isEmpty()) {
                    PDEB(INFO) << "Invalid MAC address, line : "  << quotedString(line) << endl;
                    active = TS_FALSE;
                    continue;
                }
            }
        }
        if (active == TS_TRUE && mac.isValid()) {
            insert(addr, mac);
            ++r;
        }
    }
    return r;
}

/**********************************************************************************************/

#define IFTYPE_IANA_ID_ETH  6

#define EX(ec,ei,_es) {\
    if (__ex) { EXCEPTION(ec, ei, _es); } \
    else {  \
        es = cError::errorMsg(eError::ec) + " : " + _es + " "; \
        DERR() << es << endl; \
        if (pEs != nullptr) *pEs += es + "\n"; \
    }   return  false; }

static cMac portSetMac(QSqlQuery& q, cSnmpDevice& node, cNPort& p, cTable& table, int i, QString& note)
{
    cMac            mac(table[_sIfPhysAddress][i].toByteArray());
    if (mac.isValid()) {
        // MAC collisions are not allowed, but it happens
        cInterface colIf;
        colIf.setMac(_sHwAddress, mac);
        if (colIf.completion(q) && colIf.getId(_sNodeId) != node.getId()) {
            note += QObject::tr("MAC %1 collision by %2, clear MAC.").arg(mac.toString(), colIf.getFullName(q));
            expError(note);
            // Create app_memos (log) record : We do not know, this is a mistake or coincidence
            QString msg = QObject::tr("Scan %1 : %2 port ").arg(node.getName(), p.getName()) + note;
            APPMEMO(q, msg, RS_WARNING);
            mac.clear();    // Dropp MAC
        }
        p.setMac(_sHwAddress, mac);
    }
    return mac;
}

static void portSetAddress(QSqlQuery& q, cSnmpDevice& node, QHostAddress& hostAddr, cNPort & p, QHostAddress& addr, bool& found, bool& foundMyIp, bool& foundJoint)
{
    if (!addr.isNull()) {   // Van IP címünk
        cInterface *pIf = p.reconvert<cInterface>();
        cIpAddress& pa = pIf->addIpAddress(addr);
        if (pa.thisIsExternal(q)) {  // Ez lehet külső cím is !! Ha nincs hozzá subnet
            expWarning(QObject::tr("A %1 cím külső cim lesz, mert nincs hozzá subnet.").arg(addr.toString()));
        }
        else {
            switch (pa.thisIsJoint(q, node.getId())) {
            case TS_NULL:   // Nincs címütközés, OK
                break;
            case TS_FALSE:  // Cím ütközés !!!
                expError(QObject::tr("%1 címmel már van bejegyzett eszköz!").arg(addr.toString()));
                break;
            case TS_TRUE:   // Ütközik, de a típus 'joint'
                expInfo(QObject::tr("%1 címmel már van bejegyzett eszköz. A cím típusa 'joint' lessz.").arg(addr.toString()));
                foundJoint = true;
                break;
            }
        }
        // A paraméterként megadott címet preferáltnak vesszük
        if (addr == hostAddr) {   // Ez az
            pa.setId(_sPreferred, 0);
            foundMyIp = true;
        }
        found = true;
    }
}
static QHostAddress portSetAddress(QSqlQuery& q, cSnmpDevice& node, QHostAddress hostAddr, cNPort & p, cTable& table, int i, bool& found, bool& foundMyIp, bool& foundJoint)
{
    QHostAddress addr(table[_sIpAdEntAddr][i].toString());
    portSetAddress(q, node, hostAddr, p, addr, found, foundMyIp, foundJoint);
    return addr;
}

bool setPortsBySnmp(cSnmpDevice& node, eEx __ex, QString *pEs, QHostAddress *ip, cTable *_pTable)
{
    QString es;
    // Előszedjük a címet
    QHostAddress hostAddr = ip == nullptr ? node.getIpAddress() : *ip;
    // Kell lennie legalább egy IP címnek!!
    bool    found = false;
    // Illene megtalálni azt az IP-t amivel lekérdezünk.
    bool    foundMyIp = false;
    bool    foundJoint = false;
    // A gyátrói baromságok kezeléséhez kell
    QString sysdescr = node.getName(_sSysDescr);
    // A portot töröljük, azt majd a lekérdezés teljes adatatartalommal felveszi
    node.ports.clear();
    if (hostAddr.isNull()) {
        if (__ex) EXCEPTION(EDATA,-1, es);
        if (pEs != nullptr) *pEs = QObject::tr("Nincs IP cím.");
        return false;
    }

    cSnmp   snmp(hostAddr.toString(), node.getName(_sCommunityRd), node.snmpVersion());
    if (!snmp.isOpened()) EX(ESNMP, 0, node.getName(_sAddress));
    cTable     *ptab = _pTable == nullptr ? new cTable : _pTable;    // Interface table container
    QStringList cols;   // Table col names container
    cols << _sIfIndex << _sIfDescr << _sIfType << /* _sIfMtu << _sIfSpeed << */ _sIfPhysAddress;
    int r = snmp.getTable(_sIfMib, cols, *ptab);
    if (r) EX(ESNMP, r, snmp.emsg);
    PDEB(VVERBOSE) << "*************************************************" << endl;
    PDEB(VVERBOSE) << "SNMP TABLE : " << endl << ptab->toString() << endl;
    PDEB(VVERBOSE) << "*************************************************" << endl;
    // A getTable() kizárást dob, ha az egyik oszlop nem kérdezhető le, ifName pedig nem mindíg van.
    *ptab << _sIfName;
    cOId oid;
    oid.set(_sIfMib + _sIfName);
    // A név oszlop kitöltése, ha van
    if (0 == snmp.getNext(oid)) do {
        if (!(snmp.name() > oid)) break;
        cOId name  = snmp.name();
        cOId oidIx = name - oid;
        int i = int(oidIx.last());
        QVariant *p = ptab->getCellPtr(_sIfIndex, i, _sIfName);
        if (oidIx.size() != 1 || p == nullptr) {
            EX(EDATA, i, QString("Not found : %1,%2").arg(_sIfIndex, _sIfName));
        }
        p->setValue(snmp.value().toString());
    } while (0 == snmp.getNext());
    // A getTable() metódus nem tudja lekérdezni az IP címet, ezért ezt az oszlopot külön kérdezzük le.
    *ptab << _sIpAdEntAddr; // Add ip address (empty column) to table
    oid.set(_sIpMib + _sIpAdEntIfIndex);
    bool ok;
    // Kitöltjük az IP cím oszlopot
    if (0 == snmp.getNext(oid)) {   // first get
        do {
            if (nullptr == snmp.first()) EX(ESNMP, -1, _sNul);
            // Az első az interface snmp indexe az OID-ben pedig az IP cím
            cOId name    = snmp.name();
            cOId oidAddr = name - oid;
            if (!oidAddr) break;    // Ha a végére értünk
            int  i    = snmp.value().toInt(&ok);
            if (!ok) EX(EDATA, -1, QString("SNMP index: %1 '%2'").arg(name.toString()).arg(snmp.value().toString()));
            QString sAddr = oidAddr.toNumString();  // IP address string
            QHostAddress addr(sAddr);               // check
            if (addr.isNull()) EX(EDATA, -1, QString("%1/%2").arg(sAddr, oidAddr.toNumString()));
            if (addr != QHostAddress(QHostAddress::Any)) {  // If address is 0.0.0.0, then dropp
                QVariant *p = ptab->getCellPtr(_sIfIndex, i, _sIpAdEntAddr);
                if (!p) EX(EDATA, i, QString("Not found : %1,%2").arg(_sIfIndex, _sIpAdEntAddr));
                p->setValue(sAddr);
                found = true;   // Van IP címünk (a táblázatban)
            }
        } while (0 == snmp.getNext());
    }
    PDEB(VVERBOSE) << "*************************************************" << endl;
    PDEB(VVERBOSE) << "SNMP TABLE+ : " << endl << ptab->toString() << endl;
    PDEB(VVERBOSE) << "*************************************************" << endl;
    // Ha nincs IP címünk, az gáz
    if (!found) EX(EDATA, 0, QString("IP not found"));
    found = false;  // A tábla feldolgozása után is kell lennie! Az sem jó, ha eldobtuk
    QSqlQuery q = getQuery();
    int n = ptab->rows();
    int i;
    // Make ports container from snmp query results
    for (i = 0; i < n; i++) {
        QString         note;
        QString         ifName  = (*ptab)[_sIfName] [i].toString();
        QString         ifDescr = (*ptab)[_sIfDescr][i].toString();
        int             ifType  = (*ptab)[_sIfType] [i].toInt(&ok);
        // ifType is numeric ?
        if (!ok) EX(EDATA, -1, QString("SNMP ifType: '%1'").arg((*ptab)[_sIfType][i].toString()));
        int             ifIndex = (*ptab)[_sIfIndex][i].toInt(&ok);
        if (!ok) EX(EDATA, -1, QString("SNMP ifIndex: '%1'").arg((*ptab)[_sIfIndex][i].toString()));
        // If ifname is empty, then ifName is ifDescr
        if (ifName.isEmpty()) ifName = ifDescr;
        // IANA type (ifType) -->  port object type
        const cIfType  *pIfType = cIfType::fromIana(ifType);
        if (pIfType == nullptr) {
            QString msg = QObject::tr("Unhandled interface type %1 : #%2 %3")
                    .arg(ifType).arg(ifIndex).arg(ifName);
            PDEB(VERBOSE) << msg << endl;
            expInfo(msg);
            continue;
        }
        ifName.prepend(pIfType->getName(_sIfNamePrefix));  // Set name prefix, avoid name collisions
        QString         ifTypeName = pIfType->getName();
        cNPort *pPort = cNPort::newPortObj(*pIfType);
        if (pPort->descr().tableName() != _sInterfaces) {
            EX(EDATA, -1, QObject::tr("Invalid port object type"));
        }
        cMac mac = portSetMac(q, node, *pPort, *ptab, i, note);
        // Host is windows
        if (node.getBool(_sNodeType, NT_WINDOWS)) {
            // The name can be accented, but not unicode
            QByteArray ban = (*ptab)[_sIfDescr][i].toByteArray();
            ifName = QString::fromLatin1(ban);
            if (ifType == IFTYPE_IANA_ID_ETH) {    // Guess at who the real Ethernet interfaces
                QRegExp pat("#[0-9]+$");   // A sample that fits the physical interface, for example: ...#34
                if (0 > pat.indexIn(ifName)) {
                    pIfType = &cIfType::ifType("veth"); // Is virtual (maybe)
                }
            }
        }
        // SonicWall returns useless portnames, catches the comment section
        else if (sysdescr.contains("sonicwall", Qt::CaseInsensitive)) {
            QStringList sl = ifName.split(QChar(' '));
            if (sl.size() > 1) {
                note = msgCat(note, ifName);
                ifName = sl[0];
            }
        }
        (*ptab)[_sIfName][i] = ifName;    // That the name in the table always matches the name in the ports container
        pPort->setName(ifName);
        pPort->setNote(note);
        pPort->setName(_sIfDescr.toLower(), ifDescr);
        pPort->set(_sPortIndex, ifIndex);
        pPort->set(_sIfTypeId, pIfType->getId());
        QHostAddress addr = portSetAddress(q, node, hostAddr, *pPort, *ptab, i, found, foundMyIp, foundJoint);
        PDEB(VVERBOSE) << "Insert port : " << pPort->toString() << endl;
        expInfo(QObject::tr("Port #%1 %2 (%3) %4 %5").arg(
                    pPort->getName(_sPortIndex),
                    dQuoted(pPort->getName()),
                    cIfType::ifType(pPort->getId(_sIfTypeId)).getName(),
                    (mac.isEmpty() ? _sNul : mac.toString()),
                    (addr.isNull() ? _sNul : addr.toString())
                    ) );
        node.ports << pPort;
    }
    // Trunk port hozzárendelések lekérdezése
    // Van hogy a trunk port a típus alapján (1, unknown) az első menetben ki lett szórva!
    oid.set("IF-MIB::ifStackStatus");
    qlonglong trunkIfTypeId = cIfType::ifTypeId(_sMultiplexor);
    ok = false;
    if (snmp.getNext(oid)) {
        QString msg = QObject::tr("cSnmp::getNext(%1) error : %2").arg(oid.toString(), snmp.emsg);
        expError(msg);
        DERR() << msg << endl;
    }
    else do {
        cOId o = snmp.name() - oid;
        PDEB(VVERBOSE) << "OID : " << snmp.name().toString() << " / " << o.toNumString() << endl;
        if (o.size() != 2) {
            if (!o.isEmpty()) {
                QString msg = QObject::tr("Nem értelmezhető next OID : %1 -> %2").arg(oid.toString(), snmp.name().toString());
                expError(msg);
                PDEB(WARNING) << msg << endl;
            }
            if (!ok) expWarning(QObject::tr("Nincs információ a trunk-ökről."));
            break;     // túl szaladtunk, vagy hibás válasz
        }
        ok = true;
        QVariant v = snmp.value().toInt();
        if (v.toInt() != 1) continue;   // Active(1) ?
        int tix = int(o[0]);    // trunk port index
        if (tix <= 0) continue; // ?
        int mix = int(o[1]);    // member port index
        if (mix <= 0) continue; // ?
        cNPort *pMem = node.ports.get(_sPortIndex, QVariant(mix), EX_IGNORE);
        if (pMem == nullptr || !pMem->ifType().isLinkage()) continue;  // It's not likely to trunk
        cNPort *pTrk = node.ports.get(_sPortIndex, QVariant(tix), EX_IGNORE);
        if (pTrk == nullptr) {  // Not found as valid port
            int row = ptab->getRowIndex(_sIfIndex, tix);
            if (row < 0) continue;  // Port not found in table by index
            pTrk = cNPort::newPortObj(cIfType::ifType(trunkIfTypeId));
            pTrk->setName((*ptab)[_sIfName][row].toString());
            pTrk->setId(_sPortIndex, tix);
            pTrk->setName(_sIfDescr.toLower(), (*ptab)[_sIfDescr][row].toString());
            QString note = QObject::tr("Feltételezett trunk port (ifType = %1)").arg((*ptab)[_sIfType][row].toString());
            pTrk->setNote(note);
            portSetMac(q, node, *pTrk, *ptab, tix, note);
            portSetAddress(q, node, hostAddr, *pTrk, *ptab, tix, found, foundMyIp, foundJoint);
            node.ports << pTrk;
        }
        else {
            if (pTrk->getId(_sIfTypeId) != trunkIfTypeId) continue; // ?
        }
        pTrk->reconvert<cInterface>()->addTrunkMember(mix);
    } while (!snmp.getNext());

    if (!found) {   // Primary IP address is not found
        for (i = 0; i < n; i++) {   // Rescan table
            int             type = (*ptab)[_sIfType][i].toInt(&ok);
            const cIfType  *pIfType = cIfType::fromIana(type);
            if (pIfType == nullptr) {  // Unknown types. This was discarded in the first round
                QHostAddress    addr((*ptab)[_sIpAdEntAddr][i].toString());
                if (addr.isNull()) continue;    // If there is no address, then we will drop it now
                pIfType = &cIfType::ifType(_sVEth);  // The address belongs to this, then it should be virtual ethernet
                QString         name = (*ptab)[_sIfName][i].toString();
                cNPort *pPort = cNPort::newPortObj(*pIfType);
                if (pPort->descr().tableName() != _sInterfaces) {
                    EX(EDATA, -1, QObject::tr("Invalid port object type"));
                }
                pPort->setName(name);
                pPort->set(_sIfDescr.toLower(), (*ptab)[_sIfDescr][i]);
                pPort->set(_sPortIndex, (*ptab)[_sIfIndex][i]);
                pPort->set(_sIfTypeId, pIfType->getId());
                QString note;
                portSetMac(q, node, *pPort, *ptab, i, note);
                portSetAddress(q, node, hostAddr, *pPort, addr, found, foundMyIp, foundJoint);
                PDEB(VVERBOSE) << "Insert port : " << pPort->toString() << endl;
                node.ports << pPort;
                expInfo(QObject::tr("Port #%1 %2 (%3) %4").arg(
                            pPort->getName(_sPortIndex),
                            dQuoted(pPort->getName()),
                            cIfType::ifType(pPort->getId(_sIfTypeId)).getName(),
                            addr.toString()
                            ) );
                found = true;
            }
        }
    }
    if (!found) EX(EDATA, -1, QString("IP is not found"));
    if (!foundMyIp) {
        expWarning(QObject::tr("A lekérdezés nem adta vissza a lekérdezéshez használlt %1 IP címet.").arg(hostAddr.toString()));
    }
    if (foundJoint && !node.getBool(_sNodeType, NT_CLUSTER)) {
        expWarning(QObject::tr("Az eszköznél nem volt megadva a 'cluster' kapcsoló, ugyanakkor találtunk 'joint' típusú címet."));
        node.enum2setOn(_sNodeType, NT_CLUSTER);
    }
    _DBGFNL() << "OK, node : " << node.toString() << endl;
    if (_pTable == nullptr) delete ptab;
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
    QString msg;
    if (0 != snmp.get(s)) {
        msg = QObject::tr("SNMP node \"%1\" : cSnmp::get(%2) error : %3").arg(node.getName(), s, snmp.emsg);
        DERR() << msg << endl;
        node.clear(f);
        if (pEs != nullptr) {
            *pEs += msg + "\n";
        }
        return false; /* WARNING */
    }
    else {
        node.set(f, snmp.value());
        msg = QObject::tr("%1.%2 <== %3").arg(node.getName(), f, node.getName(f));
        expInfo(msg);
    }
    return true; /* OK */
}

int setSysBySnmp(cSnmpDevice &node, eEx __ex, QString *pEs, QHostAddress *ip)
{
    int r = 1;  //OK
    QString es;
    QString ma = ip == nullptr ? node.getIpAddress().toString() : ip->toString();
    if (ma.isEmpty()) {
        es = QObject::tr("Hiányzó IP cím.");
        if (__ex) EXCEPTION(EDATA, -1, es);
        DERR() << es << endl;
        if (pEs != nullptr) *pEs += es;
        return -1;  // ERROR
    }
    cSnmp   snmp(ma, node.getName(_sCommunityRd), node.snmpVersion());

    if (!snmp.isOpened()) {
        es = QObject::tr("SNMP open(%1, %2) error. ").arg(ma).arg(node.getName(_sCommunityRd));
        if (__ex) EXCEPTION(EDATA, -1, es);
        DERR() << es << endl;
        if (pEs != nullptr) *pEs += es;
        return -1;  // ERROR
    }
    bool war = false;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysDescr.0",    _sSysDescr)    || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysObjectID.0", _sSysObjectId) || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysContact.0",  _sSysContact)  || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysName.0",     _sSysName)     || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysLocation.0", _sSysLocation) || war;
    war = snmpset(node, snmp, pEs, "SNMPv2-MIB::sysServices.0", _sSysServices) || war;
    if (!war) r = 0;
    else {
        qlonglong type = node.getId(_sNodeType);
        type &= ~ENUM2SET2(NT_NODE, NT_HUB);
        type |=  ENUM2SET2(NT_SNMP, NT_HOST);
        cSelect sel;
        QSqlQuery q = getQuery();
        sel.choice(q, "snmp.node_type", node.getName(_sSysDescr));
        const cColEnumType* pnte = cColEnumType::fetchOrGet(q, _sNodetype);
        if (!sel.isEmptyRec()) {
            QString s = sel.getName(_sChoice);
            if (!s.isEmpty()) {
                QStringList sl = s.split(QRegExp(",\\s*"));
                type |= pnte->lst2set(sl);
            }
        }
        node.setId(_sNodeType, type);
        expInfo(QObject::tr("Típus jelzők: %1").arg(pnte->set2lst(type).join(_sCommaSp)));
    }
    return r;
}

/**********************************************************************************************/

QString lookup(const QHostAddress& ha, eEx __ex)
{
    QHostInfo hi = QHostInfo::fromName(ha.toString());
    QString r = hi.hostName();
    if (hi.error() != QHostInfo::NoError || ha.toString() == r) {
        if (__ex) EXCEPTION(EFOUND, int(hi.error()), QObject::tr("A hoszt név nem állapítható meg"));
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
    friend void ::scanByLldp(QSqlQuery& q, const cSnmpDevice& __dev, bool _parser);
    friend LV2SHARED_EXPORT void ::lldpInfo(QSqlQuery& q, const cSnmpDevice& __dev, bool _parser);
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
        QString         toString();
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
    bool rowTail(QSqlQuery &q, const QString &ma, const tStringPair &ip, cAppMemo &em, bool exists);
    void memo(cAppMemo em, QSqlQuery &q, int port_ix, rowData &row);

    typedef tRecordList<cSnmpDevice> tDevList;
    bool        parser;         ///< A parser-ből lett hivva?
    tDevList    setted;         ///< A portjait lekérdeztuk/frissítettük
    tDevList    scanned;        ///< A megtalált eszközök nevei, ill. amit már nem kell lekérdezni
    tDevList    queued;         ///< A megtatlált, de még nem lekérdezett eszközök nevei
    cSnmpDevice *pDev;          ///< Az aktuális scennelt eszköz
    QSqlQuery&  q;              ///< Az adatbázis műveletekhez használt SQL Query objektum
    cMac        rMac;           ///< Aktuális távoli eszköz MAC
    cSnmpDevice rDev;           ///< Aktuális távoli device, azonos rHost-al vagy üres
    cNode       rHost;          ///< Aktuális távoli hoszt
    int         rPortIx;
    cInterface  rPort;          ///< Aktuális távoli port
    cInterface  rMacPort;       ///< Aktuális távoli port a távoli MAC címhez
    cInterface  rIpPort;        ///< Aktuális távoli port a távoli IP  címhez
    cInterface  lPort;          ///< A lokális port objektum
    cLldpLink   lnk;            ///< Link objektum
    int         lPortIx;        ///< Lokális port indexe
    QMap<int, rowData>  rRows;
    QString choice;
    QString lPrefix;

    static QStringList  sOids;      ///< Az SNMP lekérdezésnél használt kiindulási objektum név lista
    static cOIdVector   oids;       ///< Az SNMP lekérdezésnél használt kiindulási objektum lista
    static QString      sAddrOid;   ///< Távoli eszköz címe objektum név
    static cOId        *pAddrOid;   ///< Távoli eszköz címe objektum, külön kell lekérdezni, hiányozhat
    static void staticInit();       ///< Az oids és oidss statikus konténerek inicializálása/feltöltése
    static QString      sIsNoSnmpDev;
    static QString      sIsMissingMac;
    static QString      sIsMissingPName;
    static QString      sIsInvalidPName;
    static QString      sIsInvalidPIndex;
};

QStringList  cLldpScan::sOids;
cOIdVector   cLldpScan::oids;
QString      cLldpScan::sAddrOid;
cOId        *cLldpScan::pAddrOid = nullptr;

QString cLldpScan::rowData::toString()
{
    QString r;
    r  = QString("Name/Descr = %1/%2; ")
            .arg(quotedString(name))
            .arg(quotedString(descr));
    r += QString("CLocal = %1; ")
            .arg(quotedString(clocal));
    r  = QString("Port Name/Descr = %1/%2; ")
            .arg(quotedString(pname))
            .arg(quotedString(pdescr));
    r += QString("MAC = %1/%2; ")
            .arg(cmac.isValid() ? cmac.toString() : _sNULL)
            .arg(cmac.isValid() ? pmac.toString() : _sNULL);
    r += QString("IP = ")
            .arg(addr.isNull() ? _sNULL : addr.toString());
    return r;
}

cLldpScan::cLldpScan(QSqlQuery& _q, bool _parser)
    : setted(), scanned(), queued(), q(_q), rMac(), rDev(), rHost(), rPort(), lPort(), lnk()
{
    parser = _parser;
    staticInit();
    pDev = nullptr;
    if (sIsNoSnmpDev.isEmpty()) {
        sIsNoSnmpDev     = QObject::tr("A távoli IP-hez tartozó node létezik (%1), de nem egy SNMP eszköz. (%2)");
        sIsMissingMac    = QObject::tr("Hiányzik az eszköz azonosító MAC cím. (%1)");
        sIsMissingPName  = QObject::tr("Hiányzik a távoli port azonosító név. (%1)");
        sIsInvalidPName  = QObject::tr("Nincs %1 nevű távoli port. (%2)");
        sIsInvalidPIndex = QObject::tr("A '%1' nem értelmezhető port indexként. (%2)");
    }
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
    if (!cLldpScan::oids.last()) EXCEPTION(EOID, i, cLldpScan::sOids.last());
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

    QString tl = QObject::tr("%1:%2 <==> %3:%4")
            .arg(pDev->getName(), lPort.getName())
            .arg(rHost.getName(), rPort.getName());
    lnk.clear();
    qlonglong lId = lPort.getId();
    qlonglong rId = rPort.getId();
    int r;
    QString es, ls;

    if (lId == NULL_ID || rId == NULL_ID) {
        es = lPrefix + QObject::tr("LLDP link (%1) update failed.").arg(tl);
        if (lId == NULL_ID) es += QObject::tr(" Local port ID is NULL.");
        if (rId == NULL_ID) es += QObject::tr(" Remote port ID is NULL.");
        HEREINWE(em, es, RS_WARNING);
        return false;
    }

    // Comparison with the logical link (report)
    cLogLink logl;
    bool confirm = logl.isLinked(q, lId, rId);     // Ha létezik ugyan ilyen logikai link
    bool collision = false;
    if (!confirm) { // ütközések ?
        bool c1 = NULL_ID != logl.getLinked(q, lId);
        if (c1) expError(QObject::tr("Ellentmondás a %1 logikai linkkel.").arg(logl.show()));
        bool c2 = NULL_ID != logl.getLinked(q, rId);
        if (c2) expError(QObject::tr("Ellentmondás a %1 logikai linkkel.").arg(logl.show()));
        collision = c1 || c2;
    }

    if (lnk.isLinked(q, lId, rId)) {
        r = lnk.touch(q);
        switch (r) {
        case 0:
            es = lPrefix + QObject::tr("A %1 link időbéllyegének a modosítása sikertelen.").arg(tl);
            break;
        case 1:
            es = lPrefix + QObject::tr("Regisztrált link %1, időbélyeg frissítve.").arg(tl);
            PDEB(INFO) << es << endl;
            if (confirm)        expGreen(es);
            else if (collision) expError(es);   // RED
            else                expInfo(es);
            return true;
        default:
            es = lPrefix + QObject::tr("Az %1 link időbéllyegének a modosítása. Értelmezhetetlen eredmény : %2").arg(tl).arg(r);
            break;
        }
        HEREINWE(em, es, RS_WARNING);
        return false;
    }

    if (NULL_ID != lnk.getLinked(q, lId)) {
        ls = lnk.show();
        es = lPrefix + QObject::tr("Régi link %1 eltávolítása (lokális port).").arg(ls);
        PDEB(INFO) << es << endl;
        expInfo(es);
        r = lnk.remove(q);
        switch (r) {
        case 0:
            es = lPrefix + QObject::tr("Régi LLDP link (L : %1) törlése sikertelen.").arg(ls);
            HEREINWE(em, es, RS_WARNING);
            return false;
        case 1:
            break;
        default:
            es = lPrefix + QObject::tr("Régi LLDP link (L : %1) törlése. Értelmezhetetlen eredmény : %2").arg(ls).arg(r);
            HEREINWE(em, es, RS_WARNING);
            return false;
        }
    }
    if (NULL_ID != lnk.getLinked(q, rId)) {
        ls = lnk.show();
        es = QObject::tr("Régi link %1 eltávolítása (távoli port).").arg(ls);
        PDEB(INFO) << es << endl;
        expInfo(es);
        r = lnk.remove(q);
        switch (r) {
        case 0:
            es = lPrefix + QObject::tr("Régi LLDP link (R : %1) törlése sikertelen.").arg(ls);
            HEREINWE(em, es, RS_WARNING);
            return false;
        case 1:
            break;
        default:
            es = lPrefix + QObject::tr("Régi LLDP link (R) törlése. Értelmezhetetlen eredmény : %1").arg(ls).arg(r);
            HEREINWE(em, es, RS_WARNING);
            return false;
        }
    }
    lnk.clear();
    lnk.setId(_sPortId1, lId);
    lnk.setId(_sPortId2, rId);
    if (lnk.insert(q)) {
        es = lPrefix + QObject::tr("Új link : %1 .").arg(tl);
        PDEB(INFO) << es << endl;
        if (confirm)        expGreen(es);
        else if (collision) expError(es);   // RED
        else                expInfo(es);
        return true;
    }
    es = lPrefix + QObject::tr("Az új LLDP link ( %1 ) kiírása sikertelen.").arg(tl);
    HEREINWE(em, es, RS_WARNING);
    return false;
}

void cLldpScan::memo(cAppMemo em, QSqlQuery& q, int port_ix, rowData& row)
{
    QString s;
    s = QObject::tr("LLDP scan, beolvasott sor %1:#%2 :").arg(pDev->getName()).arg(port_ix);
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

static bool nodeCompare(QSqlQuery& q, qlonglong nidByMac, qlonglong& nidByIp, cInterface& rIpPort)
{
    nidByIp = rIpPort.getId(_sNodeId);
    if (nidByMac == NULL_ID) return false;
    if (nidByIp == nidByMac) return true;
    while (q.next()) {
        rIpPort.set(q);
        nidByIp = rIpPort.getId(_sNodeId);
        if (nidByIp == nidByMac) return true;
    }
    return false;
}

// DEBUG
#define LLDP_WRITE_CSV  1

void cLldpScan::scanByLldpDevRow(QSqlQuery& q, cSnmp& snmp, int port_ix, rowData& row)
{
    _DBGFN() << " ROW : " << row.name << "(" << row.addr.toString() << "):" << row.pname << " / "
             << row.cmac.toString() << ":" << row.pmac.toString()
             << row.descr << " : " << row.pdescr << endl;
    bool f, r = false;
    int  e = 0, i, portIdSubType, n;
    QString portDescr, portId;
    QStringList soids;
    cAppMemo em;
    cPatch *pp = nullptr, *pp2 = nullptr;
    cSelect sel;
    QHostAddress a;
    cMac        portMac;
    cNPort *plp = nullptr;
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
    enum {
        LPORT_NOT_IDENT     = -1,
        LPORT_NOT_CREDIBLE  = -2,
        LPORT_AMBIGUITY     = -3

    };
    lPortIx   = LPORT_NOT_IDENT;

    // Lokális port objektum Az LLDP-ben nem biztos, hogy azonos a portok indexelése (gratula annak aki ezt kitalálta)
    // És az sem biztos, hogy a portDescr mező az interfész descriptort tartalmazza, ez a 3COM esetén az portId-ben van (brávó-brávó, gondolom létezik több elcseszett variáció is)
    soids <<  QString("LLDP-MIB::lldpLocPortDesc.%1")     .arg(port_ix);
    soids <<  QString("LLDP-MIB::lldpLocPortIdSubtype.%1").arg(port_ix);
    soids <<  QString("LLDP-MIB::lldpLocPortId.%1")       .arg(port_ix);

    a = pDev->getIpAddress();
    snmp.open(a.toString(), pDev->getName(_sCommunityRd), int(pDev->getId(_sSnmpVer)));
    e = snmp.get(soids);
    if  (e) {
        HEREINWE(em, QObject::tr("A %1 LLDP indexű lokális port sikertelen azonosítás: snmp error #%2.").arg(port_ix).arg(e), RS_WARNING);
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
        if (!f) lPortIx = LPORT_NOT_CREDIBLE;
        break;
    case 3: // macAddress
        portMac.set(snmp.value());
        break;
    case 5: // interfaceName :? ifDescr
        portId  = snmp.value().toString();
        break;
    }

    if (lPortIx >= 0) {     // Van indexünk ?
        plp = pDev->ports.get(_sPortIndex, lPortIx, EX_IGNORE);
        if (plp != nullptr && 0 != plp->chkObjType<cInterface>(EX_IGNORE)) {
            lPortIx = LPORT_NOT_CREDIBLE;       // Is not interface
            plp = nullptr;
        }
        else if (plp == nullptr) {              // Not found
            lPortIx = LPORT_NOT_CREDIBLE;
        }
    }




    if (lPortIx < 0) for (i = 0; i < pDev->ports.size(); ++i) {     // Nincs index, vagy nem ok, nézzük a neveket
        cNPort *pnp = pDev->ports.at(i);
        if (0 != pnp->chkObjType<cInterface>(EX_IGNORE)) continue;
        cInterface *pif = dynamic_cast<cInterface *>(pnp);
        QString name = pif->getName();
        cMac    pmac = pif->getMac(_sHwAddress);
        r = name == portDescr;
        r = r || (portId.isEmpty() == false && portId  == name);
        r = r || (portMac.isValid()         && portMac == pmac);
        if (r) {
            if (lPortIx == LPORT_NOT_CREDIBLE || lPortIx == LPORT_NOT_IDENT) {  // Volt már egy találat ?
                lPortIx = int(pif->getId(_sPortIndex));  // First
                plp = pnp;
            }
            else {                                  // Not first
                lPortIx = LPORT_AMBIGUITY;
                if (pnp->getId(_sPortIndex) == port_ix) {   // Matching index, maybe this is it
                    plp = pnp;
                }
            }
        }
    }
    if (lPortIx == LPORT_AMBIGUITY && plp != nullptr && plp->getId(_sPortIndex) == port_ix) {
        lPortIx = port_ix;
    }
    if (lPortIx < 0) {
        switch (lPortIx) {
        case LPORT_NOT_IDENT:
            HEREINWE(em, QObject::tr("A %1 LLDP indexű lokális port sikertelen azonosítás, nincs találat. portDescr = %2, portId = %3, portMac = %4.").arg(port_ix).arg(portDescr, portId, portMac.toString()), RS_WARNING);
            break;
        case LPORT_AMBIGUITY:
            HEREINWE(em, QObject::tr("A %1 LLDP indexű lokális port sikertelen azonosítás, nem egyértelmű. portDescr = %2, portId = %3, portMac = %4.").arg(port_ix).arg(portDescr, portId, portMac.toString()), RS_WARNING);
            break;
        case LPORT_NOT_CREDIBLE:
            HEREINWE(em, QObject::tr("A %1 LLDP indexű lokális port sikertelen azonosítás, a lokális %2 index nem értelmezhető, vagy hihető").arg(port_ix).arg(snmp.value().toString()), RS_WARNING);
            break;
        }
        goto scanByLldpDevRow_error;
    }
    lPort.clone(*plp);
    lPrefix = lPort.getName() + " ==> ";

    // Do you have the remote device address?
    if (row.addr.isNull()) {
        if (0 == choice.compare(_sEmpty, Qt::CaseInsensitive)) {
            r = rowEmpty(q, snmp, row, em);
            r = r && updateLink(em);
            if (r) return;
        }
        else {
            HEREINWE(em, lPrefix + QObject::tr("Hiányzik, vagy nem értelmezhető a távoli eszköz IP címe."), RS_WARNING);
        }
        goto scanByLldpDevRow_error;
    }
    // Look for the database by address
    f =      (row.cmac.isValid() && 0 < rMacPort.fetchByMac(q, row.cmac));
    f = f || (row.pmac.isValid() && 0 < rMacPort.fetchByMac(q, row.pmac));
    n = rIpPort.fetchByIp(q, row.addr); // The q will still need to call nodeCompare()
    if (n != 0 || f) {  // Exist (mac or/and IP)
        qlonglong nid;
        qlonglong nidByMac = rMacPort.getId(_sNodeId);
        qlonglong nidByIp;
        if (nodeCompare(q, nidByMac, nidByIp, rIpPort)) {      // Ugyan azt találtuk a címek alapján, öröm boldogság!
            nid = nidByMac;
            pp  = cPatch::getNodeObjById(q, nid, EX_ERROR);
        }
        else if (nidByIp == NULL_ID) {  // Nem találtuk IP alapján
            nid = nidByMac;
            pp = cPatch::getNodeObjById(q, nid, EX_ERROR);
            HEREINWE(em, lPrefix + QObject::tr("A távoli eszköz %1/%2 MAC alapján :\n\t%3\n\tA %4 IP cím alapján nics találat")
                   .arg(row.cmac.toString(), row.pmac.toString(), pp->identifying(), row.addr.toString()), RS_WARNING);
            memo(em, q, port_ix, row);
        }
        else if (nidByMac == NULL_ID) { // Nem találtuk a cím alapján
            nid = nidByIp;
            pp = cPatch::getNodeObjById(q, nid, EX_ERROR);
            if (choice.compare(_s3Com, Qt::CaseInsensitive) != 0) { // 3Com-nal egy fiktív MAC-ot ad, amire nem lessz találat.
                HEREINWE(em, lPrefix + QObject::tr("A távoli eszköz %1 IP cím alapján :\n\t%2\n\tA %3/%4 MAC alapján nics találat")
                       .arg(row.addr.toString(), pp->identifying(), row.cmac.toString(), row.pmac.toString()), RS_WARNING);
                memo(em, q, port_ix, row);
            }
        }
        else {                          // Két különböző eszköz lett azonosítva
            pp  = cPatch::getNodeObjById(q, nidByMac, EX_ERROR);
            pp2 = cPatch::getNodeObjById(q, nidByIp,  EX_ERROR);
            mm = lPrefix + QObject::tr(
                       "Nem egyezik a MAC %1/%2 alapján : %3\n\t"
                       "az IP %4 alapján talált eszközzel : %5\n\t"
                       )
                   .arg(row.cmac.toString(), row.pmac.toString(), pp->identifying())
                   .arg(row.addr.toString(),                     pp2->identifying());
            pDelete(pp2);
            if (rIpPort.isNull()) { // töröltük
                mm += QObject::tr("Az Ip alapján talált eszközön az IP cím törölve lessz!");
                HEREINWE(em, mm, RS_WARNING);
                memo(em, q, port_ix, row);
            }
            else {
                mm += QObject::tr("Az Ip alapján talált eszközön az IP cím törlése sikertelen!");
                HEREINWE(em, mm, RS_CRITICAL);
                goto scanByLldpDevRow_error;
            }
            nid = nidByMac;     // Ezt hisszük el.
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
        }
        // Exist, and SNMP device
        if (0 == pp->chkObjType<cSnmpDevice>(EX_IGNORE)) {
            rDev.clone(*pp);
            rDev.fetchPorts(q, CV_PORTS_ADDRESSES); // Csak a port címekkel, node paraméter, port paraméter vlan-ok nem.
            rHost.clone(rDev);
        }
        // Exist, and not node, patch ???!!!
        else if (0 != pp->chkObjType<cNode>(EX_IGNORE)) {
            HEREINWE(em, lPrefix + QObject::tr("A % talált eszköz típusa patch !?.").arg(pp->getName()), RS_WARNING);
            goto scanByLldpDevRow_error;
        }
        // Exist, and node.
        else {
            rHost.clone(*pp);
            rHost.fetchPorts(q, CV_PORTS_ADDRESSES);    // Szintén csak a címek
        }
    }
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
    lPrefix += QChar('[') + choice + "] ";
    if      (0 == choice.compare(_sProCurve,     Qt::CaseInsensitive))
        r = rowProCurve(q, snmp, row, em);
    else if (0 == choice.compare(_sOfficeConnect,Qt::CaseInsensitive))
        r = rowProCurve(q, snmp, row, em);
    else if (0 == choice.compare(_sProCurveWeb,  Qt::CaseInsensitive))
        r = rowProCurveWeb(q, snmp, row, em);
    else if (0 == choice.compare(_s3Com,         Qt::CaseInsensitive))
        r = row3COM(q, snmp, row, em);
    else if (0 == choice.compare(_sCisco,        Qt::CaseInsensitive))
        r = rowCisco(q, snmp, row, em);
    else if (0 == choice.compare(_sHPAPC,        Qt::CaseInsensitive))
        r = rowHPAPC(q, snmp, row, em);
    else if (0 == choice.compare(_sLinux,        Qt::CaseInsensitive))
        r = rowLinux(q, snmp, row, em);
    else {
        HEREINWE(em, lPrefix + QObject::tr("Az eszköz '%1' descriptora alapján az adatfeldogozás módja ismeretlen.").arg(row.descr), RS_WARNING);
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
    if (nullptr == p) {
        em = QObject::tr("Nincs SNMP adat #%1, OID: %2; %3").arg(n).arg(cLldpScan::sOids[n], snmp.name().toString());
        return -1;
    }
    if (!(cLldpScan::oids[n] < snmp.name())) {
        if (n == 0) return  0;  // End off table
        else {
            em = QObject::tr("Csonka táblázat sor #%1, OID: %2 port ID = %3").arg(n).arg(snmp.name().toString()).arg(_ix);
            return -1;
        }
    }
    int ix = int(snmp.name()[int(cLldpScan::oids[n].size()) +1]);
    if (n == 0) _ix = ix;   // First column set row id (port index)
    else if (_ix != ix) {   // Check row id
        em = QObject::tr("Felcserélt adatok #%1, OID: %2  port ID = %3").arg(n).arg(snmp.name().toString()).arg(_ix);
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
    pDev->fetchPorts(q, CV_PORTS_ADDRESSES);
    if (pDev->open(q, snmp, EX_IGNORE, &em)) {
        DWAR() << QObject::tr("A %1 eszköz lekérdezése meghiusult : ").arg(pDev->getName()) << em << endl;
        return;
    }
    // ----- Egy eszköz lekérdezése
    em = QObject::tr("**** SNMP eszköz %1 lekérdezése (SNMP/LLDP) [%2] ...").arg(pDev->getName(), sOids.join(_sCommaSp));
    PDEB(INFO) << em << endl;
    expInfo(em);
    rRows.clear();
    bool first = true;
    while(1) {
        lPrefix.clear();
        // SNMP Get Next
        r = snmpGetNext(snmp, first, oids);
        if (r) {
            em = QObject::tr("SNMP get-%1 (%2) hiba #%3, '%4'")
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
        NEXTSNMPDATA(LLDP_REM_CID_TYPE)        // first : set index : local port index
        RemChassisIdSubtype = snmp.value().toInt(); // lldpRemChassisIdSubtype
        NEXTSNMPDATA(LLDP_REM_CID)
        RemChasisId = snmp.value();                 // lldpRemChassisId
        NEXTSNMPDATA(LLDP_REM_PID_TYPE)
        RemPortIdSubtype = snmp.value().toInt();    // lldpRemPortIdSubtype
        NEXTSNMPDATA(LLDP_REM_PID)
        RemPortId = snmp.value();                   // lldpRemPortId
        NEXTSNMPDATA(LLDP_REM_PDSC)
        RemPortDesc = snmp.value().toString();      // lldpRemPortDesc
        NEXTSNMPDATA(LLDP_REM_SDSC)
        RemSysDesc  = snmp.value().toString();      // lldpRemSysDesc
        NEXTSNMPDATA(LLDP_REM_SNAME)
        RemSysName  = snmp.value().toString();      // lldpRemSysName

        lPrefix = QObject::tr("[#%1] ==> ").arg(index);

#if LLDP_WRITE_CSV
        {
            QString sRemChassisIdSubtype, sRemPortIdSubtype;
            switch (RemChassisIdSubtype) {
            case 1: sRemChassisIdSubtype = "chassisComponent";  break;
            case 2: sRemChassisIdSubtype = "interfaceAlias";    break;
            case 3: sRemChassisIdSubtype = "portComponent";     break;
            case 4: sRemChassisIdSubtype = "macAddress";        break;
            case 5: sRemChassisIdSubtype = "networkAddress";    break;
            case 6: sRemChassisIdSubtype = "interfaceName";     break;
            case 7: sRemChassisIdSubtype = "local";             break;
            default:sRemChassisIdSubtype = QString::number(RemChassisIdSubtype);    break;
            }
            switch (RemPortIdSubtype) {
            case 1: sRemPortIdSubtype = "interfaceAlias";   break;
            case 2: sRemPortIdSubtype = "portComponent";    break;
            case 3: sRemPortIdSubtype = "macAddress";       break;
            case 4: sRemPortIdSubtype = "networkAddress";   break;
            case 6: sRemPortIdSubtype = "agentCircuitId";   break;
            case 5: sRemPortIdSubtype = "interfaceName";    break;
            case 7: sRemPortIdSubtype = "local";            break;
            default:sRemPortIdSubtype = QString::number(RemChassisIdSubtype);    break;
            }
            const QString sep = ";";
            QFile f("lldp_test_gets.csv");
            f.open(QIODevice::WriteOnly | QIODevice::Append);
            QTextStream strm(&f);
            strm << quotedString(pDev->getName()) << sep << index << sep
                 << sRemChassisIdSubtype << sep
                 << quotedString(RemChasisId.toString()) << sep << dump(RemChasisId.toByteArray()) << sep
                 << sRemPortIdSubtype << sep
                 << quotedString(RemPortId.toString()) << sep << dump(RemPortId.toByteArray()) << sep
                 << quotedString(RemPortDesc) << sep
                 << quotedString(RemSysDesc) << sep
                 << quotedString(RemSysName) << endl;
            f.close();
        }
#endif

        if (!RemSysName.isEmpty()) {
            if (!rRows[index].name.isEmpty() && rRows[index].name != RemSysName) {
                s = QObject::tr("LLDP:\nA %1 eszköz %2 indexű portján a távoli eszköz neve nem egyértelmű : %3, %4")
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
                s = QObject::tr("LLDP:\nA %1 eszköz %2 indexű portján a távoli eszköz port descripzor nem egyértelmű : %3, %4")
                        .arg(pDev->getName()).arg(index).arg(rRows[index].pdescr, RemPortDesc);
                APPMEMO(q, s, RS_WARNING);
                PDEB(WARNING) << s << endl;
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
scanByLldpDev_error: // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                {
                    QList<QHostAddress> al = pDev->fetchAllIpAddress(q);
                    QString sa;
                    if (al.isEmpty()) sa = QObject::tr("Nincs IP cím");
                    else {
                        foreach (QHostAddress a, al) {
                            sa += a.toString() + ", ";
                        }
                        sa.chop(2);
                    }
                    QString ln = QObject::tr("\nLocal node %1 (%2)").arg(pDev->getName()).arg(sa);
                    QString line = "\nSNMP LINE : ";
                    snmp.first();   line += snmp.name().toString() + ", ";
                    snmp.next();    line += snmp.name().toNumString() + ", ";
                    snmp.next();    line += snmp.name().toString() + " = " + snmp.value().toString() + ", ";
                    snmp.next();    line += snmp.name().toNumString() + "\n";
                    em += ln + line;
                    APPMEMO(q, em, RS_CRITICAL);
                    DERR() << em << endl;
                }
                return;     // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    }
    /* IP */
    static cOIdVector aoi;
    cOId o;
    if (aoi.isEmpty()) aoi << *pAddrOid;
    first = true;
    while (1) {
        r = snmpGetNext(snmp, first, aoi);
        if (r) {
            em = QObject::tr("SNMP get - %1(%2) error #%3, %4)")
                      .arg(pDev->toString())
                      .arg(first ? "first" : "next")
                      .arg(sAddrOid)
                      .arg(r);
            PDEB(WARNING) << QObject::tr("SNMP %1 %2 (%3) lekérdezési hiba #%4")
                             .arg(pDev->getName(), sAddrOid, (first ? "first" : "next")).arg(r)
                          << endl;
            if (!first) {
                break;
            }
            goto scanByLldpDev_errorA;
        }
        first = false;
        /* - */
        if (nullptr == snmp.first()) {
            em = QObject::tr("No SNMP data, OID:%1; %2").arg(sAddrOid, snmp.name().toString());
            PDEB(WARNING) << QObject::tr("SNMP %1 %2 nincs adat.")
                             .arg(pDev->getName(), sAddrOid)
                          << endl;
            goto scanByLldpDev_errorA;
        }
        if (!(*pAddrOid < snmp.name())) {
            break;  // End
        }
        /* - */
        o = snmp.name();
        o -= *pAddrOid;
        if (o.size() < 2) continue;
        index = int(o[1]);
        // IPV4 !!
        if (o.size() == 9 && o[3] == 1 && o[4] == 4) {   // ?.index.?.1.4.IPV4
            QHostAddress a = o.toIPV4();
            if (!a.isNull()) rRows[index].addr = a;
        }
        // IPV6 ????
        continue;
scanByLldpDev_errorA:
                {   // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    QList<QHostAddress> al = pDev->fetchAllIpAddress(q);
                    QString sa;
                    if (al.isEmpty()) sa = QObject::tr("Nincs IP cím");
                    else {
                        foreach (QHostAddress a, al) {
                            sa += a.toString() + ", ";
                        }
                        sa.chop(2);
                    }
                    QString ln = QObject::tr("\nLocal node %1 (%2)").arg(pDev->getName()).arg(sa);
                    em += ln;
                    APPMEMO(q, em, RS_CRITICAL);
                    DERR() << em << endl;
                }
                return; // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
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
    setted.clear();
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
    if (pp == nullptr) {
        HEREINWE(em, lPrefix + QObject::tr("Nincs %1 indexű távoli port.").arg(rPortIx), RS_WARNING);
        return false;
    }
    if (pp->chkObjType<cInterface>(EX_IGNORE)) {
        HEREINWE(em, lPrefix + QObject::tr("A %1 indexű távoli port %2 típusa nem megfelelő.").arg(rPortIx).arg(typeid(*pp).name()), RS_WARNING);
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
    if (0 == rHost.ports.size() && 0 == rHost.fetchPorts(q, CV_PORTS_ADDRESSES)) {
        HEREINWE(em, lPrefix + QObject::tr("Talált, és regisztrált '%1'-nek nincs egy portja sem.").arg(row.name), RS_WARNING);
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
    HEREINWE(em, lPrefix + QObject::tr("Talált, és regisztrált '%1'-nek nincs megfelelő portja %2 címmel.").arg(row.name, row.cmac.toString()), RS_WARNING)
    return false;
}

int cLldpScan::portDescr2Ix(QSqlQuery &q, cSnmp &snmp, QHostAddress ha, const QString& pdescr, cAppMemo& em)
{
    _DBGFN() << "@(,," << ha.toString() << ", " << pdescr << ",)" << endl;
    (void)q;
    if (pdescr.isEmpty()) {
        HEREINWE(em, lPrefix + QObject::tr("Hiányzik a port azonosító név."), RS_CRITICAL)
        return -1;
    }
    int r = snmp.open(ha.toString());
    if (r) {
        HEREINWE(em, lPrefix + QObject::tr("Port index keresése. Az SNMP megnyitás sikertelen #%1").arg(r), RS_WARNING)
        return -1;
    }
    cOId oid(_sIfMib + _sIfDescr);
    if (oid.isEmpty()) EXCEPTION(ESNMP,0,_sIfMib + _sIfDescr);
    r = snmp.getNext(oid);
    if (r) {
        HEREINWE(em, lPrefix + QObject::tr("Port index %1 keresése. Sikertelen SNMP (%2, first) lekérdezés #%3").arg(pdescr, ha.toString()).arg(r), RS_WARNING)
        return -1;
    }
    do {
        if (nullptr == snmp.first()) {
            HEREINWE(em, lPrefix + QObject::tr("Port index %1 keresése. No SNMP (%2) data").arg(pdescr, ha.toString()), RS_WARNING);
            return -1;
        }
        cOId o = snmp.name();
        PDEB(VVERBOSE) << o.toString() << " = " << debVariantToString(snmp.value()) << endl;
        if (!(oid < o)) {
            HEREINWE(em, lPrefix + QObject::tr("Port index keresése. Nincs %1 nevű port (%2).").arg(pdescr, ha.toString()), RS_WARNING);
            return -1;
        }
        /* - */
        o -= oid;
        if (o.size() >= 1) {
            r = int(o[0]);
            QString n = snmp.value().toString();
            PDEB(VVERBOSE) << "#" << r << "; compare : " << quotedString(n) << " == " << quotedString(pdescr) << endl;
            if (n == pdescr) return r;
        }
        else {
            DWAR() << "Invalid OID size : " << o.toString() << " , host IP : " << ha.toString() << endl;
        }
        r = snmp.getNext();
        if (r) {
            HEREINWE(em, lPrefix + QObject::tr("Port index keresése. Sikertelen SNMP (next) lekérdezés #%1").arg(r), RS_WARNING);
            return -1;
        }
    } while (true);
}

bool cLldpScan::rowTail(QSqlQuery &q, const QString& ma, const tStringPair& ip, cAppMemo& em, bool exists)
{
    QString es;
    cError *pe = nullptr;
    if (!exists) {
        rDev.asmbNode(q, _sLOOKUP, nullptr, &ip, &ma, _sNul, NULL_ID, EX_IGNORE);
        rDev.setNote(QObject::tr("By LLDP scan."));
        if (!rDev.setBySnmp(_sNul, EX_IGNORE, &es)) {
            HEREINWE(em, lPrefix + QObject::tr("A %1 MAC és %2 IP című eszköz SNMP felderítése sikertelen :\n%3\n").arg(ma, ip.first, es), RS_WARNING);
            return false;
        }
        rDev.containerValid = CV_PORTS | CV_PORTS_ADDRESSES;
        pe = rDev.tryInsert(q);
        if (pe != nullptr) {
            es = lPrefix + QObject::tr("A %1 MAC és %2 IP című SNMP %3 felderített eszköz mentése sikertelen.").arg(ma, ip.first, rDev.getName());
            expError(es + pe->shortMsg());
            HEREINW(em, es + pe->longMsg(), RS_WARNING);
            pDelete(pe);
            return false;
        }
        es = QObject::tr("A %1 SNMP eszköz sikeres felvétele.").arg(rDev.getName());
        PDEB(INFO) << es << endl;
        expInfo(es);
        setted << rDev;
    }
    else {
        if (!setted.contains(rDev.getId())) {
            pe = rDev.tryRewrite(q);
            if (pe != nullptr) {
                es = lPrefix + QObject::tr("A %1 MAC és %2 IP című %3 SNMP felderített eszköz frissítése sikertelen.").arg(ma, ip.first, rDev.getName());
                expError(es + pe->shortMsg());
                HEREINW(em, es + pe->longMsg(), RS_WARNING);
                pDelete(pe);
                return false;
            }
            es = QObject::tr("A %1 SNMP eszköz sikeres frissítése.").arg(rDev.getName());
            PDEB(INFO) << es << endl;
            expInfo(es);
            setted << rDev;
        }
    }
    rHost.clone(rDev);
    if (!queued.contains(rDev.getId())) queued << rDev;
    return setRPortFromIx(em);;
}

QString cLldpScan::sIsNoSnmpDev;
QString cLldpScan::sIsMissingMac;
QString cLldpScan::sIsMissingPName;
QString cLldpScan::sIsInvalidPName;
QString cLldpScan::sIsInvalidPIndex;
#define IS_NO_SNMPDEV(em)   HEREINWE(em, lPrefix + sIsNoSnmpDev.arg(rHost.identifying(false), row.toString()), RS_WARNING)
#define IS_MISSING_MAC(em)  HEREINWE(em, lPrefix + sIsMissingMac.arg(row.toString()), RS_WARNING)
#define IS_MISSING_PNAME(em) HEREINWE(em, lPrefix + sIsInvalidPName.arg(row.toString()), RS_WARNING)
#define IS_INVLID_PNAME(em, nm) HEREINWE(em, lPrefix + sIsInvalidPName.arg(nm, row.toString()), RS_WARNING)
#define IS_INVLID_PINDEX(em, ix) HEREINWE(em, lPrefix + sIsInvalidPIndex.arg(ix, row.toString()), RS_WARNING)

/// ProCurwe switch management
bool cLldpScan::rowProCurve(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo& em)
{
    bool ok;
    bool exists;
    tStringPair ip;
    QString     ma;
    (void)snmp;

    if (!row.cmac.isValid()) {
        IS_MISSING_MAC(em);
        return false;
    }
    rPortIx = row.pname.toInt(&ok);         // A port név = index
    if (!ok) {
        rPortIx = row.pdescr.toInt(&ok);    // vagy port descr = index (pl.: 1820-Gx)
        if (!ok) {
            IS_INVLID_PINDEX(em, row.pname);
            return false;
        }
    }
    exists = !rHost.isEmptyRec();
    if (exists) {
        qlonglong id = rDev.getId();
        if (id == NULL_ID) {
            IS_NO_SNMPDEV(em);
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
    tStringPair ip;
    QString     ma;
    cNPort     *pp;

    if (!row.cmac.isValid()) {
        IS_MISSING_MAC(em);
        return false;
    }
    if (row.pname.isEmpty()) {
        IS_MISSING_PNAME(em);
        return false;
    }
    exists = !rHost.isEmptyRec();
    if (exists) {
        if (rDev.isEmptyRec()) {
            IS_NO_SNMPDEV(em);
            return false;
        }
        pp = rHost.ports.get(_sPortName, row.pdescr, EX_IGNORE);
        if (pp == nullptr) {
            IS_INVLID_PNAME(em, row.pname);
            return false;
        }
        rPortIx = int(pp->getId(_sPortIndex));
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
    tStringPair ip;
    QString     ma;
    cNPort     *pp;

    if (!row.cmac.isValid()) {
        IS_MISSING_MAC(em);
        return false;
    }
    if (row.pname.isEmpty()) {
        IS_MISSING_PNAME(em);
        return false;
    }
    exists = !rHost.isEmptyRec();
    if (exists) {
        if (rDev.isEmptyRec()) {
            IS_NO_SNMPDEV(em);
            return false;
        }
        pp = rHost.ports.get(_sPortName, row.pname, EX_IGNORE);
        if (pp == nullptr) {
            IS_INVLID_PNAME(em, row.pname);
            return false;
        }
        rPortIx = int(pp->getId(_sPortIndex));
        // Frissítettük már ?
        if (queued.contains(rDev.getId()) || scanned.contains(rDev.getId())) return setRPortFromIx(em);
        // Nincs újra felderítés, mert hiányozni fognak a trunk adatok !!!
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
    ma = QString(); // row.cmac.toString(); // az eszköz MAC nem jelenik meg sehol, men azonos a port MAC-al
    ip.first  = row.addr.toString();
    ip.second = _sFixIp;
    return rowTail(q, ma, ip, em, exists);
}

bool cLldpScan::rowCisco(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em)
{
    bool exists;
    tStringPair ip;
    QString     ma;
    cNPort     *pp;

    // MAC nincs
    if (row.pname.isEmpty()) {
        IS_MISSING_PNAME(em);
        return false;
    }
    exists = !rHost.isEmptyRec();
    if (exists) {
        if (rDev.isEmptyRec()) {
            IS_NO_SNMPDEV(em);
            return false;
        }
        pp = rHost.ports.get(_sPortName, row.pname, EX_IGNORE);
        if (pp == nullptr) {
            IS_INVLID_PNAME(em, row.pname);
            return false;
        }
        rPortIx = int(pp->getId(_sPortIndex));
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
        IS_MISSING_MAC(em);
        return false;
    }
    bool exists = !rHost.isEmptyRec();
    if (exists) {
        if (rHost.getName().compare(row.name, Qt::CaseInsensitive) == 0 || rHost.getName().startsWith(row.name + ".")) {
            return setRPortFromMac(row, em);
        }
        rHost.setName(row.name);
        HEREINWE(em, lPrefix + QObject::tr("Név ütjözés. Talált '%1', azonos címmekkel bejegyzett '%2'. Az eszköz át lessz nevezve.").arg(row.name, rHost.getName()), RS_WARNING);
        while (rHost.existByNameKey(q)) {   // New name collision?
            HEREINWE(em, QObject::tr("Név ütközés. A '%1', létezik, új elem átnevezve.").arg(rHost.getName()), RS_WARNING);
            rHost.setName(rHost.getName() + "-new");
        }
        rHost.update(q, false, _bit(rHost.nameIndex()));
        return setRPortFromMac(row, em);
    }

    QString n = row.name;
    if (n.isEmpty()) n = "node-" + row.cmac.toString().remove(char(':'));
    rHost.clear();
    rDev.clear();
    rHost.setName(n);

    while (rHost.existByNameKey(q)) { // Név szerint sincs ?
        HEREINWE(em, QObject::tr("Név ütközés. A '%1', létezik, nem a  '%2' címmel, új elem átnevezve.").arg(rHost.getName(), row.addr.toString()), RS_WARNING);
        rHost.setName(rHost.getName() + "-new");
    }

    rHost.setNote(row.descr);
    rHost.addPort(_sEthernet, row.pdescr, row.pname, 1);
    cInterface *pi = rHost.ports.first()->reconvert<cInterface>();
    *pi = row.cmac;
    pi->addIpAddress(row.addr, cDynAddrRange::isDynamic(q, row.addr), "By LLDP");
    rHost.setId(_sNodeType, enum2set(NT_HOST, NT_AP));
    if (row.clocal.isEmpty() == false) {   // SN
        rHost.setName(_sSerialNumber, row.clocal);
    }
    cError *pe = rHost.tryInsert(q);
    if (pe != nullptr) {
        QString se = lPrefix + QObject::tr("A %1 MAC és %2 IP című felderített eszköz %3 kiírása sikertelen.").arg(row.pmac, row.addr.toString(), rHost.getName());
        expError(se + pe->shortMsg());
        HEREINW(em, se + pe->longMsg(), RS_WARNING);
        pDelete(pe);
        return false;
    }
    expInfo(lPrefix + QObject::tr("Inserted AP (HP) : %1").arg(rHost.getName()));
    rPort.clone(*pi);
    return true;
}

bool cLldpScan::rowLinux(QSqlQuery &q, cSnmp &snmp, rowData &row, cAppMemo &em)
{
    cNPort     *pp;
    (void)snmp;
    if (!(row.pmac.isValid() && row.cmac.isValid())) {
        IS_MISSING_MAC(em);
        return false;
    }
    bool exists = !rHost.isEmptyRec();
    if (exists) {
        if (rHost.getName().compare(row.name, Qt::CaseInsensitive) == 0 || rHost.getName().startsWith(row.name + ".")) {
            pp = rHost.ports.get(_sPortName, row.pdescr, EX_IGNORE);
            if (pp == nullptr) {
                HEREINWE(em, lPrefix + QObject::tr("Nincs %1 nevű port a %2 távoli eszközön.").arg(row.pdescr, rHost.getName()), RS_WARNING);
                return false;
            }
            rPortIx = int(pp->getId(_sPortIndex));
            // A MAC alapján nem biztos, hogy azonosítható
            if (pp->chkObjType<cInterface>(EX_IGNORE)) {
                HEREINWE(em, lPrefix + QObject::tr("A %1 indexű távoli port %2 típusa nem megfelelő.").arg(rPortIx).arg(typeid(*pp).name()), RS_WARNING);
                return false;
            }
            rPort.clone(*pp->reconvert<cInterface>());
            return true;
        }
        HEREINWE(em, lPrefix + QObject::tr("Név ütközés. Talált '%1', azonos címmel bejegyzett '%2'").arg(row.name, rHost.getName()), RS_WARNING);
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
    if (pe != nullptr) {
        QString se = lPrefix + QObject::tr("A %1 MAC és %2 IP című felderített eszköz %3 kiírása sikertelen").arg(row.pmac, row.addr.toString(), rHost.getName());
        expError(se + pe->shortMsg());
        HEREINW(em, se + pe->longMsg(), RS_WARNING);
        pDelete(pe);
        return false;
    }
    expInfo(lPrefix + QObject::tr("Inserted Linux host : %1").arg(rHost.getName()));
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
        IS_MISSING_MAC(em);
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
        HEREINWE(em, lPrefix + QObject::tr("A %1 MAC nem azonosít egyértelmüen egy portot.").arg(mac.toString()), RS_WARNING);
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
            HEREINWE(em, lPrefix + QObject::tr("A %1 MAC címhez nem azonosítható egyértelmüen IP cím (%2).").arg(mac.toString(),s), RS_WARNING);
        }
        else {  // nincs találat
            HEREINWE(em, lPrefix + QObject::tr("A %1 MAC címhez nem azonosítható IP cím.").arg(mac.toString()), RS_WARNING);
        }
        return false;
    }
    // Van egy MAC, egy IP. Keresünk hozzá egy nevet.
    QHostAddress a = al.first();
    QHostInfo hi = QHostInfo::fromName(a.toString());
    QString name = hi.hostName();
    if (name == a.toString()) name = "host-" + mac.toString();

    rHost.setName(name);
    QString note = QObject::tr("LLDP auto %1").arg(QDateTime::currentDateTime().toString());
    rHost.setNote(note);
    rHost.setId(_sNodeType, enum2set(NT_HOST, NT_WORKSTATION));
    rHost.addPort(_sEthernet, _sEthernet, _sEthernet, 1);
    cInterface *pi = rHost.ports.first()->reconvert<cInterface>();
    *pi = row.cmac;
    rDev.setId(_sNodeType, enum2set(NT_HOST));
    pi->addIpAddress(a, cDynAddrRange::isDynamic(q, a), note);
    cError *pe = rHost.tryInsert(q);
    if (pe != nullptr) {
        QString se = lPrefix + QObject::tr("A %1 MAC és %2 IP című felderített eszköz %3 kiírása sikertelen.")
                .arg(row.pmac, a.toString(), rHost.getName());
        expError(se + pe->shortMsg());
        HEREINW(em, se + pe->longMsg(), RS_WARNING);
        pDelete(pe);
        return false;
    }
    expInfo(lPrefix + QObject::tr("Inserted any host : %1").arg(rHost.getName()));
    rPort.clone(*pi);
    return true;
}

void scanByLldp(QSqlQuery& q, const cSnmpDevice& __dev, bool _parser)
{
    cLldpScan(q, _parser).scanByLldp(q, __dev);
}

void lldpInfo(QSqlQuery& q, const cSnmpDevice& __dev, bool _parser)
{
    cLldpScan o(q, _parser);
    cSnmpDevice *pDev = dynamic_cast<cSnmpDevice *>(__dev.dup());
    if (pDev->ports.isEmpty()) pDev->fetchPorts(q, CV_PORTS_ADDRESSES);
    o.pDev = pDev;
    o.scanByLldpDev(q);
    delete pDev;
}

/* ************************************************************************************ */

int ping(QHostAddress _ip)
{
    QString cmd = "ping";
    QStringList args;
    args << "-c4" << _ip.toString();
    QString msg;
    int r = startProcessAndWait(cmd, args, &msg, PROCESS_START_TO, 30000);
    expItalic(msg, true); // Több soros
    return r;
}

static QSet<int> queryAddr(cSnmp& snmp, const cOId& __o, cMac& __mac)
{
    cOId    o = __o;
    QSet<int>  r;
    QString msg;
    while(true) {
        int e = snmp.getNext(o);
        if (e) {
            msg = QObject::tr("SNMP hiba #%1 (%2). OID:%3").arg(e).arg(snmp.emsg).arg(__o.toString());
            DERR() << msg << endl;
            expError(msg);
            break;
        }
        o = snmp.name();
        if (!(__o < o)) {
            PDEB(VERBOSE) << QObject::tr("Nincs több cím. (IDs %1 < %2)").arg(__o.toString(), o.toString()) << endl;
            break;
        }
        bool ok;
        int pix = snmp.value().toInt(&ok);
        if (!ok) {
            msg = QObject::tr("Az SNMP válaszban: nem értelmezhető index. OID:%1: %2 = %3")
                    .arg(__o.toString()).arg(o.toString())
                    .arg(debVariantToString(snmp.value()));
            DERR() << msg << endl;
            expError(msg);
            continue;   // Ilyennek nem kéne lennie, pedig de.
        }
        cMac mac = o.toMac();
        if (!mac) {
            msg = QObject::tr("Az SNMP válaszban: nem értelmezhető MAC. OID: %1 : %2 <= %3")
                    .arg(__o.toString()).arg(o.toString()).arg(pix);
            DWAR() << msg << endl;
            // expWarning(msg); // zavaró
            continue;       // előfordul, ugorjunk
        }
        if (__mac == mac) { // Találat
            PDEB(VERBOSE) << "found #" << pix << " => " << mac.toString() << endl;
            r += pix;
        }
        PDEB(VVERBOSE) << "#" << pix << " => " << mac.toString() << endl;
    };
    DBGFNL();
    return r;
}

#define MAXLINK 20
void exploreByAddress(cMac _mac, QHostAddress _ip, cSnmpDevice& _start)
{
    cError *pe = nullptr;
    QSqlQuery q = getQuery();
    QString msg;
    const int ixPortIndex = cNPort::ixPortIndex();
    try {
        cOId oIdx("SNMPv2-SMI::mib-2.17.1.4.1.2");
        cOId oId1("SNMPv2-SMI::mib-2.17.4.3.1.2");
        cOId oId2("SNMPv2-SMI::mib-2.17.7.1.2.2.1.2");
        if (!oIdx) EXCEPTION(ESNMP, oIdx.status, oIdx.emsg);
        if (!oId1) EXCEPTION(ESNMP, oId1.status, oId1.emsg);
        if (!oId2) EXCEPTION(ESNMP, oId2.status, oId2.emsg);
        if (0 != ping(_ip)) {
            msg = QObject::tr("A %1 cél eszköz pingelése sikertelen. Nincs keresés.").arg(_ip.toString());
            expWarning(msg);
            return;
        }
        expHtmlLine();
        cSnmpDevice dev(_start);
        int cnt = 0;
        while (cnt++ < MAXLINK) {    // Keresés a cím táblákban
            // Az aktuális switch címtáblájának a lekérdezése:
            cSnmp   snmp;
            dev.open(q, snmp);
            QString actNodeName = dev.getName();
            QSet<int> ixl;
            ixl  = queryAddr(snmp, oId1, _mac);
            ixl |= queryAddr(snmp, oId2, _mac);
            if (ixl.isEmpty()) {
                msg = QObject::tr("A %1 switch-en nincs találat a keresett %2 MAC-re.").arg(dev.getName(), _mac.toString());
                PDEB(VERBOSE) << msg << endl;
                expWarning(msg);
                return;
            }
            tRecordList<cInterface> ifl;
            dev.fetchPorts(q, CV_PORTS);
            qlonglong trk = cIfType::ifTypeId(_sMultiplexor);
            int ixStapleId = cInterface().toIndex(_sPortStapleId);
            QMap<int,int> xix;   // A port indexek újra lehetnek számozva!, kell egy kereszt index
            int e = snmp.getXIndex(oIdx, xix);
            if (e) {
                msg = QObject::tr("A port keresztindex (OID : %1) lekérdezése sikertelen : #%2 ").arg(oIdx.toString()).arg(e);
                PDEB(DERROR) << msg << endl;
                expError(msg);
                return;
            }
            // Az indexek alapján előszedjük a portokat, trunk esetén a tagok kerülnek a listába, ha nincs port eldobjuk az indexet
            foreach (int ix, ixl) {
                if (!xix.contains(ix)) {
                    msg = QObject::tr("A %1 index nem található a kereszt index táblában.").arg(ix);
                    PDEB(VERBOSE) << msg << endl;
                    continue;
                }
                ix = xix[ix];
                cNPort *p = dev.ports.get(ixPortIndex, QVariant(ix), EX_IGNORE);    // index -> port
                // Ha nincs ilen interfész, eldobjuk
                if (p == nullptr || p->chkObjType<cInterface>(EX_IGNORE) < 0) continue;
                cInterface *pi = p->reconvert<cInterface>();
                if (pi->getId(_sIfTypeId) == trk) {   // Trunk?
                    // Trunk-nel a member portok kellenek
                    qlonglong pid = pi->getId();
                    QList<cNPort *>::const_iterator it;
                    it = dev.ports.constBegin();
                    for (; it < dev.ports.constEnd(); it++) {   // végigszaladunk az összes porton
                        if ((*it)->getId(ixStapleId) == pid) {  // member port ?
                            p = *it;
                            if (ifl.contains(p->getId())) continue;     // már van a konténerben
                            if (p->chkObjType<cInterface>(EX_IGNORE) < 0) continue; // nem interfész
                            pi = p->reconvert<cInterface>();
                            ifl << *pi; // másolat!!
                        }
                    }
                }
                else {
                    if (ifl.contains(pi->getId())) continue;     // már van a konténerben
                    ifl << *pi;
                }

            }
            // Minden indexet eldobtunk, mert nem tartozott regisztrált porthoz
            if (ifl.isEmpty()) {
                msg = QObject::tr("A %1 switch-en nincs releváns találat a keresett %2 MAC-re.").arg(actNodeName, _mac.toString());
                PDEB(VERBOSE) << msg << endl;
                expWarning(msg);
                return;
            }
            msg = QObject::tr("A kereset MAC-re a %1 swich-en találat a köv. portokon : ").arg(actNodeName);
            QList<cInterface *>::const_iterator it;
            it = ifl.constBegin();
            for (; it < ifl.constEnd(); it++) {
                msg += (*it)->getName() + _sCommaSp;
            }
            msg.chop(_sCommaSp.size());
            expInfo(msg);
            // Megnézzük van-e a talált portokhoz LLDP link ?
            cLldpLink lldp;
            dev.clear();            // A linkelt objektum, még nincs
            it = ifl.constBegin();
            for (; it < ifl.constEnd(); it++) {
                qlonglong lpid = lldp.getLinked(q, (*it)->getId());
                if (lpid != NULL_ID) {      // van linkelt port
                    cNPort p;
                    p.setById(q, lpid);   // a linkelt port az id alapján
                    msg = QObject::tr("Link %1 ==> %2").arg((*it)->getFullName(q), p.getFullName(q));
                    expInfo(msg);
                    qlonglong nid = p.getId(_sNodeId);  // A linkelt eszköz ID
                    if (dev.isEmptyRec_()) {               // Még nincs beolvasva
                        if (!dev.fetchById(q, nid)) { // A következő lekérdezendő switch
                            msg = QObject::tr("LLDP link nem SNMP eszközre : %1").arg(cNode().getNameById(nid));
                            expWarning(msg);
                            continue;
                        }
                    }
                    else if (dev.getId() != nid) {      // Ha több link van mindnek ugyanode kall mutatnia
                        msg = QObject::tr("Ellentmindás a linkek között, egynél több cél eszköz: %1, %2").arg(dev.getName(), cNode().getNameById(q, nid));
                        expError(msg);
                        return;
                    }
                }
            }
            if (!dev.isEmptyRec_()) {
                msg = QObject::tr("Következő SNMP eszköz : %1").arg(dev.getName());
                expInfo(msg);
                continue;
            }
            else {
                msg = QObject::tr("Végponti port(ok) %1:").arg(actNodeName);
                it = ifl.constBegin();
                for (; it < ifl.constEnd(); it++) {
                    msg += (*it)->getName() + _sCommaSp;
                }
                msg.chop(_sCommaSp.size());
                expInfo(msg);
                return;
            }
        }
    } CATCHS(pe);
    if (pe != nullptr) {
        msg = pe->msg();
        pDelete(pe);
        expError(msg, true);
    }
    msg = QObject::tr("Túl sok link, a keresés megszakítva.");
    expError(msg);
}

#else
#error "Invalid projects file ?"
#endif // SNMP_IS_EXISTS

