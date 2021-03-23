#include "mainwindow.h"
#include "menu.h"
#include "setup.h"
#include "gparse.h"
#include "popupreport.h"



cMainWindow::cMainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    if (lv2g::pMainWindow != nullptr) EXCEPTION(EPROGFAIL);
    lv2g::pMainWindow = this;
    QString title = tr("LanView %1 V%2, API V%3")
            .arg(lanView::appName, lanView::appVersion, lanView::libVersion);
    setWindowTitle(title);
    setWindowIcon(QIcon("://icons/LanView2.ico"));
    pMdiArea = new QMdiArea(this);
    pMdiArea->setBackground(QBrush(QImage(":/LanView2.png")));
    pMdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pMdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pMdiArea->setDocumentMode(true);
    pMdiArea->setViewMode(QMdiArea::TabbedView);
    pMdiArea->setTabsMovable(true);
    pMdiArea->setTabsClosable(true);
    setCentralWidget(pMdiArea);
}

void cMainWindow::init(bool _setup)
{
    // Central Widget
    if (_setup || !lanView::dbIsOpen()) {   // Minimal setup
        setSetupMenu();
    }
    else {        // Menu
        cMenuItem mi;
        QSqlQuery *pq2 = newQuery();
        QSqlQuery *pqm = newQuery();
        if (!mi.fetchFirstItem(*pqm, lanView::getInstance()->appName)) {
            QString msg = tr("Nincs menü a GUI applikációhoz");
            if (lanView::isAuthorized(PL_ADMIN)) {
                cMsgBox::warning(msg, this);
                setSetupMenu();
            }
            else {
                EXCEPTION(EDBDATA, 0, msg);
            }
        }
        else do {
            mi.fetchText(*pq2);
            QString t = mi.getText(cMenuItem::LTX_MENU_TITLE, mi.getName());
            action(menuBar()->addAction(t), mi, pq2);
        } while(mi.next(*pqm));
        delete pq2;
        delete pqm;
    }
}

cMainWindow::~cMainWindow()
{
    lv2g::pMainWindow = nullptr;
}

void cMainWindow::setSetupMenu()
{
    QAction *pFile = menuBar()->addAction(tr("File"));
    QMenu   *pm    = new QMenu(menuBar());
    pFile->setMenu(pm);
    QAction *pa;
    cMenuAction *po;

    QString nm = tr("Setup");
    pa = pm->addAction(nm);
    cIntSubObj *pot =  new cSetupWidget(nullptr);
    pot->setObjectName(nm);
    po  = new cMenuAction(pot, INT_SETUP, pa, pMdiArea);
    po->setObjectName(nm);

    nm = tr("Restart");
    pa = pm->addAction(nm);
    po  = new cMenuAction(nm, pa, pMdiArea);
    po->setObjectName(nm);

    nm = tr("Exit");
    pa = pm->addAction(nm);
    po  = new cMenuAction(nm, pa, pMdiArea);
    po->setObjectName(nm);

    if (lanView::dbIsOpen()) {
        QAction *pTools = menuBar()->addAction(tr("Tools"));
        QMenu   *pm    = new QMenu(menuBar());
        pTools->setMenu(pm);
        QAction *pa;
        cMenuAction *po;

        QString nm = tr("Import");
        pa = pm->addAction(nm);
        cIntSubObj *pot =  new cParseWidget(nullptr);
        pot->setObjectName(nm);
        po  = new cMenuAction(pot, INT_PARSER, pa, pMdiArea);
        po->setObjectName(nm);
    }
}

void cMainWindow::action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq)
{
    _DBGFN() << _mi.toString() << endl;
    pa->setObjectName(_mi.getName());
    QString s;
    if (!(s = _mi.getText(cMenuItem::LTX_TOOL_TIP)  ).isEmpty()) pa->setToolTip(s);
    if (!(s = _mi.getText(cMenuItem::LTX_WHATS_THIS)).isEmpty()) pa->setWhatsThis(s);
    if (_mi.getId(cMenuItem::ixMenuItemType()) == MT_MENU) {    // Almenük vannak
        cMenuItem sm;
        QMenu *pm = new QMenu(this);
        pm->setObjectName(sm.getName());
        pa->setMenu(pm);
        if (pq == nullptr) EXCEPTION(EPROGFAIL);
        if (sm.fetchFirstItem(*pq, _mi.getName(_sAppName), _mi.getId())) {
            PDEB(VERBOSE) << "Menu :" << _mi.getName() << " #" << pq->size() << " sub menü:" << endl;
            do {
                cError *pe = nullptr;
                try {
                    sm.splitFeature(); // features
                } CATCHS(pe);
                if (pe == nullptr) {
                    PDEB(VERBOSE) << _mi.getName() << " sub menu : " << sm.getName() << endl;
                    QSqlQuery *pq2 = newQuery();
                    sm.fetchText(*pq2);
                    QString t = sm.getText(cMenuItem::LTX_MENU_TITLE, sm.getName());
                    action(pm->addAction(t), sm, pq2);
                    delete pq2;
                }
                else {  // Nem ok a features mező
                    QString mm = tr("A hiba miatt a %1:%2 nevű menüpont figyelmen kívül lessz hagyva.")
                            .arg(sm.getName(_sAppName), sm.getName());
                    cErrorMessageBox::messageBox(pe, this, mm);
                }
            } while (sm.next(*pq));
        }
        else DWAR() << tr("üres menü : ") << _mi.getName() << endl;
    }
    else {
        new cMenuAction(pq, &_mi, pa, pMdiArea);
    }
    DBGFNL();
}

