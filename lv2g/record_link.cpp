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
    buttons << DBT_SPACER;
    if (pTableShape->isFeature(_sButtonCopy) || pTableShape->isFeature(_sReport)) {
        if (pTableShape->isFeature(_sButtonCopy)) buttons << DBT_COPY;
        if (pTableShape->isFeature(_sReport))     buttons << DBT_REPORT;
        buttons << DBT_SPACER;
    }

    buttons << DBT_REFRESH << DBT_FIRST << DBT_PREV << DBT_NEXT << DBT_LAST;
    buttons << DBT_SPACER;
    if (!isNoDelete) buttons << DBT_DELETE;
    if (!isReadOnly) buttons << DBT_SIMILAR;
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
    case DBT_CLOSE:     close();        break;
    case DBT_REFRESH:   refresh();      break;
    case DBT_INSERT:    edit();         break;
    case DBT_SIMILAR:   edit(true);     break;
    case DBT_FIRST:     first();        break;
    case DBT_PREV:      prev();         break;
    case DBT_NEXT:      next();         break;
    case DBT_LAST:      last();         break;
    case DBT_DELETE:    remove();       break;
    case DBT_COPY:      copy();         break;
    case DBT_REPORT:    report();       break;
    // case DBT_COMPLETE:  lldp2phs();     break;
    default:
        DWAR() << "Invalid button id : " << id << endl;
        break;
    }
    DBGFNL();

}


void cRecordLink::edit(bool _similar, eEx __ex)
{
    (void)_similar;     // Itt nincs
    if (isReadOnly) {
        if (_similar) cRecordTable::modify(__ex);
        return;
    }
    cLinkDialog dialog(_similar, this);
    int r;
    cPhsLink link;
    while (true) {
        dialog.exec();
        r = dialog.result();
        switch (r) {
        case DBT_CLOSE:
        case DBT_CANCEL:
            break;
        case DBT_SAVE:
        case DBT_OK:
            if (dialog.get(link)) { // insert
                if (!cErrorMessageBox::condMsgBox(link.tryInsert(*pq))) continue;
            }
            else {                  // replace
                if (!cErrorMessageBox::condMsgBox(link.tryReplace(*pq))) continue;
            }
            refresh(false);
            if (r == DBT_SAVE) {
                dialog.changed();
                link.clear();
                continue;
            }
            break;
        default:
            EXCEPTION(EPROGFAIL, r);
        }
        break;
    }
}

/* void cRecordLink::lldp2phs()
{
    if (isReadOnly || linkType != LT_LLDP) return;
    cRecord *pa = actRecord();
    if (pa == NULL) return;
    modify();
}*/

void cRecordLink::modifyByIndex(const QModelIndex & index)
{
    (void)index;
    edit(true);
}


