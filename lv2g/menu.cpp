#include "menu.h"
#include "setup.h"
#include "gparse.h"

cMenuAction::cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QTabWidget * par, bool __ex)
    : QObject(par), type(MAT_ERROR)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordTable = NULL;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;

    setObjectName(pmi->title()); // Ez lessz a TAB neve
    tMagicMap mm;
    mm = splitMagic(pmi->getName(_sProperties), mm);
    QString mp;
    if      (!(mp = magicParam(QString("shape"),  mm)).isEmpty()) {
        if (pq == NULL) EXCEPTION(EPROGFAIL);   // Ha nincs adatbázis, akkor ezt nem kéne
        pTableShape = new cTableShape;
        pTableShape->setParent(this);
        pTableShape->setByName(*pq, mp);
        setType(MAT_SHAPE);
        connect(pAction,      SIGNAL(triggered()), this, SLOT(displayIt()));
    }
    else if (!(mp = magicParam(QString("exec"),   mm)).isEmpty()) {
        setObjectName(mp); // Nincs tab, a név a parancs lessz
        setType(MAT_EXEC);
        connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
    }
    else if (!(mp = magicParam(QString("own"),    mm)).isEmpty()) {
        cOwnTab *pot =  NULL;
        if (0 == mp.compare("setup", Qt::CaseInsensitive)) {
            pot =  new cSetupWidget(*lanView::getInstance()->pSet, par);
            pWidget = (QWidget *)pot;
        }
        else if (0 == mp.compare("parser", Qt::CaseInsensitive)) {
            pot =  new cParseWidget(par);
            pWidget = (QWidget *)pot;
        }
        else {
            if (__ex) EXCEPTION(ENONAME, -1, mp);
            return;
        }
        connect(pAction,      SIGNAL(triggered()), this, SLOT(displayIt()));
        connect(pot,          SIGNAL(closeIt()),   this, SLOT(removeIt()));
        connect(pWidget,      SIGNAL(destroyed()), this, SLOT(destroyedChild()));
        setType(MAT_OWN);
    }
    else if (__ex) EXCEPTION(EDBDATA);
}

cMenuAction::cMenuAction(cTableShape *ps, const QString &nm, QAction *pa, QTabWidget *par)
    : QObject(par), type(MAT_SHAPE)
{
    pTabWidget   = par;
    pTableShape  = ps;
    pRecordTable = NULL;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;

    pTableShape->setParent(this);
    connect(pAction,      SIGNAL(triggered()), this, SLOT(displayIt()));
    connect(pRecordTable, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    setObjectName(nm);
}

cMenuAction::cMenuAction(const QString&  ps, QAction *pa, QTabWidget * par)
    : QObject(par), type(MAT_EXEC)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordTable = NULL;
    pWidget      = NULL;
    pDialog      = NULL;
    pAction      = pa;

    setObjectName(ps);
    connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
}

cMenuAction::cMenuAction(cOwnTab *po, QAction *pa, QTabWidget * par)
    : QObject(par), type(MAT_WIDGET)
{
    pTabWidget   = par;
    pTableShape  = NULL;
    pRecordTable = NULL;
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
    pRecordTable = new cRecordTable(pTableShape, false, pTabWidget);
    pWidget = pRecordTable->pWidget();
    connect(pRecordTable, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pRecordTable, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

void cMenuAction::displayIt()
{
    switch (type) {
    case MAT_SHAPE:
        if (pWidget == NULL) initRecordTable();
        break;
    case MAT_OWN:
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
        delete pRecordTable;    // Ha becsukta, akkor csinalunk az objektumnal egy resetet
        pRecordTable = NULL;
        break;
    default:
        break;
    }
}

void cMenuAction::destroyedChild()
{
    DBGFN();
    pWidget = NULL;         // A tab tényleg elitézi a többit?
    pRecordTable = NULL;
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

