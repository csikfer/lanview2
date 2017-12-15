
#include "guidata.h"
#include "lv2user.h"

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
    if (0 == n.compare(_sReadOnly,  Qt::CaseInsensitive)) return FF_READ_ONLY;
    if (0 == n.compare(_sPasswd,    Qt::CaseInsensitive)) return FF_PASSWD;
    if (0 == n.compare(_sHuge,      Qt::CaseInsensitive)) return FF_HUGE;
    if (0 == n.compare(_sBatchEdit, Qt::CaseInsensitive)) return FF_BATCH_EDIT;
    if (0 == n.compare(_sBgColor,   Qt::CaseInsensitive)) return FF_BG_COLOR;
    if (0 == n.compare(_sFgColor,   Qt::CaseInsensitive)) return FF_FG_COLOR;
    if (0 == n.compare(_sFont,      Qt::CaseInsensitive)) return FF_FONT;
    if (0 == n.compare(_sToolTip,   Qt::CaseInsensitive)) return FF_TOOL_TIP;
    if (0 == n.compare(_sHTML,      Qt::CaseInsensitive)) return FF_HTML;
    if (__ex) EXCEPTION(EENUMVAL, -1, n);
    return FF_UNKNOWN;
}

const QString& fieldFlag(int e, eEx __ex)
{
    switch(e) {
    case FF_TABLE_HIDE: return _sTableHide;
    case FF_DIALOG_HIDE:return _sDialogHide;
    case FF_READ_ONLY:  return _sReadOnly;
    case FF_PASSWD:     return _sPasswd;
    case FF_HUGE:       return _sHuge;
    case FF_BATCH_EDIT: return _sBatchEdit;
    case FF_BG_COLOR:   return _sBgColor;
    case FF_FG_COLOR:   return _sFgColor;
    case FF_FONT:       return _sFont;
    case FF_TOOL_TIP:   return _sToolTip;
    case FF_HTML:       return _sHTML;
    default:            break;
    }
    if (__ex) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int dataCharacter(const QString& n, eEx __ex)
{
    if (0 == n.compare(_sHead,   Qt::CaseInsensitive)) return DC_HEAD;
    if (0 == n.compare(_sData,   Qt::CaseInsensitive)) return DC_DATA;
    if (0 == n.compare(_sId,     Qt::CaseInsensitive)) return DC_ID;
    if (0 == n.compare(_sName,   Qt::CaseInsensitive)) return DC_NAME;
    if (0 == n.compare(_sPrimary,Qt::CaseInsensitive)) return DC_PRIMARY;
    if (0 == n.compare(_sKey,    Qt::CaseInsensitive)) return DC_KEY;
    if (0 == n.compare(_sFname,  Qt::CaseInsensitive)) return DC_FNAME;
    if (0 == n.compare(_sDerived,Qt::CaseInsensitive)) return DC_DERIVED;
    if (0 == n.compare(_sTree,   Qt::CaseInsensitive)) return DC_TREE;
    if (0 == n.compare(_sForeign,Qt::CaseInsensitive)) return DC_FOREIGN;
    if (0 == n.compare(_sNull,   Qt::CaseInsensitive)) return DC_NULL;
    if (0 == n.compare(_sDefault,Qt::CaseInsensitive)) return DC_DEFAULT;
    if (0 == n.compare(_sAuto,   Qt::CaseInsensitive)) return DC_AUTO;
    if (0 == n.compare(_sInfo,   Qt::CaseInsensitive)) return DC_INFO;
    if (0 == n.compare(_sWarning,Qt::CaseInsensitive)) return DC_WARNING;
    if (0 == n.compare(_sError,  Qt::CaseInsensitive)) return DC_ERROR;
    if (0 == n.compare(_sNotPermit,Qt::CaseInsensitive))return DC_NOT_PERMIT;
    if (0 == n.compare(_sHaveNo, Qt::CaseInsensitive)) return DC_HAVE_NO;
    if (0 == n.compare(_sText,   Qt::CaseInsensitive)) return DC_TEXT;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, n);
    return DC_INVALID;
}

