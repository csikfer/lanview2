#include "lv2g.h"
#include "phslinkform.h"
#include "ui_phslinkform.h"


qlonglong phsLinkWidget::getPortId() const
{
    return pPrt == NULL ? NULL_ID : pPrt->getId();
}
ePhsLinkType phsLinkWidget::getLinkType() const
{
    return (ePhsLinkType)linkType;
}
ePortShare   phsLinkWidget::getPortShare() const
{
    if (linkType == LT_FRONT) return (ePortShare)portShare(shared);
    else return (ePortShare)portShare(pOther->shared);
}

#define BEGIN_SLOT  if (stat != READY) return; stat = SET
#define END_SLOT    stat = READY
#define SAVE_STAT   eState _save_stat_ = stat; stat = SET
#define REST_STAT   stat = _save_stat_
#define SAVE_OSTAT  eState _save_o_stat_ = pOther->stat; pOther->stat = SET
#define REST_OSTAT  pOther->stat = _save_o_stat_

phsLinkWidget::phsLinkWidget(cLinkDialog * par) :  QWidget(par)
{
    stat = INIT;
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
    // Az nports tábla helyett a patchable_ports view-t használjuk, hogy csak a patch-elhető portokat listázza.
    const cRecStaticDescr *ppDescr = cRecStaticDescr::get("patchable_ports");
    pModelPort = new cRecordListModel(*ppDescr);
    pUi->comboBoxPort->setModel(pModelPort);

    connect(pButtonsLinkType,       SIGNAL(buttonToggled(int,bool)),    this, SLOT(changeLinkType(int, bool)));
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
    stat = INIT;
    pUi->comboBoxZone->setCurrentText(_sAll);
    pModelPlace->setFilter();              // Az all zónára nem kell szürni
    qlonglong id;
    if (parent->pActRecord != NULL) {
        // set by act record
        id = parent->pActRecord->getId(sNodeIdX);
        node.setById(*pq, id);
        id = node.getId(_sPlaceId);
        plac.setById(*pq, id);

        if (!first) {
            bool f = plac == pOther->plac;
            pUi->checkBoxPlaceEqu->setChecked(f);
            pUi->comboBoxZone->setDisabled(f);
            pUi->comboBoxPlace->setDisabled(f);
        }
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

        if (!first) {
            pUi->checkBoxPlaceEqu->setChecked(true);
            pUi->comboBoxZone->setDisabled(true);
            pUi->comboBoxPlace->setDisabled(true);
        }

        if (first && parent->parentOwnerId != NULL_ID) {
            const cRecordsViewBase *pParentTableView = parent->parent->pUpper;
            const cRecStaticDescr* pParentRecDescr = pParentTableView->pRecDescr;
            // Két parentre számítunk: node vagy port
            if (pParentRecDescr->isFieldName(_sPortId)) {   // port
                id = parent->parentOwnerId;
                pPrt = cNPort::getPortObjById(*pq, id);
                id = pPrt->getId(_sNodeId);
                node.setById(*pq, id);
                id = node.getId(_sPlaceId);
                plac.setById(*pq, id);
                pUi->comboBoxPlace->setCurrentText(plac.getName());
                if (!nodeFilter()) EXCEPTION(EDATA);
                pUi->comboBoxNode->setCurrentText(node.getName());
                pUi->lineEditNodeType->setText(node.getName(_sNodeType));
                if (portFilter()) EXCEPTION(EDATA);
                pUi->comboBoxPort->setCurrentText(pPrt->getName());
            }
            else {                                          // node
                id = parent->parentOwnerId;
                node.setById(*pq, id);
                id = node.getId(_sPlaceId);
                plac.setById(*pq, id);
                pUi->comboBoxPlace->setCurrentText(plac.getName());

                if (!nodeFilter()) EXCEPTION(EDATA);
                pUi->comboBoxNode->setCurrentText(node.getName());
                pUi->lineEditNodeType->setText(node.getName(_sNodeType));
                portFilter() && firstGetPort();
            }
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
                pUi->lineEditNodeType->setText(_sNul);
            }
            else {
                pUi->comboBoxNode->setEnabled(true);
                node.setById(id);
                pUi->lineEditNodeType->setText(node.getName(_sNodeType));
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
    stat = READY;
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
        r = false;
    }
    else  {
        const static QString _sql = "is_parent_place(place_id, %1)";
        QString sql = _sql.arg(plac.getId());
        pModelNode->setFilter(sql, OT_ASC, FT_SQL_WHERE);
        r = pModelNode->rowCount() > 0;
    }
    if (!r) {
        node.clear();
        pUi->lineEditNodeType->setText(_sNul);
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
    SAVE_STAT;
    int checkedId = pButtonsLinkType->checkedId();
    switch (linkType) {
    case LT_FRONT:
        pUi->radioButtonLinkFront->setCheckable(true);
        pUi->radioButtonLinkBack->setCheckable(true);
        if (checkedId != LT_FRONT) pButtonsLinkType->button(LT_FRONT)->setChecked(true);
        pUi->radioButtonLinkTerm->setCheckable(false);
        break;
    case LT_BACK:
        pUi->radioButtonLinkFront->setCheckable(true);
        pUi->radioButtonLinkBack->setCheckable(true);
        if (checkedId != LT_BACK) pButtonsLinkType->button(LT_BACK)->setChecked(true);
        pUi->comboBoxPortShare->setEnabled(false);
        pUi->comboBoxPortShare->setCurrentIndex(0);
        pUi->radioButtonLinkTerm->setCheckable(false);
        shared = _sNul;
        break;
    case LT_TERM:
        pUi->radioButtonLinkTerm->setCheckable(true);
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
    uiSetShared();
    REST_STAT;
}

void phsLinkWidget::uiSetShared()
{
    bool enable;
    //
    enable = linkType == LT_FRONT && pOther->linkType != LT_FRONT;
    if (!enable) shared = _sNul;
    pUi->comboBoxPortShare->setCurrentText(shared);
    pUi->comboBoxPortShare->setEnabled(enable);
    // Other
    SAVE_OSTAT;
    enable = linkType != LT_FRONT && pOther->linkType == LT_FRONT;
    if (!enable) pOther->shared = _sNul;
    pOther->pUi->comboBoxPortShare->setCurrentText(pOther->shared);
    pOther->pUi->comboBoxPortShare->setEnabled(enable);
    REST_OSTAT;
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
    pUi->lineEditNodeType->setText(node.getName(_sNodeType));
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
    if (pPrt->descr() == cPPort().descr()) linkType = LT_FRONT;
    else                                   linkType = LT_TERM;
    uiSetLinkType();
    return true;
}

void phsLinkWidget::changeLinkType(int id, bool f)
{
    BEGIN_SLOT;
    if (f && linkType != id) {
        linkType = id;
        uiSetShared();
        changed();
    }
    END_SLOT;
}

void phsLinkWidget::toglePlaceEqu(bool f)
{
    BEGIN_SLOT;
    if (f && !(pgrp == pOther->pgrp && plac == pOther->plac)) {
        if (pgrp != pOther->pgrp) {
            pgrp.clone(pOther->pgrp);
            int i = pOther->pUi->comboBoxZone->currentIndex();
            pUi->comboBoxZone->setCurrentIndex(i);
        }
        if (plac != pOther->plac) {
            plac.clone(pOther->plac);
            int i = pOther->pUi->comboBoxPlace->currentIndex();
            pUi->comboBoxPlace->setCurrentIndex(i);
            nodeFilter() && firstGetNode() && portFilter() && firstGetPort();
        }
    }
    pUi->comboBoxZone->setDisabled(f);
    pUi->comboBoxPlace->setDisabled(f);
    changed();
    END_SLOT;
}

void phsLinkWidget::zoneCurrentIndex(int i)
{
    BEGIN_SLOT;
    qlonglong id = pModelZone->atId(i);
    if (pgrp.getId() != id) {
        pgrp.setById(*pq, id);
        placeFilter();
        firstGetPlace() && nodeFilter() && firstGetNode() && portFilter() && firstGetPort();
        if (first && pOther->pUi->checkBoxPlaceEqu->isChecked() && pgrp != pOther->pgrp) {
            pOther->toglePlaceEqu(true);
        }
    }
    changed();
    END_SLOT;
}

void phsLinkWidget::placeCurrentIndex(int i)
{
    BEGIN_SLOT;
    qlonglong id = pModelPlace->atId(i);
    if (plac.getId() != id) {
        plac.setById(*pq, id);
        nodeFilter() && firstGetNode() && portFilter() && firstGetPort();
    }
    changed();
    END_SLOT;
}

void phsLinkWidget::nodeCurrentIndex(int i)
{
    BEGIN_SLOT;
    qlonglong id = pModelNode->atId(i);
    if (node.getId() != id) {
        node.setById(*pq, id);
        pUi->lineEditNodeType->setText(node.getName(_sNodeType));
        portFilter() && firstGetPort();
    }
    changed();
    END_SLOT;
}

void phsLinkWidget::portCurrentIndex(int i)
{
    BEGIN_SLOT;
    qlonglong id = pModelPort->atId(i);
    if (pPrt == NULL || pPrt->getId() != id) {
        pDelete(pPrt);
        pPrt = cNPort::getPortObjById(*pq, id);
        if (pPrt->descr() == cPPort().descr()) linkType = LT_FRONT;
        else                                   linkType = LT_TERM;
        uiSetLinkType();
    }
    changed();
    END_SLOT;
}

void phsLinkWidget::portShareCurrentText(const QString& s)
{
    BEGIN_SLOT;
    shared = s;
    uiSetShared();
    changed();
    END_SLOT;
}


