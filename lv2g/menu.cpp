#include "menu.h"
#include "record_table.h"
#include "record_tree.h"
#include "onlinealarm.h"
#include "setup.h"
#include "gsetupwidget.h"
#include "gparse.h"
#include "gexports.h"
#include "hsoperate.h"
#include "findbymac.h"
#include "apierrcodes.h"
#include "workstation.h"
#include "deducepatch.h"
#include "object_dialog.h"
#include "snmpdevquery.h"
#include "translator.h"
#include "changeswitch.h"

QMap<QString, QAction *>  cMenuAction::actionsMap;

cMenuAction::cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QMdiArea * par)
    : QObject(par), type(MAT_ERROR)
{
    pMdiArea     = par;
    pIntSubObj   = nullptr;
    intType      = INT_UNKNOWN;
    pDialog      = nullptr;
    pAction      = pa;
    pMenuItem    = new cMenuItem(*pmi);

    setObjectName(pmi->getName());
    actionsMap.insert(pa->objectName(), pa);
    QString param = pmi->getName(cMenuItem::ixMenuParam());
    int itemType = int(pmi->getId(cMenuItem::ixMenuItemType()));
    cTableShape tableShape;
    enum ePrivilegeLevel rights = PL_SYSTEM;
    QString msg;
    switch (itemType) {
    case MT_SHAPE:
        if (pq == nullptr) EXCEPTION(EPROGFAIL);   // Ha nincs adatbázis, akkor ezt nem kéne
        setObjectName(param);
        setType(MAT_SHAPE);
        if (!tableShape.fetchByName(param)) {
            msg = tr("A %1 nevű menúben a %2 nevű tábla megelenítő rekordot (table_shapes) nem sikerült beolvasni.")
                    .arg(pmi->getName(), param);
            msg += tr("A leíró vaószínüleg nem létezik. A menüpont le lessz tiltva.");
            cMsgBox::warning(msg, par);
            pa->setDisabled(true);
        }
        else if (lanView::isAuthorized(tableShape.getId(_sViewRights))) {           // Jog?
            connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
        }
        else {
            pa->setDisabled(true);
        }
        break;
    case MT_OWN:
#define _SETINTWIN(S, D, C) \
        if (0 == param.compare(S, Qt::CaseInsensitive)) { \
            intType = D; \
            rights = C::rights; \
        }
#define SETINTWIN(S, D, C) else _SETINTWIN(S, D, C)
       _SETINTWIN("setup",      INT_SETUP,      cSetupWidget)
        SETINTWIN("gsetup",     INT_GSETUP,     cGSetupWidget)
        SETINTWIN("parser",     INT_PARSER,     cParseWidget)
        SETINTWIN("export",     INT_EXPORT,     cExportsWidget)
        SETINTWIN("olalarm",    INT_OLALARM,    cOnlineAlarm)
        SETINTWIN("errcodes",   INT_ERRCODES,   cErrcodesWidget)
        SETINTWIN("hsop",       INT_HSOP,       cHSOperate)
        SETINTWIN("findmac",    INT_FINDMAC,    cFindByMac)
        SETINTWIN(_sWorkstation,INT_WORKSTATION,cWorkstation)
        SETINTWIN("enumedit",   INT_ENUMEDIT,   cEnumValsEdit)
        SETINTWIN("deducepatch",INT_DEDUCEPATCH,cDeducePatch)
        SETINTWIN("snmpdquery", INT_SNMPDQUERY, cSnmpDevQuery)
        SETINTWIN("translator", INT_TRANSLATOR, cTranslator)
        SETINTWIN("changeswitch",INT_CHANGESWITCH,cChangeSwitch)
#undef SETINTWIN
#undef _SETINTWIN
        else {
            msg = tr("A %1 nevű menüben ismeretlen %2 nevű form.")
                   .arg(pmi->getName(), param);
            msg += tr("A menüpont le lessz tiltva.");
            cMsgBox::warning(msg, par);
            pa->setDisabled(true);
            break;
        }
        setType(MAT_OWN);
        if (lanView::isAuthorized(rights)) {
            connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
        }
        else {
            pa->setDisabled(true);
        }
        break;
    case MAT_EXEC:
        setObjectName(param); // Nincs tab, a név a parancs lessz
        setType(MAT_EXEC);
        if (lanView::isAuthorized(pmi->getId(_sMenuRights))) {           // Jog?
            connect(pa, SIGNAL(triggered()), this, SLOT(executeIt()));
        }
        else {
            pa->setDisabled(true);
        }
        break;
    default:
        msg = tr("A %1 nevű menü típusa '%2' ismeretlen.")
               .arg(pmi->getName(), pmi->getName(cMenuItem::ixMenuItemType()));
        msg += tr("A menüpont le lessz tiltva.");
        cMsgBox::warning(msg, par);
        pa->setDisabled(true);
        break;
    }
}

