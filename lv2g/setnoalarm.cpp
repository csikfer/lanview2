#include "setnoalarm.h"

const enum ePrivilegeLevel cSetNoAlarm::rights = PL_OPERATOR;

enum eFilterButtonId {
    FT_PATTERN, FT_SELECT
};

enum eTableColumnIx {
    TC_HOST_SERVICE,
    TC_PLACE,
    TC_NOALARM,
    TC_FROM,
    TC_TO,
    TC_CHECK_BOX,
    TC_COUNT
};

#define MSG_MIN_SIZE 8

cSetNoAlarm::cSetNoAlarm(QWidget *par)
    : cOwnTab(par)
{
    pq  = newQuery();
    pq2 = newQuery();
    pUi = new Ui_setNoAlarm;
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

    pButtonGroupType   = new QButtonGroup(this);
    pButtonGroupType->addButton(pUi->radioButtonOff,    NAT_OFF);
    pButtonGroupType->addButton(pUi->radioButtonOn,     NAT_ON);
    pButtonGroupType->addButton(pUi->radioButtonTo,     NAT_TO);
    pButtonGroupType->addButton(pUi->radioButtonFrom,   NAT_FROM);
    pButtonGroupType->addButton(pUi->radioButtonFromTo, NAT_FROM_TO);

    pUi->dateTimeEditFrom->setMinimumDate(QDate::currentDate());
    pUi->dateTimeEditTo->setMinimumDate(QDate::currentDate());

    pZoneModel = new cRecordListModel(cPlaceGroup().descr(), this);
    pZoneModel->setConstFilter(_sPlaceGroupType + " = " + quoted(_sZone), FT_SQL_WHERE);
    pUi->comboBoxZone->setModel(pZoneModel);
    pZoneModel->setFilter();
    pUi->comboBoxZone->setCurrentText(_sAll);

    connect(pUi->pushButtonClose,   SIGNAL(clicked()),  this,   SLOT(endIt()));
    connect(pUi->pushButtonSet,     SIGNAL(clicked()),  this,   SLOT(set()));
    connect(pUi->pushButtonFetch,   SIGNAL(clicked()),  this,   SLOT(fetch()));
    connect(pUi->pushButtonAll,     SIGNAL(clicked()),  this,   SLOT(all()));
    connect(pUi->pushButtonNone,    SIGNAL(clicked()),  this,   SLOT(none()));

    connect(pUi->radioButtonOff,    SIGNAL(toggled(bool)), this,SLOT(off(bool)));
    connect(pUi->radioButtonOn,     SIGNAL(toggled(bool)), this,SLOT(on(bool)));
    connect(pUi->radioButtonTo,     SIGNAL(toggled(bool)), this,SLOT(to(bool)));
    connect(pUi->radioButtonFrom,   SIGNAL(toggled(bool)), this,SLOT(from(bool)));
    connect(pUi->radioButtonFromTo, SIGNAL(toggled(bool)), this,SLOT(fromTo(bool)));

    connect(pUi->lineEditPlacePattern,   SIGNAL(textChanged(QString)), this, SLOT(changePlacePattern(QString)));
    connect(pUi->lineEditNodePattern,    SIGNAL(textChanged(QString)), this, SLOT(changeNodePattern(QString)));
    connect(pUi->lineEditServicePattern, SIGNAL(textChanged(QString)), this, SLOT(changeServicePattern(QString)));
    connect(pUi->textEditMsg,            SIGNAL(textChanged()),        this, SLOT(changeMsg()));

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

cSetNoAlarm::~cSetNoAlarm()
{
    delete pq;
    delete pq2;
}


void cSetNoAlarm::fetch()
{
    static const QChar j('%');
    QString sql =
            "SELECT"
                " hs.host_service_id, "                             // ID
                " host_service_id2name(hs.host_service_id) as hsn, "// TC_HOST_SERVICE
                " p.place_name, "                                   // TC_PLACE
                " hs.noalarm_flag,"                                 // TC_NOALARM
                " hs.noalarm_from,"                                 // TC_FROM
                " hs.noalarm_to"                                    // TC_TO
            " FROM host_services AS hs"
            " JOIN nodes  AS n USING(node_id)"
            " JOIN places AS p USING(place_id)";
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
                sql +=  " JOIN services AS s USING(service_id)";
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

                QCheckBox *pCb = new QCheckBox;
                pCb->setChecked(true);
                pi = new QTableWidgetItem();
                pUi->tableWidget->setCellWidget(row, TC_CHECK_BOX, pCb);

                row++;
            } while (pq->next());
            pUi->tableWidget->resizeColumnsToContents();
            pUi->pushButtonAll->setDisabled(false);
            pUi->pushButtonNone->setDisabled(false);
            pUi->pushButtonSet->setEnabled(pUi->textEditMsg->toPlainText().size() > MSG_MIN_SIZE);
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

void cSetNoAlarm::set()
{
    static const QString tn = "setNoAlarmSet";
    pUi->pushButtonSet->setDisabled(true);
    int rows = pUi->tableWidget->rowCount();
    int nat  = pButtonGroupType->checkedId();
    if (noalarmtype(nat, EX_IGNORE).isEmpty()) return;
    cHostService hs;
    QBitArray um = hs.mask(_sNoalarmFlag, _sNoalarmFrom, _sNoalarmTo, _sLastNoalarmMsg);
    sqlBegin(*pq2, tn);
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CHECK_BOX));
        if (pCb->isChecked()) {
            pUi->tableWidget->selectRow(row);
            hs.clear();
            qlonglong id = idList.at(row);
            hs.setId(id);
            hs.setId(_sNoalarmFlag, nat);
            if (nat == NAT_FROM || nat == NAT_FROM_TO) hs.set(_sNoalarmFrom, pUi->dateTimeEditFrom->dateTime());
            if (nat == NAT_TO   || nat == NAT_FROM_TO) hs.set(_sNoalarmTo,   pUi->dateTimeEditTo->dateTime());
            if (!cErrorMessageBox::condMsgBox(hs.tryUpdate(*pq2, false, um), this)) {
                sqlRollback(*pq2, tn);
                return;
            }
        }
    }
    sqlEnd(*pq2, tn);
    fetch();
}

