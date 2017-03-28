#include "mainwindow.h"
#include "menu.h"
#include "setup.h"
#include "gparse.h"



cMainWindow::cMainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    lv2g::pMainWindow = this;
    QString title = trUtf8("LanView %1 V%2, API V%3")
            .arg(lanView::appName, lanView::appVersion, lanView::libVersion);
    setWindowTitle(title);
    pMdiArea = new QMdiArea(this);
    pMdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pMdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pMdiArea->setDocumentMode(true);
    pMdiArea->setViewMode(QMdiArea::TabbedView);
    pMdiArea->setTabsMovable(true);
    pMdiArea->setTabsClosable(true);
    // Central Widget
    setCentralWidget(pMdiArea);
    if (!lanView::dbIsOpen()) {   // Minimalista setup, nincs adatbázisunk, vagy csak ez kell
        setSetupMenu();
    }
    else {        // Menu
        cMenuItem mi;
        QSqlQuery *pqm = newQuery();
        if (!mi.fetchFirstItem(*pqm, lanView::getInstance()->appName)) {
            QString msg = trUtf8("Nincs menü a GUI applikációhoz");
            if (lanView::isAuthorized(PL_ADMIN)) {
                QMessageBox::warning(NULL, design().titleWarning, msg);
                setSetupMenu();
            }
            else {
                EXCEPTION(EDBDATA, 0, msg);
            }
        }
        else do {
            QSqlQuery *pq2 = newQuery();
            action(menuBar()->addAction(mi.getName(_sMenuTitle)), mi, pq2);
            delete pq2;
        } while(mi.next(*pqm));
    }
}

cMainWindow::~cMainWindow()
{
    lv2g::pMainWindow = NULL;
}

void cMainWindow::setSetupMenu()
{
    QAction *pFile = menuBar()->addAction(trUtf8("File"));
    QMenu   *pm    = new QMenu(menuBar());
    pFile->setMenu(pm);
    QAction *pa;
    cMenuAction *po;

    QString nm = trUtf8("Setup");
    pa = pm->addAction(nm);
    cIntSubObj *pot =  new cSetupWidget(NULL);
    pot->setObjectName(nm);
    po  = new cMenuAction(pot, INT_SETUP, pa, pMdiArea);
    po->setObjectName(nm);

    nm = trUtf8("Restart");
    pa = pm->addAction(nm);
    po  = new cMenuAction(nm, pa, pMdiArea);
    po->setObjectName(nm);

    nm = trUtf8("Exit");
    pa = pm->addAction(nm);
    po  = new cMenuAction(nm, pa, pMdiArea);
    po->setObjectName(nm);

    if (lanView::dbIsOpen()) {
        QAction *pTools = menuBar()->addAction(trUtf8("Tools"));
        QMenu   *pm    = new QMenu(menuBar());
        pTools->setMenu(pm);
        QAction *pa;
        cMenuAction *po;

        QString nm = trUtf8("Import");
        pa = pm->addAction(nm);
        cIntSubObj *pot =  new cParseWidget(NULL);
        pot->setObjectName(nm);
        po  = new cMenuAction(pot, INT_PARSER, pa, pMdiArea);
        po->setObjectName(nm);
    }
}

void cMainWindow::action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq)
{
    _DBGFN() << _mi.toString() << endl;
    pa->setObjectName(_mi.getName());
    if (!_mi.isNull(_sToolTip))     pa->setToolTip(_mi.getName(_sToolTip));
    if (!_mi.isNull(_sWhatsThis))   pa->setWhatsThis(_mi.getName(_sWhatsThis));
    QString mp;
    if (_mi.isFeature("sub")) {    // Almenük vannak
        cMenuItem sm;
        QMenu *pm = new QMenu(this);
        pm->setObjectName(sm.getName());
        pa->setMenu(pm);
        if (pq == NULL) EXCEPTION(EPROGFAIL);
        if (sm.fetchFirstItem(*pq, _mi.getName(_sAppName), _mi.getId())) {
            PDEB(VERBOSE) << "Menu :" << _mi.getName(_sMenuTitle) << " #" << pq->size() << " sub menü:" << endl;
            do {
                sm.splitFeature(); // features
                PDEB(VERBOSE) << _mi.getName(_sMenuTitle) << " sub menu : " << sm.getName(_sMenuTitle) << endl;
                QSqlQuery *pq2 = newQuery();
                action(pm->addAction(sm.getName(_sMenuTitle)), sm, pq2);
                delete pq2;
            } while (sm.next(*pq));
        }
        else DWAR() << trUtf8("üres menü : ") << _mi.getName(_sMenuTitle) << endl;
    }
    else {
        new cMenuAction(pq, &_mi, pa, pMdiArea);
    }
    DBGFNL();
}

/*
void cMainWindow::widgetSplitterOrientation(int index)
{
    QWidget *tab = pTabWidget->widget(index);
    if (tab == NULL) return;
    // minden tab tartalma egy Widget/layout -ban van, a layout.nak egy eleme van
    QLayout * ply = tab->layout();
    if (ply == NULL) return;
    QLayoutItem *pli =  ply->itemAt(0);
    if (pli == NULL) return;
    QWidget *pw = pli->widget();    // Az a widget jeleníti meg a valós tartalmat
    if (pw == NULL) return;
    // Ha ő egy splitter, akkor ennek akarjuk az orientációját váltani
    bool f = pw->inherits("QSplitter");
    if (!f) return;
    QSplitter *psp = dynamic_cast<QSplitter *>(pw);
    switch (psp->orientation()) {
    case Qt::Horizontal:    psp->setOrientation(Qt::Vertical);   break;
    case Qt::Vertical:      psp->setOrientation(Qt::Horizontal); break;
    }
}
*/
