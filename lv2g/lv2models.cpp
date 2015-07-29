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
    : QStringListModel(__par), pattern(), cnstFlt(), stringList(), idList(), descr(__d)
{
    order = OT_ASC;
    filter = FT_NO;
    pq = newQuery();
    setStringList(stringList);
    firstTime = true;
    nullable  = false;
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

bool cRecordListModel::setFilter(const QString& _par, enum eOrderType __o, enum eFilterType __f)
{
    _DBGFN() << "@(" << _par << ")" << endl;
    firstTime = false;
    if (__o != OT_DEFAULT) order   = __o;
    if (__f != FT_DEFAULT) filter  = __f;
    if (!_par.isEmpty())   pattern = _par;
    stringList.clear();
    idList.clear();
    if (nullable) {
        stringList << _sNul;
        idList     << NULL_ID;
    }
    QString fn, nn;
    QString in = descr.columnNameQ(descr.idIndex());
    if (toNameFName.isEmpty()) {
        nn = descr.columnNameQ(descr.nameIndex());
    }
    else {
        fn = toNameFName + QChar('(') + in +QChar(')') + QChar(' ');
        nn = quotedString(_sName);
    }
    QString sql = "SELECT " + in + QChar(',') + fn + nn + " FROM " + descr.tableName();
    switch (filter) {
    case FT_NO:         sql += where();                                             break;
    case FT_LIKE:       sql += where(nn + " LIKE " + quoted(pattern));              break;
    case FT_SIMILAR:    sql += where(nn + " SIMILAR TO " + quoted(pattern));        break;
    case FT_REGEXP:     sql += where(nn + " ~ " +          quoted(pattern));        break;
    case FT_REGEXPI:    sql += where(nn + " ~* " +         quoted(pattern));        break;
    case FT_BEGIN:      sql += where(nn + " LIKE " + quoted(pattern + QChar('%'))); break;
    case FT_SQL_WHERE:  sql += where(pattern);                                      break;
    default:            EXCEPTION(EPROGFAIL);
    }
    switch (order) {
    case OT_NO:  break;
    case OT_ASC: sql += " ORDER BY " + nn;   break;
    case OT_DESC:sql += " ORDER BY " + nn + " DESC"; break;
    default:    EXCEPTION(EPROGFAIL);  // lehetetlen, de warningol
    }
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


void cRecordListModel::setPatternSlot(const QString& __pat)
{
    PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << " __pat = "  << __pat << endl;
    setFilter(__pat);
}

