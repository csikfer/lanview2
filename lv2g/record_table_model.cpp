#include "record_table_model.h"
#include "record_table.h"
#include "cerrormessagebox.h"

QString cRecordViewModelBase::sIrrevocable;

cRecordViewModelBase::cRecordViewModelBase(cRecordsViewBase& _rt)
    : _col2shape(), _col2field()
    , recordView(_rt)
    , recDescr(_rt.recDescr())
    , tableShape(_rt.tableShape())
    , columns(_rt.fields)
{
    if (sIrrevocable.isEmpty()) {
        sIrrevocable = QObject::trUtf8("A művelet nem visszavonható.");
    }
    _viewRowNumbers = true;
    _viewHeader     = true;
    _firstRowNumber =   0;
    pq = newQuery();
    _maxRows = (int)tableShape.feature(lv2g::sMaxRows, (qlonglong)lv2g::getInstance()->maxRows);

    int i, n = columns.size();
    for (i = 0; i < n; ++i) {
        const cRecordTableColumn&  column = *columns[i];
        if (column.shapeField.getBool(_sFieldFlags, FF_TABLE_HIDE)) {
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

cRecord *cRecordViewModelBase::qGetRecord(QSqlQuery& q)
{
    cRecord *p = NULL;
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

int cRecordViewModelBase::updateRec(const QModelIndex& mi, cRecord *pRec)
{
    if (!cErrorMessageBox::condMsgBox(pRec->tryUpdate(*pq, false, QBitArray(), QBitArray(), true))) {
        return 0;
    }
    PDEB(VVERBOSE) << "Update returned : " << pRec->toString() << endl;
    return updateRow(mi, pRec) ? 1 : 0;
}

bool cRecordViewModelBase::insertRec(cRecord *pRec)
{
    bool r = SqlInsert(*pq, pRec);
    PDEB(VVERBOSE) << QString("Insert returned : %1; record : %2").arg(r).arg(pRec->toString()) << endl;
    return r && insertRow(pRec);
}

bool cRecordViewModelBase::SqlInsert(QSqlQuery& q, cRecord *pRec)
{
    if (!cErrorMessageBox::condMsgBox(pRec->tryInsert(q, true))) {
        return false;
    }
    return true;
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

bool cRecordViewModelBase::dataColor(const cRecord *pr, int fix, int role, int& dataRole, QVariant& r) const
{
    // Ha nem létezik, nem szines mező, vagy értéke NULL
    if (fix < 0 || 0 == (dataRole & GDR_COLOR) || pr->isNull(fix)) return false;
    const cColStaticDescr& cd = pr->descr().colDescr(fix);
    QString cn;
    switch (cd.eColType) {          // Adat típus
    case cColStaticDescr::FT_TEXT:      // Text = color (az adat maga a szín, csak a háttér)
        if (role == Qt::BackgroundRole) {
            r = QColor(pr->getName(fix));
            return true;
        }
        break;
    case cColStaticDescr::FT_ENUM:        // enum colored (típus, és érték alapján)
        switch (role) {
        case Qt::ForegroundRole: cn = cEnumVal::fgColor(cd.udtName, pr->getName(fix)); break;
        case Qt::BackgroundRole: cn = cEnumVal::bgColor(cd.udtName, pr->getName(fix)); break;
        }
        if (!cn.isEmpty()) {
            r = QColor(cn);
            return true;
        }
        break;
    case cColStaticDescr::FT_BOOLEAN:    // boolean colored (mező teljex név, és az érték alapján)
        switch (role) {     // Eredeti táblanevet használjuk, a mező név az aktuálisból, mivel a fix arra vonatkozik!
        case Qt::ForegroundRole: cn = cEnumVal::fgColor(recDescr.tableName(), pr->descr().columnName(fix), pr->getBool(fix)); break;
        case Qt::BackgroundRole: cn = cEnumVal::bgColor(recDescr.tableName(), pr->descr().columnName(fix), pr->getBool(fix)); break;
        }
        if (!cn.isEmpty()) {
            r = QColor(cn);
            return true;
        }
        break;
    }
    return false;
}

QVariant cRecordTableModel::data(const QModelIndex &index, int role) const
{
    QVariant r;
    if (!index.isValid())           return r;
    int row = index.row();      // Sor index a táblázatban
    if (row >= _records.size())     return r;
    int col = index.column();   // oszlop index a táblázatban
    if (col >= _col2field.size())   return r;
    const cRecord *pr = _records.at(row);
    int fix = _col2field[col];  // Mező index a (fő)rekordbam
    int mix = _col2shape[col];  // Index a leíróban (shape)
    if (recDescr != pr->descr()) { // A mező sorrend nem feltétlenül azonos (öröklés)
        const QString& fn = recDescr.columnName(fix);
        fix = pr->toIndex(fn, EX_IGNORE);   // Nem biztos, hogy van ilyen mező (ős)
    }
    // _DBGFN() << VDEB(row) << VDEB(col) << VDEB(role) << endl;
    int dataRole = columns[mix]->dataRole;
    switch (role) {
    case Qt::DisplayRole:       return pr->view(*pq, fix);
    case Qt::TextAlignmentRole: return columns[mix]->dataAlign;
    case Qt::ForegroundRole:
    case Qt::BackgroundRole:    if (dataColor(pr, fix, role, dataRole, r)) return r;
    case Qt::FontRole:          break;
    default:                    return r;
    }
    const colorAndFont&   cf = pr->isNull(fix)
            ?   design().null
            :   design()[dataRole];
    switch (role) {
    case Qt::ForegroundRole:    return cf.fg;
    case Qt::BackgroundRole:    return cf.bg;
    case Qt::FontRole:          return cf.font;
    }
    return QVariant();
}

QVariant cRecordTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::ForegroundRole) {
        int mix = _col2shape[section];  // Index a leíróban (shape)
        cRecordTable& rt = (cRecordTable&)recordView;
        if (rt.enabledBatchEdit(*rt.pTableShape->shapeFields[mix])) {
            return QColor(Qt::blue);
        }
    }
    return _headerData(section, orientation, role);
}

cRecord *cRecordTableModel::record(const QModelIndex &index)
{
    if (index.isValid()) {
        int row = index.row();      // Sor index a táblázatban
        if (row < _records.size()) {
            return _records.at(row);
        }
    }
    return NULL;
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

QBitArray cRecordTableModel::index2map(const QModelIndexList &mil)
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

void cRecordTableModel::removeRecords(const QModelIndexList &mil)
{
    QBitArray   rb = index2map(mil);
    if (rb.count(true) == 0) return;
    int b = QMessageBox::warning(recordView.pWidget(),
                         trUtf8("Kijelölt objektu(mo)k törlése!"),
                         trUtf8("Valóban törölni akarja a kijelölt objektumo(ka)t ?\n") + sIrrevocable,
                         QMessageBox::Ok, QMessageBox::Cancel);
    if (b != QMessageBox::Ok) return;
    int s = rb.size();    // Az összes rekord száma
    for (int i = s - 1; i >= 0; --i) {   // végigszaladunk a sorokon, visszafelé
        if (rb[i]) removeRec(index(i, 0));
    }

}

bool cRecordTableModel::removeRec(const QModelIndex &mi)
{
    if (mi.isValid() && isContIx(_records, mi.row())) {
        cRecord * p = _records.at(mi.row());
        PDEB(INFO) << "Remove : " << p->toString() << endl;
        if (cErrorMessageBox::condMsgBox(p->tryRemove(*pq, false, QBitArray(), true))) {
            return removeRow(mi);
        }
        else {
            recordView.refresh(true);
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

bool cRecordTableModel::updateRow(const QModelIndex& mi, cRecord *pRec)
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

bool cRecordTableModel::insertRow(cRecord *pRec)
{
    int row = _records.size();
    beginInsertRows(QModelIndex(), row, row);
    _records.pushAt(row, pRec);
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
    dataReloded(_records);
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

