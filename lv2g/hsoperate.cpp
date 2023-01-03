#include "hsoperate.h"
#include "srvdata.h"
#include "record_dialog.h"
#include "record_tree.h"
#include "lv2user.h"
#include "popupreport.h"

#include "input_dialog.h"


const enum ePrivilegeLevel cHSOperate::rights = PL_VIEWER;

enum ePermit {
    PERMIT_NO, PERMIT_PART, PERMIT_ALL
};

/// Field indexs in SQL query
enum eFieldIx {
    RX_ID,
    RX_HOST_NAME, RX_HOST_STAT, RX_SERVICE_NAME, RX_PORT_ID, RX_PORT_NAME, RX_SRV_EXT,
    RX_PLACE_NAME, RX_PLACE_TYPE, RX_NOALARM, RX_FROM, RX_TO,
    RX_DISABLED, RX_SRV_DISABLED, RX_STATE, RX_SOFT_STATE, RX_DELEGATE,
    RX_NSUB, RX_SUPERIOR_ID, RX_SUPERIOR_NAME,
    RX_LAST_TOUCHED
};

const QString cHSOperate::_sql =
        "SELECT"
            " hs.host_service_id, "         // RX_ID
            " node_name, "                  // RX_HOST_NAME
            " node_stat, "                  // RX_NODE_STAT
            " service_name, "               // RX_SERVICE_NAME
            " hs.port_id,"                  // RX_PORT_ID
            " CASE WHEN hs.port_id IS NULL THEN NULL"
                 " WHEN np.node_id = hs.node_id THEN np.port_name"
                 " ELSE port_id2full_name(hs.port_id)"
                 " END, "                   // RX_PORT_NAME
            " CASE WHEN proto_service_id < 0 AND prime_service_id < 0 THEN NULL"
                 " WHEN prime_service_id < 0 THEN service_id2name(proto_service_id) || ':'"
                 " WHEN proto_service_id < 0 THEN ':' || service_id2name(prime_service_id)"
                 " ELSE service_id2name(proto_service_id) || ':' || service_id2name(prime_service_id)"
                 " END, "                   // RX_SRV_EXT
            " p.place_name, "               // RX_PLACE_NAME
            " p.place_type,"                // RX_PLACE_TYPE
            " hs.noalarm_flag, "            // RX_NOALARM
            " hs.noalarm_from, "            // RX_FROM
            " hs.noalarm_to, "              // RX_TO
            " hs.disabled, "                // RX_DISABLED
            " s.disabled AS sd,"            // RX_SRV_DISABLED
            " hs.host_service_state, "      // RX_STATE
            " hs.soft_state, "              // RX_SOFT_STATE
            " hs.delegate_host_state, "     // RX_DELEGATE
            " (SELECT COUNT(*) FROM host_services AS shs WHERE shs.superior_host_service_id = hs.host_service_id), "    // RX_NSUB
            " hs.superior_host_service_id, "// RX_SUPERIOR_ID
            " CASE WHEN hs.superior_host_service_id IS NULL THEN NULL"
                 " ELSE host_service_id2name(hs.superior_host_service_id)"
                 " END"
                 " AS superior_host_service_name,"  // RX_SUPERIOR_NAME
            "hs.last_touched"               // RX_LAST_TOUCHED
        " FROM host_services AS hs"
        " LEFT JOIN nports AS np ON np.port_id = hs.port_id"
        " JOIN nodes  AS n ON n.node_id = hs.node_id"
        " JOIN places AS p USING(place_id)"
        " JOIN services AS s USING(service_id)";
const QString cHSOperate::_ord = " ORDER BY node_name, service_name, hs.port_id ASC";

const cColEnumType    *cHSORow::pPlaceType   = nullptr;
const cColEnumType    *cHSORow::pNoAlarmType = nullptr;
const cColEnumType    *cHSORow::pNotifSwitch = nullptr;

cHSORow::cHSORow(QSqlQuery& q, cHSOState *par, int _row)
    : QObject(par), nsub(0), rec(q.record()), row(_row)
{
    pDialog = par->pDialog;
    pCheckBoxSub = nullptr;
    pCheckBoxSet = nullptr;
    pToolButtonCmd = nullptr;
    pComboBoxCmd = nullptr;
    pService = nullptr;
    pq = par->pq;
    staticInit();
    set = false; // pDialog->permit == PERMIT_ALL;
    sub = false;
    bool ok;
    id  = rec.value(RX_ID).toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA, RX_ID, rec.value(RX_ID).toString());
    QVariant v = rec.value(RX_NSUB);
    if (v.isValid()) {
        nsub  = v.toInt(&ok);
        if (!ok) EXCEPTION(EDATA, RX_NSUB, v.toString());
    }
    QSqlQuery qq = getQuery();
    pService = cService::service(qq, rec.value(RX_SERVICE_NAME).toString());
    _serviceIsApp = serviceIsApp(rec.value(RX_SUPERIOR_ID), pService);
}

void cHSORow::staticInit()
{
    if (pPlaceType == nullptr) {
        QSqlQuery q = getQuery();
        pNoAlarmType = cColEnumType::fetchOrGet(q, "noalarmtype");
        pNotifSwitch = cColEnumType::fetchOrGet(q, _sNotifswitch);
        pPlaceType   = cColEnumType::fetchOrGet(q, "placetype");
    }
}

QCheckBox * cHSORow::getCheckBoxSet()
{
    pCheckBoxSet = new QCheckBox;
    pCheckBoxSet->setChecked(set);
    connect(pCheckBoxSet, SIGNAL(toggled(bool)), this, SLOT(togleSet(bool)));
    return pCheckBoxSet;
}

QWidget * cHSORow::getWidgetSub()
{
    if (nsub == 0) return nullptr;
    QWidget *pWidget = new QWidget;
    QHBoxLayout *pLayout = new QHBoxLayout;
    pLayout->setMargin(0);
    pWidget->setLayout(pLayout);
    pCheckBoxSub = new QCheckBox;
    pCheckBoxSub->setChecked(sub);
    pCheckBoxSub->setEnabled(true);
    pLayout->addWidget(pCheckBoxSub);
    QToolButton *pButton = new QToolButton;
    pButton->setIcon(QIcon("://icons/down.ico"));
    pButton->setEnabled(true);
    pLayout->addWidget(pButton);
    connect(pCheckBoxSub, SIGNAL(toggled(bool)), this, SLOT(togleSub(bool)));
    connect(pButton,      SIGNAL(clicked()),     this, SLOT(goSub()));
    return pWidget;
}

QWidget* cHSORow::getButtonCmd()
{
    QStringList cmds = pService->features().slValue("control");
    if (cmds.isEmpty()) return nullptr;
    QWidget *pWidget = new QWidget;
    QHBoxLayout *pLayout = new QHBoxLayout;
    pLayout->setMargin(0);
    pComboBoxCmd = new QComboBox;
    pToolButtonCmd = new QToolButton();
    pComboBoxCmd->addItems(cmds);
    pComboBoxCmd->setCurrentIndex(0);
    pToolButtonCmd->setIcon(QIcon("://icons/control.png"));
    pLayout->setMargin(0);
    pLayout->addWidget(pComboBoxCmd);
    pLayout->addWidget(pToolButtonCmd);
    pWidget->setLayout(pLayout);
    connect(pToolButtonCmd, SIGNAL(clicked()), this, SLOT(pressCmd()));
    return pWidget;
}


QTableWidgetItem * cHSORow::item(int vix)
{
    QVariant v = rec.value(vix);
    if (v.isNull()) return nullptr;
    switch (v.userType()) {
    case QMetaType::QDateTime: {
        v = dateTimeFromSql(v);
    }   break;
    default:
        break;
    }
    return new QTableWidgetItem(v.toString());
}

QTableWidgetItem * cHSORow::item(int ix, const cColEnumType *pType)
{
    QString s = rec.value(ix).toString();
    const cEnumVal& ev = cEnumVal::enumVal(*pType, pType->str2enum(s), EX_IGNORE);
    QString sIcon = ev.getName(_sIcon);
    QTableWidgetItem *pi;
    if (sIcon.isEmpty()) {
        pi = new QTableWidgetItem(s);
        pi->setBackground(bgColorByEnum(*pType, pType->str2enum(s)));
    }
    else {
        pi = new QTableWidgetItem(resourceIcon(sIcon), _sNul);
    }
    return pi;
}

