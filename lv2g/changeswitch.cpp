#include "changeswitch.h"

template <class P>  QString paramToString(const P *p) {
    return QString("%1 = %2").arg(p->getName(), p->getName(_sParamValue));
}

static void setChecked(QListWidget *p, Qt::CheckState f)
{
    int i, n = p->count();
    for (i = 0; i < n; ++i) {
        QListWidgetItem *pi = p->item(i);
        if (pi == nullptr) EXCEPTION(EPROGFAIL,i, p->objectName());
        pi->setCheckState(f);
    }
}

static bool isChecked(QListWidget *p, int ix)
{
    if (p == nullptr) return false;
    QListWidgetItem *pi = p->item(ix);
    if (pi == nullptr) return false;
    return pi->checkState() == Qt::Checked;
}

template <class P> QListWidget * paramsWidget(const QList<P *>& params, QListWidget *_cbl = nullptr)
{
    QString s;
    QListWidget * checkBoxList = _cbl;
    if (checkBoxList != nullptr) checkBoxList->clear();

    if (!params.isEmpty()) {
        if (checkBoxList == nullptr) {
            checkBoxList = new QListWidget;
            checkBoxList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        }
        int n = params.size();
        for (int i = 0; i < n; ++i) {
            s = paramToString(params.at(i));
            QListWidgetItem *pi = new QListWidgetItem(s, checkBoxList);
            pi->setFlags(pi->flags() | Qt::ItemIsUserCheckable);
            pi->setCheckState(Qt::Checked);
        }
    }
    return checkBoxList;
}

static QListWidget * servicesWidget(QSqlQuery& q, qlonglong nid, qlonglong pid, QList<qlonglong>& idList, QListWidget *_cbl = nullptr)
{
    static const QString sql =
            "SELECT host_service_id, srv.service_name || '(' || pri.service_name || ':' || pro.service_name || ')' AS name"
            " FROM host_services AS hs"
            " JOIN services AS srv ON hs.service_id    = srv.service_id"
            " JOIN services AS pri ON prime_service_id = pri.service_id"
            " JOIN services AS pro ON proto_service_id = pro.service_id"
            " WHERE node_id = ? AND port_id = ?"
            " ORDER BY srv.service_name ASC";
    QListWidget * checkBoxList = _cbl;
    if (checkBoxList != nullptr) checkBoxList->clear();
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, nid);
    q.bindValue(1, pid == NULL_ID ? QVariant() : QVariant(pid));
    if (!q.exec()) SQLQUERYERR(q);
    if (q.first()) {
       if (checkBoxList == nullptr) {
           checkBoxList = new QListWidget;
           checkBoxList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
       }
       do {
           idList << q.value(0).toLongLong();
           QString s = q.value(1).toString();
           QListWidgetItem *pi = new QListWidgetItem(s, checkBoxList);
           pi->setFlags(pi->flags() | Qt::ItemIsUserCheckable);
           pi->setCheckState(Qt::Checked);
       } while (q.next());
    }
    return checkBoxList;
}

static QListWidget * oneCheckBox(const QString& s)
{
    QListWidget * checkBoxList = new QListWidget;
    checkBoxList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    QListWidgetItem *pi = new QListWidgetItem(s, checkBoxList);
    pi->setFlags(pi->flags() | Qt::ItemIsUserCheckable);
    pi->setCheckState(Qt::Checked);
    return checkBoxList;
}

static cPhsLink * link(QSqlQuery& q, cNPort *pp)
{
    cPhsLink * pl = new cPhsLink;
    switch (pl->isExist(q, pp->getId(), LT_TERM)) {
    case 1:
        break;
    case 0:
        pDelete(pl);
        break;
    default:
        EXCEPTION(EDATA, pp->getId(), pp->identifying());
    }
    return pl;
}

static QString linkToString(QSqlQuery& q, const cPhsLink * _pl)
{
    QString s, ss;
    s = cNPort().getFullNameById(q, _pl->getId(_sPortId2));
    ss = _pl->getName(_sPortShared);
    if (!ss.isEmpty()) s += "/" + ss;
    return s;
}
/* ************************************************************** */

void cCSRow::setItem(const QString& txt, int col)
{
    QTableWidgetItem *pi;
    pi = new QTableWidgetItem(txt);
    pTableWidget->setItem(row, col, pi);
}

void cCSRow::setItem(qlonglong n, int col)
{
    QTableWidgetItem *pi;
    pi = new QTableWidgetItem(QString::number(n));
    pi->setTextAlignment(Qt::AlignRight | Qt::AlignCenter);
    pTableWidget->setItem(row, col, pi);
}

