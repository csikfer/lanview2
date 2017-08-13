#include "deducepatch.h"

cSelectNode::cSelectNode(QComboBox *_pZone, QComboBox *_pPLace, QComboBox *_pNode, QLineEdit *_pFilt, const QString& _cFilt)
    : QObject()
    , pComboBoxZone(_pZone)
    , pComboBoxPLace(_pPLace)
    , pComboBoxNode(_pNode)
    , pLineEditFilt(_pFilt)
    , constFilter(_cFilt)
{
    blockSignal = false;
    pZoneModel = new cZoneListModel(this);
    pComboBoxZone->setModel(pZoneModel);
    pPlaceModel = new cPlacesInZoneModel(this);
    pComboBoxPLace->setModel(pPlaceModel);
    pComboBoxPLace->setCurrentIndex(0);
    pNodeModel = new cRecordListModel(cPatch().descr(), this);
    pNodeModel->nullable = true;
    pComboBoxNode->setModel(pNodeModel);
    if (!constFilter.isEmpty()) {
        pNodeModel->setConstFilter(constFilter, FT_SQL_WHERE);
    }
    connect(pComboBoxZone,  SIGNAL(currentIndexChanged(int)),   this, SLOT(zoneChanged(int)));
    connect(pComboBoxPLace, SIGNAL(currentIndexChanged(int)),   this, SLOT(placeChanged(int)));
    if (_pFilt != NULL) {
        connect(pLineEditFilt, SIGNAL(textChanged(QString)),    this, SLOT(patternChanged(QString)));
    }
    connect(pComboBoxNode,  SIGNAL(currentIndexChanged(int)),   this, SLOT(_nodeChanged(QString)));
}

void cSelectNode::zoneChanged(int ix)
{
    qlonglong id = pZoneModel->atId(ix);
    pPlaceModel->setFilter(id);
}

void cSelectNode::placeChanged(int ix)
{
    qlonglong nid = pNodeModel->atId(pComboBoxNode->currentIndex());
    blockSignal = true;
    qlonglong pid = pPlaceModel->atId(ix);
    pNodeModel->setFilter(pid);
    int nix = pNodeModel->indexOf(nid);
    if (nix < 0) {  // Nincs már megfelelő node -> NULL
        pComboBoxNode->setCurrentIndex(0);
        blockSignal = false;
        _nodeChanged(pNodeModel->at(0));
    }
    else {          // A node változatlan (csak az indexe változhatott)
        pComboBoxNode->setCurrentIndex(nix);
        blockSignal = false;
    }
}

void cSelectNode::patternChanged(const QString& s)
{
    qlonglong nid = pNodeModel->atId(pComboBoxNode->currentIndex());
    blockSignal = true;
    if (s.isEmpty()) {
        pNodeModel->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    }
    else {
        pNodeModel->setFilter(s, OT_DEFAULT, FT_LIKE);
    }
    int nix = pNodeModel->indexOf(nid);
    if (nix < 0) {  // Nincs már megfelelő node -> NULL
        pComboBoxNode->setCurrentIndex(0);
        blockSignal = false;
        _nodeChanged(pNodeModel->at(0));
    }
    else {          // A node változatlan (csak az indexe változhatott)
        pComboBoxNode->setCurrentIndex(nix);
        blockSignal = false;
    }
}

void cSelectNode::_nodeChanged(const QString& s)
{
    if (blockSignal) return;
    nodeChanged(s);
    int ix = pComboBoxNode->currentIndex();
    qlonglong id = pNodeModel->atId(ix);
    nodeChanged(id);
}

/* *** */

enum eFieldIx {
    CX_PPORT_LEFT, RX_SHARE_LEFT, CX_NODE_LEFT, CX_NPORT_LEFT,
    CX_STATE, CS_SAVE,
    CX_NPORT_RIGHT, CX_NODE_RIGHT, RX_SHARE_RIGHT, CX_PPORT_RIGHT
};


cDPRow::cDPRow(QSqlQuery& q, cDeducePatch *par, int _row)
    :QObject(par), pTable(par->pUi->tableWidget), row(_row)
{

}

/* --- */

cDeducePatch::cDeducePatch(QMdiArea *par)
    :cIntSubObj(par)
{
    static const QString sql = "'patch' = SOME (node_type)";
    nidLeft = nidRight = NULL_ID;
    pSelNodeLeft  = new cSelectNode(pUi->comboBoxZoneLeft,  pUi->comboBoxPlaceLeft,  pUi->comboBoxPatchLeft,  NULL, sql);
    pSelNodeRight = new cSelectNode(pUi->comboBoxZoneRight, pUi->comboBoxPlaceRight, pUi->comboBoxPatchRight, NULL, sql);

}

cDeducePatch::~cDeducePatch()
{

}

void cDeducePatch::clearTable()
{
    foreach (cDPRow *p, rows) {
        delete p;
    }
    rows.clear();
    pUi->tableWidget->setRowCount(0);
    pUi->pushButtonSave->setDisabled(true);
}

void cDeducePatch::changedNode()
{
    clearTable();
    bool f = nidLeft == NULL_ID || nidRight == NULL_ID;
    pUi->pushButtonStart->setDisabled(f);
}

void cDeducePatch::changeNodeLeft(qlonglong id)
{
    nidLeft = id;
    changedNode();
}

void cDeducePatch::changeNodeRight(qlonglong id)
{
    nidRight = id;
    changedNode();
}

void cDeducePatch::start()
{
    QSqlQuery q = getQuery();
    if (nidLeft == NULL_ID || nidRight == NULL_ID) {
        EXCEPTION(EPROGFAIL);
    }
    patchLeft.setById(q, nidLeft);
    patchRight.setById(q, nidRight);
    endsIdRight.clear();
    endsIdLeft.clear();
}

void cDeducePatch::save()
{

}
