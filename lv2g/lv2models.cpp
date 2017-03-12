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
    : QStringListModel(__par), descr(__d), pattern(), cnstFlt(), stringList(), idList()
{
    order = OT_ASC;
    filter = FT_NO;
    pq = newQuery();
    setStringList(stringList);
    nullable  = false;
    nullIdIsAll = false;
    only = false;
}

cRecordListModel::cRecordListModel(const QString& __t, const QString& __s, QObject * __par)
    : QStringListModel(__par), descr(*cRecStaticDescr::get(__t, __s)), pattern(), cnstFlt(), stringList(), idList()
{
    order = OT_ASC;
    filter = FT_NO;
    pq = newQuery();
    setStringList(stringList);
    nullable  = false;
    nullIdIsAll = false;
    only = false;
}

cRecordListModel::~cRecordListModel()
{
    delete pq;
}

void cRecordListModel::setConstFilter(const QString& _par, enum eFilterType __f)
{
    QString fn, nn;
    QString in = descr.columnNameQ(descr.idIndex());
    if (toNameFName.isEmpty()) {
        nn = descr.columnNameQ(descr.nameIndex());
    }
    else {
        fn = toNameFName + QChar('(') + in +QChar(')') + QChar(' ');
        nn = _sName;
    }
    switch (__f) {
    case FT_NO:         cnstFlt.clear();    break;
    case FT_LIKE:       cnstFlt = nn + " LIKE " +       quoted(_par);   break;
    case FT_SIMILAR:    cnstFlt = nn + " SIMILAR TO " + quoted(_par);   break;
    case FT_REGEXP:     cnstFlt = nn + " ~ " +          quoted(_par);   break;
    case FT_REGEXPI:    cnstFlt = nn + " ~* " +         quoted(_par);   break;
    case FT_BEGIN:      cnstFlt = nn + " LIKE " + quoted(_par + QChar('%')); break;
    case FT_SQL_WHERE:  cnstFlt = _par;                                 break;
    default:            EXCEPTION(EPROGFAIL);
    }
}

QString cRecordListModel::where(QString s)
{
    if (cnstFlt.isEmpty()) {
        if (s.isEmpty()) return QString();
        return " WHERE " + s;
    }
    else {
        QString r = " WHERE " + cnstFlt;
        if (s.isEmpty()) return r;
        return r + " AND " + s;
    }
}

bool cRecordListModel::setFilter(const QVariant& _par, enum eOrderType __o, enum eFilterType __f)
{
    _DBGFN() << "@(" << _par << ")" << endl;
    if (__o != OT_DEFAULT) order   = __o;
    if (__f != FT_DEFAULT) filter  = __f;
    if (!_par.isNull()) setPattern(_par);
    stringList.clear();
    idList.clear();
    if (nullable) {
        stringList << _sNul;
        idList     << NULL_ID;
    }
    QString sql = select();
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
    bool r = pq->first();
    if (r) do {
        idList     << variantToId(pq->value(0));
        stringList << pq->value(1).toString();
    } while (pq->next());
    PDEB(VVERBOSE) << "name list :" << stringList.join(_sCommaSp) << endl;
    setStringList(stringList);
    return r;
}

QString cRecordListModel::select()
{
    QString fn, nn;
    QString in = descr.columnNameQ(descr.idIndex());
    if (toNameFName.isEmpty()) {
        nn = descr.nameName();
    }
    else {
        fn = toNameFName + QChar('(') + in +QChar(')') + QChar(' ');
        nn = quotedString(_sName);
    }
    QString sOnly = only ? " ONLY " : _sNul;
    QString sql = "SELECT " + in + QChar(',') + fn + nn + " FROM " + sOnly + descr.tableName();
    switch (filter) {
    case FT_NO:         sql += where();                                             break;
    case FT_LIKE:       sql += where(nn + " LIKE " + quoted(pattern));              break;
    case FT_SIMILAR:    sql += where(nn + " SIMILAR TO " + quoted(pattern));        break;
    case FT_REGEXP:     sql += where(nn + " ~ " +          quoted(pattern));        break;
    case FT_REGEXPI:    sql += where(nn + " ~* " +         quoted(pattern));        break;
    case FT_BEGIN:      sql += where(nn + " LIKE " + quoted(pattern + QChar('%'))); break;
    case FT_SQL_WHERE:  sql += where(pattern);                                      break;
    case FT_FKEY_ID:    {
        QString s;
        if (fkey_id == NULL_ID) {
            if (!nullIdIsAll) s = " FALSE ";
        }
        else {
            if (sFkeyName.isEmpty()) {  // Ha nem owner vayg parent ID, akkor a mezőnevet be kel állítani elsőre!!!
                int ix = descr.ixToOwner(EX_IGNORE);
                if (ix < 0) ix = descr.ixToParent();
                sFkeyName = descr.columnName(ix);
            }
            s = sFkeyName + " = " + QString::number(fkey_id);
        }
        sql += where(s);
        break;
    }
    default:            EXCEPTION(EPROGFAIL);
    }
    switch (order) {
    case OT_NO:     break;
    case OT_ASC:    sql += " ORDER BY " + nn;               break;
    case OT_DESC:   sql += " ORDER BY " + nn + " DESC";     break;
    case OT_ID_ASC: sql += " ORDER BY " + descr.idName();   break;
    default:    EXCEPTION(EPROGFAIL);  // lehetetlen, de warningol
    }
    PDEB(VERBOSE) << "SQL : \"" << sql << "\"" << endl;
    return sql;
}