cCSRow::cCSRow(QSqlQuery& q, cChangeSwitch *par, int _r, cNPort *_ps, cNPort *_pt)
    : QObject (par)
{
    QString s;
    checkBoxDelLnk = checkBoxDelPar = checkBoxDelSrv = nullptr;
    checkBoxCpyLnk = checkBoxCpyPar = checkBoxCpySrv = nullptr;
    pLnkDel = pLnkCpy = nullptr;
    pParent = par;
    pTableWidget = par->pUi->tableWidget;
    row = _r;
    pSrc = _ps;
    pTrg = _pt;
    pTableWidget->setRowCount(row +1);

    if (pSrc != nullptr) {
        setItem(pSrc->getId(_sPortIndex),  CIX_INDEX_SRC);
        setItem(pSrc->getName(),           CIX_PORT_SRC);
        // port param(s)
        if (!pSrc->params.isEmpty()) {
            checkBoxCpyPar = paramsWidget(pSrc->params);
            pTableWidget->setCellWidget(row, CIX_CPY_PAR, checkBoxCpyPar);
        }
        // link
        pLnkCpy = link(q, pSrc);
        if (pLnkCpy != nullptr) {
            s = linkToString(q, pLnkCpy);
            checkBoxCpyLnk = oneCheckBox(s);
            pTableWidget->setCellWidget(row, CIX_CPY_LNK, checkBoxCpyLnk);
        }
        // servce(s)
        checkBoxCpySrv = servicesWidget(q, pParent->nidSrc, pSrc->getId(), hsIdCpyList);
        if (checkBoxCpySrv != nullptr) {
            pTableWidget->setCellWidget(row, CIX_CPY_SRV, checkBoxCpySrv);
        }
    }
    if (pTrg != nullptr) {
        setItem(pTrg->getId(_sPortIndex), CIX_INDEX_TRG);
        setItem(pTrg->getName(),          CIX_PORT_TRG);
        // port param(s)
        if (!pTrg->params.isEmpty()) {
            checkBoxDelPar = paramsWidget(pTrg->params);
            pTableWidget->setCellWidget(row, CIX_DEL_PAR, checkBoxDelPar);
        }
        // link
        pLnkDel = link(q, pTrg);
        if (pLnkDel != nullptr) {
            s = linkToString(q, pLnkDel);
            checkBoxDelLnk = oneCheckBox(s);
            pTableWidget->setCellWidget(row,  CIX_DEL_LNK, checkBoxDelLnk);
        }
        // service(s)
        checkBoxDelSrv = servicesWidget(q, pParent->nidTrg, pTrg->getId(), hsIdDelList);
        if (checkBoxDelSrv != nullptr) {
            pTableWidget->setCellWidget(row, CIX_DEL_SRV, checkBoxDelSrv);
        }
    }
    if (checkBoxCpyLnk != nullptr && checkBoxDelLnk != nullptr) {
        connect(checkBoxCpyLnk, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(on_checkBoxCpyLink_chenged(QListWidgetItem *)));
        connect(checkBoxDelLnk, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(on_checkBoxDelLink_changed(QListWidgetItem *)));
    }
}

cCSRow::~cCSRow()
{

}

void cCSRow::setChecked(int col, bool _f)
{
    Qt::CheckState f = _f ? Qt::Checked : Qt::Unchecked;
    QListWidget *plw = nullptr;
    switch (col) {
    case cCSRow::CIX_CPY_LNK:   plw = checkBoxCpyLnk;   break;
    case cCSRow::CIX_CPY_SRV:   plw = checkBoxCpySrv;   break;
    case cCSRow::CIX_CPY_PAR:   plw = checkBoxCpyPar;   break;
    case cCSRow::CIX_DEL_LNK:   plw = checkBoxDelLnk;   break;
    case cCSRow::CIX_DEL_SRV:   plw = checkBoxDelSrv;   break;
    case cCSRow::CIX_DEL_PAR:   plw = checkBoxDelPar;   break;
    }
    if (plw != nullptr) {
        ::setChecked(plw, f);
    }
}

