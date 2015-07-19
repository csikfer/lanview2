#include "guidata.h"
#include "record_table.h"
#include "record_tree.h"
#include "cerrormessagebox.h"


cRecordTree::cRecordTree(cTableShape *pts, bool _isDialog, cRecordViewBase *_upper, QWidget * par)
    : cRecordViewBase(_isDialog, par)
{
    pMaster = pUpper = _upper;
    if (pMaster != NULL && pUpper->pMaster != NULL) pMaster = pUpper->pMaster;
    initShape(pts);
    init();
}

cRecordTree::~cRecordTree()
{
    ;
}

void cRecordTree::init()
{
    pTreeView  = NULL;
    // Az alapértelmezett gombok:
    buttons << DBT_CLOSE << DBT_SPACER << DBT_EXPAND << DBT_REFRESH;
    if (isReadOnly == false) buttons << DBT_SPACER << DBT_DELETE << DBT_INSERT << DBT_MODIFY;
    switch (shapeType) {
    case ENUM2SET2(TS_TREE, TS_NO):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_TREE;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_TREE):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_SINGLE | RTF_TREE;
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_TREE, TS_MEMBER):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_MEMBER | RTF_TREE;
        initMaster();
        break;
    case ENUM2SET2(TS_TREE, TS_GROUP):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_GROUP | RTF_TREE;
        initMaster();
        break;
    case ENUM2SET2(TS_TREE, TS_IGROUP):
    case ENUM2SET2(TS_TREE, TS_NGROUP):
    case ENUM2SET2(TS_TREE, TS_IMEMBER):
    case ENUM2SET2(TS_TREE, TS_NMEMBER):
        if (pUpper == NULL) EXCEPTION(EDATA);
        if (tit != TIT_NO && tit != TIT_ONLY) EXCEPTION(EDATA);
        buttons.clear();
        buttons << DBT_EXPAND << DBT_REFRESH << DBT_SPACER;
        switch (shapeType) {
        case ENUM2SET2(TS_TREE, TS_IGROUP):
            flags = RTF_SLAVE | RTF_IGROUP;
            if (isReadOnly == false) buttons << DBT_TAKE_OUT << DBT_DELETE << DBT_INSERT << DBT_MODIFY;
            break;
        case ENUM2SET2(TS_TREE, TS_NGROUP):
            flags = RTF_SLAVE | RTF_NGROUP;
            if (isReadOnly == false) buttons << DBT_INSERT << DBT_PUT_IN;
            break;
        case ENUM2SET2(TS_TREE, TS_IMEMBER):
            flags = RTF_SLAVE | RTF_IMEMBER;
            if (isReadOnly == false) buttons << DBT_TAKE_OUT;
            break;
        case ENUM2SET2(TS_TREE, TS_NMEMBER):
            flags = RTF_SLAVE | RTF_NMEMBER;
            if (isReadOnly == false) buttons << DBT_PUT_IN;
            break;
        default:
            EXCEPTION(EPROGFAIL);
        }
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_TREE, TS_OWNER):
        if (pUpper != NULL) EXCEPTION(EDATA);
        flags = RTF_MASTER | RTF_OVNER | RTF_TREE;
        initMaster();
        break;
    case ENUM2SET2(TS_TREE, TS_CHILD):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD | RTF_TREE;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        break;
    case ENUM2SET3(TS_TREE, TS_OWNER, TS_CHILD):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_OVNER | RTF_SLAVE | RTF_CHILD | RTF_TREE;
        buttons.pop_front();    // A close nem kell
        initSimple(_pWidget);
        pRightTable = cRecordViewBase::newRecordView(*pq, pTableShape->getId(_sRightShapeId), this, _pWidget);
        break;
    default:
        EXCEPTION(ENOTSUPP);
    }
    refresh();
}

