#include "onlinealarm.h"
#include "mainwindow.h"

const enum ePrivilegeLevel cOnlineAlarm::rights = PL_INDALARM;

QString  cOnlineAlarm::sPlaceTitle;
QString  cOnlineAlarm::sNodeTitle;
QString  cOnlineAlarm::sTicket;

cOnlineAlarm::cOnlineAlarm(QMdiArea *par) : cIntSubObj(par)
{
    lanView::getInstance()->subsDbNotif(_sAlarm);
    connect(getSqlDb()->driver(), SIGNAL(notification(QString,QSqlDriver::NotificationSource,QVariant)),
            this,                 SLOT(notify(QString,QSqlDriver::NotificationSource,QVariant)));
    pTargetRec = pActRecord = NULL;
    isTicket  = false;
    pSound = new QSound(lv2g::getInstance()->soundFileAlarm, this);
    pSound->setLoops(QSound::Infinite);
    pq = newQuery();
    isAdmin = lanView::user().privilegeLevel() >= PL_ADMIN;

    pMainLayout = new QVBoxLayout(this);                // (ös) widget layout
    pMainSplitter  = new QSplitter(Qt::Horizontal);     // Fő splitter
    pMainLayout->addWidget(pMainSplitter);

    pAlarmSplitter = new QSplitter(Qt::Vertical);       // Jobb oldalon a két tábla
    pMainSplitter->addWidget(pAlarmSplitter);

    pRecTabNoAck  = new cRecordTable("uaalarms", false, pAlarmSplitter);    // nem nyugtázott riasztások tábla; bal felső
    pRecTabNoAck->init();
    if (isAdmin) {
        pRecTabNoAck->tableView()->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Több soros kijelölés
    }
    else {
        pRecTabNoAck->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);    // Nincs több soros kijelölés
    }
    connect(pRecTabNoAck->tableView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(curRowChgNoAck(QItemSelection,QItemSelection)));
    pNoAckModel = dynamic_cast<cRecordTableModel *>(pRecTabNoAck->pModel);
    connect(pNoAckModel, SIGNAL(dataReloded(tRecords)), this, SLOT(noAckDataReloded(tRecords)));

    pRecTabAckAct = new cRecordTable("aaalarms", false, pAlarmSplitter);    // Nyugtázott. aktív riasztások; bal alsó
    pRecTabAckAct->init();
    pRecTabAckAct->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);   // Nincs több soros kijelölés
    connect(pRecTabAckAct->tableView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(curRowChgAckAct(QItemSelection,QItemSelection)));

//    connect(pRecordTable, SIGNAL(closeIt()),   this, SLOT(removeIt()));
//    connect(pRecordTable, SIGNAL(destroyed()), this, SLOT(destroyedChild()));

    pRightWidget   = new QWidget();                     // Jobb oldali widget ablak a main splitterben
    pMainSplitter->addWidget(pRightWidget);

    pRightVBLayout = new QVBoxLayout();
    pRightWidget->setLayout(pRightVBLayout);

    pMapText = new QTextEdit();
    pMapText->setReadOnly(true);
//??    pMapLabel.setPalette(QPalette::Dark);
    pRightVBLayout->addWidget(pMapText,1);
    pMap  = new cImageWidget();
    pRightVBLayout->addWidget(pMap, 10);
    pButtonLayout  = new QHBoxLayout();
    pRightVBLayout->addLayout(pButtonLayout);
    pAckButton     = new QPushButton(trUtf8("Nyugtázás"));
    pAckButton->setDisabled(true);
    pButtonLayout->addWidget(pAckButton);
    connect(pAckButton, SIGNAL(clicked()), this, SLOT(acknowledge()));
    if (isAdmin) {
        pAckAllButton = new QPushButton(trUtf8("Mind nyugtázása"));
        pAckAllButton->setDisabled(true);
        pButtonLayout->addWidget(pAckAllButton);
        connect(pAckAllButton, SIGNAL(clicked()), this, SLOT(allAcknowledge()));
    }
    else {
        pAckButton = NULL;
    }
    noAckDataReloded(pNoAckModel->records());

    // Létezik a virtuális ticket szolgáltatás ?
    pTicket = new cHostService;
    if (1 != pTicket->fetchByNames(*pq, _sNil, _sTicket, EX_IGNORE)) pDelete(pTicket);
}

