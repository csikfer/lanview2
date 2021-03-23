#include "guidata.h"
#include "record_table.h"
#include "record_tree.h"
#include "cerrormessagebox.h"


cRecordTree::cRecordTree(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordsViewBase(pts, _upper, _isDialog, par)
{
    ;
}

cRecordTree::~cRecordTree()
{
    ;
}

void cRecordTree::init()
{
    pTreeView  = nullptr;
    // Az alapértelmezett gombok:
    buttons << DBT_CLOSE << DBT_SPACER;
    if (pTableShape->isFeature(_sButtonCopy) || pTableShape->isFeature(_sReport)) {
        if (pTableShape->isFeature(_sButtonCopy)) buttons << DBT_COPY;
        if (pTableShape->isFeature(_sReport))     buttons << DBT_REPORT;
    }
    buttons << DBT_EXPAND << DBT_REFRESH << DBT_ROOT << DBT_RESTORE;
    if (isReadOnly == false) {
        buttons << DBT_SPACER << DBT_DELETE << DBT_INSERT << DBT_SIMILAR << DBT_MODIFY;
    }
    if (pUpper != nullptr && shapeType < ENUM2SET(TS_LINK)) shapeType |= ENUM2SET(TS_CHILD);
    qlonglong st = shapeType & ~ENUM2SET2(TS_TREE, TS_BARE);
    if (st == 0 && shapeType != ENUM2SET2(TS_TREE, TS_BARE)) st = TS_TREE;
    switch (st) {
    case 0:
        if (pUpper != nullptr) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_TREE;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_SIMPLE):
    case ENUM2SET(TS_TREE):
        if (pUpper != nullptr) EXCEPTION(EDATA);
        flags = RTF_SINGLE | RTF_TREE;
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_MEMBER):
        if (pUpper != nullptr) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_MEMBER | RTF_TREE;
        initMaster();
        break;
    case ENUM2SET(TS_GROUP):
        if (pUpper != nullptr) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_GROUP | RTF_TREE;
        initMaster();
        break;
    case ENUM2SET(TS_IGROUP):
    case ENUM2SET(TS_NGROUP):
    case ENUM2SET(TS_IMEMBER):
    case ENUM2SET(TS_NMEMBER):
        if (pUpper == nullptr) EXCEPTION(EDATA);
        if (tableInhType != TIT_NO && tableInhType != TIT_ONLY) EXCEPTION(EDATA);
        buttons.clear();
        buttons << DBT_EXPAND << DBT_REFRESH << DBT_SPACER;
        switch (shapeType) {
        case ENUM2SET(TS_IGROUP):
            flags = RTF_SLAVE | RTF_IGROUP;
            if (isReadOnly == false) buttons << DBT_TAKE_OUT << DBT_DELETE << DBT_INSERT << DBT_SIMILAR << DBT_MODIFY;
            break;
        case ENUM2SET(TS_NGROUP):
            flags = RTF_SLAVE | RTF_NGROUP;
            if (isReadOnly == false) buttons << DBT_INSERT << DBT_SIMILAR << DBT_PUT_IN;
            break;
        case ENUM2SET(TS_IMEMBER):
            flags = RTF_SLAVE | RTF_IMEMBER;
            if (isReadOnly == false) buttons << DBT_TAKE_OUT;
            break;
        case ENUM2SET(TS_NMEMBER):
            flags = RTF_SLAVE | RTF_NMEMBER;
            if (isReadOnly == false) buttons << DBT_PUT_IN;
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_OWNER):
        if (pUpper != nullptr) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_OVNER | RTF_TREE;
        initMaster();
        break;
    case ENUM2SET(TS_CHILD):
        if (pUpper == nullptr) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD | RTF_TREE;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_OWNER, TS_CHILD):
        if (pUpper == nullptr) EXCEPTION(EDATA);
        flags = RTF_OVNER | RTF_SLAVE | RTF_CHILD | RTF_TREE;
        buttons.pop_front();    // A close nem kell
        initMaster();
        break;
    default:
        EXCEPTION(ENOTSUPP, pTableShape->getId(_sTableShapeType),
                  tr("TABLE %1 SHAPE %2 TYPE : %3")
                  .arg(pTableShape->getName(),
                       pTableShape->getName(_sTableName),
                       pTableShape->getName(_sTableShapeType))
                  );
    }
    refresh();
    connect(pTreeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(modifyByIndex(QModelIndex)));
}

cRecordViewModelBase * cRecordTree::newModel()
{
    return new cRecordTreeModel(*this);
}

