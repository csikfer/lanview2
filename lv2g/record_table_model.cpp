#include "record_table_model.h"
#include "record_table.h"
#include "cerrormessagebox.h"
#include "popupreport.h"
#include "export.h"

bool isRowSelected(int row, const QModelIndexList& selected, bool retIfEmpty)
{
    if (selected.isEmpty()) return retIfEmpty;
    foreach (QModelIndex mi, selected) {
        if (mi.row() == row) return true;
    }
    return false;
}


QString cRecordViewModelBase::sIrrevocable;

cRecordViewModelBase::cRecordViewModelBase(cRecordsViewBase& _rt)
    : _col2field()
    , recordView(_rt)
    , recDescr(_rt.recDescr())
    , tableShape(_rt.tableShape())
    , columns(_rt.fields)
{
    if (sIrrevocable.isEmpty()) {
        sIrrevocable = QObject::tr("A művelet nem visszavonható.");
    }
    lineBgColorEnum2Ix = lineBgColorEnumIx = NULL_IX;  // Nincs teljes sor háttérszinezés
    QString colorFildName = tableShape.feature(_sBgColor);
    if (!colorFildName.isEmpty()) {
        QStringList sl = colorFildName.split(QChar(','));
        colorFildName = sl.takeFirst();
        if (!colorFildName.isEmpty()) {    // Teljes sor háttérszíne a megadott mező (enum!) szerint
            lineBgColorEnumIx = recDescr.toIndex(colorFildName, EX_IGNORE);
            if (lineBgColorEnumIx >= 0) {
                if (recDescr.colDescr(lineBgColorEnumIx).eColType == cColStaticDescr::FT_ENUM) {        // Enum ?
                    lineBgColorEnumType = recDescr.colDescr(lineBgColorEnumIx).enumType();
                }
                else if (recDescr.colDescr(lineBgColorEnumIx).eColType == cColStaticDescr::FT_BOOLEAN) {// Boolean ?
                    lineBgColorEnumType = mCat(recDescr.tableName(), recDescr.columnName(lineBgColorEnumIx));
                }
                else {  // Ha nem enum, vagy boolean, akkor figyelmen kívül hagyjuk
                    lineBgColorEnumIx = NULL_IX;
                    DWAR() << QObject::tr("Invalid field type (%1), Shape : %2").arg(_sBgColor + "=" + colorFildName, tableShape.identifying()) << endl;
                }
                // Fellülbíráló szín, egy másik mező kitüntetett értékére
                if (lineBgColorEnumIx >= 0 && !sl.isEmpty()) {
                    lineBgColorEnum2Ix = recDescr.toIndex(sl.first(), EX_IGNORE);
                    if (lineBgColorEnum2Ix >= 0) {
                        if (recDescr.colDescr(lineBgColorEnum2Ix).eColType == cColStaticDescr::FT_ENUM) {        // Enum ?
                            lineBgColorEnum2Type = recDescr.colDescr(lineBgColorEnum2Ix).enumType();
                            if (sl.size() > 2) lineBgColorEnum2Val = cColEnumType::getByName(lineBgColorEnum2Type).str2enum(sl.at(1));
                            else               lineBgColorEnum2Val = 0;
                        }
                        else if (recDescr.colDescr(lineBgColorEnum2Ix).eColType == cColStaticDescr::FT_BOOLEAN) {// Boolean ?
                            lineBgColorEnum2Type = mCat(recDescr.tableName(), recDescr.columnName(lineBgColorEnum2Ix));
                            if (sl.size() > 2) lineBgColorEnum2Val = str2bool(sl.at(1), EX_IGNORE) ? 1 : 0;
                            else               lineBgColorEnum2Val = 1;
                        }
                        else {  // Ha nem enum, vagy boolean, akkor figyelmen kívül hagyjuk
                            lineBgColorEnum2Ix = NULL_IX;
                            DWAR() << QObject::tr("Invalid field type (%1), Shape : %2").arg(_sBgColor + "=" + sl.first(), tableShape.identifying()) << endl;
                        }
                    }
                }
            }
            else {
                DWAR() << QObject::tr("Invalid field name (%1), Shape : %2").arg(_sBgColor + "=" + colorFildName, tableShape.identifying()) << endl;
            }
        }
    }
    _viewRowNumbers = true;
    _viewHeader     = true;
    _firstRowNumber =   0;
    pq = newQuery();
    _maxRows = int(tableShape.feature(lv2g::sMaxRows, lv2g::getInstance()->maxRows));

    int i, n = columns.size();
    for (i = 0; i < n; ++i) {
        const cRecordTableColumn&  column = *columns[i];
        if (column.fieldFlags & ENUM2SET(FF_TABLE_HIDE)) {   // Ha nem kel megjeleníteni
            // PDEB(VVERBOSE) << "Hidden field : " << columns[i]->shapeField.getName(_sTableShapeFieldName) << endl;
            recordView.hideColumn(i);   // Ez itt valamiért hatástalan, nem tüntet el egyetlen oszlopot sem.
        }
        _col2field << column.fieldIndex;
    }
    PDEB(VVERBOSE) << "X tab : " << tIntVectorToString(_col2field) << endl;
}

