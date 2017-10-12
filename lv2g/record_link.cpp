#include "phslinkform.h"
#include "lv2link.h"

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{
    QString sViewName = pts->getName(_sTableName);
    if      (sViewName.startsWith("phs"))  linkType = LT_PHISICAL;
    else if (sViewName.startsWith("log"))  linkType = LT_LOGICAL;
    else if (sViewName.startsWith("lldp")) linkType = LT_LLDP;
    else                                   EXCEPTION(EDATA, 0, sViewName);
}

cRecordLink::~cRecordLink()
{
    ;
}

void cRecordLink::init()
{
    pTimer = NULL;
    pTableView = NULL;
    // A read-only flag eket újra ki kell értékelni! (most mind true)
    isReadOnly = pTableShape->getBool(_sTableShapeType, TS_READ_ONLY);
    isNoDelete = isReadOnly || false == lanView::isAuthorized(pTableShape->getId(_sRemoveRights));
    isNoInsert = isReadOnly || false == lanView::isAuthorized(pTableShape->getId(_sInsertRights));
    isReadOnly = isReadOnly || false == lanView::isAuthorized(pTableShape->getId(_sEditRights));

    // Az alapértelmezett gombok:
    buttons << DBT_SPACER << DBT_REFRESH << DBT_FIRST << DBT_PREV << DBT_NEXT << DBT_LAST;
    buttons << DBT_SPACER;
    if (!isNoDelete) buttons << DBT_DELETE;
    if (!isReadOnly) buttons << DBT_MODIFY;
    if (!isNoInsert) buttons << DBT_INSERT;
    flags = 0;
    if (pUpper != NULL) shapeType |= ENUM2SET(TS_CHILD);
    switch (shapeType) {
    case ENUM2SET2(TS_CHILD, TS_LINK):
        if (pUpper == NULL) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD;
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_LINK):
        flags = RTF_SINGLE;
        initSimple(_pWidget);
        break;
    default:
        EXCEPTION(ENOTSUPP, pTableShape->getId(_sTableShapeType),
                  trUtf8("TABLE %1 SHAPE %2 TYPE : %3")
                  .arg(pTableShape->getName(),
                       pTableShape->getName(_sTableName),
                       pTableShape->getName(_sTableShapeType))
                  );
    }
    pTableView->horizontalHeader()->setSectionsClickable(true);
    connect(pTableView->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(clickedHeader(int)));
    if (!(shapeType & ENUM2SET(TS_READ_ONLY))) {
        connect(pTableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(modifyByIndex(QModelIndex)));
    }
    // Auto refresh ?
    qlonglong ar = pTableShape->getId(_sAutoRefresh);
    if (ar > 0) {
        pTimer = new QTimer(this);
        connect(pTimer, SIGNAL(timeout()), this, SLOT(autoRefresh()));
        pTimer->setInterval(ar);
    }
/*  pTableView->setDragDropMode(QAbstractItemView::DragOnly);
    pTableView->setDragEnabled(true); */
    // Change model
    cRecordLinkModel *pm = new cRecordLinkModel(*this);
    pTableView->setModel(pm);
    delete pModel;
    pModel = pm;
    refresh();
}


QStringList cRecordLink::where(QVariantList& qParams)
{
    QStringList wl;
    if (pUpper != NULL) {
        int ofix;
        // node or port ?
        if (pUpper->pRecDescr->isFieldName(_sPortId)) {
            ofix = recDescr().toIndex(_sPortId1);
        }
        else {
            ofix = recDescr().toIndex(_sNodeId1);
        }
        wl << dQuoted(recDescr().columnName(ofix)) + " = " + QString::number(owner_id);
    }
    wl << filterWhere(qParams);
    wl << refineWhere(qParams);
    return wl;
}

void cRecordLink::buttonPressed(int id)
{
    _DBGFN() << " #" << id << endl;
    switch (id) {
    case DBT_CLOSE:     close();    break;
    case DBT_REFRESH:   refresh();  break;
    case DBT_INSERT:    insert();   break;
    case DBT_MODIFY:    modify();   break;
    case DBT_FIRST:     first();    break;
    case DBT_PREV:      prev();     break;
    case DBT_NEXT:      next();     break;
    case DBT_LAST:      last();     break;
    case DBT_DELETE:    remove();   break;
    case DBT_COPY:      copy();     break;
    case DBT_COMPLETE:  lldp2phs(); break;
    default:
        DWAR() << "Invalid button id : " << id << endl;
        break;
    }
    DBGFNL();

}


void cRecordLink::insert(bool _similar)
{
    (void)_similar;     // Itt nincs
    if (isReadOnly) {
        return;
    }
    cLinkDialog dialog(true, this);
    int r;
    do {
        dialog.exec();
        r = dialog.result();
        if (DBT_CANCEL != r) {
            cPhsLink link;
            if (dialog.get(link)) { // insert
                if (!cErrorMessageBox::condMsgBox(link.tryInsert(*pq))) continue;
            }
            else {                  // replace
                if (!cErrorMessageBox::condMsgBox(link.tryReplace(*pq))) continue;
            }
            refresh(false);
        }
    } while (r == DBT_NEXT);
}

