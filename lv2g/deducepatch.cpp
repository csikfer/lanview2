#include "deducepatch.h"
#include "report.h"
#include "lv2widgets.h"
#include "popupreport.h"

int cDPRow::ixSharedPortId = NULL_IX;
const QString cDPRow::sIOk      = "://icons/ok.ico";
const QString cDPRow::sIWarning = "://icons/emblem-important-yellow.ico";
const QString cDPRow::sIError   = "://icons/emblem-important-red.ico";
const QString cDPRow::sIReapeat = "://icons/go-jump-3.ico";
const QString cDPRow::sIHelp    = "://icons/help-hint.ico";
const QString cDPRow::sIConflict= "://icons/emblem-conflicting.ico";
const QString cDPRow::sITagSt   = "://icons/tag_st.png";
const QString cDPRow::sIDbAdd   = "://icons/db_add.ico";
const QString cDPRow::sILldp    = "://icons/servers.png";   // :-/
const QString cDPRow::sIMap     = "://icons/map.png";

cDPRow::cDPRow(cDeducePatch *par, int _row, int _save_col, cPPort &_ppl, cPhsLink &_pll, cPPort &_ppr, cPhsLink &_plr)
    : phsLink(), model(*par->pModel), ppl(_ppl), ppr(_ppr), pll(_pll), plr(_plr)
{
    staticInit();
    parent = par;
    reapeat = exists = colision = false;
    warning = 0;    // 0: OK, 1: warning, 2: error
    stateMAC = stateTag = stateLLDP = TS_NULL;
    pid = spidl = spidr = NULL_ID;
    shl = shr = shel = sher = ES_;
    npidl = pll.getId(_sPortId1);
    npidr = plr.getId(_sPortId1);
    pidl  = ppl.getId();
    pidr  = ppr.getId();
    spidl = ppl.getId(ixSharedPortId);
    spidr = ppr.getId(ixSharedPortId);
    if (_save_col != CID_HAVE_NO) {
        pItemSave  = new QStandardItem;
        model.setItem(_row, _save_col, pItemSave);
    }
    else {
        pItemSave = nullptr;
    }
    pItemIndex = new QStandardItem;
    model.setItem(_row, 0, pItemIndex);
}

cDPRow::~cDPRow()
{

}

QList<QString>      cDPRow::titles;
QList<QString>      cDPRow::icons;
QList<QString>      cDPRow::tooltips;

void cDPRow::colHead(int x, const QString& title, const QString& icon, const QString& tooltip)
{
    if (titles.size() != x || icons.size() != x || tooltips.size() != x) EXCEPTION(EPROGFAIL, x);
    titles   << title;
    icons    << icon;
    tooltips << tooltip;
}

void cDPRow::staticInit()
{
    if (ixSharedPortId != NULL_IX) return;

    ixSharedPortId = cPPort().toIndex(_sSharedPortId);
    //                          title                           icon        tool tip
    colHead(CID_INDEX,          "#",                            _sNul,      QObject::trUtf8("A helyi patch port indexek."));
    colHead(CID_STATE_WARNING,  QObject::trUtf8("Megjegyzés"),  sIHelp,     QObject::trUtf8("Megjegyzés, hiba üzenetek."));
    colHead(CID_STATE_CONFLICT, QObject::trUtf8("Konfliktus"),  sIConflict, QObject::trUtf8("Ütközések, mentés esetén törlendő linkek."));
    colHead(CID_STATE_TAG,      QObject::trUtf8("Cimke"),       sITagSt,    QObject::trUtf8("A patch port címkék összevetése."));
    colHead(CID_STATE_MAC,      QObject::trUtf8("Cím"),         sIMap,      QObject::trUtf8("Osszevetés az switch címtáblákkal."));
    colHead(CID_STATE_LLDP,     QObject::trUtf8("LLDP"),        sILldp,     QObject::trUtf8("Osszevetés az LLDP lekérdezésekkel."));
    colHead(CID_SAVE,           QObject::trUtf8("Mentve"),      sIDbAdd,    QObject::trUtf8("Mentés."));
    colHead(CID_IF_LEFT,        QObject::trUtf8("Interfész"),   _sNul,      QObject::trUtf8("Helyi vagy bal oldali interfész."));
    colHead(CID_SHARE_LEFT,     QObject::trUtf8("Sh."),         _sNul,      QObject::trUtf8("Helyi vagy bal oldali megosztás, eredő megosztás."));
    colHead(CID_PPORT_LEFT,     QObject::trUtf8("Patch"),       _sNul,      QObject::trUtf8("Helyi vagy bal oldali patch port."));
    colHead(CID_TAG_LEFT,       QObject::trUtf8("Címke"),       _sNul,      QObject::trUtf8("Helyi vagy bal oldali patch port címke."));
    colHead(CID_TAG,            QObject::trUtf8("Címke"),       _sNul,      QObject::trUtf8("Patch port címke."));
    colHead(CID_TAG_RIGHT,      QObject::trUtf8("Címke"),       _sNul,      QObject::trUtf8("Távoli vagy jobb oldali patch port címke."));
    colHead(CID_PPORT_RIGHT,    QObject::trUtf8("Patch"),       _sNul,      QObject::trUtf8("Távoli vagy jobb oldali patch port."));
    colHead(CID_SHARE_RIGHT,    QObject::trUtf8("Sh."),         _sNul,      QObject::trUtf8("Távoli vagy jobb oldali megosztás, eredő megosztás."));
    colHead(CID_IF_RIGHT,       QObject::trUtf8("Inteface"),    _sNul,      QObject::trUtf8("Távoli vagy jobb oldali interfész."));
}

void cDPRow::showText(int column, const QString& text, const QString& tool_tip)
{
    pi = new QStandardItem(text);
    if (!tool_tip.isEmpty()) pi->setToolTip(tool_tip);
    model.setItem(row(), column, pi);
}

void cDPRow::showCell(int column, const QString& text, const QString& icon, const QString& tool_tip)
{
    pi = new QStandardItem(text);
    if (!icon.isEmpty()) pi->setIcon(QIcon(icon));
    if (!tool_tip.isEmpty()) pi->setToolTip(tool_tip);
    model.setItem(row(), column, pi);
}

void cDPRow::showIndex(const QString& _tt)
{
    pItemIndex->setText(ppl.getName(_sPortIndex).rightJustified(3));
    if (!_tt.isEmpty()) pItemIndex->setToolTip(_tt);
}


void cDPRow::save(QSqlQuery& q)
{
    if (isChecked()) {
        phsLink.replace(q);
        pItemSave = new QStandardItem(QIcon(sIOk), _sNul);
        model.setItem(pItemSave->row(), cxSave(), pItemSave);
    }
}

bool cDPRow::isChecked()
{
    return pItemSave != nullptr && pItemSave->isCheckable() && pItemSave->checkState() == Qt::Checked;
}