// Run under cCSSaver thread!
void cCSRow::save(QSqlQuery& q)
{
    int i, n;
//    QTableWidgetItem *pi;
    if (pTrg != nullptr) {
        // Delete Port param(s)
        n = pTrg->params.size();
        for (i = 0; i < n; ++i) {
            if (isChecked(checkBoxDelPar, i)) {
                setCheckBoxColor(CIX_DEL_PAR, i, ET_FATAL);
                switch(pTrg->params.at(i)->remove(q, false, QBitArray(), EX_ERROR)) {
                case 0:  setCheckBoxColor(CIX_DEL_PAR, i, ET_WARNING); break;
                case 1:  setCheckBoxColor(CIX_DEL_PAR, i, ET_OK);      break;
                default: setCheckBoxColor(CIX_DEL_PAR, i, ET_ERROR);   break;
                }
            }
        }
        // Delete service(s)
        n = hsIdDelList.size();
        for (i = 0; i < n; ++i) {
            if (isChecked(checkBoxDelSrv, i)) {
                setCheckBoxColor(CIX_DEL_SRV, i, ET_FATAL);
                cHostService hs;
                hs._toReadBack = RB_NO;
                switch(hs.removeById(q, hsIdDelList.at(i))) {
                case 0:  setCheckBoxColor(CIX_DEL_SRV, i, ET_WARNING); break;
                case 1:  setCheckBoxColor(CIX_DEL_SRV, i, ET_OK);      break;
                default: setCheckBoxColor(CIX_DEL_SRV, i, ET_ERROR);   break;
                }
            }
        }
        // Delet link
        if (pLnkDel != nullptr) {
            if (checkBoxDelLnk == nullptr) EXCEPTION(EPROGFAIL);
            if (isChecked(checkBoxDelLnk, 0)) {
                setCheckBoxColor(CIX_DEL_LNK, 0, ET_FATAL);
                pLnkDel->_toReadBack = RB_NO;
                switch(pLnkDel->remove(q, false, QBitArray(), EX_ERROR)) {
                case 0:  setCheckBoxColor(CIX_DEL_LNK, 0, ET_WARNING); break;
                case 1:  setCheckBoxColor(CIX_DEL_LNK, 0, ET_OK);      break;
                default: setCheckBoxColor(CIX_DEL_LNK, 0, ET_ERROR);   break;
                }
            }
        }
        if (pSrc != nullptr) {
            qlonglong pid = pTrg->getId();
            qlonglong nid = pTrg->getId(_sNodeId);
            // Move port param(s)
            n = pSrc->params.size();
            for (i = 0; i < n; ++i) {
                if (isChecked(checkBoxCpyPar, i)) {
                    setCheckBoxColor(CIX_CPY_PAR, i, ET_FATAL);
                    cPortParam& pp = *pSrc->params.at(i);
                    pp._toReadBack = RB_NO;
                    pp.setId(_sPortId, pid);
                    switch(pp.update(q, false, pp.mask(_sPortId), QBitArray(), EX_ERROR)) {
                    case 0:  setCheckBoxColor(CIX_CPY_PAR, i, ET_WARNING); break;
                    case 1:  setCheckBoxColor(CIX_CPY_PAR, i, ET_OK);      break;
                    default: setCheckBoxColor(CIX_CPY_PAR, i, ET_ERROR);   break;
                    }
                }
            }
            // Move port service(s)
            n = hsIdCpyList.size();
            for (i = 0; i < n; ++i) {
                if (isChecked(checkBoxCpySrv, i)) {
                    setCheckBoxColor(CIX_CPY_SRV, i, ET_FATAL);
                    cHostService hs;
                    hs._toReadBack = RB_NO;
                    hs.setId(hsIdCpyList.at(i));
                    hs.setId(_sNodeId, nid);
                    hs.setId(_sPortId, pid);
                    switch(hs.update(q, false, hs.mask(_sPortId, _sNodeId), QBitArray(), EX_ERROR)) {
                    case 0:  setCheckBoxColor(CIX_CPY_SRV, i, ET_WARNING); break;
                    case 1:  setCheckBoxColor(CIX_CPY_SRV, i, ET_OK);      break;
                    default: setCheckBoxColor(CIX_CPY_SRV, i, ET_ERROR);   break;
                    }
                }
            }
            // Move link
            if (pLnkCpy != nullptr) {
                if (checkBoxCpyLnk == nullptr) EXCEPTION(EPROGFAIL);
                if (isChecked(checkBoxCpyLnk, 0)) {
                    setCheckBoxColor(CIX_CPY_LNK, 0, ET_FATAL);
                    pLnkCpy->_toReadBack = RB_NO;
                    pLnkCpy->setId(_sPortId1, pid);
                    switch(pLnkCpy->update(q, false, pLnkCpy->mask(_sPortId1), QBitArray(), EX_ERROR)) {
                    case 0:  setCheckBoxColor(CIX_CPY_LNK, 0, ET_WARNING); break;
                    case 1:  setCheckBoxColor(CIX_CPY_LNK, 0, ET_OK);      break;
                    default: setCheckBoxColor(CIX_CPY_LNK, 0, ET_ERROR);   break;
                    }
                }
            }
        }
    }
}

void cCSRow::on_checkBoxCpyLnk_changed(QListWidgetItem *_pi)
{
    if (_pi != checkBoxCpyLnk->item(0)) EXCEPTION(EPROGFAIL);
    bool f = _pi->checkState() == Qt::Checked;
    if (f && checkBoxDelLnk != nullptr) {
        QListWidgetItem *pi = checkBoxDelLnk->item(0);
        if (pi == nullptr) {
            EXCEPTION(EPROGFAIL);
        }
        pi->setCheckState(Qt::Checked);
    }
}