const QString& dataCharacter(int e, eEx __ex)
{
    switch (e) {
    case DC_HEAD:       return _sHead;
    case DC_DATA:       return _sData;
    case DC_ID:         return _sId;
    case DC_NAME:       return _sName;
    case DC_PRIMARY:    return _sPrimary;
    case DC_KEY:        return _sKey;
    case DC_FNAME:      return _sFname;
    case DC_DERIVED:    return _sDerived;
    case DC_TREE:       return _sTree;
    case DC_FOREIGN:    return _sForeign;
    case DC_NULL:       return _sNull;
    case DC_DEFAULT:    return _sDefault;
    case DC_AUTO:       return _sAuto;
    case DC_INFO:       return _sInfo;
    case DC_WARNING:    return _sWarning;
    case DC_ERROR:      return _sError;
    case DC_NOT_PERMIT: return _sNotPermit;
    case DC_HAVE_NO:    return _sHaveNo;
    case DC_TEXT:       return _sText;
    default:            break;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

QString dcViewShort(int id)
{
    if (!lanView::dbIsOpen()) {  // Nincs adatbázis
        return dataCharacter(id);
    }
    return cEnumVal::viewShort(_sDatacharacter, id, dataCharacter(id));
}


/* ------------------------------ cTableShape ------------------------------ */

QMap<QString, QString>   cTableShape::fieldDialogTitleMap;

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

int  cTableShape::_ixFeatures       = NULL_IX;
int  cTableShape::_ixTableShapeType = NULL_IX;
const cRecStaticDescr&  cTableShape::descr() const
{
    if (initPDescr<cTableShape>(_sTableShapes)) {
        _ixFeatures       = _descr_cTableShape().toIndex(_sFeatures);
        _ixTableShapeType = _descr_cTableShape().toIndex(_sTableShapeType);
        CHKENUM(_ixTableShapeType,  tableShapeType);
        CHKENUM(_sTableInheritType, tableInheritType);
    }
    return *_pRecordDescr;
}
CRECDEFNC(cTableShape)
cTableShape& cTableShape::clone(const cRecord& __o)
{
    clear();
    copy(__o);
    shapeFields.clear();
    foreach (cTableShapeField *p, (const QList<cTableShapeField *>)__o.creconvert<cTableShape>()->shapeFields) {
        shapeFields << new cTableShapeField(*p);
    }
    return *this;
}

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
        _stat |= ES_MODIFY | ES_DEFECTIVE;
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
    foreach (cTableShapeField *p, (QList<cTableShapeField *>)shapeFields) {
        p->fetchText(q);
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
    /* !!!
    setName(_sTableTitle, n);
    setName(_sDialogTitle, n);
    setName(_sDialogTabTitle, n);
    */
    int type = ENUM2SET(TS_SIMPLE);
    const cRecStaticDescr& rDescr = *cRecStaticDescr::get(getName(_sTableName), getName(_sSchemaName));
    shapeFields.clear();
    int ixFieldFlags = cTableShapeField().toIndex(_sFieldFlags);
    for (int i = 0; rDescr.isIndex(i); ++i) {
        const cColStaticDescr& cDescr = rDescr.colDescr(i);
        cTableShapeField fm;
        fm.setName(cDescr); // mező név a rekordban
        fm.setName(_sTableShapeFieldNote, cDescr);
        // fm.setName(_sTableTitle, cDescr);
        // fm.setName(_sDialogTitle, cDescr);
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
    /* !!!
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
    */
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
    /* !!!
    p->setName(_sTableTitle, _name);
    p->setName(_sDialogTitle, _name);
    */
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

const QString &cTableShape::getFieldDialogTitle(QSqlQuery& q, const QString& _sn, const QString& _fn, eEx __ex)
{
    QString key = mCat(_sn, _fn);
    QMap<QString, QString>::const_iterator i = fieldDialogTitleMap.find(key);
    if (i == fieldDialogTitleMap.constEnd()) {
        cTableShape ts;
        QString r = _fn;
        if (ts.fetchByName(q, _sn)) {
            cTableShapeField fs;
            fs.setId(_sTableShapeId, ts.getId());
            fs.setName(_fn);
            if (fs.completion(q) == 1) {
                fs.fetchText(q);
                r = fs.getText(cTableShapeField::LTX_DIALOG_TITLE, _fn);
            }
            else {
                if (__ex > EX_ERROR) EXCEPTION(EFOUND, 0, key);
            }
        }
        else {
            if (__ex > EX_ERROR) EXCEPTION(EFOUND, 0, key);
        }
        i = fieldDialogTitleMap.insert(key, r);
    }
    return *i;
}

/* ------------------------------ cTableShapeField ------------------------------ */

int cTableShapeField::_ixTableShapeId = NULL_IX;
int cTableShapeField::_ixFeatures = NULL_IX;

cTableShapeField::cTableShapeField() : cRecord()
{
    _pFeatures = NULL;
    _set(cTableShapeField::descr());
}

cTableShapeField::cTableShapeField(const cTableShapeField &__o) : cRecord()
{
    _pFeatures = NULL;
    _set(cTableShapeField::descr());
    _cp(__o);
    copy(__o);
}

cTableShapeField::cTableShapeField(QSqlQuery& q) : cRecord()
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
        STFIELDIX(cTableShapeField, Features);
        STFIELDIX(cTableShapeField, TableShapeId);
        CHKENUM(_sOrdTypes,   orderType);
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
    if (i == _ixFeatures) {
        pDelete( _pFeatures);
        return true;
    }
    return false;
}

void cTableShapeField::clearToEnd()
{
    pDelete(_pFeatures);
}

void cTableShapeField::setTitle(const QStringList& _tt)
{
    /* !!!
    QString n = getName();
    if (_tt.size() > 0 && _tt.at(0).size() > 0) {
        if (_tt.at(0) != _sAt) n = _tt.at(0);
        setName(_sTableTitle, n);

        if (_tt.size() > 1 && _tt.at(1).size() > 0) {
            if (_tt.at(1) != _sAt) n = _tt.at(1);
            setName(_sDialogTitle, n);
        }
    }
    */
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


/* ------------------------------ cEnumVal ------------------------------ */
CRECCNTR(cEnumVal) CRECDEFD(cEnumVal)

int cEnumVal::_ixBgColor  = NULL_IX;
int cEnumVal::_ixFgColor  = NULL_IX;
int cEnumVal::_ixTypeName = NULL_IX;
int cEnumVal::_ixValName  = NULL_IX;
int cEnumVal::_ixFontFamily=NULL_IX;
int cEnumVal::_ixFontAttr = NULL_IX;
QList<cEnumVal *> cEnumVal::enumVals;
QMap<QString, QVector<cEnumVal *> > cEnumVal::mapValues;
cEnumVal *cEnumVal::pNull = NULL;
QStringList     cEnumVal::forcedList;

const cRecStaticDescr&  cEnumVal::descr() const
{
    if (initPDescr<cEnumVal>(_sEnumVals, _sPublic)) {
        _ixTypeName = _descr_cEnumVal().toIndex(_sEnumTypeName);
        _ixValName  = _descr_cEnumVal().toIndex(_sEnumValName);
        _ixBgColor  = _descr_cEnumVal().toIndex(_sBgColor);
        _ixFgColor  = _descr_cEnumVal().toIndex(_sFgColor);
        _ixFontFamily=_descr_cEnumVal().toIndex(_sFontFamily);
        _ixFontAttr = _descr_cEnumVal().toIndex(_sFontAttr);
        QSqlQuery q = getQuery();
        textName2ix(q, _sNul);
    }
    return *_pRecordDescr;
}

cEnumVal::cEnumVal(const QString _tn, const QString _en)
    : cRecord()
{
    _set(cEnumVal::descr());
    _fields[_ixTypeName] = _tn;
    _fields[_ixValName]  = _en;
    pTextList = new QStringList();
    *pTextList << (_en.isEmpty() ? _tn : _en);  // LTX_VIEW_LONG
    *pTextList << pTextList->first();           // LTX_VIEW_SHORT
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

int cEnumVal::toInt(eEx __ex) const
{
    if (isNull(_ixTypeName) || isNull(_ixValName)) {
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, 0, trUtf8("Is NULL"));
        return ENUM_INVALID;
    }
    QSqlQuery q = getQuery();
    const cColEnumType *t = cColEnumType::fetchOrGet(q, getName(_ixTypeName), __ex);
    if (t == NULL) return ENUM_INVALID;
    return (int)t->str2enum(getName(_ixValName));
}

int cEnumVal::textName2ix(QSqlQuery& q, const QString& _n, eEx __ex) const
{
    static const QString    sTextEnumTypeName = "enumvaltext";
    const cColEnumType *pTextEnumType = cColEnumType::fetchOrGet(q, sTextEnumTypeName);
    if (_n.isEmpty()) { // Check only
        if (LTX_NUMBER != pTextEnumType->enumValues.size()) {
            EXCEPTION(EENUMVAL, LTX_NUMBER, sTextEnumTypeName);
        }
        int n = 0;
        foreach (QString e, pTextEnumType->enumValues) {
            if (n != textName2ix(q, e)) {
                EXCEPTION(EENUMVAL, n, mCat(sTextEnumTypeName, e));
            }
            ++n;
        }
        return -1;
    }
    if (0 == _n.compare("view_long",  Qt::CaseInsensitive)) return LTX_VIEW_LONG;
    if (0 == _n.compare("view_short", Qt::CaseInsensitive)) return LTX_VIEW_SHORT;
    if (0 == _n.compare(_sToolTip,    Qt::CaseInsensitive)) return LTX_TOOL_TIP;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, mCat(sTextEnumTypeName, _n));
    return -1;
}

void cEnumVal::fetchEnumVals()
{
    if (pNull == NULL) pNull = new cEnumVal();
    else {
        EXCEPTION(EPROGFAIL);   // We do not handle re-reading well.
    }
    QBitArray   ba(1, false);   // Nem null, egyeseket nem tartalmazó maszk, minden rekord kiválasztásához
    int n = enumVals.count();   // Megtalálandó rekordok száma
    QList<cEnumVal *> oldList = enumVals;   // Eredeti lista
    mapValues.clear();
    enumVals.clear();
    int found = 0;                  // Megtalált rekordok számláló
    cEnumVal ev;                    // objektum a fetch-hez
    QString currentTypeName;        // Ha típust váltun, észre kell vennünk
    const cColEnumType *pE = NULL;  // Az enumerációs típus leírója
    bool          isBool = false;   // Nem csak valós enumok lesznek!!
    int e;                          // Enumerációs érték (int) (a -1 a típus 'indexe', a név ekkor üres)
    QVector<cEnumVal *> *pActV = NULL;

    QSqlQuery q  = getQuery();
    QSqlQuery q2 = getQuery();
    // All enum records, sorted by type name
    if (ev.fetch(q, false, ba, ev.iTab(_sEnumTypeName))) do {
        QString typeName = ev.getName(ev.ixTypeName());
        QString val      = ev.getName(ev.ixValName());
        if (currentTypeName != typeName) {  // This record belongs to another type
            if (!currentTypeName.isEmpty()) {   // Old type, closure
                QVector<cEnumVal *>& v = mapValues[currentTypeName];
                for (int i = 0; i < v.size(); ++i) {
                    cEnumVal *& o = v[i];
                    if (o == NULL) {
                        if (i == 0) o = new cEnumVal(currentTypeName, QString());
                        else {
                            if (isBool) {
                                switch (i) {
                                case 1: o = new cEnumVal(currentTypeName, _sFalse); break;
                                case 2: o = new cEnumVal(currentTypeName, _sTrue);  break;
                                default:    EXCEPTION(EPROGFAIL);   // Impossible.
                                }
                            }
                            else {
                                QString e = pE->enum2str(i -1);
                                o = new cEnumVal(currentTypeName, e);
                            }
                        }
                        enumVals << o;
                    }
                }
            }
            // New type, preparation
            pE = cColEnumType::fetchOrGet(q2, typeName, EX_IGNORE);
            isBool = (pE == NULL);
            if (isBool) {   // Is not ENUM!
                if (1 != typeName.contains(QChar('.'))) {   // Name convention control, bool?
                    APPMEMO(q2, trUtf8("Invalid 'enum_vals' objet : ") + ev.toString(), RS_WARNING);
                    currentTypeName.clear();
                    continue;
                }
                mapValues[typeName].fill((cEnumVal*)NULL, 3);
            }
            else {
                mapValues[typeName].fill((cEnumVal*)NULL, pE->enumValues.size() +1);
            }
            pActV = &mapValues[typeName];
            currentTypeName = typeName;
        }
        else {
            if (!isBool && pE == NULL) EXCEPTION(EPROGFAIL);
        }
        e = NULL_IX;    // No O.K.
        if (val.isEmpty())  e = -1; // Type index
        else {
            if (isBool) {
               if      (val == _sFalse) e =  0;
               else if (val == _sTrue)  e =  1;
            }
            else {
                e = pE->enumValues.indexOf(val);
                if (e < 0) e = NULL_IX;
            }
        }
        if (e == NULL_IX || !isContIx(*pActV, (e +1))) {
            QString em = trUtf8("Wrong 'enum_vals' objekt : %1 . Delete record.") + ev.toString();
            APPMEMO(q2, em, RS_WARNING);
            ev.remove(q2);
            continue;   // Drop
        }
        cEnumVal *p;
        foreach (p, oldList) {
            if (p->getId() == ev.getId()) {
                found++;
                if (1 != oldList.removeAll(p)) EXCEPTION(EPROGFAIL);
                delete p;
            }
        }
        p = ev.dup()->reconvert<cEnumVal>();
        (*pActV)[e + 1] = p;
        enumVals << p;
    } while (ev.next(q));
    foreach (QString ft, forcedList) {
        int nn = enumForce(q, ft);
        if (nn == 0) continue;
        cEnumVal *p;
        foreach (p, oldList) {
            if (p->getName(_sEnumTypeName) == ft) {
                found++;
                --nn;
                if (1 != oldList.removeAll(p)) EXCEPTION(EPROGFAIL);
                delete p;
            }
        }
        if (nn != 0) EXCEPTION(EDATA, nn, ft);
    }
    if (n < found) EXCEPTION(EPROGFAIL);    // Too many results. Impossible.
    if (n > found) {
        QString em = trUtf8("Not found enum_values record(s) : \n");
        foreach (cEnumVal *p, oldList) {
            em += p->identifying() + '\n';
            delete p;
        }
        em.chop(1);
        EXCEPTION(EDATA, n - found, em);
    }
}

const cEnumVal& cEnumVal::enumVal(const QString &_tn, int e, eEx __ex)
{
    if (pNull == NULL) {
        fetchEnumVals();
    }
    cEnumVal *p = NULL;
    QMap<QString, QVector<cEnumVal *> >::const_iterator i = mapValues.find(_tn);
    if (mapValues.end() != i) {
        const QVector<cEnumVal *>& v = i.value();
        int ix = e + 1;
        if (isContIx(v, ix)) {
            p = v[ix];
        }
    }
    if (p == NULL) {
        if (EX_WARNING <= __ex) EXCEPTION(EFOUND, e, _tn);
        p = pNull;
    }
    return *p;
}

int cEnumVal::enumForce(QSqlQuery& q, const QString& _tn)
{
    if (pNull == NULL) {
        fetchEnumVals();
    }
    QMap<QString, QVector<cEnumVal *> >::const_iterator i = mapValues.find(_tn);
    if (mapValues.end() != i) {
        if (!forcedList.contains(_tn)) forcedList << _tn;
        const cColEnumType *pet = cColEnumType::fetchOrGet(q, _tn);
        QVector<cEnumVal *>& v = mapValues[_tn];
        cEnumVal *p = new cEnumVal(_tn, QString());
        enumVals << p;
        v        << p;
        foreach (QString e, pet->enumValues) {
            p = new cEnumVal(_tn, e);
            enumVals << p;
            v        << p;
        }
        return pet->enumValues.size() +1;
    }
    return 0;
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
        STFIELDIX(cMenuItem, Features);
    }
    return *_pRecordDescr;
}

void cMenuItem::toEnd()
{
    toEnd(_ixFeatures);
}

bool cMenuItem::toEnd(int i)
{
    if (i == _ixFeatures) {
        pDelete(_pFeatures);
        return true;
    }
    return false;
}

void cMenuItem::clearToEnd()
{
    pDelete(_pFeatures);
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

