#include "guidata.h"


int tableShapeType(const QString& n, bool __ex)
{
    if (0 == n.compare(_sNo,     Qt::CaseInsensitive)) return TS_NO;
    if (0 == n.compare(_sSimple, Qt::CaseInsensitive)) return TS_SIMPLE;
    if (0 == n.compare(_sTree,   Qt::CaseInsensitive)) return TS_TREE;
    if (0 == n.compare(_sOwner,  Qt::CaseInsensitive)) return TS_OWNER;
    if (0 == n.compare(_sChild,  Qt::CaseInsensitive)) return TS_CHILD;
    if (0 == n.compare(_sSwitch, Qt::CaseInsensitive)) return TS_SWITCH;
    if (0 == n.compare(_sLink,   Qt::CaseInsensitive)) return TS_LINK;
    if (0 == n.compare(_sMember, Qt::CaseInsensitive)) return TS_MEMBER;
    if (0 == n.compare(_sGroup,  Qt::CaseInsensitive)) return TS_GROUP;
    if (0 == n.compare(_sIGroup, Qt::CaseInsensitive)) return TS_IGROUP;
    if (0 == n.compare(_sNGroup, Qt::CaseInsensitive)) return TS_NGROUP;
    if (0 == n.compare(_sIMember,Qt::CaseInsensitive)) return TS_IMEMBER;
    if (0 == n.compare(_sNMember,Qt::CaseInsensitive)) return TS_NMEMBER;
    if (__ex) EXCEPTION(EDATA, -1, n);
    return TS_UNKNOWN;
}

const QString& tableShapeType(int e, bool __ex)
{
    switch(e) {
    case TS_NO:         return _sNo;
    case TS_SIMPLE:     return _sSimple;
    case TS_TREE:       return _sTree;
    case TS_OWNER:      return _sOwner;
    case TS_CHILD:      return _sChild;
    case TS_SWITCH:     return _sSwitch;
    case TS_LINK:       return _sLink;
    case TS_MEMBER:     return _sMember;
    case TS_GROUP:      return _sGroup;
      // TS_INVALID_PAD
    case TS_IGROUP:     return _sIGroup;
    case TS_NGROUP:     return _sNGroup;
    case TS_IMEMBER:    return _sIMember;
    case TS_NMEMBER:    return _sNMember;
    default:            break;
    }
    if (__ex) EXCEPTION(EDATA, e);
    return _sNul;
}

int tableInheritType(const QString& n, bool __ex)
{
    if (0 == n.compare(_sNo,        Qt::CaseInsensitive)) return TIT_NO;
    if (0 == n.compare(_sOnly,      Qt::CaseInsensitive)) return TIT_ONLY;
    if (0 == n.compare(_sOn,        Qt::CaseInsensitive)) return TIT_ON;
    if (0 == n.compare(_sAll,       Qt::CaseInsensitive)) return TIT_ALL;
    if (0 == n.compare(_sReverse,   Qt::CaseInsensitive)) return TIT_REVERSE;
    if (0 == n.compare(_sListed,    Qt::CaseInsensitive)) return TIT_LISTED;
    if (0 == n.compare(_sListedRev, Qt::CaseInsensitive)) return TIT_LISTED_REV;
    if (0 == n.compare(_sListedAll, Qt::CaseInsensitive)) return TIT_LISTED_ALL;
    if (__ex) EXCEPTION(EDATA, -1, n);
    return TIT_UNKNOWN;
}

const QString& tableInheritType(int e, bool __ex)
{
    switch (e) {
    case TIT_NO:            return _sNo;
    case TIT_ONLY:          return _sOnly;
    case TIT_ON:            return _sOn;
    case TIT_ALL:           return _sAll;
    case TIT_REVERSE:       return _sReverse;
    case TIT_LISTED:        return _sListed;
    case TIT_LISTED_REV:    return _sListedRev;
    case TIT_LISTED_ALL:    return _sListedAll;
    default:                break;
    }
    if (__ex) EXCEPTION(EDATA, e);
    return _sNul;
}