int cDPRow::row() const
{
    if (pItemIndex == nullptr) EXCEPTION(EPROGFAIL);
    return pItemIndex->row();
}


void cDPRow::setSaveCell()
{
    if (pItemSave == nullptr) EXCEPTION(EPROGFAIL);
    if     (reapeat) pItemSave->setIcon(QIcon(sIReapeat));
    else if (exists) pItemSave->setIcon(QIcon(sIOk));
    else {
        pItemSave->setCheckable(true);
        pItemSave->setCheckState(warning || colision ? Qt::Unchecked : Qt::Checked);
    }
}

/// Set calculated link, and checks
void cDPRow::calcLinkCheck(QSqlQuery& q)
{
    bool isPrimary;
    shl = ePortShare(ppl.getId(_sSharedCable));
    pid = spidl;
    isPrimary = shl == ES_ || shl == ES_A || shl == ES_AA;
    if (spidl == NULL_ID) {
        pid = pidl;
        if (!isPrimary) {
            warnMsg += htmlError(QObject::trUtf8("A lokális/bal oldali hátlapi nem elsődleges megosztásnál hiányzik az elsődleges port hivatkozás."));
            warning = 2;
        }
    }
    else {
        if (isPrimary) {
            warnMsg += htmlError(QObject::trUtf8("A lokális/bal oldali hátlapi elsődleges megosztásnál nem lehet elsődleges port hivatkozás (%1).").arg(cNPort().getNameById(q, pid)));
            warning = 2;
        }
    }
    phsLink.setId(_sPortId1, pid);
    phsLink.setId(_sPhsLinkType1, LT_BACK);
    shr = ePortShare(ppr.getId(_sSharedCable));
    pid = spidr;
    isPrimary = shr == ES_ || shr == ES_A || shr == ES_AA;
    if (spidr == NULL_ID) {
        pid = pidr;
        if (!isPrimary) {
            warnMsg += htmlError(QObject::trUtf8("A távoli/jobb oldali hátlapi nem elsődleges megosztásnál hiányzik az elsődleges port hivatkozás."));
            warning = 2;
        }
    }
    else {
        if (isPrimary) {
            warnMsg += htmlError(QObject::trUtf8("A távoli/jobb oldali hátlapi elsődleges megosztásnál nem lehet elsődleges port hivatkozás (%1).").arg(cNPort().getNameById(q, pid)));
            warning = 2;
        }
    }
    phsLink.setId(_sPortId2, pid);
    phsLink.setId(_sPhsLinkType2, LT_BACK);
    phsLink.setId(_sPortShared, ES_);   // Back-Back: not possible share!
    reapeat = parent->findLink(phsLink);
    colision = linkColisionTest(q, exists, phsLink, colMsg);
}

void cDPRow::checkAndShowTag(int _cx_st, int _cx_left, int _cx_right)
{
    s = ppl.getName(_sPortTag);
    showText(_cx_left, s);
    s2 = ppr.getName(_sPortTag);
    showText(_cx_right, s2);
    if (s.isEmpty() || s2.isEmpty()) return;
    if (s.compare(s2, Qt::CaseInsensitive)) {
        s = sIWarning;
        stateTag = TS_FALSE;
    }
    else {
        s = sIOk;
        stateTag = TS_TRUE;
    }
    showCell(_cx_st, _sNul, s);
}

void cDPRow::showWarning(int _cx)
{
    s = warningIconName(warning);
    showCell(_cx, _sNul, s, warnMsg);
}

void cDPRow::showPatchPort(QSqlQuery& q, int _cx, cPPort& _pp, const QString& _name)
{
    s = _name;
    if (s.isEmpty()) s = _pp.getName();
    ePortShare sh = ePortShare(_pp.getId(_sSharedCable));
    pi = new QStandardItem;
    if (sh != ES_) {
        if      (sh == ES_A)  s += " (A)";
        else if (sh == ES_AA) s += " (AA)";
        else {
            if (_pp.getId(ixSharedPortId) == NULL_ID) {
                s += " ( ? <- " + portShare(shl) + ")";
                pi->setForeground(QBrush(dcFgColor(DC_ERROR)));
                pi->setFont(dcFont(DC_ERROR));
            }
            else {
                s += " (" + _pp.view(q, ixSharedPortId) + " <- " + portShare(sh) + ")";
            }
        }
    }
    pi->setText(s);
    model.setItem(row(), _cx, pi);
}

void cDPRow::showShare(int _cx, ePortShare sh, ePortShare msh, bool _warn)
{
    if (msh != ES_INVALID && (sh != ES_ || msh != ES_)) {
        s =  portShare(sh).rightJustified(2) + "/" + portShare(msh).rightJustified(2);
        showText(_cx, s);
        if (_warn) {
            pi->setForeground(QBrush(dcFgColor(DC_ERROR)));
            pi->setFont(dcFont(DC_ERROR));
        }
    }
}

void cDPRow::showNodePort(QSqlQuery q, int _cx, qlonglong _pid)
{
    if (_pid != NULL_ID) {
        s = cNPort::getFullNameById(q, _pid);
        showText(_cx, s);
    }
}

void cDPRow::showConflict(int _cx)
{
    s = warningIconName(colision ? 2 : 0);
    showCell(_cx, _sNul, s, colMsg);
}

void cDPRow::checkAndShowSharing(int _cx_left, int _cx_right)
{
    shel = pll.isNull() ? ES_INVALID : shareResultant(shl, ePortShare(pll.getId(_sPortShared)));
    sher = plr.isNull() ? ES_INVALID : shareResultant(shr, ePortShare(plr.getId(_sPortShared)));
    bool _warn = false;
    if (shel != ES_INVALID && sher != ES_INVALID) {
        _warn = shel != sher;
        if (_warn) {
            warnMsg += htmlWarning(QObject::trUtf8("Nem egyező végponti megosztások"));
            if (warning == 0) warning = 1; // suspicious
        }
    }
    showShare(_cx_left,  shl, shel, _warn);
    showShare(_cx_right, shr, sher, _warn);
}

void cDPRow::checkAndShowLLDP(QSqlQuery& q, int _cx)
{
    if (npidl == NULL_ID || npidr == NULL_ID) return;
    s2.clear(); // MSG
    cLldpLink lldp;
    if (lldp.isLinked(q, npidl, npidr)) {
        s = sIOk;
        stateLLDP = TS_TRUE;
    }
    else {
        if (lldp.clear().setId(_sPortId1, npidl).completion(q)) {
            s2  = htmlError(lldpLink2str(q, lldp));
        }
        if (lldp.clear().setId(_sPortId1, npidr).completion(q)) {
            s2 += htmlError(lldpLink2str(q, lldp));
        }
        if (s2.isEmpty()) {
            s = sIWarning;
            s2 = QObject::trUtf8("Nincs találat az LLDP linkek között.");
        }
        else {
            s = sIError;
            stateLLDP = TS_FALSE;
        }
    }
    pi = new QStandardItem(QIcon(s), _sNul);
    if (!s2.isEmpty()) pi->setToolTip(s2);
    model.setItem(row(), _cx, pi);
}

