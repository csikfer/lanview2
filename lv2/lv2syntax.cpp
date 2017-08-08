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

/*      ----        ----        */

cExportThread::cExportThread(QObject *par) : QThread(par)
{
    pLastError = NULL;
    pStrQueue  = NULL;
}

cExportThread::~cExportThread()
{
    if (isRunning()) terminate();
    wait();
    pDelete(pLastError);
}

void cExportThread::run()
{
    if (pStrQueue != NULL) EXCEPTION(EPROGFAIL);
    pStrQueue = new cStringQueue(this);
    connect(pStrQueue, SIGNAL(ready()), this, SLOT(queue()));
    pLastError = exportRecords(sTableName, *pStrQueue);
    int nwait = 2;
    while (!pStrQueue->isEmpty()) {
        if (nwait == 0) EXCEPTION(EPROGFAIL);
        --nwait;
        msleep(50);
    }
    pDelete(pStrQueue);
}

void cExportThread::queue()
{
    if (pStrQueue == NULL) {
        DWAR() << "pStrQueue is NULL" << endl;
        return;
    }
    QString s = pStrQueue->pop();
    if (s.isEmpty()) return;
    sReady(s);
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
    friend LV2SHARED_EXPORT cError * exportRecords(const QString& __tn, cStringQueue& __sq );
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
    int             actIndndent;
    QSqlQuery       q1, q2;
    cParseSyntax *  pParent;
    void exec(cStringQueue *__psq);
private:
    void exportRecordGroup();
    void exportRecordTree();
    void exportRecordSimple();
    void parseString(const QString& __src, const QStringList &__pl = QStringList());
    QString getRecordFieldValue(const QString& __s, QString::const_iterator& ite, QString::const_iterator& end, QString& emsg);
    QString getFieldValue(const QString& vname, const QStringList &pl = QStringList());
    QString getCondString(QString::const_iterator& i, const QString::const_iterator& e, QStringList &pl);
    static QString exportFieldValue(QSqlQuery &q, const cRecordFieldConstRef &fr);
    void exportRecordLine(QString &line);
    void exportRecord();
    void subExport(int __t, const QString& __tn = QString());
    QString         lb;     ///< Line buffer
    QString         divert;
    QStringList     blocks;
    void            move2blk(QString& s) {
        if (!s.simplified().isEmpty()) {
            blocks.last() += s;
        }
        s.clear();
    }
    cStringQueue *  pSQ;
};