void cRecordLink::modify(eEx __ex)
{
    if (isReadOnly) {
        cRecordTable::modify(__ex);
    }
    else {
        cLinkDialog dialog(false, this);
        int r;
        do {
            dialog.exec();
            r = dialog.result();
            if (DBT_CANCEL != r) {
                cPhsLink link;
                if (dialog.get(link)) { // insert
                    if (!cErrorMessageBox::condMsgBox(link.tryInsert(*pq))) continue;
                }
                else {                  // replace
                    if (!cErrorMessageBox::condMsgBox(link.tryReplace(*pq))) continue;
                }
                refresh(false);
            }
        } while (r == DBT_NEXT);
    }
}

void cRecordLink::lldp2phs()
{
    if (isReadOnly || linkType != LT_LLDP) return;
    cRecord *pa = actRecord();
    if (pa == NULL) return;
    modify();
}

cLinkDialog::cLinkDialog(bool isInsert, cRecordLink * __parent)
    : QDialog(__parent == NULL ? NULL : __parent->pWidget())
{
    parent = __parent;
    pActRecord    = NULL;
    parentOwnerId = NULL_ID;
    pq = newQuery();
    if (parent != NULL) {
        if (!isInsert) pActRecord = parent->actRecord();
        parentOwnerId = parent->owner_id;
    }
    pLink1 = new phsLinkWidget(this);
    pLink2 = new phsLinkWidget(this);
    pLink1->pOther = pLink2;
    pLink2->pOther = pLink1;
    pLink1->setFirst(true);
    pLink2->setFirst(false);

    QHBoxLayout *pHBoxL;
    QVBoxLayout *pVBoxL;
    QVBoxLayout *pVBoxL2;

    pVBoxL = new QVBoxLayout;
     setLayout(pVBoxL);
     pVBoxL->addWidget(pLink1);
     pVBoxL->addWidget(line(2,4));
     pVBoxL->addWidget(pLink2);
     pVBoxL->addWidget(line());

      pHBoxL = new QHBoxLayout;
       pVBoxL2 = new QVBoxLayout;
        pVBoxL2->addWidget(new QLabel(trUtf8("Megjegyzés a linkhez :")));
        pVBoxL2->addStretch();
        pPushButtonNote = new QPushButton(trUtf8("Megjegyzés mentése"));
        pPushButtonNote->setToolTip(trUtf8("Meglévő link esetén a megadott megjegyzés mentése."));
        pVBoxL2->addWidget(pPushButtonNote);
      pHBoxL->addLayout(pVBoxL2);
      pTextEditNote = new QTextEdit;
      pHBoxL->addWidget(pTextEditNote);
     pVBoxL->addLayout(pHBoxL);
     pVBoxL->addWidget(line());

       pLabelCollisions      = new QLabel(trUtf8("Ütköző linkek :"));
       pCheckBoxCollisions   = new QCheckBox(trUtf8("Automatikus törlés"));
       pCheckBoxCollisions->setChecked(false);
       pTextEditCollisions   = new QTextEdit;
       pTextEditCollisions->setReadOnly(true);
      pVBoxL2 = new QVBoxLayout;
      pVBoxL2->addWidget(pLabelCollisions);
      pVBoxL2->addWidget(pCheckBoxCollisions);
      pVBoxL2->addStretch();
     pHBoxL = new QHBoxLayout;
     pHBoxL->addLayout(pVBoxL2);
     pHBoxL->addWidget(pTextEditCollisions);
     pVBoxL->addLayout(pHBoxL);

     pVBoxL->addWidget(line());

      tIntVector buttons;
      buttons << DBT_SPACER << DBT_SAVE << DBT_NEXT << DBT_SPACER << DBT_CANCEL;
      pButtons = new cDialogButtons(buttons);
      pVBoxL->addWidget(pButtons->pWidget());

    connect(pButtons,           SIGNAL(buttonPressed(int)), this, SLOT(done(int)));
    connect(pLink1,             SIGNAL(changed()),          this, SLOT(changed()));
    connect(pLink2,             SIGNAL(changed()),          this, SLOT(changed()));
    connect(pCheckBoxCollisions,SIGNAL(toggled(bool)),      this, SLOT(collisionsTogled(bool)));
    connect(pTextEditNote,      SIGNAL(textChanged()),      this, SLOT(modifyNote()));
    connect(pPushButtonNote,    SIGNAL(pressed()),          this, SLOT(saveNote()));

    insertOnly = false;
    init();
}

cLinkDialog::~cLinkDialog()
{
    delete pq;
}

void cLinkDialog::init()
{
    pLink1->init();
    pLink2->init();
    changed();
}