eTristate cDPRow::checkMac(QSqlQuery& q, cNPort& pnp1, cNPort& pnp2)
{
    cMac mac = pnp1.getMac(_sHwAddress, EX_IGNORE); // MAC, if there is
    if (mac.isValid()) {
        cMacTab mt;                                 // Find
        if (mt.setMac(_sHwAddress, mac).completion(q) == 1) {
            qlonglong swpid = mt.getId(_sPortId);
            if (swpid == pnp2.getId()) {
                s2 += QObject::trUtf8("Megerősített link, a %1 címtáblája alapján (%2 - %3)\n")
                        .arg(pnp2.getFullName(q), mt.getName(_sFirstTime), mt.getName(_sLastTime));
                return TS_TRUE;
            }
            else {
                s2 += QObject::trUtf8("A címtábla lekérdezésnek ellentmondó link, a %1 -> %2 címtáblája alapján (%3 - %4)\n")
                        .arg(cNPort::getFullNameById(q, swpid), pnp2.getFullName(q),
                             mt.getName(_sFirstTime), mt.getName(_sLastTime));
                return  TS_FALSE;
            }
        }
    }
    return TS_NULL;
}

eTristate cDPRow::checkMac(QSqlQuery &q)
{
    cNPort *pnpl = cNPort::getPortObjById(q, npidl);
    cNPort *pnpr = cNPort::getPortObjById(q, npidr);
    eTristate r = checkMac(q, *pnpl, *pnpr);
    if (r == TS_NULL) {
        r = checkMac(q, *pnpr, *pnpl);
    }
    delete pnpl;
    delete pnpr;
    return r;
}

void cDPRow::checkAndShowMAC(QSqlQuery& q, int _cx)
{
    if (npidl == NULL_ID || npidr == NULL_ID) return;
    s2.clear(); // MSG
    stateMAC = checkMac(q);
    switch (stateMAC) {
    case TS_NULL:   return;
    case TS_FALSE:
        s = sIError;
        if (warning == 0) warning = 1;
        break;
    case TS_TRUE:
        s = sIOk;
        break;
    }
    s2.chop(1);
    showCell(_cx, _sNul, s, toHtml(s2, true));
}

void cDPRow::setHeader(const QVector<int> &cid2col)
{
    for (int i = 0; i < CID_NUMBER; ++i) {
        int col = cid2col[i];
        if (col >= 0) setHeaderItem(i, col);
    }
}

QStringList cDPRow::exportHeader(const QVector<int> &cid2col)
{
    QStringList r;
    for (int cid = 0; cid < CID_NUMBER; ++cid) {
        int col = cid2col[cid];
        if (col >= 0) r << titles.at(cid);
    }
    return r;
}

QList<tStringPair> cDPRow::exporter(const QVector<int> &cid2col)
{
    QList<tStringPair> r;
    for (int cid = 0; cid < CID_NUMBER; ++cid) {    // All column id
        int col = cid2col[cid];                      // To column index
        if (col >= 0) {                             // Index valid (show)?
            QStandardItem *pi = model.item(row(), col); // Get sell item
            if (pi == nullptr) {
                r << tStringPair();
            }
            else {
                QString text;
                // QString tooltip = pi->toolTip();
                switch (cid) {
                case CID_SAVE:
                    text = langBool(exists);
                    break;
                case CID_STATE_WARNING:
                    switch (warning) {
                    case 0: text = _sOk;                      break;
                    case 1: text = dcViewShort(DC_WARNING);   break;
                    default:text = dcViewShort(DC_ERROR);     break;
                    }
                    break;
                case CID_STATE_CONFLICT:
                    text = langBool(colision);
                    break;
                case CID_STATE_LLDP:
                    text = langTristate(stateLLDP);
                    break;
                case CID_STATE_MAC:
                    text = langTristate(stateMAC);
                    break;
                case CID_STATE_TAG:
                    text = langTristate(stateTag);
                    break;
                default:
                    text = pi->text();
                    break;
                }
                r << tStringPair(text, pi->toolTip());
            }
        }
    }
    return r;
}

/* *** */

cDPRowMAC::cDPRowMAC(QSqlQuery& q, cDeducePatch *par, int _row, cMacTab &mt, cPPort& _ppl, cPhsLink& _pll, cPPort &_ppr, cPhsLink& _plr)
    : cDPRow(par, _row, CX_SAVE, _ppl, _pll, _ppr, _plr)
{
    (void)mt;
    if (_row == 0) setHeader();
    showIndex(toHtml(mactab2str(q, mt)));
    checkAndShowTag(CX_STATE_TAG, CX_TAG_LEFT, CX_TAG_RIGHT);
    calcLinkCheck(q);
    checkAndShowSharing(CX_SHARE_LEFT, CX_SHARE_RIGHT);
    showWarning(CX_STATE_WARNING);
    showConflict(CX_STATE_CONFLICT);
    showNodePort(q, CX_IF_LEFT, npidl);
    showPatchPort(q, CX_PPORT_LEFT, ppl);
    showPatchPort(q, CX_PPORT_RIGHT, ppr, ppr.getFullName(q));
    showNodePort(q, CX_IF_RIGHT, npidr);
    checkAndShowLLDP(q, CX_STATE_LLDP);
    setSaveCell();
}

cDPRowMAC::~cDPRowMAC()
{
    ;
}

QVector<int> cDPRowMAC::cid2col;

void cDPRowMAC::setHeader()
{
    if (cid2col.isEmpty()) {
        addCid2Col<cid2col>(CID_INDEX            ,  CX_INDEX);
        addCid2Col<cid2col>(CID_STATE_WARNING    ,  CX_STATE_WARNING);
        addCid2Col<cid2col>(CID_STATE_CONFLICT   ,  CX_STATE_CONFLICT);
        addCid2Col<cid2col>(CID_STATE_TAG        ,  CX_STATE_TAG);
        addCid2Col<cid2col>(CID_STATE_LLDP       ,  CX_STATE_LLDP);
        addCid2Col<cid2col>(CID_SAVE             ,  CX_SAVE);
        addCid2Col<cid2col>(CID_IF_LEFT          ,  CX_IF_LEFT);
        addCid2Col<cid2col>(CID_SHARE_LEFT       ,  CX_SHARE_LEFT);
        addCid2Col<cid2col>(CID_PPORT_LEFT       ,  CX_PPORT_LEFT);
        addCid2Col<cid2col>(CID_TAG_LEFT         ,  CX_TAG_LEFT);
        addCid2Col<cid2col>(CID_TAG_RIGHT        ,  CX_TAG_RIGHT);
        addCid2Col<cid2col>(CID_PPORT_RIGHT      ,  CX_PPORT_RIGHT);
        addCid2Col<cid2col>(CID_SHARE_RIGHT      ,  CX_SHARE_RIGHT);
        addCid2Col<cid2col>(CID_IF_RIGHT         ,  CX_IF_RIGHT);
        if (cid2col.size() != CID_NUMBER) EXCEPTION(EPROGFAIL);     // Last column not hide!
    }
    cDPRow::setHeader(cid2col);
}

