#include "record_tree_model.h"
#include "record_tree.h"
#include "cerrormessagebox.h"

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
cRecordTreeModel::cRecordTreeModel(cRecordTree&  _rt)
    : QAbstractItemModel(_rt.pWidget())
    , cRecordViewModelBase(_rt)
{
    pActRootNode = NULL;
    pRootNode = NULL;
    _viewRowNumbers = false;    // NINCS!!
    pq = newQuery();
}

cRecordTreeModel::~cRecordTreeModel()
{
    pDelete(pRootNode);
    delete pq;
}

QModelIndex cRecordTreeModel::findNode(qlonglong pid, const QModelIndex& mi)
{
    cTreeNode *pn = nodeFromIndex(mi);
    _DBGFN() << QString("@(%1, %2)").arg(pid).arg(pn->name()) << endl;
    if (pn->pData != NULL && pid == pn->pData->getId()) {
        _DBGFNL() << " found : " << pn->name() << endl;
        return mi;
    }
    int rows = rowCount(mi);
    for (int i = 0; i < rows; ++i) {
        QModelIndex cmi = index(i, 0, mi);
        cmi = findNode(pid, cmi);
        if (cmi.isValid()) {
            _DBGFNL() << " found parent " << pn->name() << " row : " << i << endl;
            return cmi;
        }
    }
    _DBGFNL() << "not found : " << pn->name() << endl;
    return QModelIndex();   // Nincs találat
}

QModelIndex cRecordTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (pActRootNode == NULL) {
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
    cTreeNode *pNode = nodeFromIndex(child);
    if (pNode == NULL) return QModelIndex();
    cTreeNode *parentNode = pNode->parent;
    if (parentNode == NULL) return QModelIndex();
    if (parentNode->parent == NULL) return QModelIndex();
    int row = parentNode->row();
    return createIndex(row, 0, parentNode);
}

int         cRecordTreeModel::rowCount(const QModelIndex &parent) const
{
    cTreeNode *parentNode = nodeFromIndex(parent);
    if (parentNode == NULL) {
        PDEB(VVERBOSE) << __PRETTY_FUNCTION__ <<  " return : 0 (parant is NULL)" << endl;
        return 0;
    }
//    PDEB(VVERBOSE) << __PRETTY_FUNCTION__ <<  " return : " << parentNode->childrens.count()
//                   << " (parant : "<< parentNode->name() << ")" << endl;
    if (parentNode->pChildrens == NULL) {
        // Csak így mőködik, de akkor minek a canFetchMore() és fetchMore(); ?? (talán előre kéne tudni a sorok számát?)
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
        // Ettül a DEBUG üzenettől nagyon belassul!!!
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
        return pActRootNode;
    }
}

void cRecordTreeModel::refresh(bool first)
{
    first = first || (pRootNode == NULL) || (pRootNode == pActRootNode);
    qlonglong rid = NULL_ID;
    if (!first) {           // Ha rész fa van
        if (pActRootNode->pData == NULL) EXCEPTION(EPROGFAIL);
        rid = pActRootNode->pData->getId();
        if (rid == NULL_ID) EXCEPTION(EPROGFAIL);
    }
    beginResetModel();
    pDelete(pRootNode);
    pRootNode = pActRootNode = new cTreeNode();;
    fetchTree();
    endResetModel();
    if (rid != NULL_ID) {
        QModelIndex rmi = findNode(rid);
        setRoot(rmi);
    }
}

