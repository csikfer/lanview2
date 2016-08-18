#include "lv2g.h"
#include "phslinkform.h"
#include "ui_phslinkform.h"

phsLinkWidget::phsLinkWidget(cLinkDialog * par) :  QWidget(par)
{
    parent = par;
    pUi = new Ui::phsLinkForm;
    pUi->setupUi(this);
    pq = newQuery();
    pPrt   = NULL;
    pOther = NULL;
    first  = false;

    pButtonsLinkType = new QButtonGroup(this);
    pButtonsLinkType->addButton(pUi->radioButtonLinkTerm,  LT_TERM);
    pButtonsLinkType->addButton(pUi->radioButtonLinkFront, LT_FRONT);
    pButtonsLinkType->addButton(pUi->radioButtonLinkBack,  LT_BACK);

    pModelZone = new cRecordListModel(pgrp.descr());
    pUi->comboBoxZone->setModel(pModelZone);
    pgrp.setById(*pq, ALL_PLACE_GROUP_ID);
    const static QString sf =
            "0 < (SELECT COUNT(*) FROM place_group_places WHERE place_group_places.place_group_id = place_groups.place_group_id)";
    pModelZone->setConstFilter(sf, FT_SQL_WHERE);
    pModelZone->setFilter();               // A zónára nincs szűrés (csak az üres zónák vannak kizárva)

    pModelPlace = new cRecordListModel(plac.descr());
    pUi->comboBoxPlace->setModel(pModelPlace);
    pModelNode = new cRecordListModel(node.descr());
    pUi->comboBoxNode->setModel(pModelNode);
    pModelPort = new cRecordListModel(cNPort().descr());
    pUi->comboBoxPort->setModel(pModelPort);

    connect(pButtonsLinkType,       SIGNAL(buttonClicked(int)),         this, SLOT(changeLinkType(int)));
    connect(pUi->comboBoxZone,      SIGNAL(currentIndexChanged(int)),   this, SLOT(zoneCurrentIndex(int)));
    connect(pUi->comboBoxPlace,     SIGNAL(currentIndexChanged(int)),   this, SLOT(placeCurrentIndex(int)));
    connect(pUi->comboBoxNode,      SIGNAL(currentIndexChanged(int)),   this, SLOT(nodeCurrentIndex(int)));
    connect(pUi->comboBoxPort,      SIGNAL(currentIndexChanged(int)),   this, SLOT(portCurrentIndex(int)));
    connect(pUi->comboBoxPortShare, SIGNAL(currentTextChanged(QString)),this, SLOT(portShareCurrentText(QString)));
}


phsLinkWidget::~phsLinkWidget()
{
    delete pUi;
    delete pq;
    pDelete(pPrt);
}

void phsLinkWidget::setFirst(bool f)
{
    first = f;
    QString s;
    if (f) {
        s = "1";
        sPhsLinkTypeX = _sPhsLinkType1;
        pUi->checkBoxPlaceEqu->hide();
    }
    else {
        s = "2";
        sPhsLinkTypeX = _sPhsLinkType2;
        connect(pUi->checkBoxPlaceEqu, SIGNAL(toggled(bool)),  this, SLOT(toglePlaceEqu(bool)));
    }
    sPortIdX     = _sPortId     + s;
    sNodeIdX     = _sNodeId     + s;
    sPortSharedX = _sPortShared + s;
}