cOnlineAlarm::~cOnlineAlarm()
{
    delete pq;
    if (isTicket) delete pTargetRec;
}

void cOnlineAlarm::map()
{
    static const QString _sBr = "<br>";
    if (pActRecord == NULL) EXCEPTION(EPROGFAIL);
    if (isTicket) {
        isTicket = false;
        pDelete(pTargetRec);
    }
    pTargetRec = pActRecord;
    QVariantList ids = pActRecord->get(_sViewUserIds).toList();
    qlonglong uid = lanView::user().getId();
    qlonglong aid = pActRecord->getId();
    if (ids.indexOf(uid) < 0) {     // Ha még nem nézte meg
        cUserEvent::insertHappened(*pq, uid, aid, UE_VIEW);
        ids << uid;
        pActRecord->set(_sViewUserIds, ids);
        noAckDataReloded(pNoAckModel->records());
    }

    if (sNodeTitle.isEmpty()) {
        sNodeTitle  = cTableShape::getFieldDialogTitle(*pq, _sNodes,  _sNodeName);
        sPlaceTitle = cTableShape::getFieldDialogTitle(*pq, _sPlaces, _sPlaceName);
        sTicket     = trUtf8("Hiba jegy");
    }
    static const qlonglong ticketId  = cHostService::ticketId(*pq, EX_IGNORE);
    QString text;   // HTML text

    if (ticketId != NULL_ID && ticketId == pActRecord->getId(_sHostServiceId)) {    // Ticket
        isTicket = true;
        text = sTicket + " :<br>";
        qlonglong id = pActRecord->getId(_sSuperiorAlarmId);
        pTargetRec = pActRecord->newObj();
        pTargetRec->fetchById(*pq, id);
    }
    qlonglong placeId = pTargetRec->getId(_sPlaceId);
    /*
    place.fetchById(*pq, placeId);
    qlonglong nodeId = pTargetRec->getId(_sNodeId);
    node.fetchById(*pq, nodeId);
    text += _sBr + sPlaceTitle + " : <b>" + place.getName() + "</b>, <i>" + place.getNote() + "</i>";
    text += _sBr + sNodeTitle  + " : <b>" + node.getName()  + "</b>, <i>" + node.getNote()  + "</i>";
    text += _sBr + trUtf8("Riasztás oka : ") + "<b><i>" + pTargetRec->getName(_sMsg) + "</i></b>";
    text += _sBr + trUtf8("Csatolt üzenet : ") + "<b><i>" + pTargetRec->getName(_sEventNote) + "</i></b>";
    */
    text += cAlarm::htmlText(*pq, pTargetRec->getId());
    if (pTargetRec->isIndex(_sAckUserIds) && pTargetRec->get(_sAckUserIds).toList().isEmpty() == false) {
        text += _sBr + trUtf8("Nyugtázva : ") + "<b>" + pTargetRec->view(*pq, _sAckUserIds) + "</b>";
        QString note = pTargetRec->getName(_sAckUserNote);
        if (!note.isEmpty()) {
            _sBr + trUtf8("Nyugtázó(k) megjegyzése(i) : ") + "<i>" + note.replace(QString("\n"), _sBr);
        }
        if (isTicket) {
            text += _sBr + trUtf8("Hiba jegyhez fűzött megjegyzés : ") + "<b>" + pActRecord->getName(_sEventNote) + "</b>";
        }
    }
    if (isTicket  && pActRecord->get(_sAckUserIds).toList().isEmpty() == false) {
        text += _sBr + trUtf8("Hiba jegyet nyugtázta : ") + "<b>" + pActRecord->view(*pq, _sAckUserIds) + "</b>";
    }
    // A parent alaprajza
    bool ok = place.fetchById(*pq, placeId);
    qlonglong id = execSqlIntFunction(*pq, &ok, "get_parent_image", placeId);
    cImage  image;
    QVariant vPol = place.get(_sFrame);
    QPolygonF pol;
    QPoint    center;
    ok = ok && image.fetchById(*pq, id);
    ok = ok && image.dataIsPic();
    clearMap();
    if (ok) {
        text += trUtf8("<br>Alaprejz: %1, %2").arg(image.getName(), image.getNote());
        if (vPol.isNull() == false) {   // Van polygon
            QColor color(Qt::red);      // Piros
            color.setAlpha(128);        // félig átlátszó
            pMap->setBrush(QBrush(color)).addDraw(vPol);
            pol = convertPolygon(vPol.value<tPolygonF>());
            center = avarage<QPolygonF, QPointF>(pol).toPoint();
        }
        else {
            text += trUtf8("<br><br><b>Nincs pontos hely adat.</b>");
        }
    }
    ok = ok && pMap->setImage(image);
    if (!ok) {
        text += trUtf8("<br><br><b>Nincs megjeleníthető alaprajz.</b>");
    }
    else {
        if (vPol.isNull() == false) {
            pMap->center(center);
        }
    }
    pMapText->setText(text);
    int height= pMapText->document()->size().height() +5;
    int width = pMapText->document()->size().width();
    pMapText->setMinimumHeight(height);
    pMapText->resize(width, height);
}


