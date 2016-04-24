
#include "record_link.h"

cRecordLink::cRecordLink(const QString& _mn, bool _isDialog, QWidget * par)
    : cRecordsViewBase(_isDialog, par)
{
    pTableShape = new cTableShape();
    if (!pTableShape->fetchByName(*pq, _mn)) EXCEPTION(EDATA,-1,_mn);
    initShape();
    init();
}

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordsViewBase(_isDialog, par)
{
    pMaster = pUpper = _upper;
    if (pMaster != NULL && pUpper->pMaster != NULL) pMaster = pUpper->pMaster;
    initShape(pts);
    init();
}


cRecordLink::~cRecordLink()
{
    ;
}

void cRecordLink::init()
{
    pTableView = NULL;
    // Az alapértelmezett gombok:
    buttons << DBT_SPACER << DBT_REFRESH << DBT_SPACER << DBT_DELETE << DBT_MODIFY;
    flags = 0;
    if (pUpper != NULL) EXCEPTION(EDATA);
    flags = RTF_LINK;
    initSimple(_pWidget);
    pTableView->horizontalHeader()->setSectionsClickable(true);
    connect(pTableView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(clickedHeader(int)));
    connect(pTableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(modifyByIndex(QModelIndex)));
    refresh();
}

/// Inicializálja a táblázatos megjelenítést
void cRecordLink::initSimple(QWidget * pW)
{
    pButtons    = new cDialogButtons(buttons);
    pMainLayout = new QVBoxLayout(pW);
    pTableView  = new QTableView();
    pModel      = new cRecordLinkModel(*this);
    QString title = pTableShape->getName(_sTableTitle);
    if (title.size() > 0) {
        QLabel *pl = new QLabel(title);
        pMainLayout->addWidget(pl);
    }
    pMainLayout->addWidget(pTableView);
    pMainLayout->addWidget(pButtons->pWidget());
    pTableView->setModel(pTableModel());

    pTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    pTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    pTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
    connect(pTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    if (pFODialog != NULL) EXCEPTION(EPROGFAIL);
    pFODialog = new cRecordTableFODialog(pq, *this);
}

void cRecordLink::empty()
{
    pTableModel()->clear();
}

QStringList cRecordLink::filterWhere(QVariantList& qParams)
{
    if (pFODialog != NULL) return pFODialog->where(qParams);
    return QStringList();
}

void cRecordLink::setEditButtons()
{
    if (!isReadOnly) {
        QModelIndexList mix = pTableView->selectionModel()->selectedRows();
        int n = mix.size();
        buttonDisable(DBT_DELETE,  isNoDelete || n <  1 );
        buttonDisable(DBT_MODIFY,                n != 1);
        // buttonDisable(DBT_INSERT,  isNoInsert || ((flags & (RTF_IGROUP | RTF_IMEMBER | RTF_CHILD)) && (owner_id == NULL_ID)));
    }
}


void cRecordLink::_refresh(bool all)
{
    QString sql;
    if (viewName.isEmpty()) sql = "SELECT NULL,* FROM " + pTableShape->getName(_sTableName);
    else                    sql = "SELECT * FROM " + viewName;
    QVariantList qParams;
    QStringList wl = where(qParams);
    if (!wl.isEmpty()) {
        if (wl.at(0) == _sFalse) {         // Ha üres..
            pTableModel()->clear();
            return;
        }
        sql += " WHERE " + wl.join(" AND ");
    }

    if (pFODialog != NULL) {
        QString ord = pFODialog->ord();
        if (!ord.isEmpty()) sql += " ORDER BY " + ord;
    }

    if (!pTabQuery->prepare(sql)) SQLPREPERR(*pTabQuery, sql);
    int i = 0;
    foreach (QVariant v, qParams) pTabQuery->bindValue(i++, v);
    if (!pTabQuery->exec()) SQLQUERYERR(*pTabQuery);
    pTableModel()->setRecords(*pTabQuery, all);
    pTableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
}


QModelIndexList cRecordLink::selectedRows()
{
    return pTableView->selectionModel()->selectedRows();
}

QModelIndex cRecordLink::actIndex()
{
    QModelIndexList mil = pTableView->selectionModel()->selectedRows();
    if (mil.size() != 1 ) {
        DERR() << endl;
        return QModelIndex();
    }
    return mil.first();
}


cRecordAny *cRecordLink::actRecord(const QModelIndex &_mi)
{
    QModelIndex mi = _mi;
    if (!mi.isValid()) mi = actIndex();
    if (mi.isValid() && isContIx(pTableModel()->records(), mi.row())) return pTableModel()->records()[mi.row()];
    return NULL;
}

cRecordAny * cRecordLink::nextRow(QModelIndex *pMi, int _upRes)
{
    (void)_upRes;
    if (pMi->isValid() == false) return NULL;    // Nincs aktív
    int row = pMi->row();
    row++;
    if (row == pTableModel()->size()) return NULL;
    if (!isContIx(pTableModel()->records(), row)) {
        DWAR() << "Invalid row : " << row << endl;
        *pMi = QModelIndex();
        return NULL;
    }
    *pMi = pTableModel()->index(row, pMi->column());
    if (!pMi->isValid()) {
        DWAR() << "Invalid index : " << row << endl;
        return NULL;
    }
    selectRow(*pMi);
    cRecordAny *pRec = actRecord(*pMi);
    if (pRec == NULL) return NULL;
    pRec = dynamic_cast<cRecordAny *>(pRec->dup());
    return pRec;
}

cRecordAny *cRecordLink::prevRow(QModelIndex *pMi, int _upRes)
{
    (void)_upRes;
    if (pMi->isValid() == false) return NULL;    // Nincs aktív
    int row = pMi->row();
    row--;
    if (row == -1) return NULL;
    if (!isContIx(pTableModel()->records(), row)) {
        DWAR() << "Invalid row : " << row << endl;
        *pMi = QModelIndex();
        return NULL;
    }
    *pMi = pTableModel()->index(row, pMi->column());
    if (!pMi->isValid()) {
        DWAR() << "Invalid index : " << row << endl;
        return NULL;
    }
    selectRow(*pMi);
    cRecordAny *pRec = actRecord(*pMi);
    if (pRec != NULL) return dynamic_cast<cRecordAny *>(pRec->dup());
    return NULL;
}

void cRecordLink::selectRow(const QModelIndex& mi)
{
    int row = mi.row();
    _DBGFN() << " row = " << row << endl;
    pTableView->selectRow(row);
}

cRecordAny * cRecordLink::record(int i)
{
    if (!isContIx(pTableModel()->_records, i)) EXCEPTION(ENOINDEX, i);
    return pTableModel()->_records[i];
}

cRecordAny * cRecordLink::record(QModelIndex mi)
{
    return record(mi.row());
}