void cRecordTree::initSimple(QWidget *pW)
{
    pButtons    = new cDialogButtons(buttons, pW);
    pMainLayer  = new QVBoxLayout(pW);
    pTreeView   = new QTreeView(pW);
    pModel      = new cRecordTreeModel(*this);
    QString title = pTableShape->getName(_sTableShapeTitle);
    if (title.size() > 0) {
        QLabel *pl = new QLabel(title);
        pMainLayer->addWidget(pl);
    }
    pMainLayer->addWidget(pTreeView);
    pMainLayer->addWidget(pButtons->pWidget());
    pTreeView->setModel(pTreeModel());

    pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);     // Csak sor jelölhető ki
    pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);    // Egyszerre csak egy sor kijelölése
    pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
    if (!isReadOnly) {
        connect(pTreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }
    if (pMaster != NULL) {
        pMaster->pMasterSplitter->addWidget(_pWidget);
    }
    pTreeView->header()->setSectionsClickable(true);
    connect(pTreeView->header(), SIGNAL(sectionClicked(int)), this, SLOT(clickedHeader(int)));
}

bool cRecordTree::queryNodeChildrens(QSqlQuery& q, cTreeNode *pn)
{
    QString sql;

    if (viewName.isEmpty()) sql = "SELECT NULL,* FROM " + pTableShape->getName(_sTableName);
    else                    sql = "SELECT * FROM " + viewName;

    QVariantList qParams;
    QStringList wl = where(qParams);
    if (!wl.isEmpty() && wl.at(0) == _sFalse) {         // Ha üres..
        return false;
    }
    int tfix = recDescr().ixToParent();
    qlonglong parId = NULL_ID;
    if (pn->pData != NULL && !pn->pData->isEmpty()) parId = pn->pData->getId();
    wl << (recDescr().columnName(tfix) + (
              (parId == NULL_ID) ? (" IS NULL")
                                 : (" = " + QString::number(parId))
                                         )
          );
    sql += " WHERE " + wl.join(" AND ");

//    QString ord = pFODialog->ord();
//    if (!ord.isEmpty()) sql += " ORDER BY " + ord;

    if (!q.prepare(sql)) SQLPREPERR(*pTabQuery, sql);
    int i = 0;
    foreach (QVariant v, qParams) pTabQuery->bindValue(i++, v);
    if (!q.exec()) SQLQUERYERR(*pTabQuery);
    return q.first();
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

cRecordAny *cRecordTree::actRecord(const QModelIndex &_mi)
{
    QModelIndex mi = _mi;
    if (!_mi.isValid()) mi = actIndex();
    cTreeNode * pn = pTreeModel()->nodeFromIndex(mi);
    if (pn->parent == NULL || pn->pData == NULL) return NULL;
    return pn->pData;
}

cRecordAny *cRecordTree::nextRow(QModelIndex *pMi, int _upRes)
{
    if (!pMi->isValid()) return NULL;
    cTreeNode * pn = pTreeModel()->nodeFromIndex(*pMi);
    if (pn->parent == NULL) {
        *pMi = QModelIndex();
        return NULL;
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
            return dynamic_cast<cRecordAny *>(pn->pData->dup());
        }
    }
    *pMi = QModelIndex();
    return NULL;
}

cRecordAny *cRecordTree::prevRow(QModelIndex *pMi, int _upRes)
{
    (void)_upRes;
    if (!pMi->isValid()) return NULL;
    cTreeNode * pn = pTreeModel()->nodeFromIndex(*pMi);
    if (pn->parent == NULL) {
        *pMi = QModelIndex();
        return NULL;
    }
    int row = pn->row();
    --row;
    if (isContIx(*pn->parent->pChildrens, row)) {
        *pMi = pTreeModel()->index(row, pMi->column(), pMi->parent());
        if (pMi->isValid()) {
            selectRow(*pMi);
            return dynamic_cast<cRecordAny *>(pn->pData->dup());
        }
    }
    *pMi = QModelIndex();
    return NULL;
}

void cRecordTree::selectRow(const QModelIndex& mi)
{
    pTreeView->selectionModel()->select(mi, QItemSelectionModel::ClearAndSelect);
}

void cRecordTree::_refresh(bool first)
{
    pTreeModel()->refresh(first);
}

void cRecordTree::buttonPressed(int id)
{
    switch (id) {
    case DBT_CLOSE:     close();    break;
    case DBT_REFRESH:   refresh();  break;
    case DBT_DELETE:    remove();   break;
    case DBT_INSERT:    insert();   break;
    case DBT_MODIFY:    modify();   break;
    case DBT_PREV:      prev();     break;
    case DBT_RESET:     setRoot();      break;
    case DBT_RESTORE:   restoreRoot();  break;
    case DBT_EXPAND:    expand();   break;
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
