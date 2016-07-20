#include "guidata.h"


int tableShapeType(const QString& n, enum eEx __ex)
{
    if (0 == n.compare(_sSimple,  Qt::CaseInsensitive)) return TS_SIMPLE;
    if (0 == n.compare(_sTree,    Qt::CaseInsensitive)) return TS_TREE;
    if (0 == n.compare(_sOwner,   Qt::CaseInsensitive)) return TS_OWNER;
    if (0 == n.compare(_sBare,    Qt::CaseInsensitive)) return TS_BARE;
    if (0 == n.compare(_sChild,   Qt::CaseInsensitive)) return TS_CHILD;
    if (0 == n.compare(_sLink,    Qt::CaseInsensitive)) return TS_LINK;
    if (0 == n.compare(_sDialog,  Qt::CaseInsensitive)) return TS_DIALOG;
    if (0 == n.compare(_sTable,   Qt::CaseInsensitive)) return TS_TABLE;
    if (0 == n.compare(_sMember,  Qt::CaseInsensitive)) return TS_MEMBER;
    if (0 == n.compare(_sGroup,   Qt::CaseInsensitive)) return TS_GROUP;
    if (0 == n.compare(_sReadOnly,Qt::CaseInsensitive)) return TS_READ_ONLY;
    if (0 == n.compare(_sIGroup,  Qt::CaseInsensitive)) return TS_IGROUP;
    if (0 == n.compare(_sNGroup,  Qt::CaseInsensitive)) return TS_NGROUP;
    if (0 == n.compare(_sIMember, Qt::CaseInsensitive)) return TS_IMEMBER;
    if (0 == n.compare(_sNMember, Qt::CaseInsensitive)) return TS_NMEMBER;
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return TS_UNKNOWN;
}

const QString& tableShapeType(int e, enum eEx __ex)
{
    switch(e) {
    case TS_SIMPLE:     return _sSimple;
    case TS_TREE:       return _sTree;
    case TS_BARE:       return _sBare;
    case TS_OWNER:      return _sOwner;
    case TS_CHILD:      return _sChild;
    case TS_LINK:       return _sLink;
    case TS_DIALOG:     return _sDialog;
    case TS_TABLE:      return _sTable;
    case TS_MEMBER:     return _sMember;
    case TS_GROUP:      return _sGroup;
    case TS_READ_ONLY:  return _sReadOnly;
      // TS_INVALID_PAD
    case TS_IGROUP:     return _sIGroup;
    case TS_NGROUP:     return _sNGroup;
    case TS_IMEMBER:    return _sIMember;
    case TS_NMEMBER:    return _sNMember;
    default:            break;
    }
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int tableInheritType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sNo,        Qt::CaseInsensitive)) return TIT_NO;
    if (0 == n.compare(_sOnly,      Qt::CaseInsensitive)) return TIT_ONLY;
    if (0 == n.compare(_sOn,        Qt::CaseInsensitive)) return TIT_ON;
    if (0 == n.compare(_sAll,       Qt::CaseInsensitive)) return TIT_ALL;
    if (0 == n.compare(_sReverse,   Qt::CaseInsensitive)) return TIT_REVERSE;
    if (0 == n.compare(_sListed,    Qt::CaseInsensitive)) return TIT_LISTED;
    if (0 == n.compare(_sListedRev, Qt::CaseInsensitive)) return TIT_LISTED_REV;
    if (0 == n.compare(_sListedAll, Qt::CaseInsensitive)) return TIT_LISTED_ALL;
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return TIT_UNKNOWN;
}

const QString& tableInheritType(int e, eEx __ex)
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
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int orderType(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sNo,   Qt::CaseInsensitive)) return OT_NO;
    if (0 == n.compare(_sAsc,  Qt::CaseInsensitive)) return OT_ASC;
    if (0 == n.compare(_sDesc, Qt::CaseInsensitive)) return OT_DESC;
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return OT_UNKNOWN;
}

