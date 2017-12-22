#include "lanview.h"
#include "others.h"
#include <QCoreApplication>

/*!
@file lv2datab.h
Máshova nem sorolt objektumok, adat definíciók, ill. eljérésok.
*/

qlonglong enum2set(int (&f)(const QString &, eEx), const QStringList &el, eEx __ex)
{
    qlonglong r = 0;
    foreach (QString e, el) r |= enum2set(f, e, __ex);
    return r;
}

QStringList set2lst(const QString&(&f)(int, eEx), qlonglong _set, eEx __ex)
{
    qlonglong b;
    int n;
    QStringList r;
    for (n = 0, b = 1 ; b <= _set && b != 0; ++n, b <<= 1) {
        const QString& s = f(n, __ex);
        if (_set & b) r << s;
    }
    return r;
}
const QString   nullString;

QString nameAndNumber(const QString& __pat, int __num, char __c)
{
    _DBGFN() << " @(" << __pat << QChar(',') << __num << QChar(',') << QChar(__c) << ")" << endl;
    QString n;
    if (__pat.contains(__c)) {
        QChar   c(__c);
        int first = __pat.indexOf(c);
        int last  = __pat.lastIndexOf(c);
        QString n = QString::number(__num);
        if (n.size() < (last - first +1)) n = QString(last - first +1 - n.size(), QChar('0')) + n;
        n = __pat.mid(0, first) + n + __pat.mid(last + 1);
        _DBGFNL() << " Retturn : " << n << endl;
        return n;
    }
    else {
        n = __pat + QString::number(__num);
        _DBGFNL() << " Retturn : " << n << endl;
        return n;
    }
}

QStringList splitBy(const QString& s, const QChar& sep, const QChar& esc)
{
    QStringList sl = s.split(sep);
    int i, n = sl.size() -1;
    for (i = 0; i < n; ++i) {
        QString& sr = sl[i];
        if (sr.endsWith(esc)) {
            sr.chop(1);     // drop esc
            sr += sep + sl.at(i +1);
            sl.removeAt(i+1);
            n--;
            i--;
        }
    }
    return sl;
}

/******************************************************************************/

bool cFeatures::split(const QString& __ms, eEx __ex)
{
    QString msg = QString(QObject::trUtf8("Invalid magic string : %1")).arg(quotedString(__ms));
    if (__ms.isEmpty()) return true;
    QStringList sl = splitBy(__ms);
    // Első és utolsó karakter a szeparátor, tehát az első és utolsó elem üres
    if (sl.isEmpty() || !(sl.first().isEmpty() && sl.last().isEmpty())) {
        if (__ex) EXCEPTION(EDATA, -1, msg);
        DERR() << msg << endl;
        return false;
    }
    if (sl.size() <= 2) return true; // űres
    sl.pop_back();  // utolsó ures elem
    sl.pop_front(); // első üres elem
    foreach (QString s, sl) {
        QStringList pv = splitBy(s, QChar('='));
        if (pv.count()  > 2) {
            if (__ex) EXCEPTION(EDATA, -1, msg);
            DERR() << msg << endl;
            return false;
        }
        QString key = pv[0].toLower();
        QString val = pv.count() == 2 ? pv[1] : _sNul;
        if (val == "!") remove(key);        // Törli (ha van)
        else            (*this)[key] = val;
    }
    return true;
}

/// Egy string stringList map-á felbontása:
/// <key>[<v1>,<v2>...],<key2>[<v2>...]
QMap<QString, QStringList> cFeatures::value2map(const QString& s)
{
    QMap<QString, QStringList>  r;
    if (s.isEmpty()) return r;
    QStringList sl = s.split(',');
    QStringList keyAndFirst = sl.first().split('[');
    if (keyAndFirst.size() != 2) EXCEPTION(EDATA, 0, s);
    QString key = keyAndFirst.first().simplified(); // first key
    sl[0] = keyAndFirst.at(1);
    int i = 0, n = sl.size() -1;    // last index
    QString v;
    QStringList vl;
    for (;;++i) {
        v = sl[i].simplified();
        if (i >= n) {                       // Last value for splitted strig
            if (v.endsWith(']')) v.chop(0);     // Last value on list (close list)
            else EXCEPTION(EDATA, 0, s);        // No closed list!!
            vl << v;
            r[key] = vl;
            break;
        }
        else {                              // Non last value for splitted strig
            if (v.endsWith(']')) {              // Last value on list (close list)
                v.chop(0);
                vl << v;
                r[key] = vl;
                vl.clear();
                v = sl[i +1].simplified();
                keyAndFirst = v.split('[');
                if (keyAndFirst.size() != 2) EXCEPTION(EDATA, 0, s);
                key = keyAndFirst.first().simplified(); // next key
                sl[i +1] = keyAndFirst.at(1);      // Next part is first value on list
            }
            else {
                vl << v;
            }
        }
    }
    return r;
}