void cCSRow::on_checkBoxDelLnk_changed(QListWidgetItem *_pi)
{
    if (_pi != checkBoxDelLnk->item(0)) EXCEPTION(EPROGFAIL);
    bool f = _pi->checkState() == Qt::Unchecked;
    if (f && checkBoxCpyLnk != nullptr) {
        QListWidgetItem *pi = checkBoxCpyLnk->item(0);
        if (pi == nullptr) {
            EXCEPTION(EPROGFAIL);
        }
        pi->setCheckState(Qt::Unchecked);
    }
}

void cCSRow::setCheckBoxColor(int cix, int ix, int col)
{
    emit pParent->doSetTableCellColor(row, cix, ix, col);
}

/* ************************************************************** */

cCSSaver::cCSSaver(cChangeSwitch *par)
    : QThread ()
    , pParent(par)
    , pe(nullptr)
{
    ;
}

cCSSaver::~cCSSaver()
{
    pDelete(pe);
}

void cCSSaver::run()
{
    static const QString tn = "changeswitch";
    QSqlQuery q = getQuery();
    try {
        sqlBegin(q, tn);
        pParent->save(q);
        sqlCommit(q, tn);
    } CATCHS(pe);
    if (pe != nullptr) {
        sqlRollback(q, tn, EX_IGNORE);
    }
}

/* ************************************************************** */

const enum ePrivilegeLevel cChangeSwitch::rights = PL_ADMIN;
const QString cChangeSwitch::sIxNote = QObject::trUtf8("A művelet csak akkor végezhető el, ha a portoknak van indexe.\n");
const QString cChangeSwitch::iconUnchecked = "://icons/close.ico";
const QString cChangeSwitch::iconChecked   = "://icons/ok.ico";

bool cChangeSwitch::setHeaderIcon(int col)
{
    bool r = headerIconState[col] = !headerIconState.at(col);
    QIcon icon(r ? iconUnchecked : iconChecked);
    QTableWidgetItem *pi = pUi->tableWidget->horizontalHeaderItem(col);
    pi->setIcon(icon);
    return  r;
}

bool cChangeSwitch::setListIcon(enum eList e)
{
    bool r = listIconState[e] = !listIconState.at(e);
    QIcon icon(r ? iconUnchecked : iconChecked);
    switch (e) {
    case EL_CPY_SRV:
        pUi->toolButtonCpySrv->setIcon(icon);
        break;
    case EL_CPY_PAR:
        pUi->toolButtonCpyPar->setIcon(icon);
        break;
    case EL_DEL_SRV:
        pUi->toolButtonDelSrv->setIcon(icon);
        break;
    case EL_DEL_PAR:
        pUi->toolButtonDelPar->setIcon(icon);
        break;
    default:
        EXCEPTION(EPROGFAIL);
    }
    return r;
}

cChangeSwitch::cChangeSwitch(QMdiArea * par)
    : cIntSubObj (par)
    , state(ES_INSUFF)
    , pUi(new Ui::changeSwitch)
    , pq(newQuery())
    , pSelNodeSrc(nullptr)
    , pSelNodeTrg(nullptr)
    , offsSrc(0)
    , offsTrg(0)
    , nidSrc(NULL_ID)
    , nidTrg(NULL_ID)
    , rows()
    , pNodeSrc(nullptr)
    , pNodeTrg(nullptr)
    , srvCpyIdList()
    , srvDelIdList()
    , headerIconState(cCSRow::COLUMN_NUMBER, false)
    , listIconState(LIST_NUMBER, false)
    , pSaver(nullptr)
{
    pUi->setupUi(this);

    static const QString sqlFilt = "'switch' = ANY (node_type)";
    pSelNodeSrc = new cSelectNode(
                pUi->comboBoxZoneSrc, pUi->comboBoxPlaceSrc, pUi->comboBoxSwitchSrc,
                pUi->lineEditPlacePatternSrc, pUi->lineEditSwitchPatternSrc,
                _sNul, sqlFilt, this);
    pSelNodeSrc->refresh();
    pSelNodeTrg = new cSelectNode(
                pUi->comboBoxZoneTrg, pUi->comboBoxPlaceTrg, pUi->comboBoxSwitchTrg,
                pUi->lineEditPlacePatternTrg, pUi->lineEditSwitchPatternTrg,
                _sNul, sqlFilt, this);
    pSelNodeTrg->refresh();

    QList<int> sizes;
    sizes << 600 << 100 << 100;
    pUi->splitterMain->setSizes(sizes);
    QStringList headers;
    headers << trUtf8("Ix.");           // CIX_INDEX_SRC
    headers << trUtf8("Régi port");     // CIX_PORT_SRC
    headers << trUtf8("Másol: param."); // CIX_CPY_PARAM
    headers << trUtf8("Másol: link");   // CIX_CPY_LINKED
    headers << trUtf8("Másol: Szolg."); // CIX_CPY SERVICES
    headers << trUtf8("Ix.");           // CIX_INDEX_TRG
    headers << trUtf8("Új port");       // CIX_PORT_TRG
    headers << trUtf8("Töröl: param."); // CIX_DEL_PARAM
    headers << trUtf8("Töröl: link");   // CIX_DEL_LINK
    headers << trUtf8("Töröl: Szolg."); // CIX_DEL SERVICES
    pUi->tableWidget->setColumnCount(cCSRow::COLUMN_NUMBER);
    pUi->tableWidget->setHorizontalHeaderLabels(headers);
    setHeaderIcon(cCSRow::CIX_CPY_PAR);
    setHeaderIcon(cCSRow::CIX_CPY_LNK);
    setHeaderIcon(cCSRow::CIX_CPY_SRV);
    setHeaderIcon(cCSRow::CIX_DEL_PAR);
    setHeaderIcon(cCSRow::CIX_DEL_LNK);
    setHeaderIcon(cCSRow::CIX_DEL_SRV);
    setListIcon(EL_CPY_SRV);
    setListIcon(EL_CPY_PAR);
    setListIcon(EL_DEL_SRV);
    setListIcon(EL_DEL_PAR);

    connect(pSelNodeSrc, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(on_selNodeSrc_nodeIdChanged(qlonglong)));
    connect(pSelNodeTrg, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(on_selNodeTrg_nodeIdChanged(qlonglong)));
    QHeaderView * phh = pUi->tableWidget->horizontalHeader();
    connect(phh, SIGNAL(sectionClicked(int)), this, SLOT(on_tableWidgetHHeader_clicked(int)));

    connect(this, SIGNAL(doSetTableCellColor(int,int,int,int)), this, SLOT(setTableCellColor(int,int,int,int)), Qt::QueuedConnection);
    connect(this, SIGNAL(doSetListCellColor(int,int,int)), this, SLOT(setListCellColor(int,int,int)), Qt::QueuedConnection);
}

