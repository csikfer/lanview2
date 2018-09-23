#include "lv2g.h"
#include "phslinkform.h"
#include "ui_phslinkform.h"
#include "popupreport.h"
#include "object_dialog.h"

phsLinkWidget::phsLinkWidget(cLinkDialog * par) :  QWidget(par)
{
    parent = par;
    pUi = new Ui::phsLinkForm;
    pUi->setupUi(this);
    pq     = newQuery();
    pOther = NULL;
    first  = false;

    pButtonsLinkType = new QButtonGroup(this);
    pButtonsLinkType->addButton(pUi->radioButtonLinkTerm,  LT_TERM);
    pButtonsLinkType->addButton(pUi->radioButtonLinkFront, LT_FRONT);
    pButtonsLinkType->addButton(pUi->radioButtonLinkBack,  LT_BACK);

    pSelectPort = new cSelectLinkedPort(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->comboBoxNode, pUi->comboBoxPort,
                                        pButtonsLinkType, pUi->comboBoxPortShare, tIntVector(),
                                        pUi->lineEditPlacePattern, pUi->lineEditNodePattern, _sNul, _sNul, this);

    connect(pSelectPort, SIGNAL(changedLink(qlonglong,int,int)), this, SLOT(change(qlonglong,int,int)));
}


phsLinkWidget::~phsLinkWidget()
{
    delete pUi;
    delete pq;
}

void phsLinkWidget::setFirst(bool f)
{
    first = f;
    QString s;
    if (f) {
        setObjectName("first");
        pUi->checkBoxPlaceEqu->hide();
        pUi->toolButtonAdd->hide();
    }
    else {
        setObjectName("last");
        connect(pUi->checkBoxPlaceEqu, SIGNAL(toggled(bool)),  this, SLOT(toglePlaceEqu(bool)));

    }
}

void phsLinkWidget::init()
{
    cPhsLink l;
    if (parent->pActRecord != NULL) {
        // set by act record, copy eq. field (phs_links_shape -> phs_links)
        l.set(*parent->pActRecord);
        // phs_links.port_shared <- phs_links_shape.port_shared1 or phs_links_shape.port_shared2
        QString sFn = _sPortShared;
        if (l.getId(_sPhsLinkType1) == LT_FRONT) sFn += "1";
        else                                     sFn += "2";
        l.setName(_sPortShared, parent->pActRecord->getName(sFn));
        if (first) l.swap();
        pSelectPort->setLink(l);
        if (!first) {
            bool equ = getPlaceId() == pOther->getPlaceId();
            pUi->checkBoxPlaceEqu->setChecked(equ);
        }
    }
    else {
        if (first) {
            qlonglong nid = parent->parentNodeId;
            if (nid != NULL_ID) {
                cPatch  no;
                no.setById(*pq, nid);
                qlonglong pid = no.getId(_sPlaceId);
                if (pid != NULL_ID) {
                    pSelectPort->setPlaceId(pid);
                }
            }
            qlonglong pid = parent->parentPortId;
            if (pid != NULL_ID) {
                cNPort po;
                po.setById(*pq, pid);
                l.setId(_sPortId2, pid);
                bool isPatch = po.getId(_sIfTypeId) == cIfType::ifTypeId(_sPatch);
                l.setId(_sPhsLinkType2, isPatch ? LT_FRONT : LT_TERM);
                pSelectPort->setLink(l);
            }
            else if (nid != NULL_ID) {
                pSelectPort->setCurrentNode(nid);
            }
        }
        else {
            pUi->checkBoxPlaceEqu->setChecked(true);
            pOther->pSelectPort->setSlave(pSelectPort);
        }
    }
    if (pSelectPort->isPatch() && pSelectPort->currentPortId() != NULL_ID) {
        pUi->toolButtonStep->show();
    }
    else {
        pUi->toolButtonStep->hide();
    }
}

bool phsLinkWidget::next()
{
    bool r;
    int n = pUi->comboBoxPort->count();
    int i = pUi->comboBoxPort->currentIndex();
    ++i;
    r = i < n;
    if (r) pUi->comboBoxPort->setCurrentIndex(i);
    return r;
}

bool phsLinkWidget::prev()
{
    bool r;
    int i = pUi->comboBoxPort->currentIndex();
    --i;
    r = i >= 0;
    if (r) pUi->comboBoxPort->setCurrentIndex(i);
    return r;

}

void phsLinkWidget::toglePlaceEqu(bool f)
{
    if (first) EXCEPTION(EPROGFAIL);
    pOther->pSelectPort->setSlave(f ? pSelectPort : NULL);
    changed();
}

void phsLinkWidget::change(qlonglong, int _lt, int)
{
    if (LT_UNKNOWN == _lt || LT_TERM == _lt ) pUi->toolButtonStep->hide();
    else                                      pUi->toolButtonStep->show();
    qlonglong nid = pSelectPort->currentNodeId();
    pUi->toolButtonInfo->setDisabled(nid == NULL_ID);
    if (pSelectPort->currentType() == LT_FRONT && pOther->pSelectPort->currentType() == LT_FRONT) {
        // Front <--> Front : Share not supported!
        pUi->comboBoxPortShare->setCurrentIndex(0);
        pUi->comboBoxPortShare->setDisabled(true);
        pOther->pUi->comboBoxPortShare->setCurrentIndex(0);
        pOther->pUi->comboBoxPortShare->setDisabled(true);
    }
    else if (pSelectPort->currentType() == LT_FRONT && pOther->pSelectPort->currentType() == LT_TERM) {
        pUi->comboBoxPortShare->setDisabled(false);
    }
    else if (pSelectPort->currentType() == LT_TERM && pOther->pSelectPort->currentType() == LT_FRONT) {
        pOther->pUi->comboBoxPortShare->setDisabled(false);
    }
    changed();
}

void phsLinkWidget::on_toolButtonInfo_clicked()
{
    qlonglong nid = pSelectPort->currentNodeId();
    if (nid == NULL_ID) return;
    popupReportNode(this, *pq, nid);
}

void phsLinkWidget::on_toolButtonStep_clicked()
{
    qlonglong pid = pSelectPort->currentPortId();
    if (pSelectPort->isPatch() && pid != NULL_ID) {
        int plt;
        switch (getLinkType()) {
        case LT_FRONT:  plt = LT_BACK;  break;
        case LT_BACK:   plt = LT_FRONT; break;
        default:        EXCEPTION(EDATA, getLinkType());
        }
        cPhsLink l, ll;
        if (first) {
            l.setId(_sPortId1,      pid);
            l.setId(_sPhsLinkType1, plt);
        }
        else {
            l.setId(_sPortId2,      pid);
            l.setId(_sPhsLinkType2, plt);
        }
        ll.set(l);
        if (ll.completion(*pq)) l.set(ll);
        phsLinkWidget *pFirst = first ? this   : pOther;
        phsLinkWidget *pLast  = first ? pOther : this;
        pFirst->pSelectPort->setLink(l);
        l.swap();
        pLast->pSelectPort->setLink(l);
    }
}

void phsLinkWidget::on_toolButtonAdd_clicked()
{
    cPatch sample;
    sample.setId(_sPlaceId, pSelectPort->currentPlaceId());
    cPatch *p = patchInsertDialog(*pq, this, &sample);
    if (p != NULL) {
        pSelectPort->setCurrentNode(p->getId());
        delete p;
    }
}

