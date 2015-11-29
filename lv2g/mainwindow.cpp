#include "mainwindow.h"
#include "menu.h"
#include "setup.h"
#include "gparse.h"



cMainWindow::cMainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    pTabWidget = new QTabWidget(this);
    // Central Widget
    setCentralWidget(pTabWidget);
    if (!lanView::dbIsOpen()) {   // Minimalista setup, nincs adatbázisunk, vagy csak ez kell
        setSetupMenu();
    }
    else {        // Menu
        cMenuItem mi;
        QSqlQuery *pqm = newQuery();
        if (!mi.fetchFirstItem(*pqm, lanView::getInstance()->appName)) {
            QString msg = trUtf8("Nincs menü a GUI applikációhoz");
            if (lanView::isAuthorized(PL_ADMIN)) {
                QMessageBox::warning(NULL, trUtf8("Figyelmeztetés!"), msg);
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
    ;
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
    cOwnTab *pot =  new cSetupWidget(*lanView::getInstance()->pSet, NULL);
    pot->setObjectName(nm);
    po  = new cMenuAction(pot, OWN_SETUP, pa, pTabWidget);
    po->setObjectName(nm);

    nm = trUtf8("Restart");
    pa = pm->addAction(nm);
    po  = new cMenuAction(nm, pa, pTabWidget);
    po->setObjectName(nm);

    nm = trUtf8("Exit");
    pa = pm->addAction(nm);
    po  = new cMenuAction(nm, pa, pTabWidget);
    po->setObjectName(nm);

    if (lanView::dbIsOpen()) {
        QAction *pTools = menuBar()->addAction(trUtf8("Tools"));
        QMenu   *pm    = new QMenu(menuBar());
        pTools->setMenu(pm);
        QAction *pa;
        cMenuAction *po;

        QString nm = trUtf8("Import");
        pa = pm->addAction(nm);
        cOwnTab *pot =  new cParseWidget(NULL);
        pot->setObjectName(nm);
        po  = new cMenuAction(pot, OWN_PARSER, pa, pTabWidget);
        po->setObjectName(nm);
    }
}

void cMainWindow::action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq)
{
    _DBGFN() << _mi.toString() << endl;
    if (!_mi.isNull(_sToolTip))     pa->setToolTip(_mi.getName(_sToolTip));
    if (!_mi.isNull(_sWhatsThis))   pa->setWhatsThis(_mi.getName(_sWhatsThis));
    QString mp;
    if (_mi.isFeature("sub")) {    // Almenük vannak
        cMenuItem sm;
        QMenu *pm = new QMenu(this);
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
        new cMenuAction(pq, &_mi, pa, pTabWidget);
    }
    DBGFNL();
}