cParseSyntax::cParseSyntax(const QString& __on, int __in, const QString &__tn)
{
    ixKey = ixMemb = NULL_IX;
    bHeadVal = false;
    q1 = getQuery();
    q2 = getQuery();
    actIndndent = indent = __in;
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

void cParseSyntax::exec(cStringQueue * __psq)
{
    pSQ = __psq;
    if (type & ~PST_MASK && pParent == NULL) EXCEPTION(EPROGFAIL);
    switch (type & PST_MASK) {
    case PST_SIMPLE:    exportRecordSimple();   break;
    case PST_GROUP_HEAD:
    case PST_GROUP:     exportRecordGroup();    break;
    case PST_TREE:      exportRecordTree();     break;
    default:            EXCEPTION(EPROGFAIL);
    }
}

cError * exportRecords(const QString& __tn, cStringQueue& __sq)
{
    cError *pe = NULL;
    try {
        cParseSyntax(__tn).exec(&__sq);
    } CATCHS(pe)
    return pe;
}

/**************************************/
/// String behelyettesítések.
/// @param __src a feldolgozandó string
/// @param __pl a teljes string lista ($1, $2... behelyettesítéshez) feltételes kifelyezés esetén
void cParseSyntax::parseString(const QString& __src, const QStringList &__pl)
{
    if (__src.simplified().isEmpty()) return;
    QString::const_iterator ite = __src.constBegin();
    QString::const_iterator end = __src.constEnd();
    QString s;
    QStringList    pl;
    while (ite < end) {
        QChar c = *ite;
        ++ite;
        switch (c.toLatin1()) {
        case '\\':          // Védett karakter
            if (ite >= end) EXCEPTION(EDATA, end - ite, __pl.isEmpty() ? __src : __pl.join(","));
            lb += *ite;
            ++ite;
            break;
        case '$': {         // Érték szerinti behelyettesítés, mező érték, vagy lista elem
            s  = getParName(ite, end, false);       // '.' nem lehet a névben
            if (ite != end && *ite == QChar('[')) { // idegen rekord hivatkozás ?
                QString emsg;
                lb += getRecordFieldValue(s, ite, end, emsg);
                if (!emsg.isEmpty()) {      // OK?
                    EXCEPTION(EDATA, 0, emsg);
                }
            }
            else {
                lb += getFieldValue(s, __pl);
            }
            break;
          }
        case '?':     // Feltételes kifejezés
            if (!__pl.isEmpty()) {  // Ha már egy feltételes kifelyezésben vagyunk,
                EXCEPTION(EDATA, 0, o.identifying());
            }
            pSQ->move(lb);
            s = getCondString(ite, end, pl);
            parseString(s, pl);
            break;
        case '@':   // Child object, vagy group, vagy tree
          {
            lb += QChar('\n');
            pSQ->move(lb);
            s = getParName(ite, end, false);
            if      (s == "GROUP") subExport(PST_MEMBER);
            else if (s == "TREE")  subExport(PST_SUB_TREE);
            else                   subExport(PST_CHILD, s);
            break;
          }
        default:
            lb += c;
            break;
        }
    }
}

static bool var2bool(const QVariant& v)
{
    if (v.isNull()) return false;
    if (v.userType() == QMetaType::Bool) return v.toBool();
    if (variantIsInteger(v)) return v.toLongLong() == 0;
    if (variantIsString(v)) return str2bool(v.toString(), EX_IGNORE);
    return false;
}

/// A feltételes kifelyezés értelmezése, a '?' után a string végéig, vagy az első '^' karakterig.
/// A kifelyezésben a szeparátor a ':' karakter. Az első mező a feltétel
/// @param i Iterátor a stringen, a '?' után
/// @param e a string végét jeklző iterátor
/// @param plo Feltétel paraméterek (kimenet)
QString cParseSyntax::getCondString(QString::const_iterator& i, const QString::const_iterator& e, QStringList& plo)
{
    QString s;
    while (i < e && *i != QChar('^')) {
        s += *i;
        i++;
    }
    if (i < e) ++i;
    if (s.isEmpty()) EXCEPTION(EDATA);
    static const QChar sep = QChar(':');
    static const QChar esc = QChar('\\');
    QStringList params = s.split(sep);
    for (int j = 1; j < (params.size() -1); ++j) {   // A "\:" védet szeparátor, korrekció (első, és utolsó mező nem kell)
        if (params.endsWith(esc)) {
            params[j].chop(1);
            params[j] += sep + params.takeAt(j +1);
            --j;
        }
    }
    enum eIfType {
        IF_VALID,       ///< Nem NULL és létező mező
        IF_NOT_EMPTY,   ///< A mező érték stringgé konvertálva nem üres string
        IF_EQU,         ///< Két érték összehasonlítása
        IF_LITLE,       ///< kisebb (lebegőpontos értékké konvertálva)
        IF_GREATER,     ///< nagyobb (lebegőpontos értékké konvertálva)
        IF_IF,          ///< igaz (bool-lá konverálva)
        IF_ROOT         ///< Tree esetén az aktuális elem egy gyökér elem (nincs parent-je)
    }   t = IF_VALID;
    bool neg = false;
    s = params.first(); // type string
    if (s.startsWith(QChar('!'))) { // A feltétel inverálása
        neg = true;
        s = s.mid(1);
    }
    params.pop_front();
    int n = 0;  // A feltétel paramétereinek a száma
    if      (s.isEmpty())   {   t = IF_IF;          n = 1;  }
    else if (s == "V")      {   t = IF_VALID;       n = 1;  }
    else if (s == "N")      {   t = IF_NOT_EMPTY;   n = 1;  }
    else if (s == "=")      {   t = IF_EQU;         n = 2;  }
    else if (s == ">")      {   t = IF_GREATER;     n = 2;  }
    else if (s == "<")      {   t = IF_LITLE;       n = 2;  }
    else if (s == ">=")     {   t = IF_LITLE;       n = 2;  neg = !neg; }
    else if (s == "<=")     {   t = IF_GREATER;     n = 2;  neg = !neg; }
    else if (s == "R")      {   t = IF_ROOT;        n = 0; }
    else { EXCEPTION(EDATA, 1, params.join(",")); }
    switch (params.size() - n) {
    case 1:
    case 2:     break;
    default:    EXCEPTION(EDATA, params.size());
    }
    plo.clear();    // Feltétel paraméter(ek) exportálható formátum
    QVariantList pl;
    for (int j = 0; j < n; ++j) {
        QString sp = params.at(j);
        pl  << QVariant();
        plo << QString();
        if (sp.startsWith(QChar('\\'))) {
            pl.last()  = sp.mid(1);   // védett string, biztos nem mező érték
            plo.last() = pl.last().toString();
        }
        else {
            pl.last() = sp;                          // vagy string
            plo.last() = pl.last().toString();
            sp = sp.trimmed();
            if (sp.startsWith(QChar('$'))) {    // vagy mező érték (space-k törölve)
                sp = sp.mid(1).trimmed();
                if (sp == "DEFAULT") {  // Ez első paraméterként hivatkozott mező alapértelmezett értéke
                    QString fn = params.first();
                    // Csak a második mező lehet, az első pedig mező hivatkozás kell legyen
                    if (j != 1 || fn[0] != QChar('$')) EXCEPTION(EDATA);
                    fn = fn.mid(1); // - $
                    const cColStaticDescr&cd = o.colDescr(o.toIndex(fn));   // Mező leíró
                    qlonglong st;   // status a set() hez, most nem érdekel...
                    pl.last()  = cd.set(QVariant(cd.colDefault), st);    // Konvertálás tárolási típussá
                    plo.last() = cd.colDefault;                 // string
                }
                else if (o.isIndex(sp)) {
                    int ix = o.toIndex(sp);
                    pl.last() = o.get(ix);
                    plo.last() = exportFieldValue(q1, o.cref(ix));
                }
                else {
                    if (j != 1 || t != IF_VALID) EXCEPTION(EDATA);    // Csak ennél a feltételnél lehet ismeretlen mező név
                    pl.last().clear();
                    plo.last().clear();
                }
            }
        }
    }
    if (n) params = params.mid(n);
    bool f;                                 // A feltétel eredménye ide
    switch (t) {
    case IF_VALID:      f = pl[0].isValid();                     break;
    case IF_NOT_EMPTY:  f = !pl[0].toString().isEmpty();         break;
    case IF_EQU:        f = pl[0].toString() == pl[1].toString(); break;
    case IF_GREATER:    f = pl[0].toDouble() >  pl[1].toDouble(); break;
    case IF_LITLE:      f = pl[0].toDouble() <  pl[1].toDouble(); break;
    case IF_IF:         f = var2bool(pl[0]);                     break;
    case IF_ROOT:       f = !(type & PST_SUB_TREE);             break;
    default:            EXCEPTION(EPROGFAIL);                   break;
    }
    if (neg) f = !f;
    switch (params.size()) {
    case 1:     return f ? params[0] : QString();
    case 2:     return f ? params[0] : params[1];
    default:    EXCEPTION(EPROGFAIL);
    }
    return QString();
}

static inline void nIndent(int &indent, QString& s)
{
    if (s[0].isDigit()) {
        indent += s[0].toLatin1() - '0';
        s.remove(0, 1);
    }
}

#define TABSIZE 4
static inline QString sIndent(int indent)
{
    return QString(indent * TABSIZE, QChar(' '));
}

void cParseSyntax::exportRecordLine(QString &line)
{
    if (!lb.isEmpty()) EXCEPTION(EPROGFAIL);
    if (line.endsWith(QChar('\n'))) line.chop(1);
    if (line.isEmpty()) return;
    actIndndent = indent;
    // Ha az első karakter egy szám, akkor az az indentálást jelenti
    nIndent(actIndndent, line);
    if (line.startsWith("{:")) {  // Begin block
        QStringList sl = line.split(QChar(':'));
        blocks << sIndent(actIndndent) + sl.first();
        blocks << sIndent(actIndndent) + sl.at(1);
        blocks << QString();
        return;   // lb = NULL string
    }
    if (line == "}") {  // End block
        if (blocks.size() < 3) EXCEPTION(EDATA);
        if (blocks.last().simplified().isEmpty()) {
            blocks.pop_back();
            lb = blocks.takeLast();
            blocks.pop_back();
        }
        else {
            lb = blocks.takeLast();
            blocks.pop_back();
            lb.prepend(blocks.takeLast() + QChar('\n'));
            lb += sIndent(actIndndent) + line;
        }
        return;
    }

    int i = 0;
    int c = line[i].toLatin1();
    if (c == '?') { // Feltételes sor
        QString::const_iterator ite = line.constBegin() +1; // ? átugrása
        QString::const_iterator end = line.constEnd();
        QStringList pl;
        QString s = getCondString(ite, end, pl);
        parseString(s, pl);
        if (ite != end) EXCEPTION(EDATA, 0, line);
    }
    else {
        parseString(line);
    }
    return;
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
    return exportFieldValue(q1, r.cref(field));
RFV__syntax_error:
    emsg = QObject::trUtf8("Syntax error.");
    return QString();
}

QString cParseSyntax::getFieldValue(const QString& vname, const QStringList &pl)
{
    QString r;
    bool ok;
    int ix = vname.toInt(&ok) -1;
    if (ok) {
        if (!isContIx(pl, ix)) {
            EXCEPTION(EDATA, ix);
        }
        r = pl.at(ix);
    }
    else {
        if (vname == "PARENT") {
            if (pParent == NULL) return r;
            return exportFieldValue(q1, pParent->o.cref(pParent->o.nameIndex()));
        }
        if      (vname == "ID")   ix = o.idIndex();
        else if (vname == "NAME") ix = o.nameIndex();
        else if (vname == "NOTE") {
            ix = o.noteIndex();
            if (o.isNull(ix)) return r;
        }
        else                      ix = o.toIndex(vname);
        r = exportFieldValue(q1, o.cref(ix));
    }
    return r;
}



QString cParseSyntax::exportFieldValue(QSqlQuery& q, const cRecordFieldConstRef& fr)
{
    QString r;
    const QVariant& vr = fr;
    switch (fr.descr().eColType) {
    case cColStaticDescr::FT_INTEGER:
        if (fr.descr().fKeyType != cColStaticDescr::FT_NONE) {
            qlonglong id = (qlonglong)fr;
            if (id != NULL_ID) {
                r = fr.descr().fKeyId2name(q, id, EX_ERROR);
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
        r = (QString)fr;
        break;
    case cColStaticDescr::FT_TIME:
        r = vr.toTime().toString("hh:mm:ss.zzz");
        break;
    case cColStaticDescr::FT_INTERVAL:
        r =QString::number(((qlonglong)fr) / 1000);  //msec to sec
        break;
    case cColStaticDescr::FT_TEXT:
    case cColStaticDescr::FT_ENUM:
        r = quotedString((QString)fr);
        break;
    case cColStaticDescr::FT_BOOLEAN:
        r = ((bool)fr) ? _sTrue : _sFalse;
        r = r.toUpper();
        break;
    case cColStaticDescr::FT_INTEGER_ARRAY:
        if (fr.descr().fKeyType != cColStaticDescr::FT_NONE) {
            foreach(QVariant v, vr.toList()) {
                qlonglong id = v.toLongLong();
                QString s = fr.descr().fKeyId2name(q, id, EX_ERROR);
                r += quotedString(s) + ", ";
            }
            if (!r.isEmpty()) r.chop(2);
            break;
        }
        // else continue ...
    case cColStaticDescr::FT_REAL_ARRAY:
        foreach(QVariant v, vr.toList()) r += v.toString() + ", ";
        if (!r.isEmpty()) r.chop(2);
        break;
    case cColStaticDescr::FT_TEXT_ARRAY:
    case cColStaticDescr::FT_SET:
        foreach(QVariant v, vr.toList()) r += quotedString(v.toString()) + ", ";
        if (!r.isEmpty()) r.chop(2);
        break;
    case cColStaticDescr::FT_DATE:
    case cColStaticDescr::FT_DATE_TIME:
    case cColStaticDescr::FT_BINARY:
        EXCEPTION(ENOTSUPP);
    }
    return r;
}


void cParseSyntax::exportRecord()
{
    blocks.clear();
    QString sentence = os.getName(_sSentence);
    QStringList lines = sentence.split('\n');
    foreach (QString line, lines) {
        exportRecordLine(line);
        if (lb[0] == QChar('>')) {
            lb = lb.mid(1);
            if (lb.simplified().isEmpty()) continue;
            divert += sIndent(actIndndent) + lb + '\n';
        }
        else {
            bool noend = lb.endsWith(QChar('\\'));
            if (noend) lb.chop(1);
            if (lb.simplified().isEmpty()) continue;
            lb = sIndent(actIndndent) + lb;
            if (!noend) lb += QChar('\n');
            if (blocks.isEmpty()) pSQ->push(lb);
            else                  blocks.last() += lb;
        }
        lb.clear();
    }
    if (blocks.size() > 0) {
        EXCEPTION(EDATA);
    }
}

void cParseSyntax::subExport(int __t, const QString &__tn)
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
        if ((type & PST_MASK) != PST_TREE) EXCEPTION(EDATA);
        t = table;
        n = name;
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    cParseSyntax sos(n, actIndndent, t);
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
    case PST_SUB_TREE:
        if (sos.type != PST_TREE) EXCEPTION(EPROGFAIL); // azonos leíró
        sos.type = type | PST_SUB_TREE;
        break;
    }
    sos.exec(pSQ);
}

void cParseSyntax::exportRecordGroup()
{
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
    divert.clear();
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
            exportRecord();
        } while (q2.next());
    }
    pSQ->push(divert);
}

void cParseSyntax::exportRecordTree()
{
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
            exportRecord();
        } while (o.next(q2));
    }
    pSQ->push(divert);
}

void cParseSyntax::exportRecordSimple()
{
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
        do {
            if (checkHead && o.get(pParent->ixMemb) == pParent->o.get(pParent->ixMemb)) continue;
             exportRecord();
        } while (o.next(q2));
    }
    pSQ->push(divert);
}

