#include "lv2syntax.h"

const cRecStaticDescr&  cObjectSyntax::descr() const
{
    if (initPDescr<cObjectSyntax>(_sObjectSyntaxs)) {
        _ixFeatures = _descr_cObjectSyntax().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

int cObjectSyntax::_ixFeatures = NULL_IX;

CRECDEFD(cObjectSyntax)

cObjectSyntax::cObjectSyntax()
    : cRecord()
{
    _pFeatures = NULL;
    _set(cObjectSyntax::descr());
}
cObjectSyntax::cObjectSyntax(const cObjectSyntax& __o)
    : cRecord()
{
    _pFeatures = NULL;
    _cp(__o);
}

/*      ****        ****        */

enum ePSType {
    PST_SIMPLE, PST_GROUP, PST_TREE, PST_GROUP_HEAD,
    PST_MASK        = 0x0f,
    PST_CHILD       = 0x10,
    PST_MEMBER      = 0x20,
    PST_SUB_TREE    = 0x40,
};

class cParseSyntax {
    friend LV2SHARED_EXPORT cError * exportRecords(const QString& __tn, QString str);
protected:
    cParseSyntax(const QString& __on, int __in = 0, const QString& __tn = QString());
    QString         name;
    QString         table;
    cRecordAny      o;
    cObjectSyntax   os;
    int             type;       ///< Típus (ePSType)
    QString         sKey;       ///< Mező név: Tree: A parent id-je; Group: Csoportosítás a mző érték alapján
    int             ixKey;      ///< Az sKey nevű mező indexe vagy NULL_IX
    QString         sMemb;      ///< PST_GROUP_HEAD: csoport tag mező
    int             ixMemb;
    QString         sHeadVal;   ///< PST_GROUP_HEAD: csoport tag mező értéke, a header rekord esetén
                                ///< PST_TREE: A root elem(ek) esetén az sKey mező értéke
    bool            bHeadVal;   ///< PST_GROUP_HEAD: csoport tag mező értéke nem NULL, a header rekord esetén
                                ///< PST_TREE: A root elem(ek) esetén az sKey mező NULL
    QBitArray       where;
    tIntVector      ord;
    int             indent;
    QString         divert;
    QSqlQuery       q1, q2;
    cParseSyntax *  pParent;
    QString exec();
private:
    QString exportRecordGroup();
    QString exportRecordTree();
    QString exportRecordSimple();
    QString parseString(const QString& __src, const QStringList &__pl = QStringList(), int * __pIx = NULL);
    QString getRecordFieldValue(const QString& __s, QString::const_iterator& ite, QString::const_iterator& end, QString& emsg);
    QString getFieldValue(const QString& vname, const QStringList& pl);
    QString getCondString(QString::const_iterator& i, const QString::const_iterator& e);
    int parseParams(QStringList& pl, int ss);
    static QString exportFieldValue(QSqlQuery &q, const cRecord &oo, int ix);
    QString exportRecordLine(QString &line);
    QString exportRecord();
    QString subExport(int __t, int _ind, const QString& __tn = QString());
};

cParseSyntax::cParseSyntax(const QString& __on, int __in, const QString &__tn)
{
    ixKey = ixMemb = NULL_IX;
    bHeadVal = false;
    q1 = getQuery();
    q2 = getQuery();
    indent = __in;
    pParent = NULL;
    name = __on;
    os.setByName(q1, name);
    table = __tn.isEmpty() ? __on : __tn;
    o.setType(table);
    if      (os.isFeature(_sGroup)) {
        sKey  = os.feature(_sGroup);
        ixKey = o.toIndex(sKey);
        type  = PST_GROUP;
        if (os.isFeature(_sHead)) {
            type  = PST_GROUP_HEAD;
            QStringList sl = os.feature(_sHead).split(",");
            sMemb = sl.first();
            ixMemb = o.toIndex(sMemb);
            bHeadVal = sl.size() > 1;
            if (bHeadVal) sHeadVal = sl.at(1);
            type = PST_GROUP_HEAD;
        }
        where = _bit(ixKey);            // A kulcsra szűrünk
    }
    else if (os.isFeature(_sTree)) {
        type  = PST_TREE;
        sKey  = os.feature(_sTree);
        ixKey = o.toIndex(sKey);
        where = _bit(ixKey);            // A kulcsra szűrünk
        bHeadVal = os.isFeature(_sRoot);
        if (bHeadVal) sHeadVal = os.feature(_sRoot);
    }
    else {
        type = PST_SIMPLE;
        ixKey = NULL_IX;
        where = QBitArray(1, false);    // Összes rekord
    }
    QString sOrd = os.feature("ord");   // rendezés ?
    if (!sOrd.isEmpty()) {
        QStringList sl = sOrd.split(',');
        foreach (QString f, sl) {
            if (f.startsWith(QChar('!'))) { // csükkenő sorrend ?
                f = f.mid(1);
                ord << - o.toIndex(f);

            }
            else {
                ord << o.toIndex(f);
            }
        }
    }
}

QString cParseSyntax::exec()
{
    if (type & ~PST_MASK && pParent == NULL) EXCEPTION(EPROGFAIL);
    switch (type & PST_MASK) {
    case PST_SIMPLE:    return exportRecordSimple();
    case PST_GROUP_HEAD:
    case PST_GROUP:     return exportRecordGroup();
    case PST_TREE:      return exportRecordTree();
    default:            EXCEPTION(EPROGFAIL);
    }
    return _sNul;   // :-/
}

cError * exportRecords(const QString& __tn, QString str)
{
    cError *pe;
    try {
        str += cParseSyntax(__tn).exec();
    } CATCHS(pe)
    return pe;
}

/**************************************/
/// String behelyettesítések.
/// @param __src a feldolgozandó string
/// @param __pl a teljes string lista ($1, $2... behelyettesítéshez) feltételes kifelyezés esetén
/// @param __pIx ha volt mező hivatkozás, és nem NULL, akkor ide kerül a mező index.
QString cParseSyntax::parseString(const QString& __src, const QStringList &__pl, int * __pIx)
{
    if (__src.simplified().isEmpty()) return QString();
    QString::const_iterator ite = __src.constBegin();
    QString::const_iterator end = __src.constEnd();
    QString s, r;
    while (ite < end) {
        QChar c = *ite;
        ++ite;
        switch (c.toLatin1()) {
        case '\\':          // Védett karakter
            ++ite;
            if (ite >= end) EXCEPTION(EDATA, end - ite, __pl.isEmpty() ? __src : __pl.join(","));
            r += *ite;
            break;
        case '$': {         // Érték szerinti behelyettesítés, mező érték, vagy lista elem
            s  = getParName(ite, end, false);       // '.' nem lehet a névben
            if (ite != end && *ite == QChar('[')) { // idegen rekord hivatkozás ?
                QString emsg;
                r += getRecordFieldValue(s, ite, end, emsg);
                if (!emsg.isEmpty()) {      // OK?
                    EXCEPTION(EDATA, 0, emsg);
                }
            }
            else {
                if (__pIx != NULL) *__pIx = o.toIndex(s, EX_IGNORE);
                r += getFieldValue(s, __pl);
            }
            break;
          }
        case '?':     // Feltételes kifejezés
            if (!__pl.isEmpty()) {  // Ha már egy feltételes kifelyezésben vagyunk,
                EXCEPTION(EDATA, 0, o.identifying());
            }
            r += getCondString(ite, end);
            break;
        case '@':   // Child object, vagy group, vagy tree
          {
            if (r.simplified().isEmpty()) r.clear();
            else                          r += QChar('\n');
            s = getParName(ite, end, false);
            if      (s == "GROUP") r += subExport(PST_MEMBER,  indent);
            else if (s == "TREE")  r += subExport(PST_SUB_TREE,indent);
            else                   r += subExport(PST_CHILD,   indent, s);
            break;
          }
        default:
            r += c;
            break;
        }
    }
    return r;
}

int cParseSyntax::parseParams(QStringList& pl, int ss)
{
    int i, n = pl.size();
    int fix = NULL_IX;
    for (i = 0; i < n; ++i) {
        QString s = pl.at(i);
        s = s.trimmed();
        pl[i] = parseString(s, pl, i == 0 ? &fix : NULL);
    }
    while (pl.size() < ss) pl << QString();
    return fix;
}

/// A feltételes kifelyezés értelmezése, a '?' után a string végéig, vagy az első '^' karakterig.
/// A kifelyezésben a szeparátor a ':' karakter. Az első mező első karaktere a feltétel típusa (case insensitive):
/// I
/// @param i Iterátor a stringen, a '?' után
/// @param e a string végét jeklző iterátor
QString cParseSyntax::getCondString(QString::const_iterator& i, const QString::const_iterator& e)
{
    QString s;
    while (i < e && *i != QChar('^')) {
        s += *i;
        i++;
    }
    if (i < e) ++i;
    if (s.isEmpty()) return s;
    QStringList params = s.split(':');
    enum eIfType
    { IF_VALID, IF_NOT_EMPTY, IF_EQU, IF_LITLE, IF_GREATER, IF_IF, IF_ROOT }
    t = IF_VALID;
    bool neg = false;
    s = params.first(); // type string
    if (s.startsWith(QChar('!'))) {
        neg = true;
        s = s.mid(1);
    }
    if (s.size() != 1) EXCEPTION(EDATA, 0, params.join(","));   // Egybetüs feltétel jelek
    params.pop_front();
    int n = 0;  // mezők száma (inicializáljuk, mert warning-ol a fordító) az utolsó mező (false-hoz tartozó érték opcionális)
    switch (s[0].toUpper().toLatin1()) {
    case 'V':   t = IF_VALID;      n = 3;   break;  // nem NULL mező
    case 'N':   t = IF_NOT_EMPTY;  n = 3;   break;  //
    case 'E':   t = IF_EQU;        n = 4;   break;  //
    case 'L':   t = IF_LITLE;      n = 4;   break;  //
    case 'G':   t = IF_GREATER;    n = 4;   break;  //
    case 'I':   t = IF_IF;         n = 3;   break;  //
    case 'R':   t = IF_ROOT;       n = 2;   break;  // tree root ?
    default:    EXCEPTION(EDATA, 1, params.join(","));
    }
    int fix = parseParams(params, n);    // fix = első mező ben az objektum mező indexe (ha van hivatkozás)
    bool f;                                 // A feltétel eredménye ide
    int true_ix  = n - 2;                   // Az "igaz" mező indexe a mezőkre bontorr string kifelyezésben
    int false_ix = n - 1;                   // A "hamis" mező indexe a mezőkre bontorr string kifelyezésben
    switch (t) {
    case IF_VALID:
        if (fix != NULL_IX) f = !o.isNull(fix);
        else                f = !params.at(0).isEmpty();
        break;
    case IF_NOT_EMPTY:
        f = !params.at(0).isEmpty();
        break;
    case IF_EQU:
        f = params.at(0) == params.at(1);
        break;
    case IF_GREATER:
        f = params.at(0).toDouble() > params.at(1).toDouble();
        break;
    case IF_LITLE:
        f = params.at(0).toDouble() < params.at(1).toDouble();
        break;
    case IF_IF:
        if (fix != NULL_IX) f = o.getBool(fix);
        else                f = str2bool(params.at(0));
        break;
    case IF_ROOT:
        f = !(type & PST_SUB_TREE);
        break;
    }
    if (neg) f = !f;
    return params.at(f ? true_ix : false_ix);
}

static void nIndent(int &indent, QString& s)
{
    if (s[0].isDigit()) {
        indent += s[0].toLatin1() - '0';
        s.remove(0, 1);
    }
}

QString cParseSyntax::exportRecordLine(QString &line)
{
    QString r;
    if (line.endsWith(QChar('\n'))) line.chop(1);
    if (line.isEmpty()) return r;
    int i = 0;
    int c = line[i].toLatin1();
    int ind = indent;
    // Ha az első karakter egy szám, akkor az az indentálást jelenti
    nIndent(ind, line);
    if (c == '?') { // Feltételes sor
        QString::const_iterator ite = line.constBegin() +1; // ? átugrása
        QString::const_iterator end = line.constEnd();
        r = getCondString(ite, end);
        if (ite != end) EXCEPTION(EDATA, 0, line);
    }
    else {
        r = parseString(line);
    }
    if (!r.isEmpty()) r = indentSp(ind) + r + "\n";
    return r;
}

/// Rekord mező érték hivatkozás
/// Ha nem létezik a megadott id-ü vagy nevű rekord, akkor a null stringgel tér vissza.
QString cParseSyntax::getRecordFieldValue(const QString& __s, QString::const_iterator& ite, QString::const_iterator& end, QString& emsg)
{
    // <tábla>[rekord név|rekord id].<mező név>
    cRecordAny r;
    QString name, field;    // Rekord és mező név
    qlonglong id;           // Rekord ID
    bool ok;
    QStringList sl;
    if (__s.isEmpty()) goto RFV__syntax_error;
    sl = __s.split('.');
    if (sl.size() > 2) goto RFV__syntax_error;
    if (sl.size() < 2) sl << _sNul;
    r.setType(sl[0], sl[1], EX_IGNORE);
    if (r.isFaceless()) {
        emsg = QObject::trUtf8("Ismeretlen tábla név : %1").arg(__s);
        return QString();
    }
    // name or id
    for (++ite; ite < end && *ite != QChar(']'); ++ite) name += *ite;
    if (name.isEmpty() || ite >= end || ++ite >= end || *ite != QChar('.')) goto RFV__syntax_error;
    ++ite;
    id = name.toLongLong(&ok);
    if (ok && r.idIndex(EX_IGNORE) >= 0) {    // ID?
        ok = r.fetchById(q1, id);
    }
    else {
        ok = false;
    }
    if (!ok && r.nameIndex(EX_IGNORE) >= 0) {   // NAME?
        ok = r.fetchByName(q1, name);
    }
    if (!ok) {
        return QString();
    }
    // field name
    field = getParName(ite, end, EX_IGNORE);
    if (field.isEmpty()) goto RFV__syntax_error;
    if (r.isIndex(field)) {
        emsg = QObject::trUtf8("Ismeretlen mező név : %1.%2").arg(__s, field);
        return QString();
    }
    return exportFieldValue(q1, r, r.toIndex(field));
RFV__syntax_error:
    emsg = QObject::trUtf8("Syntax error.");
    return QString();
}

QString cParseSyntax::getFieldValue(const QString& vname, const QStringList& pl)
{
    QString r;
    bool ok;
    int ix = vname.toInt(&ok);
    if (ok) {
        if (ix <= 0 || ix > pl.size()) {
            EXCEPTION(EDATA, ix);
        }
        r = pl.at(ix -1);
    }
    else {
        if (vname == "PARENT") {
            if (pParent == NULL) return r;
            return exportFieldValue(q1, pParent->o, pParent->o.nameIndex());
        }
        if      (vname == "ID")   ix = o.idIndex();
        else if (vname == "NAME") ix = o.nameIndex();
        else if (vname == "NOTE") {
            ix = o.noteIndex();
            if (o.isNull(ix)) return r;
        }
        else                      ix = o.toIndex(vname);
        r = exportFieldValue(q1, o, ix);
    }
    return r;
}

QString cParseSyntax::exportFieldValue(QSqlQuery& q, const cRecord& oo, int ix)
{
    QString r;
    switch (oo.colDescr(ix).eColType) {
    case cColStaticDescr::FT_INTEGER:
        if (oo.colDescr(ix).fKeyType != cColStaticDescr::FT_NONE) {
            qlonglong id = oo.getId(ix);
            if (id != NULL_ID) {
                r = oo.colDescr(ix).fKeyId2name(q, id, EX_ERROR);
                r = quotedString(r);
            }
            break;
        }
        // else continue ... Stringé konvertáljuk
    case cColStaticDescr::FT_REAL:
    case cColStaticDescr::FT_MAC:
    case cColStaticDescr::FT_INET:
    case cColStaticDescr::FT_CIDR:
    case cColStaticDescr::FT_POLYGON:
        r = oo.getName(ix);
        break;
    case cColStaticDescr::FT_TIME:
        r = oo.get(ix).toTime().toString("hh:mm:ss.zzz");
        break;
    case cColStaticDescr::FT_INTERVAL:
        r =QString::number(oo.getId(ix) / 1000);  //msec to sec
        break;
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_ENUM:
        r = quotedString(oo.getName(ix));
        break;
    case cColStaticDescr::FT_BOOLEAN:
        r = oo.getBool(ix) ? _sTrue : _sFalse;
        r = r.toUpper();
        break;
    case cColStaticDescr::FT_INTEGER_ARRAY:
        if (oo.colDescr(ix).fKeyType != cColStaticDescr::FT_NONE) {
            foreach(QVariant v, oo.get(ix).toList()) {
                qlonglong id = v.toLongLong();
                QString s = oo.colDescr(ix).fKeyId2name(q, id, EX_ERROR);
                r += quotedString(s) + ", ";
            }
            if (!r.isEmpty()) r.chop(2);
            break;
        }
        // else continue ...
    case cColStaticDescr::FT_REAL_ARRAY:
        foreach(QVariant v, oo.get(ix).toList()) r += v.toString() + ", ";
        if (!r.isEmpty()) r.chop(2);
        break;
    case cColStaticDescr::FT_TEXT_ARRAY:
    case cColStaticDescr::FT_SET:
        foreach(QVariant v, oo.get(ix).toList()) r += quotedString(v.toString()) + ", ";
        if (!r.isEmpty()) r.chop(2);
        break;
    case cColStaticDescr::FT_DATE:
    case cColStaticDescr::FT_DATE_TIME:
    case cColStaticDescr::FT_BINARY:
        EXCEPTION(ENOTSUPP);
    }
    return r;
}


QString cParseSyntax::exportRecord()
{
    QString sentence = os.getName(_sSentence);
    QStringList lines = sentence.split('\n');
    QString r;
    foreach (QString line, lines) {
        QString l = exportRecordLine(line);
        if (l[0] == QChar('>')) {
            divert += l.mid(1);
        }
        else {
            r += l;
        }
    }
    return r;
}

QString cParseSyntax::subExport(int __t, int _ind, const QString &__tn)
{
    QString n, t;
    switch (__t) {
    case PST_CHILD:
        t = n = __tn;
        break;
    case PST_MEMBER:
        if (type != PST_GROUP || type != PST_GROUP_HEAD) EXCEPTION(EDATA);
        t = table;
        n = mCat(name, sKey);
        break;
    case PST_SUB_TREE:
        if (type != PST_TREE) EXCEPTION(EDATA);
        t = table;
        n = name;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    cParseSyntax sos(n, _ind, t);
    sos.pParent = this;
    switch (__t) {
    case PST_CHILD:
        switch (sos.type) {
        case PST_SIMPLE:
        case PST_TREE:
            break;
        default:
            EXCEPTION(EDATA);
        }
        sos.type |= PST_CHILD;
        break;
    case PST_MEMBER:
        if (sos.type != PST_SIMPLE) EXCEPTION(EDATA);   // örökli!
        sos.type = type | PST_MEMBER;
        break;
    case PST_TREE:
        if (sos.type != PST_TREE) EXCEPTION(EPROGFAIL); // azonos leíró
        sos.type = type | PST_SUB_TREE;
    }
    return sos.exec();
}

QString cParseSyntax::exportRecordGroup()
{
    QString r;
    switch (type) {
    case PST_GROUP:
    case PST_GROUP_HEAD:
    case PST_GROUP | PST_MEMBER:
    case PST_GROUP_HEAD | PST_MEMBER:
        break;
    default:
        EXCEPTION(EDATA);
    }
    // Fetch groups names
    QString sql = QString("SELECT DISTINCT %1 FROM %2").arg(sKey, o.tableName());
    if (execSql(q2, sql)) {
        QString s;
        do {
            QString group = q2.value(0).toString();
            o.clear();
            o.setName(ixKey, group);
            if ((type & PST_MASK) == PST_GROUP_HEAD) {
                where = _bits(ixKey, ixMemb);
                if (bHeadVal) o.setName(ixMemb, sHeadVal);
                if (!o.fetch(q1, false, where)) {
                    if (q1.next()) EXCEPTION(EDATA, 0, QObject::trUtf8("Invalid group header."));   // Csak egy lehet!
                }
                else {  // nincs header rekord
                    o.setName(ixKey, group);    // visszarakjuk
                }
            }
            s = exportRecord();
            PDEB(VERBOSE) << s << endl;
            r += s;
        } while (q2.next());
    }
    return r;
}

QString cParseSyntax::exportRecordTree()
{
    QString r;
    divert.clear();
    if (pParent == NULL) {      // Nincs parent: gyökér
        if (bHeadVal) o.setName(ixKey, sHeadVal);
        else          o.clear(ixKey);
    }
    else {
        switch (pParent->type & PST_MASK) {
        case PST_TREE:
            o.setId(ixKey, pParent->o.getId());
            break;
        case PST_GROUP:
        case PST_GROUP_HEAD:
            if (bHeadVal) o.setName(ixKey, sHeadVal);
            else          o.clear(ixKey);
            where |= _bit(pParent->ixKey);
            o.set(pParent->ixKey, pParent->o.get(pParent->ixKey));
            break;
        default:
            EXCEPTION(ENOTSUPP);
        }

    }
    if (o.fetch(q2, false, where, ord)) {
        bool checkHead = pParent != NULL && pParent->type == PST_GROUP_HEAD;
        QString s;
        do {
            if (checkHead && o.get(pParent->ixMemb) == pParent->o.get(pParent->ixMemb)) continue;
            s = exportRecord();
            PDEB(VERBOSE) << s << endl;
            r += s;
        } while (o.next(q2));
    }
    return r + divert;
}

QString cParseSyntax::exportRecordSimple()
{
    QString r;
    divert.clear();
    if (pParent != NULL) {  // Van parent?
        switch (pParent->type) {
        case PST_GROUP:
        case PST_GROUP_HEAD:
            where |= _bit(pParent->ixKey);
            o.set(pParent->ixKey, pParent->o.get(pParent->ixKey));
            break;
        case PST_SIMPLE:    // Owner -- Child
            break;
        default:
            EXCEPTION(ENOTSUPP);
        }
    }
    if (o.fetch(q2, false, where, ord)) {
        bool checkHead = pParent != NULL && pParent->type == PST_GROUP_HEAD;
        QString s;
        do {
            if (checkHead && o.get(pParent->ixMemb) == pParent->o.get(pParent->ixMemb)) continue;
            s = exportRecord();
            PDEB(VERBOSE) << s << endl;
            r += s;
        } while (o.next(q2));
    }
    return r + divert;
}