QStringList cDPRowMAC::exportHeader()
{
    return cDPRow::exportHeader(cid2col);
}

QList<tStringPair> cDPRowMAC::exporter()
{
    return cDPRow::exporter(cid2col);
}

/* --- */

cDPRowTag::cDPRowTag(QSqlQuery& q, cDeducePatch *par, int _row, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr)
    : cDPRow(par, _row, CX_SAVE, _ppl, _pll, _ppr, _plr)
{
    if (_row == 0) setHeader();
    showIndex();
    showText(CX_TAG, ppl.getName(_sPortTag));
    calcLinkCheck(q);
    checkAndShowSharing(CX_SHARE_LEFT, CX_SHARE_RIGHT);
    showWarning(CX_STATE_WARNING);
    showConflict(CX_STATE_CONFLICT);
    showNodePort(q, CX_IF_LEFT, npidl);
    showPatchPort(q, CX_PPORT_LEFT, ppl);
    showPatchPort(q, CX_PPORT_RIGHT, ppr);
    showNodePort(q, CX_IF_RIGHT, npidr);
    checkAndShowLLDP(q, CX_STATE_LLDP);
    checkAndShowMAC(q, CX_STATE_MAC);
    setSaveCell();
}

cDPRowTag::~cDPRowTag()
{
    ;
}

QVector<int> cDPRowTag::cid2col;

void cDPRowTag::setHeader()
{
    if (cid2col.isEmpty()) {
        addCid2Col<cid2col>(CID_INDEX            ,  CX_INDEX);
        addCid2Col<cid2col>(CID_STATE_WARNING    ,  CX_STATE_WARNING);
        addCid2Col<cid2col>(CID_STATE_CONFLICT   ,  CX_STATE_CONFLICT);
        addCid2Col<cid2col>(CID_STATE_MAC        ,  CX_STATE_MAC);
        addCid2Col<cid2col>(CID_STATE_LLDP       ,  CX_STATE_LLDP);
        addCid2Col<cid2col>(CID_SAVE             ,  CX_SAVE);
        addCid2Col<cid2col>(CID_IF_LEFT          ,  CX_IF_LEFT);
        addCid2Col<cid2col>(CID_SHARE_LEFT       ,  CX_SHARE_LEFT);
        addCid2Col<cid2col>(CID_PPORT_LEFT       ,  CX_PPORT_LEFT);
        addCid2Col<cid2col>(CID_TAG              ,  CX_TAG);
        addCid2Col<cid2col>(CID_PPORT_RIGHT      ,  CX_PPORT_RIGHT);
        addCid2Col<cid2col>(CID_SHARE_RIGHT      ,  CX_SHARE_RIGHT);
        addCid2Col<cid2col>(CID_IF_RIGHT         ,  CX_IF_RIGHT);
        if (cid2col.size() != CID_NUMBER) EXCEPTION(EPROGFAIL);     // Last column not hide!
    }
    cDPRow::setHeader(cid2col);
}

QStringList cDPRowTag::exportHeader()
{
    return cDPRow::exportHeader(cid2col);
}

QList<tStringPair> cDPRowTag::exporter()
{
    return cDPRow::exporter(cid2col);
}

/* --- */

cDPRowLLDP::cDPRowLLDP(QSqlQuery& q, cDeducePatch *par, int _row, cLldpLink &lldp, cPPort& _ppl, cPhsLink& _pll, cPPort &_ppr, cPhsLink& _plr)
    : cDPRow(par, _row, CX_SAVE, _ppl, _pll, _ppr, _plr)
{
    if (_row == 0) setHeader();
    showIndex(toHtml(lldpLink2str(q, lldp)));
    checkAndShowTag(CX_STATE_TAG, CX_TAG_LEFT, CX_TAG_RIGHT);
    calcLinkCheck(q);
    checkAndShowSharing(CX_SHARE_LEFT, CX_SHARE_RIGHT);
    showWarning(CX_STATE_WARNING);
    showConflict(CX_STATE_CONFLICT);
    showNodePort(q, CX_IF_LEFT, npidl);
    showPatchPort(q, CX_PPORT_LEFT, ppl);
    showPatchPort(q, CX_PPORT_RIGHT, ppr, parent->nidr == NULL_ID ? ppr.getFullName(q) : ppr.getName());
    showNodePort(q, CX_IF_RIGHT, npidr);
    checkAndShowMAC(q, CX_STATE_MAC);
    setSaveCell();
}

cDPRowLLDP::~cDPRowLLDP()
{
    ;
}

QVector<int> cDPRowLLDP::cid2col;

void cDPRowLLDP::setHeader()
{
    if (cid2col.isEmpty()) {
        addCid2Col<cid2col>(CID_INDEX            ,  CX_INDEX);
        addCid2Col<cid2col>(CID_STATE_WARNING    ,  CX_STATE_WARNING);
        addCid2Col<cid2col>(CID_STATE_CONFLICT   ,  CX_STATE_CONFLICT);
        addCid2Col<cid2col>(CID_STATE_TAG        ,  CX_STATE_TAG);
        addCid2Col<cid2col>(CID_STATE_MAC        ,  CX_STATE_MAC);
        addCid2Col<cid2col>(CID_SAVE             ,  CX_SAVE);
        addCid2Col<cid2col>(CID_IF_LEFT          ,  CX_IF_LEFT);
        addCid2Col<cid2col>(CID_SHARE_LEFT       ,  CX_SHARE_LEFT);
        addCid2Col<cid2col>(CID_PPORT_LEFT       ,  CX_PPORT_LEFT);
        addCid2Col<cid2col>(CID_TAG_LEFT         ,  CX_TAG_LEFT);
        addCid2Col<cid2col>(CID_TAG_RIGHT        ,  CX_TAG_RIGHT);
        addCid2Col<cid2col>(CID_PPORT_RIGHT      ,  CX_PPORT_RIGHT);
        addCid2Col<cid2col>(CID_SHARE_RIGHT      ,  CX_SHARE_RIGHT);
        addCid2Col<cid2col>(CID_IF_RIGHT         ,  CX_IF_RIGHT);
        if (cid2col.size() != CID_NUMBER) EXCEPTION(EPROGFAIL);     // Last column not hide!
    }
    cDPRow::setHeader(cid2col);
}

QStringList cDPRowLLDP::exportHeader()
{
    return cDPRow::exportHeader(cid2col);
}

QList<tStringPair> cDPRowLLDP::exporter()
{
    return cDPRow::exporter(cid2col);
}

/* --- */

