#include "hsoperate.h"
#include "srvdata.h"
#include "record_dialog.h"
#include "record_tree.h"

const enum ePrivilegeLevel cHSOperate::rights = PL_OPERATOR;

enum eFieldIx {
    RX_ID,
    RX_HOST_NAME, RX_SERVICE_NAME, RX_PORT_NAME, RX_SRV_EXT,
    RX_PLACE_NAME, RX_PLACE_TYPE, RX_NOALARM, RX_FROM, RX_TO,
    RX_DISABLED, RX_SRV_DISABLED, RX_STATE, RX_NSUB,
    RX_SUPERIOR_ID, RX_SUPERIOR_NAME
};

const QString cHSOperate::_sql =
        "SELECT"
            " hs.host_service_id, "         // RX_ID
            " node_name, "                  // RX_HOST_NAME
            " service_name, "               // RX_SERVICE_NAME
            " CASE WHEN hs.port_id IS NULL THEN NULL"
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
            " (SELECT COUNT(*) FROM host_services AS shs WHERE shs.superior_host_service_id = hs.host_service_id), "    // RX_NSUB
            " hs.superior_host_service_id, "// RX_SUPERIOR_ID
            " CASE WHEN hs.superior_host_service_id IS NULL THEN NULL"
                 " ELSE host_service_id2name(hs.superior_host_service_id)"
                 " END"                     // RX_SUPERIOR_NAME
        " FROM host_services AS hs"
        " JOIN nodes  AS n USING(node_id)"
        " JOIN places AS p USING(place_id)"
        " JOIN services AS s USING(service_id)";
const QString cHSOperate::_ord = " ORDER BY node_name, service_name, hs.port_id ASC";

const cColEnumType    *cHSORow::pPlaceType   = NULL;
const cColEnumType    *cHSORow::pNoAlarmType = NULL;
const cColEnumType    *cHSORow::pNotifSwitch = NULL;

cHSORow::cHSORow(QSqlQuery& q, cHSOState *par)
    : QObject(par), nsub(0), rec(q.record())
{
    pq = par->pq;
    staticInit();
    set = true;
    sub = false;
    bool ok;
    id  = rec.value(RX_ID).toLongLong(&ok);
    if (!ok) EXCEPTION(EDATA, RX_ID, rec.value(RX_ID).toString());
    QVariant v = rec.value(RX_NSUB);
    if (v.isValid()) {
        nsub  = v.toLongLong(&ok);
        if (!ok) EXCEPTION(EDATA, RX_NSUB, v.toString());
    }
}

void cHSORow::staticInit()
{
    if (pPlaceType == NULL) {
        QSqlQuery q = getQuery();
        pPlaceType   = cColEnumType::fetchOrGet(q, "placetype");
        pNoAlarmType = cColEnumType::fetchOrGet(q, "noalarmtype");
        pNotifSwitch = cColEnumType::fetchOrGet(q, "notifswitch");
    }
}

QCheckBox * cHSORow::getCheckBoxSet()
{
    QCheckBox *p = new QCheckBox;
    p->setChecked(set);
    connect(p, SIGNAL(toggled(bool)), this, SLOT(togleSet(bool)));
    return p;
}

QCheckBox * cHSORow::getCheckBoxSub()
{
    if (nsub == 0) return NULL;
    QCheckBox *p = new QCheckBox;
    p->setChecked(sub);
    connect(p, SIGNAL(toggled(bool)), this, SLOT(togleSub(bool)));
    return p;
}


QToolButton* cHSORow::getButtonReset()
{
    static QStringList srvList;
    if (srvList.isEmpty()) {    // újraindítható szolgáltatások/lekérdezések nevei
        srvList << "lv2d" << "portmac" << "porstat" << "arpd";
    }
    QString srvName = rec.value(RX_SERVICE_NAME).toString();
    bool disabled = rec.value(RX_DISABLED).toBool();
    disabled = disabled || rec.value(RX_SRV_DISABLED).toBool();
    QToolButton *pButton = NULL;
    if (!disabled && srvList.contains(srvName)) {
        pButton = new QToolButton();
        pButton->setIcon(QIcon("://icons/system-restart.ico"));
        connect(pButton, SIGNAL(clicked()), this, SLOT(pressReset()));
    }
    return pButton;
}