QTableWidgetItem * cHSORow::item(int vix, int eix, const cColEnumType * pType)
{
    QString s = rec.value(vix).toString();
    QTableWidgetItem *pi = new QTableWidgetItem(s);
    s = rec.value(eix).toString();
    pi->setBackground(bgColorByEnum(*pType, pType->str2enum(s)));
    return pi;
}
QTableWidgetItem * cHSORow::item(int ix, const cColStaticDescr &cd)
{
    QVariant v = rec.value(ix);
    if (v.isNull()) return nullptr;
    v = cd.fromSql(v);
    QString s = cd.toView(*pq, v);
    return new QTableWidgetItem(s);
}

QTableWidgetItem * cHSORow::boolItem(int ix, const QString& tn, const QString& fn)
{
    bool     b = rec.value(ix).toBool();
    const cEnumVal& ev = cEnumVal::enumVal(mCat(tn, fn), bool2boolVal(b), EX_IGNORE);
    QString sIcon = ev.getName(_sIcon);
    QTableWidgetItem *pi;
    if (sIcon.isEmpty()) {
        QString  s = langBool(b);
        pi = new QTableWidgetItem(s);
        pi->setBackground(bgColorByBool(tn, fn, b));
    }
    else {
        pi = new QTableWidgetItem(resourceIcon(sIcon), _sNul);
    }
    return pi;

}

bool cHSORow::serviceIsApp(const QVariant& suphsid, const cService *ps)
{
    return suphsid.isNull() ||
        (false == ps->isEmpty(ps->toIndex(_sCheckCmd)) &&
         ps->feature(_sMethod).contains(_sInspector, Qt::CaseInsensitive));
}

QString cHSORow::findAppName(QSqlQuery& q, qlonglong hsid)
{
    static const QString sql =
            "SELECT shs.service_id, hs.superior_host_service_id, shs.superior_host_service_id"
            " FROM host_services AS hs"
            " JOIN host_services AS shs ON hs.superior_host_service_id = shs.host_service_id"
            " WHERE hs.host_service_id = ?";
    if (execSql(q, sql, hsid)) {
        qlonglong sid = q.value(0).toLongLong();
        QVariant vshsid = q.value(1);
        QVariant vsshsid = q.value(2);
        const cService *ps = cService::service(q, sid);
        if (serviceIsApp(vsshsid, ps)) return ps->getName();
        return findAppName(q, vshsid.toLongLong());
    }
    return _sNul;
}

void cHSORow::togleSet(bool f)
{
    set = f;
    if (!pDialog->lockSetButton) {
        pDialog->setButtonsFromTo();
    }
}

void cHSORow::goSub() {
    pDialog->goSub(row);
}

void cHSORow::pressCmd()
{
    QString srvName;
    if (_serviceIsApp) {
        srvName = rec.value(RX_SERVICE_NAME).toString();
    }
    else {
        QSqlQuery q = getQuery();
        srvName = findAppName(q, id);
        if (srvName.isEmpty()) {
            QString msg = tr("A szolgáltatáspéldányt futtató applikáció neve nem megállapítható.");
            QMessageBox::warning(nullptr, dcViewLong(DC_WARNING), msg);
            return;
        }
    }
    QString sPayload = pComboBoxCmd->currentText();
    sPayload += _sSpace + QString::number(id);
    if (!sqlNotify(*pq, srvName, sPayload)) {
        QString msg = tr("A NOTIFY SQL parancs kiadása sikertelen (%1, %2).").arg(srvName, sPayload);
        QMessageBox::warning(nullptr, dcViewLong(DC_WARNING), msg);
    }
    else {
        QString msg = tr("SQL NOTIFY O.K. (%1, %2).").arg(srvName, sPayload);
        QMessageBox::information(nullptr, dcViewLong(DC_INFO), msg);
    }
}

cHSOState::cHSOState(QSqlQuery& q, const QString& _sql, const QVariantList _binds, cHSOperate *par)
    : QObject(par), sql(_sql), binds(_binds)
{
    pDialog = par;
    size = nsup = 0;
    pq = par->pq2;
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    int i = 0;
    foreach (QVariant v, binds) {
        q.bindValue(i, v);
        ++i;
    }
    if (!q.exec()) SQLQUERYERR(q);
    size = 0;
    if (q.first()) do {
        cHSORow *p = new cHSORow(q, this, size);
        if (p->nsub > 0) ++nsup;
        rows << p;
        size++;
    } while(q.next());
}

QStringList cHSOState::getSupIds()
{
    QStringList r;
    foreach (cHSORow *pRow, rows) {
        if (pRow->sub) r << QString::number(pRow->id);
    }
    return r;
}

cHSORow * cHSOState::rowAtId(qlonglong id)
{
    foreach (cHSORow *p, rows) {
        if (p->id == id) return p;
    }
    return nullptr;
}

enum eFilterButtonId {
    FT_PATTERN, FT_SELECT
};

enum eTableColumnIx {
    TC_ID,
    TC_HOST,
    TC_HOST_STATE,
    TC_SERVICE,
    TC_PORT,
    TC_EXT,         /// prot:prim
    TC_PLACE,
    TC_NOALARM,
    TC_FROM,
    TC_TO,
    TC_DISABLED,
    TC_DISABLED_SRV,
    TC_STATE,
    TC_SOFT_STATE,
    TC_DELEGATE,
    TC_LAST_TM,
    TC_CBOX_SELECT,
    TC_NSUB,
    TC_CBOX_NSUB,
    TC_SUPERIOR,
    TC_SEND_CMD,
    TC_COUNT
};

/* *************************************************************************************** */

static QString _sMinInterval = "DisableAlarmMinInterval";

