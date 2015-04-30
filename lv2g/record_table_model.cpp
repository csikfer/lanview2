#include "record_table_model.h"
#include "record_table.h"
#include "cerrormessagebox.h"

cRecordViewModelBase::cRecordViewModelBase(cRecordViewBase& _rt)
    : _col2shape(), _col2field()
    , recordView(_rt)
    , recDescr(_rt.recDescr())
    , tableShape(_rt.tableShape())
    , columns(_rt.fields)
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
    PDEB(VVERBOSE) << "X tabs : " << tIntVectorToString(_col2field) << QChar(' ') << tIntVectorToString(_col2shape) << endl;
}

cRecordViewModelBase::~cRecordViewModelBase()
{
    delete pq;
}

QVariant cRecordViewModelBase::_headerData(int section, Qt::Orientation orientation, int role) const
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
                r = design().head.fg;
                break;
            case Qt::BackgroundRole:
                r = design().head.bg;
                break;
            case Qt::FontRole:
                r = design().head.font;
                break;
            }
        }
    break;
    }
//    _DBGFNL() << " = " << quotedString(r.toString()) << endl;
    return r;
}

cRecordAny *cRecordViewModelBase::qGetRecord(QSqlQuery& q)
{
    cRecordAny *p = NULL;
    if (q.value(0).isNull()) {
        p = new cRecordAny(&recDescr);
    }
    else {
        qlonglong tableoid = variantToId(q.value(0));
        if (tableoid == recDescr.tableoid()) {
            p = new cRecordAny(&recDescr);
        }
        else {
            p = new cRecordAny(&recordView.inhRecDescr(tableoid));
        }
    }
    p->set(q);
    return p;
}

bool cRecordViewModelBase::updateRec(const QModelIndex& mi, cRecordAny *pRec)
{
    sqlBegin(*pq);
    if (!cErrorMessageBox::condMsgBox(pRec->tryUpdate(*pq, false))) {
        sqlRollback(*pq);
        return false;
    }
    sqlEnd(*pq);
    PDEB(VVERBOSE) << "Update returned : " << pRec->toString() << endl;
    return updateRow(mi, pRec);
}

bool cRecordViewModelBase::insertRec(cRecordAny *pRec)
{
    sqlBegin(*pq);
    if (!cErrorMessageBox::condMsgBox(pRec->tryInsert(*pq))) {
        sqlRollback(*pq);
        return false;
    }
    sqlEnd(*pq);
    PDEB(VVERBOSE) << "Insert returned : " << pRec->toString() << endl;
    return insertRow(pRec);
}


/* ********************************************************************** */

cRecordTableModel::cRecordTableModel(cRecordTable& _rt)
    : QAbstractTableModel(_rt.pWidget())
    , cRecordViewModelBase(_rt)
    , _records()
{
    ;
}

cRecordTableModel::~cRecordTableModel()
{
    clear();
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
    if (row < _records.size() && col < _col2field.size()) {
        int fix = _col2field[col];  // Mező index a rekordbam
        int mix = _col2shape[col];  // Index a leíróban
        // _DBGFN() << VDEB(row) << VDEB(col) << VDEB(role) << endl;
        if (role == Qt::DisplayRole)       return _records.at(row)->view(*pq, fix);
        if (role == Qt::TextAlignmentRole) return columns[mix]->dataAlign;
        const colorAndFont&   cf = _records.at(row)->isNull(fix)
                ?   design().null
                :   design()[columns[mix]->dataRole];
        switch (role) {
        case Qt::ForegroundRole:    return cf.fg;
        case Qt::BackgroundRole:    return cf.bg;
        case Qt::FontRole:          return cf.font;
        }
    }
    return QVariant();
}

QVariant cRecordTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return _headerData(section, orientation, role);
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

void cRecordTableModel::removeRecords(const QModelIndexList &mil)
{
    if (mil.isEmpty()) return;
    int b = QMessageBox::warning(recordView.pWidget(),
                         trUtf8("Kijelölt objektu(mo)k törlése!"),
                         trUtf8("Valóban törölni akarja a kijelölt objektumo(ka)t ?"),
                         QMessageBox::Ok, QMessageBox::Cancel);
    if (b != QMessageBox::Ok) return;
    int s = _records.size();
    QBitArray   rb(s, false);   // Sorismétlések kivédése
    foreach(QModelIndex mi, mil) {
        int row = mi.row();
        if (row < s) {
            rb.setBit(row, true);
        }
    }
    for (int i = s - 1; i >= 0; --i) {   // végigszaladunk a sorokon, visszafelé
        removeRec(index(i, 0));
    }

}

bool cRecordTableModel::removeRec(const QModelIndex &mi)
{
    if (mi.isValid() && isContIx(_records, mi.row())) {
        cRecordAny * p = _records.at(mi.row());
        PDEB(INFO) << "Remove : " << p->toString() << endl;
        sqlBegin(*pq);
        if (cErrorMessageBox::condMsgBox(p->tryRemove(*pq))) {
            return removeRow(mi);
            sqlRollback(*pq);
            recordView.refresh(true);
        }
        else {
            sqlEnd(*pq);
        }
    }
    return false;
}

bool cRecordTableModel::removeRow(const QModelIndex &mi)
{
    int r = mi.row();
    beginRemoveRows(mi.parent(), r, r);
    delete _records.takeAt(r);
    endRemoveRows();
    return true;
}

bool cRecordTableModel::updateRow(const QModelIndex& mi, cRecordAny *pRec)
{
    int row = mi.row();
    if (!isContIx(_records, row)) EXCEPTION(EPROGFAIL);
    beginRemoveRows(QModelIndex(), row, row);
    delete _records.pullAt(row);
    endRemoveColumns();
    beginInsertRows(QModelIndex(), row, row);
    _records.insert(row, pRec);
    return false;
}

bool cRecordTableModel::insertRow(cRecordAny *pRec)
{
    int row = _records.size();
    beginInsertRows(QModelIndex(), row, row);
    _records.insert(row, pRec);
    endInsertRows();
    return true;
}

cRecordTableModel& cRecordTableModel::clear()
{
    beginResetModel();
    _records.clear();
    _firstRowNumber = 0;
    endResetModel();
    q.clear();
    return *this;
}

int cRecordTableModel::setRecords(QSqlQuery& _q, bool _first)
{
    DBGFN();
    q = _q;
    int r = _first ? qFirst() : qView();
    _DBGFNL() << " = " << r << endl;
    return r;
}

int cRecordTableModel::qView()
{
    DBGFN();
    int qpos = _firstRowNumber;
    _records.clear();
    beginResetModel();
    int cnt = 0;
    if (q.seek(qpos)) do {
        _records << qGetRecord(q);
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

int cRecordTableModel::qFirst()
{
    _firstRowNumber = 0;
    return qView();
}

int cRecordTableModel::qNext()
{
    _firstRowNumber += _maxRows;
    return qView();
}

int cRecordTableModel::qPrev()
{
    if (_firstRowNumber < _maxRows) {
        DWAR() << "Nothing previous page." << endl;
        return -1;
    }
    _firstRowNumber -= _maxRows;
    return qView();
}

int cRecordTableModel::qLast()
{
    int s = q.size();
    if (s < 0) {
        DWAR() << "Query size is negative." << endl;
        return -1;
    }
    _firstRowNumber = (s / _maxRows) * _maxRows;
    return qView();
}