QTableWidgetItem * cHSORow::item(int vix)
{
    QVariant v = rec.value(vix);
    if (v.isNull()) return NULL;
    return new QTableWidgetItem(v.toString());
}

QTableWidgetItem * cHSORow::item(int ix, const cColEnumType *pType)
{
    QString s = rec.value(ix).toString();
    QTableWidgetItem *pi = new QTableWidgetItem(s);
    pi->setBackground(bgColorByEnum(*pType, pType->str2enum(s)));
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
    if (v.isNull()) return NULL;
    v = cd.fromSql(v);
    QString s = cd.toView(*pq, v);
    return new QTableWidgetItem(s);
}

QTableWidgetItem * cHSORow::boolItem(int ix, const QString& tn, const QString& fn)
{
    bool     b = rec.value(ix).toBool();
    QString  s = langBool(b);
    QTableWidgetItem *pi = new QTableWidgetItem(s);
    pi->setBackground(bgColorByBool(tn, fn, bool2boolVal(b)));
    return pi;

}

void cHSORow::pressReset()
{
    QString srvName = rec.value(RX_SERVICE_NAME).toString();
    sqlNotify(*pq, srvName, _sReset);
}


cHSOState::cHSOState(QSqlQuery& q, const QString& _sql, const QVariantList _binds, cHSOperate *par)
    : QObject(par), sql(_sql), binds(_binds)
{
    size = nsup = 0;
    pq = par->pq2;
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    int i = 0;
    foreach (QVariant v, binds) {
        q.bindValue(i, v);
        ++i;
    }
    if (!q.exec()) SQLQUERYERR(q);
    if (q.first()) do {
        cHSORow *p = new cHSORow(q, this);
        if (p->nsub > 0) ++nsup;
        rows << p;
    } while(q.next());
    size = rows.size();
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
    return NULL;
}

enum eFilterButtonId {
    FT_PATTERN, FT_SELECT
};

enum eTableColumnIx {
    TC_HOST,
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
    TC_CBOX_SEL,
    TC_NSUB,
    TC_CBOX_NSUB,
    TC_SUPERIOR,
    TC_RESTART,
    TC_COUNT
};

cHSOperate::cHSOperate(QMdiArea *par)
    : cIntSubObj(par)
{
    pq  = newQuery();
    pq2 = newQuery();
    pUi = new Ui_hostServiceOp;
    stateIx = -1;
    pUi->setupUi(this);
    if (TC_COUNT != pUi->tableWidget->columnCount()) EXCEPTION(EDATA);

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

    pZoneModel = new cZoneListModel(this);
    pUi->comboBoxZone->setModel(pZoneModel);
    pZoneModel->setFilter();
    pUi->comboBoxZone->setCurrentText(_sAll);

    connect(pUi->comboBoxZone,      SIGNAL(currentIndexChanged(int)), this, SLOT(zoneChanged(int)));

    connect(pUi->pushButtonClose,   SIGNAL(clicked()),  this,   SLOT(endIt()));
    connect(pUi->pushButtonSet,     SIGNAL(clicked()),  this,   SLOT(set()));
    connect(pUi->pushButtonFetch,   SIGNAL(clicked()),  this,   SLOT(fetchByFilter()));
    connect(pUi->pushButtonSub,     SIGNAL(clicked()),  this,   SLOT(fetchSubs()));
    connect(pUi->pushButtonAll,     SIGNAL(clicked()),  this,   SLOT(all()));
    connect(pUi->pushButtonNone,    SIGNAL(clicked()),  this,   SLOT(none()));
    connect(pUi->pushButtonSubAll,  SIGNAL(clicked()),  this,   SLOT(subAll()));
    connect(pUi->pushButtonSubNone, SIGNAL(clicked()),  this,   SLOT(subNone()));

    connect(pUi->lineEditPlacePattern,   SIGNAL(textChanged(QString)), this, SLOT(changePlacePattern(QString)));
    connect(pUi->lineEditNodePattern,    SIGNAL(textChanged(QString)), this, SLOT(changeNodePattern(QString)));
    connect(pUi->lineEditServicePattern, SIGNAL(textChanged(QString)), this, SLOT(changeServicePattern(QString)));

    connect(pUi->checkBoxDisable, SIGNAL(toggled(bool)), this, SLOT(disable(bool)));
    connect(pUi->checkBoxEnable,  SIGNAL(toggled(bool)), this, SLOT(enable(bool)));
    connect(pUi->checkBoxClrStat, SIGNAL(toggled(bool)), this, SLOT(clrStat(bool)));

    connect(pUi->checkBoxAlarmOn,  SIGNAL(toggled(bool)), this, SLOT(enableAlarm(bool)));
    connect(pUi->checkBoxAlarmOff, SIGNAL(toggled(bool)), this, SLOT(disableAlarm(bool)));

    connect(pUi->tableWidget,      SIGNAL(doubleClicked(QModelIndex)), this, SLOT(doubleClickCell(QModelIndex)));

    connect(pUi->toolButtonBack,   SIGNAL(clicked()),     this, SLOT(back()));
    connect(pUi->toolButtonForward,SIGNAL(clicked()),     this, SLOT(forward()));
    connect(pUi->toolButtonClear,  SIGNAL(clicked()),     this, SLOT(clear()));

    connect(pUi->pushButtonRefresh,SIGNAL(clicked()),     this, SLOT(refresh()));
    connect(pUi->pushButtonRoot,   SIGNAL(clicked()),     this, SLOT(root()));

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
}


