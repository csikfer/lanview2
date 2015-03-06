#include "record_tree_model.h"
#include "record_tree.h"

cTreeNode::cTreeNode(cRecordAny *__po, cTreeNode * _parentNode)
    : pData(__po)
{
    parent   = _parentNode;
    pChildrens = NULL;
}

cTreeNode::cTreeNode(const cTreeNode&)
{
    EXCEPTION(EPROGFAIL);
}

cTreeNode::~cTreeNode()
{
    if (pData) {
        delete pData;
    }
    if (pChildrens) {
        qDeleteAll(*pChildrens);
        delete pChildrens;
    }
}

int cTreeNode::row() const
 {
     if (parent && parent->pChildrens)
         return parent->pChildrens->indexOf(const_cast<cTreeNode*>(this));
     return 0;
 }

/* */
cRecordTreeModel::cRecordTreeModel(const cRecordTree&  _rt)
    : QAbstractItemModel(_rt.pWidget())
    , cRecordViewModelBase(_rt)
{
    pRootNode = NULL;
    _viewRowNumbers = false;
    pq = newQuery();
}

cRecordTreeModel::~cRecordTreeModel()
{
    pDelete(pRootNode);
    delete pq;
}

QModelIndex cRecordTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (pRootNode == NULL) {
        DWAR() << "pRootNode is NULL." << endl;
        return QModelIndex();
    }
    if (!hasIndex(row, column, parent)) {
        DWAR() << "Is no valid index." << endl;
        return QModelIndex();
    }
    cTreeNode *parentNode = nodeFromIndex(parent);
    if (parentNode->pChildrens == NULL) EXCEPTION(EPROGFAIL);
    PDEB(VVERBOSE) << "Index: " << VDEB(row) << VDEB(column) << " parent = " << parentNode->name() << endl;
    if (parentNode->pChildrens->size() <= row) {
        DWAR() << "Invalid row = " << row << "child number : " << parentNode->pChildrens->size() << endl;
        return QModelIndex();
    }
    return createIndex(row, column, parentNode->pChildrens->at(row));
}

QModelIndex cRecordTreeModel::parent(const QModelIndex& child) const
{
    cTreeNode *node = nodeFromIndex(child);
    if (node == NULL) return QModelIndex();
    cTreeNode *parentNode = node->parent;
    if (parentNode == NULL) return QModelIndex();
    cTreeNode *grandparentNode = parentNode->parent;
    if (grandparentNode == NULL) return QModelIndex();
    int row = grandparentNode->pChildrens->indexOf(parentNode);
    return createIndex(row, child.column(), parentNode);
}

int         cRecordTreeModel::rowCount(const QModelIndex &parent) const
{
    cTreeNode *parentNode = nodeFromIndex(parent);
    if (parentNode == NULL) {
//        PDEB(VVERBOSE) << __PRETTY_FUNCTION__ <<  " return : 0 (parant is NULL)" << endl;
        return 0;
    }
//    PDEB(VVERBOSE) << __PRETTY_FUNCTION__ <<  " return : " << parentNode->childrens.count()
//                   << " (parant : "<< parentNode->name() << ")" << endl;
    if (parentNode->pChildrens == NULL) {
        // Csak így mőködik, de akkor minak a canFetchMore() és fetchMore(); ??
        const_cast<cRecordTreeModel *>(this)->readChilds(parentNode);
    }
    return parentNode->pChildrens->count();
}

int         cRecordTreeModel::columnCount( const QModelIndex & parent) const
{
    (void)parent;
    return _col2field.size();
}
QVariant    cRecordTreeModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()) {
        //DWAR() << "Invalid modelIndex." << endl;
        return QVariant();
    }
    cTreeNode *node = nodeFromIndex(index);
    if (node == NULL) return QVariant();
    int col = index.column();
    if (col < _col2field.size()) {
        int fix = _col2field[col];  // Mező index a rekordbam
        int mix = _col2shape[col];  // Index a leíróban
        // Ettül a DEBUG üzenettől bagyon belassul!!!
//        _DBGFN() << " name = " << node->name() << "; " << VDEB(col) << VDEB(role) << endl;
        if (role == Qt::DisplayRole)       return node->pData->view(*pq, fix);
        if (role == Qt::TextAlignmentRole) return columns[mix]->dataAlign;
        const colorAndFont&   cf = node->pData->isNull(fix)
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
QVariant    cRecordTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return _headerData(section, orientation, role);
}

Qt::ItemFlags cRecordTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool cRecordTreeModel::canFetchMore(const QModelIndex &parent) const
{
    cTreeNode *pNode = nodeFromIndex(parent);
    return pNode->pChildrens == NULL;
}

void cRecordTreeModel::fetchMore(const QModelIndex &parent)
{
    cTreeNode *pNode = nodeFromIndex(parent);
    _DBGFN() << pNode->name() << endl;
    readChilds(pNode);
}


cTreeNode * cRecordTreeModel::nodeFromIndex(const QModelIndex& index) const
{
    if (index.isValid()) {
        return static_cast<cTreeNode *>(index.internalPointer());
    }
    else {
        return pRootNode;
    }
}

void cRecordTreeModel::fetchTree()
{
    beginResetModel();

    readChilds(pRootNode);

    endResetModel();
}

void cRecordTreeModel::readChilds(cTreeNode *pNode)
{
    if (pNode->pChildrens != NULL) {
        DWAR() << "node : " << pNode->name() << " pChildrens pointer is not NULL." << endl;
        return;
    }
    pNode->pChildrens = new QList<cTreeNode *>;
    if (((cRecordTree&)recordView).queryNodeChildrens(*pq, pNode)) {
        do {
            pNode->addChild(new cTreeNode(qGetRecord(*pq), pNode));
        } while (pq->next());
    }
}
