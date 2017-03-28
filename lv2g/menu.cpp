#include "menu.h"
#include "record_table.h"
#include "record_tree.h"
#include "onlinealarm.h"
#include "setup.h"
#include "gsetupwidget.h"
#include "gparse.h"
#include "setnoalarm.h"
#include "hsoperate.h"
#include "findbymac.h"
#include "apierrcodes.h"
#include "workstation.h"

QMap<QString, QAction *>  cMenuAction::actionsMap;

cMenuAction::cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QMdiArea * par, eEx __ex)
    : QObject(par), type(MAT_ERROR)
{
    pMdiArea     = par;
    pIntSubObj      = NULL;
    intType      = INT_UNKNOWN;
    pDialog      = NULL;
    pAction      = pa;
    pMenuItem    = new cMenuItem(*pmi);

    setObjectName(pmi->getName());
    actionsMap.insert(pa->objectName(), pa);
    QString feature;
    if      (!(feature = pmi->feature("shape")).isEmpty()) {
        if (pq == NULL) EXCEPTION(EPROGFAIL);   // Ha nincs adatbázis, akkor ezt nem kéne
        setObjectName(feature);
        setType(MAT_SHAPE);
        cTableShape tableShape;;
        tableShape.setByName(feature);
        if (lanView::isAuthorized(tableShape.getId(_sViewRights))) {           // Jog?
            connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
        }
        else {
            pa->setDisabled(true);
        }
    }
    else if (!(feature = pmi->feature("exec")).isEmpty()) { // "exec"   Belső parancs végrehajtása (Exit, Restart,...)
        setObjectName(feature); // Nincs tab, a név a parancs lessz
        setType(MAT_EXEC);
        if (lanView::isAuthorized(pmi->getId(_sMenuRights))) {           // Jog?
            connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
        }
        else {
            pa->setDisabled(true);
        }
    }
    else if (!(feature = pmi->feature("own")).isEmpty()) { // "own"    Egyedi előre definiált cOwnTab leszármazott hívása
        enum ePrivilegeLevel rights = PL_SYSTEM;
        if (0 == feature.compare("setup", Qt::CaseInsensitive)) {            // "setup"      Alap beállítások szerkesztése widget
            intType = INT_SETUP;
            rights = cSetupWidget::rights;
        }
        else if (0 == feature.compare("gsetup", Qt::CaseInsensitive)) {      // "gsetup"     GUI beállítások szerkesztése widget
            intType = INT_GSETUP;
            rights = cGSetupWidget::rights;
        }
        else if (0 == feature.compare("parser", Qt::CaseInsensitive)) {      // "parser"     A parser widget
            intType = INT_PARSER;
            rights = cParseWidget::rights;
        }
        else if (0 == feature.compare("olalarm", Qt::CaseInsensitive)) {     // "olalarm"    On-Line riasztások widget
            intType = INT_OLALARM;
            rights = cOnlineAlarm::rights;
        }
        else if (0 == feature.compare("errcodes", Qt::CaseInsensitive)) {    // "errcodes"   API hibakódok
            intType = INT_ERRCODES;
            rights = cErrcodesWidget::rights;
        }
        else if (0 == feature.compare("noalarm", Qt::CaseInsensitive)) {     // "noalarm"   Riasztás tiltások beállítása
            intType = INT_NOALARM;
            rights = cSetNoAlarm::rights;
        }
        else if (0 == feature.compare("hsop", Qt::CaseInsensitive)) {       // "hsop"   host-services állpot man.
            intType = INT_HSOP;
            rights = cHSOperate::rights;
        }
        else if (0 == feature.compare("findmac", Qt::CaseInsensitive)) {    // "findmac"
            intType = INT_FINDMAC;
            rights = PL_VIEWER;
        }
        else if (0 == feature.compare(_sWorkstation, Qt::CaseInsensitive)) { // "workstation"
            intType = INT_WORKSTATION;
            rights = PL_VIEWER;
        }
        else {
            if (__ex) EXCEPTION(ENONAME, -1, feature);
            return;
        }
        setType(MAT_OWN);
        if (lanView::isAuthorized(rights)) {
            connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
        }
        else {
            pa->setDisabled(true);
        }
    }
    else if (__ex) EXCEPTION(EDBDATA);
}

cMenuAction::cMenuAction(const QString&  ps, QAction *pa, QMdiArea * par)
    : QObject(par), type(MAT_EXEC)
{
    pMdiArea     = par;
    pIntSubObj   = NULL;
    intType      = INT_UNKNOWN;
    pDialog      = NULL;
    pAction      = pa;
    pMenuItem    = NULL;

    setObjectName(ps);
    connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
    actionsMap.insert(pa->objectName(), pa);
}

cMenuAction::cMenuAction(cIntSubObj *po, eIntSubWin t, QAction *pa, QMdiArea * par)
    : QObject(par), type(MAT_WIDGET)
{
    pMdiArea     = par;
    intType      = t;
    pIntSubObj   = po;
    pDialog      = NULL;
    pAction      = pa;
    pMenuItem    = NULL;

    connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
    actionsMap.insert(pa->objectName(), pa);
}

cMenuAction::~cMenuAction()
{
    ;
}

