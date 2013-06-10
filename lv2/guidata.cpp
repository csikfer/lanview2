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

cTableShape::cTableShape() : cRecord()
{
    _pMagicMap = NULL;
    _set(cTableShape::descr());
}

cTableShape::cTableShape(const cTableShape &__o) : cRecord()
{
    _pMagicMap = NULL;
    _set(cTableShape::descr());
    _cp(__o);
    shapeFields = __o.shapeFields;
}

cTableShape::~cTableShape()
{
    clearToEnd();
}

int  cTableShape::_ixProperties = NULL_IX;

const cRecStaticDescr&  cTableShape::descr() const
{
    if (initPDescr<cTableShape>(_sTableShapes)) {
        _ixProperties = _descr_cTableShape().toIndex(_sProperties);
        CHKENUM(_sTableShapeType, tableShapeType);
        CHKENUM(_sTableInheritType, tableInheritType);
    }
    return *_pRecordDescr;
}
CRECDEF(cTableShape)

void cTableShape::toEnd()
{
    toEnd(_descr_cTableShape().idIndex());
    toEnd(_ixProperties);
}

bool cTableShape::toEnd(int i)
{
    if (i == _descr_cTableShape().idIndex()) {
        // Ha üres nem töröljük, mert minek,
        if (shapeFields.isEmpty()
        // ha a prortoknál nincs kitöltve az ID, akkor is békénhagyjuk. (csak az első elemet vizsgáljuk
         || shapeFields.first()->getId() == NULL_ID
        // ha stimmel a node_id akkor sem töröljük
         || shapeFields.first()->getId(_sTableShapeId) == getId())
            return true;
        shapeFields.clear();
        return true;
    }
    if (_ixProperties) {
        if (_pMagicMap != NULL) {
            delete _pMagicMap;
            _pMagicMap = NULL;
        }
        return true;
    }
    return false;
}

void cTableShape::clearToEnd()
{
    shapeFields.clear();
    if (_pMagicMap != NULL) {
        delete _pMagicMap;
        _pMagicMap = NULL;
    }
}

bool cTableShape::insert(QSqlQuery &__q, bool __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (shapeFields.count()) {
        shapeFields.clearId();
        shapeFields.setId(_sTableShapeId, getId());
        if (getBool(_sIsReadOnly)) shapeFields.set(_sIsReadOnly, QVariant(true));
        int i = shapeFields.insert(__q, __ex);
        return i == shapeFields.count();
    }
    return true;
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
        fm.setName(_sTableShapeFieldDescr, cDescr);
        fm.setName(_sTableShapeFieldTitle, cDescr);
        fm.setId(_sFieldSequenceNumber, (i + 1) * 10);
        bool ro = (!fm.isUpdatable()) || rDescr.primaryKey()[i] || rDescr.autoIncrement()[i];
        if (cDescr.fKeyType == cColStaticDescr::FT_OWNER) {
            ro = true;
            type |= enum2set(TS_CHILD);
            cTableShape owner;
            owner.setName(cDescr.fKeyTable);
            if (1 == owner.completion(q)) setId(_sLeftShapeId, owner.getId());
            else {
                DWAR() << "Ovner " << cDescr.fKeyTable << " table Shape not found, or unequivocal." << endl;
                r = false;
            }
        }
        if (cDescr.fKeyType     == cColStaticDescr::FT_PROPERTY
         && rDescr.tableName()  == cDescr.fKeyTable
         && rDescr.schemaName() == cDescr.fKeySchema) {
            type = (type & ~enum2set(TS_SIMPLE)) | enum2set(TS_TREE);
        }
        fm.set(_sIsReadOnly, ro);
        shapeFields << fm;
    }
    // Kapcsoló tábla ?
    if (rDescr.cols() >= 3                                            // Legalább 3 mező
     && rDescr.primaryKey().count(true) == 1 && rDescr.primaryKey()[0]  // Első elem egy prímary kulcs
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
            DWAR() << "Member " << tn << " table Shape not found, or unequivocal for switch table." << endl;
            r = false;
        }
        mod.clear();
        tn = rDescr.colDescr(2).fKeyTable;
        mod.setName(tn);
        if (1 == mod.completion(q)) setId(_sRightShapeId, mod.getId());
        else {
            DWAR() << "Group " << tn << " table Shape not found, or unequivocal for switch table." << endl;
            r = false;
        }
    }
    setId(_sTableShapeType, type);
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

bool cTableShape::setAllOrders( QStringList& _ord, int last, bool __ex)
{
    qlonglong s = enum2set(orderType, _ord, __ex);
    if (s < 0LL) return false;
    if (!s) s = enum2set(OT_NO);
    int step = 10;
    int seq = last + step;
    tTableShapeFields::Iterator i, e = shapeFields.end();
    for (i = shapeFields.begin(); e != i; ++i) {
        cTableShapeField& f = **i;
        f.set(_sOrdTypes, _ord);
        if (s == enum2set(OT_NO)) continue;
        f.set(_sOrdInitType, _ord.first());
        f.set(_sOrdInitSequenceNumber, seq);
        seq += step;
    }
    return true;
}

