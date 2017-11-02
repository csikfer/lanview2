#include "record_tree_model.h"
#include "record_tree.h"
#include "cerrormessagebox.h"

cTreeNode::cTreeNode(cRecord *__po, cTreeNode * _parentNode)
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

void cTreeNode::addChild(cTreeNode *pn)
{
    pChildrens->append(pn);
    pn->parent = this;
}

void cTreeNode::addChild(cRecord *pRec)
{
    cTreeNode *pn = new cTreeNode(pRec, this);
    pChildrens->append(pn);
}

int cTreeNode::row() const
 {
     if (parent && parent->pChildrens)
         return parent->pChildrens->indexOf(const_cast<cTreeNode*>(this));
     return 0;
 }

/* -------------------------------------------------------------------------------- */

cRecordTreeModel::cRecordTreeModel(cRecordTree&  _rt)
    : QAbstractItemModel(_rt.pWidget())
    , cRecordViewModelBase(_rt)
{
    pActRootNode = NULL;
    pRootNode    = NULL;
    rootId       = NULL_ID;
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
        DWAR() << "pActRootNode is NULL." << endl;
        return QModelIndex();
    }
    if (!hasIndex(row, column, parent)) {
        DWAR() << "Is no valid index." << endl;
        return QModelIndex();
    }
    cTreeNode *parentNode = nodeFromIndex(parent);
    if (parentNode->pChildrens == NULL) EXCEPTION(EPROGFAIL);
//    PDEB(VVERBOSE) << "Index: " << VDEB(row) << VDEB(column) << " parent = " << parentNode->name() << endl;
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

int         cRecordTreeModel::columnCount(const QModelIndex & parent) const
{
    (void)parent;
    return _col2field.size();
}

