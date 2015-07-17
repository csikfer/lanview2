#include "onlinealarm.h"

cOnlineAlarm::cOnlineAlarm(QWidget *par) : cOwnTab(par)
{
    pActRecord = NULL;
    pq = newQuery();

    pMainLayout = new QVBoxLayout(this);                // (ös) widget layout
    pMainSplitter  = new QSplitter(Qt::Horizontal);     // Fő splitter
    pMainLayout->addWidget(pMainSplitter);

    pAlarmSplitter = new QSplitter(Qt::Vertical);       // Jobb oldalon a két tábla
    pMainSplitter->addWidget(pAlarmSplitter);

    pRecTabNoAck  = new cRecordTable("uaalarms", false, pAlarmSplitter);    // nem nyugtázott riasztások tábla; bal felső
    pRecTabNoAck->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);    // Nincs több soros kijelölés
    connect(pRecTabNoAck->tableView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(curRowChgNoAck(QItemSelection,QItemSelection)));

    pRecTabAckAct = new cRecordTable("aaalarms", false, pAlarmSplitter);    // Nyugtátott. aktív riasztások; bal alsó
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
    pAckButton     = new QPushButton(trUtf8("Nyugtázás"));
    pAckButton->setDisabled(true);
    pRightVBLayout->addWidget(pAckButton);
    connect(pAckButton, SIGNAL(clicked()), this, SLOT(acknowledge()));
}

cOnlineAlarm::~cOnlineAlarm()
{
    delete pq;
}

void cOnlineAlarm::map()
{
    if (pActRecord == NULL) EXCEPTION(EPROGFAIL);
    // A tiasztás helye (ID)
    qlonglong pid = pActRecord->getId(_sPlaceId);
    cPlace pl;
    pl.fetchById(*pq, pid);
    // Hely megnevezése a note mező, vagy ha üres akkor a név
    QString h = pl.getNoteOrName();
    // A parent alaprajza
    bool ok;
    qlonglong id = execSqlIntFunction(*pq, &ok, "get_parent_image", pid);
    cImage  image;
    ok = ok && image.fetchById(*pq, id);
    ok = ok && image.dataIsPic();
    if (ok) {
        h = trUtf8("%1; %2").arg(image.getNoteOrName()).arg(h);
        pMap->clearDraws();
        if (pl.isNull(_sFrame) == false) {
            pMap->setBrush(QBrush(QColor(Qt::red))).addDraw(pl.get(_sFrame));
        }
        else {
            h += trUtf8(" (nincs pontos hely adat)");
        }
    }
    ok = ok && pMap->setImage(image);
    if (!ok) {
        pMap->setText(trUtf8(" (Nincs megjeleníthető alaprajz.)"));
    }
    pMapLabel->setText(h);
}


void cOnlineAlarm::curRowChgNoAck(QItemSelection sel, QItemSelection)
{
    QModelIndexList mil = sel.indexes();
    if (mil.size()) {
        int row = mil.at(0).row();
        pRecTabAckAct->tableView()->selectionModel()->clearSelection();
        pActRecord = pRecTabNoAck->recordAt(row);
        map();
        pAckButton->setDisabled(false);
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
    }
}

void cOnlineAlarm::acknowledge()
{
    if (pActRecord == NULL) EXCEPTION(EPROGFAIL);
    cAckDialog dialog(*pActRecord, this);
    if (dialog.exec() == QDialog::Accepted) {
        cAlarm  a;
        a.fetchById(*pq, pActRecord->getId("alarm_id"));
        if (a.isNull("ack_user_id")) {
            a.setId("ack_user_id", lv2g::getInstance()->user().getId());
            a.setName("ack_time", "NOW");
            a.setName("ack_msg", dialog.pUi->msgL->text());
            a.update(*pq,  false, a.mask("ack_user_id", "ack_time", "ack_msg"));
        }
        else {
            ; // Valaki már nyugtázta
        }
    }

}

cAckDialog::cAckDialog(const cRecordAny& __r, QWidget *par)
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
