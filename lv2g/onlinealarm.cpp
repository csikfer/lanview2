#include "onlinealarm.h"

cOnlineAlarm::cOnlineAlarm(QWidget *par) : cOwnTab(par)
{
    QLabel *pLabel;
    pActRecord = NULL;
    pq = newQuery();

    pMainLayout = new QVBoxLayout(this);                // (ös) widget layout
    pMainSplitter  = new QSplitter(Qt::Horizontal);     // Fő splitter
    pMainLayout->addWidget(pMainSplitter);

    pAlarmSplitter = new QSplitter(Qt::Vertical);       // Jobb oldalon a két tábla
    pMainSplitter->addWidget(pAlarmSplitter);

    pRecTabNoAck  = new cRecordTable("uaalarms", false, pAlarmSplitter);    // nem nyugtázott riasztások tábla; bal felső
    pRecTabNoAck->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);    // Nincs több soros kijelölés
    pLabel = new QLabel(trUtf8("Nem nyugtázott riasztások"));
//??    pLabel.setPalette(QPalette::Dark);
    pRecTabNoAck->pMainLayer->insertWidget(0, pLabel);
    connect(pRecTabNoAck->tableView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(curRowChgNoAck(QItemSelection,QItemSelection)));

    pRecTabAckAct = new cRecordTable("aaalarms", false, pAlarmSplitter);    // Nyugtátott. aktív riasztások; bal alsó
    pRecTabAckAct->tableView()->setSelectionMode(QAbstractItemView::SingleSelection);   // Nincs több soros kijelölés
    pLabel = new QLabel(trUtf8("Nyugtázott, aktív riasztások"));
//??    pLabel.setPalette(QPalette::Dark);
    pRecTabAckAct->pMainLayer->insertWidget(0, pLabel);
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
    pMap  = new QLabel();
    pRightVBLayout->addWidget(pMap);
    pAckButton     = new QPushButton(trUtf8("Nyugtázás"));
    pAckButton->setDisabled(true);
    pRightVBLayout->addWidget(pAckButton);
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
    QString h = pl.getNote();
    if (h.isEmpty()) h = pl.getName();
    // a fejléc szövege
    h = trUtf8("%1 alaprajza").arg(h);
    pMapLabel->setText(h);
    // A parant alaprajza
    execSqlFunction(*pq, "get_parent_image", pid);
    bool ok;
    qlonglong id = pq->value(0).toLongLong(&ok);
    if (ok) {
        cImage  image;
        image.fetchById(*pq, id);
        PDEB(VERBOSE) << "MAP image : " << image.getName() << " type : " << image.getType() << endl;
        QPixmap pmap;
        if (!pmap.loadFromData(image.getImage(), image.getType()))
            EXCEPTION(EDATA, id, image.getType());
        pMap->setPixmap(pmap);
    }
    else {
        pMap->clear();
        pMap->setText(trUtf8(" (Nincs megjeleníthető alaprajz.)"));
    }
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