void phsLinkWidget::init()
{
    pUi->comboBoxZone->setCurrentText(_sAll);
    pModelPlace->setFilter();              // Az all zónára nem kell szürni
    qlonglong id;
    if (parent->pActRecord != NULL) {
        // set by act record
        utter = true;

        id = parent->pActRecord->getId(sNodeIdX);
        node.setById(*pq, id);
        id = node.getId(_sPlaceId);
        plac.setById(*pq, id);

        if (!first) pUi->checkBoxPlaceEqu->setChecked(plac == pOther->plac);

        pUi->comboBoxPlace->setCurrentText(plac.getName());
        nodeFilter();
        pUi->comboBoxNode->setCurrentText(node.getName());
        pUi->lineEditNodeType->setText(node.getName(_sNodeType));
        portFilter();
        id = parent->pActRecord->getId(sPortIdX);
        pPrt = cNPort::getPortObjById(*pq, id, EX_ERROR);
        pUi->comboBoxPort->setCurrentText(pPrt->getName());

        linkType  = parent->pActRecord->getId(sPhsLinkTypeX);
        if (linkType == LT_FRONT) shared = parent->pActRecord->getName(sPortSharedX);
        else                      shared = _sNul;
    }
    else {
        pDelete(pPrt);
        node.clear();

        if (!first) pUi->checkBoxPlaceEqu->setChecked(true);

        if (first && parent->parentOwnerId != NULL_ID) {
            id = parent->parentOwnerId;
            node.setById(*pq, id);
            id = node.getId(_sPlaceId);
            plac.setById(*pq, id);
            pUi->comboBoxPlace->setCurrentText(plac.getName());

            nodeFilter();
            pUi->comboBoxNode->setCurrentText(node.getName());
            pUi->lineEditNodeType->setText(node.getName(_sNodeType));
            portFilter();
        }
        else {
            if (parent->parentOwnerId != NULL_ID) {
                plac.clone(pOther->plac);
                pUi->comboBoxPlace->setCurrentText(plac.getName());
            }
            else {
                pUi->comboBoxPlace->setCurrentIndex(0);
                id = pModelPlace->atId(0);
                plac.setById(*pq, id);
            }
            nodeFilter();
            pUi->comboBoxNode->setCurrentIndex(0);
            id = pModelNode->atId(0);
            if (id == NULL_ID) {
                pUi->comboBoxNode->setDisabled(true);
                pModelPort->setFilter(_sFaile, OT_ASC, FT_SQL_WHERE);
            }
            else {
                pUi->comboBoxNode->setEnabled(true);
                node.setById(id);
                portFilter();
            }
        }
        pUi->comboBoxPort->setCurrentIndex(0);
        id = pModelPort->atId(0);
        if (id == NULL_ID) {
            pUi->comboBoxPort->setDisabled(true);
            linkType = NULL_ID;
        }
        else {
            pPrt = cNPort::getPortObjById(*pq, id);
            if (pPrt->descr() == cPPort().descr()) linkType = LT_FRONT;
            else                                   linkType = LT_TERM;
        }
    }
    uiSetLinkType();
}

void phsLinkWidget::placeFilter()
{
    const static QString _sql = "is_group_place(place_id, %1)";
    QString sql = _sql.arg(pgrp.getId());
    pModelPlace->setFilter(sql, OT_ASC, FT_SQL_WHERE);
}

bool phsLinkWidget::nodeFilter()
{
    bool r;
    if (plac.isEmpty_()) {
        pModelNode->setFilter(_sFalse, OT_ASC, FT_SQL_WHERE);
        node.clear();
        r = false;
    }
    else  {
        const static QString _sql = "is_parent_place(place_id, %1)";
        QString sql = _sql.arg(plac.getId());
        pModelNode->setFilter(sql, OT_ASC, FT_SQL_WHERE);
        r = pModelNode->rowCount() > 0;
    }
    if (r) {
        node.clear();
        portFilter();
    }
    pUi->comboBoxNode->setEnabled(r);
    return r;
}

bool phsLinkWidget::portFilter()
{
    bool r;
    if (node.isEmpty_()) {
        pModelNode->setFilter(_sFalse, OT_ASC, FT_SQL_WHERE);
        r = false;
    }
    else {
        const static QString _sql = "node_id = %1";
        QString sql =  _sql.arg(node.getId());
        pModelPort->setFilter(sql, OT_ASC, FT_SQL_WHERE);
        r = pModelPort->rowCount() > 0;
    }
    if (!r) pDelete(pPrt);
    pUi->comboBoxPort->setEnabled(r);
    return r;
}

void phsLinkWidget::uiSetLinkType()
{
    int checkedId = pButtonsLinkType->checkedId();
    switch (linkType) {
    case LT_FRONT:
        if (checkedId != LT_FRONT) pButtonsLinkType->button(LT_FRONT)->setChecked(true);
        pUi->comboBoxPortShare->setEnabled(true);
        pUi->comboBoxPortShare->setCurrentText(shared);
        pUi->radioButtonLinkTerm->setCheckable(false);
        // Mindkét oldalon nem lehet a shared (pOther a konstruktorban még NULL)
        if (shared.isEmpty() == false && pOther != NULL && pOther->shared.isEmpty() == false) {
            pOther->uiSetShared(_sNul);
        }
        break;
    case LT_BACK:
        if (checkedId != LT_BACK) pButtonsLinkType->button(LT_BACK)->setChecked(true);
        pUi->comboBoxPortShare->setEnabled(false);
        pUi->comboBoxPortShare->setCurrentIndex(0);
        pUi->radioButtonLinkTerm->setCheckable(false);
        shared = _sNul;
        break;
    case LT_TERM:
        if (checkedId != LT_TERM) pButtonsLinkType->button(LT_TERM)->setChecked(true);
        pUi->comboBoxPortShare->setEnabled(false);
        pUi->comboBoxPortShare->setCurrentIndex(0);
        pUi->radioButtonLinkFront->setCheckable(false);
        pUi->radioButtonLinkBack->setCheckable(false);
        shared = _sNul;
        break;
    default:
        if (checkedId != -1) pButtonsLinkType->button(checkedId)->setChecked(false);
        pUi->comboBoxPortShare->setEnabled(false);
        pUi->comboBoxPortShare->setCurrentIndex(0);
        pUi->radioButtonLinkFront->setCheckable(false);
        pUi->radioButtonLinkBack->setCheckable(false);
        pUi->radioButtonLinkTerm->setCheckable(false);
        shared = _sNul;
        break;
    }
}

