#include "lv2models.h"
#include <algorithm>

QVector<int> mil2rowsAsc(const QModelIndexList& mil)
{
    QVector<int> rows;
    foreach (QModelIndex mi, mil) {
        rows << mi.row();
    }
    std::sort(rows.begin(), rows.end());
    int i = rows.size() -1;
    while (i > 0) {
        if (rows[i] == rows[i -1]) rows.remove(i);
        --i;
    }
    return rows;
}

QVector<int> mil2rowsDesc(const QModelIndexList& mil)
{
    QVector<int> rows;
    foreach (QModelIndex mi, mil) {
        rows << mi.row();
    }
    std::sort(rows.begin(), rows.end());
    int i = rows.size() -1;
    QVector<int> rrows;
    while (i > 0) {
        if (rows[i] == rows[i -1]) rows.remove(i);
        else rrows << rows[i];
        --i;
    }
    rrows << rows[0];
    return rrows;
}

/* **************************************** cStringListModel ****************************************  */

cStringListModel::cStringListModel(QObject *__par)
    : QAbstractListModel(__par)
    , _stringList()
{
    _rowNumbers = false;
}

QVariant cStringListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && _rowNumbers && orientation == Qt::Vertical) {
        return QVariant(section + 1);
    }
    return QVariant();
}

int cStringListModel::rowCount(const QModelIndex & /*parent*/) const
{
    return _stringList.size();
}
int cStringListModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 1;
}
QVariant cStringListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    int row = index.row();
    if (row >= (int)_stringList.size()) return QVariant();
    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    case Qt::DisplayRole:
        return _stringList.at(row);
    }
    return QVariant();
}

bool cStringListModel::setStringList(const QStringList& sl)
{
    beginResetModel();
    _stringList = sl;
    endResetModel();
    return !_stringList.isEmpty();
}

cStringListModel& cStringListModel::remove(QModelIndexList& mil)
{
    QVector<int> rows = mil2rowsDesc(mil);
    beginResetModel();
    int s = rows.size();
    for (int i = 0; i < s; ++i) {
        remove(rows[i]);
    }
    endResetModel();
    return *this;
}

cStringListModel& cStringListModel::up(const QModelIndexList& mil)
{
    QVector<int> rows = mil2rowsAsc(mil);
    beginResetModel();
    int s = rows.size();
    for (int i = 0; i < s; ++i) {
        int row1 = rows[i];
        if (!isContIx(_stringList, row1)) continue;
        int row2 = row1 -1;
        if (row2 < 0) row2 = _stringList.size() -1;
        _stringList.swap(row1, row2);
    }
    endResetModel();
    return *this;
}

cStringListModel& cStringListModel::down(const QModelIndexList& mil)
{
    QVector<int> rows = mil2rowsDesc(mil);
    beginResetModel();
    int s = rows.size();
    for (int i = 0; i < s; ++i) {
        int row1 = rows[i];
        if (!isContIx(_stringList, row1)) continue;
        int row2 = row1 +1;
        if (row2 >=  _stringList.size()) row2 = 0;
        _stringList.swap(row1, row2);
    }
    endResetModel();
    return *this;
}

/* **************************************** cStringListDecModel ****************************************  */


cStringListDecModel::cStringListDecModel(QObject *__par)
    : QAbstractListModel(__par)
{
    defDecoration = &cEnumVal::enumVal(_sDatacharacter, DC_DATA);
    decorations = new QList<const cEnumVal *>();
}

cStringListDecModel::~cStringListDecModel()
{
    delete decorations;
}

int cStringListDecModel::rowCount(const QModelIndex &parent) const
{
    (void)parent;
    return _stringList.size();
}

int cStringListDecModel::columnCount(const QModelIndex &parent) const
{
    (void)parent;
    return 1;
}

QVariant cStringListDecModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (!isContIx(_stringList, row)) EXCEPTION(ENOINDEX, row);
    if (!isContIx(*decorations, row)) EXCEPTION(ENOINDEX, row);
    if (role == Qt::DisplayRole) {
        return _stringList.at(row);
    }
    return enumRole(*decorations->at(row), role);
}