cChangeSwitch::~cChangeSwitch()
{
    clear();
    pDelete(pNodeSrc);
    pDelete(pNodeTrg);
    delete pq;
}

void cChangeSwitch::setButtons(bool chk)
{
    if (chk) {
        if (pNodeSrc == nullptr || pNodeTrg == nullptr) {
            state = ES_INSUFF;
        }
        else if (pNodeSrc->getId() == pNodeTrg->getId()) {
            QMessageBox::warning(this, dcViewShort(DC_WARNING), trUtf8("Ugyan azt az eszközt választotta ki forrás és cél eszköznek."));
            state = ES_INSUFF;
        }
        else if (state == ES_INSUFF) {
            state = ES_SRC_READY;
        }
        if (state == ES_INSUFF && !rows.isEmpty()) {
            clear();
        }
    }
    switch (state) {
    case ES_INSUFF:
    case ES_ERROR:
    case ES_RUN_SAVER:
        pUi->pushButtonCopy->setDisabled(true);
        pUi->pushButtonRefresh->setDisabled(true);
        break;
    case ES_SRC_READY:
        pUi->pushButtonCopy->setDisabled(true);
        pUi->pushButtonRefresh->setDisabled(false);
        break;
    case ES_DATA_READY:
        pUi->pushButtonCopy->setDisabled(false);
        pUi->pushButtonRefresh->setDisabled(true);
        break;
    }
    bool ready = state == ES_DATA_READY;
    pUi->toolButtonCpySrv->setEnabled(ready || pUi->listWidgetCpySrv->count() > 0);
    pUi->toolButtonCpyPar->setEnabled(ready || pUi->listWidgetCpyPar->count() > 0);
    pUi->toolButtonDelSrv->setEnabled(ready || pUi->listWidgetDelSrv->count() > 0);
    pUi->toolButtonDelPar->setEnabled(ready || pUi->listWidgetDelPar->count() > 0);
    bool run = state == ES_RUN_SAVER;
    pSelNodeSrc->setDisabled(run);
    pSelNodeTrg->setDisabled(run);
    pUi->spinBoxOffsetSrc->setDisabled(run);
    pUi->spinBoxOffsetTrg->setDisabled(run);
}

void cChangeSwitch::clear()
{
    while (!rows.isEmpty()) {
        delete rows.takeFirst();
    }
    pUi->tableWidget->setRowCount(0);
    pUi->listWidgetCpyPar->clear();
    pUi->listWidgetCpySrv->clear();
    pUi->listWidgetDelPar->clear();
    pUi->listWidgetDelSrv->clear();
    srvCpyIdList.clear();
    srvDelIdList.clear();
}

