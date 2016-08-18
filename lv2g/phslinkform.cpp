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
    pModelZone->setFilter();               // A zónára nincs szűrés

    pModelPlace = new cRecordListModel(plac.descr());
    pUi->comboBoxPlace->setModel(pModelPlace);
    pModelNode = new cRecordListModel(node.descr());
    pUi->comboBoxNode->setModel(pModelNode);
    pModelPort = new cRecordListModel(cNPort().descr());
    pUi->comboBoxPort->setModel(pModelPort);
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
        pUi->checkBoxPlaceEqu->hide();
        s = "1";
        sPhsLinkTypeX = _sPhsLinkType1;
    }
    else {
        s = "2";
        sPhsLinkTypeX = _sPhsLinkType2;
    }
    sPortIdX     = _sPortId     + s;
    sNodeIdX     = _sNodeId     + s;
    sPortSharedX = _sPortShared + s;
}

void phsLinkWidget::init()
{
    pUi->comboBoxZone->setCurrentText(_sAll);
    pModelPlace->setFilter();              // Az all zónára nem kell szürni

    if (parent->pActRecord != NULL) {
        // set by act record
        utter = true;

        pPrt = cNPort::getPortObjById(*pq, parent->pActRecord->getId(sPortIdX));
        node.setById(*pq, parent->pActRecord->getId(sNodeIdX));
        plac.setById(*pq, node.getId(_sPlaceId));

        if (!first) pUi->checkBoxPlaceEqu->setChecked(plac == pOther->plac);

        pUi->comboBoxPlace->setCurrentText(plac.getName());
        nodeFilter();
        pUi->comboBoxNode->setCurrentText(node.getName());
        pUi->lineEditNodeType->setText(node.getName(_sNodeType));
        portFilter();
        pUi->comboBoxPort->setCurrentText(pPrt->getName());

        linkType  = parent->pActRecord->getId(sPhsLinkTypeX);
        if (linkType == LT_FRONT) shared = parent->pActRecord->getName(sPortSharedX);
        else                      shared = _sNul;
    }
    else {
        qlonglong id;
        pDelete(pPrt);
        node.clear();

        if (!first) pUi->checkBoxPlaceEqu->setChecked(true);

        if (first && parent->parentOwnerId != NULL_ID) {
            node.setById(*pq, parent->parentOwnerId);
            plac.setById(*pq, node.getId(_sPlaceId));
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
    ;
}

bool phsLinkWidget::nodeFilter()
{
    if (plac.isEmpty_()) {
        pModelNode->setFilter(_sFalse, OT_ASC, FT_SQL_WHERE);
        return false;
    }
    const static QString _sql = "is_parent_place(place_id, %1)";
    QString sql = _sql.arg(plac.getId());
    pModelNode->setFilter(sql, OT_ASC, FT_SQL_WHERE);
    return pModelNode->rowCount() > 0;
}

bool phsLinkWidget::portFilter()
{
    if (node.isEmpty_()) {
        pModelNode->setFilter(_sFalse, OT_ASC, FT_SQL_WHERE);
        return false;
    }
    const static QString _sql = "node_id = %1";
    QString sql =  _sql.arg(node.getId());
    pModelPort->setFilter(sql, OT_ASC, FT_SQL_WHERE);
    return pModelPort->rowCount() > 0;
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

void phsLinkWidget::zoneCurrentIndex(int i)
{
    qlonglong id = pModelZone->atId(i);
    if (pgrp.getId() == id) return;
    pgrp.setById(*pq, id);
    // ...
}
void phsLinkWidget::placeCurrentIndex(int i)
{
    (void)i;
}
void phsLinkWidget::nodeCurrentIndex(int i)
{
    (void)i;
}
void phsLinkWidget::portCurrentIndex(int i)
{
    (void)i;
}
void phsLinkWidget::portShareCurrentIndex(int i)
{
    (void)i;
}