QVariant cStringListDecModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    (void)section;
    (void)orientation;
    (void)role;
    return QVariant();
}

cStringListDecModel& cStringListDecModel::setStringList(const QStringList& sl)
{
    beginResetModel();
    _stringList = sl;
    decorations->clear();
    for (int i = 0; i < _stringList.size(); ++i) *decorations << defDecoration;
    endResetModel();
    return *this;
}

cStringListDecModel &cStringListDecModel::setLists(const QStringList& sl, const QList<const cEnumVal *>& decs)
{
    beginResetModel();
    _stringList = sl;
    *decorations = decs;
    endResetModel();
    return *this;
}

cStringListDecModel &cStringListDecModel::setDecorationAt(int ix, const cEnumVal * pe)
{
    beginResetModel();
    if (!isContIx(*decorations, ix)) EXCEPTION(ENOINDEX, ix);
    (*decorations)[ix] = pe;
    endResetModel();
    return *this;
}

const cEnumVal * cStringListDecModel::getDecorationAt(int ix) const
{
    if (!isContIx(*decorations, ix)) EXCEPTION(ENOINDEX, ix);
    return (*decorations)[ix];
}

/* **************************************** cPolygonTableModel ****************************************  */

cPolygonTableModel::cPolygonTableModel(QObject *__par)
    : QAbstractTableModel(__par)
    , _polygon()
{
    _rowNumbers = true;
    _viewHeader = true;
}

int cPolygonTableModel::rowCount(const QModelIndex & /*parent*/) const
{
    return _polygon.size();
}
int cPolygonTableModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;   // X,Y
}
QVariant cPolygonTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    int row = index.row();
    int col = index.column();
    if (row >= (int)_polygon.size()) return QVariant();
    switch (role) {
    case Qt::TextAlignmentRole:
        return int(Qt::AlignRight | Qt::AlignVCenter);
    case Qt::DisplayRole:
        switch (col) {
        case 0: return QString::number(_polygon.at(row).x(), 'f', 0);
        case 1: return QString::number(_polygon.at(row).y(), 'f', 0);
        }
        break;
    }
    return QVariant();
}
QVariant cPolygonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) switch (orientation) {
    case Qt::Vertical:      return _rowNumbers ? QVariant(section + 1) : QVariant();
    case Qt::Horizontal:    return _viewHeader ? QVariant(section ? trUtf8("Y") : trUtf8("X")) : QVariant();
    }
    return QVariant();
}
bool cPolygonTableModel::setPolygon(const tPolygonF &__pol)
{
    beginResetModel();
    _polygon = __pol;
    endResetModel();
    return !_polygon.isEmpty();
}
const tPolygonF &cPolygonTableModel::polygon() const
{
    return _polygon;
}

cPolygonTableModel& cPolygonTableModel::up(const QModelIndexList& mil)
{
    QVector<int> rows = mil2rowsAsc(mil);
    beginResetModel();
    int s = rows.size();
    for (int i = 0; i < s; ++i) {
        int row1 = rows[i];
        if (!isContIx(_polygon, row1)) continue;
        int row2 = row1 -1;
        if (row2 < 0) row2 = _polygon.size() -1;
        _polygon.swap(row1, row2);
    }
    endResetModel();
    return *this;
}
cPolygonTableModel& cPolygonTableModel::down(const QModelIndexList& mil)
{
    QVector<int> rows = mil2rowsDesc(mil);
    beginResetModel();
    int s = rows.size();
    for (int i = 0; i < s; ++i) {
        int row1 = rows[i];
        if (!isContIx(_polygon, row1)) continue;
        int row2 = row1 +1;
        if (row2 >=  _polygon.size()) row2 = 0;
        _polygon.swap(row1, row2);
    }
    endResetModel();
    return *this;
}

