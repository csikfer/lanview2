#include "lanview.h"
#include "others.h"
#include <QCoreApplication>

/*!
@file lv2datab.h
Máshova nem sorolt objektumok, adat definíciók, ill. eljérésok.
*/

qlonglong enum2set(int (&f)(const QString &, bool), const QStringList &el, bool __ex)
{
    qlonglong r = 0;
    foreach (QString e, el) r |= enum2set(f, e, __ex);
    return r;
}

QStringList set2lst(const QString&(&f)(int e, bool __ex), qlonglong _set, bool __ex)
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

/******************************************************************************/

cFeatures& cFeatures::split(const QString& __ms, bool __ex)
{
    QString msg = QString(QObject::trUtf8("Invalid magic string : %1")).arg(quotedString(__ms));
    if (__ms.isEmpty()) return *this;
    QStringList sl = __ms.split(QChar(':'));
    // Első és utolsó karakter a szeparátor, tehát az első és utolsó elem üres
    if (sl.isEmpty() || !(sl.first().isEmpty() && sl.last().isEmpty())) {
        if (__ex) EXCEPTION(EDATA, -1, msg);
        DERR() << msg << endl;
        return *this;
    }
    if (sl.size() <= 2) return *this; // űres
    sl.pop_back();  // utolsó ures elem
    sl.pop_front(); // első üres elem
    foreach (QString s, sl) {
        QStringList pv = s.split(QChar('='));
        if (pv.count()  > 2) {
            if (__ex) EXCEPTION(EDATA, -1, msg);
            DERR() << msg << endl;
            return *this;
        }
        QString key = pv[0].toLower();
        QString val = pv.count() == 2 ? pv[1] : _sNul;
        if (val == "!") remove(key);        // Törli (ha van)
        else            (*this)[key] = val;
    }
    return *this;
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

qlonglong variantToId(const QVariant& v, bool _ex, qlonglong def)
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

qlonglong stringToId(const QString& v, bool _ex, qlonglong def)
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