cHSOperate::cHSOperate(QMdiArea *par)
    : cIntSubObj(par)
{
    pUi = nullptr;
    pButtonGroupPlace = nullptr;
    pButtonGroupService = nullptr;
    pButtonGroupNode = nullptr;
    pButtonGroupSup = nullptr;
    pButtonGroupAlarm = nullptr;
    pZoneModel = nullptr;
    pPlaceModel = nullptr;
    pNodeModel = nullptr;
    pServiceModel = nullptr;
    pSupModel = nullptr;
    pq = pq2  = nullptr;

    privilegLevel = lanView::getInstance()->user().privilegeLevel();
    if (privilegLevel == PL_NONE) {
        noRightsSetup(this, PL_VIEWER, "");
        return;
    }

    pq  = newQuery();
    pSrvLogsTable = pSrvVarsTable = nullptr;
    minIntervalMs = cSysParam::getIntervalSysParam(*pq, _sMinInterval, 1800000LL);  // default 1800sec (30m)
    pq2 = newQuery();
    pUi = new Ui::hostServiceOp;
    stateIx = -1;
    refreshTime = 0;
    timerId = -1;
    lockSetButton = false;
    lastAlramButtonId = -1;
    sStart = tr("Start");
    sStop  = tr("Stop");

    pUi->setupUi(this);
    if (TC_COUNT != pUi->tableWidget->columnCount()) EXCEPTION(EDATA);
    // Jogosultsági szintek:
    switch (privilegLevel) {
    case PL_NONE:
        EXCEPTION(EPROGFAIL);
        break;
    case PL_VIEWER:
    case PL_INDALARM:
        permit = PERMIT_NO;     // Csak nézelődhet
        pUi->textEditJustify->setDisabled(true);
        pUi->splitter->widget(1)->hide();   // Nem kell, nincs mit indokolnia
        break;
    case PL_OPERATOR:
        permit = PERMIT_PART;   // Csak riasztás tiltás, kötelező indoklással
        pUi->splitter->setStretchFactor(0, 3);
        pUi->splitter->setStretchFactor(1, 1);
        break;
    case PL_ADMIN:
    case PL_SYSTEM:
        permit = PERMIT_ALL;    // Mindent szabad
        pUi->splitter->setStretchFactor(0, 1);
        pUi->splitter->setStretchFactor(1, 0);
        break;
    }

    pButtonGroupPlace   = new QButtonGroup(this);
    pButtonGroupPlace->addButton(pUi->radioButtonPlacePattern, FT_PATTERN);
    pButtonGroupPlace->addButton(pUi->radioButtonPlaceSelect,  FT_SELECT);
    pUi->radioButtonPlacePattern->setChecked(true);

    pButtonGroupService = new QButtonGroup(this);
    pButtonGroupService->addButton(pUi->radioButtonServicePattern, FT_PATTERN);
    pButtonGroupService->addButton(pUi->radioButtonServiceSelect,  FT_SELECT);
    pUi->radioButtonServicePattern->setChecked(true);

    pButtonGroupNode    = new QButtonGroup(this);
    pButtonGroupNode->addButton(pUi->radioButtonNodePattern, FT_PATTERN);
    pButtonGroupNode->addButton(pUi->radioButtonNodeSelect,  FT_SELECT);
    pUi->radioButtonNodePattern->setChecked(true);

    pButtonGroupSup   = new QButtonGroup(this);
    pButtonGroupSup->addButton(pUi->radioButtonSupPattern, FT_PATTERN);
    pButtonGroupSup->addButton(pUi->radioButtonSupSelect,  FT_SELECT);
    pUi->radioButtonSupPattern->setChecked(true);

    pButtonGroupAlarm   = new QButtonGroup(this);
    pButtonGroupAlarm->addButton(pUi->checkBoxAlarmOn,     NAT_OFF);
    pButtonGroupAlarm->addButton(pUi->checkBoxAlarmOff,    NAT_ON);
    pButtonGroupAlarm->addButton(pUi->checkBoxSetTo,       NAT_TO);
    pButtonGroupAlarm->addButton(pUi->checkBoxSetFrom,     NAT_FROM);
    pButtonGroupAlarm->addButton(pUi->checkBoxSetInterval, NAT_FROM_TO);

    pUi->checkBoxDisable->setCheckState(Qt::PartiallyChecked);
    pUi->checkBoxDelegate->setCheckState(Qt::PartiallyChecked);
    pUi->checkBoxFiltDelegate->setCheckState(Qt::PartiallyChecked);

    pUi->pushButtonAutoRefresh->setText(sStart);
    if (permit != PERMIT_ALL) {
        pUi->tableWidget->hideColumn(TC_SEND_CMD);
    }

    if (permit == PERMIT_NO) {
        pUi->tableWidget->hideColumn(TC_CBOX_SELECT);
        pUi->checkBoxAlarmOn->setDisabled(true);
        pUi->checkBoxAlarmOff->setDisabled(true);
        pUi->checkBoxSetTo->setDisabled(true);
        pUi->checkBoxSetFrom->setDisabled(true);
        pUi->checkBoxSetInterval->setDisabled(true);
        pUi->checkBoxRemove->setDisabled(true);
        pUi->pushButtonSet->setDisabled(true);
        pUi->pushButtonAll->setDisabled(true);
        pUi->pushButtonNone->setDisabled(true);
        pUi->checkBoxDisable->setDisabled(true);
        pUi->checkBoxDelegate->setDisabled(true);
        pUi->checkBoxClrStat->setDisabled(true);
        pUi->checkBoxDelAlarm->setDisabled(true);
        pUi->checkBoxDelStatLog->setDisabled(true);
        pUi->checkBoxDelMemo->setDisabled(true);
    }
    else {
        connect(pButtonGroupAlarm,      SIGNAL(buttonClicked(int)), this, SLOT(setAlarmButtons(int)));
        if (permit == PERMIT_PART) {
            pUi->checkBoxDisable->setDisabled(true);
            pUi->checkBoxDelegate->setDisabled(true);
            pUi->checkBoxClrStat->setDisabled(true);
            pUi->checkBoxDelAlarm->setDisabled(true);
            pUi->checkBoxDelStatLog->setDisabled(true);
            pUi->checkBoxDelMemo->setDisabled(true);
            pUi->checkBoxRemove->setDisabled(true);
        }
        else {
            pUi->checkBoxRemove->setEnabled(true);
        }
        pUi->dateTimeEditFrom->setDisplayFormat(lanView::sDateTimeForm);
        pUi->dateTimeEditTo->setDisplayFormat(lanView::sDateTimeForm);
        now = QDateTime::currentDateTime();
        QDateTime dt = now.addMSecs(minIntervalMs);
        pUi->dateTimeEditFrom->setMinimumDateTime(now);
        pUi->dateTimeEditTo->setMinimumDateTime(dt);
        pUi->dateTimeEditFrom->setDateTime(now);
        pUi->dateTimeEditTo->setDateTime(dt);

        pUi->checkBoxFiltHsDisa->setCheckState(Qt::PartiallyChecked);
    }

    pZoneModel = new cZoneListModel(this);
    pUi->comboBoxZone->setModel(pZoneModel);
    pZoneModel->setFilter();
    pUi->comboBoxZone->setCurrentText(_sAll);

    pPlaceModel = new cPlacesInZoneModel();
    pUi->comboBoxPlaceSelect->setModel(pPlaceModel);
    pPlaceModel->setFilter();   // Nincs szűrés, rendezés növekvő sorrendben
    pUi->comboBoxPlaceSelect->setCurrentIndex(0);

    pNodeModel = new cRecordListModel(cNode().descr(), this);
    pUi->comboBoxNodeSelect->setModel(pNodeModel);
    pNodeModel->setFilter();
    pUi->comboBoxNodeSelect->setCurrentIndex(0);

    pServiceModel = new cRecordListModel(cService().descr(), this);
    pUi->comboBoxServiceSelect->setModel(pServiceModel);
    pServiceModel->setFilter();
    pUi->comboBoxServiceSelect->setCurrentIndex(0);

    pSupModel = new cRecordListModel(cHostService().descr(), this);
    // Kell egy tábla alias a következő szűréshez
    pSupModel->sTableAlias = "hs";
    // Csak azokat listázza, amelyekre mutat tábla mint superior példány
    pSupModel->setConstFilter("0 < (SELECT COUNT(*) FROM host_services AS hss WHERE hss.superior_host_service_id = hs.host_service_id)", FT_SQL_WHERE);
    pUi->comboBoxSupSelect->setModel(pSupModel);
    pSupModel->setFilter();
    pUi->comboBoxSupSelect->setCurrentIndex(0);

    // Service variables table
    cTableShape *ptsv = new cTableShape;
    ptsv->fetchByName(*pq, _sServiceRrdVars);
    ptsv->setId(_sTableShapeType, ENUM2SET2(TS_BARE, TS_READ_ONLY));
    ptsv->setName(_sRefine, _sFalse);    // Empty table
    pSrvVarsTable = new cRecordTable(ptsv, pUi->widgetSrvVars);
    pSrvVarsTable->init();

    // Service logs table
    cTableShape *ptsl = new cTableShape;
    ptsl->fetchByName(*pq, "host_service_logs");
    ptsl->setId(_sTableShapeType, ENUM2SET2(TS_BARE, TS_READ_ONLY));
    ptsl->setName(_sRefine, _sFalse);    // Empty table
    pSrvLogsTable = new cRecordTable(ptsl, pUi->widgetSrvLogs);
    pSrvLogsTable->init();
}


cHSOperate::~cHSOperate()
{
    pDelete(pq);
    pDelete(pq2);
}

int cHSOperate::queryNodeServices(qlonglong _nid)
{
    pUi->checkBoxPlace->setChecked(false);
    pUi->checkBoxService->setChecked(false);
    pUi->checkBoxNode->setChecked(true);
    pUi->lineEditNodePattern->setText(_sNul);
    pNodeModel->setFilter(QVariant(), OT_DEFAULT, FT_NO);
    int ix = pNodeModel->indexOf(_nid);
    if (ix < 0) return false;
    pUi->comboBoxNodeSelect->setCurrentIndex(ix);
    pUi->radioButtonNodeSelect->setChecked(true);
    on_pushButtonFetch_clicked();
    return actState()->rows.size();
}

void cHSOperate::timerEvent(QTimerEvent *event)
{
    (void)event;
    static int cnt = 0;
    if (refreshTime < 10) {
        if (cnt) cnt = 0;
        return;
    }
    cnt++;
    if (cnt >= refreshTime) {
        cnt = 0;
        if (stateIx >= 0) on_pushButtonRefresh_clicked();
    }
    pUi->progressBarRefresh->setValue(cnt);
}