void cMenuAction::initRecordTable()
{
     pIntSubObj = new cTableSubWin(objectName(), pMdiArea);
     connect(pIntSubObj,             SIGNAL(closeIt()),   this, SLOT(removeIt()));
     connect(pIntSubObj->pSubWindow, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

/** Egy megjelenítést végző cIntSubObj leszármazott objektum létrehozása

A menüben az "own=<típus>" stringgel lehet megadni a properties mezőben megjelenítő típusát.
A jelenleg implementállt lehetőségek:
| Típus név  | Konstans név   | Objektum név    | Funkció                     |
|------------|----------------|-----------------|-----------------------------|
| setup      | INT_SETUP      | cSetupWidget    | Alapbeállítások megadása    |
| gsetup     | INT_GSETUP     | cGSetupWidget   | Megjelenítési beállítások   |
| parser     | INT_PARSER     | cParseWidget    | A parser hvása              |
| olalarm    | INT_OLALARM    | cOnlineAlarm    | OnLine riasztások           |
| errcodes   | INT_ERRCODES   | cErrcodesWidget | API hibakódok listája       |
| noalarm    | INT_NOALARM    | cSetNoAlarm     | Riasztás tiltások beállítása|
| hsop       | INT_HSOP       | cHSOperate      | host-services állapot man.  |
| findmac    | INT_FINDMAC    | cFindByMac      | MAC keresés                 |
| workstation| INT_WORKSTATION| cWorkstation    | Munkaállomás új/modosít     |
 */
void cMenuAction::initInt()
{
    switch (intType) {
#define CREATEINTWIN(c,t)    case INT_##c: pIntSubObj = new t(pMdiArea); break
    CREATEINTWIN(SETUP,      cSetupWidget);
    CREATEINTWIN(GSETUP,     cGSetupWidget);
    CREATEINTWIN(PARSER,     cParseWidget);
    CREATEINTWIN(OLALARM,    cOnlineAlarm);
    CREATEINTWIN(ERRCODES,   cErrcodesWidget);
    CREATEINTWIN(NOALARM,    cSetNoAlarm);
    CREATEINTWIN(HSOP,       cHSOperate);
    CREATEINTWIN(FINDMAC,    cFindByMac);
    CREATEINTWIN(WORKSTATION,cWorkstation);
#undef CREATEINTWIN
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    connect(pIntSubObj,      SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pIntSubObj->pSubWindow,      SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

void cMenuAction::displayIt()
{
    if (pMdiArea == NULL) {
        EXCEPTION(EPROGFAIL, -1, trUtf8("pMdiArea is NULL"));
    }
    if (pIntSubObj == NULL) {
        switch (type) {
        case MAT_SHAPE:     initRecordTable();                          break;
        case MAT_OWN:       initInt();                                  break;
        case MAT_WIDGET:    EXCEPTION(ENOTSUPP, type);                  break;
        default:            EXCEPTION(EPROGFAIL,-1,"Invalid signal.");  break;
        }
        pIntSubObj->setWindowTitle(pMenuItem->getName(_sTabTitle));
    }
    pMdiArea->setActiveSubWindow(pIntSubObj->pSubWindow);
    pIntSubObj->pWidget()->show();
}

void cMenuAction::removeIt()
{
    if (pIntSubObj == NULL) {
        DWAR() << "pIntSubObj is NULL." << endl;
        return;
    }
    pMdiArea->removeSubWindow(pIntSubObj->pSubWindow);
    pDelete(pIntSubObj);
}

void cMenuAction::destroyedChild()
{
    DBGFN();
    pDelete(pIntSubObj);
}

void  cMenuAction::executeIt()
{
    switch (type) {
    case MAT_DIALOG: {
        if (pDialog == NULL) EXCEPTION(EPROGFAIL, -1, "pDialog is NULL.");
        QString name = pDialog->objectName();
        PDEB(VERBOSE) << "Start " << name << " dialog..." << endl;
        int r = pDialog->exec();
        PDEB(INFO) << "Return " << name << " dialog :" << r << endl;
    }
        break;
    case MAT_EXEC: {
        QString name = objectName();
        if      (0 == name.compare("exit",    Qt::CaseInsensitive)) qApp->exit(0);
        else if (0 == name.compare("restart", Qt::CaseInsensitive)) appReStart();
        else if (0 == name.compare("tabs",    Qt::CaseInsensitive)) pMdiArea->setViewMode(QMdiArea::TabbedView);
        else if (0 == name.compare("windows", Qt::CaseInsensitive)) pMdiArea->setViewMode(QMdiArea::SubWindowView);
        else if (0 == name.compare("close",   Qt::CaseInsensitive)) pMdiArea->closeAllSubWindows();
        else    EXCEPTION(EDBDATA,-1, name);
    }
        break;
    default:
        DERR() << "Invalid type : #" << (int)type << endl;
        break;
    }
}

cTableSubWin::cTableSubWin(const QString& shape, QMdiArea * pMdiArea)
    : cIntSubObj(pMdiArea)
{
    pTableShape = new cTableShape();
    pTableShape->setParent(this);
    pTableShape->setByName(shape);
    pRecordsView = cRecordsViewBase::newRecordView(pTableShape, NULL, this);
    QHBoxLayout *pHBL = new QHBoxLayout;
    pWidget()->setLayout(pHBL);
    pHBL->addWidget(pRecordsView->pWidget());
    connect(pRecordsView, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pRecordsView, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}
