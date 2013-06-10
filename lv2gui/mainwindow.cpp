#include "lv2gui.h"

#include "setup.h"


cMainWindow::cMainWindow(bool _setupOnly, QWidget *parent) :
    QMainWindow(parent)
{
    pTabWidget = new QTabWidget(this);
    // Central Widget
    setCentralWidget(pTabWidget);
    cMenuItem mi;
    if (_setupOnly) {   /// Minimalista setup, nincs adatbázis
        pTabWidget = NULL;
        QAction *pFile = menuBar()->addAction(trUtf8("File"));
        QMenu   *pm    = new QMenu(menuBar());
        pFile->setMenu(pm);
        QAction *pa;
        mi.setName(trUtf8("Setup"));   mi.setName(_sProperties, ":own=setup:");     pa = pm->addAction(mi.getName()); action(pa, mi, NULL);
        pa->trigger();  // Ez a Setup-ot kirakja a tab-ba
        mi.setName(trUtf8("Restart")); mi.setName(_sProperties, ":exec=restart:");  pa = pm->addAction(mi.getName()); action(pa, mi, NULL);
        mi.setName(trUtf8("Exit"));    mi.setName(_sProperties, ":exec=exit:");     pa = pm->addAction(mi.getName()); action(pa, mi, NULL);
    }
    else {
        // Menu
        QSqlQuery *pqm = newQuery();
        if (!mi.fetchFirstItem(*pqm, lanView::getInstance()->appName)) EXCEPTION(EDBDATA,-1, QObject::trUtf8("Nincs menü a GUI applikációhoz"));
        do {
            QSqlQuery *pq2 = newQuery();
            action(menuBar()->addAction(mi.title()), mi, pq2);
            delete pq2;
        } while(mi.next(*pqm));
    }

}

cMainWindow::~cMainWindow()
{

}

void cMainWindow::action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq)
{
    _DBGFN() << _mi.toString() << endl;
    if (!_mi.isNull(_sToolTip))     pa->setToolTip(_mi.getName(_sToolTip));
    if (!_mi.isNull(_sWhatsThis))   pa->setWhatsThis(_mi.getName(_sWhatsThis));
    tMagicMap mm;
    mm = splitMagic(_mi.getName(_sProperties), mm);
    QString mp;
    if (findMagic(QString("sub"), mm)) {    // Almenük vannak
        cMenuItem sm;
        QMenu *pm = new QMenu(this);
        pa->setMenu(pm);
        if (pq == NULL) EXCEPTION(EPROGFAIL);
        if (sm.fetchFirstItem(*pq, _mi.getName(_sAppName), _mi.getId())) {
            PDEB(VERBOSE) << "Menu :" << _mi.title() << " #" << pq->size() << " sub menü:" << endl;
            do {
                PDEB(VERBOSE) << _mi.title() << " sun menu : " << sm.title() << endl;
                QSqlQuery *pq2 = newQuery();
                action(pm->addAction(sm.title()), sm, pq2);
                delete pq2;
            } while (sm.next(*pq));
        }
        else DWAR() << trUtf8("üres menü : ") << _mi.title() << endl;
     }
    else if (!(mp = magicParam(QString("shape"),  mm)).isEmpty()) {
        setShapeAction(pq, mp, _mi, pa);
    }
    else if (!(mp = magicParam(QString("exec"), mm)).isEmpty()) {
        setExecAction(mp, pa);
    }
    else if (!(mp = magicParam(QString("own"), mm)).isEmpty()) {
        setOwnAction(pq, mp, _mi, pa, mm);
    }
    else EXCEPTION(EDBDATA);
    DBGFNL();
}

void cMainWindow::setShapeAction(QSqlQuery *pq, const QString mp, cMenuItem& _mi, QAction *_pa)
{
    DBGFN();
    if (pq == NULL) EXCEPTION(EPROGFAIL);   // Ha nincs adatbázis, akkor ezt nem kéne
    cTableShape *pts = new cTableShape;
    pts->setByName(*pq, mp);
    cMenuAction *po  = new cMenuAction(pts, _pa, this);
    po->setObjectName(_mi.title()); // Ez lessz a TAB neve
}