void cHSOperate::refreshTable()
{
    QVariant v;
    QString s;
    cHostService hs;

    int row = 0;
    pUi->tableWidget->setRowCount(0);
    pUi->tableWidget->setRowCount(actState()->size);
    foreach (cHSORow *pRow, actState()->rows) {
        setCell(row, TC_ID,         pRow->item(RX_ID));
        setCell(row, TC_HOST,       pRow->item(RX_HOST_NAME));
        setCell(row, TC_HOST_STATE, pRow->item(RX_HOST_STAT, cHSORow::pNotifSwitch));
        setCell(row, TC_SERVICE,    pRow->item(RX_SERVICE_NAME));
        setCell(row, TC_PORT,       pRow->item(RX_PORT_NAME));
        setCell(row, TC_EXT,        pRow->item(RX_SRV_EXT));
        setCell(row, TC_PLACE,      pRow->item(RX_PLACE_NAME, RX_PLACE_TYPE, cHSORow::pPlaceType));
        setCell(row, TC_NOALARM,    pRow->item(RX_NOALARM, cHSORow::pNoAlarmType));
        setCell(row, TC_FROM,       pRow->item(RX_FROM, hs.colDescr(hs.toIndex(_sNoalarmFrom))));
        setCell(row, TC_TO,         pRow->item(RX_TO, hs.colDescr(hs.toIndex(_sNoalarmTo))));
        setCell(row, TC_DISABLED,   pRow->boolItem(RX_DISABLED, _sHostServices, _sDisabled));
        setCell(row, TC_DISABLED_SRV,pRow->boolItem(RX_SRV_DISABLED, _sServices, _sDisabled));
        setCell(row, TC_STATE,      pRow->item(RX_STATE, cHSORow::pNotifSwitch));
        setCell(row, TC_SOFT_STATE, pRow->item(RX_SOFT_STATE, cHSORow::pNotifSwitch));
        setCell(row, TC_DELEGATE,   pRow->boolItem(RX_DELEGATE, _sHostServices, _sDelegateHostState));
        setCell(row, TC_LAST_TM,    pRow->item(RX_LAST_TOUCHED));
        setCell(row, TC_CBOX_SELECT,pRow->getCheckBoxSet());
        setCell(row, TC_NSUB,       pRow->item(RX_NSUB));
        setCell(row, TC_CBOX_NSUB,  pRow->getWidgetSub());
        setCell(row, TC_SUPERIOR,   pRow->item(RX_SUPERIOR_NAME));
        setCell(row, TC_SEND_CMD,   pRow->getButtonCmd());
        row++;
    }
    bool noSup = actState()->nsup == 0;
    pUi->tableWidget->resizeColumnsToContents();
    pUi->pushButtonAll->setDisabled(false);
    pUi->pushButtonNone->setDisabled(false);
    pUi->pushButtonSubAll->setDisabled(noSup);
    pUi->pushButtonSubNone->setDisabled(noSup);
    pUi->pushButtonSub->setDisabled(noSup);
    setButtonsFromTo();
    return;
}

bool cHSOperate::fetch(const QString& sql, const QVariantList& bind)
{
    cHSOState *pState = new cHSOState(*pq, sql, bind, this);
    if (pState->size == 0) {
        delete pState;
        return false;
    }
    while (states.size() > (stateIx +1)) delete states.takeLast();
    states << pState;
    stateIx++;
    refreshTable();
    pUi->toolButtonBack->setEnabled(stateIx > 0);
    pUi->toolButtonClearHist->setEnabled(stateIx > 0);
    pUi->toolButtonForward->setDisabled(true);
    pUi->pushButtonRefresh->setEnabled(true);
    return true;
}

void cHSOperate::goSub(int row)
{
    on_pushButtonSubNone_clicked();
    actState()->rows[row]->pCheckBoxSub->setChecked(true);
    on_pushButtonSub_clicked();
}

void cHSOperate::on_pushButtonRefresh_clicked()
{
    cHSOState *p = new cHSOState(*pq, actState()->sql, actState()->binds, this);
    cHSOState *pOld = actState();
    foreach (cHSORow *pRow, p->rows) {
        qlonglong id = pRow->id;
        cHSORow *pOldRow = pOld->rowAtId(id);
        if (pOldRow != nullptr) {
            pRow->set = pOldRow->set;
            pRow->sub = pOldRow->sub;
        }
    }
    states[stateIx] = p;
    delete pOld;
    refreshTable();
}

void cHSOperate::on_pushButtonFetch_clicked()
{
    static const QChar j('%');

    eTristate isHsDis = checkBoxState2tristate(pUi->checkBoxFiltHsDisa->checkState());
    eTristate isSDis  = checkBoxState2tristate(pUi->checkBoxFiltSDisa->checkState());
    eTristate isDeleg = checkBoxState2tristate(pUi->checkBoxFiltDelegate->checkState());

    bool isAOff    = pUi->checkBoxFiltAlarmOff->isChecked();
    bool isAOn     = pUi->checkBoxFiltAlarmOn->isChecked();
    bool isATo     = pUi->checkBoxFiltAlarmTo->isChecked();
    bool isAFrom   = pUi->checkBoxFiltAlarmFrom->isChecked();
    bool isAFromTo = pUi->checkBoxFiltAlarmFromTo->isChecked();

    bool isStateOn     = pUi->checkBoxFiltStateOn->isChecked();
    bool isRecovered   = pUi->checkBoxFiltStatRecovered->isChecked();
    bool isWarning     = pUi->checkBoxFiltStatWarning->isChecked();
    bool isCritical    = pUi->checkBoxFiltStatCritical->isChecked();
    bool isUnreachable = pUi->checkBoxFiltStatUnreachable->isChecked();
    bool isDown        = pUi->checkBoxFiltStatDown->isChecked();
    bool isFlapping    = pUi->checkBoxFiltStatFlapping->isChecked();
    bool isUnknown     = pUi->checkBoxFiltStatUnknown->isChecked();

    QString         sql = _sql;
    QStringList     where;
    QVariantList    bind;
    QStringList     wOr;

    where.clear();
    switch (isHsDis) {
    case TS_NULL:   break;
    case TS_TRUE:   where << " hs.disabled ";       break;
    case TS_FALSE:  where << " NOT hs.disabled ";   break;
    }

    switch (isSDis) {
    case TS_NULL:   break;
    case TS_TRUE:   where << " s.disabled ";        break;
    case TS_FALSE:  where << " NOT s.disabled ";    break;
    }

    switch (isDeleg) {
    case TS_NULL:   break;
    case TS_TRUE:   where << " hs.delegate_host_state ";        break;
    case TS_FALSE:  where << " NOT hs.delegate_host_state ";    break;
    }

    if ((isAOff || isAOn || isATo || isAFrom || isAFromTo) && !(isAOff && isAOn && isATo && isAFrom && isAFromTo)) {
        wOr.clear();
        if (isAOff)  wOr << " hs.noalarm_flag = 'on' ";     // NINCS tiltva
        if (isAOn)   wOr << " hs.noalarm_flag = 'off' ";    // VAN tiltva
        if (isATo)   wOr << " hs.noalarm_flag = 'to' ";
        if (isAFrom) wOr << " hs.noalarm_flag = 'from' ";
        if (isAFrom) wOr << " hs.noalarm_flag = 'from_to' ";
        if (!wOr.isEmpty()) {
            if (wOr.size() > 1) {
                where << "(" + wOr.join("OR") + ")";
            }
            else {
                where << wOr;
            }
        }
    }

    if (!(isStateOn && isRecovered && isWarning && isCritical && isUnreachable && isDown  && isFlapping && isUnknown)) {
        wOr.clear();
        if (isStateOn)      wOr << " hs.host_service_state = 'on' ";
        if (isRecovered)    wOr << " hs.host_service_state = 'recovered' ";
        if (isWarning)      wOr << " hs.host_service_state = 'warning' ";
        if (isCritical)     wOr << " hs.host_service_state = 'critical' ";
        if (isUnreachable)  wOr << " hs.host_service_state = 'unreachable' ";
        if (isDown)         wOr << " hs.host_service_state = 'down' ";
        if (isFlapping)     wOr << " hs.host_service_state = 'flapping'";
        if (isUnknown)      wOr << " hs.host_service_state = 'unknown' ";
        if (!wOr.isEmpty()) {
            if (wOr.size() > 1) {
                where << "(" + wOr.join("OR") + ")";
            }
            else {
                where << wOr;
            }
        }
    }

    // Filter by zone
    if (pUi->checkBoxZone->isChecked() && pUi->comboBoxZone->currentText() != _sAll) {
        int       ci  = pUi->comboBoxZone->currentIndex();
        qlonglong gid = pZoneModel->atId(ci);
        where << " is_place_in_zone(place_id, ?::bigint)";
        bind  << gid;
    }
    // Filter by place
    if (pUi->checkBoxPlace->isChecked()) {
        if (pUi->radioButtonPlacePattern->isChecked()) {
            QString pat = pUi->lineEditPlacePattern->text();
            if (!pat.isEmpty()) {
                where << " p.place_name LIKE ?";
                if (pat.indexOf(j) < 0) pat += j;
                bind  << pat;
            }
        }
        else {
            int i = pUi->comboBoxPlaceSelect->currentIndex();
            qlonglong id = pPlaceModel->atId(i);
            if (id == NULL_ID) return;
            else {
                where << " is_parent_place(?, n.place_id)";
                bind  << id;
            }
        }
    }
    // Filter by node
    if (pUi->checkBoxNode->isChecked()) {
        if (pUi->radioButtonNodePattern->isChecked()) {
            QString pat = pUi->lineEditNodePattern->text();
            if (!pat.isEmpty()) {
                where << " n.node_name LIKE ?";
                if (pat.indexOf(j) < 0) pat += j;
                bind  << pat;
            }
        }
        else {
            int i = pUi->comboBoxNodeSelect->currentIndex();
            qlonglong id = pNodeModel->atId(i);
            if (id == NULL_ID) return;
            else {
                where << " n.node_id = ?";
                bind  << id;
            }
        }
    }
    // Filter by service
    if (pUi->checkBoxService->isChecked()) {
        if (pUi->radioButtonServicePattern->isChecked()) {
            QString pat = pUi->lineEditServicePattern->text();
            if (!pat.isEmpty()) {
                where << " s.service_name LIKE ?";
                if (pat.indexOf(j) < 0) pat += j;
                bind  << pat;
            }
        }
        else {
            int i = pUi->comboBoxServiceSelect->currentIndex();
            qlonglong id = pServiceModel->atId(i);
            if (id == NULL_ID) return;
            else {
                where << " s.service_id = ?";
                bind  << id;
            }
        }
    }
    // Filter by sup.
    if (pUi->checkBoxSup->isChecked()) {
        if (pUi->radioButtonSupPattern->isChecked()) {
            if (pUi->toolButtonSupNull->isChecked()) {
                where << " superior_host_service_id IS NULL";
            }
            else {
                QString pat = pUi->lineEditSupPattern->text();
                if (!pat.isEmpty()) {
                    where << " superior_host_service_name LIKE ?";
                    if (pat.indexOf(j) < 0) pat += j;
                    bind  << pat;
                }
            }
        }
        else {
            int i = pUi->comboBoxSupSelect->currentIndex();
            qlonglong id = pSupModel->atId(i);
            if (id == NULL_ID) return;
            else {
                where << " hs.superior_host_service_id = ?";
                bind  << id;
            }
        }
    }

    if (!where.isEmpty()) sql += " WHERE" + where.join(" AND");
    sql += _ord;
    fetch(sql, bind);
}

