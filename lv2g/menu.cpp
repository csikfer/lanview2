#include "menu.h"
#include "setup.h"
#include "gparse.h"

cMenuAction::cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QTabWidget * par, bool __ex)
    : QObject(par), type(MAT_ERROR), mm()
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordView  = NULL;
    pOwnTab      = NULL;
    pWidget      = NULL;
    ownType      = OWN_UNKNOWN;
    pDialog      = NULL;
    pAction      = pa;

    setObjectName(pmi->title()); // Ez lessz a TAB neve
    mm = splitMagic(pmi->getName(_sProperties), mm);
    QString mp;
    if      (!(mp = magicParam(QString("shape"),  mm)).isEmpty()) {
        if (pq == NULL) EXCEPTION(EPROGFAIL);   // Ha nincs adatbázis, akkor ezt nem kéne
        setObjectName(mp);
        setType(MAT_SHAPE);
        pTableShape = new cTableShape();
        pTableShape->setParent(this);
        pTableShape->setByName(mp);
        if (lanView::isAuthorized(pTableShape->getId(_sViewRights))) {           // Jog?
            connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
        }
        else {
            pa->setDisabled(true);
        }
    }
    else if (!(mp = magicParam(QString("exec"),   mm)).isEmpty()) { // "exec"   Belső parancs végrehajtása (Exit, Restart,...)
        setObjectName(mp); // Nincs tab, a név a parancs lessz
        setType(MAT_EXEC);
        connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
    }
    else if (!(mp = magicParam(QString("own"),    mm)).isEmpty()) { // "own"    Egyedi előre definiált cOwnTab leszármazott hívása
        enum ePrivilegeLevel rights = PL_SYSTEM;
        if (0 == mp.compare("setup", Qt::CaseInsensitive)) {            // "setup"      Beállítások szerkesztése widget
            ownType = OWN_SETUP;
            rights = cSetupWidget::rights;
        }
        else if (0 == mp.compare("parser", Qt::CaseInsensitive)) {      // "parser"     A parser widget
            ownType = OWN_PARSER;
            rights = cParseWidget::rights;
        }
        else if (0 == mp.compare("olalarm", Qt::CaseInsensitive)) {     // "olalarm"    On-Line riasztások widget
            ownType = OWN_OLALARM;
            rights = cOnlineAlarm::rights;
        }
        else {
            if (__ex) EXCEPTION(ENONAME, -1, mp);
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

/*
cMenuAction::cMenuAction(cTableShape *ps, const QString &nm, QAction *pa, QTabWidget *par)
    : QObject(par), type(MAT_SHAPE)
{
    pTabWidget   = par;
    pTableShape  = ps;
    pRecordTable = NULL;
    pOwnTab      = NULL;
    ownType      = OWN_UNKNOWN;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;

    pTableShape->setParent(this);
    connect(pAction,      SIGNAL(triggered()), this, SLOT(displayIt()));
    connect(pRecordTable, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    setObjectName(nm);
}
*/

cMenuAction::cMenuAction(const QString&  ps, QAction *pa, QTabWidget * par)
    : QObject(par), type(MAT_EXEC)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordView = NULL;
    pOwnTab      = NULL;
    ownType      = OWN_UNKNOWN;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;

    setObjectName(ps);
    connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
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

    connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
}

cMenuAction::~cMenuAction()
{
    ;
}

void cMenuAction::initRecordTable()
{
    if (pTableShape == NULL) EXCEPTION(EPROGFAIL);
    pRecordView = cRecordsViewBase::newRecordView(pTableShape, NULL, pTabWidget);
    pWidget = pRecordView->pWidget();
    connect(pRecordView, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pRecordView, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

void cMenuAction::initOwn()
{
    switch (ownType) {
    case OWN_SETUP:
        pOwnTab = new cSetupWidget(*lanView::getInstance()->pSet, pTabWidget);
        break;
    case OWN_PARSER:
        pOwnTab = new cParseWidget(pTabWidget);
        break;
    case OWN_OLALARM:
        pOwnTab = new cOnlineAlarm(pTabWidget);
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
        i = pTabWidget->addTab(pWidget, objectName());
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
    pWidget = NULL;         // A tab tényleg elitézi a többit?
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

