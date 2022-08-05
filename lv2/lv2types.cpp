
#include <QtCore>

#include "cdebug.h"
#include "cerror.h"
#include "lv2types.h"
#include "lv2dict.h"
#include <float.h>
#include "lv2sql.h"

QString tristate2string(int e, eEx __ex)
{
    switch (e) {
    case TS_NULL:   return _sNull;
    case TS_TRUE:   return _sTrue;
    case TS_FALSE:  return _sFalse;
    default:        break;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

/************************ enum converters ************************/
QString sInvalidEnum() { return QObject::tr("Invalid"); }

QString ProcessError2String(QProcess::ProcessError __e)
{
    switch (__e) {
        case QProcess::FailedToStart:   return QObject::tr("FailedToStart");
        case QProcess::Crashed:         return QObject::tr("Crashed");
        case QProcess::Timedout:        return QObject::tr("Timedout");
        case QProcess::WriteError:      return QObject::tr("WriteError");
        case QProcess::ReadError:       return QObject::tr("ReadError");
        case QProcess::UnknownError:    return QObject::tr("UnknownError");
    }
    QString s = QString("QProcess::ProcessError %1 : %2").arg(sInvalidEnum()).arg(int(__e));
    DERR() << s << endl;
    return s;
}
QString ProcessError2Message(QProcess::ProcessError __e)
{
    switch (__e) {
        case QProcess::FailedToStart:
            return QObject::tr("The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.");
        case QProcess::Crashed:
            return QObject::tr("The process crashed some time after starting successfully.");
        case QProcess::Timedout:
            return QObject::tr("The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.");
        case QProcess::WriteError:
            return QObject::tr("An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.");
        case QProcess::ReadError:
            return QObject::tr("An error occurred when attempting to read from the process. For example, the process may not be running.");
        case QProcess::UnknownError:
            return QObject::tr("An unknown error occurred. This is the default return value of error().");
    }
    QString s = QString("QProcess::ProcessError %1 : %2").arg(sInvalidEnum()).arg(int(__e));
    DERR() << s << endl;
    return s;
}

QString ProcessState2String(QProcess::ProcessState __e)
{
    QString s;
    switch (__e){
        case QProcess::NotRunning:  return QObject::tr("NotRunning");
        case QProcess::Starting:    return QObject::tr("Starting");
        case QProcess::Running:     return QObject::tr("Running");
    }
    s = QString("QProcess::ProcessState %1 : %2").arg(sInvalidEnum()).arg(int(__e));
    DERR() << s << endl;
    return s;
}

QString dump(const QByteArray& __a)
{
    QString r = "[";
    foreach (char b, __a) {
        r += QString("%1 ").arg(int(uchar(b)),2,16,QChar('0'));
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
        r.push_front(QString(":%1").arg(int(m & 0x00ff), 2, 16, QChar('0')));
        m >>= 8;
    }
    return r.right(17);
}

const QString cMac::_sMacPattern1 = "^([A-F\\d]{6,12})^$";
const QString cMac::_sMacPattern2 = "^([A-F\\d]{1,2})[:-]([A-F\\d]{1,2})[:-]([A-F\\d]{1,2})[:-]([A-F\\d]{1,2})[:-]([A-F\\d]{1,2})[:-]([A-F\\d]{1,2})$";
const QString cMac::_sMacPattern3 = "^([A-F\\d]{1,6})-([A-F\\d]{1,6})$";
const QString cMac::_sMacPattern4 = "^(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)$";
const QString cMac::_sMacPattern5 = "^([A-F\\d]{1,2})\\s+([A-F\\d]{1,2})\\s+([A-F\\d]{1,2})\\s+([A-F\\d]{1,2})\\s+([A-F\\d]{1,2})\\s+([A-F\\d]{1,2})$";
cMac& cMac::set(const QString& __mac)
{
    QString s = __mac.simplified();
    // _DBGFN() << " @(" << s << ")" << endl;
    val = 0LL;
    if (s.isEmpty()) return *this;
    bool ok = true;
    QRegularExpression re;
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match;
    if (re.setPattern(_sMacPattern1), (match = re.match(s)).hasMatch()) {
        val = match.captured(1).toLongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
    }
    else if ((re.setPattern(_sMacPattern2), (match = re.match(s)).hasMatch())
          || (re.setPattern(_sMacPattern5), (match = re.match(s)).hasMatch())) {
        for (int i = 1; i <= 6; ++i) {
            val <<= 8;
            val |= match.captured(i).toInt(&ok, 16);
            if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
        }
    }
    else if (re.setPattern(_sMacPattern3), (match = re.match(s)).hasMatch()) {
        val |= match.captured(1).toLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
        val <<= 24;
        val |= match.captured(2).toLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL, -1, __mac);
    }
    else if (re.setPattern(_sMacPattern1), (match = re.match(s)).hasMatch()) {
        QString t(__mac);
        for (int i = 1; i <= 6; ++i) {
        int d = match.captured(i).toInt(&ok);
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
            val |= uchar(__mac[i]);
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
    else switch (__mac.userType()) {
    case QMetaType::LongLong:
    case QMetaType::ULongLong:  return set(__mac.toLongLong());
    case QMetaType::QString:    return set(__mac.toString());
    case QMetaType::QByteArray: return set(__mac.toByteArray());
    default:
        DERR() << "Invalid MAC QVariant type : " << __mac.typeName() << endl;
        val = -1LL;
        break;
    }

    return *this;
}

bool cMac::isValid(const QString& v)
{
    if (v.isEmpty()) return false;
    QRegularExpression re;
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    re.setPattern(_sMacPattern1);
    if (re.match(v).hasMatch()) return true;
    re.setPattern(_sMacPattern2);
    if (re.match(v).hasMatch()) return true;
    re.setPattern(_sMacPattern3);
    if (re.match(v).hasMatch()) return true;
    re.setPattern(_sMacPattern5);
    if (re.match(v).hasMatch()) return true;
    re.setPattern(_sMacPattern4);
    QRegularExpressionMatch match = re.match(v);
    if (match.hasMatch()) {
        for (int i = 1; i <= 6; ++i) {
            bool ok;
            int d = match.captured(i).toInt(&ok, 10);
            if (!ok) EXCEPTION(EPROGFAIL, -1, v);
            if (d > 255) return false;
        }
        return true;
    }
    return false;
}

bool cMac::isValid(const QByteArray& v)
{
    return v.size() == 6;
}

bool cMac::isValid(const QVariant& v)
{
    switch (v.userType()) {
    case QMetaType::LongLong:
    case QMetaType::ULongLong:   return isValid(v.toLongLong());
    case QMetaType::QByteArray:  return isValid(v.toByteArray());
    case QMetaType::QString:     return isValid(v.toString());
    }
    return false;
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

int toIPv4Mask(const QHostAddress& __a, eEx __ex)
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
    quint32 bits = 0xffffffffUL;
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
    if (n < 16) {
        if (b) bits[i++] = quint8(0xff << b);
        for (i = n; i < 16; ++i) bits[i] = 0;
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
        QRegularExpression     sep("\\s+");
        while (!(line = strm.readLine()).isNull()) {
            int i = line.indexOf(QString('#')); // kommentek
            if (i == 0) continue;
            if (i >  1) line = line.left(i);
            QStringList fields = line.split(sep, Q_SKIPEMPTYPARTS);
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

int netAddress::ipv4NetMask(const QString& _mask)
{
    bool ok;
    QStringList ml = _mask.split('.');
    if (ml.size() == 1) {
        int r = int(ml[0].toUInt(&ok));
        if (!ok || r > 32) return -1;
        return r;
    }
    if (ml.size() == 4) {
        uint iml[4];
        for (int i = 0; i < 4; ++i) {
            iml[i] = ml[4].toUInt(&ok);
            if (!ok || iml[i] > 255) return -1;
        }
        if ((iml[1] + iml[2] + iml[3]) == 0) switch (iml[0]) {
        case 0:     return  0;
        case 0x80:  return  1;
        case 0xc0:  return  2;
        case 0xe0:  return  3;
        case 0xf0:  return  4;
        case 0xf8:  return  5;
        case 0xfc:  return  6;
        case 0xfe:  return  7;
        case 0xff:  return  8;
        default:    return -1;
        }
        if (iml[0] != 255) return -1;
        if ((iml[2] + iml[3]) == 0) switch (iml[1]) {
        case 0x80:  return  9;
        case 0xc0:  return 10;
        case 0xe0:  return 11;
        case 0xf0:  return 12;
        case 0xf8:  return 13;
        case 0xfc:  return 14;
        case 0xfe:  return 15;
        case 0xff:  return 16;
        default:    return -1;
        }
        if (iml[1] != 255) return -1;
        if (iml[3] == 0) switch (iml[2]) {
        case 0x80:  return 17;
        case 0xc0:  return 18;
        case 0xe0:  return 19;
        case 0xf0:  return 20;
        case 0xf8:  return 21;
        case 0xfc:  return 22;
        case 0xfe:  return 23;
        case 0xff:  return 24;
        default:    return -1;
        }
        if (iml[2] != 255) return -1;
        switch (iml[3]) {
        case 0x80:  return 25;
        case 0xc0:  return 26;
        case 0xe0:  return 27;
        case 0xf0:  return 28;
        case 0xf8:  return 29;
        case 0xfc:  return 30;
        case 0xfe:  return 31;
        case 0xff:  return 32;
        default:    return -1;
        }
    }
    return -1;
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
    tIntVector r(1,a);
    if (b == NULL_IX) return r;
    r << b;
    if (c == NULL_IX) return r;
    r << c;
    if (d == NULL_IX) return r;
    r << d;
    if (e == NULL_IX) return r;
    r << e;
    if (f == NULL_IX) return r;
    r << f;
    if (g == NULL_IX) return r;
    r << g;
    if (h == NULL_IX) return r;
    r << h;
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

int _UMTID_tPolygonF     = QMetaType::UnknownType;
int _UMTID_QHostAddress  = QMetaType::UnknownType;
int _UMTID_cMac          = QMetaType::UnknownType;
int _UMTID_netAddress    = QMetaType::UnknownType;
int _UMTID_netAddressList= QMetaType::UnknownType;

void initUserMetaTypes()
{
    if (_UMTID_cMac == QMetaType::UnknownType) {

        _UMTID_tPolygonF       =  qRegisterMetaType<tPolygonF>     (__sTPolygonF);
        _UMTID_QHostAddress    =  qRegisterMetaType<QHostAddress>  (__sQHostAddress);
        _UMTID_cMac            =  qRegisterMetaType<cMac>          (__sCMac);
        _UMTID_netAddress      =  qRegisterMetaType<netAddress>    (__sNetAddress);
        _UMTID_netAddressList  =  qRegisterMetaType<netAddressList>(__sNetAddressList);

        if (_UMTID_tPolygonF      == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a tPolygonF típushoz.");
        if (_UMTID_QHostAddress   == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a QHostAddress típushoz.");
        if (_UMTID_cMac           == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a cMac típushoz.");
        if (_UMTID_netAddress     == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a netAddress típushoz.");
        if (_UMTID_netAddressList == QMetaType::UnknownType) EXCEPTION(EPROGFAIL, -1, "Nincs QMetaType ID a netAddressList típushoz.");
    }
}

QString QStringListToString(const QStringList& _v)
{
    if (_v.isEmpty()) return "{}";
    QString r = QChar('{');
    foreach (QString s, _v) {
        r += s + QChar(',');
    }
    r.chop(1);
    return r + QChar('}');
}

QString _QVariantListToString(const QVariantList& _v, bool *pOk)
{
    if (pOk != nullptr) *pOk = true;
    QString r;
    if (_v.size() > 0) {
        bool ok = true;
        foreach (QVariant v, _v) {
            r += QVariantToString(v, &ok) + QChar(',');
            if (!ok && pOk != nullptr) *pOk = false;
        }
        if (r.size() > 1) r.chop(1);
    }
    return r;
}

QString QVariantListToString(const QVariantList& _v, bool *pOk)
{
    QString r = QChar('{');
    r += _QVariantListToString(_v, pOk);
    return r + QChar('}');
}

QString QPointTosString(const QPoint& p)
{
    return QString("(%1,%2)").arg(p.x()).arg(p.y());
}

QString QPointFTosString(const QPointF& p)
{
    return QString("[%1,%2]").arg(p.x()).arg(p.y());
}

QString tPolygonFToString(const tPolygonF& pol)
{
    QString r = QChar('{');
    if (pol.size() > 0) {
        foreach (QPointF p, pol) {
            r += QPointFTosString(p) + QChar(',');
        }
        r.chop(1);
    }
    return r + QChar('}');
}


QString QVariantToString(const QVariant& _v, bool *pOk)
{
    int type = _v.userType();
    if (pOk != nullptr) *pOk = true;
    if (type < QMetaType::User) {
        switch (type) {
        case QMetaType::QStringList:    return QStringListToString(_v.toStringList());
        case QMetaType::QVariantList:   return QVariantListToString(_v.toList(), pOk);
        case QMetaType::QPoint:         return QPointTosString(_v.toPoint());
        case QMetaType::QPointF:        return QPointFTosString(_v.toPointF());
/*      These are only when we are compiled GUI !!! Crashes !!!
        case QMetaType::QPolygon:       return QPolygonToString(_v.value<QPolygon>());
        case QMetaType::QPolygonF:      return QPolygonFToString(_v.value<QPolygonF>()); */
        default:                        break;
        }
        if (pOk != nullptr) *pOk = _v.canConvert<QString>();
        return _v.toString();
    }
//    if (type == _UMTID_QPoint)          return QPointTosString(_v.value<QPoint>());
//    if (type == _UMTID_QPointF)         return QPointFTosString(_v.value<QPointF>());
    if (type == _UMTID_tPolygonF)       return tPolygonFToString(_v.value<tPolygonF>());
    if (type == _UMTID_QHostAddress)    return _v.value<QHostAddress>().toString();
    if (type == _UMTID_cMac)            return _v.value<cMac>().toString();
    if (type == _UMTID_netAddress)      return _v.value<netAddress>().toString();
    if (type == _UMTID_netAddressList)  return _v.value<netAddressList>().toString();
    if (pOk != nullptr) *pOk = false;
    return QString();
}

// Language localization

int setLanguage(QSqlQuery& q, const QString& _l, const QString& _c)
{
    QVariant l, c;
    if (_l.size() == 2) {
        l = _l;
        if (!_c.isEmpty()) {
            if (_c.size() == 2) c = _c;
            else EXCEPTION(ESETTING, 0, QObject::tr("Invalid country : %1, (languagr : %2)").arg(_c, _l));
        }
    }
    else if (_l.size() == 5 && _c.isNull()) {
        QStringList sl = _l.split("_");
        if (sl.size() == 2 && sl.first().size() == 2 && sl.at(1).size() == 2) {
            l = sl.first();
            c = sl.at(1);
        }
    }
    if (l.isNull()) EXCEPTION(ESETTING, 0, QObject::tr("Invalid language : %1, (country : %2)").arg(_l, _c));
    qlonglong r = execSqlIntFunction(q, nullptr, "set_language", l, c);
    return int(r);
}

int setLanguage(QSqlQuery& q, int id)
{
    qlonglong r = execSqlIntFunction(q, nullptr, "set_language_id", id);;
    return int(r);
}

int getLanguageId(QSqlQuery& q)
{
    qlonglong id = execSqlIntFunction(q, nullptr, "get_language_id");
    return int(id);
}

QString getLanguage(QSqlQuery& q, int lid)
{
    return execSqlTextFunction(q, "language_id2code", lid);
}

QString textId2text(QSqlQuery& q, qlonglong id, const QString& _table, int index)
{
    static const QString sql = "SELECT (localization_texts(?, ?)).texts[?]";
    execSql(q, sql, id, _table, index +1);
    return q.value(0).toString();
}

QStringList textId2texts(QSqlQuery& q, qlonglong id, const QString& _table)
{
    static const QString sql = "SELECT unnest((localization_texts(?, ?)).texts)";
    QStringList sl;
    if (execSql(q, sql, id, _table)) {
        do {
            sl << q.value(0).toString();
        } while (q.next());
    }
    return sl;
}

int textName2ix(QSqlQuery &q, const QString& _t, const QString& _n, eEx __ex)
{
    QString sql = QString("SELECT array_length(enum_range(NULL, ?::%1), 1)").arg(_t);
    execSql(q, sql, _n);
    bool ok;
    int ix = q.value(1).toInt(&ok);
    if (!ok) ix = -1;
    if (ix < 0 && __ex != EX_IGNORE) EXCEPTION(EENUMVAL, ix, mCat(_t, _n));
    return ix;
}

int paramTypeType(const QString& __n, eEx __ex)
{
    if (0 == __n.compare(_sText,    Qt::CaseInsensitive))   return PT_TEXT;
    if (0 == __n.compare(_sBoolean, Qt::CaseInsensitive))   return PT_BOOLEAN;
    if (0 == __n.compare(_sInteger, Qt::CaseInsensitive))   return PT_INTEGER;
    if (0 == __n.compare(_sReal,    Qt::CaseInsensitive))   return PT_REAL;
    if (0 == __n.compare(_sInterval,Qt::CaseInsensitive))   return PT_INTERVAL;
    if (0 == __n.compare(_sDate,    Qt::CaseInsensitive))   return PT_DATE;
    if (0 == __n.compare(_sTime,    Qt::CaseInsensitive))   return PT_TIME;
    if (0 == __n.compare(_sDateTime,Qt::CaseInsensitive))   return PT_DATETIME;
    if (0 == __n.compare(_sINet,    Qt::CaseInsensitive))   return PT_INET;
    if (0 == __n.compare(_sCidr,    Qt::CaseInsensitive))   return PT_CIDR;
    if (0 == __n.compare(_sMac,     Qt::CaseInsensitive))   return PT_MAC;
    if (0 == __n.compare(_sPoint,   Qt::CaseInsensitive))   return PT_POINT;
    if (0 == __n.compare(_sByteA,   Qt::CaseInsensitive))   return PT_BYTEA;
    if (__ex != EX_IGNORE)  EXCEPTION(EDATA, -1, __n);
    return ENUM_INVALID;
}

const QString& paramTypeType(int __e, eEx __ex)
{
    switch (__e) {
    case PT_TEXT:       return _sText;
    case PT_BOOLEAN:    return _sBoolean;
    case PT_INTEGER:    return _sInteger;
    case PT_REAL:       return _sReal;
    case PT_INTERVAL:   return _sInterval;
    case PT_DATE:       return _sDate;
    case PT_TIME:       return _sTime;
    case PT_DATETIME:   return _sDateTime;
    case PT_INET:       return _sINet;
    case PT_CIDR:       return _sCidr;
    case PT_MAC:        return _sMac;
    case PT_POINT:      return _sPoint;
    case PT_BYTEA:      return _sByteA;
    }
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, __e);
    return _sNul;
}


int filterType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sNo,       Qt::CaseInsensitive)) return FT_NO;
    if (0 == n.compare(_sBegin,    Qt::CaseInsensitive)) return FT_BEGIN;
    if (0 == n.compare(_sLike,     Qt::CaseInsensitive)) return FT_LIKE;
    if (0 == n.compare(_sSimilar,  Qt::CaseInsensitive)) return FT_SIMILAR;
    if (0 == n.compare(_sRegexp,   Qt::CaseInsensitive)) return FT_REGEXP;
    if (0 == n.compare(_sEqual,    Qt::CaseInsensitive)) return FT_EQUAL;
    if (0 == n.compare(_sLitle,    Qt::CaseInsensitive)) return FT_LITLE;
    if (0 == n.compare(_sBig,      Qt::CaseInsensitive)) return FT_BIG;
    if (0 == n.compare(_sInterval, Qt::CaseInsensitive)) return FT_INTERVAL;
    if (0 == n.compare(_sBoolean,  Qt::CaseInsensitive)) return FT_BOOLEAN;
    if (0 == n.compare(_sEnum,     Qt::CaseInsensitive)) return FT_ENUM;
    if (0 == n.compare(_sSet,      Qt::CaseInsensitive)) return FT_SET;
    if (0 == n.compare(_sNull,     Qt::CaseInsensitive)) return FT_NULL;
    if (0 == n.compare(_sSQL,      Qt::CaseInsensitive)) return FT_SQL_WHERE;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, n);
    return FT_UNKNOWN;
}

const QString& filterType(int e, eEx __ex)
{
    switch(e) {
    case FT_NO:         return _sNo;
    case FT_BEGIN:      return _sBegin;
    case FT_LIKE:       return _sLike;
    case FT_SIMILAR:    return _sSimilar;
    case FT_REGEXP:     return _sRegexp;
    case FT_EQUAL:      return _sEqual;
    case FT_LITLE:      return _sLitle;
    case FT_BIG:        return _sBig;
    case FT_INTERVAL:   return _sInterval;
    case FT_BOOLEAN:    return _sBoolean;
    case FT_ENUM:       return _sEnum;
    case FT_SET:        return _sSet;
    case FT_NULL:       return _sNull;
    case FT_SQL_WHERE:  return _sSQL;
    default:            break;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

QString nameToCast(int __e, eEx __ex)
{
    switch (__e) {
    case PT_TEXT:
    case PT_BYTEA:
        if (__ex >= EX_WARNING)   EXCEPTION(EDATA, __e);
        return _sNul;
    case PT_BOOLEAN:    return "cast_to_boolean";
    case PT_INTEGER:    return "cast_to_integer";
    case PT_REAL:       return "cast_to_real";
    case PT_DATE:       return "cast_to_date";
    case PT_TIME:       return "cast_to_time";
    case PT_DATETIME:   return "cast_to_datetime";
    case PT_INTERVAL:   return "cast_to_interval";
    case PT_INET:       return "cast_to_inet";
    case PT_CIDR:       return "cast_to_cidr";
    case PT_MAC:        return "cast_to_mac";
    case PT_POINT:      return "cast_to_point";
    }
    if (__ex != EX_IGNORE)   EXCEPTION(EDATA, __e);
    return _sNul;
}