int orderType(const QString& n, bool __ex)
{
    if (0 == n.compare(_sNo,   Qt::CaseInsensitive)) return OT_NO;
    if (0 == n.compare(_sAsc,  Qt::CaseInsensitive)) return OT_ASC;
    if (0 == n.compare(_sDesc, Qt::CaseInsensitive)) return OT_DESC;
    if (__ex) EXCEPTION(EDATA, -1, n);
    return OT_UNKNOWN;
}

const QString& orderType(int e, bool __ex)
{
    switch(e) {
    case OT_NO:         return _sNo;
    case OT_ASC:        return _sAsc;
    case OT_DESC:       return _sDesc;
    default:            break;
    }
    if (__ex) EXCEPTION(EDATA, e);
    return _sNul;
}


int filterType(const QString& n, bool __ex)
{
    if (0 == n.compare(_sBegin,    Qt::CaseInsensitive)) return FT_BEGIN;
    if (0 == n.compare(_sLike,     Qt::CaseInsensitive)) return FT_LIKE;
    if (0 == n.compare(_sSimilar,  Qt::CaseInsensitive)) return FT_SIMILAR;
    if (0 == n.compare(_sRegexp,   Qt::CaseInsensitive)) return FT_REGEXP;
    if (0 == n.compare(_sRegexpI,  Qt::CaseInsensitive)) return FT_REGEXPI;
    if (0 == n.compare(_sBig,      Qt::CaseInsensitive)) return FT_BIG;
    if (0 == n.compare(_sLitle,    Qt::CaseInsensitive)) return FT_LITLE;
    if (0 == n.compare(_sInterval, Qt::CaseInsensitive)) return FT_INTERVAL;
    if (0 == n.compare(_sProc,     Qt::CaseInsensitive)) return FT_PROC;
    if (0 == n.compare(_sSQL,      Qt::CaseInsensitive)) return FT_SQL_WHERE;
    if (__ex) EXCEPTION(EDATA, -1, n);
    return FT_UNKNOWN;
}

const QString& filterType(int e, bool __ex)
{
    switch(e) {
    case FT_BEGIN:      return _sBegin;
    case FT_LIKE:       return _sLike;
    case FT_SIMILAR:    return _sSimilar;
    case FT_REGEXP:     return _sRegexp;
    case FT_REGEXPI:    return _sRegexpI;
    case FT_BIG:        return _sBig;
    case FT_LITLE:      return _sLitle;
    case FT_INTERVAL:   return _sInterval;
    case FT_PROC:       return _sProc;
    case FT_SQL_WHERE:  return _sSQL;
    default:            break;
    }
    if (__ex) EXCEPTION(EDATA, e);
    return _sNul;
}

/* ------------------------------ cTableShape ------------------------------ */

cTableShape::cTableShape() : cRecord(), shapeFields(this)
{
    _pFeatures = NULL;
    _set(cTableShape::descr());
}

cTableShape::cTableShape(const cTableShape &__o) : cRecord(), shapeFields(this, __o.shapeFields)
{
    _pFeatures = NULL;
    _set(cTableShape::descr());
    _cp(__o);
}

cTableShape::~cTableShape()
{
    clearToEnd();
}

int  cTableShape::_ixFeatures = NULL_IX;
int  cTableShape::_ixTableShapeType = NULL_IX;
const cRecStaticDescr&  cTableShape::descr() const
{
    if (initPDescr<cTableShape>(_sTableShapes)) {
        _ixFeatures = _descr_cTableShape().toIndex(_sFeatures);
        _ixTableShapeType = _descr_cTableShape().toIndex(_sTableShapeType);
        CHKENUM(_ixTableShapeType, tableShapeType);
        CHKENUM(_sTableInheritType, tableInheritType);
    }
    return *_pRecordDescr;
}
CRECDEF(cTableShape)