cDPRowPar::cDPRowPar(QSqlQuery& q, cDeducePatch *par, int _row, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr)
    : cDPRow(par, _row, CX_SAVE, _ppl, _pll, _ppr, _plr)
{
    if (_row == 0) setHeader();
    showIndex();
    checkAndShowTag(CX_STATE_TAG, CX_TAG_LEFT, CX_TAG_RIGHT);
    calcLinkCheck(q);
    checkAndShowSharing(CX_SHARE_LEFT, CX_SHARE_RIGHT);
    showWarning(CX_STATE_WARNING);
    showConflict(CX_STATE_CONFLICT);
    showNodePort(q, CX_IF_LEFT, npidl);
    showPatchPort(q, CX_PPORT_LEFT, ppl);
    showPatchPort(q, CX_PPORT_RIGHT, ppr);
    showNodePort(q, CX_IF_RIGHT, npidr);
    checkAndShowLLDP(q, CX_STATE_LLDP);
    checkAndShowMAC(q, CX_STATE_MAC);
    setSaveCell();
}

cDPRowPar::~cDPRowPar()
{
    ;
}

QVector<int> cDPRowPar::cid2col;

void cDPRowPar::setHeader()
{
    if (cid2col.isEmpty()) {
        addCid2Col<cid2col>(CID_INDEX            ,  CX_INDEX);
        addCid2Col<cid2col>(CID_STATE_WARNING    ,  CX_STATE_WARNING);
        addCid2Col<cid2col>(CID_STATE_CONFLICT   ,  CX_STATE_CONFLICT);
        addCid2Col<cid2col>(CID_STATE_TAG        ,  CX_STATE_TAG);
        addCid2Col<cid2col>(CID_STATE_MAC        ,  CX_STATE_MAC);
        addCid2Col<cid2col>(CID_STATE_LLDP       ,  CX_STATE_LLDP);
        addCid2Col<cid2col>(CID_SAVE             ,  CX_SAVE);
        addCid2Col<cid2col>(CID_IF_LEFT          ,  CX_IF_LEFT);
        addCid2Col<cid2col>(CID_SHARE_LEFT       ,  CX_SHARE_LEFT);
        addCid2Col<cid2col>(CID_PPORT_LEFT       ,  CX_PPORT_LEFT);
        addCid2Col<cid2col>(CID_TAG_LEFT         ,  CX_TAG_LEFT);
        addCid2Col<cid2col>(CID_TAG_RIGHT        ,  CX_TAG_RIGHT);
        addCid2Col<cid2col>(CID_PPORT_RIGHT      ,  CX_PPORT_RIGHT);
        addCid2Col<cid2col>(CID_SHARE_RIGHT      ,  CX_SHARE_RIGHT);
        addCid2Col<cid2col>(CID_IF_RIGHT         ,  CX_IF_RIGHT);
        if (cid2col.size() != CID_NUMBER) EXCEPTION(EPROGFAIL);     // Last column not hide!
    }
    cDPRow::setHeader(cid2col);
}

QStringList cDPRowPar::exportHeader()
{
    return cDPRow::exportHeader(cid2col);
}

QList<tStringPair> cDPRowPar::exporter()
{
    return cDPRow::exporter(cid2col);
}

/* --- */

cDPRowRep::cDPRowRep(QSqlQuery& q, cDeducePatch *par, int _row, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr)
    : cDPRow(par, _row, CID_HAVE_NO, _ppl, _pll, _ppr, _plr)
{
    if (_row == 0) setHeader();
    showIndex();
    checkAndShowTag(CX_STATE_TAG, CX_TAG_LEFT, CX_TAG_RIGHT);
    calcLinkCheck(q);
    checkAndShowSharing(CX_SHARE_LEFT, CX_SHARE_RIGHT);
    showWarning(CX_STATE_WARNING);
    showConflict(CX_STATE_CONFLICT);
    showNodePort(q, CX_IF_LEFT, npidl);
    showPatchPort(q, CX_PPORT_LEFT, ppl);
    showPatchPort(q, CX_PPORT_RIGHT, ppr, parent->nidr == NULL_ID ? ppr.getFullName(q) : ppr.getName());
    showNodePort(q, CX_IF_RIGHT, npidr);
    checkAndShowLLDP(q, CX_STATE_LLDP);
    checkAndShowMAC(q, CX_STATE_MAC);
}

cDPRowRep::~cDPRowRep()
{
    ;
}

QVector<int> cDPRowRep::cid2col;

void cDPRowRep::setHeader()
{
    if (cid2col.isEmpty()) {
        addCid2Col<cid2col>(CID_INDEX            ,  CX_INDEX);
        addCid2Col<cid2col>(CID_STATE_WARNING    ,  CX_STATE_WARNING);
        addCid2Col<cid2col>(CID_STATE_CONFLICT   ,  CX_STATE_CONFLICT);
        addCid2Col<cid2col>(CID_STATE_TAG        ,  CX_STATE_TAG);
        addCid2Col<cid2col>(CID_STATE_MAC        ,  CX_STATE_MAC);
        addCid2Col<cid2col>(CID_STATE_LLDP       ,  CX_STATE_LLDP);
        addCid2Col<cid2col>(CID_IF_LEFT          ,  CX_IF_LEFT);
        addCid2Col<cid2col>(CID_SHARE_LEFT       ,  CX_SHARE_LEFT);
        addCid2Col<cid2col>(CID_PPORT_LEFT       ,  CX_PPORT_LEFT);
        addCid2Col<cid2col>(CID_TAG_LEFT         ,  CX_TAG_LEFT);
        addCid2Col<cid2col>(CID_TAG_RIGHT        ,  CX_TAG_RIGHT);
        addCid2Col<cid2col>(CID_PPORT_RIGHT      ,  CX_PPORT_RIGHT);
        addCid2Col<cid2col>(CID_SHARE_RIGHT      ,  CX_SHARE_RIGHT);
        addCid2Col<cid2col>(CID_IF_RIGHT         ,  CX_IF_RIGHT);
        if (cid2col.size() != CID_NUMBER) EXCEPTION(EPROGFAIL);     // Last column not hide!
    }
    cDPRow::setHeader(cid2col);
}

QStringList cDPRowRep::exportHeader()
{
    return cDPRow::exportHeader(cid2col);
}

QList<tStringPair> cDPRowRep::exporter()
{
    return cDPRow::exporter(cid2col);
}

/* *** */

const enum ePrivilegeLevel cDeducePatch::rights = PL_OPERATOR;