void cHSOperate::on_pushButtonSub_clicked()
{
    QStringList ids = actState()->getSupIds();
    if (ids.isEmpty()) return;
    QString sql = _sql + " WHERE superior_host_service_id = ";
    if (ids.size() == 0) {
        sql += ids.first();
    }
    else {
        sql += "ANY ('{" + ids.join(QChar(',')) + "}')";
    }
    sql += _ord;
    fetch(sql);
}

#define MIN_JUSTIFY 8
void cHSOperate::on_pushButtonSet_clicked()
{
    if (permit <= PERMIT_NO) EXCEPTION(EPROGFAIL);
    if (pUi->checkBoxRemove->isChecked()) { // REMOVE
        removeSelected();
        return;
    }
    QString sJustify = pUi->textEditJustify->toPlainText();
    if (permit <= PERMIT_PART) {
        if (sJustify.size() < MIN_JUSTIFY) return;
    }
    static const QString tn = "HSOparateSet";
    pUi->pushButtonSet->setDisabled(true);
    int rows = pUi->tableWidget->rowCount();
    cHostService hs;
    QBitArray um_disabled = hs.mask(_sDisabled);
    QBitArray um_noalarm  = hs.mask(_sNoalarmFlag, _sNoalarmFrom, _sNoalarmTo);
    QBitArray um_delegate = hs.mask(_sDelegateHostState);
    QStringList csf;
    csf << _sHostServiceState << _sSoftState << _sHardState << _sStateMsg << _sCheckAttempts
        << _sLastChanged  << _sLastTouched << _sActAlarmLogId;
    QBitArray um_ClrState = hs.mask(csf);
    eTristate disable  = checkBoxState2tristate(pUi->checkBoxDisable->checkState());
    eTristate delegate = checkBoxState2tristate(pUi->checkBoxDelegate->checkState());
    bool clrStat = pUi->checkBoxClrStat->isChecked();
    bool statLog = pUi->checkBoxDelStatLog->isChecked();
    bool alarm   = pUi->checkBoxDelAlarm->isChecked();
    bool memo    = pUi->checkBoxDelMemo->isChecked();
    if (permit != PERMIT_ALL) {
        if (disable != TS_NULL || delegate != TS_NULL || clrStat || statLog || alarm || memo) EXCEPTION(EPROGFAIL);
    }
    cError *pe = nullptr;
    sqlBegin(*pq2, tn);
    try {
        for (int row = 0; row < rows; ++row) {
            cHSORow *pRow = actState()->rows.at(row);
            if (pRow->set) {
                QSqlRecord& rec = pRow->rec;
                hs.clear();
                QBitArray um(hs.cols());
                qlonglong id = pRow->id;
                if (disable != TS_NULL) {
                    bool v = disable == TS_TRUE;
                    hs.setBool(_sDisabled, v);
                    um |= um_disabled;
                    rec.setValue(RX_DISABLED, v);
                }
                if (delegate != TS_NULL) {
                    bool v = delegate == TS_TRUE;
                    hs.setBool(_sDelegateHostState, v);
                    um |= um_delegate;
                    rec.setValue(RX_DELEGATE, v);
                }
                if (lastAlramButtonId != -1) {
                    QString sNoAl = noalarmtype(lastAlramButtonId);
                    hs.setName(_sNoalarmFlag, sNoAl);
                    switch (lastAlramButtonId) {
                    case NAT_OFF:
                    case NAT_ON:
                        hs.clear(_sNoalarmFrom);
                        hs.clear(_sNoalarmTo);
                        rec.setValue(RX_FROM, QVariant());
                        rec.setValue(RX_TO,  QVariant());
                        break;
                    case NAT_TO:
                        hs.set(_sNoalarmFrom, pUi->dateTimeEditTo->dateTime());
                        hs.clear(_sNoalarmTo);
                        rec.setValue(RX_FROM, pUi->dateTimeEditTo->dateTime());
                        rec.setValue(RX_TO,  QVariant());
                        break;
                    case NAT_FROM:
                        hs.clear(_sNoalarmFrom);
                        hs.set(_sNoalarmTo, pUi->dateTimeEditTo->dateTime());
                        rec.setValue(RX_FROM, QVariant());
                        rec.setValue(RX_TO,  pUi->dateTimeEditTo->dateTime());
                        break;
                    case NAT_FROM_TO:
                        hs.set(_sNoalarmFrom, pUi->dateTimeEditTo->dateTime());
                        hs.set(_sNoalarmTo, pUi->dateTimeEditTo->dateTime());
                        rec.setValue(RX_FROM, pUi->dateTimeEditTo->dateTime());
                        rec.setValue(RX_TO,  pUi->dateTimeEditTo->dateTime());
                        break;
                    default:
                        EXCEPTION(EPROGFAIL);
                        break;
                    }
                    um |= um_noalarm;
                    rec.setValue(RX_NOALARM, sNoAl);
                }
                if (clrStat) {
                    hs.setId(_sHostServiceState, RS_UNKNOWN);
                    hs.setId(_sSoftState, RS_UNKNOWN);
                    hs.setId(_sHardState, RS_UNKNOWN);
                    hs.setId(_sCheckAttempts, 0);
                    um |= um_ClrState;
                    rec.setValue(RX_STATE, _sUnknown);
                    rec.setValue(RX_SOFT_STATE, _sUnknown);
                }
                if (um.count(true) > 0) {
                    hs.setId(id);
                    pe = hs.tryUpdate(*pq2, false, um);
                    if (pe != nullptr) break;
                }
                if (statLog) {
                    static const QString sql = "DELETE FROM host_service_logs WHERE host_service_id = ?";
                    execSql(*pq2, sql, id);
                }
                if (alarm) {
                    static const QString sql = "DELETE FROM alarms WHERE host_service_id = ?";
                    execSql(*pq2, sql, id);
                }
                if (memo) {
                    static const QString sql = "DELETE FROM app_memos WHERE host_service_id = ?";
                    execSql(*pq2, sql, id);
                }
            }
        }
    } CATCHS(pe)
    if (pe != nullptr) {
        sqlRollback(*pq2, tn);
        cErrorMessageBox::messageBox(pe, this);
        on_pushButtonRefresh_clicked();  // Elrontottuk, újra olvassuk
    }
    sqlCommit(*pq2, tn);
    refreshTable();
}