cRecordViewModelBase::~cRecordViewModelBase()
{
    delete pq;
}

/// @param fix      Mező index az aktuális rekordban
/// @param column   Oszlop megjelenítését vezérlő objektum
/// @param pr       Az aktuális rekordra mutató pointer
/// @param role     A hívó eredeti QTableModel::data() role paraméter
/// @”aram isTextId Igaz, ha az aktuális rekordban az aktuális mező egy text_id mező
QVariant cRecordViewModelBase::_data(int fix, cRecordTableColumn& column, const cRecord *pr, int role, bool isTextId) const
{
    qlonglong& ff = column.fieldFlags;
    QString s, et;
    if (column.isImage != cRecordTableColumn::IS_NOT_IMAGE) {
        if (role == Qt::DecorationRole) {
            switch (column.isImage) {
            case cRecordTableColumn::IS_IMAGE: {
                qlonglong id;
                cImage  image;
                QPixmap pix;
                id = pr->getId(fix);
                image.setById(*pq, id);
                if (pixmap(image, pix)) return QVariant(pix);
                return QVariant();
              }
            case cRecordTableColumn::IS_ICON:
            case cRecordTableColumn::IS_ICON_NAME:
                if (cRecordTableColumn::IS_ICON == column.isImage) {
                    qlonglong e = pr->getId(fix);
                    const cEnumVal *pev = nullptr;
                    if (e != NULL_ID) {
                        et = column.enumTypeName;
                        pev = &cEnumVal::enumVal(et, int(e), EX_IGNORE);
                    }
                    else {
                        pev = &cEnumVal::enumVal(_sDatacharacter, DC_NULL, EX_IGNORE);
                    }
                    s  = pev->getName(_sIcon);
                }
                else {  // IS_ICON_NAME
                    s = pr->getName(fix);
                }
                return QVariant(resourceIcon(s));
            default:
                EXCEPTION(EPROGFAIL);
            }
        }
        else if (role == Qt::DisplayRole && (ff & ENUM2SET(FF_NOTEXT))) {
            return QVariant();
        }
    }
    if (isTextId) fix = column.textIndex;
    //  Háttér szín                   az egész sorra              erre a mezőre saját
    if (Qt::BackgroundRole == role && 0 <= lineBgColorEnumIx && !(ff & ENUM2SET(FF_BG_COLOR))) {
        QColor c;
        if (0 <= lineBgColorEnum2Ix && pr->isIndex(lineBgColorEnum2Ix) && lineBgColorEnum2Val == pr->getId(lineBgColorEnum2Ix)) {
            c = bgColorByEnum(lineBgColorEnum2Type, lineBgColorEnum2Val);
        }
        else {
            qlonglong e = NULL_ID;
            if (pr->isIndex(lineBgColorEnumIx)) e = pr->getId(lineBgColorEnumIx);
            if (e == NULL_ID) {
                c = dcBgColor(DC_NULL);
            }
            else {
                c = bgColorByEnum(lineBgColorEnumType, int(e));
            }
        }
        return QVariant(c);
    }
    if (fix < 0) {          // A mező nem létezik, vagy text_id
        return dcRole(DC_HAVE_NO, role);
    }
    if (!isTextId && pr->isNull(fix)) {  // A mező értéke NULL
        return dcRole(DC_NULL,    role);
    }
    // Az opcionális mező dekorációs típus neve (vagy egy " = " string)
    et = column.enumTypeName;
    if (et == _sEquSp) {    // A mező tartalma a szín
        QString c = pr->getName(fix);
        if (!isTextId && !c.isEmpty()) switch (role) {
        case Qt::ForegroundRole:
        case Qt::BackgroundRole:
            return QColor(c);
        case Qt::DisplayRole:
            return QVariant();
        }
        et.clear();
    }
    if (et.isEmpty()) {
        if (role == Qt::DisplayRole) {
            if (isTextId) {
                return pr->getText(fix);
            }
            else {
                return column.shapeField.view(*pq, *pr, fix);
            }
        }
        return dcRole(column.dataCharacter, role);
    }
    int        id = int(pr->getId(fix));
    int        dd = column.dataCharacter;
    switch (role) {
    case Qt::ForegroundRole: return (ff & ENUM2SET(FF_FG_COLOR)) ? fgColorByEnum(et, id) : dcFgColor(dd);
    case Qt::BackgroundRole:return (ff & ENUM2SET(FF_BG_COLOR)) ? bgColorByEnum(et, id) : dcBgColor(dd);
    case Qt::DisplayRole:   return cEnumVal::viewShort(et, id, pr->view(*pq, fix));
    case Qt::ToolTipRole:   return (ff & ENUM2SET(FF_TOOL_TIP)) ? cEnumVal::toolTip(et, id) : QVariant();
    case Qt::FontRole:      return (ff & ENUM2SET(FF_FONT))     ? fontByEnum(et, id) : dcFont(dd);
    default:                return QVariant();
    }
}