void cRecordTree::initSimple(QWidget *pW)
{
    pButtons    = new cDialogButtons(buttons, pW);
    pMainLayout  = new QVBoxLayout(pW);
    pTreeView   = new QTreeView(pW);
    pModel      = newModel();
    if (!pTableShape->getBool(_sTableShapeType, TS_BARE)) {
        QString title =  pTableShape->getText(cTableShape::LTX_TABLE_TITLE, pTableShape->getName());
        if (title.size() > 0) {
            QLabel *pl = new QLabel(title);
            pMainLayout->addWidget(pl);
        }
    }
    pMainLayout->addWidget(pTreeView);

    // PDEB(VERBOSE) << "Indent : " << pTreeView->indentation() << endl;

    pMainLayout->addWidget(pButtons->pWidget());
    pTreeView->setModel(pTreeModel());
    pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);     // Csak sor jelölhető ki
    pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);    // Egyszerre csak egy sor kijelölése
    pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QString style = tableShape().getName(_sStyleSheet);
    if (!style.isEmpty()) pTreeView->setStyleSheet(style);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
    if (!isReadOnly) {
        connect(pTreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }
    if (pMaster != nullptr) {
        pMaster->pMasterSplitter->addWidget(_pWidget);
    }
    pTreeView->header()->setSectionsClickable(true);
    connect(pTreeView->header(), SIGNAL(sectionClicked(int)), this, SLOT(clickedHeader(int)));
    connect(pTreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    if (pFODialog != nullptr) EXCEPTION(EPROGFAIL);
    pFODialog = new cRecordTableFODialog(pq, *this);
    // A model konstruktorban nem megy az oszlopok eltüntetése, így mégegyszer...
    for (int i = 0; i < fields.size(); ++i) {
        hideColumn(i, field(i).fieldFlags & ENUM2SET(FF_TABLE_HIDE));
    }
    pTreeView->header()->setMinimumSectionSize(24); // Icon
    // PDEB(VERBOSE) << "Indent : " << pTreeView->indentation() << endl;
    pTreeView->setIndentation(16);
    // PDEB(VERBOSE) << "Indent : " << pTreeView->indentation() << endl;
    pTreeView->setRootIsDecorated(true);

}

bool cRecordTree::queryNodeChildrens(QSqlQuery& q, cTreeNode *pn)
{
    _DBGFN() << "pData : " << (pn->pData == nullptr ? "NULL" : pn->pData->identifying()) << endl;
    QString sql;

    if (viewName.isEmpty()) sql = "SELECT NULL,* FROM " + pTableShape->getName(_sTableName);
    else                    sql = "SELECT * FROM " + viewName;

    QVariantList qParams;
    QStringList wl = where(qParams);
    if (!wl.isEmpty() && wl.at(0) == _sFalse) {         // Ha üres..
        return false;
    }
    int tfix = recDescr().ixToParent();
    const QString idname  = dQuoted(recDescr().idName());
    const QString pidname = recDescr().columnNameQ(tfix);
    if (!wl.isEmpty()) sql += " WHERE " + wl.join(" AND ");
    // Két lépésben kérdezünk le:
    // A szűrés miatt nem jó a parent_id IS NULL-ra vizsgálni, a gyökér/gyökerek-nél.
    // Kell a gyerek rkordok száma is.
    // 'r' lessz a teljes szűrt rekord készlet. Ebből válogatjuk le a rész fák gyökereit.
    sql  = "WITH r AS (" + sql + ") ";
    sql += "SELECT *, (SELECT COUNT(*) FROM r AS c WHERE r." + idname + " = c." + pidname + ") AS child_number ";
    sql += "FROM r WHERE ";
    if (pn->pData == nullptr || pn->pData->isEmptyRec()) {    // gyökér vagy gyökerek
        sql += pidname + " IS NULL OR ";
        sql += "0 = (SELECT COUNT(*) FROM r AS p WHERE r." + pidname + " = p." + idname + ")";
    }
    else {
        qlonglong parId = pn->pData->getId();
        sql += pidname + " = " + QString::number(parId);
    }
    if (pFODialog != nullptr) {
        QString ord = pFODialog->ord();
        if (!ord.isEmpty()) sql += " ORDER BY " + ord;
    }
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    int i = 0;
    foreach (QVariant v, qParams) q.bindValue(i++, v);
    if (!q.exec()) SQLQUERYERR(q);
    bool r = q.first();
    _DBGFNL() << langBool(r) << " SQL :\n" << quotedString(sql) << "Bind :\n" << debVariantToString(qParams) << endl;
    return r;
}

QModelIndexList cRecordTree::selectedRows()
{
    return pTreeView->selectionModel()->selectedRows();
}

QModelIndex cRecordTree::actIndex()
{
    QModelIndexList mil = pTreeView->selectionModel()->selectedRows();
    if (mil.size() != 1) return QModelIndex();
    return mil.first();
}

cRecord *cRecordTree::actRecord(const QModelIndex &_mi)
{
    QModelIndex mi = _mi;
    if (!_mi.isValid()) mi = actIndex();
    cTreeNode * pn = pTreeModel()->nodeFromIndex(mi);
    if (pn->parent == nullptr || pn->pData == nullptr) return nullptr;
    return pn->pData;
}

cRecord *cRecordTree::nextRow(QModelIndex *pMi, int _upRes)
{
    if (!pMi->isValid()) return nullptr;
    cTreeNode * pn = pTreeModel()->nodeFromIndex(*pMi);
    if (pn->parent == nullptr) {
        *pMi = QModelIndex();
        return nullptr;
    }
    int row = pn->row();
    if (_upRes == 1) {  // Nem változott a fa szerkezete
        ++row;
    }
    else {      // Megváltozott a fa szerkezete, vagyis az aktuális parent-ből el lett távolítva.
        ;   // A sor sorszáma változatlan
    }
    if (isContIx(*pn->parent->pChildrens, row)) {
        *pMi = pTreeModel()->index(row, pMi->column(), pMi->parent());
        if (pMi->isValid()) {
            selectRow(*pMi);
            pn = pTreeModel()->nodeFromIndex(*pMi);
            return pn->pData->dup();
        }
    }
    *pMi = QModelIndex();
    return nullptr;
}

cRecord *cRecordTree::prevRow(QModelIndex *pMi, int _upRes)
{
    (void)_upRes;
    if (!pMi->isValid()) return nullptr;
    cTreeNode * pn = pTreeModel()->nodeFromIndex(*pMi);
    if (pn->parent == nullptr) {
        *pMi = QModelIndex();
        return nullptr;
    }
    int row = pn->row();
    if (_upRes == 1) {  // Nem változott a fa szerkezete
        --row;
    }
    else {      // Megváltozott a fa szerkezete, vagyis az aktuális parent-ből el lett távolítva.
        ;   // A sor sorszáma változatlan
    }
    if (isContIx(*pn->parent->pChildrens, row)) {
        *pMi = pTreeModel()->index(row, pMi->column(), pMi->parent());
        if (pMi->isValid()) {
            selectRow(*pMi);
            return pn->pData->dup();
        }
    }
    *pMi = QModelIndex();
    return nullptr;
}

void cRecordTree::selectRow(const QModelIndex& mi)
{
    pTreeView->selectionModel()->select(mi, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void cRecordTree::_refresh(bool first)
{
    pTreeModel()->refresh(first);
    for (int i = 0; i < pTreeModel()->columnCount(); ++i) {
        pTreeView->resizeColumnToContents(i);
    }
//    PDEB(VERBOSE) << "Indent : " << pTreeView->indentation() << endl;
//    pTreeView->resetIndentation();
//    PDEB(VERBOSE) << "Indent : " << pTreeView->indentation() << endl;
}

void cRecordTree::buttonPressed(int id)
{
    switch (id) {
    case DBT_CLOSE:     close();        break;
    case DBT_REFRESH:   refresh(false); break;
    case DBT_DELETE:    remove();       break;
    case DBT_INSERT:    insert();       break;
    case DBT_SIMILAR:   insert(true);       break;
    case DBT_MODIFY:    modify();       break;
    case DBT_PREV:      prev();         break;
    case DBT_ROOT:      setRoot();      break;
    case DBT_RESTORE:   restoreRoot();  break;
    case DBT_EXPAND:    expand();       break;
    case DBT_REPORT:    report();       break;
    default:
        DWAR() << "Invalid button id : " << id << endl;
        break;
    }
}

/*
void cRecordTree::insert()
{

}
*/
void cRecordTree::setEditButtons()
{

}
void cRecordTree::setButtons()
{
    setEditButtons();
}

void cRecordTree::prev()
{
    pTreeModel()->prevRoot(true); // egyet vissza
}

void cRecordTree::setRoot()
{
    QModelIndexList mil = pTreeView->selectionModel()->selectedRows();
    if (mil.size() == 0) return;
    pTreeModel()->setRoot(mil.at(0));
}

void cRecordTree::restoreRoot()
{
    pTreeModel()->prevRoot(false);    // vissza az eredeti gyökérig
}

void cRecordTree::expand()
{
    QModelIndex mi = actIndex();
    if (mi.isValid()) pTreeView->setExpanded(mi, true);
    else              pTreeView->expandAll();
}

void cRecordTree::hideColumn(int ix, bool f)
{
    if (f) pTreeView->hideColumn(ix);
    else   pTreeView->showColumn(ix);
}