QVariant    cRecordTreeModel::data(const QModelIndex & index, int role) const
{
    QVariant r;
    if (!index.isValid())           return r;
    cTreeNode *node = nodeFromIndex(index);
    if (node == NULL)               return r;
    int col = index.column();
    if (col >= _col2field.size())   return r;
    int fix = _col2field[col];  // Mező index a rekordbam
    int mix = _col2shape[col];  // Index a leíróban
    cRecord *pr = node->pData;
    cRecordTableColumn * pColumn = columns[mix];
    bool bTxt = pColumn->textIndex != NULL_IX;
    if (!bTxt && recDescr != pr->descr()) { // A mező sorrend nem feltétlenül azonos (öröklés)
        const QString& fn = recDescr.columnName(fix);
        fix = pr->toIndex(fn, EX_IGNORE);   // Nem biztos, hogy van ilyen mező (ős)
    }
    return _data(fix, *columns[mix], pr, role, bTxt);
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
    if (pNode == NULL) EXCEPTION(EPROGFAIL);
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
    qlonglong rid = rootId;
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
/// Az adatok változásáról nem küld szignáltt. Rekourzív.
bool cRecordTreeModel::removeNode(cTreeNode *pn)
{
    if (pn->parent == NULL) {
        EXCEPTION(EPROGFAIL);   // A gyökér törlése nem értelmezhető
    }
    // Töröljük az elemet a parentből
    pn->parent->pChildrens->removeAt(pn->row());
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
    static const QString tn = "delete_tree";
    cTreeNode *pn = nodeFromIndex(mi);
    int b = QMessageBox::warning(recordView.pWidget(),
                         trUtf8("Egy rész fa teljes törlése!"),
                         trUtf8("Valóban törölni akarja a kijelölt %1 nevű objektumot, az összes alárendelt objektummal eggyütt ?\n").arg(pn->name()) + sIrrevocable,
                         QMessageBox::Ok, QMessageBox::Cancel);
    if (b != QMessageBox::Ok) return false;
    beginRemoveRows(mi.parent(), mi.row(), mi.row());
    sqlBegin(*pq, tn);
    bool r = removeNode(pn);
    if (r) sqlCommit(*pq, tn);
    else   sqlRollback(*pq, tn);
    endRemoveRows();
    if (!r) recordView.refresh(true);
    return r;
}

bool cRecordTreeModel::removeRow(const QModelIndex & mi)
{
    (void)mi;
    EXCEPTION(ENOTSUPP);
    return false;
}

cRecord *cRecordTreeModel::record(const QModelIndex &index)
{
    if (index.isValid()) {
        cTreeNode *node = nodeFromIndex(index);
        if (node != NULL) {
            return node->pData;
        }
    }
    return NULL;
}

int cRecordTreeModel::checkUpdateRow(const QModelIndex& mi, cRecord * pRec, QModelIndex& new_parent)
{
    cTreeNode *pn = nodeFromIndex(mi);  // A modosított elem (node)
    if (pn->parent == NULL || pn->pData == NULL) EXCEPTION(EPROGFAIL);
    int ixPId = pRec->descr().ixToParent();     // Az ősre mutató ID mező indexe
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

int cRecordTreeModel::updateRec(const QModelIndex& mi, cRecord *pRec)
{
    QModelIndex npi;
    // Lecsekkoljuk, változott-e a fa, az új rendben van-e, és mi a parent új indexe
    int chkr = checkUpdateRow(mi, pRec, npi);
    if (chkr == 0) {    // Az új fa szerkezet nem OK
        QMessageBox::warning(recordView.pWidget(), trUtf8("Hibás adatok"), trUtf8("Nem megengedett szülő megadása. Hurok a fában."));
        return 0;
    }
    if (!cErrorMessageBox::condMsgBox(pRec->tryUpdateById(*pq))) {
        return 0;
    }
    int row = mi.row(); // A modosított rekord eredeti sorszáma a parentjében
    cTreeNode *pn = nodeFromIndex(mi);  // A modosított elem (node)
    cTreeNode *pp = pn->parent;         // A szülő
    switch (chkr) {
    case 1: {
        delete pn->pData;
        pn->pData = pRec;
        QModelIndex topLeft     = index(row, 0,                    mi.parent());
        QModelIndex bottomRight = index(row, _col2field.size() -1, mi.parent());
        dataChanged(topLeft, bottomRight);
        break;
    }
    case 2: {
        beginRemoveRows(mi.parent(), row, row);
        delete pp->pChildrens->takeAt(row)->pData; // pp->pChildrens->takeAt(row) === pn
        pn->pData = pRec;
        endRemoveRows();
        cTreeNode *pnp = nodeFromIndex(npi);
        row = pnp->rows();
        beginInsertRows(npi, row, row);
        pnp->addChild(pn);
        endInsertRows();
        break;
    }
    default:
        EXCEPTION(EPROGFAIL);
        return 0;
    }
    return chkr;
}

bool cRecordTreeModel::updateRow(const QModelIndex& mi, cRecord * pRec)
{
    (void)mi;
    (void)pRec;
    EXCEPTION(ENOTSUPP);
    return false; //
}

bool cRecordTreeModel::insertRow(cRecord *pRec)
{
    int ixPId = pRec->descr().ixToParent(EX_IGNORE);     // Az ősre mutató ID mező indexe
    // Az ős típusban nem biztos hogy van önmagára mutató ID mező (pl.: nports, interfaces)
    qlonglong pid = ixPId == NULL_IX ? NULL_ID : pRec->getId(ixPId);
    QModelIndex pmi;        // Gyökér
    if (pid != NULL_ID) {   // Ha nem a gyökér a parent
        pmi = findNode(pid);
        if (!pmi.isValid()) {   // nincs meg a parent!!
            DWAR() << "Parent not found, record : " << pRec->toString() << endl;
            return false;
        }
    }
    cTreeNode *ppn = nodeFromIndex(pmi);
    int row = rowCount(pmi);
    beginInsertRows(pmi, row, row);
    ppn->addChild(pRec);
    endInsertRows();
    return true;
}