cHSOperate::~cHSOperate()
{
    delete pq;
    delete pq2;
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
        setCell(row, TC_HOST,    pRow->item(RX_HOST_NAME));
        setCell(row, TC_SERVICE, pRow->item(RX_SERVICE_NAME));
        setCell(row, TC_PORT,    pRow->item(RX_PORT_NAME));
        setCell(row, TC_EXT,     pRow->item(RX_SRV_EXT));
        setCell(row, TC_PLACE,   pRow->item(RX_PLACE_NAME, RX_PLACE_TYPE, cHSORow::pPlaceType));
        setCell(row, TC_NOALARM, pRow->item(RX_NOALARM, cHSORow::pNoAlarmType));
        setCell(row, TC_FROM,    pRow->item(RX_FROM, hs.colDescr(hs.toIndex(_sNoalarmFrom))));
        setCell(row, TC_TO,      pRow->item(RX_TO, hs.colDescr(hs.toIndex(_sNoalarmTo))));
        setCell(row, TC_DISABLED,pRow->boolItem(RX_DISABLED, _sHostServices, _sDisabled));
        setCell(row, TC_DISABLED_SRV, pRow->boolItem(RX_SRV_DISABLED, _sServices, _sDisabled));
        setCell(row, TC_STATE,   pRow->item(RX_STATE, cHSORow::pNotifSwitch));
        setCell(row, TC_CBOX_SEL,pRow->getCheckBoxSet());
        setCell(row, TC_NSUB,    pRow->item(RX_NSUB));
        setCell(row, TC_CBOX_NSUB,pRow->getCheckBoxSub());
        setCell(row, TC_SUPERIOR,pRow->item(RX_SUPERIOR_NAME));
        setCell(row, TC_RESTART, pRow->getButtonReset());
        row++;
    }
    bool noSup = actState()->nsup == 0;
    pUi->tableWidget->resizeColumnsToContents();
    pUi->pushButtonAll->setDisabled(false);
    pUi->pushButtonNone->setDisabled(false);
    pUi->pushButtonSubAll->setDisabled(noSup);
    pUi->pushButtonSubNone->setDisabled(noSup);
    pUi->pushButtonSub->setDisabled(noSup);
    setButton();
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
    pUi->toolButtonClear->setEnabled(stateIx > 0);
    pUi->toolButtonForward->setDisabled(true);
    pUi->pushButtonRefresh->setEnabled(true);
    return true;
}