void cRecordTreeModel::fetchTree()
{
    readChilds(pActRootNode);
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

/// Törli a csomóponthoz rendelt rekordot, és a gyerek csomópontokat, vagyis a teljes rész fát.
/// A csomopontot eltávolítja a pearent-ből.
/// Az adatok változásáról nem küld szignáltt.
bool cRecordTreeModel::removeNode(cTreeNode *pn)
{
    if (pn->parent != NULL) {   // Töröljük az elemet a parentből
        pn->parent->pChildrens->removeAt(pn->row());
    }
    if (pn->pChildrens == NULL) {   // Még be sincsenek olvasva a gyerköcök
        readChilds(pn);
    }
    // Töröljük a gyerköcöket..
    while (pn->pChildrens->size()) removeNode(pn->pChildrens->at(0));
    bool r = pn->pData->remove(*pq); // Töröljük a rekordot;
    delete pn;  // Végül magát (a már gyermektelen) a node-ot
    return r;
}

void cRecordTreeModel::setRoot(const QModelIndex &mi)
{
    cTreeNode *pn = nodeFromIndex(mi);
    beginResetModel();
    pActRootNode = pn;
    endResetModel();
}

void cRecordTreeModel::prevRoot(bool _sing)
{
    if (pActRootNode->parent == NULL) return;  // nincs mit visszaállítani
    beginResetModel();
    if (_sing) {
        // egyet vissza
        pActRootNode = pActRootNode->parent;
    }
    else while (pActRootNode->parent != NULL) {
        // elballagunk az eredeti gyökérig
        pActRootNode = pActRootNode->parent;
    }
    endResetModel();
}


void cRecordTreeModel::removeRecords(const QModelIndexList& mil)
{
    foreach (QModelIndex mi, mil) {
        removeRec(mi);
    }
}

bool cRecordTreeModel::removeRec(const QModelIndex & mi)
{
    cTreeNode *pn = nodeFromIndex(mi);
    int b = QMessageBox::warning(recordView.pWidget(),
                         trUtf8("Egy rész fa teljes törlése!"),
                         trUtf8("Valóban törölni akarja a kijelölt %1 nevű objektumot, az összes alárendelt objektummal eggyütt ?").arg(pn->name()),
                         QMessageBox::Ok, QMessageBox::Cancel);
    if (b != QMessageBox::Ok) return false;
    beginRemoveRows(mi.parent(), mi.row(), mi.row());
    sqlBegin(*pq);
    bool r = removeNode(pn);
    r = r ? sqlEnd(*pq) : !sqlRollback(*pq);
    endRemoveRows();
    if (!r) recordView.refresh(true);
    return r;
}

bool cRecordTreeModel::removeRow(const QModelIndex & mi)
{
    (void)mi;
    // EXCEPTION(ENOTSUPP);
    return false;
}

int cRecordTreeModel::checkUpdateRow(const QModelIndex& mi, cRecordAny * pRec, QModelIndex& new_parent)
{
    cTreeNode *pn = nodeFromIndex(mi);  // A modosított elem (node)
    if (pn->parent == NULL || pn->pData == NULL) EXCEPTION(EPROGFAIL);
    int ixPId = pRec->descr().ixToTree();     // Az ősre mutató ID mező indexe
    qlonglong old_pid = pn->pData->getId(ixPId);    // Régi parent ID
    qlonglong new_pid = pRec->getId(ixPId);         // Új parent ID
    if (old_pid == new_pid) {   // A modosítottat ugyanode kell visszarakni
        return 1;   // OK, A fát nem kell átrendezni
    }
    else {                      // Átírta az owber ID-t
        QModelIndex pmi;        // Parent index (alapból a gyökér)
        if (new_pid != NULL_ID) {   // Ha nem a gyökér a parent
            pmi = findNode(new_pid, pmi);
            if (!pmi.isValid()) {
                return 0;   // Ez gáz, hurkot csinált a júzer.
            }
        }
        new_parent = pmi;
        return 2;   // Fa modosult, új parent megvan
    }
}

bool cRecordTreeModel::updateRec(const QModelIndex& mi, cRecordAny * pRec)
{
    QModelIndex npi;
    int chkr = checkUpdateRow(mi, pRec, npi);
    if (chkr == 0) {
        QMessageBox::warning(recordView.pWidget(), trUtf8("Hibás adatok"), trUtf8("Nem megengedett szülő megadása. Hurok a fában."));
        return false;
    }
    if (!cErrorMessageBox::condMsgBox(pRec->tryUpdate(*pq, false))) {
        return false;
    }
    int row = mi.row();
    cTreeNode *pn = nodeFromIndex(mi);  // A modosított elem (node)
    cTreeNode *pp = pn->parent;         // A szülő
    beginRemoveRows(mi.parent(), row, row);
    pp->pChildrens->takeAt(row);
    endRemoveRows();
    delete pn->pData;
    pn->pData = pRec;
    switch (chkr) {
    case 1:
        beginInsertRows(mi.parent(), row, row);
        pp->pChildrens->insert(row, pn);
        endInsertRows();
        break;
    case 2: {
        cTreeNode *pnp = nodeFromIndex(npi);
        row = pnp->rows();
        beginInsertRows(npi, row, row);
        pnp->pChildrens->push_back(pn);
        endInsertRows();
        break;
    }
    default:
        EXCEPTION(EPROGFAIL);
        return false;
    }
    return true;
}

bool cRecordTreeModel::updateRow(const QModelIndex& mi, cRecordAny * pRec)
{
    (void)mi;
    (void)pRec;
//  EXCEPTION(ENOTSUPP);
    return false;
}

bool cRecordTreeModel::insertRow(cRecordAny *pRec)
{
    int ixPId = pRec->descr().ixToTree();     // Az ősre mutató ID mező indexe
    qlonglong pid = pRec->getId(ixPId);
    QModelIndex pmi;
    if (pid == NULL_ID) {
        pmi = findNode(pid);
        if (!pmi.isValid()) {   // nincs meg a parent!!
            DWAR() << "Parent not found, record : " << pRec->toString() << endl;
            return false;
        }
    }
    cTreeNode *ppn = nodeFromIndex(pmi);
    int row = rowCount(pmi);
    beginInsertRows(pmi, row,row);
    cTreeNode *pnn = new cTreeNode(pRec, ppn);
    ppn->pChildrens->append(pnn);
    endInsertRows();
    return true;
}
