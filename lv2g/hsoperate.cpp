#include "hsoperate.h"

const enum ePrivilegeLevel cHSOperate::rights = PL_OPERATOR;

enum eFilterButtonId {
    FT_PATTERN, FT_SELECT
};

enum eTableColumnIx {
    TC_HOST_SERVICE,
    TC_PLACE,
    TC_NOALARM,
    TC_FROM,
    TC_TO,
    TC_DISABLED,
    TC_STATE,
    TC_CHECK_BOX,
    TC_COUNT
};


cHSOperate::cHSOperate(QWidget *par)
    : cOwnTab(par)
{
    pq  = newQuery();
    pq2 = newQuery();
    pUi = new Ui_hostServiceOp;
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

    pZoneModel = new cRecordListModel(cPlaceGroup().descr(), this);
    pZoneModel->setConstFilter(_sPlaceGroupType + " = " + _sZone, FT_SQL_WHERE);
    pUi->comboBoxZone->setModel(pZoneModel);
    pZoneModel->setFilter();
    pUi->comboBoxZone->setCurrentText(_sAll);

    connect(pUi->pushButtonClose,   SIGNAL(clicked()),  this,   SLOT(endIt()));
    connect(pUi->pushButtonSet,     SIGNAL(clicked()),  this,   SLOT(set()));
    connect(pUi->pushButtonFetch,   SIGNAL(clicked()),  this,   SLOT(fetch()));
    connect(pUi->pushButtonAll,     SIGNAL(clicked()),  this,   SLOT(all()));
    connect(pUi->pushButtonNone,    SIGNAL(clicked()),  this,   SLOT(none()));

    connect(pUi->lineEditPlacePattern,   SIGNAL(textChanged(QString)), this, SLOT(changePlacePattern(QString)));
    connect(pUi->lineEditNodePattern,    SIGNAL(textChanged(QString)), this, SLOT(changeNodePattern(QString)));
    connect(pUi->lineEditServicePattern, SIGNAL(textChanged(QString)), this, SLOT(changeServicePattern(QString)));

    connect(pUi->checkBoxDisable, SIGNAL(toggled(bool)), this, SLOT(disable(bool)));
    connect(pUi->checkBoxEnable,  SIGNAL(toggled(bool)), this, SLOT(enable(bool)));
    connect(pUi->checkBoxClrStat, SIGNAL(toggled(bool)), this, SLOT(clrStat(bool)));

    pPlaceModel = new cRecordListModel(cPlace().descr(), this);
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


void cHSOperate::fetch()
{
    static const QChar j('%');
    QString sql =
            "SELECT"
                " hs.host_service_id, "                             // ID
                " host_service_id2name(hs.host_service_id) AS hsn, "// TC_HOST_SERVICE
                " p.place_name, "                                   // TC_PLACE
                " hs.noalarm_flag,"                                 // TC_NOALARM
                " hs.noalarm_from,"                                 // TC_FROM
                " hs.noalarm_to,"                                   // TC_TO
                " s.disabled AS s_disabled,"
                " hs.disabled,"                                     // TC_DISABLED
                " hs.host_service_state"                            // TC_STATE
            " FROM host_services AS hs"
            " JOIN nodes  AS n USING(node_id)"
            " JOIN places AS p USING(place_id)"
            " JOIN services AS s USING(service_id)";
    QStringList     where;
    QVariantList    bind;
    bool            empty = false;

    bool idOff    = pUi->checkBoxOff->isChecked();
    bool idOn     = pUi->checkBoxOn->isChecked();
    bool idTo     = pUi->checkBoxTo->isChecked();
    bool idFrom   = pUi->checkBoxFrom->isChecked();
    bool idFromTo = pUi->checkBoxFromTo->isChecked();
    empty = !(idOff || idOn || idTo || idFrom || idFromTo);
    if (!empty && !(idOff && idOn && idTo && idFrom && idFromTo)) {
        if (idOff)  where << "hs.noalarm_flag = 'off' ";
        if (idOn)   where << "hs.noalarm_flag = 'on' ";
        if (idTo)   where << "hs.noalarm_flag = 'to' ";
        if (idFrom) where << "hs.noalarm_flag = 'from' ";
        if (idFrom) where << "hs.noalarm_flag = 'from_to' ";
        QString w = where.join("OR ");
        if (w.size() > 1) w = "(" + w + ") ";
        where.clear();
        where << w;
    }
    // Filter by zone
    QString zone = pUi->comboBoxZone->currentText();
    if (zone != _sAll) {
        int       ci  = pUi->comboBoxZone->currentIndex();
        qlonglong gid = pZoneModel->atId(ci);
        where << " is_group_place(place_id, ?)";
        bind  << gid;
    }
    // Filter by place
    if (!empty && pUi->checkBoxPlace->isChecked()) {
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
            if (id == NULL_ID) empty = true;
            else {
                where << " n.place_id = ?";
                bind  << id;
                where << " is_parent_place(?, n.place_id)";
                bind  << id;
            }
        }
    }
    // Filter by node
    if (!empty && pUi->checkBoxNode->isChecked()) {
        if (pUi->radioButtonNodePattern) {
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
            if (id == NULL_ID) empty = true;
            else {
                where << " ns.node_id = ?";
                bind  << id;
            }
        }
    }
    // Filter by service
    if (!empty && pUi->checkBoxService->isChecked()) {
        if (pUi->radioButtonServicePattern) {
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
            if (id == NULL_ID) empty = true;
            else {
                where << " ns.service_id = ?";
                bind  << id;
            }
        }
    }
    idList.clear();
    if (!empty) {
        cHostService hs;
        if (!where.isEmpty()) sql += " WHERE" + where.join(" AND");
        sql += " ORDER BY hsn ASC";
        if (!pq->prepare(sql)) SQLPREPERR(*pq, sql);
        int i = 0;
        foreach (QVariant v, bind) {
            pq->bindValue(i, v);
            ++i;
        }
        if (!pq->exec()) SQLQUERYERR(*pq);
        if (pq->first()) {
            QVariant v;
            QString s;
            QTableWidgetItem *pi;
            const cColStaticDescr *pCd;
            int row = 0;

            do {
                pUi->tableWidget->setRowCount(row + 1);

                v = pq->value(0);                       // #0: ID
                idList << v.toLongLong();

                v = pq->value(1);                       // #1: TC_HOST_SERVICE
                s = v.toString();
                pi = new QTableWidgetItem(s);
                pUi->tableWidget->setItem(row, TC_HOST_SERVICE, pi);

                v = pq->value(2);                       // #2: TC_PLACE
                s = v.toString();
                pi = new QTableWidgetItem(s);
                pUi->tableWidget->setItem(row, TC_PLACE, pi);

                v = pq->value(3);                       // #3:  TC_NOALARM
                pCd = &hs.colDescr(hs.toIndex(_sNoalarmFlag));
                v = pCd->fromSql(v);
                s = pCd->toView(*pq2, v);
                pi = new QTableWidgetItem(s);
                pUi->tableWidget->setItem(row, TC_NOALARM, pi);

                v = pq->value(4);                       // #4:  TC_FROM
                pCd = &hs.colDescr(hs.toIndex(_sNoalarmFrom));
                v = pCd->fromSql(v);
                s = pCd->toView(*pq2, v);
                pi = new QTableWidgetItem(s);
                pUi->tableWidget->setItem(row, TC_FROM, pi);

                v = pq->value(5);                       // #5:  TC_TO
                pCd = &hs.colDescr(hs.toIndex(_sNoalarmTo));
                v = pCd->fromSql(v);
                s = pCd->toView(*pq2, v);
                pi = new QTableWidgetItem(s);
                pUi->tableWidget->setItem(row, TC_TO, pi);

                v = pq->value(6);                       // #6
                pCd = &hs.colDescr(hs.toIndex(_sDisabled));
                v = pCd->fromSql(v);
                bool sd = v.toBool();
                v = pq->value(7);                       // #7:  TC_DISABLED
                v = pCd->fromSql(v);
                s = pCd->toView(*pq2, v);
                pi = new QTableWidgetItem(s);
                if (sd) {
                    pi->setTextColor(QColor(Qt::red));
                    pi->setToolTip(trUtf8("Disabled by service."));
                }
                pUi->tableWidget->setItem(row, TC_DISABLED, pi);

                v = pq->value(8);                       // #8:  TC_STATE
                pCd = &hs.colDescr(hs.toIndex(_sHardState));
                v = pCd->fromSql(v);
                s = pCd->toView(*pq2, v);
                pi = new QTableWidgetItem(s);
                pUi->tableWidget->setItem(row, TC_STATE, pi);

                QCheckBox *pCb = new QCheckBox;
                pCb->setChecked(true);
                pi = new QTableWidgetItem();
                pUi->tableWidget->setCellWidget(row, TC_CHECK_BOX, pCb);

                row++;
            } while (pq->next());
            pUi->tableWidget->resizeColumnsToContents();
            pUi->pushButtonAll->setDisabled(false);
            pUi->pushButtonNone->setDisabled(false);
            setButton();
            return;
        }
        // empty
    }
    // EMPTY
    pUi->tableWidget->setRowCount(0);
    pUi->pushButtonAll->setDisabled(true);
    pUi->pushButtonNone->setDisabled(true);
    pUi->pushButtonSet->setDisabled(true);
}

