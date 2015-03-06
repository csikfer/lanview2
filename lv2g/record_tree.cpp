#include "guidata.h"
#include "record_table.h"
#include "record_tree.h"
#include "cerrormessagebox.h"


cRecordTree::cRecordTree(cTableShape *pts, bool _isDialog, cRecordViewBase *_upper, QWidget * par)
    : cRecordViewBase(_isDialog, par)
{
    pMaster = pUpper = _upper;
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
        pRightTable = cRecordViewBase::newRecordViewBase(*pq, pTableShape->getId(_sRightShapeId), this, _pWidget);
        break;
    default:
        EXCEPTION(ENOTSUPP);
    }
    refresh();
}

void cRecordTree::initSimple(QWidget *pW)
{
    int buttons = 0;
    int closeBut = 0;
    if (!(flags & RTF_SLAVE)) closeBut = enum2set(DBT_CLOSE);
    if (flags & RTF_SINGLE) buttons =  closeBut;
    buttons |= enum2set(DBT_REFRESH);
    if (isReadOnly == false) {
        buttons |= enum2set(DBT_DELETE, DBT_INSERT, DBT_MODIFY);
    }
    pButtons    = new cDialogButtons(buttons, 0, pW);
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

//    pTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
//    pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
//    pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(pButtons,    SIGNAL(buttonPressed(int)),   this, SLOT(buttonPressed(int)));
//    if (!isReadOnly) {
//        connect(pTableModel, SIGNAL(removed(cRecordAny*)), this, SLOT(recordRemove(cRecordAny*)), Qt::DirectConnection);
//        connect(pTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
//    }
    if (pMaster != NULL) {
        pMaster->pMasterFrame->addWidget(_pWidget);
    }
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

void cRecordTree::refresh(bool first)
{
    if (pTreeModel->pRootNode == NULL) {
        pTreeModel->pRootNode = new cTreeNode();
    }
    else if (first) {
        pTreeModel->pRootNode->clearChild();
        pDelete(pTreeModel->pRootNode->pData);

    }
    pTreeModel->fetchTree();
}

void cRecordTree::modify()
{

}
void cRecordTree::remove()
{

}
void cRecordTree::insert()
{

}
void cRecordTree::setEditButtons()
{

}
void cRecordTree::setPageButtons()
{

}
void cRecordTree::setButtons()
{

}