cDeducePatch::cDeducePatch(QMdiArea *par)
    : cIntSubObj(par)
{
    tableSet = false;
    pUi = new Ui::deducePatch();
    pUi->setupUi(this);
    static const QString sql = "'patch' = SOME (node_type)";
    nidl = nidr = NULL_ID;
    pSelNode  = new cSelectNode(pUi->comboBoxZone,  pUi->comboBoxPlace,  pUi->comboBoxPatch, pUi->lineEditPlacePattern, pUi->lineEditPatchPattern, QString(), sql);
    pSelNode->setPatchInsertButton(pUi->toolButtonPatchAdd);
    pSelNode->setPlaceInsertButton(pUi->toolButtonPlaceAdd);
    pSelNode->setNodeRefreshButton(pUi->toolButtonRefresh);
    pSelNode->setPlaceInfoButton(pUi->toolButtonPlaceInfo);
    pSelNode->setNodeInfoButton(pUi->toolButtonPatchInfo);
    pSelNode->refresh(false);
    pSelNode->setParent(this);
    connect(pSelNode, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(changeNode(qlonglong)));
    pSelNode2 = new cSelectNode(pUi->comboBoxZone2, pUi->comboBoxPlace2, pUi->comboBoxPatch2, pUi->lineEditPlacePattern2, pUi->lineEditPatchPattern2, QString(), sql);
    pSelNode2->setPatchInsertButton(pUi->toolButtonPatchAdd2);
    pSelNode2->setPlaceInsertButton(pUi->toolButtonPlaceAdd2);
    pSelNode2->setNodeRefreshButton(pUi->toolButtonRefresh2);
    pSelNode2->setPlaceInfoButton(pUi->toolButtonPlaceInfo2);
    pSelNode2->setNodeInfoButton(pUi->toolButtonPatchInfo2);
    pSelNode2->refresh(false);
    pSelNode2->setParent(this);
    pModel = new QStandardItemModel;
    pUi->tableView->setModel(pModel);
    connect(pModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(on_tableItem_changed(QStandardItem*)));
    connect(pSelNode2, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(changeNode2(qlonglong)));
    changeMethode(DPM_MAC);
    pUi->radioButtonMAC->setChecked(true);
}

cDeducePatch::~cDeducePatch()
{
    clearTable();
}

void cDeducePatch::setButtons()
{
    bool f = nidl != NULL_ID;
    if (f) {
        switch (methode) {
        case DPM_TAG:
        case DPM_PAR:
            f = nidr != NULL_ID;
            break;
        default:
            break;
        }
    }
    pUi->pushButtonStart->setEnabled(f);
    f = false;
    if (methode != DPM_REP) {
        foreach (cDPRow *p, rows) {
            f = p->isChecked();
            if (f) break;
        }
    }
    pUi->pushButtonSave->setEnabled(f);
}

void cDeducePatch::clearTable()
{
    rows.clear();
    foreach (cDPRow *p, rows) {
        delete p;
    }
    pModel->clear();
}

void cDeducePatch::byLLDP()
{
    tableSet = true;
    clearTable();
    QSqlQuery q  = getQuery();
    QSqlQuery q2 = getQuery();
    cPPort   ppl;   // Left patch port
    cPPort   ppr;   // Right patch port
    cPhsLink pll;   // Left Fron-Term link if exists
    cPhsLink plr;   // Right Fron-Term link if exists
    cLldpLink lldp;

    static const QString _sql = // Mindkét oldal megadva
            "WITH term_front_links AS ("
               " SELECT p.*, l.*"
               " FROM pports AS p"
               " JOIN phs_links AS l ON l.port_id2 = p.port_id  AND phs_link_type1 = 'Term' AND  phs_link_type2 = 'Front' )"
            " SELECT ll.*, lr.*, lldp.*"
            " FROM (SELECT * FROM term_front_links WHERE node_id = ?) AS ll"
            " JOIN lldp_links AS lldp ON ll.port_id1 = lldp.port_id1"
            " JOIN (SELECT * FROM term_front_links WHERE %1) AS lr ON lr.port_id1 = lldp.port_id2"
            " ORDER BY ll.port_index ASC, lr.port_index ASC";
    QString sql = _sql.arg(nidr == NULL_ID ? _sTrue : "node_id = ?");
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, QVariant(nidl));
    if (nidr != NULL_ID) q.bindValue(1, QVariant(nidr));
    _EXECSQL(q);
    if (q.first()) {
        do {
            int i = 0;
            ppl.set(q, &i, ppl.cols());
            pll.set(q, &i, pll.cols());
            ppr.set(q, &i, ppr.cols());
            plr.set(q, &i, plr.cols());
            lldp.set(q, &i, lldp.cols());
            rows << new cDPRowLLDP(q2, this, rows.size(), lldp, ppl, pll, ppr, plr);
        } while (q.next());
        pUi->tableView->setSortingEnabled(true);
        pUi->tableView->sortByColumn(cDPRowMAC::CX_INDEX, Qt::AscendingOrder);
    }
    else {
        pModel->setHorizontalHeaderItem(0, new QStandardItem(QIcon(cDPRow::sIHelp), _sNul));
        pModel->setItem(0, 0, new QStandardItem(trUtf8("Az LLDP alapján nincs találat.")));
        pUi->tableView->setSortingEnabled(false);
    }
    pUi->tableView->resizeColumnsToContents();
    tableSet = false;
    setButtons();;
}


void cDeducePatch::byMAC()
{
    tableSet = true;
    clearTable();
    QSqlQuery q  = getQuery();
    QSqlQuery q2 = getQuery();
    cPPort   ppl;   // Local patch port
    cPhsLink pll;   // Local Front link (left)
    cPhsLink plr;   // Remote Front link (right)
    cPPort   ppr;   // Remote patch port
    cMacTab  mt;
    static const QString sql =
            "WITH x AS ("
                "WITH ppl AS (SELECT * FROM pports WHERE node_id = ?)"  // Tested ports (local/left)
                " SELECT "          // Find local nodes from remote switch address table
                    "ppl.*,"                // local patch port
                    "pll.*,"                // local patch port Front link
                    "plr.*,"                // remote patch port Front Link
                    "ppr.*,"                // remote patch port
                    "mt.*,"                 // mactab record
                    "min_shared(min_shared(ppl.shared_cable, pll.port_shared), min_shared(ppr.shared_cable, plr.port_shared)) AS msh"
                " FROM ppl"
                " JOIN phs_links  AS pll ON ppl.port_id = pll.port_id2"
                " JOIN interfaces AS ifl ON pll.port_id1 = ifl.port_id"    // Local interface -> MAC
                " JOIN mactab     AS mt  ON ifl.hwaddress = mt.hwaddress"  // MAC -> mactab
                " JOIN phs_links  AS plr ON mt.port_id   = plr.port_id1"   // mactab -> remote link
                " JOIN pports     AS ppr ON plr.port_id2 = ppr.port_id"
                " WHERE pll.phs_link_type2 = 'Front'"
               " UNION"
                " SELECT "          // Find remote nodes from local switch address table
                    "ppl.*,"                // local patch port
                    "pll.*,"                // local patch port Front link
                    "plr.*,"                // remote patch port Front Link
                    "ppr.*,"                // remote patch port
                    "mt.*,"                 // mactab record
                    "min_shared(min_shared(ppl.shared_cable, pll.port_shared), min_shared(ppr.shared_cable, plr.port_shared)) AS msh"
                " FROM ppl"
                " JOIN phs_links  AS pll ON ppl.port_id = pll.port_id2"
                " JOIN interfaces AS swp ON pll.port_id1 = swp.port_id"    // Local interface, switch port
                " JOIN mactab     AS mt  ON swp.port_id  = mt.port_id"     // mactab records
                " JOIN interfaces AS ifr ON mt.hwaddress = ifr.hwaddress"  // remote interface
                " JOIN phs_links  AS plr ON ifr.port_id  = plr.port_id1"
                " JOIN pports     AS ppr ON plr.port_id2 = ppr.port_id"
                " WHERE pll.phs_link_type2 = 'Front'"
            ") SELECT * FROM x WHERE msh <> 'NC'"
            " ORDER BY msh ASC";
    if (execSql(q, sql, nidl)) {
        do {
            int i = 0;
            ppl.set(q, &i, ppl.cols());
            pll.set(q, &i, pll.cols());
            plr.set(q, &i, plr.cols());
            ppr.set(q, &i, ppr.cols());
            mt. set(q, &i, mt.cols());
            rows << new cDPRowMAC(q2, this, rows.size(), mt, ppl, pll, ppr, plr);
        } while (q.next());
        pUi->tableView->setSortingEnabled(true);
        pUi->tableView->sortByColumn(cDPRowMAC::CX_INDEX, Qt::AscendingOrder);
    }
    else {
        pModel->setHorizontalHeaderItem(0, new QStandardItem(QIcon(cDPRow::sIHelp), _sNul));
        pModel->setItem(0, 0, new QStandardItem(trUtf8("A címtáblák alapján nincs találat.")));
        pUi->tableView->setSortingEnabled(false);
    }
    pUi->tableView->resizeColumnsToContents();
    tableSet = false;
    setButtons();
}