QVariant cRecordViewModelBase::_headerData(const QAbstractItemModel *pModel, int section, Qt::Orientation orientation, int role) const
{
//    _DBGFN() << VDEB(section) << VDEB(orientation) << VDEB(role) << endl;
    QVariant r;
    switch (orientation) {
    case Qt::Vertical:
        switch (role) {
        case Qt::DisplayRole:
            if (_viewRowNumbers) return QVariant(section + _firstRowNumber +1) ;
            return r;
        case Qt::TextAlignmentRole:
        case Qt::ForegroundRole:
        case Qt::BackgroundRole:
        case Qt::FontRole:
            break;
        }
        break;
    case Qt::Horizontal: {
        const cRecordTableColumn& column = *columns[section];
        switch (role) {
        case Qt::DisplayRole:    return column.headerText;
        case Qt::DecorationRole: return column.headerIcon;
        case Qt::ToolTipRole:    return column.headerToolTyp;
        default:                 return dcRole(DC_HEAD, role);
        }
    }
    }
//    _DBGFNL() << " = " << quotedString(r.toString()) << endl;
    r = pModel->QAbstractItemModel::headerData(section, orientation, role);
    return r;
}

cRecord *cRecordViewModelBase::qGetRecord(QSqlQuery& q)
{
    cRecord *p = nullptr;
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
    if (!cErrorMessageBox::condMsgBox(pRec->tryUpdateById(*pq, TS_NULL, true))) {
        return 0;
    }
    PDEB(VVERBOSE) << "Update returned : " << pRec->toString() << endl;
    return updateRow(mi, pRec) ? 1 : 0;
}

