#include "record_table_model.h"
#include "record_table.h"

cRecordTableModel::cRecordTableModel(const cRecordTable& _rt)
    : QAbstractTableModel(_rt.pWidget())
    , recordTable(_rt)
    , recDescr(_rt.recDescr())
    , descriptor(_rt.tableShape())
    , columns(_rt.fields)
    , extLines()
    , _records()
    , _col2shape(), _col2field()
{
    _viewRowNumbers = true;
    _viewHeader     = true;
    _firstRowNumber =   0;
    _maxRows        = 100;
    pq = newQuery();
    int i, n = columns.size();
    for (i = 0; i < n; ++i) {
        const cRecordTableColumn&  column = *columns[i];
        if (column.shapeField.getBool(_sIsHide)) {
            PDEB(VVERBOSE) << "Hidden field : " << columns[i]->shapeField.getName(_sTableShapeFieldName) << endl;
            continue;
        }
        _col2shape << i;
        _col2field << recDescr.toIndex(column.shapeField.getName());
        PDEB(VVERBOSE) << "Visible field : " << column.shapeField.getName(_sTableShapeFieldName) << endl;
    }
    PDEB(VVERBOSE) << "X tabs : " << tIntVectorToString(_col2field) << _sSpace << tIntVectorToString(_col2shape) << endl;
}

cRecordTableModel::~cRecordTableModel()
{
    clear();
    delete pq;
}

int cRecordTableModel::rowCount(const QModelIndex &) const
{
    return _records.size();
}

int cRecordTableModel::columnCount(const QModelIndex &) const
{
    return _col2field.size();
}

QVariant cRecordTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        //DWAR() << "Invalid modelIndex." << endl;
        return QVariant();
    }
    int row = index.row();      // Sor index a táblázatban
    int col = index.column();   // oszlop index a táblázatban
    QVariant r;
    if (row < _records.size() && col < _col2field.size()) {
        int fix = _col2field[col];  // Mező index a rekordbam
        int mix = _col2shape[col];  // Index a leíróban
        // _DBGFN() << VDEB(row) << VDEB(col) << VDEB(role) << endl;
        switch (role) {
        case Qt::DisplayRole:
            r =  _records.at(row)->view(*pq, fix);
            break;
        case Qt::TextAlignmentRole:
            r = columns[mix]->dataAlign;
            break;
        case Qt::ForegroundRole:
            if (_records.at(row)->isNull(fix)) r = columns[mix]->fgNullColor;
            else if (extLines.contains(row))   r = columns[mix]->fgExDataColor;
            else                               r = columns[mix]->fgDataColor;
            break;
        case Qt::BackgroundRole:
            if (extLines.contains(row))        r = columns[mix]->bgExDataColor;
            else                               r = columns[mix]->bgDataColor;
            break;
        case Qt::FontRole:
            if (_records.at(row)->isNull(fix)) r = columns[mix]->nullFont;
            else                               r = columns[mix]->dataFont;
            break;
        }
        //_DBGFNL() << " = " << quotedString(r.toString()) << endl;
    }
 /*   else {
        DWAR() << "Index out of range : " << VDEB(row) << VDEB(col) << endl;
    }*/
    return r;
}

QVariant cRecordTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
//    _DBGFN() << VDEB(section) << VDEB(orientation) << VDEB(role) << endl;
    QVariant r;
    switch (orientation) {
    case Qt::Vertical:
        switch (role) {
        case Qt::DisplayRole:
            if (_viewRowNumbers) r = QVariant(section + _firstRowNumber +1) ;
            break;
        case Qt::TextAlignmentRole:
        case Qt::ForegroundRole:
        case Qt::BackgroundRole:
        case Qt::FontRole:
            break;
        }
        break;
    case Qt::Horizontal:
        if (section < _col2shape.size()) {
            int mix = _col2shape[section];
            switch (role) {
            case Qt::DisplayRole:
                if (_viewHeader) r = columns[mix]->header;
                break;
            case Qt::TextAlignmentRole:
                r = columns[mix]->headAlign;
                break;
            case Qt::ForegroundRole:
                r = columns[mix]->fgHeadColor;
                break;
            case Qt::BackgroundRole:
                r = columns[mix]->bgHeadColor;
                break;
            case Qt::FontRole:
                r = columns[mix]->headFont;
                break;
            }
        }
    break;
    }
//    _DBGFNL() << " = " << quotedString(r.toString()) << endl;
    return r;
}

