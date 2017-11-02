#include "record_table_model.h"
#include "record_table.h"
#include "cerrormessagebox.h"
#include "report.h"

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
    lineBgColorEnumIx = NULL_IX;  // Nincs teljes sor háttérszinezés
    QString fn = tableShape.feature(_sBgColor);
    if (!fn.isEmpty()) {    // Teljes sor háttérszíne a megadott mező (enum!) szerint
        lineBgColorEnumIx = recDescr.toIndex(fn, EX_IGNORE);
        if (lineBgColorEnumIx >= 0) {
            if (recDescr.colDescr(lineBgColorEnumIx).eColType == cColStaticDescr::FT_ENUM) {        // Enum ?
                lineBgColorEnumType = recDescr.colDescr(lineBgColorEnumIx).enumType();
            }
            else if (recDescr.colDescr(lineBgColorEnumIx).eColType == cColStaticDescr::FT_BOOLEAN) {// Boolean ?
                lineBgColorEnumType = mCat(recDescr.tableName(), recDescr.columnName(lineBgColorEnumIx));
            }
            else {  // Ha nem enum, vagy boolean, akkor figyelmen kívül hagyjuk
                lineBgColorEnumIx = NULL_IX;
                DWAR() << QObject::trUtf8("Invalid field type (%1), Shape : %2").arg(_sBgColor + "=" + fn, tableShape.identifying()) << endl;
            }
        }
        else {
            DWAR() << QObject::trUtf8("Invalid field name (%1), Shape : %2").arg(_sBgColor + "=" + fn, tableShape.identifying()) << endl;
        }
    }
    _viewRowNumbers = true;
    _viewHeader     = true;
    _firstRowNumber =   0;
    pq = newQuery();
    _maxRows = (int)tableShape.feature(lv2g::sMaxRows, (qlonglong)lv2g::getInstance()->maxRows);

    int i, n = columns.size();
    for (i = 0; i < n; ++i) {
        const cRecordTableColumn&  column = *columns[i];
        if (column.shapeField.getBool(_sFieldFlags, FF_TABLE_HIDE)) {   // Ha aem kel megjeleníteni
            PDEB(VVERBOSE) << "Hidden field : " << columns[i]->shapeField.getName(_sTableShapeFieldName) << endl;
            continue;
        }
        _col2shape << i;
        _col2field << column.fieldIndex;
        PDEB(VVERBOSE) << "Visible field : " << column.shapeField.getName(_sTableShapeFieldName) << endl;
    }
    PDEB(VVERBOSE) << "X tabs : " << tIntVectorToString(_col2field) << QChar(' ') << tIntVectorToString(_col2shape) << endl;
}

cRecordViewModelBase::~cRecordViewModelBase()
{
    delete pq;
}

QVariant cRecordViewModelBase::_data(int fix, cRecordTableColumn& column, const cRecord *pr, int role, bool bTxt) const
{
    qlonglong& ff = column.fieldFlags;
    if (bTxt) fix = column.textIndex;
    //  Háttér szín                   az egész sorra              erre a mezőre saját
    if (Qt::BackgroundRole == role && 0 <= lineBgColorEnumIx && !(ff & ENUM2SET(FF_BG_COLOR))) {
        return bgColorByEnum(lineBgColorEnumType, pr->getId(lineBgColorEnumIx));
    }
    if (fix < 0) {          // A mező nem létezik, vagy text_id
        return dcRole(DC_HAVE_NO, role);
    }
    if (!bTxt && pr->isNull(fix)) {  // A mező értéke NULL
        return dcRole(DC_NULL,    role);
    }
    const QString& et = column.enumTypeName;
    if (et.isEmpty()) {
        if (role == Qt::DisplayRole) return bTxt ? pr->getText(fix) : pr->view(*pq, fix);
        return dcRole(column.dataCharacter, role);
    }
    int        id = (int)pr->getId(fix);
    int        dd = column.dataCharacter;
    switch (role) {
    case Qt::TextColorRole: return (ff & ENUM2SET(FF_FG_COLOR)) ? fgColorByEnum(et, id) : dcFgColor(dd);
    case Qt::BackgroundRole:return (ff & ENUM2SET(FF_BG_COLOR)) ? bgColorByEnum(et, id) : dcBgColor(dd);
    case Qt::DisplayRole:   return cEnumVal::viewShort(et, id, pr->view(*pq, fix));
    case Qt::ToolTipRole:   return (ff & ENUM2SET(FF_TOOL_TIP)) ? cEnumVal::toolTip(et, id) : QVariant();
    case Qt::FontRole:      return (ff & ENUM2SET(FF_FONT))     ? fontByEnum(et, id) : dcFont(dd);
    default:                return QVariant();
    }
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
            default:
                return dcRole(DC_HEAD, role);
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
    if (recDescr.textIdIndex(EX_IGNORE) > 0) {
        QSqlQuery q = getQuery();
        p->fetchText(q);
    }
    return p;
}