void cHSOperate::set()
{
    static const QString tn = "HSOparateSet";
    pUi->pushButtonSet->setDisabled(true);
    int rows = pUi->tableWidget->rowCount();
    cHostService hs;
    QBitArray um_disabled = hs.mask(_sDisabled);
    QStringList csf;
    csf << _sHostServiceState << _sSoftState << _sHardState << _sStateMsg << _sCheckAttempts
        << _sLastChanged  << _sLastTouched << _sActAlarmLogId;
    QBitArray um_ClrState = hs.mask(csf);
    sqlBegin(*pq2, tn);
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CHECK_BOX));
        if (pCb->isChecked()) {
            pUi->tableWidget->selectRow(row);
            hs.clear();
            QBitArray um(hs.cols());
            qlonglong id = idList.at(row);
            if (pUi->checkBoxDisable->isChecked()) {
                hs.setBool(_sDisabled, true);
                um = um_disabled;
            }
            else if (pUi->checkBoxEnable->isChecked()) {
                hs.setBool(_sDisabled, false);
                um = um_disabled;
            }
            if (pUi->checkBoxClrStat->isChecked()) {
                hs.setId(_sHostServiceState, RS_UNKNOWN);
                hs.setId(_sSoftState, RS_UNKNOWN);
                hs.setId(_sHardState, RS_UNKNOWN);
                hs.setId(__sCheckAttempts, 0);
                um |= um_ClrState;
            }
            if (um.count(true) > 0) {
                hs.setId(id);
                if (!cErrorMessageBox::condMsgBox(hs.tryUpdate(*pq2, false, um), this)) {
                    sqlRollback(*pq2, tn);
                    return;
                }
            }
            cError *pe = NULL;
            try {
                if (pUi->checkBoxStatLog->isChecked()) {
                    static const QString sql = "DELETE FROM host_service_logs WHERE host_service_id = ?";
                    execSql(*pq2, sql, id);
                }
                if (pUi->checkBoxAlarm->isChecked()) {
                    static const QString sql = "DELETE FROM alarms WHERE host_service_id = ?";
                    execSql(*pq2, sql, id);
                }
                if (pUi->checkBoxMemo->isChecked()) {
                    static const QString sql = "DELETE FROM app_memos WHERE host_service_id = ?";
                    execSql(*pq2, sql, id);
                }
            } CATCHS(pe)
            if (pe != NULL) {
                sqlRollback(*pq2, tn);
                cErrorMessageBox::messageBox(pe, this);
                return;
            }
        }
    }
    sqlEnd(*pq2, tn);
    fetch();
}

void cHSOperate::setButton()
{
    bool f = !idList.isEmpty()
          && (pUi->checkBoxDisable->isChecked()
           || pUi->checkBoxEnable->isChecked()
           || pUi->checkBoxClrStat->isChecked());
    pUi->pushButtonSet->setEnabled(f);
}

void cHSOperate::all()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CHECK_BOX));
        pCb->setChecked(true);
    }
}

void cHSOperate::none()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CHECK_BOX));
        pCb->setChecked(false);
    }
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

