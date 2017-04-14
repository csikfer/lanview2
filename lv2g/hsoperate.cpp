#include "hsoperate.h"
#include "srvdata.h"
#include "record_dialog.h"
#include "record_tree.h"

const enum ePrivilegeLevel cHSOperate::rights = PL_OPERATOR;

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
    TC_STATE,
    TC_CBOX_SEL,
    TC_NSUB,
    TC_CBOX_NSUB,
    TC_SUPERIOR,
    TC_COUNT
};

cHSOperate::cHSOperate(QMdiArea *par)
    : cIntSubObj(par)
{
    pq  = newQuery();
    pq2 = newQuery();
    pUi = new Ui_hostServiceOp;
    historyIx = -1;
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

    _sql =
            "SELECT"
                " hs.host_service_id, "         // RX_ID
                " node_name, "                  // RX_HOST
                " service_name, "               // RX_SERVICE
                " hs.port_id, "                 // RX_PORT (ID)
                " proto_service_id, "           // RX_PROTO_SRV (ID)
                " prime_service_id, "           // RX_PRIME_SRV (ID)
                " p.place_name, "               // RX_PLACE
                " hs.noalarm_flag, "            // RX_NOALARM
                " hs.noalarm_from, "            // RX_FROM
                " hs.noalarm_to, "              // RX_TO
                " hs.disabled, "                // RX_DISABLED
                " s.disabled AS sd,"            // RX_SRV_DISABLED
                " hs.host_service_state,"        // RX_STATE
                " (SELECT COUNT(*) FROM host_services AS shs WHERE shs.superior_host_service_id = hs.host_service_id) AS nsub, "
                " CASE WHEN hs.superior_host_service_id IS NULL THEN NULL"
                     " ELSE host_service_id2name(hs.superior_host_service_id) END"
            " FROM host_services AS hs"
            " JOIN nodes  AS n USING(node_id)"
            " JOIN places AS p USING(place_id)"
            " JOIN services AS s USING(service_id)";
    _ord = " ORDER BY node_name, service_name, hs.port_id ASC";
}

enum eFieldIx {
    RX_ID,
    RX_HOST, RX_SERVICE, RX_PORT, RX_PROTO_SRV, RX_PRIME_SRV,
    RX_PLACE, RX_NOALARM, RX_FROM, RX_TO,
    RX_DISABLED, RX_SRV_DISABLED, RX_STATE, RX_NSUB, RX_SUPERIOR
};

cHSOperate::~cHSOperate()
{
    delete pq;
    delete pq2;
}

