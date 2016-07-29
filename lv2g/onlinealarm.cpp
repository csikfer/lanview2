#include "onlinealarm.h"

const enum ePrivilegeLevel cOnlineAlarm::rights = PL_INDALARM;

cOnlineAlarm::cOnlineAlarm(QWidget *par) : cOwnTab(par)
{
    pActRecord = NULL;
    pq = newQuery();
    isAdmin = lanView::user().privilegeLevel() >= PL_ADMIN;

    pMainLayout = new QVBoxLayout(this);                // (ös) widget layout
    pMainSplitter  = new QSplitter(Qt::Horizontal);     // Fő splitter
    pMainLayout->addWidget(pMainSplitter);

    pAlarmSplitter = new QSplitter(Qt::Vertical);       // Jobb oldalon a két tábla
    pMainSplitter->addWidget(pAlarmSplitter);

    pRecTabNoAck  = new cRecordTable("uaalarms", false, pAlarmSplitter);    // nem nyugtázott riasztások tábla; bal felső
    if (isAdmin) {
        pRecTabNoAck->tableView()->setSelectionMode(QAbstractItemView::ExtendedSelection);  // Több soros kijelölés
    }
    else {
        pRecTabNoAck->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);    // Nincs több soros kijelölés
    }
    connect(pRecTabNoAck->tableView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(curRowChgNoAck(QItemSelection,QItemSelection)));

    pRecTabAckAct = new cRecordTable("aaalarms", false, pAlarmSplitter);    // Nyugtázott. aktív riasztások; bal alsó
    pRecTabAckAct->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);   // Nincs több soros kijelölés
    connect(pRecTabAckAct->tableView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(curRowChgAckAct(QItemSelection,QItemSelection)));

//    connect(pRecordTable, SIGNAL(closeIt()),   this, SLOT(removeIt()));
//    connect(pRecordTable, SIGNAL(destroyed()), this, SLOT(destroyedChild()));

    pRightWidget   = new QWidget();                     // Jobb oldali widget ablak a main splitterben
    pMainSplitter->addWidget(pRightWidget);

    pRightVBLayout = new QVBoxLayout();
    pRightWidget->setLayout(pRightVBLayout);

    pMapLabel = new QLabel();
//??    pMapLabel.setPalette(QPalette::Dark);
    pRightVBLayout->addWidget(pMapLabel);
    pMap  = new cImageWidget();
    pRightVBLayout->addWidget(pMap);
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
}

cOnlineAlarm::~cOnlineAlarm()
{
    delete pq;
}

void cOnlineAlarm::map()
{
    if (pActRecord == NULL) EXCEPTION(EPROGFAIL);
    // A riasztás helye (ID)
    qlonglong pid = pActRecord->getId(_sPlaceId);
    cPlace pl;
    pl.fetchById(*pq, pid);
    // Hely megnevezése a note mező, vagy ha üres akkor a név
    QString h = pl.getNoteOrName();
    // A parent alaprajza
    bool ok;
    qlonglong id = execSqlIntFunction(*pq, &ok, "get_parent_image", pid);
    cImage  image;
    QVariant vPol = pl.get(_sFrame);
    QPolygonF pol;
    QPoint    center;
    ok = ok && image.fetchById(*pq, id);
    ok = ok && image.dataIsPic();
    if (ok) {
        h = trUtf8("%1; %2").arg(image.getNoteOrName()).arg(h);
        pMap->clearDraws();
        if (vPol.isNull() == false) {
            pMap->setBrush(QBrush(QColor(Qt::red))).addDraw(vPol);
            pol = convertPolygon(vPol.value<tPolygonF>());
            center = avarage<QPolygonF, QPointF>(pol).toPoint();
        }
        else {
            h += trUtf8(" (nincs pontos hely adat)");
        }
    }
    ok = ok && pMap->setImage(image);
    if (!ok) {
        pMap->setText(trUtf8(" (Nincs megjeleníthető alaprajz.)"));
    }
    else {
        if (vPol.isNull() == false) {
            pMap->center(center);
        }
    }
    pMapLabel->setText(h);
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
    foreach (QModelIndex mi, mil) {
        int row = mi.row();
        const cRecord *pr = pRecTabNoAck->recordAt(row, EX_IGNORE);
        if (pr == NULL) {
            DERR() << trUtf8("Invalid selected row : %1").arg(row) << endl;
            continue;
        }
        qlonglong aid = pr->getId();
        cAlarm a;
        if (!a.fetchById(*pq, aid)) {
            DERR() << trUtf8("Alarm record fetch failed, id = %1").arg(aid) << endl;
            continue;
        }
        if (a.isNull(_sAckUserId)) {
            a.setId(_sAckUserId, lanView::user().getId());
            a.setName(_sAckTime, "NOW");
            a.update(*pq,  false, a.mask(_sAckUserId, _sAckTime));
        }
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
    cAckDialog dialog(*pActRecord, this);
    if (dialog.exec() == QDialog::Accepted) {
        cAlarm  a;
        a.fetchById(*pq, pActRecord->getId());
        if (a.isNull(_sAckUserId)) {
            a.setId(_sAckUserId, lanView::user().getId());
            a.setName(_sAckTime, "NOW");
            a.setName(_sAckMsg, dialog.pUi->msgL->text());
            a.update(*pq,  false, a.mask(_sAckUserId, _sAckTime, _sAckMsg));
        }
        else {
            ; // Valaki már nyugtázta
        }
    }
    pRecTabAckAct->refresh();
    pRecTabNoAck->refresh();
}

cAckDialog::cAckDialog(const cRecord& __r, QWidget *par)
    : QDialog(par)
{
    pUi = new Ui_ackDialog;
    pUi->setupUi(this);
    pUi->placeL->setText(__r.getName(_sPlaceName));
    pUi->nodeL->setText(__r.getName(_sNodeName));
    pUi->msgL->setText(__r.getName("msg"));
    pUi->fromL->setText(__r.getName(_sBeginTime));
    if (__r.isNull(_sEndTime)) {
        pUi->toL->hide();
        pUi->labelTo->hide();
    }
    else {
        pUi->toL->setText(__r.getName(_sEndTime));
    }
    connect(pUi->msgTE, SIGNAL(textChanged()), this, SLOT(changed()));
}

cAckDialog::~cAckDialog()
{
    ;
}

#define MIN_MSG_SIZE 8
void cAckDialog::changed()
{
    pUi->ackPB->setDisabled(pUi->msgTE->toPlainText().size() < MIN_MSG_SIZE);
}