void cChangeSwitch::refreshTables()
{
    if (state != ES_SRC_READY) return;
    int row, ix, ixSrc, ixTrg, off, maxIx;
    clear();
    if (pNodeSrc == nullptr || pNodeTrg == nullptr) EXCEPTION(EPROGFAIL);

    off = offsTrg - offsSrc;
    ix    = std::min(minIxSrc, minIxTrg + off);
    maxIx = std::min(maxIxSrc, maxIxTrg + off);
    for (row = 0; ix <= maxIx ; ++ix) {
        ixSrc = ix;
        ixTrg = ix - off;
        cNPort *pOld = pNodeSrc->ports.get(_sPortIndex, ixSrc, EX_IGNORE);
        cNPort *pNew = pNodeTrg->ports.get(_sPortIndex, ixTrg, EX_IGNORE);
        if (pOld != nullptr || pNew != nullptr) {
            rows << new cCSRow(*pq, this, row, pOld, pNew);
            ++row;
        }
    }
    pUi->tableWidget->resizeColumnsToContents();
    pUi->tableWidget->resizeRowsToContents();
    (void)servicesWidget(*pq, nidSrc, NULL_ID, srvCpyIdList, pUi->listWidgetCpySrv);
    (void)servicesWidget(*pq, nidTrg, NULL_ID, srvDelIdList, pUi->listWidgetDelSrv);
    (void)paramsWidget(pNodeSrc->params, pUi->listWidgetCpyPar);
    (void)paramsWidget(pNodeTrg->params, pUi->listWidgetDelPar);
    state = ES_DATA_READY;
    setButtons(false);
    return;
}

// Run in cCSSaver thread!
void cChangeSwitch::save(QSqlQuery& q)
{
    int i, n;
    QListWidgetItem *pi;
    // Del assigned node param(s)
    n = pNodeTrg->params.size();
    for (i = 0; i < n; ++i) {
        pi = pUi->listWidgetDelPar->item(i);
        if (pi == nullptr) EXCEPTION(EPROGFAIL);
        if (pi->checkState() == Qt::Checked) {
            emit doSetListCellColor(EL_DEL_PAR, i, ET_FATAL);
            cNodeParam& par = *pNodeTrg->params.at(i);
            par._toReadBack = RB_NO;
            switch (par.remove(q, false, QBitArray(), EX_ERROR)) {
            case 0:  emit doSetListCellColor(EL_DEL_PAR, i, ET_WARNING);  break;
            case 1:  emit doSetListCellColor(EL_DEL_PAR, i, ET_OK);       break;
            default: emit doSetListCellColor(EL_DEL_PAR, i, ET_ERROR);    break;
            }
        }
    }
    // Del assigned node service(s)
    n = srvDelIdList.size();
    for (i = 0; i < n; ++i) {
        pi = pUi->listWidgetDelSrv->item(i);
        if (pi == nullptr) EXCEPTION(EPROGFAIL);
        if (pi->checkState() == Qt::Checked) {
            emit doSetListCellColor(EL_DEL_SRV, i, ET_FATAL);
            cHostService hs;
            hs._toReadBack = RB_NO;
            switch (hs.removeById(q, srvDelIdList.at(i))) {
            case 0:  emit doSetListCellColor(EL_DEL_SRV, i, ET_WARNING);  break;
            case 1:  emit doSetListCellColor(EL_DEL_SRV, i, ET_OK);       break;
            default: emit doSetListCellColor(EL_DEL_SRV, i, ET_ERROR);    break;
            }
        }
    }
    // Move assigned param(s)
    n = pNodeSrc->params.size();
    for (i = 0; i < n; ++i) {
        pi = pUi->listWidgetCpyPar->item(i);
        if (pi == nullptr) EXCEPTION(EPROGFAIL);
        if (pi->checkState() == Qt::Checked) {
            emit doSetListCellColor(EL_CPY_PAR, i, ET_FATAL);
            cNodeParam& par = *pNodeSrc->params.at(i);
            par._toReadBack = RB_NO;
            par.setId(_sNodeId, nidTrg);
            switch (par.update(q, false, par.mask(_sNodeId), QBitArray(), EX_ERROR)) {
            case 0:  emit doSetListCellColor(EL_CPY_PAR, i, ET_WARNING);  break;
            case 1:  emit doSetListCellColor(EL_CPY_PAR, i, ET_OK);       break;
            default: emit doSetListCellColor(EL_CPY_PAR, i, ET_ERROR);    break;
            }
        }
    }
    // Del assigned node service(s)
    n = srvCpyIdList.size();
    for (i = 0; i < n; ++i) {
        pi = pUi->listWidgetCpySrv->item(i);
        if (pi == nullptr) EXCEPTION(EPROGFAIL);
        if (pi->checkState() == Qt::Checked) {
            emit doSetListCellColor(EL_CPY_SRV, i, ET_FATAL);
            cHostService hs;
            hs._toReadBack = RB_NO;
            hs.setId(srvCpyIdList.at(i));
            hs.setId(_sNodeId, nidTrg);
            switch (hs.update(q, false, hs.mask(_sNodeId), QBitArray(), EX_ERROR)) {
            case 0:  emit doSetListCellColor(EL_CPY_SRV, i, ET_WARNING);  break;
            case 1:  emit doSetListCellColor(EL_CPY_SRV, i, ET_OK);       break;
            default: emit doSetListCellColor(EL_CPY_SRV, i, ET_ERROR);    break;
            }
        }
    }
    // Port(s)
    foreach (cCSRow *pRow, rows) {
        pRow->save(q);
    }
}

