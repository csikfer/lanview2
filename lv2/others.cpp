#include "lanview.h"
#include "others.h"
#include <QCoreApplication>
#include <QTextStream>

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

/******************************************************************************/

cFeatures::cFeatures()
    : QMap<QString, QString>()
{
    ;
}

bool cFeatures::split(const QString& __ms, bool merge, eEx __ex)
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
            pv[1] = pv.mid(1).join(QChar('='));
        }
        QString key = pv[0].toLower();
        QString val = pv.count() >= 2 ? pv[1] : _sNul;
        if (merge && val == "!") remove(key);        // Törli (ha van)
        else            (*this)[key] = val;
    }
    return true;
}

QStringList cFeatures::value2list(const QString &s)
{
    QStringList r = s.split(QRegExp("\\s*,\\s*"));
    for (int i = 0; i < r.size(); ++i) {
        while (r.at(i).endsWith(QChar('\\'))) {
            if ((i +1) >= r.size()) EXCEPTION(EDATA, 0, s);
            r[i].chop(1);
            r[i] += QChar(',') + r[i +1];
            r.removeAt(i +1);
        }
    }
    return r;
}

QString cFeatures::list2value(const QStringList& l)
{
    static const QString sep = ",";
    static const QString esep = "\\,";
    QString r;
    if (!l.isEmpty()) {
        foreach (QString s, l) {
            s = s.replace(sep, esep);
            r += s + sep;
        }
        r.chop(sep.size());
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


QString cFeatures::join() const
{
    static const QString sep = ":";
    static const QString esep = "::";
    QString r = sep;
    for (ConstIterator i = constBegin(); i != constEnd(); ++i) {
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
        else            insert(key, val);
    }
    return *this;
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

cFeatures cFeatures::noSimple() const
{
    cFeatures r;
    foreach (QString key, keys()) {
        if (key.contains(QChar('.'))) {
            r[key] = value(key);
        }
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
}

const QString& cCommaSeparatedValues::toString() const
{
    return csv;
}

int cCommaSeparatedValues::state(QString& msg)
{
    switch (_state) {
    case CSVE_OK:                   msg.clear(); break;
    case CSVE_STRM_ReadPastEnd:     msg = QObject::trUtf8("Stream: ReadPastEnd.");  break;
    case CSVE_STRM_ReadCorruptData: msg = QObject::trUtf8("Stream: ReadCorruptData.");  break;
    case CSVE_STRM_WriteFailed:     msg = QObject::trUtf8("Stream: WriteFailed.");  break;
    case CSVE_EMPTY_LINE:           msg = QObject::trUtf8("Emoty line.");  break;
    case CSVE_PARSE_ERROR:          msg = QObject::trUtf8("Parse error.");  break;
    case CSVE_IS_NOT_NUMBER:        msg = QObject::trUtf8("A mező nem egész szám típus.");  break;
    case CSVE_DROP_INVCH:           msg = QObject::trUtf8("Invalid character, dropped.");  break;
    case CSVE_DROP_CRLF:            msg = QObject::trUtf8("Cr. or Lf. dropped.");  break;
    case CSVE_END_OF_LINE:          msg = QObject::trUtf8("End of line.");  break;
    case CSVE_EDD_OF_FILE:          msg = QObject::trUtf8("End of file.");  break;
    default:                        msg = QObject::trUtf8("Invalid result state (%1).").arg(_state);  break;
    }
    if (_state != CSVE_OK) {
        msg.prepend(QObject::trUtf8("Line %1 : ").arg(_lineNo));
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