void cDeducePatch::byTag()
{
    tableSet = true;
    clearTable();
    QSqlQuery q  = getQuery();
    QSqlQuery q2 = getQuery();
    cPPort   ppl;   // Left patch port
    cPPort   ppr;   // Right patch port
    cPhsLink pll;   // Left Fron-Term link if exists
    cPhsLink plr;   // Right Fron-Term link if exists

    static const QString sql =
            "WITH term_front_links AS ("
               " SELECT p.*, l.*"
               " FROM pports AS p"
               " LEFT OUTER JOIN phs_links AS l ON l.port_id2 = p.port_id  AND phs_link_type1 = 'Term' AND  phs_link_type2 = 'Front' )"
            " SELECT ll.*, lr.*"
            " FROM (SELECT * FROM term_front_links WHERE node_id = ?) AS ll"
            " JOIN (SELECT * FROM term_front_links WHERE node_id = ?) AS lr USING(port_tag)"
            " ORDER BY ll.port_index ASC, lr.port_index ASC";
    if (execSql(q, sql, nidl, nidr)) {
        do {
            int i = 0;
            ppl.set(q, &i, ppl.cols());
            pll.set(q, &i, pll.cols());
            ppr.set(q, &i, ppr.cols());
            plr.set(q, &i, plr.cols());
            rows << new cDPRowTag(q2, this, rows.size(), ppl, pll, ppr, plr);
        } while (q.next());
        pUi->tableView->setSortingEnabled(true);
        pUi->tableView->sortByColumn(cDPRowMAC::CX_INDEX, Qt::AscendingOrder);
    }
    else {
        pModel->setHorizontalHeaderItem(0, new QStandardItem(QIcon(cDPRow::sIHelp), _sNul));
        pModel->setItem(0, 0, new QStandardItem(trUtf8("Nincs egy megegyező cimke sem.")));
        pUi->tableView->setSortingEnabled(false);
    }
    pUi->tableView->resizeColumnsToContents();
    tableSet = false;
    setButtons();
}

void cDeducePatch::paralel()
{
    int offl = pUi->spinBoxOffset->value();
    int offr = pUi->spinBoxOffset2->value();
    tableSet = true;
    clearTable();
    QSqlQuery q  = getQuery();
    QSqlQuery q2 = getQuery();
    cPPort   ppl;   // Left patch port
    cPPort   ppr;   // Right patch port
    cPhsLink pll;   // Left Fron-Term link if exists
    cPhsLink plr;   // Right Fron-Term link if exists

    static const QString sql =
            "WITH term_front_links AS ("
               " SELECT p.*, l.*"
               " FROM pports AS p"
               " LEFT OUTER JOIN phs_links AS l ON l.port_id2 = p.port_id  AND phs_link_type1 = 'Term' AND  phs_link_type2 = 'Front' )"
            " SELECT ll.*, lr.*"
            " FROM (SELECT * FROM term_front_links WHERE node_id = ?) AS ll"
            " JOIN (SELECT * FROM term_front_links WHERE node_id = ?) AS lr ON ll.port_index + ? = lr.port_index + ?"
            " ORDER BY ll.port_index ASC, lr.port_index ASC";
    if (execSql(q, sql, nidl, nidr, offl, offr)) {
        do {
            int i = 0;
            ppl.set(q, &i, ppl.cols());
            pll.set(q, &i, pll.cols());
            ppr.set(q, &i, ppr.cols());
            plr.set(q, &i, plr.cols());
            rows << new cDPRowPar(q2, this, rows.size(), ppl, pll, ppr, plr);
        } while (q.next());
        pUi->tableView->setSortingEnabled(true);
        pUi->tableView->sortByColumn(cDPRowMAC::CX_INDEX, Qt::AscendingOrder);
    }
    else {
        pModel->setHorizontalHeaderItem(0, new QStandardItem(QIcon(cDPRow::sIHelp), _sNul));
        pModel->setItem(0, 0, new QStandardItem(trUtf8("Nincs egy megegyező cimke sem.")));
        pUi->tableView->setSortingEnabled(false);
    }
    pUi->tableView->resizeColumnsToContents();
    tableSet = false;
    setButtons();
}

