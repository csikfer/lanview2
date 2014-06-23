
#include <QtCore>

#include "cdebug.h"
#include "cerror.h"
#include "lv2types.h"
#include "strings.h"
#include <float.h>

/************************ enum converters ************************/
QString sInvalidEnum() { return QObject::trUtf8("Invalid"); }

QString ProcessError2String(QProcess::ProcessError __e)
{
    QString s;
    switch (__e) {
        case QProcess::FailedToStart:   s = QObject::trUtf8("FailedToStart");    break;
        case QProcess::Crashed:         s = QObject::trUtf8("Crashed");          break;
        case QProcess::Timedout:        s = QObject::trUtf8("Timedout");         break;
        case QProcess::WriteError:      s = QObject::trUtf8("WriteError");       break;
        case QProcess::ReadError:       s = QObject::trUtf8("ReadError");        break;
        case QProcess::UnknownError:    s = QObject::trUtf8("UnknownError");     break;
        default:
            s = sInvalidEnum();
            DERR() << "QProcess::ProcessError : " << (int)__e << QChar('/') << s << endl;
            break;
    }
    return s;
}
QString ProcessError2Message(QProcess::ProcessError __e)
{
    QString m;
    switch (__e) {
        case QProcess::FailedToStart:
            m = QObject::trUtf8("The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.");
            break;
        case QProcess::Crashed:
            m = QObject::trUtf8("The process crashed some time after starting successfully.");
            break;
        case QProcess::Timedout:
            m = QObject::trUtf8("The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.");
            break;
        case QProcess::WriteError:
            m = QObject::trUtf8("An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            m = QObject::trUtf8("An error occurred when attempting to read from the process. For example, the process may not be running.");
            break;
        case QProcess::UnknownError:
            m = QObject::trUtf8("An unknown error occurred. This is the default return value of error().");
            break;
        default:
            m = QObject::trUtf8("Invalid QProcess::ProcessError enum value, program error.");
            DERR() << m << endl;
            break;
    }
    return m;
}

QString ProcessState2String(QProcess::ProcessState __e)
{
    QString s;
    switch (__e){
        case QProcess::NotRunning:  s = QObject::trUtf8("NotRunning");   break;
        case QProcess::Starting:    s = QObject::trUtf8("Starting");     break;
        case QProcess::Running:     s = QObject::trUtf8("Running");      break;
        default:
            s = sInvalidEnum();
            DERR() << "QProcess::ProcessError : " << (int)__e << QChar('/') << s << endl;
            break;
    }
    return s;
}

QString dump(const QByteArray& __a)
{
    QString r = "[";
    foreach (char b, __a) {
        r += QString("%1,").arg((int)(unsigned char)b,2,16,QChar('0'));
    }
    r.chop(1);
    return r + "]";
}

/* ********************************************************************************************************
   *                                        class cMac                                                    *
   ******************************************************************************************************** */
const qlonglong    cMac::mask = 0x0000ffffffffffffLL;

cMac::cMac()
{
    //PDEB(OBJECT) << "cMac::cMac(), this = " << QString().sprintf("%p", this) << endl;
    val = -1LL;
}
cMac::cMac(const QString& __mac)
{
    //PDEB(OBJECT) << "cMac::cMac(" << __mac << "), this = " << QString().sprintf("%p", this) << endl;
    set(__mac);
}
cMac::cMac(const QByteArray& __mac)
{
    //PDEB(OBJECT) << "cMac::cMac(" << dump(__mac) << "), this = " << QString().sprintf("%p", this) << endl;
    set(__mac);
}bool operator<(const QHostAddress& __a1, const QHostAddress& __a);
cMac::cMac(const QVariant& __mac)
{
    //PDEB(OBJECT) << "cMac::cMac(" << __mac.toString() << "), this = " << QString().sprintf("%p", this) << endl;
    set(__mac);
}
cMac::cMac(qlonglong __mac)
{
    //PDEB(OBJECT) << "cMac::cMac(" << __mac << "), this = " << QString().sprintf("%p", this) << endl;
    set(__mac);
}

cMac::~cMac()
{
    //PDEB(OBJECT) << "delete cMac* " << QString().sprintf("%p", this) << endl;
}
QString   cMac::toString() const
{
    QString r;
    qlonglong m = val;
    for (int i = 6; i > 0; --i) {
        QString b;
        r.push_front(b.sprintf(":%02X", (int) (m & 0x00ff)));
        m >>= 8;
    }
    return r.right(17);
}

const char cMac::_sMacPattern1[] = "^\\s*([a-fA-F\\d]{1-12})\\s*$";
const char cMac::_sMacPattern2[] = "^\\s*([a-fA-F\\d]{1,2}):([a-fA-F\\d]{1,2}):([a-fA-F\\d]{1,2}):([a-fA-F\\d]{1,2}):([a-fA-F\\d]{1,2}):([a-fA-F\\d]{1,2})\\s*$";
const char cMac::_sMacPattern3[] = "^\\s*([a-fA-F\\d]{1,6})-([a-fA-F\\d]{1,6})\\s*$";
const char cMac::_sMacPattern4[] = "^\\s*(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\s*$";
const char cMac::_sMacPattern5[] = "^\\s*([a-fA-F\\d]{1,2})\\s+([a-fA-F\\d]{1,2})\\s+([a-fA-F\\d]{1,2})\\s+([a-fA-F\\d]{1,2})\\s+([a-fA-F\\d]{1,2})\\s+([a-fA-F\\d]{1,2})\\s*$";
cMac& cMac::set(const QString& __mac)
{
    // _DBGFN() << " @(" << __mac << ")" << endl;
    val = 0LL;
    if (__mac.isEmpty()) return *this;
    bool ok = true;
    QRegExp pat;
    if (QRegExp(QString(_sMacPattern1)).exactMatch(__mac)) {
        val = pat.cap(1).toLongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
    }
    else if ((pat = QRegExp(QString(_sMacPattern2))).exactMatch(__mac)
          || (pat = QRegExp(QString(_sMacPattern5))).exactMatch(__mac)) {
        for (int i = 1; i <= 6; ++i) {
            val <<= 8;
            val |= pat.cap(i).toLongLong(&ok, 16);
            if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
            // PDEB(VVERBOSE) << "#" << i << " : " << pat.cap(i) << " : " << hex << val << dec << endl;
        }
        // PDEB(VVERBOSE) << "val = " << hex << val << dec << endl;
    }
    else if (QRegExp(QString(_sMacPattern3)).exactMatch(__mac)) {
        val =  pat.cap(1).toLongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
        val <<= 24;
        val |= pat.cap(2).toLongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
    }
    else if (QRegExp(QString(_sMacPattern4)).exactMatch(__mac)) {
        QString t(__mac);
        for (int i = 1; i <= 6; ++i) {
            qlonglong d = pat.cap(i).toLongLong(&ok, 10);
            if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
            if (d > 255) {
                DERR() << "Invalid MAC decimal value : \"" << __mac << "\"" << endl;
                val = -1LL; // Invaidbool operator<(const QHostAddress& __a1, const QHostAddress& __a);
                break;
            }
            val <<= 8;
            val |= d;
        }
    }
    else {
        //DERR() << "Invalid MAC string : \"" << __mac << "\"" << endl;
        val = -1LL; // Invaid
    }
    //_DBGFNL() << " = " << toString() << " / " << hex << val << dec << endl;
    return *this;
}

cMac& cMac::set(const QByteArray& __mac)
{
    if (__mac.count() == 6) {
        val = 0;
        for (int i = 0; i < 6; i++) {
            val <<= 8;
            val |=(unsigned char) __mac[i];
        }
    }
    else {
        DERR() << "Invalid MAC byte array size : " << __mac.size()  << endl;
        val = -1LL; // Invaid
    }
    return *this;
}

cMac& cMac::set(const QVariant& __mac)
{
    if (__mac.isNull()) val = 0LL;
    else switch (__mac.type()) {
        case QVariant::LongLong:
        case QVariant::ULongLong:   return set(__mac.toULongLong());
        case QVariant::ByteArray:   return set(__mac.toByteArray());
        case QVariant::String:      return set(__mac.toString());
        default:
            DERR() << "Invalid MAC QVariant type : " << __mac.typeName() << endl;
            val = -1LL;
            break;
    }
    return *this;
}

/* ********************************************************************************************************
   *                                        class netAddress                                              *
   ******************************************************************************************************** */

bool operator<(const Q_IPV6ADDR& __a, const Q_IPV6ADDR& __b)
{
    int i;
    for (i = 0; __a[i] == __b[i] && i < 16; i++);
    return i == 16 ? false : __a[i] < __b[i];
}

bool operator<(const QHostAddress& __a1, const QHostAddress& __a2)
{
    enum QAbstractSocket::NetworkLayerProtocol t1 = __a1.protocol();
    enum QAbstractSocket::NetworkLayerProtocol t2 = __a2.protocol();
    if (t1 != t2) return t1 < t2;
    switch (t1) {
    case QAbstractSocket::UnknownNetworkLayerProtocol: break;
    case QAbstractSocket::IPv4Protocol: return __a1.toIPv4Address() < __a2.toIPv4Address();
    case QAbstractSocket::IPv6Protocol: return __a1.toIPv6Address() < __a2.toIPv6Address();
    default:    EXCEPTION(EPROGFAIL);
    }
    return false;
}

int toIPv4Mask(const QHostAddress& __a, bool __ex)
{
    quint32 a = __a.toIPv4Address();
    //_DBGFN() << "@(" << __a.toString() << " (" << hex << a <<  "), " << DBOOL(__ex) << QChar(')') << endl;
    quint32 m = 0xffffffff;
    int     i = 32;
    for (;i >= 0; i--, m <<= 1) if (a == m) return i;
    if (__ex) EXCEPTION(EDATA);
    return -1;
}

Q_IPV6ADDR operator&(const Q_IPV6ADDR& __a, const Q_IPV6ADDR& __b)
{
    Q_IPV6ADDR r;
    for (int i = 0; i < 16; i++) r[i] = __a[i] & __b[i];
    return r;
}

bool operator==(const Q_IPV6ADDR& __a, const Q_IPV6ADDR& __b)
{
    for (int i = 0; i < 16; i++) if (__a[i] != __b[i]) return false;
    return true;

}

netAddress& netAddress::set(const QHostAddress& __a, int __m)
{
    if (__a.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol) {
        clear();
        return *this;
    }
    else {
        addr() = __a;
        if (__m < 0) {
            switch (__a.protocol()) {
            case QAbstractSocket::IPv4Protocol: mask() =  32;   break;
            case QAbstractSocket::IPv6Protocol: mask() = 128;   break;
            default:                            EXCEPTION(EPROGFAIL);
            }
        }
        else {
            mask() = __m;
        }
    }
    return masking();
}

netAddress& netAddress::set(quint32 __a, int __m)
{
    addr().setAddress(__a);
    mask() = __m;
    return masking();
}

netAddress& netAddress::set(const Q_IPV6ADDR& __a, int __m)
{
    addr().setAddress(__a);
    mask() = __m;
    return masking();
}

netAddress& netAddress::set(const QString& __n, int __m )
{
    // _DBGFN() << VDEB(__n) << endl;
    set(QHostAddress::parseSubnet(__n));
    if (__n.contains('/') && __m > 0) {
        setMask(__m);
    }
    // _DBGFNL() << VDEB(this->toString()) << endl;
    return *this;
}

netAddress& netAddress::setr(const QString& __n, int __m)
{
    if (set(__n, __m).    isValid()) return *this;
    if (setNetByName(__n).isValid()) return *this;
    return setAddressByName(__n);
}


netAddress& netAddress::setMask(int __m)
{
    if (__m > 0) {
        if      (isIPV4()) mask() = qMin( 32, __m);
        else if (isIPV6()) mask() = qMin(128, __m);
        else               mask() = -1;
    }
    if (mask() > 0) masking();
    return *this;
}

quint32 netAddress::bitmask(int __m)
{
    ulong bits = 0xffffffffUL;
    if (__m < 32) bits = ~(bits >> __m);   // Ha a proci hülye, attól még a fordító lehetne jó :-/
    return bits;
}
Q_IPV6ADDR netAddress::bitmask6(int __m)
{
    Q_IPV6ADDR bits;
    int m = qMax(128, __m);
    int n = m / 8;
    int b = m % 8;
    int i;
    for (i = 0; i < n; i++) bits[i] = 0xff;
    if (n == 16) {
        bits[i] = 0xff << b;
        for (i = n + 1; i < 16; ++i) bits[i] = 0;
    }
    return bits;
}

netAddress netAddress::operator&(const netAddress& __o) const
{
    netAddress r;   // Invalid
    // if (isNull() || __o.isNull()) return r;   // Bármelyik invalid, az eredmény is invalid, de ez az IPV4/IPV6 viszgálatnál kibukik.
    bool    f = false;
    if (isIPV4()) { // IPV4 címek
        if (!__o.isIPV4()) return r;    // Mindkét címnek IPV4-nek kell lennie.
        quint32 bm = bitmask(qMin(mask(), __o.mask()));
        f = ((bm & addr().toIPv4Address()) == (bm & __o.addr().toIPv4Address()));
    }
    else if (isIPV6()) {
        if (!__o.isIPV6()) return r;    // Mindkét címnek IPV6-nek kell lennie.
        Q_IPV6ADDR bm = bitmask6(qMin(mask(), __o.mask()));
        f = ((bm & addr().toIPv6Address()) == (bm & __o.addr().toIPv6Address()));
    }
    if (f) {
        r = mask() > __o.mask() ? *this : __o;
        // PDEB(VVERBOSE | cDebug::ADDRESS) << toString() << " & " << __o.toString() << " = " << r.toString() << endl;
    }
    else {
        // PDEB(VVERBOSE | cDebug::ADDRESS) << toString() << " & " << __o.toString() << " = <null>" << endl;
    }
    return r;
}

netAddress netAddress::operator|(const netAddress& __o) const
{
    netAddress r;   // Invalid
    // if (!*this || !__o) return r;   // Bármelyik invalid, az eredmény is invalid, de ez az IPV4/IPV6 viszgálatnál kibukik.
    if (isIPV4()) { // IPV4 címek
        if (!__o.isIPV4()) return r;    // Mindkét címnek IPV4-nek kell lennie.
        quint32 a  =     addr().toIPv4Address(),
                b  = __o.addr().toIPv4Address();
        int     am =     mask(),
                bm = __o.mask();
        quint32 mm = bitmask(qMin(am, bm));
        if ((mm & a) == (mm & b)) {     // Az egyik tartalmazza a másikat
            r = am < bm ? *this : __o;  // A nagyobb tartomány az eredmény
        }
        else {
            if (am == bm) {     // Ha somszédos tartományok?, uniójuk egy tartomány?
                mm = bitmask(am -1);
                if ((mm & a) == (mm & b)) {
                    r.mask() = am -1;
                    r.addr().setAddress(a & mm);
                }
            }
        }
    }
    else if (isIPV6()) {
        if (!__o.isIPV6()) return r;    // Mindkét címnek IPV6-nak kell lennie.
        Q_IPV6ADDR a  =     addr().toIPv6Address(),
                   b  = __o.addr().toIPv6Address();
        int        am =     mask(),
                   bm = __o.mask();
        Q_IPV6ADDR mm = bitmask6(qMin(am, bm));
        if ((mm & a) == (mm & b)) {     // Az egyik tartalmazza a másikat
            r = am < bm ? *this : __o;  // A nagyobb tartomány az eredmény
        }
        else {
            if (am == bm) {     // Ha somszédos tartományok?, uniójuk egy tartomány?
                mm = bitmask6(am -1);
                if ((mm & a) == (mm & b)) {
                    r.mask() = am -1;
                    r.addr().setAddress(a & mm);
                }
            }
        }
    }
    return r;
}

netAddressList netAddress::operator-(const netAddress& __a) const
{
    netAddressList r;
    if (*this &  __a) {
        if (mask() > __a.mask()) return r;  // Az eredmény üres
        if (isIPV4()) {
            for (netAddress addra = *this; addra != __a;) {
                // Kettévájuk a tartományt
                int     m = addra.mask();
                addra.mask()++;
                quint32 a = addr().toIPv4Address();
                quint32 b = a ^ (0x80000000UL >> m);
                netAddress  addrb(b, addra.mask());
                if (addrb & __a) addra.swap(addrb);
                r.add(addrb);
            }
        }
        else if (isIPV6()) {
            for (netAddress addra = *this; addra != __a;) {
                // Kettévájuk a tartományt
                int     m = addra.mask();
                addra.mask()++;
                Q_IPV6ADDR a = addr().toIPv6Address();
                Q_IPV6ADDR b = a;  //  &  (0x80000000UL >> m);
                complementOneBit(b, m);
                netAddress  addrb(b, addra.mask());
                if (addrb & __a) addra.swap(addrb);
                r.add(addrb);
            }
        }
        else {
            EXCEPTION(EPROGFAIL);   // Ez ugye nem lehetséges.
        }
        return r;
    }
    r.add(*this);   // Ha nincs metszet, nincs mit kivonni
    return r;
}

netAddress& netAddress::masking()
{
    if (isNull()) return clear();
    if (isAddress()) return *this;
    if (isIPV4()) {
        quint32 a,bits;
        a = addr().toIPv4Address();
        bits = bitmask();
        addr().setAddress(a & bits);
        // PDEB(VVERBOSE | cDebug::ADDRESS) << "netAddress::masking() " << addr().toString() << "/" << mask() << " => " << toString()
        //        << " bitmask : " << QString::number(bits, 16) << endl;
    }
    else {
        Q_IPV6ADDR a,bits;
        a = addr().toIPv6Address();
        bits = bitmask6();
        addr().setAddress(a & bits);
        // PDEB(VVERBOSE | cDebug::ADDRESS) << "netAddress::masking() " << addr().toString() << "/" << mask() << " => " << toString()
        //        << " bitmask : " << bits << endl;

    }
    return *this;
}

QString hostAddressToString(const QHostAddress& a)
{
    QString s = a.toString();
    if (s.contains(QChar('%'))) {   // IPV6 címeknél megjelenhet az interface neve egy '%' után
        s = s.section(QChar('%'), 0, 0);
    }
    return s;
}

QString netAddress::toString() const
{
    QString r;
    if (!*this) return r;
    r = hostAddressToString(addr());
    if (isSubnet()) r += QChar('/') + QString::number(mask());
    return r;
}

netAddress& netAddress::setNetByName(const QString& __nn)
{
    //_DBGFN() << VDEB(__nn) << endl;
    clear();
    QFile   networks("/etc/networks");
    if (networks.open(QIODevice::ReadOnly)) {
        QTextStream strm(&networks);
        QString     line;
        QRegExp     sep("\\s+");
        while (!(line = strm.readLine()).isNull()) {
            int i = line.indexOf(QString('#')); // kommentek
            if (i == 0) continue;
            if (i >  1) line = line.left(i);
            QStringList fields = line.split(sep,QString::SkipEmptyParts);
            if (fields.count() < 2) {
                DWAR() << "Invalid /etc/networks line : " << line;
                continue;
            }
            //PDEB(VVERBOSE | cDebug::ADDRESS) << "Find " << __nn << " from " << line << endl;
            QString net = fields[1];    // a második elem a net cím
            fields.removeAt(1);         // ha a cimet töröljük, csak a nevek maradnak
            QString name;
            foreach (name, fields) {
                if (name == __nn) {
                    while (net.endsWith(".0")) net.chop(2); // Nincs maszk, ettől lessz
                    if (!set(net)) {
                        DWAR() << "Invalid net address in /etc/networks line : " << line;
                    }
                    else {
                        // PDEB(VVERBOSE | cDebug::ADDRESS) << "Finded name, netAddress : " << toString() << endl;
                    }
                    break;
                }
            }
            if (*this)  break;
        }
        networks.close();
    }
    return *this;
}

netAddress& netAddress::setAddressByName(const QString& __hn)
{
    clear();
    QHostInfo   hi = QHostInfo::fromName(__hn);
    if (hi.error() == QHostInfo::NoError) {
        addr() = hi.addresses().front();
        mask() = addr().protocol() == QAbstractSocket::IPv6Protocol ? 128 : 32;
    }
    return *this;
}

/* ********************************************************************************************************
   *                                        class netAddressList                                          *
   ******************************************************************************************************** */

int netAddressList::add(const netAddress& __n)
{
    if (!__n) return NAL_NOMOD; // A semmi hozzáadása
    for (int i = 0; i < vec.size(); ++i) {
        if (vec[i] == __n) {    // Már van ilyen elem
            return NAL_NOMOD;   // A lista nem bővült
        }
        if (vec[i] &  __n) {     // A metszetük nem üres
            if (vec[i].mask() < __n.mask()) {  // Az új tartomány benne van egy másikban.
                return NAL_NOMOD;           // A lista nem bővölt
            }
            else {              // Ha az új a nagyobb, akkor ...
                vec[i] = __n;      // Kicseréljük
                return compress() ? NAL_MOD : i;    // elég a csere?
            }
        }
        if (vec[i] | __n) {  // Szomszédos tartományok, összevonhatóak?
            vec[i].mask()--;
            vec[i].masking();
            return compress() ? NAL_MOD : i;    // elég a csere?
        }
    }
    vec.push_back(__n);
    return NAL_APPEND;
}

bool netAddressList::add(const QList<QHostAddress>& __al)
{
    bool r = false;
    QHostAddress    a;
    foreach(a, __al) {
        r = add(a) != NAL_NOMOD || r;
    }
    return r;
}
bool netAddressList::add(const netAddressList& __al)
{
    bool r = false;
    netAddress  a;
    foreach(a, __al.vec) {
        r = add(a) != NAL_NOMOD || r;
    }
    return r;
}

bool netAddressList::compress()
{
    bool r = false;
    for (int i = 0; i < vec.size() -1; i++) {
        for (int j = i +1; j < vec.size(); j++) {
            if      (vec[i] & vec[j]) {    // Az egyik benne van a másikban, akkor az egyik nem kell
                if (vec[i].mask() < vec[j].mask()) vec.remove(j);
                else                               vec.remove(i);
                r = true;
                i = -1; break;  // kezdjük elölről
            }
            else if (vec[i] | vec[j]) {    // A két elem összevonható
                vec[i] = (vec[i] | vec[j]);  // Egy elemet csinálunk belőle
                vec.remove(j);
                r = true;
                i = -1; break;  // kezdjük elölről
            }
        }
    }
    return r;
}

netAddress netAddressList::pop()
{
    if (vec.size() == 0) EXCEPTION(ENOINDEX, 0, "netAddress object is empty.");
    netAddress  a = vec.back();
    vec.pop_back();
    return a;
}

QStringList netAddressList::toStringList() const
{
    QStringList r;
    netAddress  a;
    foreach (a, vec) {
        r.push_back(a.toString());
    }
    return r;
}

// NAL_FNDHIT  A kereset tartomány a talát tartománnyal megegyezik
// NAL_FNDLIT  A kereset tartomány a talát tartománynál kisebb
// NAL_FNDBIG  A kereset tartomány a talát tartománynál nagyobb, (index az első találat)
int netAddressList::find(const netAddress& __a) const
{
    for(int i = 0; i < vec.size(); i++) {
        if (vec[i] == __a)                  return i | NAL_FNDHIT;
        if (vec[i] &  __a) {
            if (vec[i].mask() < __a.mask()) return i | NAL_FNDLIT;
            else                            return i | NAL_FNDBIG;
        }
    }
    return -1;
}

// NAL_REMREM  egy elem eltávolítva (+ volt index)
// NAL_REMMOD  egy elem módosítva (+ index)
// NAL_MOD     több elem változott
int netAddressList::remove(const netAddress& __a)
{
    int i = find(__a);
    if (i == -1) return NAL_NOMOD;
    int flg = i & NAL_FNDMSK;
    int idx = i & NAL_IDXMSK;
    NAL_CHKINDEX(idx);
    switch (flg) {
    case NAL_FNDHIT:
        return remove(idx); // return idx | NAL_REMREM
    case NAL_FNDLIT: {
        netAddressList  al = vec[idx] - __a;
        if (!al) EXCEPTION(EPROGFAIL);  // képtelenség
        if (al.size() == 1) {
            vec[idx] = al[0];
            return idx | NAL_REMMOD;
        }
        else {
            remove(idx);
            add(al);
            return idx | NAL_REMMOD;
        }
    }
    case NAL_FNDBIG:
        remove(idx);
        return (remove(__a) == NAL_NOMOD) ? NAL_REMMOD | idx : NAL_MOD;
    }
    return -4;  // nem élő kód, csak hogy ne ugasson a fordító
}

QString netAddressList::toString() const
{
    QString s = QChar('[');
    foreach (netAddress a, vec) s += QChar(' ') + a.toString() + QChar(',');
    if (s.size() > 1) s.chop(1);
    return s + QChar(' ') + QChar(']');
}

/* *********************************************************************************************************** */

tIntVector   iTab(int a, int b, int c, int d, int e, int f, int g, int h)
{
    if (a == NULL_IX) return tIntVector();
    if (b == NULL_IX) return tIntVector(1, a);
    int i;
    if      (c == NULL_IX) i = 2;
    else if (d == NULL_IX) i = 3;
    else if (e == NULL_IX) i = 4;
    else if (f == NULL_IX) i = 5;
    else if (g == NULL_IX) i = 6;
    else if (h == NULL_IX) i = 7;
    else                   i = 8;
    tIntVector   r(i);
    switch (i) {
    case 8:     r[7] = h;
    case 7:     r[6] = g;
    case 6:     r[5] = f;
    case 5:     r[4] = e;
    case 4:     r[3] = d;
    case 3:     r[2] = c;
    case 2:     r[1] = b;
    }
    return r;
}

QString tIntVectorToString(const tIntVector& __iv)
{
    QString s = QChar('[');
    foreach (int i, __iv) s += QChar(' ') + QString::number(i) + QChar(',');
    if (s.size() > 1) s.chop(1);
    return s + QChar(']');
}

QString QBitArrayToString(const QBitArray& __ba)
{
    QString s = QChar('[');
    for (int i = 0; i <__ba.size(); ++i) s += (__ba.at(i) ? '1' : '0');
    return s + QChar(']');
}

QString QSqlRecordToString(const QSqlRecord& __r)
{
    int n = __r.count();
    QString r = "{ ";
    for (int i = 0; i < n; ++i) {
        r += __r.field(i).name();
        if (__r.isNull(i)) r += " IS NULL, ";
        else r += " = " + __r.value(i).toString() + _sCommaSp;
    }
    r.chop(2);
    return r + QChar('}');
}
 /* ***************************************************************************************************** */

// int _UMTID_QPoint        = QMetaType::UnknownType;
// int _UMTID_QPointF       = QMetaType::UnknownType;
int _UMTID_tPolygonF     = QMetaType::UnknownType;
int _UMTID_QHostAddress  = QMetaType::UnknownType;
int _UMTID_cMac          = QMetaType::UnknownType;
int _UMTID_netAddress    = QMetaType::UnknownType;
int _UMTID_netAddressList= QMetaType::UnknownType;

void initUserMetaTypes()
{
    if (_UMTID_cMac == QMetaType::UnknownType) {

//        qRegisterMetaType<QPoint>       (__sQPoint);
//        qRegisterMetaType<QPointF>      (__sQPointF);
        qRegisterMetaType<tPolygonF>     (__sTPolygonF);
        qRegisterMetaType<QHostAddress> (__sQHostAddress);
        qRegisterMetaType<cMac>         (__sCMac);
        qRegisterMetaType<netAddress>   (__sNetAddress);
        qRegisterMetaType<netAddressList>(__sNetAddressList);

//      _UMTID_QPoint          =  QMetaType::type(__sQPoint);
//      _UMTID_QPointF         =  QMetaType::type(__sQPointF);
        _UMTID_tPolygonF       =  QMetaType::type(__sTPolygonF);
        _UMTID_QHostAddress    =  QMetaType::type(__sQHostAddress);
        _UMTID_cMac            =  QMetaType::type(__sCMac);
        _UMTID_netAddress      =  QMetaType::type(__sNetAddress);
        _UMTID_netAddressList  =  QMetaType::type(__sNetAddressList);

//      if (_UMTID_QPoint         == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a QPoint típushoz.");
//      if (_UMTID_QPointF        == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a QPointF típushoz.");
        if (_UMTID_tPolygonF      == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a tPolygonF típushoz.");
        if (_UMTID_QHostAddress   == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a QHostAddress típushoz.");
        if (_UMTID_cMac           == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a cMac típushoz.");
        if (_UMTID_netAddress     == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a netAddress típushoz.");
        if (_UMTID_netAddressList == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a netAddressList típushoz.");
    }
}

QString QStringListToString(const QStringList& _v)
{
    return QChar('{') + _v.join(QChar(',')) + QChar('}');
}

QString QVariantListToString(const QVariantList& _v, bool *pOk)
{
    if (pOk != NULL) *pOk = true;
    QString r = QChar('{');
    if (_v.size() > 0) {
        bool ok = true;
        foreach (QVariant v, _v) {
            r += QVariantToString(v, &ok) + QChar(',');
            if (!ok && pOk != NULL) *pOk = false;
        }
        if (r.size() > 1) r.chop(1);
    }
    return r + QChar('}');
}

QString QPointTosString(const QPoint& p)
{
    return QString("(%1,%2)").arg(p.x()).arg(p.y());
}

QString QPointFTosString(const QPointF& p)
{
    return QString("(%1,%2)").arg(p.x()).arg(p.y());
}

QString tPolygonFToString(const tPolygonF& pol)
{
    QString r = QChar('(');
    if (pol.size() > 0) {
        foreach (QPointF p, pol) {
            r += QPointFTosString(p) + QChar(',');
        }
        r.chop(1);
    }
    return r + QChar(')');
}


QString QVariantToString(const QVariant& _v, bool *pOk)
{
    int type = _v.userType();
    if (pOk != NULL) *pOk = true;
    if (type < QMetaType::User) {
        switch (type) {
        case QMetaType::QStringList:    return QStringListToString(_v.toStringList());
        case QMetaType::QVariantList:   return QVariantListToString(_v.toList(), pOk);
        case QMetaType::QPoint:         return QPointTosString(_v.toPoint());
        case QMetaType::QPointF:        return QPointFTosString(_v.toPointF());
/*      case QMetaType::QPolygon:       return QPolygonToString(_v.value<QPolygon>());
        case QMetaType::QPolygonF:      return QPolygonFToString(_v.value<QPolygonF>()); */
        default:                        break;
        }
        if (pOk != NULL) *pOk = !_v.canConvert<QString>();
        return _v.toString();
    }
//    if (type == _UMTID_QPoint)          return QPointTosString(_v.value<QPoint>());
//    if (type == _UMTID_QPointF)         return QPointFTosString(_v.value<QPointF>());
    if (type == _UMTID_tPolygonF)       return tPolygonFToString(_v.value<tPolygonF>());
    if (type == _UMTID_QHostAddress)    return _v.value<QHostAddress>().toString();
    if (type == _UMTID_cMac)            return _v.value<cMac>().toString();
    if (type == _UMTID_netAddress)      return _v.value<netAddress>().toString();
    if (type == _UMTID_netAddressList)  return _v.value<netAddressList>().toString();
    if (pOk != NULL) *pOk = false;
    return QString();
}