void cHSOperate::refreshTable(QList<QSqlRecord> &recs)
{
    QVariant v;
    QString s;
    QTableWidgetItem *pi;
    const cColStaticDescr *pCd;
    cHostService hs;
    QCheckBox *pCb;
    qlonglong id;

    int row = 0;
    QColor gray_color("gray");
    QBrush gray(gray_color);
    QFont  italic;
    italic.setItalic(true);
    pUi->tableWidget->setRowCount(0);
    pUi->tableWidget->setRowCount(recs.size());
    idList.clear();
    supIdMap.clear();
    for (QList<QSqlRecord>::const_iterator i = recs.begin(); i < recs.end(); ++i) {
        const QSqlRecord& r = *i;
        v = r.value(RX_ID);
        qlonglong hsid = v.toLongLong();
        idList << hsid;

        v = r.value(RX_HOST);
        s = v.toString();
        pUi->tableWidget->setItem(row, TC_HOST, new QTableWidgetItem(s));

        v = r.value(RX_SERVICE);
        s = v.toString();
        pUi->tableWidget->setItem(row, TC_SERVICE, new QTableWidgetItem(s));

        v = r.value(RX_PORT);
        if (!v.isNull()) {
            id = v.toLongLong();
            s = cNPort().getNameById(*pq2, id);
            pUi->tableWidget->setItem(row, TC_PORT, new QTableWidgetItem(s));
        }

        // TC_EXT
        QString pro, pri;
        v = r.value(RX_PROTO_SRV);
        if (!v.isNull() && (id = v.toLongLong()) != cService::nilId) {
            pro = cService::id2name(*pq2, id);
        }
        v = r.value(RX_PRIME_SRV);
        if (!v.isNull() && (id = v.toLongLong()) != cService::nilId) {
            pri = cService::id2name(*pq2, id);
        }
        if (!(pro.isEmpty() && pri.isEmpty())) {
            s = QString("%1:%2").arg(pro, pri);
            pUi->tableWidget->setItem(row, TC_EXT, new QTableWidgetItem(s));
        }

        v = r.value(RX_PLACE);
        s = v.toString();
        pi = new QTableWidgetItem(s);
        if (s == _sUnknown) {
            pi->setForeground(gray);
            pi->setFont(italic);
        }
        pUi->tableWidget->setItem(row, TC_PLACE, pi);

        v = r.value(RX_NOALARM);
        pCd = &hs.colDescr(hs.toIndex(_sNoalarmFlag));
        v = pCd->fromSql(v);
        s = pCd->toView(*pq2, v);
        pi = new QTableWidgetItem(s);
        Qt::GlobalColor c;
        switch (pCd->toId(v)) {
        case NAT_ON:    c = Qt::red;        break;
        case NAT_OFF:   c = Qt::green;      break;
        default:        c = Qt::yellow;     break;
        }
        pi->setBackground(QBrush(c));
        pUi->tableWidget->setItem(row, TC_NOALARM, pi);

        v = r.value(RX_FROM);
        if (!v.isNull()) {
            pCd = &hs.colDescr(hs.toIndex(_sNoalarmFrom));
            v = pCd->fromSql(v);
            s = pCd->toView(*pq2, v);
            pi = new QTableWidgetItem(s);
            pUi->tableWidget->setItem(row, TC_FROM, pi);
        }

        v = r.value(RX_TO);
        if (!v.isNull()) {
            pCd = &hs.colDescr(hs.toIndex(_sNoalarmTo));
            v = pCd->fromSql(v);
            s = pCd->toView(*pq2, v);
            pi = new QTableWidgetItem(s);
            pUi->tableWidget->setItem(row, TC_TO, pi);
        }

        v = r.value(RX_SRV_DISABLED);
        pCd = &hs.colDescr(hs.toIndex(_sDisabled));
        v = pCd->fromSql(v);
        bool sd = v.toBool();
        v = r.value(RX_DISABLED);
        v = pCd->fromSql(v);
        s = pCd->toView(*pq2, v);
        pi = new QTableWidgetItem(s);
        if (sd) {
            c = Qt::magenta;
            pi->setToolTip(trUtf8("Disabled by service."));
        }
        else if (v.toBool()) {
            c = Qt::red;
        }
        else {
            c = Qt::green;
        }
        pi->setBackground(QBrush(c));
        pUi->tableWidget->setItem(row, TC_DISABLED, pi);

        v = r.value(RX_STATE);
        pCd = &hs.colDescr(hs.toIndex(_sHardState));
        v = pCd->fromSql(v);
        s = pCd->toView(*pq2, v);
        pi = new QTableWidgetItem(s);
        switch (pCd->toId(v)) {
        case RS_ON:
        case RS_RECOVERED:      c = Qt::green;      break;
        case RS_WARNING:        c = Qt::yellow;     break;
        case RS_CRITICAL:
        case RS_UNREACHABLE:
        case RS_DOWN:           c = Qt::red;        break;
        case RS_FLAPPING:       c = Qt::magenta;    break;
        case RS_UNKNOWN:
        default:                c = Qt::white;      break;
        }
        pi->setBackground(QBrush(c));
        pUi->tableWidget->setItem(row, TC_STATE, pi);

        pCb = new QCheckBox;
        pCb->setChecked(true);
        pUi->tableWidget->setCellWidget(row, TC_CBOX_SEL, pCb);

        v = r.value(RX_NSUB);
        int n = v.toInt();
        pi = new QTableWidgetItem(QString::number(n));
        pUi->tableWidget->setItem(row, TC_NSUB, pi);
        if (n > 0) {
            pCb = new QCheckBox;
            pUi->tableWidget->setCellWidget(row, TC_CBOX_NSUB, pCb);
            supIdMap[row] = hsid;
        }

        v = r.value(RX_SUPERIOR);
        if (!v.isNull()) {
            s = v.toString();
            pi = new QTableWidgetItem(s);
            pUi->tableWidget->setItem(row, TC_SUPERIOR, pi);
        }

        row++;
    }
    pUi->tableWidget->resizeColumnsToContents();
    pUi->pushButtonAll->setDisabled(false);
    pUi->pushButtonNone->setDisabled(false);
    pUi->pushButtonSubAll->setDisabled(supIdMap.isEmpty());
    pUi->pushButtonSubNone->setDisabled(supIdMap.isEmpty());
    pUi->pushButtonSub->setDisabled(supIdMap.isEmpty());
    setButton();
    return;
}