void cTableShape::toEnd()
{
    toEnd(_descr_cTableShape().idIndex());
    toEnd(_ixFeatures);
}

bool cTableShape::toEnd(int i)
{
    if (i == _descr_cTableShape().idIndex()) {
        atEndCont(shapeFields, cTableShapeField::ixTableShapeId());
        return true;
    }
    if (i == _ixFeatures) {
        pDelete(_pFeatures);
        return true;
    }
    return false;
}

void cTableShape::clearToEnd()
{
    shapeFields.clear();
    pDelete(_pFeatures);
}

bool cTableShape::insert(QSqlQuery &__q, bool __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (shapeFields.count()) {
        if (getBool(_sIsReadOnly)) shapeFields.sets(_sIsReadOnly, QVariant(true));
        int i = shapeFields.insert(__q, __ex);
        return i == shapeFields.count();
    }
    return true;
}
bool cTableShape::rewrite(QSqlQuery &__q, bool __ex)
{
    return tRewrite(__q, shapeFields, 0, __ex);
}

/// A típus mezőnek lehetnek olyan értékei is, melyek az adatbázisban nem szerepelnek,
/// viszont a megjelenítésnél van szerepük.
/// Az alapértelmezett metódus ezeket az értékeket eldobná, természetesen ezek az extra értékek
/// elvesznek, ha az objektumoot kiírnámk az adatbázisba. Az objektum státuszában bebillentjük az invalid bitet, ha extra tíous lett megadva.
/// Az extra értékeket a getId() matódus nem adja vissza, mert a konverziónál elvesznek, csak a get() metódus használható,
/// ami egy QStringList -et ad vissza, és a enum2set() és tableShapeType() függvénnyekkel konvertálható qlonglong típussá.
cTableShape& cTableShape::setShapeType(qlonglong __t)
{
    static const qlonglong extraValues = ENUM2SET4(TS_IGROUP, TS_NGROUP, TS_IMEMBER, TS_NMEMBER);
    if (extraValues & __t) {   // extra érték (bit)?
        if (_fields.isEmpty()) cRecord::set();
        _fields[_ixTableShapeType] = set2lst(tableShapeType, __t, false);
        _stat |= ES_MODIFY | ES_INVALID;
        // toEnd(__i);          // Feltételezzük, hogy most nem kell
        // fieldModified(__i);  // Feltételezzük, hogy most nem kell
        return *this;
    }
    return *(set(_ixTableShapeType, __t).reconvert<cTableShape>());
}

int cTableShape::fetchFields(QSqlQuery& q)
{
    shapeFields.clear();
    cTableShapeField f;
    int fix = f.toIndex(_sTableShapeId);
    f.setId(fix, getId());
    if (f.fetch(q, false, _bit(fix), f.iTab(_sFieldSequenceNumber))) {
        do {
            shapeFields << f;
        } while (f.next(q));
    }
    return shapeFields.count();
}