void cDeducePatch::report()
{
    tableSet = true;
    clearTable();
    QSqlQuery q  = getQuery();
    QSqlQuery q2 = getQuery();
    cPPort   ppl;   // Left patch port
    cPPort   ppr;   // Right patch port
    cPhsLink pll;   // Left Fron-Term link if exists
    cPhsLink plr;   // Right Fron-Term link if exists

    static const QString _sql =
            "WITH term_front_links AS ("
               " SELECT p.*, l.*"
               " FROM pports AS p"
               " LEFT OUTER JOIN phs_links AS l ON l.port_id2 = p.port_id  AND phs_link_type1 = 'Term' AND  phs_link_type2 = 'Front' )"
            " SELECT ll.*, lr.*"
            " FROM term_front_links AS ll"
            " JOIN phs_links        AS pl ON pl.port_id1 = ll.port_id"
            " JOIN term_front_links AS lr ON pl.port_id2 = lr.port_id"
            " WHERE ll.node_id = ? AND %1 pl.phs_link_type1 = 'Back' AND pl.phs_link_type2 = 'Back' "
            " ORDER BY ll.port_index ASC";
    QString sql = _sql.arg(nidr == NULL_ID ? _sNul : "lr.node_id = ? AND");
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, QVariant(nidl));
    if (nidr != NULL_ID) q.bindValue(1, QVariant(nidr));
    _EXECSQL(q);
    if (q.first()) {
        do {
            int i = 0;
            ppl.set(q, &i, ppl.cols());
            pll.set(q, &i, pll.cols());
            ppr.set(q, &i, ppr.cols());
            plr.set(q, &i, plr.cols());
            rows << new cDPRowRep(q2, this, rows.size(), ppl, pll, ppr, plr);
        } while (q.next());
        pUi->tableView->setSortingEnabled(true);
        pUi->tableView->sortByColumn(cDPRowMAC::CX_INDEX, Qt::AscendingOrder);
    }
    else {
        pModel->setHorizontalHeaderItem(0, new QStandardItem(QIcon(cDPRow::sIHelp), _sNul));
        pModel->setItem(0, 0, new QStandardItem(trUtf8("Nincs egy megegyező cimke sem.")));
        pUi->tableView->setSortingEnabled(false);
    }
    pUi->tableView->resizeColumnsToContents();
    tableSet = false;
    setButtons();
}


bool cDeducePatch::findLink(cPhsLink& pl)
{
    foreach (cDPRow *p, rows) {
        if (pl.compare(p->phsLink)) return true;
    }
    return false;
}
void cDeducePatch::changeMethode(eDeducePatchMeth _met)
{
    methode = _met;
    bool show_ix = methode == DPM_PAR;
    pUi->labelMinIndex->setVisible(show_ix);
    pUi->labelMinIndex2->setVisible(show_ix);
    pUi->spinBoxOffset->setVisible(show_ix);
    pUi->spinBoxOffset2->setVisible(show_ix);
    pSelNode2->setEnabled(methode != DPM_MAC);
    clearTable();
    setButtons();
}

void cDeducePatch::changeNode(qlonglong id)
{
    nidl = id;
    clearTable();
    setButtons();
}

void cDeducePatch::changeNode2(qlonglong id)
{
    nidr = id;
    clearTable();
    setButtons();
}

void cDeducePatch::on_radioButtonLLDP_pressed()
{
    changeMethode(DPM_LLDP);
}

void cDeducePatch::on_radioButtonMAC_pressed()
{
    changeMethode(DPM_MAC);
}

void cDeducePatch::on_radioButtonTag_pressed()
{
    changeMethode(DPM_TAG);
}

void cDeducePatch::on_radioButtonParalel_pressed()
{
    changeMethode(DPM_PAR);
}

void cDeducePatch::on_radioButtonReport_pressed()
{
    changeMethode(DPM_REP);
}

void cDeducePatch::on_pushButtonStart_clicked()
{
    bool f = (nidl != NULL_ID) && (methode != DPM_TAG || nidr != NULL_ID);
    if (!f) {
        EXCEPTION(EPROGFAIL);
    }
    switch (methode) {
    case DPM_LLDP:  byLLDP();   break;
    case DPM_MAC:   byMAC();    break;
    case DPM_TAG:   byTag();    break;
    case DPM_PAR:   paralel();  break;
    case DPM_REP:   report();   break;
    default:        EXCEPTION(EPROGFAIL);
    }
    setButtons();
}

void cDeducePatch::on_pushButtonSave_clicked()
{
    QString msg = trUtf8(
                "Valóban menti a kijelölt linkeket?\n"
                "Az ütköző linkek automatikusan törlődnek."
                "A művelet visszavonására nincs legetőség.");
    if (QMessageBox::Ok != QMessageBox::warning(this, dcViewShort(DC_WARNING), msg, QMessageBox::Ok, QMessageBox::Cancel)) {
        return;
    }
    QSqlQuery q = getQuery();
    cError *pe = nullptr;
    QString tr = "deduce_patch";
    try {
        sqlBegin(q, tr);
        foreach (cDPRow *pRow, rows) {
            pRow->save(q);
        }
        sqlCommit(q, tr);
    }
    CATCHS(pe)
    if (pe != nullptr) {
        sqlRollback(q, tr);
        cErrorMessageBox::messageBox(pe, this);
        delete pe;
    }
    on_pushButtonStart_clicked();   // Refresh
}

void cDeducePatch::on_tableItem_changed(QStandardItem * pi)
{
    (void)pi;
    if (!tableSet) setButtons();
}


void cDeducePatch::on_toolButtonCopyZone_clicked()
{
    qlonglong zid = pSelNode->currentZoneId();
    pSelNode2->setCurrentZone(zid);
}

void cDeducePatch::on_toolButtonCopyPlace_clicked()
{
    qlonglong pid = pSelNode->currentPlaceId();
    pSelNode2->setCurrentPlace(pid);
}

void cDeducePatch::on_pushButtonReport_clicked()
{
    if (rows.size() == 0) return;
    QStringList header = rows.first()->exportHeader();
    QMap<int, cDPRow *> rowsMap;
    cDPRow * pRow;
    foreach (pRow, rows) {
        rowsMap[pRow->row()] = pRow;
    }
    QList<int>  rowIndexs = rowsMap.keys();
    std::sort(rowIndexs.begin(), rowIndexs.end());
    QList<QStringList> table;
    foreach (int i, rowIndexs) {
        QList<tStringPair> slp = rowsMap[i]->exporter();
        // Nem tudjuk (még?) megjeleníteni a tool tip stringeket, eltávolítjuk
        QStringList tableRow;
        foreach (tStringPair p, slp) {
            tableRow << p.first;
        }
        table << tableRow;
    }
    QString html = htmlTable(header, table, true);
    QString title;
    switch (methode) {
    case DPM_LLDP:
        title = trUtf8("Fali kábel felfedezés LDAP alapján: %1 - %2").arg(pSelNode->currentNodeName(), pSelNode2->currentNodeName());
        break;
    case DPM_MAC:
        title = trUtf8("Fali kábel felfedezés címtáblák alapján alapján: %1").arg(pSelNode->currentNodeName());
        break;
    case DPM_TAG:
        title = trUtf8("Fali kábel felfedezés cimkék alapján: %1 - %2").arg(pSelNode->currentNodeName(), pSelNode2->currentNodeName());
        break;
    case DPM_PAR:
        title = trUtf8("Patch panel hátlap 1-1 beközés: %1 - %2").arg(pSelNode->currentNodeName(), pSelNode2->currentNodeName());
        break;
    case DPM_REP:
        title = trUtf8("Patch panel bekötés riport: %1 - %2").arg(pSelNode->currentNodeName(), pSelNode2->currentNodeName());
        break;
    default:
        EXCEPTION(EPROGFAIL);
        break;
    }
    popupReportWindow(this, html, title);
}
