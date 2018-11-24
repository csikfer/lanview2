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
    QString title = trUtf8("LanView %1 V%2, API V%3")
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
            QString msg = trUtf8("Nincs menü a GUI applikációhoz");
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
    QAction *pFile = menuBar()->addAction(trUtf8("File"));
    QMenu   *pm    = new QMenu(menuBar());
    pFile->setMenu(pm);
    QAction *pa;
    cMenuAction *po;

    QString nm = trUtf8("Setup");
    pa = pm->addAction(nm);
    cIntSubObj *pot =  new cSetupWidget(nullptr);
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
                    QString mm = trUtf8("A hiba miatt a %1:%2 nevű menüpont figyelmen kívül lessz hagyva.")
                            .arg(sm.getName(_sAppName), sm.getName());
                    cErrorMessageBox::messageBox(pe, this, mm);
                }
            } while (sm.next(*pq));
        }
        else DWAR() << trUtf8("üres menü : ") << _mi.getName() << endl;
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