bool cRecordViewModelBase::insertRec(cRecord *pRec)
{
    bool r = SqlInsert(*pq, pRec);  // Insert, if error, view error message box
    PDEB(VVERBOSE) << QString("Insert returned : %1; record : %2").arg(r).arg(pRec->toString()) << endl;
    if (r && !insertRow(pRec)) {    // Insert ok and view error
        cMsgBox::warning(QObject::tr("A megjelenítés frissítése sikertelen, kérem frissítse a megjelenítést!"), recordView.pWidget());
    }
    return r;
}

bool cRecordViewModelBase::SqlInsert(QSqlQuery& q, cRecord *pRec)
{
    if (!cErrorMessageBox::condMsgBox(pRec->tryInsert(q, TS_NULL, true))) {
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

}

cRecordTableModel::~cRecordTableModel()
{
    // clear();
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
    int fix = _col2field[col];  // Mező index a (fő)rekordban
    cRecordTableColumn * pColumn = columns[col];
    bool bTxt = pColumn->textIndex != NULL_IX;
    if (!bTxt && recDescr != pr->descr()) { // A mező sorrend nem feltétlenül azonos (öröklés miatt)
        const QString& fn = recDescr.columnName(fix);
        fix = pr->toIndex(fn, EX_IGNORE);   // Nem biztos, hogy van ilyen mező (ős)
    }
    return _data(fix, *columns[col], pr, role, bTxt);
}

QVariant cRecordTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::ForegroundRole) {
        cRecordTable& rt = static_cast<cRecordTable&>(recordView);
        if (rt.enabledBatchEdit(*rt.pTableShape->shapeFields[section])) {
            return QColor(Qt::blue);
        }
    }
    return _headerData(this, section, orientation, role);
}