/// A megjelenítés típusát elöször beállítja az alapértelmezett TS_SIMPLE típusra.
/// Törli a shapeField konténert, majd feltölti azt:
/// Felveszi a tábla összes mezőjét, a *note és *title mező értéke a mező neve lessz.
/// A mező sorrendjét meghatározó "field_sequence_number" mező értéke a mező sorszám * 10 lessz.
/// Ha talál olyan mezőt, ami távoli kulcs, és 'FT_OWNER'tulajdonságú, akkor a tábla megjelenítés típushoz hozzáadja az TS_CHILD jelzőt.
/// Ha a távoli kulcs a saját táblára mutat és tulajdonságot jelöl, akkor a TS_SIMPLE tíoust lecseréli TS_TREE típusra.
/// A fenti két esetben a mezőn beállítja az 'is_hide' mezőt true-ra.
/// Kapcsoló tábla esetén :
bool cTableShape::setDefaults(QSqlQuery& q)
{
    bool r = true;
    int type = enum2set(TS_SIMPLE);
    const cRecStaticDescr& rDescr = *cRecStaticDescr::get(getName(_sTableName), getName(_sSchemaName));
    shapeFields.clear();
    for (int i = 0; rDescr.isIndex(i); ++i) {
        const cColStaticDescr& cDescr = rDescr.colDescr(i);
        cTableShapeField fm;
        fm.setName(cDescr);
        fm.setName(_sTableShapeFieldNote, cDescr);
        fm.setName(_sTableShapeFieldTitle, cDescr);
        fm.setId(_sFieldSequenceNumber, (i + 1) * 10);
        bool ro = (!fm.isUpdatable()) || rDescr.primaryKey()[i] || rDescr.autoIncrement()[i];
        bool hide = i == rDescr.idIndex(false);
        if (i == 0 && rDescr.idIndex(false) == 0 && rDescr.autoIncrement().at(0)) {
            hide = true;
            ro   = true;
        }
        else if (rDescr.deletedIndex(false) == i) {
            hide = true;
            ro   = true;
        }
        else if (cDescr.fKeyType == cColStaticDescr::FT_OWNER) {
            ro   = true;
            hide = true;
            type = (type & ~enum2set(TS_SIMPLE)) | enum2set(TS_CHILD);
        }
        // Önmagára mutató fkey?
        else if (cDescr.fKeyType     == cColStaticDescr::FT_PROPERTY
         &&      rDescr.tableName()  == cDescr.fKeyTable
         &&      rDescr.schemaName() == cDescr.fKeySchema) {
            type = (type & ~enum2set(TS_SIMPLE)) | enum2set(TS_TREE);
            hide = true;
        }
        fm.setBool(_sIsReadOnly, ro);
        fm.setBool(_sIsHide, hide);
        shapeFields << fm;
    }
    // Kapcsoló tábla ?
    if (rDescr.cols() >= 3                                            // Legalább 3 mező
     && rDescr.primaryKey().count(true) == 1 && rDescr.primaryKey()[0]  // Első elem egy elsődleges kulcs
     && rDescr.colDescr(1).fKeyType == cColStaticDescr::FT_PROPERTY     // Masodik egy távoli kulcs
     && rDescr.colDescr(2).fKeyType == cColStaticDescr::FT_PROPERTY) {  // Harmadik is egy távoli kulcs
        if (rDescr.cols() == 3)  type &= ~enum2set(TS_SIMPLE);
        if (rDescr.colDescr(1).fKeyTable == rDescr.colDescr(2).fKeyTable) type |= enum2set(TS_LINK);
        else                                                              type |= enum2set(TS_SWITCH);
        cTableShape mod;
        QString tn = rDescr.colDescr(1).fKeyTable;
        mod.setName(tn);
        if (1 == mod.completion(q)) setId(_sLeftShapeId, mod.getId());
        else {
            DWAR() << QObject::trUtf8("A tag %1 tábla azonos nevű leírüja hiányzik.").arg(tn) << endl;
            r = false;
        }
        mod.clear();
        tn = rDescr.colDescr(2).fKeyTable;
        mod.setName(tn);
        if (1 == mod.completion(q)) setId(_sRightShapeId, mod.getId());
        else {
            DWAR() << QObject::trUtf8("A csoport %1 tábla azonos nevű leírüja hiányzik.").arg(tn) << endl;
            r = false;
        }
    }
    setId(_ixTableShapeType, type);
    setBool(_sIsReadOnly, !rDescr.isUpdatable());
    return r;
}

bool cTableShape::fset(const QString& _fn, const QString& _fpn, const QVariant& _v, bool __ex)
{
    int i = shapeFields.indexOf(_fn);
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1, _fn);
        return false;
    }
    cTableShapeField& f = *(shapeFields[i]);
    if (!__ex && !f.isIndex(_fpn)) return false;
    f.set(_fpn, _v);
    return true;
}

bool cTableShape::fsets(const QStringList& _fnl, const QString& _fpn, const QVariant& _v, bool __ex)
{
    bool r = true;
    foreach (QString fn, _fnl) r = fset(fn, _fpn, _v, __ex) && r;
    return r;
}

