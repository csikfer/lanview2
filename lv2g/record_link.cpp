#include "phslinkform.h"
#include "lv2link.h"
#include "report.h"

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
    , dialogRect(0,0,0,0)
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
    pTimer = nullptr;
    pTableView = nullptr;
    // A read-only flag eket újra ki kell értékelni! (most mind true)
    isReadOnly = pTableShape->getBool(_sTableShapeType, TS_READ_ONLY);
    isNoDelete = isReadOnly || linkType == LT_LOGICAL  || false == lanView::isAuthorized(pTableShape->getId(_sRemoveRights));
    isNoInsert = isReadOnly || linkType != LT_PHISICAL || false == lanView::isAuthorized(pTableShape->getId(_sInsertRights));
    isReadOnly = isReadOnly || linkType != LT_PHISICAL || false == lanView::isAuthorized(pTableShape->getId(_sEditRights));

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
    if (pUpper != nullptr) shapeType |= ENUM2SET(TS_CHILD);
    switch (shapeType & ~ENUM2SET(TS_BARE)) {
    case ENUM2SET2(TS_CHILD, TS_LINK):
        if (pUpper == nullptr) EXCEPTION(EDATA);
        flags = RTF_SLAVE | RTF_CHILD;
        initSimple(_pWidget);
        break;
    case ENUM2SET(TS_LINK):
        flags = RTF_SINGLE;
        initSimple(_pWidget);
        break;
    default:
        EXCEPTION(ENOTSUPP, pTableShape->getId(_sTableShapeType),
                  tr("TABLE %1 SHAPE %2 TYPE : %3")
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
        pTimer->setInterval(int(ar));
    }
/*  pTableView->setDragDropMode(QAbstractItemView::DragOnly);
    pTableView->setDragEnabled(true); */
    // Change model
    cRecordLinkModel *pm = new cRecordLinkModel(*this);
    pTableView->setModel(pm);
    delete pModel;
    pModel = pm;
    // refresh();
}


