
#include "record_link.h"

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{
    ;
}


cRecordLink::~cRecordLink()
{
    ;
}

QStringList cRecordLink::where(QVariantList& qParams)
{
    QStringList wl;
    int ofix = recDescr().toIndex(_sNodeId1);
    wl << dQuoted(recDescr().columnName(ofix)) + " = " + QString::number(owner_id);
    wl << filterWhere(qParams);
    wl << refineWhere(qParams);
    return wl;
}

void cRecordLink::insert()
{
    /*
    if (flags & RTF_CHILD) {
        if (pUpper == NULL) EXCEPTION(EPROGFAIL);
        if (owner_id == NULL_ID) return;
    }
    if (pRecordDialog != NULL) {
        pDelete(pRecordDialog);
    }
    // A dialógusban megjelenítendő nyomógombok.
    int buttons = enum2set(DBT_OK, DBT_INSERT, DBT_CANCEL);
    cRecord rec(pRecDescr);
    // Ha CHILD, akkor a owner id adott
    if (flags & RTF_CHILD) {
        int ofix = rec.descr().ixToOwner();
        rec[ofix] = owner_id;
    }
    cRecordDialog   rd(*pTableShape, buttons, true, pWidget());  // A rekord szerkesztő dialógus
    pRecordDialog = &rd;
    rd.restore(&rec);
    while (1) {
        int r = rd.exec();
        if (r == DBT_INSERT || r == DBT_OK) {   // Csak az OK, és Insert gombra csinálunk valamit
            bool ok = rd.accept();
            if (ok) {
                cRecord *pRec = rd.record().dup();
                ok = pModel->insertRec(pRec);
                if (!ok) pDelete(pRec);
                else if (flags & RTF_IGROUP) {    // Group, tagja listába van a beillesztés?
                    ok = cGroupAny(*pRec, *(pUpper->actRecord())).insert(*pq, false);
                    if (!ok) {
                        QMessageBox::warning(pWidget(), design().titleError, trUtf8("A kijelölt tag felvétele az új csoportba sikertelen"),QMessageBox::Ok);
                        refresh();
                        break;
                    }
                }
            }
            if (ok) {
                if (r == DBT_OK) break; // Ha OK-t nyomott becsukjuk az dialóg-ot
                continue;               // Ha Insert-et, akkor folytathatja a következővel
            }
            else {
                //QMessageBox::warning(pWidget(), trUtf8("Az új rekord beszúrása sikertelen."), rd.errMsg());
                continue;
            }
        }
        break;
    }
    pRecordDialog = NULL;
*/
}

void cRecordLink::modify(eEx __ex)
{
    (void)__ex;
}

void cRecordLink::remove()
{

}

cLinkDialog::cLinkDialog(cRecordLink * __parent)
    : QDialog(__parent == NULL ? NULL : __parent->pWidget())
{
    parent = __parent;
    utter1 = utter2 = false;
    pPrt1  = pPrt2  = NULL;
    pActRecord    = NULL;
    parentOwnerId = NULL_ID;
    pq = newQuery();
    if (parent != NULL) {
        pActRecord    = parent->actRecord();
        parentOwnerId = parent->owner_id;
    }

    pUi = new Ui_DialogAddPhsLink;
    pUi->setupUi(this);

    pButtonsLink1Type = new QButtonGroup(this);
    pButtonsLink1Type->addButton(pUi->radioButtonLink1Term,  LT_TERM);
    pButtonsLink1Type->addButton(pUi->radioButtonLink1Front, LT_FRONT);
    pButtonsLink1Type->addButton(pUi->radioButtonLink1Back,  LT_BACK);

    pButtonsLink2Type = new QButtonGroup(this);
    pButtonsLink2Type->addButton(pUi->radioButtonLink2Term,  LT_TERM);
    pButtonsLink2Type->addButton(pUi->radioButtonLink2Front, LT_FRONT);
    pButtonsLink2Type->addButton(pUi->radioButtonLink2Back,  LT_BACK);

    pModelZone1 = new cRecordListModel(pgrp1.descr());
    pUi->comboBoxZone1->setModel(pModelZone1);
    pModelZone2 = new cRecordListModel(pgrp2.descr());
    pUi->comboBoxZone2->setModel(pModelZone2);

    pModelPlace1 = new cRecordListModel(plac1.descr());
    pUi->comboBoxPlace1->setModel(pModelPlace1);
    pModelPlace2 = new cRecordListModel(plac2.descr());
    pUi->comboBoxPlace2->setModel(pModelPlace2);

    pModelNode1 = new cRecordListModel(node1.descr());
    pUi->comboBoxNode1->setModel(pModelNode1);
    pModelNode2 = new cRecordListModel(node2.descr());
    pUi->comboBoxNode2->setModel(pModelNode2);

    pModelPort1 = new cRecordListModel(cNPort().descr());
    pUi->comboBoxPort1->setModel(pModelPort1);
    pModelPort2 = new cRecordListModel(cNPort().descr());
    pUi->comboBoxPort2->setModel(pModelPort2);

    init();
}

cLinkDialog::~cLinkDialog()
{
    delete pq;
    pDelete(pPrt1);
    pDelete(pPrt2);
}