const QString& orderType(int e, eEx __ex)
{
    switch(e) {
    case OT_NO:         return _sNo;
    case OT_ASC:        return _sAsc;
    case OT_DESC:       return _sDesc;
    default:            break;
    }
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int fieldFlag(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sTableHide, Qt::CaseInsensitive)) return FF_TABLE_HIDE;
    if (0 == n.compare(_sDialogHide,Qt::CaseInsensitive)) return FF_DIALOG_HIDE;
    if (0 == n.compare(_sAutoSet,   Qt::CaseInsensitive)) return FF_AUTO_SET;
    if (0 == n.compare(_sReadOnly,  Qt::CaseInsensitive)) return FF_READ_ONLY;
    if (0 == n.compare(_sPasswd,    Qt::CaseInsensitive)) return FF_PASSWD;
    if (0 == n.compare(_sHuge,      Qt::CaseInsensitive)) return FF_HUGE;
    if (0 == n.compare(_sBatchEdit, Qt::CaseInsensitive)) return FF_BATCH_EDIT;
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return FF_UNKNOWN;
}

const QString& fieldFlag(int e, eEx __ex)
{
    switch(e) {
    case FF_TABLE_HIDE: return _sTableHide;
    case FF_DIALOG_HIDE:return _sDialogHide;
    case FF_AUTO_SET:   return _sAutoSet;
    case FF_READ_ONLY:  return _sReadOnly;
    case FF_PASSWD:     return _sPasswd;
    case FF_HUGE:       return _sHuge;
    case FF_BATCH_EDIT: return _sBatchEdit;
    default:            break;
    }
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}



int filterType(const QString& n, eEx __ex)
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
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return FT_UNKNOWN;
}

const QString& filterType(int e, eEx __ex)
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
    if (__ex) EXCEPTION(EENUMVAL, e);
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