void cHSOperate::refresh()
{
    cHSOState *p = new cHSOState(*pq, actState()->sql, actState()->binds, this);
    cHSOState *pOld = actState();
    foreach (cHSORow *pRow, p->rows) {
        qlonglong id = pRow->id;
        cHSORow *pOldRow = pOld->rowAtId(id);
        if (pOldRow != NULL) {
            pRow->set = pOldRow->set;
            pRow->sub = pOldRow->sub;
        }
    }
    states[stateIx] = p;
    delete pOld;
    refreshTable();
}

void cHSOperate::fetchByFilter()
{
    static const QChar j('%');

    bool isHsDis  = pUi->checkBoxHsDisa->isChecked();
    bool isHsEna  = pUi->checkBoxHsEna->isChecked();
    bool isSDis   = pUi->checkBoxSDisa->isChecked();
    bool isSEna   = pUi->checkBoxSEna->isChecked();

    bool isOff    = pUi->checkBoxOff->isChecked();
    bool isOn     = pUi->checkBoxOn->isChecked();
    bool isTo     = pUi->checkBoxTo->isChecked();
    bool isFrom   = pUi->checkBoxFrom->isChecked();
    bool isFromTo = pUi->checkBoxFromTo->isChecked();

    QString         sql = _sql;
    QStringList     where;
    QVariantList    bind;
    QStringList     wl;

    where.clear();
    if (!(isHsDis == isHsEna)) {
        if (isHsDis) where << " hs.disabled ";
        if (isHsEna) where << " NOT hs.disabled ";
    }

    if (!(isSDis == isSEna)) {
        if (isSDis) where << " s.disabled ";
        if (isSEna) where << " NOT s.disabled ";
    }

    if ((isOff || isOn || isTo || isFrom || isFromTo) && !(isOff && isOn && isTo && isFrom && isFromTo)) {
        wl.clear();
        if (isOff)  wl << " hs.noalarm_flag = 'off' ";
        if (isOn)   wl << " hs.noalarm_flag = 'on' ";
        if (isTo)   wl << " hs.noalarm_flag = 'to' ";
        if (isFrom) wl << " hs.noalarm_flag = 'from' ";
        if (isFrom) wl << " hs.noalarm_flag = 'from_to' ";
        if (!wl.isEmpty()) {
            QString w = wl.join("OR ");
            if (wl.size() > 1) w = "(" + w + ") ";
            where << w;
        }
    }

    // Filter by zone
    QString zone = pUi->comboBoxZone->currentText();
    if (zone != _sAll) {
        int       ci  = pUi->comboBoxZone->currentIndex();
        qlonglong gid = pZoneModel->atId(ci);
        where << " is_group_place(place_id, ?::bigint)";
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
                where << " ns.node_id = ?";
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
    if (!where.isEmpty()) sql += " WHERE" + where.join(" AND");
    sql += _ord;
    fetch(sql, bind);
}

void cHSOperate::fetchSubs()
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

void cHSOperate::set()
{
    static const QString tn = "HSOparateSet";
    pUi->pushButtonSet->setDisabled(true);
    int rows = pUi->tableWidget->rowCount();
    cHostService hs;
    QBitArray um_disabled = hs.mask(_sDisabled);
    QBitArray um_noalarm  = hs.mask(_sNoalarmFlag);
    QStringList csf;
    csf << _sHostServiceState << _sSoftState << _sHardState << _sStateMsg << _sCheckAttempts
        << _sLastChanged  << _sLastTouched << _sActAlarmLogId;
    QBitArray um_ClrState = hs.mask(csf);
    bool disable = pUi->checkBoxDisable->isChecked();
    bool enable  = pUi->checkBoxEnable->isChecked();
    bool alarmOn = pUi->checkBoxAlarmOn->isChecked();
    bool alarmOff= pUi->checkBoxAlarmOff->isChecked();
    bool clrStat = pUi->checkBoxClrStat->isChecked();
    bool statLog = pUi->checkBoxStatLog->isChecked();
    bool alarm   = pUi->checkBoxAlarm->isChecked();
    bool memo    = pUi->checkBoxMemo->isChecked();
    cError *pe = NULL;
    sqlBegin(*pq2, tn);
    try {
        for (int row = 0; row < rows; ++row) {
            cHSORow *pRow = actState()->rows.at(row);
            if (pRow->set) {
                QSqlRecord& rec = pRow->rec;
                hs.clear();
                QBitArray um(hs.cols());
                qlonglong id = pRow->id;
                if (disable) {
                    hs.setBool(_sDisabled, true);
                    um |= um_disabled;
                    rec.setValue(RX_DISABLED, true);
                }
                else if (enable) {
                    hs.setBool(_sDisabled, false);
                    um |= um_disabled;
                    rec.setValue(RX_DISABLED, false);
                }
                if (alarmOn) {
                    hs.setName(_sNoalarmFlag, _sOff);
                    um |= um_noalarm;
                    rec.setValue(RX_NOALARM, _sOff);
                }
                else if (alarmOff) {
                    hs.setName(_sNoalarmFlag, _sOn);
                    um |= um_noalarm;
                    rec.setValue(RX_NOALARM, _sOn);
                }
                if (clrStat) {
                    hs.setId(_sHostServiceState, RS_UNKNOWN);
                    hs.setId(_sSoftState, RS_UNKNOWN);
                    hs.setId(_sHardState, RS_UNKNOWN);
                    hs.setId(_sCheckAttempts, 0);
                    um |= um_ClrState;
                    rec.setValue(RX_STATE, _sUnknown);
                }
                if (um.count(true) > 0) {
                    hs.setId(id);
                    pe = hs.tryUpdate(*pq2, false, um);
                    if (pe != NULL) break;
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
    if (pe != NULL) {
        sqlRollback(*pq2, tn);
        cErrorMessageBox::messageBox(pe, this);
        refresh();  // Elrontottuk, újra olvassuk
    }
    sqlEnd(*pq2, tn);
    refreshTable();
}

void cHSOperate::setButton()
{
    bool f = !states.isEmpty()
          && (pUi->checkBoxDisable->isChecked()
           || pUi->checkBoxEnable->isChecked()
           || pUi->checkBoxClrStat->isChecked()
           || pUi->checkBoxAlarmOn->isChecked()
           || pUi->checkBoxAlarmOff->isChecked());
    pUi->pushButtonSet->setEnabled(f);
}

cHSOState * cHSOperate::actState(eEx __ex)
{
    if (__ex == EX_IGNORE && stateIx < 0) return NULL;
    if (!isContIx(states, stateIx)) EXCEPTION(ENOINDEX, stateIx);
    return states.at(stateIx);
}

void cHSOperate::all()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CBOX_SEL));
        pCb->setChecked(true);
    }
}

void cHSOperate::none()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CBOX_SEL));
        pCb->setChecked(false);
    }
}