bool cChangeSwitch::filteringPorts(QList<cNPort *>& ports)
{
    for (int i = 0; i < ports.size(); ++i) {
        cNPort *pp = ports.at(i);
        const cIfType& ifType = cIfType::ifType(pp->getId(_sIfTypeId));
        eLinkType lt = eLinkType(ifType.getId(_sIfTypeLinkType));
        if (lt == LT_PTP || lt == LT_BUS) {    // patchable
            if (pp->isNull(_sPortIndex)) {
                return false;   // Missing index
            }
        }
        else {      // uninteresting, discarded
            delete ports.takeAt(i);
            --i;
        }
    }
    return true;
}

void cChangeSwitch::on_spinBoxOffsetSrc_valueChanged(int arg1)
{
    switch (state) {
    case ES_INSUFF:
    case ES_ERROR:
        break;
    case ES_SRC_READY:
    case ES_DATA_READY:
        state = ES_SRC_READY;
        offsSrc = arg1;
        break;
    case ES_RUN_SAVER:
        EXCEPTION(EPROGFAIL);
    }
    setButtons(false);
}

void cChangeSwitch::on_spinBoxOffsetTrg_valueChanged(int arg1)
{
    switch (state) {
    case ES_INSUFF:
    case ES_ERROR:
        break;
    case ES_SRC_READY:
    case ES_DATA_READY:
        state = ES_SRC_READY;
        offsTrg = arg1;
        break;
    case ES_RUN_SAVER:
        EXCEPTION(EPROGFAIL);
    }
    setButtons(false);
}

void cChangeSwitch::on_selNodeSrc_nodeIdChanged(qlonglong nid)
{
    if (state == ES_RUN_SAVER) {
        EXCEPTION(EPROGFAIL);
    }
    QString e;
    pDelete(pNodeSrc);
    clear();
    nidSrc = nid;
    if (nid != NULL_ID) {
        pNodeSrc = new cNode();
        pNodeSrc->setById(*pq, nidSrc);
        pNodeSrc->fetchPorts(*pq, CV_PORT_PARAMS);
        pNodeSrc->fetchParams(*pq);
        if (!filteringPorts(pNodeSrc->ports)) {
            e = sIxNote + trUtf8("A forrás eszköznél nincs minden portnak indexe.");
            goto discard_node_old;
        }
        if (pNodeSrc->ports.isEmpty()) {
            e = trUtf8("Nincs mit másolni.");
            goto discard_node_old;
        }
        pNodeSrc->sortPortsByIndex();
        minIxSrc= int(pNodeSrc->ports.first()->getId(_sPortIndex));
        offsSrc = minIxSrc -1;
        maxIxSrc = int(pNodeSrc->ports.last()->getId(_sPortIndex));
        pUi->spinBoxOffsetSrc->setValue(offsSrc);
    }
    setButtons();
    return;
discard_node_old:
    QMessageBox::warning(this, dcViewShort(DC_WARNING), e);
    pDelete(pNodeSrc);
    setButtons();
    return;
}

void cChangeSwitch::on_selNodeTrg_nodeIdChanged(qlonglong nid)
{
    if (state == ES_RUN_SAVER) {
        EXCEPTION(EPROGFAIL);
    }
    QString e;
    pDelete(pNodeTrg);
    clear();
    nidTrg = nid;
    if (nid != NULL_ID) {
        pNodeTrg = new cNode();
        pNodeTrg->setById(*pq, nidTrg);
        pNodeTrg->fetchPorts(*pq, CV_PORT_PARAMS);
        pNodeTrg->fetchParams(*pq);
        if (!filteringPorts(pNodeTrg->ports)) {
            e = sIxNote + trUtf8("A cél eszköznél nincs minden portnak indexe.");
            goto discard_node_new;
        }
        if (pNodeTrg->ports.isEmpty()) {
            e = trUtf8("Nincs hova másolni.");
            goto discard_node_new;
        }
        pNodeTrg->sortPortsByIndex();
        minIxTrg = int(pNodeTrg->ports.first()->getId(_sPortIndex));
        offsTrg = minIxTrg -1;
        maxIxTrg = int(pNodeTrg->ports.last()->getId(_sPortIndex));
        pUi->spinBoxOffsetTrg->setValue(offsTrg);
    }
    setButtons();
    return;
discard_node_new:
    QMessageBox::warning(this, dcViewShort(DC_WARNING), e);
    pDelete(pNodeTrg);
    setButtons();
    return;
}