int cRecordViewModelBase::updateRec(const QModelIndex& mi, cRecord *pRec)
{
    if (!cErrorMessageBox::condMsgBox(pRec->tryUpdateById(*pq))) {
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
    if (!cErrorMessageBox::condMsgBox(pRec->tryInsert(q))) {
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
    cRecordTableColumn * pColumn = columns[mix];
    bool bTxt = pColumn->textIndex != NULL_IX;
    if (!bTxt && recDescr != pr->descr()) { // A mező sorrend nem feltétlenül azonos (öröklés miatt)
        const QString& fn = recDescr.columnName(fix);
        fix = pr->toIndex(fn, EX_IGNORE);   // Nem biztos, hogy van ilyen mező (ős)
    }
    return _data(fix, *columns[mix], pr, role, bTxt);
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
    QString text = trUtf8("Valóban törölni akarja a kijelölt objektumo(ka)t ?\n") + sIrrevocable;
    text += trUtf8("\nTörlésre kijelölt objektu(mok) :");
    int s = rb.size();    // Az összes rekord száma
    for (int i = 0; i < s; ++i) {   // végigszaladunk a sorokon
        if (rb[i]) {
            cRecord * p = _records.at(i);
            text += trUtf8("\n %1 tábla : ").arg(p->tableName()) + p->identifying(false);
        }
    }

    int b = QMessageBox::warning(recordView.pWidget(),
                         trUtf8("Kijelölt objektu(mo)k törlése!"),
                         text,
                         QMessageBox::Ok, QMessageBox::Cancel);
    if (b != QMessageBox::Ok) return;
    for (int i = s - 1; i >= 0; --i) {   // végigszaladunk a sorokon, visszafelé
        if (rb[i]) removeRec(index(i, 0));
    }

}

bool cRecordTableModel::removeRec(const QModelIndex &mi)
{
    if (mi.isValid() && isContIx(_records, mi.row())) {
        cRecord * p = _records.at(mi.row());
        PDEB(INFO) << "Remove : " << p->toString() << endl;
        if (cErrorMessageBox::condMsgBox(p->tryRemove(*pq, false, QBitArray()))) {
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

QList<QStringList>  cRecordTableModel::toStringTable()
{
    QList<QStringList> r;
    int rows    = rowCount(QModelIndex());
    int columns = columnCount(QModelIndex());
    int col, row;
    QStringList lrow;
    for (col = 0; col <columns; ++col) {
        lrow << headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
    }
    r << lrow;
    for (row = 0; row < rows; ++row) {
        lrow.clear();
        for (col = 0; col <columns; ++col) {
            QModelIndex mi = index(row, col);
            lrow << data(mi, Qt::DisplayRole).toString();
        }
        r << lrow;
    }
    return r;
}

QString             cRecordTableModel::toCSV()
{
    QString r;
    foreach (QStringList row, toStringTable()) {
        foreach (QString cel, row) {
            r += quotedString(cel) + ';';
        }
        r.chop(1);
        r += '\n';
    }
    r.chop(1);
    return r;
}

QString             cRecordTableModel::toHtml()
{
    QString r;
    QList<QStringList>  m = toStringTable();
    QStringList head = m.takeFirst();
    r = htmlTable(head, m);
    return r;
}

QList<QStringList>  cRecordTableModel::toStringTable(QModelIndexList mil)
{
    int rows    = rowCount(QModelIndex());
    int columns = columnCount(QModelIndex());
    int col, row;
    QList<int> selectedRows;
    foreach (QModelIndex mi, mil) {
        row = mi.row();
        if (!selectedRows.contains(row)) selectedRows << row;
    }
    QList<QStringList> r;
    QStringList lrow;
    for (col = 0; col <columns; ++col) {
        lrow << headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
    }
    r << lrow;
    for (row = 0; row < rows; ++row) {
        if (!selectedRows.contains(row)) continue;
        lrow.clear();
        for (col = 0; col <columns; ++col) {
            QModelIndex mi = index(row, col);
            lrow << data(mi, Qt::DisplayRole).toString();
        }
        r << lrow;
    }
    return r;
}

QString             cRecordTableModel::toCSV(QModelIndexList mil)
{
    QString r;
    foreach (QStringList row, toStringTable(mil)) {
        foreach (QString cel, row) {
            r += quotedString(cel) + ';';
        }
        r.chop(1);
        r += '\n';
    }
    r.chop(1);
    return r;
}

QString             cRecordTableModel::toHtml(QModelIndexList mil)
{
    QString r;
    QList<QStringList>  m = toStringTable(mil);
    QStringList head = m.takeFirst();
    r = htmlTable(head, m);
    return r;
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