void cHSOperate::subAll()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QWidget *pw = pUi->tableWidget->cellWidget(row, TC_CBOX_NSUB);
        if (pw == NULL) continue;
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pw);
        pCb->setChecked(true);
    }
}

void cHSOperate::subNone()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QWidget *pw = pUi->tableWidget->cellWidget(row, TC_CBOX_NSUB);
        if (pw == NULL) continue;
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pw);
        pCb->setChecked(false);
    }
}

void cHSOperate::zoneChanged(int ix)
{
    pPlaceModel->setZone(pZoneModel->atId(ix));
}

void cHSOperate::disable(bool f)
{
    if (f) {
        pUi->checkBoxEnable->setChecked(false);
    }
    setButton();
}

void cHSOperate::enable(bool f)
{
    if (f) {
        pUi->checkBoxDisable->setChecked(false);
    }
    setButton();
}

void cHSOperate::clrStat(bool f)
{
    (void)f;
    setButton();
}

void cHSOperate::disableAlarm(bool f)
{
    if (f) {
        pUi->checkBoxAlarmOn->setChecked(false);
    }
    setButton();
}

void cHSOperate::enableAlarm(bool f)
{
    if (f) {
        pUi->checkBoxAlarmOff->setChecked(false);
    }
    setButton();
}