bool cRecordTableModel::setRecords(const tRecords& __recs, int _firstNm)
{
    DBGFN();
    if (__recs.size() && __recs.first()->descr() != recDescr) EXCEPTION(EDATA);
    beginResetModel();
    _firstRowNumber = _firstNm;
    _records        = __recs;
    endResetModel();
    int r = _records.size();
    _DBGFNL() << " = " << r << endl;
    return r;

}

bool cRecordTableModel::update(int row, cAlternate *pRec)
{
    if (row < 0 || row >= _records.size()) return false;
    beginResetModel();
    delete _records[row];
    _records[row] = pRec;
    endResetModel();
    return true;
}

cRecordTableModel& cRecordTableModel::operator<<(const cAlternate& _r)
{
    beginResetModel();
    _records << _r;
    endResetModel();
    return *this;
}

cRecordTableModel& cRecordTableModel::insert(const cAlternate& _r, int i)
{
    // ?????!!!!!
    if (i < 0) i = _records.size();
    beginResetModel();
    _records.insert(i, _r);
    endResetModel();
    return *this;
}

cRecordTableModel& cRecordTableModel::remove(int i)
{
    // ?????!!!!!
    _DBGFN() << " #" << i << endl;
    beginResetModel();
    cAlternate * p = _records.list().takeAt(i);
    endResetModel();
    _removed(p);
    return *this;
}

cRecordTableModel& cRecordTableModel::pop_back()
{
    if (_records.isEmpty()) return *this;
    return remove(_records.size() -1);
}

cRecordTableModel& cRecordTableModel::pop_front()
{
    if (_records.isEmpty()) return *this;
    return remove(0);
}

cRecordTableModel& cRecordTableModel::clear()
{
    beginResetModel();
    _records.clear();
    _firstRowNumber = 0;
    endResetModel();
    return *this;
}

cRecordTableModel& cRecordTableModel::removeAll()
{
    while (_records.size()) pop_back();
    return *this;
}

cRecordTableModel& cRecordTableModel::remove(QModelIndexList& mil)
{
    int s = _records.size();
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

void cRecordTableModel::_removed(cAlternate *p)
{
    removed(p);
    delete p;
}

/* ******************************************************************************************************************** */
cRecordTableModelSql::cRecordTableModelSql(const cRecordTable& _rt)
    : cRecordTableModel(_rt)
{
    ;
}

cRecordTableModelSql::~cRecordTableModelSql()
{
    clear();
}


int cRecordTableModelSql::setRecords(QSqlQuery& _q, bool _first)
{
    DBGFN();
    extLines.clear();
    q = _q;
    int r = _first ? qFirst() : qView();
    _DBGFNL() << " = " << r << endl;
    return r;
}

int cRecordTableModelSql::qView()
{
    DBGFN();
    int qpos = _firstRowNumber;
    _records.clear();
    beginResetModel();
    int cnt = 0;
    extLines.clear();
    if (q.seek(qpos)) do {
        qlonglong tableoid = variantToId(q.value(0));
        cAlternate *p = NULL;
        if (tableoid == recDescr.tableoid()) {
            p = new cAlternate(&recDescr);
        }
        else {
            p = new cAlternate(&recordTable.inhRecDescr(tableoid));
        }
        p->set(q);
        _records << p;
        ++cnt;
    } while (q.next() && cnt < _maxRows);
    else {
        if (_firstRowNumber > 0 && q.size() > 0) {
            endResetModel();
            return qPrev();
        }
    }
    endResetModel();
    int r = _records.size();
    _DBGFNL() << " = " << r << endl;
    return r;
}

int cRecordTableModelSql::qFirst()
{
    _firstRowNumber = 0;
    return qView();
}

int cRecordTableModelSql::qNext()
{
    _firstRowNumber += _maxRows;
    return qView();
}

int cRecordTableModelSql::qPrev()
{
    if (_firstRowNumber < _maxRows) {
        DWAR() << "Nothing previous page." << endl;
        return -1;
    }
    _firstRowNumber -= _maxRows;
    return qView();
}

int cRecordTableModelSql::qLast()
{
    int s = q.size();
    if (s < 0) {
        DWAR() << "Query size is negative." << endl;
        return -1;
    }
    _firstRowNumber = (s / _maxRows) * _maxRows;
    return qView();
}

cRecordTableModel& cRecordTableModelSql::clear()
{
    q.clear();
    return cRecordTableModel::clear();
}

