#include "record_link_model.h"
#include "record_link.h"

cRecordLinkModel::cRecordLinkModel(cRecordLink& _rt)
    : QAbstractTableModel(_rt.pWidget())
    , cRecordViewModelBase(_rt)
    , _records()
{
    ;
}

cRecordLinkModel::~cRecordLinkModel()
{
    clear();
}

int cRecordLinkModel::rowCount(const QModelIndex &) const
{
    return _records.size();
}

int cRecordLinkModel::columnCount(const QModelIndex &) const
{
    return _col2field.size();
}

QVariant cRecordLinkModel::data(const QModelIndex &index, int role) const
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

QVariant cRecordLinkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return _headerData(section, orientation, role);
}

bool cRecordLinkModel::setRecords(const tRecords& __recs, int _firstNm)
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

QBitArray cRecordLinkModel::index2map(const QModelIndexList &mil)
{
    if (mil.isEmpty()) return QBitArray();
    int s = _records.size();    // Az összes rekord
    QBitArray   rb(s, false);   // Sorismétlések kivédése
    foreach(QModelIndex mi, mil) {
        int row = mi.row();
        if (row < s) {
            rb.setBit(row, true);
        }
    }
    return rb;
}

void cRecordLinkModel::removeRecords(const QModelIndexList &mil)
{
    QBitArray   rb = index2map(mil);
    if (rb.count(true) == 0) return;
    int b = QMessageBox::warning(recordView.pWidget(),
                         trUtf8("Kijelölt objektu(mo)k törlése!"),
                         trUtf8("Valóban törölni akarja a kijelölt objektumo(ka)t ?"),
                         QMessageBox::Ok, QMessageBox::Cancel);
    if (b != QMessageBox::Ok) return;
    int s = rb.size();    // Az összes rekord száma
    for (int i = s - 1; i >= 0; --i) {   // végigszaladunk a sorokon, visszafelé
        if (rb[i]) removeRec(index(i, 0));
    }

}

bool cRecordLinkModel::removeRec(const QModelIndex &mi)
{
    if (mi.isValid() && isContIx(_records, mi.row())) {
        cRecordAny * p = _records.at(mi.row());
        PDEB(INFO) << "Remove : " << p->toString() << endl;
        sqlBegin(*pq);
        if (cErrorMessageBox::condMsgBox(p->tryRemove(*pq))) {
            sqlEnd(*pq);
            return removeRow(mi);
        }
        else {
            sqlRollback(*pq);
            recordView.refresh(true);
        }
    }
    return false;
}

bool cRecordLinkModel::removeRow(const QModelIndex &mi)
{
    int r = mi.row();
    beginRemoveRows(mi.parent(), r, r);
    delete _records.takeAt(r);
    endRemoveRows();
    return true;
}

bool cRecordLinkModel::updateRow(const QModelIndex& mi, cRecordAny *pRec)
{
    int row = mi.row();
    if (!isContIx(_records, row)) EXCEPTION(EPROGFAIL);
    beginRemoveRows(QModelIndex(), row, row);
    delete _records.pullAt(row);
    endRemoveRows();
    beginInsertRows(QModelIndex(), row, row);
    _records.pushAt(row, pRec);
    endInsertRows();
    return true;
}

bool cRecordLinkModel::insertRow(cRecordAny *pRec)
{
    int row = _records.size();
    beginInsertRows(QModelIndex(), row, row);
    _records.pushAt(row, pRec);
    endInsertRows();
    return true;
}

cRecordLinkModel& cRecordLinkModel::clear()
{
    beginResetModel();
    _records.clear();
    _firstRowNumber = 0;
    endResetModel();
    q.clear();
    return *this;
}

int cRecordLinkModel::setRecords(QSqlQuery& _q, bool)
{
    DBGFN();
    q = _q;
    int r = qView();
    _DBGFNL() << " = " << r << endl;
    return r;
}

int cRecordLinkModel::qView()
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
    endResetModel();
    int r = _records.size();
    _DBGFNL() << " = " << r << endl;
    return r;
}