QMap<int, qlonglong> cFeatures::mapEnum(QMap<QString, QStringList> smap, const QStringList& enums)
{
    QMap<int, qlonglong>    r;
    foreach (QString se, smap.keys()) {
        int e = enums.indexOf(se);
        if (e < 0) EXCEPTION(EDATA,0,se);
        qlonglong m = 0;
        foreach (QString ev, smap[se]) {
            int i = enums.indexOf(ev);
            if (i < 0) EXCEPTION(EDATA,0,ev);
            m |= enum2set(i);
        }
        r[e] = m;
    }
    return r;
}


QString cFeatures::join() const
{
    QString r = QChar(':');
    for (ConstIterator i = constBegin(); i != constEnd(); ++i) {
        r += i.key();
        if (!i.value().isEmpty()) r +=  "=" + i.value();
        r += QChar(':');
    }
    return r;
}

/******************************************************************************/

qlonglong variantToId(const QVariant& v, eEx _ex, qlonglong def)
{
    if (v.isNull()) {
        if (_ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Hiányzó numerikus adat."));
        return def;
    }
    bool    ok;
    qlonglong n = v.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA, -1, QObject::trUtf8("Nem numerikus adat."));
    return n;
}

qlonglong stringToId(const QString& v, eEx _ex, qlonglong def)
{
    if (v.isEmpty()) {
        if (_ex) EXCEPTION(EDATA, -1, QObject::trUtf8("Hiányzó numerikus adat."));
        return def;
    }
    bool    ok;
    qlonglong n = v.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA, -1, QObject::trUtf8("Nem numerikus adat."));
    return n;
}

qlonglong lko(qlonglong a, qlonglong b)
{
    if (a < 1 || b < 1) EXCEPTION(EDATA);
    while (a != b) {
        if (a>b) { a -= b; }
        else     { b -= a; }
    }
    return a;
}

/* ************************************************************************************** */

QString getEnvVar(const char * cnm)
{
#if   defined(Q_CC_MSVC)
    char *val;
    size_t requiredSize;
    getenv_s( &requiredSize, NULL, 0, cnm);
    if (requiredSize == 0) return QString();
    val = (char*) malloc(requiredSize * sizeof(char));
    if (!val) EXCEPTION(ESTD, ENOMEM);
    getenv_s(&requiredSize, val, requiredSize, cnm );
    QString r = QString::fromLatin1(val);
    free(val);
    return r;
#elif defined(Q_CC_GNU)
    const char * v = getenv(cnm);
    if (v == NULL) return QString();
    return QString::fromLatin1(v);
#endif
}

/* ************************************************************************************** */


QString getSysError(int eCode)
{
#if   defined(Q_CC_MSVC)
    static char buff[256];
    strerror_s(buff, 256, eCode);

    return QString::fromUtf8(buff);
#elif defined(Q_CC_GNU)
    return QString::fromUtf8(strerror(eCode));
#endif
}


void appReStart()
{
    QProcess::startDetached(QCoreApplication::applicationFilePath(), qApp->arguments());
    exit(123);
}

/* ***************************************************************************************** */

void writeRollLog(QFile& __log, const QByteArray& __data, qlonglong __size, int __arc)
{
    __log.write(__data);
    if (__size == 0) return;
    qint64 pos = __log.pos();
    if (__size < pos) {
        __log.close();
        QString     old;
        QString     pre;
        for (int i = __arc; i > 1; --i) {
            old = __log.fileName() + QChar('.') + QString::number(i);
            (void)QFile::remove(old);
            pre = __log.fileName() + QChar('.') + QString::number(i -1);
            QFile::rename(pre, old);
        }
        old = __log.fileName() + ".1";
        (void)QFile::remove(old);
        pre = __log.fileName();
        QFile::rename(pre, old);
        if (!__log.open(QIODevice::Append | QIODevice::WriteOnly)) {
            EXCEPTION(EFOPEN, -1, __log.fileName());
        }
    }
}

QString hrmlFrame(const QString& title, const QString& body)
{
    const QString frame =
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head> <title> %1 </title> </head>\n"
            "<body>\n %2\n</body>"
            "</html>\n";
    return frame.arg(title, body);
}


QString getParName(QString::const_iterator& i, const QString::const_iterator& e, bool _point, eEx __ex)
{
    QString r;
    if (i == e) return r;
    if (*i == QChar('{')) {
        for (++i; i != e && *i != QChar('}'); ++i) r += *i;
        if (i == e) {
            if (__ex) EXCEPTION(EDATA);
            return _sNul;
        }
        ++i;
        return r;
    }
    for (; i != e; ++i) {
        QChar c = *i;
        if (c.isLetterOrNumber() || c == QChar('_') || (_point && c == QChar('.'))) {
            r += c;
        }
        else {
            break;
        }
    }
    return r;
}