void cHSOperate::fetch()
{

    if (!pq->exec()) SQLQUERYERR(*pq);
    if (pq->first()) {
        QList<QSqlRecord>   recs;
        do {
            recs << pq->record();
        } while(pq->next());
        while (history.size() > (historyIx +1)) history.pop_back();
        history << recs;
        historyIx++;
        pUi->toolButtonBack->setEnabled(historyIx > 0);
        pUi->toolButtonClear->setEnabled(historyIx > 0);
        pUi->toolButtonForward->setDisabled(true);
        refreshTable(recs);
    }
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

    if (!(isHsDis == isHsEna)) {
        QStringList wl;
        if (isHsDis) wl << " hs.disabled ";
        if (isHsEna) wl << " NOT hs.disabled ";
        if (!wl.isEmpty()) {
            QString w = where.join("OR ");
            if (wl.size() > 1) w = "(" + w + ") ";
            where << w;
        }
    }

    if (!(isSDis == isSEna)) {
        QStringList wl;
        if (isSDis) wl << " s.disabled ";
        if (isSEna) wl << " NOT s.disabled ";
        if (!wl.isEmpty()) {
            QString w = wl.join("OR ");
            if (wl.size() > 1) w = "(" + w + ") ";
            where << w;
        }
    }

    if ((isOff || isOn || isTo || isFrom || isFromTo) && !(isOff && isOn && isTo && isFrom && isFromTo)) {
        QStringList wl;
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
                where << " n.place_id = ?";
                bind  << id;
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
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    int i = 0;
    foreach (QVariant v, bind) {
        pq->bindValue(i, v);
        ++i;
    }
    fetch();
}

void cHSOperate::fetchSubs()
{
    if (supIdMap.isEmpty()) return;
    QList<QString> ids;
    foreach (int row, supIdMap.keys()) {
        QWidget *pW = pUi->tableWidget->cellWidget(row, TC_CBOX_NSUB);
        if (pW == NULL) EXCEPTION(EPROGFAIL);
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pW);
        if (pCb->isChecked()) ids << QString::number(idList.at(row));
    }
    if (ids.isEmpty()) return;
    QString sql = _sql + " WHERE superior_host_service_id = ";
    if (ids.size() == 0) {
        sql += ids.first();
    }
    else {
        sql += "ANY ('{" + ids.join(QChar(',')) + "}')";
    }
    sql += _ord;
    PDEB(SQL) << "SQL : " << dQuoted(sql) << endl;
    if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
    fetch();
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
    if (!isContIx(history, historyIx)) EXCEPTION(ENOINDEX, historyIx);
    QList<QSqlRecord> recs = history.at(historyIx);
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
            QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CBOX_SEL));
            if (pCb->isChecked()) {
                // pUi->tableWidget->selectRow(row);
                if (!isContIx(recs, row)) EXCEPTION(ENOINDEX, row);
                QSqlRecord& rec = recs[row];
                hs.clear();
                QBitArray um(hs.cols());
                qlonglong id = idList.at(row);
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
                    hs.setId(__sCheckAttempts, 0);
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
    }
    sqlEnd(*pq2, tn);
    history[historyIx] = recs;
    refreshTable(recs);
}

void cHSOperate::setButton()
{
    bool f = !idList.isEmpty()
          && (pUi->checkBoxDisable->isChecked()
           || pUi->checkBoxEnable->isChecked()
           || pUi->checkBoxClrStat->isChecked()
           || pUi->checkBoxAlarmOn->isChecked()
           || pUi->checkBoxAlarmOff->isChecked());
    pUi->pushButtonSet->setEnabled(f);
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
    foreach (int row, supIdMap.keys()) {
        QWidget *pW = pUi->tableWidget->cellWidget(row, TC_CBOX_NSUB);
        if (pW == NULL) EXCEPTION(EPROGFAIL);
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pW);
        pCb->setChecked(true);
    }
//    pUi->pushButtonSub->setEnabled(true);
}

void cHSOperate::subNone()
{
    foreach (int row, supIdMap.keys()) {
        QWidget *pW = pUi->tableWidget->cellWidget(row, TC_CBOX_NSUB);
        if (pW == NULL) EXCEPTION(EPROGFAIL);
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pW);
        pCb->setChecked(false);
    }
//    pUi->pushButtonSub->setDisabled(true);
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
        hs.setById(*pq2, idList.at(row));
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
        pModel->rootId = idList.at(row);
        rt.refresh();
        rt.dialog().exec();
        break;
      }
    case TC_SUPERIOR: {
        cHostService hs;
        hs.setById(*pq2, idList.at(row));
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
    if (history.isEmpty() || historyIx < 0) return;
    if (historyIx > 0) {
        --historyIx;
        refreshTable(history[historyIx]);
        pUi->toolButtonBack->setEnabled(historyIx > 0);
        pUi->toolButtonForward->setEnabled(true);
    }
}

void cHSOperate::forward()
{
    if (history.isEmpty() || historyIx < 0) return;
    int last = history.size() -1;
    if (last > historyIx) {
        ++historyIx;
        refreshTable(history[historyIx]);
        pUi->toolButtonBack->setEnabled(true);
        pUi->toolButtonForward->setEnabled(last > historyIx);
    }
}

void cHSOperate::clear()
{
    if (history.isEmpty() || historyIx < 0) return;
    QList<QSqlRecord> recs = history.takeAt(historyIx);
    history.clear();
    history << recs;
    historyIx = 0;
    pUi->toolButtonBack->setDisabled(true);
    pUi->toolButtonForward->setDisabled(true);
    pUi->toolButtonClear->setDisabled(true);
}
