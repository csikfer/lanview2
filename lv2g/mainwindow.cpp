#include "mainwindow.h"
#include "menu.h"
#include "setup.h"



cMainWindow::cMainWindow(bool _setupOnly, QWidget *parent) :
    QMainWindow(parent)
{
    pTabWidget = new QTabWidget(this);
    // Central Widget
    setCentralWidget(pTabWidget);
    if (_setupOnly) {   // Minimalista setup, nincs adatbázisunk, vagy csak ez kell
        QAction *pFile = menuBar()->addAction(trUtf8("File"));
        QMenu   *pm    = new QMenu(menuBar());
        pFile->setMenu(pm);
        QAction *pa;
        cMenuAction *po;

        QString nm = trUtf8("Setup");
        pa = pm->addAction(nm);
        cOwnTab *pot =  new cSetupWidget(*lanView::getInstance()->pSet, this);
        pot->setObjectName(nm);
        po  = new cMenuAction(pot, OWN_SETUP, pa, pTabWidget);
        po->setObjectName(nm);

/*      nm = trUtf8("Restart");
        pa = pm->addAction(nm);
        po  = new cMenuAction(nm, pa, pTabWidget);
        po->setObjectName(nm);*/

        nm = trUtf8("Exit");
        pa = pm->addAction(nm);
        po  = new cMenuAction(nm, pa, pTabWidget);
        po->setObjectName(nm);
    }
    else {        // Menu
        cMenuItem mi;
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
                PDEB(VERBOSE) << _mi.title() << " sub menu : " << sm.title() << endl;
                QSqlQuery *pq2 = newQuery();
                action(pm->addAction(sm.title()), sm, pq2);
                delete pq2;
            } while (sm.next(*pq));
        }
        else DWAR() << trUtf8("üres menü : ") << _mi.title() << endl;
    }
    else {
        new cMenuAction(pq, &_mi, pa, pTabWidget);
    }
    DBGFNL();
}