bool cLinkDialog::get(cPhsLink& link)
{
    qlonglong pid1 = pLink1->getPortId();
    qlonglong pid2 = pLink2->getPortId();
    ePhsLinkType lt1 = pLink1->getLinkType();
    ePhsLinkType lt2 = pLink2->getLinkType();
    ePortShare   ps  = pLink1->getPortShare();
    link.setId(_sPortId1, pid1);
    link.setId(_sPortId2, pid2);
    link.setId(_sPhsLinkType1, lt1);
    link.setId(_sPhsLinkType2, lt2);
    link.setId(_sPortShared, ps);
    link.setName(_sPhsLinkNote, pTextEditNote->toPlainText());
    return insertOnly;
}

void cLinkDialog::changed()
{
    insertOnly = false;
    imperfect  = true;
    exists     = false;
    cPhsLink link;
    qlonglong pid1 = pLink1->getPortId();
    qlonglong pid2 = pLink2->getPortId();
    ePhsLinkType lt1 = pLink1->getLinkType();
    ePhsLinkType lt2 = pLink2->getLinkType();
    ePortShare   ps  = pLink1->getPortShare();
    QString msg;
    if (pid1 == NULL_ID && pid2 == NULL_ID) {
        pButtons->disableExcept();
        pTextEditCollisions->setText(trUtf8("Nincs megadva egy port sem."));
        pCheckBoxCollisions->setChecked(false);
        pCheckBoxCollisions->setCheckable(false);
        return;
    }
    if (pid1 == pid2) {
        pButtons->disableExcept();
        pTextEditCollisions->setText(trUtf8("A két oldal azonos."));
        pCheckBoxCollisions->setChecked(false);
        pCheckBoxCollisions->setCheckable(false);
        return;
    }
    if (pid1 == NULL_ID || pid2 == NULL_ID) {
        pButtons->disableExcept();
        msg = trUtf8("Csak a link egyik fele van megadva.<br>");
    }
    else {
        link.setId(_sPortId1, pid1);
        link.setId(_sPortId2, pid2);
        link.setId(_sPhsLinkType1, lt1);
        link.setId(_sPhsLinkType2, lt2);
        link.setId(_sPortShared, ps);
        int rows = link.completion(*pq);
        if (rows == 1) {
            exists = true;
            linkId = link.getId();
            pButtons->disableExcept();
            pTextEditCollisions->setText(trUtf8("A megadott link létezik."));
            pCheckBoxCollisions->setChecked(false);
            pCheckBoxCollisions->setCheckable(false);
            pTextEditNote->setText(link.getNote());
            return;
        }
        if (rows > 0) EXCEPTION(AMBIGUOUS, rows, link.toString());
        imperfect = false;
    }
    tRecordList<cPhsLink>   list;
    if (pid1 != NULL_ID) link.collisions(*pq, list, pid1, lt1, ps);
    if (pid2 != NULL_ID) link.collisions(*pq, list, pid2, lt2, ps);
    if (list.count() == 0) {
        insertOnly = true;
        if (imperfect) pButtons->disableExcept();
        else           pButtons->enabeAll();
        msg += trUtf8("Nincs ütközö/törlendő link.");
        pCheckBoxCollisions->setChecked(false);
        pCheckBoxCollisions->setCheckable(false);
        pTextEditCollisions->setText(msg);
    }
    else {
        msg += trUtf8("ütközö/törlendő linkek :");
        pButtons->disableExcept();
        pCheckBoxCollisions->setChecked(false);
        pCheckBoxCollisions->setCheckable(true);
        pTextEditCollisions->setText(msg);
        while (list.count()) {
            cPhsLink *p = list.pop_back();
            QString s = "<br><b>";
            s += cNPort::getFullNameById(*pq, p->getId(_sPortId1));
            QString lt = p->getName(_sPhsLinkType1);
            if (lt != _sTerm) s += "<i>:" + lt + "</i>";
            s += " </b> &lt;==&gt; <b>";
            s += cNPort::getFullNameById(*pq, p->getId(_sPortId2));
            lt = p->getName(_sPhsLinkType2);
            if (lt != _sTerm) s += "<i>:" + lt + "</i></b>";
            QString shared = p->getName(_sPortShared);
            if (!shared.isEmpty()) s += trUtf8("<i>; Megosztás</i> <b>%1</b>").arg(shared);
            delete p;
            msg += s;
        }
        pTextEditCollisions->setText(msg);
    }
}

void cLinkDialog::collisionsTogled(bool f)
{
    if (f) {
        if (!imperfect) pButtons->enabeAll();
    }
    else {
        if (!insertOnly) pButtons->disableExcept();
    }
}

void cLinkDialog::saveNote()
{
    if (exists) {
        cPhsLink link;
        link.setId(linkId);
        link.setName(_sPhsLinkNote, pTextEditNote->toPlainText());
        link.update(*pq, false, link.mask(_sPhsLinkNote));
        parent->refresh(false);
    }
}

void cLinkDialog::modifyNote()
{
    pPushButtonNote->setEnabled(exists);
}