void cHSOperate::setButtonsFromTo()
{
    if (permit == PERMIT_NO) {
        pUi->pushButtonSet->setEnabled(false);
        return;
    }
    bool bFrom = false, bTo = false;
    switch (lastAlramButtonId) {
    case NAT_FROM:
        bFrom = true;
        break;
    case NAT_TO:
        bTo = true;
        break;
    case NAT_FROM_TO:
        bFrom = bTo = true;
        break;
    default:
        break;
    }
    pUi->dateTimeEditFrom->  setEnabled(bFrom);
    pUi->dateTimeEditTo->    setEnabled(bTo);
    pUi->toolButtonRstFrom-> setEnabled(bFrom);
    pUi->toolButtonRstTo->   setEnabled(bTo);
    pUi->toolButtonDateFrom->setEnabled(bFrom);
    pUi->toolButtonDateTo->  setEnabled(bTo);
    pUi->toolButtonIntervalDef->setEnabled(bTo && bFrom);

    if (states.isEmpty()) {
        pUi->pushButtonSet->setEnabled(false);
        return;
    }
    bool f = false;
    if (permit == PERMIT_ALL) {
        f = pUi->checkBoxDisable ->checkState() != Qt::PartiallyChecked
         || pUi->checkBoxDelegate->checkState() != Qt::PartiallyChecked
         || pUi->checkBoxClrStat->isChecked()
         || pUi->checkBoxRemove->isChecked();
    }
    else {
        QString sJustify = pUi->textEditJustify->toPlainText();
        if (sJustify.size() < MIN_JUSTIFY) {
            pUi->pushButtonSet->setEnabled(false);
            return;
        }
    }
    f = f || lastAlramButtonId != -1;
    if (f) {
        f = false;
        cHSOState *pStat = actState();
        foreach (cHSORow *pRow, pStat->rows) {
            if (pRow->pCheckBoxSet->isChecked()) {
                f = true;
                break;
            }
        }
    }
    pUi->pushButtonSet->setEnabled(f);
}

cHSOState * cHSOperate::actState(eEx __ex)
{
    if (__ex == EX_IGNORE && stateIx < 0) return nullptr;
    if (!isContIx(states, stateIx)) EXCEPTION(ENOINDEX, stateIx);
    return states.at(stateIx);
}

void cHSOperate::on_pushButtonAll_clicked()
{
    if (permit <= PERMIT_NO) EXCEPTION(EPROGFAIL);
    cHSOState *pStat = actState(EX_IGNORE);
    if (pStat == nullptr) return;
    lockSetButton = true;
    foreach (cHSORow *pRow, pStat->rows) {
        QCheckBox *pCheckBox = pRow->pCheckBoxSet;
        if (pCheckBox != nullptr) {
            pCheckBox->setChecked(true);
        }
    }
    lockSetButton = false;
    setButtonsFromTo();
}

void cHSOperate::on_pushButtonNone_clicked()
{
    cHSOState *pStat = actState(EX_IGNORE);
    if (pStat == nullptr) return;
    lockSetButton = true;
    foreach (cHSORow *pRow, pStat->rows) {
        QCheckBox *pCheckBox = pRow->pCheckBoxSet;
        if (pCheckBox != nullptr) {
            pCheckBox->setChecked(false);
        }
    }
    lockSetButton = false;
    setButtonsFromTo();
}

void cHSOperate::on_pushButtonSubAll_clicked()
{
    cHSOState *pStat = actState(EX_IGNORE);
    if (pStat == nullptr) return;
    foreach (cHSORow *pRow, pStat->rows) {
        QCheckBox *pCheckBox = pRow->pCheckBoxSub;
        if (pCheckBox != nullptr) {
            pCheckBox->setChecked(true);
        }
    }
}

void cHSOperate::on_pushButtonSubNone_clicked()
{
    cHSOState *pStat = actState(EX_IGNORE);
    if (pStat == nullptr) return;
    foreach (cHSORow *pRow, pStat->rows) {
        QCheckBox *pCheckBox = pRow->pCheckBoxSub;
        if (pCheckBox != nullptr) {
            pCheckBox->setChecked(false);
        }
    }
}

void cHSOperate::on_comboBoxZone_currentIndexChanged(int ix)
{
    if (pPlaceModel == nullptr) return;
    pPlaceModel->setZone(pZoneModel->atId(ix));
}

void cHSOperate::on_checkBoxClrStat_toggled(bool f)
{
    if (permit <= PERMIT_NO) EXCEPTION(EPROGFAIL);
    (void)f;
    setButtonsFromTo();
}

void cHSOperate::setAlarmButtons(int id)
{
    if (lastAlramButtonId == id) {
        pButtonGroupAlarm->setExclusive(false);
        QAbstractButton *pButton = pButtonGroupAlarm->button(id);
        pButton->setChecked(false);
        pButtonGroupAlarm->setExclusive(true);
        lastAlramButtonId = -1;
    }
    else {
        lastAlramButtonId = id;
    }
    setButtonsFromTo();
}


void cHSOperate::on_lineEditPlacePattern_textChanged(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pPlaceModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxPlaceSelect->setCurrentIndex(0);
}

void cHSOperate::on_lineEditNodePattern_textChanged(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pNodeModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxNodeSelect->setCurrentIndex(0);
}

void cHSOperate::on_lineEditServicePattern_textChanged(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pServiceModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxServiceSelect->setCurrentIndex(0);
}

void cHSOperate::on_lineEditSupPattern_textChanged(const QString& text)
{
    QString param = text;
    enum eFilterType ft = FT_SQL_WHERE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) param += "%";
    QString sql = QString("host_service_id2name(host_service_id) LIKE $pattern$%1$pattern$").arg(param);
    pSupModel->setFilter(sql, OT_DEFAULT, ft);
}