bool cTableShape::addFilter(const QString& _fn, const QString& _t, const QString& _d, bool __ex)
{
    int i = shapeFields.indexOf(_fn);
    if (i < 0) {
        if (__ex) EXCEPTION(EDATA, -1, _fn);
        return false;
    }
    cTableShapeField& f = *(shapeFields[i]);
    return f.addFilter(_t, _d, __ex);
}

bool cTableShape::addFilter(const QStringList& _fnl, const QString& _t, const QString& _d, bool __ex)
{
    bool r = true;
    foreach (QString fn, _fnl) r = addFilter(fn, _t, _d, __ex) && r;
    return r;
}

bool cTableShape::addFilter(const QStringList& _fnl, const QStringList& _ftl, bool __ex)
{
    bool r = true;
    foreach (QString fn, _fnl) {
        foreach (QString ft, _ftl) {
            r = addFilter(fn, ft, _sNul, __ex) && r;
        }
    }
    return r;
}

bool cTableShape::setAllOrders( QStringList& _ord, bool __ex)
{
    if (shapeFields.isEmpty()) {
        QString msg = trUtf8("A shape Fields konténer üres.");
        if (__ex) EXCEPTION(EPROGFAIL, -1, msg);
        DERR() << msg << endl;
        return false;
    }
    qlonglong deford;
    qlonglong s = enum2set(orderType, _ord, __ex);
    if (s < 0LL) return false;  /// Hibás volt egy név az _ord listában, de __ex false, ezért nem dobot kizárást, vége
    if (!s) {
        s = enum2set(OT_NO);/// Üres lista esetén ez az alapértelnezés
        deford = OT_NO;
    }
    else {
        deford = enum2set(orderType, _ord[0]);
    }
    tTableShapeFields::Iterator i, e = shapeFields.end();
    int seq = 0;
    for (i = shapeFields.begin(); e != i; ++i) {
        cTableShapeField& f = **i;
        f.set(_sOrdTypes, _ord);
        if (s == enum2set(OT_NO)) continue;
        f.setId(_sOrdInitType, deford);
        seq += 10;
        f.setId(_sOrdInitSequenceNumber, deford);
    }
    return true;
}