bool cTableShape::insert(QSqlQuery &__q, eEx __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (shapeFields.count()) {
        // if (getBool(_sIsReadOnly)) shapeFields.sets(_sIsReadOnly, QVariant(true));
        int i = shapeFields.insert(__q, __ex);
        return i == shapeFields.count();
    }
    return true;
}
bool cTableShape::rewrite(QSqlQuery &__q, eEx __ex)
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
        _fields[_ixTableShapeType] = set2lst(tableShapeType, __t, EX_IGNORE);
        _stat |= ES_MODIFY | ES_INVALID;
        // toEnd(__i);          // Feltételezzük, hogy most nem kell
        // fieldModified(__i);  // Feltételezzük, hogy most nem kell
        return *this;
    }
    set(_ixTableShapeType, __t);
    return *this;
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
/// Ha talál olyan mezőt, ami távoli kulcs, és 'FT_OWNER'tulajdonságú, akkor a tábla megjelenítés típushoz hozzáadja az TS_CHILD jelzőt,
///  és a mezőt rejtetté teszi a táblázatos megjelenítésnél.
/// Ha _disable_tree értéke hamis, valamint a távoli kulcs a saját táblára mutat és tulajdonságot jelöl,
/// akkor a TS_SIMPLE tíoust lecseréli TS_TREE típusra, és a mezőt a táblázatos megjelenítésnél rejtetté teszi.
/// A maző rejtett lessz, ha a deleted mező, vagy elsődleges kulcs és autoincrement (ID) mező
/// A flag mező (ha van) szintén rejtett.
/// Kapcsoló tábla esetén :
bool cTableShape::setDefaults(QSqlQuery& q, bool _disable_tree)
{
    bool r = true;
    QString n = getName();
    setName(_sTableTitle, n);
    setName(_sDialogTitle, n);
    setName(_sDialogTabTitle, n);
    int type = ENUM2SET(TS_SIMPLE);
    const cRecStaticDescr& rDescr = *cRecStaticDescr::get(getName(_sTableName), getName(_sSchemaName));
    shapeFields.clear();
    int ixFieldFlags = cTableShapeField().toIndex(_sFieldFlags);
    for (int i = 0; rDescr.isIndex(i); ++i) {
        const cColStaticDescr& cDescr = rDescr.colDescr(i);
        cTableShapeField fm;
        fm.setName(cDescr); // mező név a rekordban
        fm.setName(_sTableShapeFieldNote, cDescr);
        fm.setName(_sTableTitle, cDescr);
        fm.setName(_sDialogTitle, cDescr);
        fm.setId(_sFieldSequenceNumber, (i + 1) * 10);  // mezők megjelenítési sorrendje
        bool ro = !cDescr.isUpdatable;
        bool hide = false;
        if (i == 0 && rDescr.idIndex(EX_IGNORE) == 0 && rDescr.autoIncrement().at(0)) {
            hide = true;
            ro   = true;
        }
        else if (rDescr.deletedIndex(EX_IGNORE) == i) {
            hide = true;
            ro   = true;
        }
        else if (cDescr.fKeyType == cColStaticDescr::FT_OWNER) {
            ro   = true;
            fm.enum2setOn(ixFieldFlags, FF_TABLE_HIDE);
            type = (type & ~enum2set(TS_SIMPLE)) | enum2set(TS_CHILD);
        }
        // Önmagára mutató fkey?
        else if (cDescr.fKeyType     == cColStaticDescr::FT_PROPERTY
         &&      rDescr.tableName()  == cDescr.fKeyTable
         &&      rDescr.schemaName() == cDescr.fKeySchema) {
            if (!_disable_tree) {
                type = (type & ~enum2set(TS_SIMPLE)) | enum2set(TS_TREE);
                fm.enum2setOn(ixFieldFlags, FF_TABLE_HIDE);
            }
        }
        else if (rDescr.flagIndex(EX_IGNORE) == i) {
            hide = true;
        }
        if (ro)   fm.enum2setOn(_sFieldFlags, FF_READ_ONLY);
        if (hide) fm.enum2setOn(_sFieldFlags, FF_TABLE_HIDE, FF_DIALOG_HIDE);
        shapeFields << fm;
    }
    // Kapcsoló tábla ?
    (void)q;
/*    if (rDescr.cols() >= 3                                            // Legalább 3 mező
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
    }*/
    if (!rDescr.isUpdatable()) type |= ENUM2SET(TS_READ_ONLY);
    setId(_ixTableShapeType, type);
    return r;
}

void cTableShape::setTitle(const QStringList& _tt)
{
    QString n = getName();
    if (_tt.size() > 0) {
        if (_tt.at(0).size() > 0) {
            if (_tt.at(0) != _sAt) n = _tt.at(0);
            setName(_sTableTitle, n);
        }

        if (_tt.size() > 1) {
            if (_tt.at(1).size() > 0) {
                if (_tt.at(1) != _sAt) n = _tt.at(1);
                setName(_sDialogTitle, n);
            }

            if (_tt.size() > 2) {
                if (_tt.at(2).size() > 0) {
                    if (_tt.at(2) != _sAt) n = _tt.at(2);
                    setName(_sDialogTabTitle, n);
                }
                if (_tt.size() > 3) {
                    if (_tt.at(3).size() > 0) {
                        if (_tt.at(3) != _sAt) n = _tt.at(3);
                        setName(_sMemberTitle, n);
                    }
                    if (_tt.size() > 4 && _tt.at(4).size() > 0) {
                        if (_tt.at(4) != _sAt) n = _tt.at(4);
                        setName(_sNotMemberTitle, n);
                    }
                }
            }
        }
    }
}

bool cTableShape::fset(const QString& _fn, const QString& _fpn, const QVariant& _v, eEx __ex)
{
    int i = shapeFields.indexOf(_fn);
    if (i < 0) {
        if (__ex) EXCEPTION(EFOUND, -1, emFieldNotFound(_fn));
        DERR() << emFieldNotFound(_fn) << endl;
        return false;
    }
    cTableShapeField& f = *(shapeFields[i]);
    if (!__ex && !f.isIndex(_fpn)) return false;
    f.set(_fpn, _v);
    return true;
}