cPolygonTableModel& cPolygonTableModel::remove(QModelIndexList& mil)
{
    QVector<int> rows = mil2rowsDesc(mil);
    beginResetModel();
    int s = rows.size();
    for (int i = 0; i < s; ++i) {
        remove(rows[i]);
    }
    endResetModel();
    return *this;
}

/* **************************************** cRecordListModel ****************************************  */

cRecordListModel::cRecordListModel(const cRecStaticDescr& __d, QObject * __par)
    : QStringListModel(__par)
    , pDescr(&__d), pattern(), cnstFlt(), nameList(), viewList(), idList()
{
    order = OT_ASC;
    filter = FT_NO;
    pq = newQuery();
    setStringList(viewList);
    nullable  = false;
    nullIdIsAll = false;
    only = false;
}

cRecordListModel::cRecordListModel(const QString& __t, const QString& __s, QObject * __par)
    : QStringListModel(__par)
    , pDescr(cRecStaticDescr::get(__t, __s)), pattern(), cnstFlt(), nameList(), viewList(), idList()
{
    order = OT_ASC;
    filter = FT_NO;
    pq = newQuery();
    setStringList(viewList);
    nullable  = false;
    nullIdIsAll = false;
    only = false;
}

cRecordListModel::~cRecordListModel()
{
    delete pq;
}

/*
void cRecordListModel::changeRecDescr(cRecStaticDescr * _descr)
{
    pDescr = _descr;
    nameList.clear();
    idList.clear();
    viewList.clear();
    setStringList(viewList);
}
*/
QVariant cRecordListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (nullable && row == 0) { // NULL
        return dcRole(DC_NULL, role);
    }
    if (role == Qt::DisplayRole) {
        return viewList[row];
    }
    return dcRole(dcData, role);
}

void cRecordListModel::_setOwnerId(qlonglong _oid, const QString& _fn, eTristate _nullIsAll)
{
    if (!_fn.isEmpty()) sFkeyName = _fn;
    setBool(nullIdIsAll, _nullIsAll);
    if (_oid == NULL_ID) {
        if (!nullIdIsAll) ownerFlt = _sFalse;     // Nincs egyetlen rekord sem
        else              ownerFlt.clear();       // Nincs szűrés
    }
    else {
        if (sFkeyName.isEmpty()) {  // Ha nem ismert owner vagy parent ID, akkor a mezőnevet be kel állítani elsőre!!!
            int ix = pDescr->ixToOwner(EX_IGNORE);
            if (ix < 0) ix = pDescr->ixToParent();
            sFkeyName = pDescr->columnName(ix);
        }
        ownerFlt = sFkeyName + " = " + QString::number(_oid);
    }
}

void cRecordListModel::setConstFilter(const QVariant& _par, enum eFilterType __f)
{
    QString nn;
    QString pat;
    bool b  = false;
    switch (__f) {
    case FT_NO:         break;
    case FT_LIKE:
    case FT_SIMILAR:
    case FT_REGEXP:
    case FT_BEGIN:
    case FT_SQL_WHERE:  pat = _par.toString();
                        break;
    case FT_BOOLEAN:    b = _par.toBool();
                        break;
    default:            EXCEPTION(EPROGFAIL, __f, _par.toString());
    }
    if (toNameFName.isEmpty()) {
        nn = pDescr->columnNameQ(pDescr->nameIndex());
    }
    else {
        nn = _sName;
    }
    switch (__f) {
    case FT_NO:         cnstFlt.clear();    break;
    case FT_LIKE:       cnstFlt = nn + " LIKE " +       quoted(pat);           break;
    case FT_SIMILAR:    cnstFlt = nn + " SIMILAR TO " + quoted(pat);           break;
    case FT_REGEXP:     cnstFlt = nn + " ~ " +          quoted(pat);           break;
    case FT_BEGIN:      cnstFlt = nn + " LIKE " + quoted(pat + QChar('%'));    break;
    case FT_SQL_WHERE:  cnstFlt = pat;                                         break;
    case FT_BOOLEAN:    cnstFlt = (b ? _sSpace : " NOT ") + nn + "::boolean";  break;
    default:            EXCEPTION(EPROGFAIL);
    }
}