cLinkDialog::cLinkDialog(bool __similar, cRecordLink * __parent)
    : QDialog(__parent == NULL ? NULL : __parent->pWidget())
{
    parent        = __parent;
    pActRecord    = NULL;
    parentNodeId  = NULL_ID;
    parentPortId  = NULL_ID;
    pq = newQuery();
    if (parent != NULL) {
        if (!__similar) {
            cRecordsViewBase *pParentParent = parent->pUpper;   // A parent tábla perent-je
            if (pParentParent != NULL) {
                cRecord *pActParRec = pParentParent->actRecord();    // Az aktuális rekord
                if (pActParRec != NULL) {
                    if (pActParRec->descr() >= cPatch::_descr_cPatch()) {       // node ?
                        parentNodeId = pActParRec->getId();
                    }
                    else if (pActParRec->descr() >= cNPort::_descr_cNPort()) {  // port ?
                        parentPortId = pActParRec->getId();
                        parentNodeId = pActParRec->getId(_sNodeId);
                    }
                    else {
                        EXCEPTION(EPROGFAIL, 0, pActParRec->identifying());
                    }
                }
            }
        }
        else {
            pActRecord = parent->actRecord();
        }
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
       pToolButtonRfresh     = new QToolButton();
       pToolButtonRfresh->setIcon(QIcon(":/icons/refresh.ico"));
       pCheckBoxCollisions->setChecked(false);
       pTextEditCollisions   = new QTextEdit;
       pTextEditCollisions->setReadOnly(true);
      pVBoxL2 = new QVBoxLayout;
      pVBoxL2->addWidget(pLabelCollisions);
      pVBoxL2->addWidget(pCheckBoxCollisions);
      pVBoxL2->addWidget(pToolButtonRfresh);
      pVBoxL2->addStretch();
     pHBoxL = new QHBoxLayout;
     pHBoxL->addLayout(pVBoxL2);
     pHBoxL->addWidget(pTextEditCollisions);
     pVBoxL->addLayout(pHBoxL);

     pVBoxL->addWidget(line());

      tIntVector buttons;
      buttons << DBT_SPACER << DBT_OK << DBT_SAVE << DBT_PREV << DBT_NEXT << DBT_SPACER << DBT_CANCEL;
      pButtons = new cDialogButtons(buttons);
      pButtons->button(DBT_PREV)->setToolTip(trUtf8("A port indexek léptetése lefelé."));
      pButtons->button(DBT_NEXT)->setToolTip(trUtf8("A port indexek léptetése felfelé."));
      pVBoxL->addWidget(pButtons->pWidget());

    insertOnly = false;
    init();

    connect(pButtons,           SIGNAL(buttonPressed(int)), this, SLOT(buttonPressed(int)));
    connect(pLink1,             SIGNAL(changed()),          this, SLOT(changed()));
    connect(pLink2,             SIGNAL(changed()),          this, SLOT(changed()));
    connect(pCheckBoxCollisions,SIGNAL(toggled(bool)),      this, SLOT(collisionsTogled(bool)));
    connect(pTextEditNote,      SIGNAL(textChanged()),      this, SLOT(modifyNote()));
    connect(pPushButtonNote,    SIGNAL(pressed()),          this, SLOT(saveNote()));
    connect(pToolButtonRfresh,  SIGNAL(pressed()),          this, SLOT(changed()));

    changed();
}

cLinkDialog::~cLinkDialog()
{
    delete pq;
}

void cLinkDialog::init()
{
    pLink1->init();
    pLink2->init();
}

bool cLinkDialog::get(cPhsLink& link)
{
    qlonglong pid1 = pLink1->getPortId();
    qlonglong pid2 = pLink2->getPortId();
    ePhsLinkType lt1 = pLink1->getLinkType();
    ePhsLinkType lt2 = pLink2->getLinkType();
    ePortShare   ps  = ES_;
    if      (lt1 == LT_FRONT && lt2 == LT_TERM)  ps  = pLink1->getPortShare();
    else if (lt1 == LT_TERM  && lt2 == LT_FRONT) ps  = pLink2->getPortShare();
    link.setId(_sPortId1, pid1);
    link.setId(_sPortId2, pid2);
    link.setId(_sPhsLinkType1, lt1);
    link.setId(_sPhsLinkType2, lt2);
    link.setId(_sPortShared, ps);
    link.setName(_sPhsLinkNote, pTextEditNote->toPlainText());
    return insertOnly;
}

bool cLinkDialog::next()
{
    bool r = pLink1->next();
    return pLink2->next() && r;
}

bool cLinkDialog::prev()
{
    bool r = pLink1->prev();
    return pLink2->prev() && r;
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
    ePortShare   ps  = ES_;
    if      (lt1 == LT_FRONT && lt2 == LT_TERM)  ps  = pLink1->getPortShare();
    else if (lt1 == LT_TERM  && lt2 == LT_FRONT) ps  = pLink2->getPortShare();
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
            pButtons->disable(ENUM2SET2(DBT_SAVE, DBT_OK));
            pButtons->enable(ENUM2SET2(DBT_NEXT, DBT_PREV));
            pTextEditCollisions->setText(trUtf8("Mentett, létező link."));
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
        pButtons->disableExcept(ENUM2SET4(DBT_CANCEL, DBT_CLOSE, DBT_NEXT, DBT_PREV));
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

void cLinkDialog::buttonPressed(int kid)
{
    switch (kid) {
    case DBT_CLOSE:
    case DBT_CANCEL:
    case DBT_SAVE:
    case DBT_OK:
        done(kid);
        break;
    case DBT_NEXT:
        next();
        break;
    case DBT_PREV:
        prev();
        break;
    default:
        EXCEPTION(EPROGFAIL, kid);
    }
}