void phsLinkWidget::uiSetShared(const QString& _sh)
{
    shared = _sh;
    pUi->comboBoxPortShare->setCurrentText(shared);
}

bool phsLinkWidget::firstGetPlace()
{
    qlonglong id = plac.getId();
    if (id != NULL_ID) {
        int i = pModelPlace->indexOf(id);
        if (i >= 0) {
            pUi->comboBoxPlace->setCurrentIndex(i);
            return false;
        }
    }
    pUi->comboBoxPlace->setCurrentIndex(0);
    id = pModelPlace->atId(0);
    plac.setById(*pq, id);
    return true;
}

bool phsLinkWidget::firstGetNode()
{
    qlonglong id = node.getId();
    if (id != NULL_ID) {
        int i = pModelNode->indexOf(id);
        if (i >= 0) {
            pUi->comboBoxNode->setCurrentIndex(i);
            return false;
        }
    }
    pUi->comboBoxNode->setCurrentIndex(0);
    id = pModelNode->atId(0);
    node.setById(*pq, id);
    return true;
}

bool phsLinkWidget::firstGetPort()
{
    if (pPrt != NULL) {
        int i = pModelPort->indexOf(pPrt->getId());
        if (i >= 0) {
            pUi->comboBoxNode->setCurrentIndex(i);
            return false;
        }
    }
    pUi->comboBoxPort->setCurrentIndex(0);
    qlonglong id = pModelPort->atId(0);
    pDelete(pPrt);
    pPrt = cNPort::getPortObjById(*pq, id);
    return true;
}

void phsLinkWidget::changeLinkType(int id)
{
    if (id != LT_FRONT) {
        uiSetShared(_sNul);
    }
}

void phsLinkWidget::toglePlaceEqu(bool f)
{
    if (f) {
        pgrp.clone(pOther->pgrp);
        int i = pOther->pUi->comboBoxZone->currentIndex();
        pUi->comboBoxZone->setCurrentIndex(i);
        qlonglong id = plac.getId();
        if (id == pOther->plac.getId()) {
            i = pModelPlace->atId(id);
            pUi->comboBoxPlace->setCurrentIndex(i);
        }
        else {
            i = pOther->pUi->comboBoxPlace->currentIndex();
            pUi->comboBoxPlace->setCurrentIndex(i);
            placeCurrentIndex(i);
        }
    }
    pUi->comboBoxZone->setDisabled(f);
    pUi->comboBoxPlace->setDisabled(f);
}

void phsLinkWidget::zoneCurrentIndex(int i)
{
    qlonglong id = pModelZone->atId(i);
    if (pgrp.getId() == id) return;
    pgrp.setById(*pq, id);
    placeFilter();
    firstGetPlace() && nodeFilter() && firstGetNode() && portFilter() && firstGetPort();
}

void phsLinkWidget::placeCurrentIndex(int i)
{
    qlonglong id = pModelPlace->atId(i);
    if (plac.getId() == id) return;
    plac.setById(*pq, id);
    nodeFilter() && firstGetNode() && portFilter() && firstGetPort();
}

void phsLinkWidget::nodeCurrentIndex(int i)
{
    qlonglong id = pModelNode->atId(i);
    if (node.getId() == id) return;
    node.setById(*pq, id);
    portFilter() && firstGetPort();
}

void phsLinkWidget::portCurrentIndex(int i)
{
    qlonglong id = pModelPort->atId(i);
    if (pPrt != NULL && pPrt->getId() == id) return;
    pDelete(pPrt);
    pPrt = cNPort::getPortObjById(*pq, id);
}

void phsLinkWidget::portShareCurrentText(const QString& s)
{
    shared = s;
    if (s.isEmpty()) return;
    if (pPrt == NULL || !pUi->radioButtonLinkFront->isChecked()) {
        uiSetShared(_sNul);
    }
    else {
        pOther->uiSetShared(_sNul);
    }
}


