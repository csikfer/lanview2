#include "lv2models.h"

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
    int s = _stringList.size();
    if (mil.isEmpty()) return *this;
    if (mil.size() == 1) {
        int row = mil[0].row();
        if (row < s) return remove(row);
        return *this;
    }
    QBitArray   rb(s, false);
    foreach(QModelIndex ix, mil) {
        int row = ix.row();
        if (row < s) rb.setBit(row, true);
    }
    beginResetModel();
    for (int i = s; i > 0;) {
        --i;
        if (rb[i]) remove(i);
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
        case 0: return _polygon.at(row).x();
        case 1: return _polygon.at(row).y();
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
bool cPolygonTableModel::setPolygon(const QPolygonF &__pol)
{
    beginResetModel();
    _polygon = __pol;
    endResetModel();
    return !_polygon.isEmpty();
}
const QPolygonF &cPolygonTableModel::polygon() const
{
    return _polygon;
}

cPolygonTableModel& cPolygonTableModel::remove(QModelIndexList& mil)
{
    int s = _polygon.size();
    if (mil.isEmpty()) return *this;
    if (mil.size() == 1) {
        int row = mil[0].row();
        if (row < s) return remove(row);
        return *this;
    }
    QBitArray   rb(s, false);
    foreach(QModelIndex ix, mil) {
        int row = ix.row();
        if (row < s) rb.setBit(row, true);
    }
    beginResetModel();
    for (int i = s; i > 0;) {
        --i;
        if (rb[i]) remove(i);
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
        fn = toNameFName + _sABraB + in +_sABraE + _sSpace;
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
        fn = toNameFName + _sABraB + in +_sABraE + _sSpace;
        nn = quotedString(_sName);
    }
    QString sql = "SELECT " + in + _sComma + fn + nn + " FROM " + descr.tableName();
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
        idList     << pq->value(0).toLongLong();
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