QStringList cRecordLink::where(QVariantList& qParams)
{
    QStringList wl;
    if (pUpper != nullptr) {
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
    if (isReadOnly) {
        if (_similar) cRecordTable::modify(__ex);
        return;
    }
    cLinkDialog dialog(_similar, this);
    int r = DBT_SAVE;
    cPhsLink link;
    while (r == DBT_SAVE) {
        if (dialogRect.height()) {
            dialog.setGeometry(dialogRect);
        }
        dialog.exec();
        dialogRect = dialog.geometry();
        r = dialog.result();
        switch (r) {
        case DBT_CLOSE:
        case DBT_CANCEL:
            break;
        case DBT_SAVE:
        case DBT_OK:
            dialog.get(link);
            if (!cErrorMessageBox::condMsgBox(link.tryReplace(*pq))) continue;
            if (r == DBT_SAVE) {
                dialog.changed();
                link.clear();
                continue;
            }
            break;
        default:
            EXCEPTION(EPROGFAIL, r);
        }
        refresh(false);
    }
}

void cRecordLink::modifyByIndex(const QModelIndex & index)
{
    (void)index;
    if (linkType == LT_PHISICAL) {
        edit(true);
    }
}


cLinkDialog::cLinkDialog(bool __similar, cRecordLink * __parent)
    : QDialog(__parent == nullptr ? nullptr : __parent->pWidget())
{
    parent        = __parent;
    pActRecord    = nullptr;
    parentNodeId  = NULL_ID;
    parentPortId  = NULL_ID;
    noteChanged   = false;
    blockChange   = false;
    pq = newQuery();
    if (parent != nullptr) {
        if (!__similar) {
            cRecordsViewBase *pParentParent = parent->pUpper;   // A parent tábla perent-je
            if (pParentParent != nullptr) {
                cRecord *pActParRec = pParentParent->actRecord();    // Az aktuális rekord
                if (pActParRec != nullptr) {
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
    QHBoxLayout *pHBoxL2;
    QVBoxLayout *pVBoxL;
    QVBoxLayout *pVBoxL2;

    pVBoxL = new QVBoxLayout;
    // #========================#
    // | select port 1 (pLink1) |
    // #========================#
    // | select port 2 (pLink2) |
    // #========================#
    // | note                   |
    // #------------------------#
    // | report                 |
    // #------------------------#
    // | buttons                |
    // #========================#
     setLayout(pVBoxL);
     pVBoxL->addWidget(pLink1);
     pVBoxL->addWidget(line(2,4));
     pVBoxL->addWidget(pLink2);
     pVBoxL->addWidget(line(2,4));
    // note
    // +------------+------------+
    // | label      |            |
    // +------------+  note text |
    // | <-> button |            |
    // +------------+------------+
      pHBoxL = new QHBoxLayout;
       pVBoxL2 = new QVBoxLayout;
        QLabel *pLab = new QLabel(tr("Megjegyzés a linkhez :"));
        pVBoxL2->addWidget(pLab);
         pHBoxL2 = new QHBoxLayout;
          pHBoxL2->addStretch();
          pToolButtonNoteNull = new QToolButton();
          pToolButtonNoteNull->setIcon(lv2g::iconNull);
          pToolButtonNoteNull->setCheckable(true);
          pHBoxL2->addWidget(pToolButtonNoteNull);
        pVBoxL2->addLayout(pHBoxL2);
        pVBoxL2->addStretch();
      pHBoxL->addLayout(pVBoxL2, 0);
      pTextEditNote = new QTextEdit;
      int s; // = pLab->height(); Ez még véletlenül sem a cimke magassága :(
      if (parent == nullptr) s = 48;
      else s = parent->pButtons->buttons().first()->height();
      pTextEditNote->setFixedHeight(3 * s);
      // pTextEditNote->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      pHBoxL->addWidget(pTextEditNote, 1);
     pVBoxL->addLayout(pHBoxL);
     pVBoxL->addWidget(line());
     // report
     // +------------+-------------+
     // | label      |             |
     // | checkbox   | report text |
     // | refr.butt. |             |
     // +------------+-------------+
       pLabelCollisions      = new QLabel(tr("Riport :"));
       pCheckBoxCollisions   = new QCheckBox(tr("Automatikus törlés(ek)"));
       pToolButtonRefresh    = new QToolButton();
       pToolButtonRefresh->setIcon(QIcon(":/icons/refresh.ico"));
       pCheckBoxCollisions->setChecked(false);
       pTextEditReport   = new QTextEdit;
       pTextEditReport->setReadOnly(true);
      pVBoxL2 = new QVBoxLayout;
      pVBoxL2->addWidget(pLabelCollisions);
      pVBoxL2->addWidget(pCheckBoxCollisions);
      pVBoxL2->addWidget(pToolButtonRefresh);
      pVBoxL2->addStretch();
     // report layout
     pHBoxL = new QHBoxLayout;
     pHBoxL->addLayout(pVBoxL2);
     pHBoxL->addWidget(pTextEditReport);
     pVBoxL->addLayout(pHBoxL, 1);

     pVBoxL->addWidget(line());
    // buttons
      tIntVector buttons;
      buttons << DBT_SPACER << DBT_DELETE<< DBT_SPACER << DBT_OK << DBT_SAVE << DBT_PREV << DBT_NEXT << DBT_SPACER << DBT_CANCEL;
      pButtons = new cDialogButtons(buttons);
      pButtons->button(DBT_PREV)->setToolTip(tr("A port indexek léptetése lefelé."));
      pButtons->button(DBT_NEXT)->setToolTip(tr("A port indexek léptetése felfelé."));
      pVBoxL->addWidget(pButtons->pWidget());

    collision = false;
    init();

    connect(pButtons,           SIGNAL(buttonPressed(int)), this, SLOT(buttonPressed(int)));
    connect(pLink1,             SIGNAL(changed()),          this, SLOT(changed()));
    connect(pLink2,             SIGNAL(changed()),          this, SLOT(changed()));
    connect(pCheckBoxCollisions,SIGNAL(toggled(bool)),      this, SLOT(collisionsTogled(bool)));
    connect(pTextEditNote,      SIGNAL(textChanged()),      this, SLOT(modifyNote()));
    connect(pToolButtonNoteNull,SIGNAL(toggled(bool)),      this, SLOT(noteNull(bool)));
    connect(pToolButtonRefresh, SIGNAL(pressed()),          this, SLOT(changed()));

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

void cLinkDialog::get(cPhsLink& link)
{
    link.clear();
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
    if (!pToolButtonNoteNull->isChecked()) {
        QString note = pTextEditNote->toPlainText();
        if (note.isEmpty()) {
            pToolButtonNoteNull->setChecked(true);
        }
        else {
            link.setNote(note);
        }
    }
    blockChange = false;
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

void cLinkDialog::remove()
{
    if (exists) {
        cPhsLink link;
        get(link);
        int rows = link.completion(*pq);
        if (rows == 1) link.remove(*pq);
        changed();
    }
}

static bool checkShare(QSqlQuery& _q, qlonglong _pid, ePhsLinkType _lt, ePortShare _sh, QString& _msg)
{
    if (_pid == NULL_ID)  return true;  // imperfect
    if (_lt  != LT_FRONT) return false;
    cPPort pp;
    pp.setById(_q, _pid);
    ePortShare sc = ePortShare(pp.getId(_sSharedCable));
    switch (sc) {
    case ES_:
        break;
    case ES_NC:
        _msg += htmlError(QObject::tr("A megadott %1 portnál a hátlap nincs bekötve.\n").arg(pp.getFullName(_q)), true);
        return true;        // imperfect
    default:
        if (shareResultant(_sh, sc) == ES_NC) {
            _msg += htmlError(QObject::tr("A megadott %1 port megosztása nem használható (nincs összeköttetés egy érpáron sem).<br>").arg(pp.getFullName(_q)), true);
            return true;    // imperfect
        }
        break;
    }
    return false;
}

static QString colLink(QSqlQuery& q, qlonglong pid, ePhsLinkType lt, ePortShare ps, bool& _col, const QString& t)
{
    QString msg;
    cPhsLink link;
    tRecordList<cPhsLink>   list;
    if (pid != NULL_ID) {
        link.collisions(q, list, pid, lt, ps);
        if (!list.isEmpty()) {
            _col = true;
            msg += htmlWarning(t);
            while (!list.isEmpty()) {
                cPhsLink *ppl = list.pop_back();
                qlonglong pid2 = ppl->getId(_sPortId2);
                msg += htmlIndent(16, "==> " + cNPort::getFullNameById(q, pid2));
                ePhsLinkType lt = ePhsLinkType(ppl->getId(_sPhsLinkType2));
                ePortShare   sh= ePortShare(ppl->getId(_sPortShared));
                if (sh != ES_) msg += "/" + portShare(sh);
                if (lt != LT_TERM) {
                    QMap<ePortShare, qlonglong> dummy;
                    QString m = linkChainReport(q, pid2, lt, sh, dummy);
                    if (!m.isEmpty()) msg += htmlIndent(32, m, false, false);
                }
                delete ppl;
            };
        }
    }
    return msg;
}

void cLinkDialog::changed()
{
    if (blockChange) return;
    collision = false;
    imperfect  = true;
    exists     = false;
    cPhsLink link;
    qlonglong pid1 = pLink1->getPortId();
    qlonglong pid2 = pLink2->getPortId();
    ePhsLinkType lt1 = pLink1->getLinkType();
    ePhsLinkType lt2 = pLink2->getLinkType();
    ePortShare   ps1 = ES_, ps2 = ES_, ps = ES_;
    if      (lt1 == LT_FRONT && lt2 == LT_TERM)  ps  = ps1 = pLink1->getPortShare();
    else if (lt1 == LT_TERM  && lt2 == LT_FRONT) ps  = ps2 = pLink2->getPortShare();
    QString msg;
    if (pid1 == NULL_ID && pid2 == NULL_ID) {
        pButtons->disableExcept();
        msg = tr("Nincs megadva egy port sem.");
        pTextEditReport->setText(htmlWarning(msg));
        pCheckBoxCollisions->setChecked(false);
        pCheckBoxCollisions->setCheckable(false);
        return;
    }
    if (pid1 == pid2) {
        pButtons->disableExcept();
        msg = tr("A két oldal azonos.");
        pTextEditReport->setText(htmlError(msg));
        pCheckBoxCollisions->setChecked(false);
        pCheckBoxCollisions->setCheckable(false);
        return;
    }
    if (pid1 == NULL_ID || pid2 == NULL_ID) {
        pButtons->disableExcept();
        msg = tr("Csak a link egyik fele van megadva.\n");
        msg = htmlWarning(msg, true);
    }
    else {
        imperfect = false;
        link.setId(_sPortId1, pid1);
        link.setId(_sPortId2, pid2);
        link.setId(_sPhsLinkType1, lt1);
        link.setId(_sPhsLinkType2, lt2);
        link.setId(_sPortShared, ps);
        int rows = link.completion(*pq);
        if (rows == 1) {
            exists = true;
            linkId = link.getId();
            pButtons->enable(ENUM2SET3(DBT_DELETE, DBT_NEXT, DBT_PREV));
            pCheckBoxCollisions->setChecked(false);
            pCheckBoxCollisions->setCheckable(false);
            QString note = link.getNote();
            if (note.isNull() == pToolButtonNoteNull->isChecked() || note == pTextEditNote->toPlainText()) {
                noteChanged = false;
            }
            if (noteChanged) {
                pButtons->enable(ENUM2SET2(DBT_SAVE, DBT_OK));
                msg = htmlInfo(tr("Mentett, létező link (ID:%1), de a megjegyzés mező változozott.").arg(link.getId()));
            }
            else {
                pButtons->disable(ENUM2SET2(DBT_SAVE, DBT_OK));
                pTextEditNote->setText(note);
                pToolButtonNoteNull->setChecked(note.isNull());
                msg = htmlGrInf(tr("Mentett, létező link (ID:%1).").arg(link.getId()));
            }
        }
        else if (rows > 0) EXCEPTION(AMBIGUOUS, rows, link.toString());
    }
    msg += sHtmlLine;
    if (!exists) {
//      tRecordList<cPhsLink>   list;
        imperfect = checkShare(*pq, pid1, lt1, ps1, msg) || imperfect;
        imperfect = checkShare(*pq, pid2, lt2, ps2, msg) || imperfect;
        msg += colLink(*pq, pid1, lt1, ps1, collision, tr("Ütköző linkek a 1. porton :"));
        msg += colLink(*pq, pid2, lt2, ps2, collision, tr("Ütköző linkek a 2. porton :"));
        if (collision) {
            pButtons->disableExcept(ENUM2SET4(DBT_CANCEL, DBT_CLOSE, DBT_NEXT, DBT_PREV));
            pCheckBoxCollisions->setChecked(false);
            pCheckBoxCollisions->setCheckable(true);
        }
        else {
            if (imperfect) pButtons->disableExcept();
            else           pButtons->enabeAll();
            msg += htmlGrInf(tr("Nincs ütközö/törlendő fizikai link (patch)."));
            pCheckBoxCollisions->setChecked(false);
            pCheckBoxCollisions->setCheckable(false);
        }
        msg += sHtmlLine;
    }
    QMap<ePortShare, qlonglong> endMap1;
    QMap<ePortShare, qlonglong> endMap2;
    QString m;
    if (pid1 != NULL_ID) {
        m = linkChainReport(*pq, pid1, lt1, ps, endMap1);
        if (!m.isEmpty()) msg += htmlInfo("Link lánc az 1. port irányában : ") + htmlIndent(16, m, false, false) + sHtmlLine;
        else msg += htmlInfo("Az első port egy végpont.");
    }
    if (pid2 != NULL_ID) {
        m = linkChainReport(*pq, pid2, lt2, ps, endMap2);
        if (!m.isEmpty()) msg += htmlInfo("Link lánc az 2. port irányában : ") + htmlIndent(16, m, false, false) + sHtmlLine;
        else msg += htmlInfo("Az második port egy végpont.");
    }
    cNPort p;
    foreach (ps1, endMap1.keys()) {
        foreach (ps2, endMap2.keys()) {
            if (shareConnect(ps1, ps2) == ES_NC) continue;
            qlonglong endPid1 = endMap1[ps1];
            qlonglong endPid2 = endMap2[ps2];
            QString msgPref = QString("%1 <==> %2 : ").arg(p.getFullNameById(*pq, endPid1), p.getFullNameById(*pq, endPid2));
            msg += linkEndEndLogReport(*pq, endPid1, endPid2, exists, msgPref);
            m  = linkEndEndMACReport(*pq, endPid1, endPid2, msgPref);
            m += linkEndEndMACReport(*pq, endPid2, endPid1, msgPref);
            if (m.isEmpty()) msg += htmlInfo(msgPref + QObject::tr("A végponti link nincs megerősítve a MAC címtáblák alapján."));
            else             msg += m;
        }
    }

    pTextEditReport->setText(msg);
}

void cLinkDialog::collisionsTogled(bool f)
{
    if (f) {
        if (!imperfect) pButtons->enabeAll();
    }
    else {
        if (collision) pButtons->disableExcept();
    }
}

void cLinkDialog::noteNull(bool st)
{
    pTextEditNote->setDisabled(st);
    noteChanged = true;
}

void cLinkDialog::modifyNote()
{
    noteChanged = true;
}

void cLinkDialog::buttonPressed(int kid)
{
    noteChanged = false;
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
    case DBT_DELETE:
        remove();
        break;
    default:
        EXCEPTION(EPROGFAIL, kid);
    }
}