void cRecordListModel::setPattern(const QVariant& _par)
{
    bool ok = true;
    switch (filter) {
    case FT_NO:         break;
    case FT_LIKE:
    case FT_SIMILAR:
    case FT_REGEXP:
    case FT_REGEXPI:
    case FT_BEGIN:
    case FT_SQL_WHERE:  pattern = _par.toString();
                        break;
    case FT_FKEY_ID:    fkey_id = _par.isNull() ? NULL_ID : _par.toLongLong(&ok);
                        if (ok) break;
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
    return stringList[ix];
}


void cRecordListModel::setPatternSlot(const QVariant &__pat)
{
    PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " __pat = "  << __pat.toString() << endl;
    setFilter(__pat);
}

cRecordListModel& cRecordListModel::copy(const cRecordListModel& _o)
{
    if (descr != _o.descr) EXCEPTION(EDATA, 0, trUtf8("Copy model type: %1 to %2").arg(descr.tableName(), _o.descr.tableName()));
    nullable    = _o.nullable;
    nullIdIsAll = _o.nullIdIsAll;
    only        = _o.only;

    order       = _o.order;
    filter      = _o.filter;
    pattern     = _o.pattern;
    fkey_id     = _o.fkey_id;
    cnstFlt     = _o.cnstFlt;
    stringList  = _o.stringList;
    idList      = _o.idList;
    toNameFName = _o.toNameFName;
    setStringList(stringList);
    return *this;
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
    stringList.clear();
    idList.clear();
    QString sql = select();
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
        stringList << pq->value(1).toString();
    } while (pq->next());
    if (all) {
        stringList.push_front(_sAll);
        idList.push_front(ALL_PLACE_GROUP_ID);
    }
    if (nullable) {
        stringList.push_front(_sNul);
        idList.push_front(NULL_ID);
    }
    PDEB(VVERBOSE) << "name list :" << stringList.join(_sCommaSp) << endl;
    setStringList(stringList);
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
    nullable = true;
    setFilter(QVariant(ALL_PLACE_GROUP_ID));
}

bool cPlacesInZoneModel::setFilter(const QVariant& _par, enum eOrderType __o, enum eFilterType __f)
{
    _DBGFN() << "@(" << _par << ")" << endl;
    (void)__f;
    if (__o != OT_DEFAULT) order = __o;
    bool ok = true;
    if (!_par.isNull()) {
        fkey_id = _par.toLongLong(&ok);
        if (!ok) EXCEPTION(EPROGFAIL, 0, _par.toString());
    }
    stringList.clear();
    idList.clear();
    if (nullable) {
        stringList << _sNul;
        idList     << NULL_ID;
    }
    QString sql = fkey_id == ALL_PLACE_GROUP_ID ? sqlSelectAll : sqlSelect.arg(fkey_id);
    sql += where();
    switch (order) {
    case OT_NO:     break;
    case OT_ASC:    sql += " ORDER BY place_name";      break;
    case OT_DESC:   sql += " ORDER BY place_name DESC"; break;
    case OT_ID_ASC: sql += " ORDER BY place_id";        break;
    default:    EXCEPTION(EPROGFAIL);  // lehetetlen, de warningol
    }
    PDEB(VERBOSE) << "SQL : \"" << sql << "\"" << endl;
    if (!pq->exec(sql)) SQLPREPERR(*pq, sql);
    bool r = pq->first();
    if (r) do {
        qlonglong id = variantToId(pq->value(0));
        if (id == UNKNOWN_PLACE_ID || id == ROOT_PLACE_ID) continue;
        idList     << id;
        stringList << pq->value(1).toString();
    } while (pq->next());
    PDEB(VVERBOSE) << "name list :" << stringList.join(_sCommaSp) << endl;
    setStringList(stringList);
    return r;
}