cRecord *cRecordTableModel::record(const QModelIndex &index)
{
    if (index.isValid()) {
        int row = index.row();      // Sor index a táblázatban
        if (row < _records.size()) {
            return _records.at(row);
        }
    }
    return nullptr;
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
    QString text = tr("Valóban törölni akarja a kijelölt objektumo(ka)t ?\n") + sIrrevocable;
    QString detailedText = tr("\nTörlésre kijelölt objektu(mok) :");
    int s = rb.size();    // Az összes rekord száma
    for (int i = 0; i < s; ++i) {   // végigszaladunk a sorokon
        if (rb[i]) {
            cRecord * p = _records.at(i);
            detailedText += tr("\n %1 tábla : ").arg(p->tableName()) + p->identifying(false);
        }
    }
     if (mil.size() < 10) {
        text += detailedText;
        detailedText = _sNul;
    }
    if (!cMsgBox::yes(text, recordView.pWidget(), detailedText)) return;
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

void cRecordTableModel::clear()
{
    beginResetModel();
    _records.clear();
    _firstRowNumber = 0;
    endResetModel();
    q.clear();
}

QString cRecordTableModel::toCSV() const
{
    cCommaSeparatedValues csv;
    foreach (QStringList row, toStringTable()) {
        foreach (QString cel, row) {
            csv << cel;
        }
        csv << endl;
    }
    return csv.toString();
}

QString cRecordTableModel::toHtml() const
{
    QString r;
    QList<QStringList>  m = toStringTable(false);
    QStringList head = m.takeFirst();
    r = htmlTable(head, m, false);
    return sHtmlHead + r + sHtmlTail;
}

static QString htmlImage(const QVariant& vImage, const QString& alt)
{
    if (vImage.isValid()) {
        QImage image;
        switch (vImage.userType()) {
        case QMetaType::QPixmap:
            image = vImage.value<QPixmap>().toImage();
            break;
        case QMetaType::QIcon:
            image = vImage.value<QIcon>().pixmap(24).toImage();
            break;
        }
        if (!image.isNull()) {
            QBuffer b;
            b.open(QIODevice::WriteOnly);
            image.save(&b, __sPNG);
            QByteArray b64 = b.data().toBase64();
            static const QString img =
                    "<img src=\"data:image/png;base64,%1\""
                         "alt=\"%2\" />";
            return img.arg(b64, alt);
        }
    }
    return alt;
}

QList<QStringList>  cRecordTableModel::toStringTable(bool raw, const QModelIndexList& mil) const
{
    int rownums = rowCount(QModelIndex());
    int colnums = columnCount(QModelIndex());
    int col, row;
    QString s;
    QList<int> selectedRows;
    foreach (QModelIndex mi, mil) {
        row = mi.row();
        if (!selectedRows.contains(row)) selectedRows << row;
    }
    const tRecordTableColumns& columns = recordView.fields;
    QList<QStringList> r;
    QStringList lrow;
    // Header
    for (col = 0; col <colnums; ++col) {
        if (columns.at(col)->fieldFlags & ENUM2SET(FF_HTML)) {
            s = headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
            if (!raw) {
                s = htmlImage(headerData(col, Qt::Horizontal, Qt::DecorationRole), s);
                s = toHtmlBold(s, true, false);
            }
            lrow << s;
        }
    }
    r << lrow;
    // Table content
    for (row = 0; row < rownums; ++row) {
        if (selectedRows.isEmpty() || selectedRows.contains(row)) {
            lrow.clear();
            for (col = 0; col <colnums; ++col) {
                if (columns.at(col)->fieldFlags & ENUM2SET(FF_HTML)) {
                    QModelIndex mi = index(row, col);
                    s = data(mi, Qt::DisplayRole).toString();
                    if (!raw) { // STYLE
                        s = ::toHtml(s);
                        s = htmlImage(data(mi, Qt::DecorationRole), s);
                        QVariant vFgColor = data(mi, Qt::ForegroundRole);
                        QVariant vBgColor = data(mi, Qt::BackgroundRole);
                        if (vFgColor.isValid() || vBgColor.isValid()) {
                            QString style;
                            if (vFgColor.isValid()) {
                                style += "color:" + vFgColor.value<QColor>().name() + _sSemicolon;
                            }
                            if (vBgColor.isValid()) {
                                style += "background-color:" + vBgColor.value<QColor>().name() + _sSemicolon;
                            }
                            s = "<div style=\"" + style + "\">" + s + "</div>";
                        }
                        QVariant vFont    = data(mi, Qt::FontRole);
                        if (vFont.isValid()) {
                            QFont f = vFont.value<QFont>();
                            if (f.italic())     s = toHtmlItalic(s, false, false);
                            if (f.bold())       s = htmlBold(s, false, false);
                            if (f.strikeOut())  s = toHtmlStrikethrough(s, false, false);
                            if (f.underline())  s = toHtmlUnderline(s, false, false);
                        }
                    }
                    lrow << s;
                }
            }
            r << lrow;
        }
    }
    return r;
}

QString cRecordTableModel::toCSV(const QModelIndexList &mil) const
{
    cCommaSeparatedValues csv;
    foreach (QStringList row, toStringTable(true, mil)) {
        foreach (QString cel, row) {
            csv << cel;
        }
        csv << endl;
    }
    return csv.toString();
}

QString cRecordTableModel::toHtml(const QModelIndexList& mil) const
{
    QString r;
    QList<QStringList>  m = toStringTable(false, mil);
    QStringList head = m.takeFirst();
    r = htmlTable(head, m, false);
    return sHtmlHead + r + sHtmlTail;
}

QString cRecordTableModel::toImport(const QModelIndexList& mil) const
{
    QString r;
    int i, n = _records.size();
    cExport e;
    QSqlQuery q = getQuery();
    for (i = 0; i < n; ++i) {
        if (!isRowSelected(i, mil, true)) continue;
        cRecord& rec = *_records.at(i);
        r += e.exportObject(q, rec);
    }

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