void cOnlineAlarm::curRowChgNoAck(QItemSelection, QItemSelection)
{
    QModelIndexList mil = pRecTabNoAck->tableView()->selectionModel()->selectedRows();
    int row;
    int sel = mil.size();
    if (sel > 0 && pActRecord != NULL) {
        disconnect(pActRecord, SIGNAL(destroyed(QObject*)), this, SLOT(actRecordDestroyed(QObject*)));
    }
    switch (sel) {
    case 0:
        if (pRecTabNoAck->tableView()->selectionModel()->selectedRows().size() == 0) {
            pActRecord = NULL;
        }
        break;
    case 1:
        row = mil.at(0).row();
        pActRecord = pRecTabNoAck->recordAt(row);
        connect(pActRecord, SIGNAL(destroyed(QObject*)), this, SLOT(actRecordDestroyed(QObject*)));
        map();
        pAckButton->setEnabled(true);
        pAckAllButton->setEnabled(isAdmin);
        break;
    default:
        if (!isAdmin) EXCEPTION(EPROGFAIL);
        pActRecord = NULL;
        break;
    }
    if (sel > 0) {
        pRecTabAckAct->tableView()->selectionModel()->clearSelection();
    }
    if (pActRecord == NULL) {
        pAckButton->setDisabled(true);
        pAckAllButton->setEnabled(sel > 1);
        clearMap();
    }
}

void cOnlineAlarm::curRowChgAckAct(QItemSelection sel, QItemSelection)
{
    QModelIndexList mil = sel.indexes();
    if (mil.size()) {
        int row = mil.at(0).row();
        pRecTabNoAck->tableView()->selectionModel()->clearSelection();
        pActRecord = pRecTabAckAct->recordAt(row);
        map();
        pAckButton->setDisabled(true);
        pAckAllButton->setDisabled(true);
    }
    else {
        mil = pRecTabNoAck->tableView()->selectionModel()->selectedRows();
        if (mil.size() == 0) {
            pActRecord = NULL;
            clearMap();
        }
    }
}

void cOnlineAlarm::allAcknowledge()
{
    if (!isAdmin) EXCEPTION(EPROGFAIL);
    QModelIndexList mil = pRecTabNoAck->tableView()->selectionModel()->selectedRows();
    qlonglong uid = lanView::user().getId();
    foreach (QModelIndex mi, mil) {
        int row = mi.row();
        const cRecord *pr = pRecTabNoAck->recordAt(row, EX_IGNORE);
        if (pr == NULL) {
            DERR() << trUtf8("Invalid selected row : %1").arg(row) << endl;
            continue;
        }
        qlonglong aid = pr->getId();
        cUserEvent::insertHappened(*pq, uid, aid, UE_ACKNOWLEDGE);
    }
    pRecTabAckAct->refresh();
    pRecTabNoAck->refresh();
}

void cOnlineAlarm::actRecordDestroyed(QObject *pO)
{
    (void)pO;
    pActRecord = NULL;
    pAckButton->setDisabled(true);
    pAckAllButton->setDisabled(true);
    clearMap();
}