bool cTableShape::setOrders(const QStringList& _fnl, QStringList& _ord, bool __ex)
{
    if (shapeFields.isEmpty()) {
        QString msg = trUtf8("A shape Fields konténer üres.");
        if (__ex) EXCEPTION(EPROGFAIL, -1, msg);
        DERR() << msg << endl;
        return false;
    }
    qlonglong deford;
    qlonglong s = enum2set(orderType, _ord, __ex);  // SET
    if (s < 0LL) return false;
    if (!s) {
        s = enum2set(OT_NO);    // SET
        deford = OT_NO;         // ENUM
    }
    else {
        deford = orderType(_ord[0]);    // ENUM!
    }
    tTableShapeFields::Iterator i, e = shapeFields.end();
    int seq = 0;
    const int step = 10;
    const int ixOrdInitSequenceNumber = shapeFields.front()->toIndex(_sOrdInitSequenceNumber);
    // Keressük a legnagyob sorszámot, onnan folytatjuk
    for (i = shapeFields.begin(); e != i; ++i) {
        cTableShapeField& f = **i;
        if (!f.isNull(ixOrdInitSequenceNumber)) {
            int _seq = f.getId(ixOrdInitSequenceNumber);
            if (_seq > seq) seq = _seq;
        }
    }

    foreach (QString fn, _fnl) {
        int i = shapeFields.indexOf(fn);
        if (i < 0) {
            if (__ex) EXCEPTION(EDATA, -1, fn);
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.setId(_sOrdTypes, s);
        f.setId(_sOrdInitType, deford);
        seq += step;
        f.setId(ixOrdInitSequenceNumber, seq);
    }
    return true;
}

bool cTableShape::setFieldSeq(const QStringList& _fnl, int last, bool __ex)
{
    if (shapeFields.isEmpty()) {
        QString msg = trUtf8("A shape Fields konténer üres.");
        if (__ex) EXCEPTION(EPROGFAIL, -1, msg);
        DERR() << msg << endl;
        return false;
    }
    const int step = 10;
    int seq = last + step;
    QStringList fnl = _fnl;
    for (int i = 0; i < shapeFields.size(); ++i) {
        cTableShapeField& f = *(shapeFields[i]);
        if (f.getId(_sFieldSequenceNumber) > last) {    // Ha last -nál kisebb, akkor nem bántjuk
            QString fn = f.getName();
            if (_fnl.indexOf(fn) < 0)  {              // Ha nem szerepel a felsorolásban, akkor a végére soroljzk
                fnl << fn;
            }
        }
    }
    foreach (QString fn, fnl) {     // A last utániakat besoroljuk, elöször a felsoroltak, aztán a maradék
        int i = shapeFields.indexOf(fn);
        if (i < 0) {
            if (__ex) EXCEPTION(EDATA, -1, fn);
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.set(_sFieldSequenceNumber, seq);
        seq += step;
    }
    return true;
}

bool cTableShape::setOrdSeq(const QStringList& _fnl, int last, bool __ex)
{
    if (shapeFields.isEmpty()) {
        QString msg = trUtf8("A shape Fields konténer üres.");
        if (__ex) EXCEPTION(EPROGFAIL, -1, msg);
        DERR() << msg << endl;
        return false;
    }
    const int step = 10;
    const int ixOrdInitSequenceNumber = shapeFields.front()->toIndex(_sOrdInitSequenceNumber);
    int seq = last + step;
    QStringList fnl = _fnl;
    for (int i = 0; i < shapeFields.size(); ++i) {
        cTableShapeField& f = *(shapeFields[i]);
        if (f.getId(ixOrdInitSequenceNumber) > last) {    // Ha last -nál kisebb, akkor nem bántjuk
            QString fn = f.getName();
            if (_fnl.indexOf(fn) < 0)  {              // Ha nem szerepel a felsorolásban, akkor a végére soroljzk
                fnl << fn;
            }
        }
    }

    foreach (QString fn, _fnl) {
        int i = shapeFields.indexOf(fn);
        if (i < 0) {
            if (__ex) EXCEPTION(EDATA, -1, fn);
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.set(ixOrdInitSequenceNumber, seq);
        seq += step;
    }
    return true;
}


bool cTableShape::fetchLeft(QSqlQuery& q, cTableShape * _po, bool _ex) const
{
    qlonglong oid = getId(_sLeftShapeId);
    if (oid == NULL_ID || !_po->fetchById(q, oid)) {
        if (_ex) EXCEPTION(EDATA);
        return false;
    }
    return true;
}

bool cTableShape::fetchRight(QSqlQuery& q, cTableShape * _po, bool _ex) const
{
    qlonglong oid = getId(_sRightShapeId);
    if (oid == NULL_ID || !_po->fetchById(q, oid)) {
        if (_ex) EXCEPTION(EDATA);
        return false;
    }
    return true;
}

cTableShapeField *cTableShape::addField(const QString& _name, const QString& _title, const QString& _note, bool __ex)
{
    if (0 <= shapeFields.indexOf(_name)) {
        if (__ex) EXCEPTION(EUNIQUE, -1, _name);
        return NULL;
    }
    cTableShapeField *p = new cTableShapeField();
    p->setName(_name);
    p->setName(_sTableShapeFieldTitle, _title.size() ? _title : _name);
    p->setName(_sTableShapeFieldNote,  _note.size()  ? _note  : _name);
    shapeFields << p;
    return p;
}

/* ------------------------------ cTableShapeField ------------------------------ */

int cTableShapeField::_ixTableShapeId = NULL_IX;

cTableShapeField::cTableShapeField() : cRecord(), shapeFilters(this)
{
    _pFeatures = NULL;
    _set(cTableShapeField::descr());
}

cTableShapeField::cTableShapeField(const cTableShapeField &__o) : cRecord(), shapeFilters(this, __o.shapeFilters)
{
    _pFeatures = NULL;
    _set(cTableShapeField::descr());
    _cp(__o);
}

cTableShapeField::cTableShapeField(QSqlQuery& q) : cRecord(), shapeFilters(this)
{
    _pFeatures = NULL;
    _set(q.record(), cTableShapeField::descr());
}

cTableShapeField::~cTableShapeField()
{
    clearToEnd();
}

int cTableShapeField::_ixFeatures = NULL_IX;

const cRecStaticDescr&  cTableShapeField::descr() const
{
    if (initPDescr<cTableShapeField>(_sTableShapeFields)) {
        _ixFeatures = _descr_cTableShapeField().toIndex(_sFeatures);
        _ixTableShapeId = _descr_cTableShapeField().toIndex(_sTableShapeId);
        CHKENUM(_sOrdTypes, orderType);
    }
    return *_pRecordDescr;
}

CRECDEF(cTableShapeField)

void cTableShapeField::toEnd()
{
    toEnd(_descr_cTableShapeField().idIndex());
    toEnd(_ixFeatures);
}

bool cTableShapeField::toEnd(int i)
{
    if (i == _descr_cTableShapeField().idIndex()) {
        // Ha üres nem töröljük, mert minek,
        if (shapeFilters.isEmpty()
        // ha a prortoknál nincs kitöltve az ID, akkor is békénhagyjuk. (csak az első elemet vizsgáljuk
         || shapeFilters.first()->getId() == NULL_ID
        // ha stimmel a node_id akkor sem töröljük
         || shapeFilters.first()->getId(_sTableShapeFieldId) == getId())
            return true;
        shapeFilters.clear();
        return true;
    }
    if (i == _ixFeatures) {
        pDelete( _pFeatures);
        return true;
    }
    return false;
}

void cTableShapeField::clearToEnd()
{
    shapeFilters.clear();
}

bool cTableShapeField::insert(QSqlQuery &__q, bool __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (shapeFilters.count()) {
        int i = shapeFilters.insert(__q, __ex);
        return i == shapeFilters.count();
    }
    return true;
}

bool cTableShapeField::rewrite(QSqlQuery &__q, bool __ex)
{
    return tRewrite(__q, shapeFilters, 0, __ex);
}

int cTableShapeField::fetchFilters(QSqlQuery& q)
{
    return shapeFilters.fetch(q, getId());
}

bool cTableShapeField::addFilter(const QString& _t, const QString& _d, bool __ex)
{
    int t = filterType(_t, __ex);
    if (t == FT_UNKNOWN) return false;
    if (shapeFilters.indexOf(_sFilterType, QVariant(_t)) >= 0) {
        // két azonos típus nem lehet !!
        if (__ex) EXCEPTION(EDATA);
        return false;
    }
    cTableShapeFilter *pF = new cTableShapeFilter();
    pF->setName(_sFilterType, _t);
    if (!_d.isEmpty()) pF->setName(_sTableShapeFieldNote);
    shapeFilters << pF;
    return true;
}


bool cTableShapeField::fetchByNames(QSqlQuery& q, const QString& tsn, const QString& fn, bool __ex)
{
    clear();
    qlonglong tsid = cTableShape().getIdByName(q, tsn, __ex);
    if (tsid == NULL_ID) return false;
    setId(_sTableShapeId, tsid);
    setName(_sTableShapeFieldName, fn);
    if (completion(q) != 1) EXCEPTION(EDATA, tsid, fn);
    return true;
}

qlonglong cTableShapeField::getIdByNames(QSqlQuery& q, const QString& tsn, const QString& fn)
{
    cTableShapeField o;
    o.fetchByNames(q, tsn, fn, true);
    return o.getId();
}

/* ------------------------------ cTableShapeFilter ------------------------------ */
CRECCNTR(cTableShapeFilter)
int cTableShapeFilter::_ixTableShapeFieldId = NULL_IX;

const cRecStaticDescr&  cTableShapeFilter::descr() const
{
    if (initPDescr<cTableShapeFilter>(_sTableShapeFilters)) {
        CHKENUM(_sFilterType, filterType);
        _ixTableShapeFieldId = _descr_cTableShapeFilter().toIndex(_sTableShapeFieldId);
    }
    return *_pRecordDescr;
}

CRECDEFD(cTableShapeFilter)
/* ------------------------------ cEnumVal ------------------------------ */
DEFAULTCRECDEF(cEnumVal, _sEnumVals)

QString cEnumVal::title(QSqlQuery& q, const QString& _ev, const QString& _tn)
{
    QString sql = "SELECT enum_name2note(" + quoted(_ev);
    if (_tn.isEmpty() == false) sql += QChar(',') + quoted(_tn);
    sql += ")";
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    if (!q.first()) EXCEPTION(EDBDATA); // Az SQL függvény akkor is ad értéket, ha nincs rekord.
    return q.value(0).toString();
}

int cEnumVal::delByTypeName(QSqlQuery& q, const QString& __n, bool __pat)
{
    QString sql = "DELETE FROM " + tableName() + " WHERE enum_type_name " + (__pat ? "LIKE " : "= ") + quoted(__n);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    int n = q.numRowsAffected();
    return  n;
}

bool cEnumVal::delByNames(QSqlQuery& q, const QString& __t, const QString& __n)
{
    QString sql = "DELETE FROM " + tableName() + " WHERE enum_type_name = " + quoted(__t) + " AND enum_val_name = " + quoted(__n);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    int n = q.numRowsAffected();
    return  (bool)n;
}

/* ------------------------------ cMenuItems ------------------------------ */
CRECDEFD(cMenuItem)

cMenuItem::cMenuItem() : cRecord()
{
    _pFeatures = NULL;
    _set(cMenuItem::descr());
}
cMenuItem::cMenuItem(const cMenuItem& __o) : cRecord()
{
    _pFeatures = NULL;
    _cp(__o);
}


int cMenuItem::_ixFeatures = NULL_IX;

const cRecStaticDescr&  cMenuItem::descr() const
{
    if (initPDescr<cMenuItem>(_sMenuItems)) {
        _ixFeatures = _descr_cMenuItem().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

bool cMenuItem::fetchFirstItem(QSqlQuery& q, const QString& _appName, qlonglong upId)
{
    clear();
    QString pl = privilegeLevel(lanView::user().privilegeLevel());  // Az aktuális jogosultsági szint
    QString sql =
            "SELECT * FROM menu_items"
            " WHERE app_name = ?"                           // A megadott nevű app menüje
              " AND menu_rights <= ?"                       // Jogosultság szerint szűrve
              " AND %1"                                     // Fő vagy al menü
            " ORDER BY item_sequence_number ASC NULLS LAST";
    bool    r;
    if (upId != NULL_ID) {
        sql = sql.arg("upper_menu_item_id = ?");                      // A mgadott menű almenü pontjai
        r = execSql(q, sql, _appName, pl, upId);
    }
    else {
        sql = sql.arg("upper_menu_item_id IS NULL");                  // Fő menü
        r = execSql(q, sql, _appName, pl);
    }
    if (r) set(q);
    return r;
}


int cMenuItem::delByAppName(QSqlQuery &q, const QString &__n, bool __pat) const
{
    QString sql = "DELETE FROM ";
    sql += tableName() + " WHERE " + dQuoted(_sAppName) + (__pat ? " LIKE " : " = ") + quoted(__n);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    int n = q.numRowsAffected();
    PDEB(VVERBOSE) << QObject::trUtf8("delByAppName SQL : \"%1\" removed %2 records.").arg(sql).arg(n) << endl;
    return  n;
}