QString cRecordListModel::_where(QString s)
{
    QStringList ws;
    if (ownerFlt == _sFalse) {
        ws << _sFalse;
    }
    else {
        if (!s.       isEmpty()) ws << s;
        if (!cnstFlt. isEmpty()) ws << cnstFlt;
        if (!ownerFlt.isEmpty()) ws << ownerFlt;
    }
    if (ws.isEmpty()) return QString();
    return " WHERE " + ws.join(" AND ");
}

bool cRecordListModel::setFilter(const QVariant& _par, enum eOrderType __o, enum eFilterType __f)
{
    _DBGFN() << "@(" << _par << ")" << endl;
    if (__o != OT_DEFAULT) order   = __o;
    if (__f != FT_DEFAULT) filter  = __f;
    if (!_par.isNull()) setPattern(_par);
    nameList.clear();
    viewList.clear();
    idList.clear();
    if (nullable) {
        nameList << _sNul;
        viewList << dcViewShort(DC_NULL);
        idList   << NULL_ID;
    }
    QString sql = select();
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
    bool r = pq->first();
    if (r) do {
        idList     << variantToId(pq->value(0));
        nameList << pq->value(1).toString();
        if (viewExpr.isEmpty()) {
            viewList << nameList.last();
        }
        else {
            viewList << pq->value(2).toString();
        }
    } while (pq->next());
    // Túl nagyok a listák: pl. a szolgáltatás példányok kezelhetetlen, hálózati elemek is túl nagy :( EZT javítani KELL
//    PDEB(VVERBOSE) << "name list :" << nameList.join(_sCommaSp) << endl;
//    PDEB(VVERBOSE) << "view list :" << viewList.join(_sCommaSp) << endl;
    setStringList(viewList);
    return r;
}

QString cRecordListModel::where(const QString& nameName)
{
    QString w;
    switch (filter) {
    case FT_NO:                                                           break;
    case FT_LIKE:       w = nameName + " LIKE " + quoted(pattern);              break;
    case FT_SIMILAR:    w = nameName + " SIMILAR TO " + quoted(pattern);        break;
    case FT_REGEXP:     w = nameName + " ~ " +          quoted(pattern);        break;
//    case FT_REGEXPI:    w = nameName + " ~* " +         quoted(pattern);        break;
    case FT_BEGIN:      w = nameName + " LIKE " + quoted(pattern + QChar('%')); break;
    case FT_SQL_WHERE:  w = pattern;                                      break;
    case FT_BOOLEAN:    w = (str2bool(pattern) ? _sSpace : " NOT ") + nameName + "::boolean";   break;
    default:            EXCEPTION(EPROGFAIL);
    }
    return _where(w);
}

QString cRecordListModel::_order(const QString& nameName, const QString& idName)
{
    QString o;
    switch (order) {
    case OT_NO:     break;
    case OT_ASC:    o = " ORDER BY " + nameName;             break;
    case OT_DESC:   o = " ORDER BY " + nameName + " DESC";   break;
    case OT_ID_ASC: o = " ORDER BY " + idName;               break;
    default:    EXCEPTION(EPROGFAIL);  // lehetetlen, de warningol
    }
    return o;
}

QString cRecordListModel::select()
{
    QString fn, nn, view;
    QString in = pDescr->columnNameQ(pDescr->idIndex());
    if (toNameFName.isEmpty()) {
        nn = pDescr->nameName();
        dcData = DC_NAME;
    }
    else {
        fn = toNameFName + QChar('(') + in +QChar(')') + QChar(' ');
        nn = quotedString(_sName);
        dcData = DC_DATA;
    }
    if (!viewExpr.isEmpty()) {
        view = ", " + viewExpr;
        dcData = DC_DERIVED;
    }
    QString sOnly = only ? " ONLY " : _sNul;
    QString sql = "SELECT " + in + QChar(',') + fn + nn + view + " FROM " + sOnly + pDescr->tableName();
    sql += where(nn);
    sql += _order(nn, pDescr->idName());
    PDEB(VERBOSE) << "SQL : \"" << sql << "\"" << endl;
    return sql;
}