void cHSOperate::on_tableWidget_doubleClicked(const QModelIndex& mi)
{
    int row = mi.row();
    int col = mi.column();
    QTableWidgetItem *pi = pUi->tableWidget->item(row, col);
    QString s;
    if (pi == nullptr) {
        if (col != TC_EXT) return;
    }
    else {
        s = pi->text();
    }
    switch (mi.column()) {
    case TC_ID: {
        cHostService hs;
        hs.setById(*pq2, actState()->rows.at(row)->id);
        recordDialog(*pq2, hs.tableName(), this, &hs, permit != PERMIT_ALL, true);
        break;
     }
    case TC_HOST: {
        cPatch *pNode = cPatch::getNodeObjByName(*pq2, s);
        recordDialog(*pq2, pNode->tableName(), this, pNode, permit != PERMIT_ALL, true);
        pDelete(pNode);
        break;
      }
    case TC_SERVICE: {
        const cService *pSrv = cService().service(*pq2, s);
        recordDialog(*pq2, _sServices, this, pSrv, permit != PERMIT_ALL, true);
        break;
      }
    case TC_STATE: {
        const QString sql = "SELECT host_service_id2name(host_service_id), state_msg FROM host_services WHERE host_service_id = ?";
        QVariant hsid = actState()->rows.at(row)->rec.value(RX_ID);
        if (execSql(*pq2, sql, hsid)) {
            QString sStatMsg = pq2->value(1).toString();
            if (!sStatMsg.isEmpty()) {
                QString title = tr("A %1 szervízpéldány utolsó statusz üzenet").arg(pq2->value(0).toString());
                cMsgBox::text(title , sStatMsg);
            }
        }
        break;
     }
    case TC_PLACE: {
        cPlace p;
        p.setByName(*pq2, s);
        recordDialog(*pq2, p.tableName(), this, &p, permit != PERMIT_ALL, true);
        break;
     }
    case TC_NSUB: {
        if (s == "0") break;
        cTableShape *pTs = new cTableShape();
        pTs->setByName(*pq2, "host_services");
        pTs->enum2setOn(_sTableShapeType, TS_TREE);
        pTs->enum2setOn(_sTableShapeType, TS_READ_ONLY);
        cRecordTree rt(pTs, true, nullptr, this);
        rt.init();
        cRecordTreeModel *pModel = static_cast<cRecordTreeModel *>(rt.pModel);
        pModel->rootId = actState()->rows.at(row)->id;
        rt.refresh();
        rt.dialog().exec();
        break;
      }
    case TC_SUPERIOR: {
        cHostService hs;
        hs.setById(*pq2, actState()->rows.at(row)->id);
        hs.setById(*pq2, hs.getId(_sSuperiorHostServiceId));
        recordDialog(*pq2, hs.tableName(), this, &hs, permit != PERMIT_ALL, true);
        break;
     }
    default:    break;
    }
}

void cHSOperate::on_toolButtonBack_clicked()
{
    if (states.isEmpty() || stateIx < 0) return;
    if (stateIx > 0) {
        --stateIx;
        refreshTable();
        pUi->toolButtonBack->setEnabled(stateIx > 0);
        pUi->toolButtonForward->setEnabled(true);
    }
}

void cHSOperate::on_toolButtonForward_clicked()
{
    if (states.isEmpty() || stateIx < 0) return;
    int last = states.size() -1;
    if (last > stateIx) {
        ++stateIx;
        refreshTable();
        pUi->toolButtonBack->setEnabled(true);
        pUi->toolButtonForward->setEnabled(last > stateIx);
    }
}

void cHSOperate::on_toolButtonClearHist_clicked()
{
    if (states.isEmpty() || stateIx < 0) return;
    cHSOState *pState = states.takeAt(stateIx);
    while (!states.isEmpty()) delete states.takeLast();
    states << pState;
    stateIx = 0;
}

void cHSOperate::on_pushButtonRoot_clicked()
{
    pUi->checkBoxPlace->setChecked(false);
    pUi->checkBoxService->setChecked(true);
    pUi->checkBoxNode->setChecked(false);
    pUi->radioButtonServiceSelect->setChecked(true);
    _setCurrentIndex(_sLv2d, pUi->comboBoxServiceSelect);
    pUi->checkBoxSup->setChecked(true);
    pUi->radioButtonSupPattern->setChecked(true);
    pUi->toolButtonSupNull->setChecked(true);
    on_pushButtonFetch_clicked();
}

#define MIN_REFRESH_TIME 10

void cHSOperate::on_pushButtonAutoRefresh_clicked()
{
    if (refreshTime < MIN_REFRESH_TIME) {
        on_spinBoxRefresh_valueChanged(refreshTime);
        return;
    }
    if (timerId > 0) {
        killTimer(timerId);
        timerId = -1;
        pUi->pushButtonAutoRefresh->setText(sStart);
        pUi->progressBarRefresh->setEnabled(false);
        pUi->progressBarRefresh->setValue(0);
    }
    else {
        timerId = startTimer(1000);
        pUi->pushButtonAutoRefresh->setText(sStop);
        pUi->progressBarRefresh->setEnabled(true);
    }
}

void cHSOperate::on_spinBoxRefresh_valueChanged(int v)
{
    refreshTime = v;
    if (v < 10) {
        if (timerId > 0) killTimer(timerId);
        timerId = -1;
        pUi->pushButtonAutoRefresh->setDisabled(true);
        pUi->pushButtonAutoRefresh->setText(sStart);
        pUi->progressBarRefresh->setDisabled(true);
    }
    else {
        pUi->pushButtonAutoRefresh->setText(timerId > 0 ? sStop : sStart);
        pUi->pushButtonAutoRefresh->setEnabled(true);
        pUi->progressBarRefresh->setMaximum(v);
        pUi->progressBarRefresh->setEnabled(timerId > 0);
    }
}

void cHSOperate::on_textEditJustify_textChanged()
{
    if (permit < PERMIT_ALL) setButtonsFromTo();
}

void cHSOperate::on_dateTimeEditFrom_dateTimeChanged(const QDateTime &dateTime)
{
    if (lastAlramButtonId == NAT_FROM_TO) {
        QDateTime dt = dateTime.addMSecs(minIntervalMs);
        if (dt > pUi->dateTimeEditTo->dateTime()) pUi->dateTimeEditTo->setDateTime(dt);
    }
}

void cHSOperate::on_dateTimeEditTo_dateTimeChanged(const QDateTime &dateTime)
{
    if (lastAlramButtonId == NAT_FROM_TO) {
        QDateTime dt = dateTime.addMSecs(- minIntervalMs);
        if (dt < pUi->dateTimeEditFrom->dateTime()) pUi->dateTimeEditFrom->setDateTime(dt);
    }
}

/// Dialog noalarm from
void cHSOperate::on_toolButtonDateFrom_clicked()
{
    QDateTime newFrom;
    QDateTime from = pUi->dateTimeEditFrom->dateTime();
    now = QDateTime::currentDateTime();
    newFrom = cDateTimeDialog::popup(from, now, now);
    if (newFrom.isValid()) {
        pUi->dateTimeEditFrom->setDateTime(newFrom);
    }
}

/// Reset noalarm from
void cHSOperate::on_toolButtonRstFrom_clicked()
{
    now = QDateTime::currentDateTime();
    pUi->dateTimeEditFrom->setDateTime(now);
}

void cHSOperate::on_toolButtonDateTo_clicked()
{
    QDateTime newTo;
    QDateTime to = pUi->dateTimeEditTo->dateTime();
    QDateTime def;
    if (lastAlramButtonId == NAT_FROM_TO) def = pUi->dateTimeEditFrom->dateTime().addMSecs(minIntervalMs);
    else                                  def = now.addMSecs(minIntervalMs);
    now = QDateTime::currentDateTime();
    newTo = cDateTimeDialog::popup(to, now.addMSecs(minIntervalMs), def);
    if (newTo.isValid()) {
        pUi->dateTimeEditFrom->setDateTime(newTo);
    }
}

void cHSOperate::on_toolButtonRstTo_clicked()
{
    QDateTime dt = pUi->dateTimeEditFrom->dateTime();
    dt = dt.addMSecs(minIntervalMs);
    pUi->dateTimeEditTo->setDateTime(dt);
}

void cHSOperate::on_toolButtonIntervalDef_clicked()
{
    now = QDateTime::currentDateTime();
    pUi->dateTimeEditFrom->setDateTime(now);
    QDateTime dt = now.addMSecs(minIntervalMs);
    pUi->dateTimeEditTo->setDateTime(dt);
}