bool cTableShape::fsets(const QStringList& _fnl, const QString& _fpn, const QVariant& _v, eEx __ex)
{
    bool r = true;
    foreach (QString fn, _fnl) r = fset(fn, _fpn, _v, __ex) && r;
    return r;
}

bool cTableShape::addFilter(const QString& _fn, const QString& _t, const QString& _d, eEx __ex)
{
    int i = shapeFields.indexOf(_fn);
    if (i < 0) {
        if (__ex) EXCEPTION(EFOUND, -1, emFieldNotFound(_fn));
        DERR() << emFieldNotFound(_fn) << endl;
        return false;
    }
    cTableShapeField& f = *(shapeFields[i]);
    return f.addFilter(_t, _d, __ex);
}

bool cTableShape::addFilter(const QStringList& _fnl, const QString& _t, const QString& _d, eEx __ex)
{
    bool r = true;
    foreach (QString fn, _fnl) r = addFilter(fn, _t, _d, __ex) && r;
    return r;
}

bool cTableShape::addFilter(const QStringList& _fnl, const QStringList& _ftl, eEx __ex)
{
    bool r = true;
    foreach (QString fn, _fnl) {
        foreach (QString ft, _ftl) {
            r = addFilter(fn, ft, _sNul, __ex) && r;
        }
    }
    return r;
}

bool cTableShape::setAllOrders(QStringList& _ord, eEx __ex)
{
    if (shapeFields.isEmpty()) {
        if (__ex) EXCEPTION(EDATA, -1, emFildsIsEmpty());
        DERR() << emFildsIsEmpty() << endl;
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
        deford = orderType(_ord[0]);
    }
    tTableShapeFields::Iterator i, e = shapeFields.end();
    int seq = 0;
    for (i = shapeFields.begin(); e != i; ++i) {
        cTableShapeField& f = **i;
        f.set(_sOrdTypes, _ord);
        if (s == enum2set(OT_NO)) continue;
        f.setId(_sOrdInitType, deford);
        seq += 10;
        f.setId(_sOrdInitSequenceNumber, seq);
    }
    return true;
}