void cRecordListModel::setPattern(const QVariant& _par)
{
    switch (filter) {
    case FT_NO:         break;
    case FT_LIKE:
    case FT_SIMILAR:
    case FT_REGEXP:
    //case FT_REGEXPI:
    case FT_BEGIN:
    case FT_SQL_WHERE:  pattern = _par.toString();
                        break;
    case FT_BOOLEAN:    pattern = _par.toBool() ? _sTrue : _sFalse;
                        break;
    default:            EXCEPTION(EPROGFAIL, filter, _par.toString());
    }
}

qlonglong cRecordListModel::idOf(const QString& __s)
{
    int ix = indexOf(__s);
    if (ix < 0) return NULL_ID;
    return idList[ix];
}

QString cRecordListModel::nameOf(qlonglong __id)
{
    int ix = indexOf(__id);
    if (ix < 0) return QString();
    return nameList[ix];
}


void cRecordListModel::setPatternSlot(const QVariant &__pat)
{
    PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " __pat = "  << __pat.toString() << endl;
    setFilter(__pat);
}

cRecordListModel& cRecordListModel::copy(const cRecordListModel& _o)
{
    if (*pDescr != *_o.pDescr) EXCEPTION(EDATA, 0, trUtf8("Copy model type: %1 to %2").arg(pDescr->tableName(), _o.pDescr->tableName()));
    nullable    = _o.nullable;
    nullIdIsAll = _o.nullIdIsAll;
    only        = _o.only;

    order       = _o.order;
    filter      = _o.filter;
    pattern     = _o.pattern;
    cnstFlt     = _o.cnstFlt;
    ownerFlt    = _o.ownerFlt;
    sFkeyName   = _o.sFkeyName;
    nameList    = _o.nameList;
    viewList    = _o.viewList;
    idList      = _o.idList;
    toNameFName = _o.toNameFName;
    viewExpr    = _o.viewExpr;
    setStringList(viewList);
    return *this;
}

void cRecordListModel::joinWith(QComboBox *_pComboBox)
{
    pComboBox = _pComboBox;
    palette   = _pComboBox->palette();
    font      = _pComboBox->font();
    nullPalette = palette;
    nullPalette.setColor(QPalette::ButtonText, dcFgColor(DC_NULL));
    nullPalette.setColor(QPalette::Button, dcBgColor(DC_NULL));
    nullFont  = dcFont(DC_NULL);
    connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndex(int)));
}

void cRecordListModel::currentIndex(int i)
{
    if (nullable && i == 0) {
        pComboBox->setPalette(nullPalette);
        pComboBox->setFont(nullFont);
    }
    else {
        pComboBox->setPalette(palette);
        pComboBox->setFont(fontByEnum(_sDatacharacter, dcData));
    }
}

/* ************************************************ cZoneListModel ***************************************************** */
cZoneListModel::cZoneListModel(QObject * __par)
    : cRecordListModel(_sPlaceGroups, _sPublic, __par)
{
    setConstFilter("place_group_type = 'zone'", FT_SQL_WHERE);
    setFilter(QVariant(), OT_ASC, FT_NO);
}