void cOnlineAlarm::acknowledge()
{
    if (pActRecord == NULL) EXCEPTION(EPROGFAIL);
    qlonglong aid = pTargetRec->getId();
    qlonglong uid = lanView::user().getId();
    cAckDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString msg = dialog.pUi->textEditMsg->toPlainText();
        cUserEvent::insertHappened(*pq, uid, aid, UE_ACKNOWLEDGE, msg);
        if (dialog.pUi->checkBoxTicket->isChecked()) {
            cAlarm::ticket(*pq,
                           (eNotifSwitch)pActRecord->getId(_sLastStatus),
                           msg,
                           NULL_ID,
                           aid,
                           (eNotifSwitch)pActRecord->getId(_sFirstStatus),
                           (eNotifSwitch)pActRecord->getId(_sMaxStatus) );
        }
    }
    pRecTabAckAct->refresh();
    pRecTabNoAck->refresh();
}

void cOnlineAlarm::noAckDataReloded(const tRecords& _recs)
{
    bool isAlarm = false;
    qlonglong uid = lanView::user().getId();    // My User ID
    QListIterator<cRecord *>    i(_recs);
    while (i.hasNext()) {
        cRecord *p = i.next();
        QVariantList uids = p->get(_sNoticeUserIds).toList();   // User ID list, nekik már megjelent
        if (uids.indexOf(uid) < 0) {     // uid not found
            cUserEvent::happened(*pq, uid, p->getId(), UE_NOTICE);
            uids << uid;        // mostmár neki is megjelent
            p->set(_sNoticeUserIds, uids);
            isAlarm = true;     // Ez neki újonnan jelenik meg, riasztunk
        }
        if (isAlarm) continue;
        uids = p->get(_sViewUserIds).toList();                  // User ID list, ők már megnézték
        if (uids.indexOf(uid) < 0) {
            isAlarm = true;     // Nem nézte még meg, riasztunk
        }
    }
    if (isAlarm) {                  // Riasztás
        pSound->play();             // Sziréna (ha van hangfájl.)
        if (lv2g::pMainWindow != NULL) {    // Aktíváljuk az ablakot
            lv2g::pMainWindow->pMdiArea->setActiveSubWindow(pSubWindow);
        }
    }
    else         pSound->stop();    // Nincs riasztás, hanjelzés kikapcsol
}

// Változás, frissíteni kell
void cOnlineAlarm::notify(const QString & name, QSqlDriver::NotificationSource, const QVariant & payload)
{
    PDEB(INFO) << name << " / " << debVariantToString(payload) << endl;
    pRecTabNoAck->refresh(false);
}


cAckDialog::cAckDialog(cOnlineAlarm *par)
    : QDialog(par)
{
    pUi = new Ui_ackDialog;
    pUi->setupUi(this);
    if (par->isTicket) setWindowTitle(trUtf8("Hiba jegy nyugtázása"));
    pUi->labelPlace->setText(par->sPlaceTitle);
    pUi->lineEditPlace->setText(par->pTargetRec->getName(_sPlaceName));
    pUi->labelNode->setText(par->sNodeTitle);
    pUi->lineEditNode->setText(par->pTargetRec->getName(_sNodeName));
    pUi->lineEditMsg->setText(par->pTargetRec->getName("msg"));
    pUi->lineEditFrom->setText(par->pTargetRec->getName(_sBeginTime));
    if (par->pTargetRec->isNull(_sEndTime)) {
        pUi->labelTo->hide();
        pUi->lineEditTo->hide();
    }
    else {
        pUi->lineEditTo->setText(par->pTargetRec->getName(_sEndTime));
    }
    connect(pUi->textEditMsg, SIGNAL(textChanged()), this, SLOT(changed()));
    if (par->pTicket != NULL && !par->isTicket) pUi->checkBoxTicket->setEnabled(true);
}

cAckDialog::~cAckDialog()
{
    ;
}

#define MIN_MSG_SIZE 8
void cAckDialog::changed()
{
    pUi->ackPB->setDisabled(pUi->textEditMsg->toPlainText().size() < MIN_MSG_SIZE);
}


