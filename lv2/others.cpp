#include <QCoreApplication>
#include <QTextStream>
#include "lanview.h"
#include "others.h"
// #include "lv2types.h"
#if defined(Q_OS_WINDOWS)
#  include <Windows.h>
#  include <wtsapi32.h>
#endif  // defined(Q_OS_WINDOWS)

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

int onCount(qlonglong _set)
{
    int r = 0;
    for (qlonglong m = 1; _set >= m; m <<= 1)  {
        if (_set & m) ++r;
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

int removeAll(QStringList& sl, const QString& s)
{
    int r = 0;
    QStringList::iterator i = sl.begin();
    QStringList::iterator e = sl.end();
    while (i < e) {
        if (0 == i->compare(s, Qt::CaseInsensitive)) {
            i = sl.erase(i);
            e = sl.end();
            ++r;
        }
        else {
            i++;
        }
    }
    return r;
}
/******************************************************************************/

const QString cFeatures::list_sep = ",";
const QString cFeatures::list_esep = "\\,";

bool cFeatures::split(const QString& __ms, bool merge, eEx __ex)
{
    static const QString msg = QObject::tr("Invalid magic string %1 : %2");
    static const QChar sep = QChar(':');
    int i, n = __ms.size();
    if (n == 0 || (n == 1 && __ms[0] == sep)) return true;  // Empty, ok.
    QString field;
    if (__ms[0] != sep) {   // The first character must be a separator
        if (__ex) EXCEPTION(EDATA, -1, msg + __ms);
        DERR() << msg.arg(QObject::tr("Missing first separator character.")).arg(__ms) << endl;
        return false;
    }
    QStringList sl;
    for (i = 1; i < n; ++i) {   // split
        QChar c = __ms[i];
        if (c == sep) {
            QChar next;
            if (n > i +1) {    // If no last separator
                next = __ms[i +1];
            }
            if (next == sep) {
                field += sep;
                ++i;
            }
            else {  // Real separator
                if (field.isEmpty()) {
                    if (__ex) EXCEPTION(EDATA, -1, msg + __ms);
                    DERR() << msg.arg(_sNul).arg(__ms) << endl;
                    return false;
                }
                else {
                   sl << field;
                   field.clear();
                }
            }
        }
        else {
            field += c;
        }
    }
    if (!field.isEmpty()) {
        if (__ex) EXCEPTION(EDATA, -1, msg + __ms);
        DERR() << msg.arg(QObject::tr("Missing last separator character.")).arg(__ms) << endl;
        return false;
    }
    if (sl.isEmpty()) {
        EXCEPTION(EPROGFAIL);
    }
    foreach (QString s, sl) {
        QStringList pv = splitBy(s, QChar('='));
        if (pv.count()  > 2) {
            pv[1] = pv.mid(1).join(QChar('='));
        }
        QString key = pv[0].toLower();
        QString val = pv.count() >= 2 ? pv[1] : _sNul;
        if (merge && val == "!") remove(key);        // Clear
        else            (*this)[key] = val;
    }
    return true;
}

QStringList cFeatures::value2list(const QString &s)
{
    QStringList r;
    if (!s.isEmpty()) {
        r = s.split(list_sep);
        for (int i = 0; i < r.size(); ++i) {
            while (r.at(i).endsWith(QChar('\\'))) {
                if ((i +1) >= r.size()) EXCEPTION(EDATA, 0, s);
                r[i].chop(1);
                r[i] += QChar(',') + r[i +1];
                r.removeAt(i +1);
            }
            r[i] = r[i].trimmed();
        }
    }
    return r;
}

QString cFeatures::list2value(const QStringList& l)
{
    QString r;
    if (!l.isEmpty()) {
        foreach (QString s, l) {
            s = s.replace(list_sep, list_esep);
            r += s + list_sep;
        }
        r.chop(list_sep.size());
    }
    return r;
}

QString cFeatures::list2value(const QVariantList& l, bool f)
{
    QString r;
    f = f && l.size() == 1;
    if (!l.isEmpty()) {
        foreach (QVariant v, l) {
            QString s;
            int t = v.userType();
            if (metaIsInteger(t) || metaIsFloat(t) || metaIsString(t)) s = v.toString();
            else if (t == _UMTID_netAddress)    s = v.value<netAddress>().toString();
            else if (t == _UMTID_QHostAddress)  s = v.value<QHostAddress>().toString();
            else if (t == _UMTID_cMac)          s = v.value<cMac>().toString();
            else                                EXCEPTION(EDATA, t, QObject::tr("Unsupported data type : %1").arg(debVariantToString(v)));
            if (f) return  s;
            s = s.replace(list_sep, list_esep);
            r += s + list_sep;
        }
        r.chop(list_sep.size());
    }
    return r;
}

qlonglong cFeatures::value2set(const QString &s, const QStringList& enums)
{
    qlonglong m = 0;
    QStringList sl = value2list(s);
    foreach (QString ev, sl) {
        int i = enums.indexOf(ev);
        if (i < 0) EXCEPTION(EDATA,0,ev);
        m |= enum2set(i);
    }
    return m;
}

qlonglong cFeatures::eValue(const QString &_nm, const QStringList& enums) const
{
    qlonglong r = 0;
    if (contains(_nm)) {
        r = value2set(value(_nm), enums);
    }
    return r;
}

QMap<QString, QStringList> cFeatures::sListMapValue(const QString &_nm) const
{
    QMap<QString, QStringList> r;
    if (contains(_nm)) {
        r =  value2listMap(value(_nm));
    }
    return r;
}
QMap<int, qlonglong> cFeatures::eMapValue(const QString &_nm, const QStringList& enums) const
{
    QMap<int, qlonglong>  r;
    if (contains(_nm)) {
        r = mapEnum(sListMapValue(_nm), enums);
    }
    return r;
}

tStringMap cFeatures::value2map(const QString& _s)
{
    tStringMap r;
    QString key;
    QString val;
    QString s = _s.simplified();
    QRegExp re("^(\\w+)\\[([^\\]]*)\\](.*)$");
    while (re.exactMatch(s)) {
        key = re.cap(1);
        val = re.cap(2);
        r[key] = val;
        s = re.cap(3);
        if (!s.startsWith(QChar(','))) break; // Missing ',' or empty
        s = s.mid(1);
    }
    return r;
}

QString cFeatures::map2value(const tStringMap& _map)
{
    QString r;
    if (!_map.isEmpty()) {
        foreach (QString key, _map.keys()) {
            QString val = _map[key];
            r += key + "[" + val + "],";
        }
        r.chop(1);
    }
    return r;
}


/// Egy string stringList map-á felbontása:
/// <key>[<v1>,<v2>...],<key2>[<v2>...]
QMap<QString, QStringList> cFeatures::value2listMap(const QString& s)
{
    QMap<QString, QStringList>  r;
#if 1
    tStringMap m = value2map(s);
    QRegExp re("\\s*,\\s*");
    foreach (QString key, m.keys()) {
        QString val = m[key];
        r[key] = val.split(re);
    }
#else
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
            if (v.endsWith(']')) v.chop(1);     // Last value on list (close list)
            else EXCEPTION(EDATA, 0, s);        // No closed list!!
            vl << v;
            r[key] = vl;
            break;
        }
        else {                              // Non last value for splitted strig
            if (v.endsWith(']')) {              // Last value on list (close list)
                v.chop(1);
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
#endif
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


QString cFeatures::join(const tStringMap &_sm)
{
    static const QString sep = ":";
    static const QString esep = "::";
    QString r = sep;
    for (ConstIterator i = _sm.constBegin(); i != _sm.constEnd(); ++i) {
        QString s = i.key();
        if (!i.value().isEmpty()) s +=  "=" + i.value();
        s.replace(sep, esep);
        r += s + sep;
    }
    return r;
}

cFeatures& cFeatures::merge(const cFeatures& _o, const QString& _cKey)
{
    QString cKey = _cKey + QChar('.');
    int cKeySize = cKey.size();
    foreach (QString key, _o.keys()) {
        QString val;
        if (cKeySize > 1) {
            if (!key.startsWith(cKey)) continue;
            val = _o.value(key);
            key = key.mid(cKeySize);
        }
        else {
            val = _o.value(key);
        }
        if (val == "!") remove(key);
        else {
            if (val.count() == val.count(QChar('!'))) val.chop(1);
            insert(key, val);
        }
    }
    return *this;
}

cFeatures& cFeatures::selfMerge(const QString& _cKey)
{
    tStringMap m;
    QString k = _cKey + '.';
    foreach (QString key, keys()) {
        if (key.startsWith(k)) {
            m[key] = take(key);
        }
    }
    return merge(m, _cKey);
}

void cFeatures::modifyField(cRecordFieldRef& _fr)
{
    QString key = _fr.columnName();
    QString val = value(key);
    if (val.isEmpty()) return;
    const cColEnumType *pEnumType = _fr.descr().getPEnumType();
    if (pEnumType == nullptr || _fr.descr().eColType == cColStaticDescr::FT_ENUM) { // Is not set or text_id
        if (val == "!") _fr.clear();
        else            _fr = val;
    }
    else if (_fr.descr().fKeyType == cColStaticDescr::FT_TEXT_ID) {
        ;   // talán majd késöbb ...
    }
    else if (_fr.descr().eColType == cColStaticDescr::FT_SET) {
        QStringList sl = value2list(val);
        QVariant fr = _fr;
        QStringList set = fr.toStringList();
        foreach (QString ev, sl) {
            if (ev.startsWith(QChar('!')) || ev.startsWith(QChar('-'))) {
                ev = ev.mid(1);
                set.removeAll(ev);
            }
            else {
                if (!set.contains(ev)) set << ev;
            }
        }
        _fr = set;
    }
}

/******************************************************************************/

const QString _sStartTimeout = "start_timeout";
const QString _sStopTimeout  = "stop_timeout";

ulong getTimeout(cFeatures& __f, const QString& _key, ulong _def)
{
    ulong r = _def;
    bool ok;
    ulong to;
    QString sto;
    r = _def;
    sto = __f.value(_key);
    if (sto.isEmpty()) sto = __f.value(_sTimeout);
    if (!sto.isEmpty()) {
        to = sto.toULong(&ok);  // Integer [mSec]
        if (ok) r = to;
        else {                  // Interval string (ISO or SQL)
            qlonglong _to = parseTimeInterval(sto, &ok);
            if (ok && _to > 0) r = ulong(_to);
        }
    }
    return r;
}

/******************************************************************************/

qlonglong variantToId(const QVariant& v, eEx _ex, qlonglong def)
{
    if (v.isNull()) {
        if (_ex) EXCEPTION(EDATA, -1, QObject::tr("Hiányzó numerikus adat."));
        return def;
    }
    bool    ok;
    qlonglong n = v.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA, -1, QObject::tr("Nem numerikus adat."));
    return n;
}

qlonglong stringToId(const QString& v, eEx _ex, qlonglong def)
{
    if (v.isEmpty()) {
        if (_ex) EXCEPTION(EDATA, -1, QObject::tr("Hiányzó numerikus adat."));
        return def;
    }
    bool    ok;
    qlonglong n = v.toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA, -1, QObject::tr("Nem numerikus adat."));
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
    if (v == nullptr) return QString();
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
    printf(" -- appReStart(): EXIT 123\n");
    exit(123);
}

int startProcessAndWait(const QString& cmd, const QStringList& args, QString *pMsg, int start_to, int stop_to)
{
    QProcess p;
    int r = startProcessAndWait(p, cmd, args, pMsg, start_to, stop_to);
    if (r >= 0) {
        QString s;
        msgAppend(pMsg, QObject::tr("Parancs : \"%1\"").arg(joinCmd(cmd, args)));
        msgAppend(pMsg, QObject::tr("Visszatérési érték : %1").arg(r));
        s = p.readAllStandardError();
        if (!s.isEmpty()) msgAppend(pMsg, QObject::tr("Hiba csatorna : \"%1\"").arg(s));
        s = p.readAllStandardOutput();
        if (!s.isEmpty()) msgAppend(pMsg, QObject::tr("Standard csatorna : \"%1\"").arg(s));
    }
    return r;
}

int startProcessAndWait(QProcess& p, const QString& cmd, const QStringList& args, QString *pMsg, int start_to, int stop_to)
{
    PDEB(VERBOSE) << cmd << _sSpace << args.join(_sSpace) << endl;
    if (pMsg != nullptr) pMsg->clear();
    QString msg;
    p.setProcessChannelMode(QProcess::MergedChannels);
    if (args.isEmpty()) p.start(cmd, QIODevice::ReadOnly);
    else          p.start(cmd, args, QIODevice::ReadOnly);
    int r = -1;
    if (p.waitForStarted(start_to)) {
        if (p.waitForFinished(stop_to)) {
            r = p.exitCode();
            if (r < 0) {
                msg = msgAppend(pMsg, QObject::tr("Run `%1` command failed (%2 [ms]), state/error : %3/%4")
                          .arg(joinCmd(cmd, args))
                          .arg(stop_to)
                          .arg(ProcessState2String(p.state()),ProcessError2String(p.error()))
                          );
                r = PE_ERROR;
                PDEB(DERROR) << msg << endl;
            }
            else {
                msg = msgAppend(pMsg, QObject::tr("%1 return : #%2").arg(cmd).arg(r));
                if (r) {
                    PDEB(WARNING) << msg << endl;
                }
                else {
                    PDEB(VERBOSE) << msg << endl;
                }
            }
        }
        else {
            msg = msgAppend(pMsg, QObject::tr("Wait stop `%1` command failed (%2 [ms]), state/error : %3/%4")
                      .arg(joinCmd(cmd, args))
                      .arg(stop_to)
                      .arg(ProcessState2String(p.state()),ProcessError2String(p.error()))
                      );
            r = PE_STOP_TIME_OUT;
            PDEB(DERROR) << msg << endl;
        }
    }
    else {
        msg = msgAppend(pMsg, QObject::tr("Start `%1` command failed (%2 [ms]), state/error : %3/%4")
                  .arg(joinCmd(cmd, args))
                  .arg(start_to)
                  .arg(ProcessState2String(p.state()),ProcessError2String(p.error()))
                  );
        r = PE_START_TIME_OUT;
        PDEB(DERROR) << msg << endl;
    }
    return r;
}
/* ***************************************************************************************** */

void writeRollLog(QFile& __log, const QByteArray& __data, qlonglong __size, int __arc)
{
    if (__data.isEmpty()) return;
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
    else {
        __log.flush();
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

QString substitute(QSqlQuery& q, void *p, const QString& str, tGetValueFnPtr fn)
{
    QString::const_iterator i = str.constBegin();
    char separator = 0;
    QString substituted;   // Command substituted
    // Substitute
    while (i != str.constEnd()) {
        char c = i->toLatin1();
        QChar qc = *i;
        ++i;
        if (separator != 0 && c == separator) {
            separator = 0;
            substituted += qc;
            continue;
        }
        if (c == '\'') {
            separator = c;
            substituted += qc;
            continue;
        }
        if (c == '\\') {
            if (i != str.constEnd()) {
                substituted += qc;
                substituted += *i;
                ++i;
            }
            continue;
        }
        if (c == '$') {
            if (i->toLatin1() == '$') {
                ++i;
            }
            else {
                QString vname;
                vname = getParName(i, str.constEnd());
                substituted += fn(vname, q, p);
                continue;
            }
        }
        substituted += qc;
    }
    return substituted;
}

QStringList splitArgs(const QString& cmd)
{
    QStringList r;
    char separator = 0;
    QString arg;
    bool prevSpace = false;
    QString::const_iterator i = cmd.constBegin();
    while (i != cmd.constEnd()) {
        char c = i->toLatin1();
        QChar qc = *i;
        ++i;
        if (separator == 0 && qc.isSpace()) {
            if (prevSpace) continue;
            prevSpace = true;
            r << arg;
            arg.clear();
            continue;
        }
        prevSpace = false;
        if (separator != 0 && c == separator) {
            separator = 0;
            continue;
        }
        if (c == '"' || c == '\'') {
            separator = c;
            continue;
        }
        if (c == '\\') {
            if (i != cmd.constEnd()) {
                arg += *i;
                ++i;
            }
            continue;
        }
        arg += qc;
    }
    r << arg;                // Utolsó paraméter
    return r;
}


QVariantList list_longlong2variant(const QList<qlonglong> &v)
{
    QVariantList r;
    foreach (qlonglong i, v) {
        r << QVariant(i);
    }
    return r;
}

/* ***************************************************************************************** */

cCommaSeparatedValues&  endl(cCommaSeparatedValues& __csv)
{
    __csv.line.chop(1); // Drop last separator
    *__csv.pStream << __csv.line << endl;
    __csv.line.clear();
    return __csv;
}

cCommaSeparatedValues&  first(cCommaSeparatedValues& __csv)
{
    __csv.pStream->seek(0);
    __csv._lineNo = 0;
    return next(__csv);
}

cCommaSeparatedValues&  next(cCommaSeparatedValues& __csv)
{
    __csv.values.clear();
    __csv.line  = __csv.pStream->readLine();
    __csv._state = __csv.pStream->status();
    if (__csv._state == 0) {
        if (__csv.line.isNull() && __csv.pStream->atEnd()) {
            __csv._state = CSVE_EDD_OF_FILE;
        }
        else {
            __csv._lineNo++;
            __csv.splitLine();
        }
    }
    return __csv;
}

const QChar cCommaSeparatedValues::sep = QChar(';');
const QChar cCommaSeparatedValues::quo = QChar('"');

cCommaSeparatedValues::cCommaSeparatedValues(const QString& _csv)
{
    csv     = _csv;
    pStream = new QTextStream(&csv, QIODevice::ReadWrite);
}

cCommaSeparatedValues::cCommaSeparatedValues(QIODevice *pIODev)
{
    pStream = new QTextStream(pIODev);
    pStream->setCodec("UTF-8");
    pStream->setGenerateByteOrderMark(true);
}

cCommaSeparatedValues::~cCommaSeparatedValues()
{
    delete pStream;
}

void cCommaSeparatedValues::clear()
{
    QIODevice *pIODev = pStream->device();
    if (pIODev != nullptr) pIODev->close();
    delete pStream;
    csv.clear();
    line.clear();
    pStream = new QTextStream(&csv, QIODevice::ReadWrite);
    (*pStream) << bom;  // BOM / UTF8
}

const QString& cCommaSeparatedValues::toString() const
{
    return csv;
}

int cCommaSeparatedValues::state(QString& msg)
{
    switch (_state) {
    case CSVE_OK:                   msg.clear(); break;
    case CSVE_STRM_ReadPastEnd:     msg = QObject::tr("Stream: ReadPastEnd.");  break;
    case CSVE_STRM_ReadCorruptData: msg = QObject::tr("Stream: ReadCorruptData.");  break;
    case CSVE_STRM_WriteFailed:     msg = QObject::tr("Stream: WriteFailed.");  break;
    case CSVE_EMPTY_LINE:           msg = QObject::tr("Emoty line.");  break;
    case CSVE_PARSE_ERROR:          msg = QObject::tr("Parse error.");  break;
    case CSVE_IS_NOT_NUMBER:        msg = QObject::tr("A mező nem egész szám típus.");  break;
    case CSVE_DROP_INVCH:           msg = QObject::tr("Invalid character, dropped.");  break;
    case CSVE_DROP_CRLF:            msg = QObject::tr("Cr. or Lf. dropped.");  break;
    case CSVE_END_OF_LINE:          msg = QObject::tr("End of line.");  break;
    case CSVE_EDD_OF_FILE:          msg = QObject::tr("End of file.");  break;
    default:                        msg = QObject::tr("Invalid result state (%1).").arg(_state);  break;
    }
    if (_state != CSVE_OK) {
        msg.prepend(QObject::tr("Line %1 : ").arg(_lineNo));
        msg += " Last line :\n" + line;
    }
    return _state;
}


cCommaSeparatedValues& cCommaSeparatedValues::operator <<(const QString& _s)
{
    _state = CSVE_OK;
    QString r;
    int n = _s.size();
    bool quote = false;
    for (int i = 0; i < n; ++i) {
        QChar c = _s.at(i);
        if (!quote && c == sep) quote = true;
        else if (c.isSpace()) {
            quote = true;
            QString esc;
            switch (c.toLatin1()) {
            case '\r':  esc = "\\r";    break;
            case '\n':  esc = "\\n";    break;
            default:                    break;
            }
            if (!esc.isEmpty()) {
                if (!dropCrLf) {
                    c = QChar(' ');
                    _state = CSVE_DROP_CRLF;
                }
                else {
                    r += esc;
                    continue;
                }
            }
        }
        else if (c == quo || c == QChar('\\')) {
            r += c;
            quote = true;
        }
        else if (!c.isPrint()) {
            _state = CSVE_DROP_INVCH;
            continue;
        }
        r += c;
    }
    if (quote) r.prepend(quo).append(quo);
    line += r + sep;
    return *this;
}

cCommaSeparatedValues& cCommaSeparatedValues::operator >>(QString& _v)
{
    if (values.isEmpty()) {
        _state = CSVE_END_OF_LINE;
        _v.clear();
    }
    else {
        _state = CSVE_OK;
        _v = values.takeFirst();
    }
    return *this;
}

cCommaSeparatedValues& cCommaSeparatedValues::operator >>(qlonglong &_v)
{
    _v = NULL_ID;
    QString s;
    operator >>(s);
    if (_state == CSVE_OK) {
        bool ok;
        qlonglong r = s.toLongLong(&ok);
        if (ok) _v = r;
        else _state = CSVE_IS_NOT_NUMBER;
    }
    return *this;
}

void cCommaSeparatedValues::splitLine()
{
    values.clear();
    if (line.isEmpty()) {
        _state = CSVE_EMPTY_LINE;
        return;
    }
    _state = CSVE_OK;
    values = line.split(sep, QString::KeepEmptyParts);
    QRegExp p1("^\\s*(\")+");
    QRegExp p2("(\")+\\s*$");
    QString tr = "\"";
    QString q2 = "\"\"";
    for (int i = 0; i < values.size(); ++i) {
        QString& f = values[i];
        bool quoted = p1.indexIn(f) >= 0;
        int  spnum  = quoted ? p1.cap(1).size() : 0;
        quoted = 0 != (spnum % 2);
        if (quoted) {
            if (spnum) f.replace(p1, tr);
            quoted = p2.indexIn(f) >= 0;
            spnum  = quoted ? p2.cap(1).size() : 0;
            quoted = 0 != (spnum % 2);
            if (quoted) {
                f.replace(p2, tr);
            }
            else {  // Splitted string
                if (i + 1 >= values.size()) {
                    _state = CSVE_PARSE_ERROR;
                    return;
                }
                f += sep + values.takeAt(i + 1);
                --i;
                continue;
            }
            f = f.mid(1, f.size() -2);
            f.replace("\\r", "\r");
            f.replace("\\n", "\n");
            f.replace(q2, tr);
        }
        else {
            f = f.trimmed();
        }
    }
}

QHostAddress rdpClientAddress()
{
    QHostAddress rdpClientAddr;
#if defined(Q_OS_WINDOWS)
    LPSTR  pBuffer = nullptr;
    DWORD  bytesReturned;
    BOOL isRDPClient = WTSQuerySessionInformationA(
                WTS_CURRENT_SERVER_HANDLE,
                WTS_CURRENT_SESSION,
                WTSClientAddress,
                &pBuffer,
                &bytesReturned);
    if (isRDPClient && pBuffer != nullptr && bytesReturned == sizeof (WTS_CLIENT_ADDRESS)) {
        WTS_CLIENT_ADDRESS& car = *(WTS_CLIENT_ADDRESS *)pBuffer;
        if (car.AddressFamily == AF_INET) {
            // QString sa = (char *)car.Address;    // A leírás szerint ennek a kliens címnek kellene lennie
            // de nem stringként hanem binárisan van benne a cím: 3-6 byte, és fordítva, mint ahogy a setAddress() várja.
            quint32 a = qFromBigEndian(*(quint32 *)(car.Address +2));
            rdpClientAddr.setAddress(a);
        }
    }
    if (isRDPClient && pBuffer != nullptr) {
        WTSFreeMemory(pBuffer);
    }
#endif  // defined(Q_OS_WINDOWS)
    return rdpClientAddr;
}