void cHSOperate::changePlacePattern(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pPlaceModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxPlaceSelect->setCurrentIndex(0);
}

void cHSOperate::changeNodePattern(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pNodeModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxNodeSelect->setCurrentIndex(0);
}

void cHSOperate::changeServicePattern(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pServiceModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxServiceSelect->setCurrentIndex(0);
}

void cHSOperate::doubleClickCell(const QModelIndex& mi)
{
    int row = mi.row();
    int col = mi.column();
    QTableWidgetItem *pi = pUi->tableWidget->item(row, col);
    QString s;
    if (pi == NULL) {
        if (col != TC_EXT && col != TC_CBOX_NSUB) return;
    }
    else {
        s = pi->text();
    }
    switch (mi.column()) {
    case TC_HOST: {
        cPatch *pNode = cPatch::getNodeObjByName(*pq2, s);
        recordInsertDialog(*pq2, pNode->tableName(), this, pNode, true);
        pDelete(pNode);
        break;
      }
    case TC_SERVICE: {
        const cService *pSrv = cService().service(*pq2, s);
        recordInsertDialog(*pq2, _sServices, this, pSrv, true);
        break;
      }
    case TC_PORT: {
        QString nodeName = getTableItemText(pUi->tableWidget, row, TC_HOST);
        qlonglong pid = cNPort().getPortIdByName(*pq2, s, nodeName);
        cNPort *pPort = cNPort::getPortObjById(*pq2, pid);
        recordInsertDialog(*pq2, pPort->tableName(), this, pPort, true);
        pDelete(pPort);
        break;
      }
    case TC_EXT: {
        cHostService hs;
        hs.setById(*pq2, actState()->rows.at(row)->id);
        recordInsertDialog(*pq2, hs.tableName(), this, &hs, true);
        break;
     }
    case TC_PLACE: {
        cPlace p;
        p.setByName(*pq2, s);
        recordInsertDialog(*pq2, p.tableName(), this, &p, true);
        break;
     }
    case TC_NSUB: {
        if (s == "0") break;
        cTableShape *pTs = new cTableShape();
        pTs->setByName(*pq2, "host_services_tree");
        pTs->enum2setOn(_sTableShapeType, TS_READ_ONLY);
        cRecordTree rt(pTs, true, NULL, this);
        rt.init();
        cRecordTreeModel *pModel = (cRecordTreeModel *)rt.pModel;
        pModel->rootId = actState()->rows.at(row)->id;
        rt.refresh();
        rt.dialog().exec();
        break;
      }
    case TC_SUPERIOR: {
        cHostService hs;
        hs.setById(*pq2, actState()->rows.at(row)->id);
        hs.setById(*pq2, hs.getId(_sSuperiorHostServiceId));
        recordInsertDialog(*pq2, hs.tableName(), this, &hs, true);
        break;
     }
    case TC_CBOX_NSUB: {
        QWidget *pW = pUi->tableWidget->cellWidget(row, TC_CBOX_NSUB);
        if (pW == NULL) break;
        subNone();
        ((QCheckBox *)pW)->setChecked(true);
        fetchSubs();
      }
    default:    break;
    }
}

void cHSOperate::back()
{
    if (states.isEmpty() || stateIx < 0) return;
    if (stateIx > 0) {
        --stateIx;
        refreshTable();
        pUi->toolButtonBack->setEnabled(stateIx > 0);
        pUi->toolButtonForward->setEnabled(true);
    }
}

void cHSOperate::forward()
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

void cHSOperate::clear()
{
    if (states.isEmpty() || stateIx < 0) return;
    cHSOState *pState = states.takeAt(stateIx);
    while (!states.isEmpty()) delete states.takeLast();
    states << pState;
    stateIx = 0;
}

void cHSOperate::root()
{
    pUi->checkBoxPlace->setChecked(false);
    pUi->checkBoxService->setChecked(true);
    pUi->checkBoxNode->setChecked(false);
    _setCurrentIndex("lv2d", pUi->comboBoxServiceSelect);
    pUi->radioButtonServiceSelect->setChecked(true);
    fetchByFilter();
}