bool cTableShape::setOrders(const QStringList& _fnl, QStringList& _ord, int last, bool __ex)
{
    qlonglong s = enum2set(orderType, _ord, __ex);
    if (s < 0LL) return false;
    if (!s) s = enum2set(OT_NO);
    int step = 10;
    int seq = last + step;
    foreach (QString fn, _fnl) {
        int i = shapeFields.indexOf(fn);
        if (i < 0) {
            if (__ex) EXCEPTION(EDATA, -1, fn);
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.set(_sOrdTypes, _ord);
        if (s == enum2set(OT_NO)) continue;
        f.set(_sOrdInitType, _ord.first());
        f.set(_sOrdInitSequenceNumber, seq);
        seq += step;
    }
    return true;
}

bool cTableShape::setFieldSeq(const QStringList& _fnl, int last, bool __ex)
{
    int step = 10;
    int seq = last + step;
    foreach (QString fn, _fnl) {
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
    int step = 10;
    int seq = last + step;
    foreach (QString fn, _fnl) {
        int i = shapeFields.indexOf(fn);
        if (i < 0) {
            if (__ex) EXCEPTION(EDATA, -1, fn);
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.set(_sOrdInitSequenceNumber, seq);
        seq += step;
    }
    return true;
}


template void _SplitMagicT<cTableShape>(cTableShape& o, bool __ex);

tMagicMap&  cTableShape::splitMagic(bool __ex)
{
    DBGFN();
    PDEB(VVERBOSE) << VDEB(_ixProperties) << endl;
    _SplitMagicT<cTableShape>(*this, __ex);
    return *_pMagicMap;
}

bool cTableShape::fetchLeft(QSqlQuery& q, cTableShape * _po, bool _ex = true) const
{
    qlonglong oid = getId(_sLeftShapeId);
    if (oid == NULL_ID || !_po->fetchById(q, oid)) {
        if (_ex) EXCEPTION(EDATA);
        return false;
    }
    return true;
}

bool cTableShape::fetchRight(QSqlQuery& q, cTableShape * _po, bool _ex = true) const
{
    qlonglong oid = getId(_sRightShapeId);
    if (oid == NULL_ID || !_po->fetchById(q, oid)) {
        if (_ex) EXCEPTION(EDATA);
        return false;
    }
    return true;
}

/* ------------------------------ cTableShapeField ------------------------------ */

cTableShapeField::cTableShapeField() : cRecord()
{
    _pMagicMap = NULL;
    _set(cTableShapeField::descr());
}

cTableShapeField::cTableShapeField(const cTableShapeField &__o) : cRecord()
{
    _pMagicMap = NULL;
    _set(cTableShapeField::descr());
    _cp(__o);
    shapeFilters = __o.shapeFilters;

}

cTableShapeField::cTableShapeField(QSqlQuery& q)
{
    _pMagicMap = NULL;
    _set(cTableShapeField::descr());
    set(q);
}

cTableShapeField::~cTableShapeField()
{
    clearToEnd();
}

int cTableShapeField::_ixProperties = NULL_IX;

const cRecStaticDescr&  cTableShapeField::descr() const
{
    if (initPDescr<cTableShapeField>(_sTableShapeFields)) {
        _ixProperties = _descr_cTableShapeField().toIndex(_sProperties);
        CHKENUM(_sOrdTypes, orderType);
    }
    return *_pRecordDescr;
}

CRECDEF(cTableShapeField)

void cTableShapeField::toEnd()
{
    toEnd(_descr_cTableShapeField().idIndex());
    toEnd(_ixProperties);
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
    if (_ixProperties) {
        if (_pMagicMap != NULL) {
            delete _pMagicMap;
            _pMagicMap = NULL;
        }
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
        shapeFilters.clearId();
        shapeFilters.setId(_sTableShapeFieldId, getId());
        int i = shapeFilters.insert(__q, __ex);
        return i == shapeFilters.count();
    }
    return true;
}

int cTableShapeField::fetchFilters(QSqlQuery& q)
{
    return shapeFilters.fetch(q, false, _sTableShapeFieldId, getId());
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
    if (!_d.isEmpty()) pF->setName(_sTableShapeFieldDescr);
    shapeFilters << pF;
    return true;
}

template void _SplitMagicT<cTableShapeField>(cTableShapeField& o, bool __ex);

tMagicMap&  cTableShapeField::splitMagic(bool __ex)
{
    DBGFN();
    PDEB(VVERBOSE) << VDEB(_ixProperties) << endl;
    _SplitMagicT<cTableShapeField>(*this, __ex);
    return *_pMagicMap;
}

/* ------------------------------ cTableShapeFilter ------------------------------ */
CRECCNTR(cTableShapeFilter)

const cRecStaticDescr&  cTableShapeFilter::descr() const
{
    if (initPDescr<cTableShapeFilter>(_sTableShapeFilters)) {
        CHKENUM(_sFilterType, filterType);
    }
    return *_pRecordDescr;
}

CRECDEFD(cTableShapeFilter)
/* ------------------------------ cEnumVal ------------------------------ */
DEFAULTCRECDEF(cEnumVal, _sEnumVals)

QString cEnumVal::title(QSqlQuery& q, const QString& _ev, const QString& _tn)
{
    QString sql = "SELECT enum_name2descr(" + quoted(_ev);
    if (_tn.isEmpty() == false) sql += _sComma + quoted(_tn);
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
DEFAULTCRECDEF(cMenuItem, _sMenuItems)

bool cMenuItem::fetchFirstItem(QSqlQuery& q, const QString& _appName, qlonglong upId)
{
    clear();
    setName(_sAppName, _appName);
    if (upId != NULL_ID) setId(_sUpperMenuItemId, upId);
    return fetch(q, false, mask(_sAppName, _sUpperMenuItemId), iTab(_sItemSequenceNumber));
}


int cMenuItem::delByAppName(QSqlQuery &q, const QString &__n, bool __pat) const
{
    QString sql = "DELETE FROM ";
    sql += tableName() + " WHERE " + dQuoted(_sAppName) + (__pat ? " LIKE " : " = ") + quoted(__n);
    if (!q.exec(sql)) SQLPREPERR(q, sql);
    int n = q.numRowsAffected();
    PDEB(VVERBOSE) << "delByAppName SQL : " << sql << " removed " << n << " records." << endl;
    return  n;
}
