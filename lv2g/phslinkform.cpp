#include "lv2g.h"
#include "phslinkform.h"
#include "ui_phslinkform.h"
#include "popupreport.h"

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
        s = "1";
        sPhsLinkTypeX = _sPhsLinkType1;
        pUi->checkBoxPlaceEqu->hide();
    }
    else {
        setObjectName("last");
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
    cPhsLink l;
    if (parent->pActRecord != NULL) {
        // set by act record
        l.set(*parent->pActRecord);
        if (first) l.swap();
        pSelectPort->setLink(l);
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
            pUi->checkBoxPlaceEqu->setChecked(true);
        }
    }
    if (!first) toglePlaceEqu(pUi->checkBoxPlaceEqu->isChecked());
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

void phsLinkWidget::change(qlonglong, int, int)
{
    changed();
}

void phsLinkWidget::on_toolButton_clicked()
{
    qlonglong nid = pSelectPort->currentNodeId();
    if (nid == NULL_ID) return;
    popupReportNode(this, *pq, nid);
}