void cHSOperate::on_tableWidget_itemSelectionChanged()
{
    QModelIndexList mil = pUi->tableWidget->selectionModel()->selectedRows();
    QString refine;
    if (mil.isEmpty())  {
        refine = _sFalse;
    }
    else if (mil.size() == 1) {
        int row = mil.first().row();
        qlonglong id = actState()->rows.at(row)->id;
        refine = QString("host_service_id = %1").arg(id);
    }
    else {
        QString ids;
        foreach (QModelIndex mi, mil) {
            int row = mi.row();
            qlonglong id = actState()->rows.at(row)->id;
            ids += QString::number(id) + _sCommaSp;
        }
        ids.chop(_sCommaSp.size());
        refine = QString("host_service_id = ANY ('{%1}'\\:\\:bigint[])").arg(ids);
    }
    pSrvVarsTable->pTableShape->setName(_sRefine, refine);
    pSrvVarsTable->refresh(true);
    pSrvLogsTable->pTableShape->setName(_sRefine, refine);
    pSrvLogsTable->refresh(true);
}

void cHSOperate::on_toolButtonUnknown_clicked()
{
    pUi->checkBoxService->setChecked(false);
    pUi->checkBoxNode->setChecked(false);
    pUi->checkBoxFiltSDisa->setChecked(false);
    pUi->checkBoxFiltHsDisa->setChecked(false);
    pUi->checkBoxFiltHsDisa->setChecked(false);

    pUi->checkBoxFiltStateOn->setChecked(false);
    pUi->checkBoxFiltStatRecovered->setChecked(false);
    pUi->checkBoxFiltStatWarning->setChecked(false);
    pUi->checkBoxFiltStatCritical->setChecked(false);
    pUi->checkBoxFiltStatUnreachable->setChecked(false);
    pUi->checkBoxFiltStatDown->setChecked(false);
    pUi->checkBoxFiltStatFlapping->setChecked(false);
    pUi->checkBoxFiltStatUnknown->setChecked(true);

    QMetaObject::invokeMethod(this, &cHSOperate::on_pushButtonFetch_clicked);
}

void cHSOperate::on_toolButtonCritical_clicked()
{
    pUi->checkBoxService->setChecked(false);
    pUi->checkBoxNode->setChecked(false);
    pUi->checkBoxFiltSDisa->setChecked(false);
    pUi->checkBoxFiltHsDisa->setChecked(false);
    pUi->checkBoxFiltHsDisa->setChecked(false);

    pUi->checkBoxFiltStateOn->setChecked(false);
    pUi->checkBoxFiltStatRecovered->setChecked(false);
    pUi->checkBoxFiltStatWarning->setChecked(false);
    pUi->checkBoxFiltStatCritical->setChecked(true);
    pUi->checkBoxFiltStatUnreachable->setChecked(true);
    pUi->checkBoxFiltStatDown->setChecked(true);
    pUi->checkBoxFiltStatFlapping->setChecked(true);
    pUi->checkBoxFiltStatUnknown->setChecked(true);

    QMetaObject::invokeMethod(this, &cHSOperate::on_pushButtonFetch_clicked);
}

void cHSOperate::on_toolButtonWarning_clicked()
{
    pUi->checkBoxService->setChecked(false);
    pUi->checkBoxNode->setChecked(false);
    pUi->checkBoxFiltSDisa->setChecked(false);
    pUi->checkBoxFiltHsDisa->setChecked(false);
    pUi->checkBoxFiltHsDisa->setChecked(false);

    pUi->checkBoxFiltStateOn->setChecked(false);
    pUi->checkBoxFiltStatRecovered->setChecked(false);
    pUi->checkBoxFiltStatWarning->setChecked(true);
    pUi->checkBoxFiltStatCritical->setChecked(true);
    pUi->checkBoxFiltStatUnreachable->setChecked(true);
    pUi->checkBoxFiltStatDown->setChecked(true);
    pUi->checkBoxFiltStatFlapping->setChecked(true);
    pUi->checkBoxFiltStatUnknown->setChecked(true);

    QMetaObject::invokeMethod(this, &cHSOperate::on_pushButtonFetch_clicked);
}

void cHSOperate::on_checkBoxSup_toggled(bool checked)
{
    enableSup(checked, pUi->radioButtonSupPattern->isChecked(), pUi->toolButtonSupNull->isChecked());
}

void cHSOperate::on_radioButtonSupPattern_toggled(bool checked)
{
    enableSup(pUi->checkBoxSup->isChecked(), checked, pUi->toolButtonSupNull->isChecked());
}

void cHSOperate::on_toolButtonSupNull_toggled(bool checked)
{
    enableSup(pUi->checkBoxSup->isChecked(), pUi->radioButtonSupPattern->isChecked(), checked);
}

void cHSOperate::on_checkBoxPlace_toggled(bool checked)
{
    enablePlace(checked, pUi->radioButtonPlacePattern->isChecked());
}

void cHSOperate::on_radioButtonPlacePattern_toggled(bool checked)
{
    enablePlace(pUi->checkBoxPlace->isChecked(), checked);
}

void cHSOperate::on_checkBoxNode_toggled(bool checked)
{
    enableNode(checked, pUi->radioButtonNodePattern->isChecked());
}

void cHSOperate::on_radioButtonNodePattern_toggled(bool checked)
{
    enableNode(pUi->checkBoxNode->isChecked(), checked);
}

void cHSOperate::on_checkBoxService_toggled(bool checked)
{
    enableService(checked, pUi->radioButtonServicePattern->isChecked());
}

void cHSOperate::on_radioButtonServicePattern_toggled(bool checked)
{
    enableService(pUi->checkBoxService->isChecked(), checked);
}

void cHSOperate::on_checkBoxRemove_toggled(bool checked)
{
    pUi->checkBoxDisable->      setDisabled(checked || permit < PERMIT_ALL);
    pUi->checkBoxDelegate->     setDisabled(checked || permit < PERMIT_ALL);
    pUi->checkBoxClrStat->      setDisabled(checked || permit < PERMIT_ALL );
    pUi->checkBoxAlarmOff->     setDisabled(checked || permit < PERMIT_PART);
    pUi->checkBoxAlarmOn->      setDisabled(checked || permit < PERMIT_PART );
    pUi->checkBoxSetInterval->  setDisabled(checked || permit < PERMIT_PART);
    pUi->checkBoxSetFrom->      setDisabled(checked || permit < PERMIT_PART);
    pUi->checkBoxSetTo->        setDisabled(checked || permit < PERMIT_PART);

    pUi->toolButtonIntervalDef->setDisabled(checked || permit < PERMIT_PART);
    pUi->toolButtonDateFrom->   setDisabled(checked || permit < PERMIT_PART);
    pUi->toolButtonDateTo->     setDisabled(checked || permit < PERMIT_PART);
    pUi->toolButtonRstFrom->    setDisabled(checked || permit < PERMIT_PART);
    pUi->toolButtonRstTo->      setDisabled(checked || permit < PERMIT_PART);

    pUi->dateTimeEditFrom->     setDisabled(checked || permit < PERMIT_PART);
    pUi->dateTimeEditTo->       setDisabled(checked || permit < PERMIT_PART);

    pUi->checkBoxClrStat->      setDisabled(checked || permit < PERMIT_ALL);
    pUi->checkBoxDelAlarm->     setDisabled(checked || permit < PERMIT_ALL);
    pUi->checkBoxDelStatLog->   setDisabled(checked || permit < PERMIT_ALL);
    pUi->checkBoxDelMemo->      setDisabled(checked || permit < PERMIT_ALL);

}

void cHSOperate::removeSelected()
{
    if (permit != PERMIT_ALL) EXCEPTION(EPROGFAIL);
    if (cMsgBox::yes(tr("Valóban törölni kívánja a kijelült példánhyokat? A művelet nem visszavonható!"), this)) {
        int rows = pUi->tableWidget->rowCount();
        static const QString tn = "HSOparateDel";
        cError *pe = nullptr;
        sqlBegin(*pq2, tn);
        try {
            for (int row = 0; row < rows; ++row) {
                cHSORow *pRow = actState()->rows.at(row);
                if (pRow->set) {
                    qlonglong hsid = pRow->id;
                    cHostService hs;
                    hs.setId(hsid);
                    hs.remove(*pq2);
                }
            }
        } CATCHS(pe)
        if (pe != nullptr) {
            sqlRollback(*pq2, tn);
            cErrorMessageBox::messageBox(pe, this);
            on_pushButtonRefresh_clicked();  // Elrontottuk, újra olvassuk
        }
        sqlCommit(*pq2, tn);
        refreshTable();
    }
}