void cSetNoAlarm::all()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CHECK_BOX));
        pCb->setChecked(true);
    }
}

void cSetNoAlarm::none()
{
    int rows = pUi->tableWidget->rowCount();
    for (int row = 0; row < rows; ++row) {
        QCheckBox *pCb = dynamic_cast<QCheckBox *>(pUi->tableWidget->cellWidget(row, TC_CHECK_BOX));
        pCb->setChecked(false);
    }

}

void cSetNoAlarm::off(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(false);
    pUi->dateTimeEditTo->setEnabled(false);
}

void cSetNoAlarm::on(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(false);
    pUi->dateTimeEditTo->setEnabled(false);
}

void cSetNoAlarm::to(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(false);
    pUi->dateTimeEditTo->setEnabled(true);
}

void cSetNoAlarm::from(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(true);
    pUi->dateTimeEditTo->setEnabled(false);
}

void cSetNoAlarm::fromTo(bool f)
{
    if (!f) return;
    pUi->dateTimeEditFrom->setEnabled(true);
    pUi->dateTimeEditTo->setEnabled(true);
}

void cSetNoAlarm::changePlacePattern(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pPlaceModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxPlaceSelect->setCurrentIndex(0);
}

void cSetNoAlarm::changeNodePattern(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pNodeModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxNodeSelect->setCurrentIndex(0);
}

void cSetNoAlarm::changeServicePattern(const QString& text)
{
    enum eFilterType ft = FT_LIKE;
    if      (text.isEmpty())               ft = FT_NO;
    else if (text.indexOf(QChar('%')) < 0) ft = FT_BEGIN;
    pServiceModel->setFilter(text, OT_DEFAULT, ft);
    pUi->comboBoxServiceSelect->setCurrentIndex(0);
}

void cSetNoAlarm::changeMsg()
{
    pUi->pushButtonSet->setEnabled(pUi->textEditMsg->toPlainText().size() > MSG_MIN_SIZE);
}