bool cZoneListModel::setFilter(const QVariant &_par, eOrderType __o, eFilterType __f)
{
    _DBGFN() << "@(" << _par << ")" << endl;
    if (__o != OT_DEFAULT) order   = __o;
    if (__f != FT_DEFAULT) filter  = __f;
    if (!_par.isNull()) setPattern(_par);
    nameList.clear();
    idList.clear();

    QString sql = "SELECT place_group_id , place_group_name  FROM place_groups ";
    sql += where(_sPlaceGroupName);
    sql += _order(_sPlaceGroupName, _sPlaceGroupId);

    PDEB(SQL) << "SQL : " << sql << endl;
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);

    bool all = false;
    bool r = pq->first();
    if (r) do {
        qlonglong id =variantToId(pq->value(0));
        if (id == ALL_PLACE_GROUP_ID) {
            all = true;
            continue;
        }
        idList     << id;
        nameList << pq->value(1).toString();
    } while (pq->next());
    if (all) {
        nameList.push_front(_sAll);
        idList.push_front(ALL_PLACE_GROUP_ID);
    }
    if (nullable) {
        nameList.push_front(_sNul);
        idList.push_front(NULL_ID);
    }
    PDEB(VVERBOSE) << "name list :" << nameList.join(_sCommaSp) << endl;
    viewList = nameList;
    dcData = DC_NAME;
    setStringList(nameList);
    return r;
}

/* ************************************************ cPlacesInZoneModel ***************************************************** */

const QString cPlacesInZoneModel::sqlSelect =
        "WITH RECURSIVE tree AS"
        " (SELECT place_id, place_name, parent_id FROM places JOIN place_group_places USING(place_id) WHERE place_group_id = %1"
         " UNION"
         " SELECT places.place_id, places.place_name, places.parent_id FROM places INNER JOIN tree ON places.parent_id = tree.place_id"
         ")"
         " SELECT place_id, place_name FROM tree ";
const QString cPlacesInZoneModel::sqlSelectAll =
        "SELECT place_id, place_name FROM places ";

cPlacesInZoneModel::cPlacesInZoneModel(QObject * __par)
    : cRecordListModel(_sPlaces, _sPublic, __par)
{
    nullable    = true;
    skipUnknown = true;
    zoneId      = ALL_PLACE_GROUP_ID;
    setFilter();
}

bool cPlacesInZoneModel::setFilter(const QVariant& _par, enum eOrderType __o, enum eFilterType __f)
{
    _DBGFN() << "@(" << _par << ")" << endl;
    if (__o != OT_DEFAULT) order   = __o;
    if (__f != FT_DEFAULT) filter  = __f;
    if (!_par.isNull()) setPattern(_par);
    nameList.clear();
    idList.clear();
    if (nullable) {
        nameList << _sNul;
        idList     << NULL_ID;
    }
    QString sql;
    if (zoneId == ALL_PLACE_GROUP_ID) {
        sql = sqlSelectAll;
    }
    else {
        sql = sqlSelect.arg(zoneId);
    }
    sql += where(_sPlaceName);
    sql += _order(_sPlaceName, _sPlaceId);
    PDEB(VERBOSE) << "SQL : \"" << sql << "\"" << endl;
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
    bool r = pq->first();
    if (r) do {
        qlonglong id = variantToId(pq->value(0));
        if (id == ROOT_PLACE_ID) continue;
        if (skipUnknown && id == UNKNOWN_PLACE_ID) continue;
        QString name = pq->value(1).toString();
        idList     << id;
        nameList << name;
    } while (pq->next());
    PDEB(VVERBOSE) << "name list :" << nameList.join(_sCommaSp) << endl;
    viewList = nameList;
    dcData = DC_NAME;
    setStringList(nameList);
    return r;
}

void cPlacesInZoneModel::setZone(const QString& name)
{
    zoneId = cPlaceGroup().getIdByName(*pq, name);
    setFilter();
}

void cPlacesInZoneModel::setZone(qlonglong id)
{
    zoneId = id;
    setFilter();
}

/* ************************************************ cEnumListModel ***************************************************** */

cEnumListModel::cEnumListModel(const QString& __t, eNullType _nullable, const tIntVector &_eList, QObject * __par)
    : QAbstractListModel(__par)
{
    pType     = NULL;
    pq        = NULL;
    pComboBox = NULL;
    setEnum(__t, _nullable, _eList);
}

cEnumListModel::cEnumListModel(const cColEnumType *_pType, eNullType _nullable, const tIntVector& _eList, QObject * __par)
    : QAbstractListModel(__par)
{
    pType     = NULL;
    pq        = NULL;
    pComboBox = NULL;
    setEnum(_pType, _nullable, _eList);
}