void cMainWindow::setExecAction(const QString mp, QAction *_pa)
{
    DBGFN();
    cMenuAction *po  = new cMenuAction(mp, _pa, this);
    po->setObjectName(mp);
}

void cMainWindow::setOwnAction(QSqlQuery *pq, const QString mp, cMenuItem& _mi, QAction *_pa, const tMagicMap &_mm)
{
    DBGFN();
    cOwnTab *pot = NULL;
    if (0 == mp.compare("setup", Qt::CaseInsensitive)) {
        (void)_mm; (void)pq;
        pot = new cSetupWidget(*lanView::getInstance()->pSet, this);
    }
    else EXCEPTION(EDBDATA, -1, mp);
    pot->setObjectName(mp);
    cMenuAction *po  = new cMenuAction(pot, _pa, this);
    po->setObjectName(_mi.getName());
}

// -----------

// cAction
cMenuAction::cMenuAction(cTableShape *ps, QAction *pa, cMainWindow * par)
    : QObject(par)
    , mainWindow(*par)
{
    pOwnTab      = NULL;
    pTableShape  = ps;
    pRecordTable = NULL;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;
    ps->setParent(this);
    initRecordTable();
}
cMenuAction::cMenuAction(const QString&  ps, QAction *pa, cMainWindow * par)
    : QObject(par)
    , mainWindow(*par)
{
    pOwnTab      = NULL;
    pTableShape  = NULL;
    pRecordTable = NULL;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;
    setObjectName(ps);
    connect(pa, SIGNAL(triggered()), this, SLOT(exec()));
}

cMenuAction::cMenuAction(cOwnTab *po, QAction *pa, cMainWindow * par)
    : QObject(par)
    , mainWindow(*par)
{
    pOwnTab      = po;
    pTableShape  = NULL;
    pRecordTable = NULL;
    pWidget      = po->pWidget();
    pDialog      = NULL;
    pAction      = pa;
    connect(pa, SIGNAL(triggered()), this, SLOT(openIt()));
    connect(po, SIGNAL(closeIt()),   this, SLOT(closeIt()));
}

cMenuAction::~cMenuAction()
{
    ;
}

void cMenuAction::initRecordTable()
{
    pRecordTable = new cRecordTable(pTableShape, false, mainWindow.pTabWidget);
    pWidget = pRecordTable->pWidget();
    pWidget->setVisible(false);
    connect(pAction,      SIGNAL(triggered()), this, SLOT(openIt()));
    connect(pRecordTable, SIGNAL(closeIt()),   this, SLOT(closeIt()));
}

void cMenuAction::openIt()
{
    int i = mainWindow.pTabWidget->indexOf(pWidget);
    if (i < 0) {
        i = mainWindow.pTabWidget->addTab(pWidget, objectName());
    }
    mainWindow.pTabWidget->setCurrentIndex(i);
}

void cMenuAction::closeIt()
{
    int i = mainWindow.pTabWidget->indexOf(pWidget);
    if (i < 0) EXCEPTION(EPROGFAIL);
    mainWindow.pTabWidget->removeTab(i);
    pWidget->setVisible(false);
    if (pRecordTable != NULL) {
        delete pRecordTable;    // Ha becsukta, akkor csinalunk az objektumnal egy resetet
        initRecordTable();
    }
}

void  cMenuAction::exec()
{
    QString name = objectName();
    if (pDialog == NULL) {
        if      (0 == name.compare("exit",    Qt::CaseInsensitive)) qApp->exit(0);
        else if (0 == name.compare("restart", Qt::CaseInsensitive)) appReStart();
        else    EXCEPTION(EDBDATA,-1, name);
    }
    else {
        PDEB(VERBOSE) << "Start " << name << " dialog..." << endl;
        int r = pDialog->exec();
        PDEB(INFO) << "Return " << name << " dialog :" << r << endl;
    }
}

void cMenuAction::closed()
{
    ; // ?
}

// OWN
cOwnTab::cOwnTab(cMainWindow &_mw)
    : QWidget(_mw.pTabWidget)
    , mainWindow(_mw)
{
    setVisible(false);
}

QWidget *cOwnTab::pWidget()
{
    return this;
}

cOwnTab *cOwnTab::closed(cMenuAction * pa)
{
    (void)pa;
    return this;
}