void cLinkDialog::init()
{
    pModelZone1->setFilter();               // A zónára nincs szűrés
    pUi->comboBoxZone1->setCurrentText(_sAll);
    pModelZone2->setFilter();
    pUi->comboBoxZone2->setCurrentText(_sAll);
    pModelPlace1->setFilter();              // Az all zónára nem kell szürni
    pModelPlace2->setFilter();

    if (pActRecord != NULL) {
        // set by act record
        utter1 = utter2 = true;
        pPrt1 = cNPort::getPortObjById(*pq, pActRecord->getId(_sPortId1));
        pPrt2 = cNPort::getPortObjById(*pq, pActRecord->getId(_sPortId2));
        node1.setById(*pq, pActRecord->getId(_sNodeId1));
        node2.setById(*pq, pActRecord->getId(_sNodeId2));
        plac1.setById(*pq, node1.getId(_sPlaceId));
        plac2.setById(*pq, node2.getId(_sPlaceId));

        pUi->checkBoxPlaceEqu->setChecked(plac1 == plac2);

        pUi->comboBoxPlace1->setCurrentText(plac1.getName());
        pUi->comboBoxPlace2->setCurrentText(plac2.getName());

        pModelNode1->setFilter(nodeFilter(plac1), OT_ASC, FT_SQL_WHERE);
        pUi->comboBoxNode1->setCurrentText(node1.getName());
        pUi->lineEditNode1Type->setText(node1.getName(_sNodeType));
        pModelNode2->setFilter(nodeFilter(plac2), OT_ASC, FT_SQL_WHERE);
        pUi->comboBoxNode2->setCurrentText(node2.getName());
        pUi->lineEditNode2Type->setText(node2.getName(_sNodeType));

        pModelPort1->setFilter(portFilter(node1), OT_ASC, FT_SQL_WHERE);
        pUi->comboBoxPort1->setCurrentText(pPrt1->getName());
        pModelPort2->setFilter(portFilter(node2), OT_ASC, FT_SQL_WHERE);
        pUi->comboBoxPort2->setCurrentText(pPrt2->getName());

        int lt = pActRecord->getId(_sPhsLinkType1);
        pButtonsLink1Type->button(lt);
        switch (lt) {
        case LT_FRONT:
            pUi->comboBoxPort1Share->setEditable(true);
            pUi->comboBoxPort1Share->setCurrentText(pActRecord->getName(_sPortShared));
            pUi->radioButtonLink1Term->setCheckable(false);
            break;
        case LT_BACK:
            pUi->comboBoxPort1Share->setEditable(false);
            pUi->comboBoxPort1Share->setCurrentIndex(0);
            pUi->radioButtonLink1Term->setCheckable(false);
            break;
        case LT_TERM:
            pUi->comboBoxPort1Share->setEditable(false);
            pUi->comboBoxPort1Share->setCurrentIndex(0);
            pUi->radioButtonLink1Front->setCheckable(false);
            pUi->radioButtonLink1Back->setCheckable(false);
        }

        lt = pActRecord->getId(_sPhsLinkType2);
        pButtonsLink2Type->button(lt);
        switch (lt) {
        case LT_FRONT:
            pUi->comboBoxPort2Share->setEditable(true);
            pUi->comboBoxPort2Share->setCurrentText(pActRecord->getName(_sPortShared));
            pUi->radioButtonLink2Term->setCheckable(false);
            break;
        case LT_BACK:
            pUi->comboBoxPort2Share->setEditable(false);
            pUi->comboBoxPort2Share->setCurrentIndex(0);
            pUi->radioButtonLink2Term->setCheckable(false);
            break;
        case LT_TERM:
            pUi->comboBoxPort2Share->setEditable(false);
            pUi->comboBoxPort2Share->setCurrentIndex(0);
            pUi->radioButtonLink2Front->setCheckable(false);
            pUi->radioButtonLink2Back->setCheckable(false);
        }
    }
    else {
        qlonglong id;

        pDelete(pPrt1);
        pDelete(pPrt2);
        pUi->checkBoxPlaceEqu->setChecked(true);

        if (parentOwnerId != NULL_ID) {
            node1.setById(*pq, parentOwnerId);
            plac1.setById(*pq, node1.getId(_sPlaceId));
            plac2.clone(plac1);
            pUi->comboBoxPlace1->setCurrentText(plac1.getName());
            pUi->comboBoxPlace2->setCurrentText(plac2.getName());

            pModelNode1->setFilter(nodeFilter(plac1), OT_ASC, FT_SQL_WHERE);
            pUi->comboBoxNode1->setCurrentText(node1.getName());
            pUi->lineEditNode1Type->setText(node1.getName(_sNodeType));
        }
        else {
            pUi->comboBoxPlace1->setCurrentIndex(0);
            pUi->comboBoxPlace2->setCurrentIndex(0);
            id = pModelPlace1->atId(0);
            plac1.setById(*pq, id);
            plac2.clone(plac1);
        }
        /* ....... */
    }

}

QString cLinkDialog::nodeFilter(const cPlace& place)
{
    if (place.isEmpty_()) return _sFalse;
    const static QString sql = "place_id = %1";
    return sql.arg(place.getId());
}

QString cLinkDialog::portFilter(const cPatch &node)
{
    if (node.isEmpty_()) return _sFalse;
    const static QString sql = "node_id = %1";
    return sql.arg(node.getId());
}

void cLinkDialog::zone1CurrentIndex(int i)
{

}

void cLinkDialog::zone2CurrentIndex(int i)
{

}
void cLinkDialog::place1CurrentIndex(int i)
{

}
void cLinkDialog::place2CurrentIndex(int i)
{

}
void cLinkDialog::node1CurrentIndex(int i)
{

}
void cLinkDialog::node2CurrentIndex(int i)

{

}
void cLinkDialog::port1CurrentIndex(int i)
{

}
void cLinkDialog::pore2CurrentIndex(int i)
{

}
void cLinkDialog::port1ShareCurrentIndex(int i)
{

}


