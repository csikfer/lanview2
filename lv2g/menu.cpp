#include "menu.h"
#include "setup.h"
#include "gsetupwidget.h"
#include "gparse.h"
#include "setnoalarm.h"
#include "hsoperate.h"
#include "apierrcodes.h"

QMap<QString, QAction *>  cMenuAction::actionsMap;

cMenuAction::cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QTabWidget * par, eEx __ex)
    : QObject(par), type(MAT_ERROR)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordView  = NULL;
    pOwnTab      = NULL;
    pWidget      = NULL;
    ownType      = OWN_UNKNOWN;
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
        pTableShape = new cTableShape();
        pTableShape->setParent(this);
        pTableShape->setByName(feature);
        if (lanView::isAuthorized(pTableShape->getId(_sViewRights))) {           // Jog?
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
            ownType = OWN_SETUP;
            rights = cSetupWidget::rights;
        }
        else if (0 == feature.compare("gsetup", Qt::CaseInsensitive)) {      // "gsetup"     GUI beállítások szerkesztése widget
            ownType = OWN_GSETUP;
            rights = cGSetupWidget::rights;
        }
        else if (0 == feature.compare("parser", Qt::CaseInsensitive)) {      // "parser"     A parser widget
            ownType = OWN_PARSER;
            rights = cParseWidget::rights;
        }
        else if (0 == feature.compare("olalarm", Qt::CaseInsensitive)) {     // "olalarm"    On-Line riasztások widget
            ownType = OWN_OLALARM;
            rights = cOnlineAlarm::rights;
        }
        else if (0 == feature.compare("errcodes", Qt::CaseInsensitive)) {    // "errcodes"   API hibakódok
            ownType = OWN_ERRCODES;
            rights = cErrcodesWidget::rights;
        }
        else if (0 == feature.compare("noalarm", Qt::CaseInsensitive)) {     // "noalarm"   Riasztás tiltások beállítása
            ownType = OWN_NOALARM;
            rights = cSetNoAlarm::rights;
        }
        else if (0 == feature.compare("hsop", Qt::CaseInsensitive)) {       // "hsop"   host-services állpot man.
            ownType = OWN_HSOP;
            rights = cHSOperate::rights;
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

cMenuAction::cMenuAction(const QString&  ps, QAction *pa, QTabWidget * par)
    : QObject(par), type(MAT_EXEC)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordView  = NULL;
    pOwnTab      = NULL;
    ownType      = OWN_UNKNOWN;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;
    pMenuItem    = NULL;

    setObjectName(ps);
    connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
    actionsMap.insert(pa->objectName(), pa);
}

cMenuAction::cMenuAction(cOwnTab *po, eOwnTab t, QAction *pa, QTabWidget * par)
    : QObject(par), type(MAT_WIDGET)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordView  = NULL;
    ownType      = t;
    pWidget      = po;
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
    if (pTableShape == NULL) {
        QString shape = objectName();
        pTableShape = new cTableShape();
        pTableShape->setParent(this);
        pTableShape->setByName(shape);
    }
    pRecordView = cRecordsViewBase::newRecordView(pTableShape, NULL, pTabWidget);
    pWidget = pRecordView->pWidget();
    connect(pRecordView, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pRecordView, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

/** Egy megjelenítést végző cOwnTab leszármazott objektum létrehozása

A menüben az "own=<típus>" stringgel lehet megadni a properties mezőben megjelenítő típusát.
A jelenleg implementállt lehetőségek:
| Típus név | Konstans név | Objektum név    | Funkció                     |
|-----------|--------------|-----------------|-----------------------------|
| setup     | OWN_SETUP    | cSetupWidget    | Alapbeállítások megadása    |
| gsetup    | OWN_GSETUP   | cGSetupWidget   | Megjelenítési beállítások   |
| parser    | OWN_PARSER   | cParseWidget    | A parser hvása              |
| olalarm   | OWN_OLALARM  | cOnlineAlarm    | OnLine riasztások           |
| errcodes  | OWN_ERRCODES | cErrcodesWidget | API hibakódok listája       |
| noalarm   | OWN_NOALARM  | cSetNoAlarm     | Riasztás tiltások beállítása|
| hsop      | OWN_HSOP     | cHSOperate      | host-services állapot man.  |
 */
void cMenuAction::initOwn()
{
    switch (ownType) {
    case OWN_SETUP:
        pOwnTab = new cSetupWidget(*lanView::getInstance()->pSet, pTabWidget);
        break;
    case OWN_GSETUP:
        pOwnTab = new cGSetupWidget(*lanView::getInstance()->pSet, pTabWidget);
        break;
    case OWN_PARSER:
        pOwnTab = new cParseWidget(pTabWidget);
        break;
    case OWN_OLALARM:
        pOwnTab = new cOnlineAlarm(pTabWidget);
        break;
    case OWN_ERRCODES:
        pOwnTab = new cErrcodesWidget(pTabWidget);
        break;
    case OWN_NOALARM:
        pOwnTab = new cSetNoAlarm(pTabWidget);
        break;
    case OWN_HSOP:
        pOwnTab = new cHSOperate(pTabWidget);
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    pWidget = pOwnTab->pWidget();
    connect(pOwnTab,      SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pWidget,      SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

void cMenuAction::displayIt()
{
    switch (type) {
    case MAT_SHAPE:
        if (pWidget == NULL) initRecordTable();
        break;
    case MAT_OWN:
        if (pWidget == NULL) initOwn();
        break;
    case MAT_WIDGET:
        if (pWidget == NULL) {
            EXCEPTION(EPROGFAIL,-1,"pWidget is NULL.");
        }
        break;
    default:
        EXCEPTION(EPROGFAIL,-1,"Invalid signal.");
        break;
    }
    if (pTabWidget == NULL) {
        EXCEPTION(EPROGFAIL, -1, trUtf8("pTabWidget is NULL"));
    }
    int i = pTabWidget->indexOf(pWidget);
    if (i < 0) {
        QString title =  pMenuItem == NULL ? objectName() : pMenuItem->getName(_sTabTitle);
        i = pTabWidget->addTab(pWidget, title);
    }
    pTabWidget->setCurrentIndex(i);
}

void cMenuAction::removeIt()
{
    if (pWidget == NULL) {
        DWAR() << "pWidget is NULL." << endl;
        return;
    }
    int i = pTabWidget->indexOf(pWidget);
    if (i < 0) EXCEPTION(EPROGFAIL);
    pTabWidget->removeTab(i);
    pWidget->setVisible(false);
    switch (type) {
    case MAT_SHAPE:
        pDelete(pRecordView);
        pTableShape = NULL; // A fenti obj. destruktora ezt is törölte!!
        break;
    case MAT_OWN:
        pDelete(pOwnTab);
        break;
    case MAT_WIDGET:
        return; // itt nem kéne nullázni a pWidget-et
        break;
    default:
        EXCEPTION(EPROGFAIL,type,"Invalid type.");
        break;
    }
    pWidget = NULL;
}

void cMenuAction::destroyedChild()
{
    DBGFN();
    pWidget = NULL;         // A tab tényleg elintézi a többit?
    pRecordView = NULL;
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
        else    EXCEPTION(EDBDATA,-1, name);
    }
        break;
    default:
        DERR() << "Invalid type : #" << (int)type << endl;
        break;
    }
}