cEnumListModel::~cEnumListModel()
{
    pDelete(pq);
}

int cEnumListModel::setEnum(const QString& __t, eNullType _nullable, const tIntVector& _eList, eEx __ex)
{
    if (pq == NULL) pq = newQuery();
    return setEnum(cColEnumType::fetchOrGet(*pq, __t, __ex), _nullable, _eList, __ex);
}

int cEnumListModel::setEnum(const cColEnumType *_pType, eNullType _nullable, const tIntVector& _eList, eEx __ex)
{
    beginResetModel();
    enumVals.clear();
    pType = _pType;
    if (pType == NULL) {
        if (__ex != EX_IGNORE) EXCEPTION(ENONAME);
        endResetModel();
        return 0;
    }
    nullType = _nullable;
    if (nullType != NT_NOT_NULL) {
        enumVals <<  &cEnumVal::enumVal(_sDatacharacter, nullType, __ex);
    }
    tIntVector eList = _eList;
    int i;
    if (eList.isEmpty()) for (i = 0; i < pType->enumValues.size(); ++i) eList << i;
    foreach (i, _eList) {
        const QString& typeName = *(const QString *)pType;
        const cEnumVal& ee = cEnumVal::enumVal(typeName, i, __ex);
        if (ee.isEmpty_()) {
            cEnumVal *p = new cEnumVal;
            p->setParent(this);
            const QString& s = pType->enumValues.at(i);
            p->setName(cEnumVal::ixTypeName(),  typeName);
            p->setName(cEnumVal::ixValName(),   s);
            enumVals << p;
        }
        else {
            enumVals <<  &ee;
        }
    }
    endResetModel();
    return enumVals.size();
}

void cEnumListModel::joinWith(QComboBox *_pComboBox)
{
    pComboBox = _pComboBox;
    pComboBox->setModel(this);
    palette   = pComboBox->palette();
    connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndex(int)));
}

void cEnumListModel::clear()
{
    beginResetModel();
    nullType = NT_NOT_NULL;
    pType = NULL;
    enumVals.clear();
    endResetModel();
}

int cEnumListModel::rowCount(const QModelIndex &) const
{
    return enumVals.size();
}

QVariant cEnumListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (!isContIx(enumVals, row)) return QVariant();
    const cEnumVal& ev = *enumVals[row];
    int             e  = pType->str2enum(ev.getName());
    return enumRole(ev, role, e);
}

int cEnumListModel::atInt(int ix) const
{
    int r = ENUM_INVALID;
    if (isContIx(enumVals, ix)) {
        r = pType->str2enum(enumVals.at(ix)->getName());
    }
    return r;
}

QString cEnumListModel::at(int ix) const
{
    QString r;
    if (isContIx(enumVals, ix)) {
        r = enumVals.at(ix)->getName();
    }
    return r;
}

int cEnumListModel::indexOf(int e) const
{
    return indexOf(pType->enum2str(e));
}
int cEnumListModel::indexOf(const QString& s) const
{
    int i = 0;
    foreach (const cEnumVal * pev, enumVals) {
        if (s == pev->getName()) return i;
        ++i;
    }
    return -1;
}

void cEnumListModel::currentIndex(int i)
{
    QPalette pal = palette;
    if (!isContIx(enumVals, i)) return ;
    const cEnumVal& ev = *enumVals[i];
    QString type = ev.getName(cEnumVal::ixTypeName());
    int     eval = ev.toInt();
    if (!ev.isNull(cEnumVal::ixBgColor())) pal.setColor(QPalette::Button,     bgColorByEnum(type, eval));
    if (!ev.isNull(cEnumVal::ixFgColor())) pal.setColor(QPalette::ButtonText, fgColorByEnum(type, eval));
    pComboBox->setPalette(pal);
    pComboBox->setFont(fontByEnum(type, eval));
}