void cChangeSwitch::on_tableWidgetHHeader_clicked(int sect)
{
    if (state != ES_DATA_READY) return;
    bool f;
    switch (sect) {
    case cCSRow::CIX_CPY_LNK:
    case cCSRow::CIX_CPY_SRV:
    case cCSRow::CIX_CPY_PAR:
    case cCSRow::CIX_DEL_LNK:
    case cCSRow::CIX_DEL_SRV:
    case cCSRow::CIX_DEL_PAR:
        f = setHeaderIcon(sect);
        foreach (cCSRow * pRow, rows) {
            pRow->setChecked(sect, f);
        }
    }
}

void cChangeSwitch::on_pushButtonRefresh_clicked()
{
    if (state == ES_SRC_READY) refreshTables();
}

void cChangeSwitch::on_pushButtonCopy_clicked()
{
    if (state != ES_DATA_READY) EXCEPTION(EPROGFAIL, state, trUtf8("Invalid state."));
    if (pSaver != nullptr) EXCEPTION(EPROGFAIL, 0, trUtf8("Rerun thread?"));
    static const QString msg = trUtf8("A művelet nem visszavonható! Valóban végrehajtja a változtatásokat ?");
    int r = QMessageBox::warning(this, dcViewShort(DC_WARNING), msg, QMessageBox::Ok, QMessageBox::Cancel);
    if (r != QMessageBox::Ok) return;
    pSaver = new cCSSaver(this);
    state = ES_RUN_SAVER;
    setButtons(false);
    connect(pSaver, SIGNAL(finished()), this, SLOT(on_saver_finished()));
    pSaver->start();
}

void cChangeSwitch::setChecked(enum eList e, QListWidget *p)
{
    Qt::CheckState f = setListIcon(e) ? Qt::Checked : Qt::Unchecked;
    int i, n = p->count();
    for (i = 0; i < n; ++i) {
        QListWidgetItem *pi = p->item(i);
        if (pi == nullptr) EXCEPTION(EPROGFAIL, e, p->objectName());
        pi->setCheckState(f);
    }
}

void cChangeSwitch::on_toolButtonCpySrv_clicked()
{
    setChecked(EL_CPY_SRV, pUi->listWidgetCpySrv);
}

void cChangeSwitch::on_toolButtonDelSrv_clicked()
{
    setChecked(EL_DEL_SRV, pUi->listWidgetDelSrv);
}

void cChangeSwitch::on_toolButtonCpyPar_clicked()
{
    setChecked(EL_CPY_PAR, pUi->listWidgetCpyPar);
}

void cChangeSwitch::on_toolButtonDelPar_clicked()
{
    setChecked(EL_DEL_PAR, pUi->listWidgetDelPar);
}

void cChangeSwitch::on_saver_finished()
{
    if (pSaver == nullptr) EXCEPTION(EPROGFAIL);
    if (pSaver->pe == nullptr) {
        pDelete(pNodeSrc);
        pDelete(pNodeTrg);
        nidSrc = nidTrg = NULL_ID;
        state = ES_INSUFF;
    }
    else {
        cErrorMessageBox::messageBox(pSaver->pe, this);
        state = ES_DATA_READY;
    }
    pDelete(pSaver);
    setButtons();
}

void cChangeSwitch::setTableCellColor(int row, int col, int ix, int ec)
{
    QWidget *pw = pUi->tableWidget->cellWidget(row, col);
    if (pw != nullptr && pw->inherits("QListWidget")) {
        QListWidget *plw = qobject_cast<QListWidget *>(pw);
        QListWidgetItem *pi = plw->item(ix);
        if (pi == nullptr) {
            EXCEPTION(EPROGFAIL,0, trUtf8("Invalid index in cell: row = %1, col = %2, ix = %3").arg(row).arg(col).arg(ix));
        }
        pi->setBackgroundColor(bgColorByEnum(sET_, ec));
    }
    else {
        EXCEPTION(EPROGFAIL,0, trUtf8("Invalid cell: row = %1, col = %2").arg(row).arg(col));
    }
}

void cChangeSwitch::setListCellColor(int lt, int row, int ec)
{
    QListWidgetItem *pi = nullptr;
    switch (lt) {
    case EL_CPY_PAR:    pi = pUi->listWidgetCpyPar->item(row);  break;
    case EL_CPY_SRV:    pi = pUi->listWidgetCpySrv->item(row);  break;
    case EL_DEL_PAR:    pi = pUi->listWidgetDelPar->item(row);  break;
    case EL_DEL_SRV:    pi = pUi->listWidgetDelSrv->item(row);  break;
    default:            break;
    }
    if (pi == nullptr) {
        EXCEPTION(EPROGFAIL);
    }
    pi->setBackgroundColor(bgColorByEnum(sET_, ec));
}
