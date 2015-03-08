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
    pTreeModel = NULL;
    pTreeView  = NULL;

    switch (pTableShape->getId(_sTableShapeType)) {
    case ENUM2SET2(TS_TREE, TS_NO):
        flags = RTF_SLAVE;
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_TREE):
        flags = RTF_SINGLE;
        initSimple(_pWidget);
        break;
    case ENUM2SET2(TS_TREE, TS_OWNER):
        flags = RTF_MASTER | RTF_OVNER;
        initOwner();
        break;
    case ENUM2SET2(TS_TREE, TS_CHILD):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD;
        initSimple(_pWidget);
        break;
    case ENUM2SET3(TS_TREE, TS_OWNER, TS_CHILD):
        flags = RTF_OVNER | RTF_SLAVE | RTF_CHILD;
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
    int buttons, buttons2 = 0;
    int closeBut = 0;
    if (!(flags & RTF_SLAVE)) closeBut = enum2set(DBT_CLOSE);
    if (flags & RTF_SINGLE || isReadOnly) {
        buttons = closeBut | enum2set(DBT_REFRESH, DBT_PREV, DBT_RESTORE, DBT_SET_ROOT);
        if (isReadOnly == false) {
            buttons |= enum2set(DBT_DELETE, DBT_INSERT, DBT_MODIFY);
        }
    }
    else {
        buttons  = enum2set(DBT_PREV, DBT_SET_ROOT, DBT_RESTORE);
        buttons2 = closeBut | enum2set(DBT_DELETE, DBT_INSERT, DBT_MODIFY, DBT_REFRESH);
    }
    pButtons    = new cDialogButtons(buttons, buttons2, pW);


    pMainLayer  = new QVBoxLayout(pW);
    pTreeView   = new QTreeView(pW);
    pTreeModel  = new cRecordTreeModel(*this);
    QString title = pTableShape->getName(_sTableShapeTitle);
    if (title.size() > 0) {
        QLabel *pl = new QLabel(title);
        pMainLayer->addWidget(pl);
    }
    pMainLayer->addWidget(pTreeView);
    pMainLayer->addWidget(pButtons->pWidget());
    pTreeView->setModel(pTreeModel);

    pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);     // Csak sor jelölhető ki
    pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);    // Egyszerre csak egy sor kijelölése
    pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
    if (!isReadOnly) {
        connect(pTreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }
    if (pMaster != NULL) {
        pMaster->pMasterFrame->addWidget(_pWidget);
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
    int tfix = recDescr().ixToTree();
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

QModelIndex cRecordTree::actIndex()
{
    QModelIndexList mil = pTreeView->selectionModel()->selectedRows();
    if (mil.size() == 0) return QModelIndex();
    return mil.first();
}

cRecordAny *cRecordTree::actRecord(const QModelIndex &_mi)
{
    QModelIndex mi = _mi;
    if (!_mi.isValid()) mi = actIndex();
    cTreeNode * pn = pTreeModel->nodeFromIndex(mi);
    if (pn->parent == NULL || pn->pData == NULL) return NULL;
    return pn->pData;
}

cRecordAny *cRecordTree::nextRow(QModelIndex *pMi)
{
    if (!pMi->isValid()) return NULL;
    cTreeNode * pn = pTreeModel->nodeFromIndex(*pMi);
    if (pn->parent == NULL) {
        *pMi = QModelIndex();
        return NULL;
    }
    int row = pn->row();
    ++row;
    if (isContIx(*pn->parent->pChildrens, row)) {
        *pMi = pTreeModel->index(row, pMi->column(), pMi->parent());
        if (pMi->isValid()) {
            selectRow(*pMi);
            return dynamic_cast<cRecordAny *>(pn->pData->dup());
        }
    }
    *pMi = QModelIndex();
    return NULL;
}

cRecordAny *cRecordTree::prevRow(QModelIndex *pMi)
{
    if (!pMi->isValid()) return NULL;
    cTreeNode * pn = pTreeModel->nodeFromIndex(*pMi);
    if (pn->parent == NULL) {
        *pMi = QModelIndex();
        return NULL;
    }
    int row = pn->row();
    --row;
    if (isContIx(*pn->parent->pChildrens, row)) {
        *pMi = pTreeModel->index(row, pMi->column(), pMi->parent());
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

bool cRecordTree::updateRow(const QModelIndex& _mi, cRecordAny *__new)
{
    return pTreeModel->updateRow(_mi, __new);
}

void cRecordTree::refresh(bool first)
{
    // EZ ITT NEM KEREK !!!! Ütközik a részfa megjelenítéssel!!!
    if (first) pDelete(pTreeModel->pRootNode);
    if (pTreeModel->pRootNode == NULL) {
        pTreeModel->pRootNode = new cTreeNode();
    }
    pTreeModel->fetchTree();
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
    case DBT_SET_ROOT:  setRoot();      break;
    case DBT_RESTORE:   restoreRoot();  break;
    default:
        DWAR() << "Invalid button id : " << id << endl;
        break;
    }
}

void cRecordTree::remove()
{
    DBGFN();
    QModelIndexList mil = pTreeView->selectionModel()->selectedRows();
    if (mil.size() == 0) return;
    pTreeModel->remove(mil.at(0));
    refresh();
}
void cRecordTree::insert()
{

}
void cRecordTree::setEditButtons()
{

}
void cRecordTree::setButtons()
{
    setEditButtons();
}

void cRecordTree::prev()
{
    pTreeModel->prevRoot(true); // egyet vissza
}

void cRecordTree::setRoot()
{
    QModelIndexList mil = pTreeView->selectionModel()->selectedRows();
    if (mil.size() == 0) return;
    pTreeModel->setRoot(mil.at(0));
}

void cRecordTree::restoreRoot()
{
    pTreeModel->prevRoot(false);    // vissza az eredeti gyökérig
}

void cRecordTree::selectionChanged(QItemSelection,QItemSelection)
{

}