bool cTableShape::setOrders(const QStringList& _fnl, QStringList& _ord, eEx __ex)
{
    if (shapeFields.isEmpty()) {
        if (__ex) EXCEPTION(EDATA, -1, emFildsIsEmpty());
        DERR() << emFildsIsEmpty() << endl;
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
            if (__ex) EXCEPTION(EFOUND, -1, emFieldNotFound(fn));
            DERR() << emFieldNotFound(fn) << endl;
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

bool cTableShape::setFieldSeq(const QStringList& _fnl, int last, eEx __ex)
{
    if (shapeFields.isEmpty()) {
        if (__ex) EXCEPTION(EDATA, -1, emFildsIsEmpty());
        DERR() << emFildsIsEmpty() << endl;
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
            if (__ex) EXCEPTION(EFOUND, -1, emFieldNotFound(fn));
            DERR() << emFieldNotFound(fn) << endl;
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.set(_sFieldSequenceNumber, seq);
        seq += step;
    }
    return true;
}

bool cTableShape::setOrdSeq(const QStringList& _fnl, int last, eEx __ex)
{
    if (shapeFields.isEmpty()) {
        if (__ex) EXCEPTION(EDATA, -1, emFildsIsEmpty());
        DERR() << emFildsIsEmpty()<< endl;
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
            if (__ex) EXCEPTION(EFOUND, -1, emFieldNotFound(fn));
            DERR() << emFieldNotFound(fn) << endl;
            return false;
        }
        cTableShapeField& f = *(shapeFields[i]);
        f.set(ixOrdInitSequenceNumber, seq);
        seq += step;
    }
    return true;
}

cTableShapeField *cTableShape::addField(const QString& _name, const QString& _note, eEx __ex)
{
    if (0 <= shapeFields.indexOf(_name)) {
        if (__ex) EXCEPTION(EUNIQUE, -1, _name);
        return NULL;
    }
    cTableShapeField *p = new cTableShapeField();
    p->setName(_name);
    p->setName(_sTableTitle, _name);
    p->setName(_sDialogTitle, _name);
    if (_note.size()) p->setName(_sTableShapeFieldNote, _note);
    qlonglong seq = 0;
    QListIterator<cTableShapeField *> i(shapeFields);
    while (i.hasNext()) {
        seq = qMax(seq, i.next()->getId(_sFieldSequenceNumber));
    }
    p->setId(_sFieldSequenceNumber, seq + 10);
    shapeFields << p;
    return p;
}

void cTableShape::addRightShape(QStringList& _snl)
{
    QSqlQuery q = getQuery();
    QVariantList list;
    int ix = toIndex(_sRightShapeIds);
    if (!isNull(ix)) list = get(ix).toList();
    foreach (QString sn, _snl) {
        qlonglong id = getIdByName(q, sn);
        list << QVariant(id);
    }
    set(ix, QVariant::fromValue(list));
}

QString cTableShape::emFildsIsEmpty()
{
    return trUtf8("A shape Fields konténer üres.");
}

QString cTableShape::emFieldNotFound(const QString& __f)
{
    return trUtf8("A %1 nevű shape objektumban nincs %2 nevű mező objektum").arg(getName(), __f);
}


/* ------------------------------ cTableShapeField ------------------------------ */

int cTableShapeField::_ixTableShapeId = NULL_IX;
int cTableShapeField::_ixFeatures = NULL_IX;

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

const cRecStaticDescr&  cTableShapeField::descr() const
{
    if (initPDescr<cTableShapeField>(_sTableShapeFields)) {
        _ixFeatures = _descr_cTableShapeField().toIndex(_sFeatures);
        _ixTableShapeId = _descr_cTableShapeField().toIndex(_sTableShapeId);
        CHKENUM(_sOrdTypes, orderType);
        CHKENUM(_sFieldFlags, fieldFlag);
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

bool cTableShapeField::insert(QSqlQuery &__q, eEx __ex)
{
    if (!cRecord::insert(__q, __ex)) return false;
    if (shapeFilters.count()) {
        int i = shapeFilters.insert(__q, __ex);
        return i == shapeFilters.count();
    }
    return true;
}

bool cTableShapeField::rewrite(QSqlQuery &__q, eEx __ex)
{
    bool r = cRecord::rewrite(__q, __ex);   // Kiírjuk magát a rekordot
    if (!r) return false;   // Ha nem sikerült, nincs több dolgunk :(
    shapeFilters.setsOwnerId();
    shapeFilters.removeByOwn(__q, __ex);
    r = shapeFilters.insert(__q, __ex);
    return r;
}

void cTableShapeField::setTitle(const QStringList& _tt)
{
    QString n = getName();
    if (_tt.size() > 0 && _tt.at(0).size() > 0) {
        if (_tt.at(0) != _sAt) n = _tt.at(0);
        setName(_sTableTitle, n);

        if (_tt.size() > 1 && _tt.at(1).size() > 0) {
            if (_tt.at(1) != _sAt) n = _tt.at(1);
            setName(_sDialogTitle, n);
        }
    }
}

int cTableShapeField::fetchFilters(QSqlQuery& q)
{
    return shapeFilters.fetch(q, getId());
}

bool cTableShapeField::addFilter(const QString& _t, const QString& _d, eEx __ex)
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


bool cTableShapeField::fetchByNames(QSqlQuery& q, const QString& tsn, const QString& fn, eEx __ex)
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
    o.fetchByNames(q, tsn, fn, EX_IGNORE);
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

/*
cMenuItem *cMenuItem::pNullItem = NULL;

const cMenuItem& cMenuItem::nullItem()
{
    if (pNullItem == NULL) pNullItem = new cMenuItem();
    return *pNullItem;
}
*/