cMenuAction::cMenuAction(const QString&  ps, QAction *pa, QMdiArea * par)
    : QObject(par), type(MAT_EXEC)
{
    pMdiArea     = par;
    pIntSubObj   = nullptr;
    intType      = INT_UNKNOWN;
    pDialog      = nullptr;
    pAction      = pa;
    pMenuItem    = nullptr;

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
    pDialog      = nullptr;
    pAction      = pa;
    pMenuItem    = nullptr;

    connect(pAction, SIGNAL(triggered()), this, SLOT(displayIt()));
    actionsMap.insert(pa->objectName(), pa);
}

cMenuAction::~cMenuAction()
{
    ;
}

void cMenuAction::initRecordTable()
{
     pIntSubObj = new cTableSubWin(objectName(), pMdiArea, pMenuItem->features());
     connect(pIntSubObj,             SIGNAL(closeIt()),   this, SLOT(removeIt()));
     connect(pIntSubObj->pSubWindow, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

/** Egy megjelenítést végző cIntSubObj leszármazott objektum létrehozása

A menüben az "own=<típus>" stringgel lehet megadni a properties mezőben megjelenítő típusát.
A jelenleg implementállt lehetőségek:
| Típus név   | Konstans név    | Objektum név    | Funkció                      |
|-------------|-----------------|-----------------|------------------------------|
| setup       | INT_SETUP       | cSetupWidget    | Alapbeállítások megadása     |
| gsetup      | INT_GSETUP      | cGSetupWidget   | Megjelenítési beállítások    |
| parser      | INT_PARSER      | cParseWidget    | A parser hvása               |
| export      | INT_EXPORT      | cExportsWidget  | Táblák (objektumok) exportja |
| olalarm     | INT_OLALARM     | cOnlineAlarm    | OnLine riasztások            |
| errcodes    | INT_ERRCODES    | cErrcodesWidget | API hibakódok listája        |
| hsop        | INT_HSOP        | cHSOperate      | host-services állapot man.   |
| findmac     | INT_FINDMAC     | cFindByMac      | MAC keresés                  |
| workstation | INT_WORKSTATION | cWorkstation    | Munkaállomás új/modosít      |
| deducepatch | INT_DEDUCEPATCH | cDeducePatch    | Fali kábel fefedező          |
| snmpdquery  | INT_SNMPDQUERY  | cSnmpDevQuery   | SNMP insert/refresh          |
| enumedit    | INT_ENUMEDIT    | cEnumValsEdit   | Enumerációk                  |
| translator  | INT_TRANSLATOR  | cTranslator     | Nyelvi szövegek, fordítás    |
| changeswitch| INT_CHANGESWITCH| cChangeSwitch   | Switch csere, linkek másolása|
 */
void cMenuAction::initInt()
{
    switch (intType) {
#define CREATEINTWIN(D,C)    case D: pIntSubObj = new C(pMdiArea); break
    CREATEINTWIN(INT_SETUP,      cSetupWidget);
    CREATEINTWIN(INT_GSETUP,     cGSetupWidget);
    CREATEINTWIN(INT_PARSER,     cParseWidget);
    CREATEINTWIN(INT_EXPORT,     cExportsWidget);
    CREATEINTWIN(INT_OLALARM,    cOnlineAlarm);
    CREATEINTWIN(INT_ERRCODES,   cErrcodesWidget);
    CREATEINTWIN(INT_HSOP,       cHSOperate);
    CREATEINTWIN(INT_FINDMAC,    cFindByMac);
    CREATEINTWIN(INT_WORKSTATION,cWorkstation);
    CREATEINTWIN(INT_DEDUCEPATCH,cDeducePatch);
    CREATEINTWIN(INT_SNMPDQUERY, cSnmpDevQuery);
    CREATEINTWIN(INT_ENUMEDIT,   cEnumValsEdit);
    CREATEINTWIN(INT_TRANSLATOR, cTranslator);
    CREATEINTWIN(INT_CHANGESWITCH, cChangeSwitch);
#undef CREATEINTWIN
    default:
        EXCEPTION(EPROGFAIL);
    }
    connect(pIntSubObj,      SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pIntSubObj->pSubWindow,      SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}

void cMenuAction::displayIt()
{
    if (pMdiArea == nullptr) {
        EXCEPTION(EPROGFAIL, -1, tr("pMdiArea is NULL"));
    }
    cError *pe = nullptr;
    try {
        if (pIntSubObj == nullptr) {
            cnt = 1;
            switch (type) {
            case MAT_SHAPE:     initRecordTable();                          break;
            case MAT_OWN:       initInt();                                  break;
            case MAT_WIDGET:    EXCEPTION(ENOTSUPP, type);                  break;
            default:            EXCEPTION(EPROGFAIL,-1,"Invalid signal.");  break;
            }
            pIntSubObj->setWindowTitle(pMenuItem->getText(cMenuItem::LTX_TAB_TITLE, pMenuItem->getName()));
        }
        else {
            // Tábla, több példányos
            if (MAT_SHAPE == type && pMenuItem->isFeature("multi")) {
                cIntSubObj *p = new cTableSubWin(objectName(), pMdiArea, pMenuItem->features());
                QString t = pMenuItem->getText(cMenuItem::LTX_TAB_TITLE, pMenuItem->getName());
                p->setWindowTitle(t + QString(" (%1)").arg(++cnt));
                connect(p, SIGNAL(closeIt()), this, SLOT(deleteLater()));
                pMdiArea->setActiveSubWindow(p->pSubWindow);
                p->pWidget()->show();
                return;
            }
        }
        pMdiArea->setActiveSubWindow(pIntSubObj->pSubWindow);
        pIntSubObj->pWidget()->show();
        return;
    } CATCHS(pe);
    QString msg = tr("Program, vagy adat hiba miatt a kért funkció nem elérhető.");
    cErrorMessageBox::messageBox(pe, pMdiArea, msg);
    delete pe;
    pIntSubObj = nullptr;
}

void cMenuAction::removeIt()
{
    if (pIntSubObj == nullptr) {
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
        if (pDialog == nullptr) EXCEPTION(EPROGFAIL, -1, "pDialog is NULL.");
        QString name = pDialog->objectName();
        PDEB(VERBOSE) << "Start " << name << " dialog..." << endl;
        int r = pDialog->exec();
        PDEB(INFO) << "Return " << name << " dialog :" << r << endl;
    }
        break;
    case MAT_EXEC: {
        QString name = objectName();
        if      (0 == name.compare(_sExit,    Qt::CaseInsensitive)) qApp->exit(0);
        else if (0 == name.compare("restart", Qt::CaseInsensitive)) appReStart();
        else if (0 == name.compare("tabs",    Qt::CaseInsensitive)) pMdiArea->setViewMode(QMdiArea::TabbedView);
        else if (0 == name.compare("windows", Qt::CaseInsensitive)) pMdiArea->setViewMode(QMdiArea::SubWindowView);
        else if (0 == name.compare("close",   Qt::CaseInsensitive)) pMdiArea->closeAllSubWindows();
        else if (0 == name.compare("zone",    Qt::CaseInsensitive)) lv2g::getInstance()->changeZone();
        else    EXCEPTION(EDBDATA,-1, name);
    }
        break;
    default:
        DERR() << "Invalid type : #" << int(type) << endl;
        break;
    }
}

cTableSubWin::cTableSubWin(const QString& shape, QMdiArea * pMdiArea, const cFeatures &_f)
    : cIntSubObj(pMdiArea)
{
    pTableShape = nullptr;
    pRecordsView = nullptr;
    pTableShape = new cTableShape();
    pTableShape->setParent(this);
    pTableShape->setByName(shape);
    pTableShape->features().merge(_f, _sShape);
    pTableShape->modifyByFeature(_sTableShapeType);
    pTableShape->modifyByFeature(_sRefine);
    pTableShape->modifyByFeature(_sStyleSheet);
    pTableShape->modifyByFeature(_sAutoRefresh);
    pRecordsView = cRecordsViewBase::newRecordView(pTableShape, nullptr, this);
    QHBoxLayout *pHBL = new QHBoxLayout;
    pWidget()->setLayout(pHBL);
    pHBL->addWidget(pRecordsView->pWidget());
    connect(pRecordsView, SIGNAL(closeIt()),   this, SLOT(removeIt()));
    connect(pRecordsView, SIGNAL(destroyed()), this, SLOT(destroyedChild()));
}
