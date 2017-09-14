#include "deducepatch.h"
#include "report.h"
#include "lv2widgets.h"


/// Egy MAC keresése a port-címtáblában. és a link rekord beolvasása
/// @param mac A keresett MAC cím.
/// @param mt A talált port címtábla rekord objektum, output, ha nincs találat, akkor üres.
/// @param pl A fizikai link rekord, ami a talált port-címtábla rekordban hivatkozott porthoz (ID) tartozik
/// @return true, ha van találat a port címtábla rekordban, és a hivatkozott porthoz van fizikai link (patch) is, és az front oldali.
static bool findMac(QSqlQuery& q, const cMac& mac, cMacTab& mt, cPhsLink& pl)
 {
    mt.clear();
    pl.clear();
    if (mac.isEmpty()) return false;        // Nincs magadva MAC (NULL)
    mt.setMac(_sHwAddress, mac);
    if (!mt.completion(q)) return false;    // Nincs találat a címtáblában
    qlonglong pid = mt.getId(_sPortId);
    pl.setId(_sPortId1, pid);
    if (1 != pl.completion(q)) return false;// Fizikai link, ha van
    return pl.getId(_sPhsLinkType2) == LT_FRONT;    // A switch portnak a patch front oldali csatlakozása?
}

static eTristate checkMac(QSqlQuery& q, cInterface& if1, cInterface& if2)
{
    cMac mac = if1.getMac(_sHwAddress);
    if (!mac.isValid()) return TS_NULL;
    cMacTab mt;
    if (mt.setMac(_sHwAddress, mac).completion(q) != 1) return TS_NULL;
    if (mt.getId(_sPortId) == if2.getId()) return TS_TRUE;
    return  TS_FALSE;
}

/* *** */

enum eFieldIx {
    CX_NPORT_LEFT, CX_SHARE_LEFT, CX_PPORT_LEFT,
    CX_STATE, CX_SAVE,
    CX_PPORT_RIGHT, CX_SHARE_RIGHT, CX_NPORT_RIGHT,
    CX_TIMES
};

cDPRow::cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, cMacTab &mt, cNPort &il, cPPort& ppl, cPhsLink& pll, cPhsLink& plr)
    :QObject(par), parent(par), pTable(par->pUi->tableWidget), row(_row)
{
    pCheckBox = NULL;
    cPPort ppr;     // Jobb oldali patch-port
    ppr.setById(q, plr.getId(_sPortId2));
    ePortShare sh = (ePortShare)ppl.getId(_sSharedCable);   // Ez nem a rekord beli share, hanem az eredő (ld.: nextLink() !)
    (void)sh;       //
    ppl.setById(q); // újraolvassuk

    pTable->setRowCount(row +1);

    phsLink.setId(_sPortId1, ppl.getId());
    phsLink.setId(_sPhsLinkType1, LT_BACK);
    phsLink.setId(_sPortId2, ppr.getId());
    phsLink.setId(_sPhsLinkType2, LT_BACK);
    phsLink.setId(_sPortShared, ES_);   // back-back nincs (nem lehet) megosztás a rekordban
    bool reapeat = parent->findLink(phsLink);
    bool exists;
    QString colMsg;
    bool colision;
    colision = linkColisionTest(q, exists, phsLink, colMsg);
    // Hátlapi megosztésok ellenörzése:
    ePortShare ppshl = (ePortShare)ppl.getId(_sSharedCable);
    ePortShare ppshr = (ePortShare)ppr.getId(_sSharedCable);
    bool bBckShMism = ppshl != ppshr; // Ha nem OK (nem biztos, hogy hiba, operátor ellenőrizze!)

    qlonglong spidl = ppl.getId(_sSharedPortId);
    qlonglong spidr = ppr.getId(_sSharedPortId);
    bool bBckShared = spidl != NULL_ID || spidr != NULL_ID; // Mégosztott, másodlagos port
    // stat...
    QString state = QString("<b><u>MAC</u></b> ");
    QString sToolTip = htmlInfo(trUtf8("A port címtáblák alapján talált hátlapi csatlakozás."));
    // A két patch porton azonos a cimke ?
    QString pptagl = ppl.getName(_sPortTag);
    QString pptagr = ppr.getName(_sPortTag);
    bool bTag  = pptagl.compare(pptagr, Qt::CaseInsensitive);
    if (bTag) {
        state    += "<b>TAG</b> ";
        sToolTip += htmlInfo(QObject::trUtf8("A patch port cimkék azonosak : '%1'.").arg(pptagl.toHtmlEscaped()));
    }
    else {
        state    += "<s>TAG</s> ";
        sToolTip += htmlInfo(QObject::trUtf8("Nincsenek patch port cimkék, vagy nem nem azonosak : '%1' &ne; '%2'.")
                             .arg(pptagl.toHtmlEscaped(), pptagr.toHtmlEscaped()));
    }
    // Van LLDP link a két végpont között?
    bool bLLDP = cLldpLink().isLinked(q, il.getId(), plr.getId(_sPortId1));
    if (bLLDP) {
        state    += "<b>LLDP</b> ";
        sToolTip += htmlInfo(QObject::trUtf8("A cím tábla értékhez van konzisztens LLDP link."));
    }
    else {
        state    += "<s>LLDP</s> ";
        sToolTip += htmlInfo(QObject::trUtf8("A cím tábla értékhez nincs konzisztens LLDP link."));
    }
    if (bBckShMism) {
        sToolTip += htmlWarning(trUtf8("Ellenőrizze a megosztásokat!"));
        state += "<font color=\"red\"><b>!</b></font>";
    }
    if (bBckShared) {
        sToolTip += htmlError(trUtf8("A hátlapi megosztáshoz tartozó kapcsolat, nem tartozhat hozzá link rekord."));
    }
    if (colision) {
        sToolTip += colMsg;
    }

    QString s;
    //...
    QTextEdit *pTextEdit = new QTextEdit(state);
    pTextEdit->setReadOnly(true);
    pTextEdit->setToolTip(sToolTip);
    pTable->setCellWidget(row, CX_STATE, pTextEdit);

    QTableWidgetItem *pi;

    pi = new QTableWidgetItem(il.getFullName(q));
    pTable->setItem(row, CX_NPORT_LEFT, pi);

    pi = new QTableWidgetItem(pll.getName(_sPortShared));
    pTable->setItem(row, CX_SHARE_LEFT, pi);

    s = ppl.getName();
    if (ppshl != ES_) {
        if (ppshl == ES_A) s += " (A)";
        else               s += " (" + ppl.view(q, __sSharedPortId) + " <- " + portShare(ppshl) + ")";
    }
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_PPORT_LEFT, pi);


    if (exists || reapeat) {
        pi = new QTableWidgetItem(reapeat ? "=" : _sOk);
        pTable->setItem(row, CX_SAVE, pi);
    }
    else {
        if (bBckShared) {
            pi = new QTableWidgetItem("-");
            pTable->setItem(row, CX_SAVE, pi);
        }
        else {
            pCheckBox = new QCheckBox();
            bool checked = !(colision || bBckShMism);
            pCheckBox->setChecked(checked);
            pTable->setCellWidget(row, CX_SAVE, pCheckBox);
            connect(pCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxchange(bool)));
        }
    }

    s = ppr.getFullName(q);
    if (ppshr != ES_) {
        if (ppshr == ES_A) s += " (A)";
        else               s += " (" + ppr.view(q, __sSharedPortId) + " <- " + portShare(ppshr) + ")";
    }
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_PPORT_RIGHT, pi);

    pi = new QTableWidgetItem(plr.getName(_sPortShared));
    pTable->setItem(row, CX_SHARE_RIGHT, pi);

    pi = new QTableWidgetItem(cNPort().getFullNameById(q, plr.getId(_sPortId1)));
    pTable->setItem(row, CX_NPORT_RIGHT, pi);

    s = mt.getName(_sFirstTime) + " < " + mt.getName(_sLastTime);
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_TIMES, pi);

}

cDPRow::cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, bool unique, cPPort& ppl, cPPort& ppr)
    : QObject(par), parent(par), pTable(par->pUi->tableWidget), row(_row)
{
    pCheckBox = NULL;
    pTable->setRowCount(row +1);

    phsLink.setId(_sPortId1, ppl.getId());
    phsLink.setId(_sPhsLinkType1, LT_BACK);
    phsLink.setId(_sPortId2, ppr.getId());
    phsLink.setId(_sPhsLinkType2, LT_BACK);
    phsLink.setId(_sPortShared, ES_);   // back-back nincs (nem lehet) megosztás a rekordban
    bool exists;
    QString colMsg;
    bool colision;
    colision = linkColisionTest(q, exists, phsLink, colMsg);
    // Hátlapi megosztésok ellenörzése:
    ePortShare ppshl = (ePortShare)ppl.getId(_sSharedCable);
    ePortShare ppshr = (ePortShare)ppr.getId(_sSharedCable);
    bool bBckShMism = ppshl != ppshr; // Ha nem OK (nem biztos, hogy hiba, operátor ellenőrizze!)

    qlonglong spidl = ppl.getId(_sSharedPortId);
    qlonglong spidr = ppr.getId(_sSharedPortId);
    bool bBckShared = spidl != NULL_ID || spidr != NULL_ID; // Mégosztott, másodlagos port
    // stat...
    QString state = QString("<b><u>TAG</u></b> ");
    QString sToolTip = htmlInfo(trUtf8("A port cimkék alapján talált hátlapi csatlakozás."));
    // végponok
    cPhsLink pll, plr;
    // A MAC alapján?
    cInterface ifl, ifr;
    bool fl, fr;
    fl = pll.setId(_sPortId1, ppl.getId()).completion(q) == 1;
    fr = plr.setId(_sPortId1, ppr.getId()).completion(q) == 1;
    fl = fl && ifl.fetchById(q, pll.getId(_sPortId2));
    fr = fr && ifr.fetchById(q, plr.getId(_sPortId2));
    eTristate bMac = TS_NULL;   // Nincs mit összehasonlitani
    if (fl && fr) { // mindkét végpont megvan, és interfészek
        bMac = checkMac(q, ifl, ifr);
        if (bMac == TS_NULL) bMac = checkMac(q, ifr, ifl);
    }
    switch (bMac) {
    case TS_NULL:
        state    += "<s>MAC</s> ";
        sToolTip += htmlInfo(QObject::trUtf8("A MAC táblában nincs találat."));
        break;
    case TS_TRUE:
        state    += "<b>MAC</b> ";
        sToolTip += htmlInfo(QObject::trUtf8("Találat a MAC táblában is."));
        break;
    case TS_FALSE:
        state    += "<font color=\"red\"><s>MAC</s></font> ";
        sToolTip += htmlInfo(QObject::trUtf8("Találat a MAC táblában, de más linkre."));
        break;
    }

    // Van LLDP link a két végpont között?
    bool bLLDP = false;
    if (fl && fr) {
        cLldpLink().isLinked(q, ifl.getId(), ifr.getId(_sPortId1));
    }
    if (bLLDP) {
        state    += "<b>LLDP</b> ";
        sToolTip += htmlInfo(QObject::trUtf8("A cím tábla értékhez van konzisztens LLDP link."));
    }
    else {
        state    += "<s>LLDP</s> ";
        sToolTip += htmlInfo(QObject::trUtf8("A cím tábla értékhez nincs konzisztens LLDP link."));
    }
    if (bBckShMism) {
        sToolTip += htmlWarning(trUtf8("Ellenőrizze a megosztásokat!"));
        state += "<font color=\"red\"><b>!</b></font>";
    }
    if (bBckShared) {
        sToolTip += htmlError(trUtf8("A hátlapi megosztáshoz tartozó kapcsolat, nem tartozhat hozzá link rekord."));
    }
    if (colision) {
        sToolTip += colMsg;
    }

    QString s;
    //...
    QTextEdit *pTextEdit = new QTextEdit(state);
    pTextEdit->setReadOnly(true);
    pTextEdit->setToolTip(sToolTip);
    pTable->setCellWidget(row, CX_STATE, pTextEdit);

    QTableWidgetItem *pi;

    pi = new QTableWidgetItem(ifl.getFullName(q, EX_IGNORE));
    pTable->setItem(row, CX_NPORT_LEFT, pi);

    pi = new QTableWidgetItem(pll.getName(_sPortShared));
    pTable->setItem(row, CX_SHARE_LEFT, pi);

    s = ppl.getName();
    if (ppshl != ES_) {
        if (ppshl == ES_A) s += " (A)";
        else               s += " (" + ppl.view(q, __sSharedPortId) + " <- " + portShare(ppshl) + ")";
    }
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_PPORT_LEFT, pi);


    if (exists) {
        pi = new QTableWidgetItem(_sOk);
        pTable->setItem(row, CX_SAVE, pi);
    }
    else {
        if (bBckShared) {
            pi = new QTableWidgetItem("-");
            pTable->setItem(row, CX_SAVE, pi);
        }
        else {
            pCheckBox = new QCheckBox();
            bool checked = !(colision || bBckShMism || unique) || bMac != TS_TRUE;
            pCheckBox->setChecked(checked);
            pTable->setCellWidget(row, CX_SAVE, pCheckBox);
            connect(pCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkBoxchange(bool)));
        }
    }

    s = ppr.getFullName(q);
    if (ppshr != ES_) {
        if (ppshr == ES_A) s += " (A)";
        else               s += " (" + ppr.view(q, __sSharedPortId) + " <- " + portShare(ppshr) + ")";
    }
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_PPORT_RIGHT, pi);

    pi = new QTableWidgetItem(plr.getName(_sPortShared));
    pTable->setItem(row, CX_SHARE_RIGHT, pi);

    pi = new QTableWidgetItem(ifr.getFullName(q, EX_IGNORE));
    pTable->setItem(row, CX_NPORT_RIGHT, pi);
}


void cDPRow::checkBoxchange(bool f)
{
    if (f) parent->pUi->pushButtonSave->setEnabled(true);
    else   parent->setButtons();
}

/* --- */

const enum ePrivilegeLevel cDeducePatch::rights = PL_OPERATOR;

cDeducePatch::cDeducePatch(QMdiArea *par)
    :cIntSubObj(par)
{
    pUi = new Ui::deducePatch();
    pUi->setupUi(this);
    static const QString sql = "'patch' = SOME (node_type)";
    nid = nid2 = NULL_ID;
    pSelNode  = new cSelectNode(pUi->comboBoxZone,  pUi->comboBoxPlace,  pUi->comboBoxPatch,NULL,  NULL, QString(), sql);
    pSelNode->setParent(this);
    connect(pSelNode, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(changeNode(qlonglong)));
    pSelNode2 = new cSelectNode(pUi->comboBoxZone2, pUi->comboBoxPlace2, pUi->comboBoxPatch2, NULL, NULL, QString(), sql);
    pSelNode2->setParent(this);
    connect(pSelNode2, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(changeNode2(qlonglong)));

    connect(pUi->radioButtonLLDP, SIGNAL(pressed()), this, SLOT(setMethodeLLDP()));
    connect(pUi->radioButtonMAC,  SIGNAL(pressed()), this, SLOT(setMethodeMAC()));
    connect(pUi->radioButtonTag,  SIGNAL(pressed()), this, SLOT(setMethodeTag()));

    connect(pUi->pushButtonStart,SIGNAL(clicked()),  this, SLOT(start()));
    connect(pUi->pushButtonSave, SIGNAL(clicked()),  this, SLOT(save()));

    methode = DPM_MAC;
    pUi->radioButtonMAC->setChecked(true);
}

cDeducePatch::~cDeducePatch()
{
    clearTable();
}

void cDeducePatch::setButtons()
{
    bool f;
    f = (nid != NULL_ID) && (methode != DPM_TAG || nid2 != NULL_ID);
    pUi->pushButtonStart->setEnabled(f);
    f = false;
    for (int i = 0; i < rows.size(); ++i) {
        QCheckBox *pcb = rows.at(i)->pCheckBox;
        f = pcb != NULL && pcb->isChecked();
        if (f) break;
    }
    pUi->pushButtonSave->setEnabled(f);
}

void cDeducePatch::clearTable()
{
    foreach (cDPRow *p, rows) {
        delete p;
    }
    rows.clear();
    pUi->tableWidget->setRowCount(0);
}

void cDeducePatch::byLLDP(QSqlQuery& q)
{
    pUi->tableWidget->setColumnHidden(CX_TIMES, false);
    (void)q;
}

void cDeducePatch::byMAC(QSqlQuery& q)
{
    pUi->tableWidget->setColumnHidden(CX_TIMES, false);
    cMac mac;
    cMacTab mt;
    cPhsLink pl1, pl2;
    QListIterator<cNPort *> i(patch.ports);
    while (i.hasNext()) {
        cPPort *pp = i.next()->reconvert<cPPort>();
        pl1.clear();
        while (pl1._isNull() || pl1.getId(_sPortShared) == ES_A) {
            pl1.nextLink(q, pp->getId(), LT_BACK, ES_);     // Előlap felé (hátlaptól)
            qlonglong pid = pl1.getId(_sPortId2);           // A linkelt port ID
            if (pid == NULL_ID) continue;                   // Nincs link
            cNPort *pnp = cNPort::getPortObjById(q, pid);   // A linkelt port objektum
            if (pnp->tableoid() == cNPort::tableoid_pports()) {
                delete pnp;
                continue;   // Ez egy patch port, bonyi, talán mmajd késöbb
            }
            if (pnp->tableoid() == cNPort::tableoid_nports()) {
                delete pnp;
                continue;   // passzív portnak nincs MAC-je
            }
            // interface
            mac = pnp->getMac(_sHwAddress);    // A MAC-je
            if (findMac(q, mac, mt, pl2)) {    // Keressük a port-cím táblában, és az ellenoldali linket
                rows << new cDPRow(q, this, rows.size(), mt, *pnp, *pp, pl1, pl2);
            }
            delete pnp;
        }
    }
}

void cDeducePatch::byTag(QSqlQuery &q)
{
    pUi->tableWidget->setColumnHidden(CX_TIMES, true);
    cPatch pr;      // Jobb oldali patch panel
    int ixTag = cPPort().toIndex(_sPortTag);
    pr.setById(q, nid2);
    pr.fetchPorts(q);
    cPPort *ppl;    // Bal oldali patch port
    cPPort *ppr;    // Jobb oldali patch port
    QString tag;
    bool unique;
    int ix;
    QListIterator<cNPort *> i(patch.ports);
    for (int i = 0; i < patch.ports.size(); ++i ) {
        ppl = patch.ports.at(i)->reconvert<cPPort>();
        tag = ppl->getName(ixTag);
        if (tag.isEmpty()) continue;    // ha nincs cimke
        unique = 0 > patch.ports.indexOf(ixTag, tag, i +1); // egyedi cimke a bal oldalon
        ix = 0;
        while (0 <= (ix = pr.ports.indexOf(ixTag, tag, ix))) {
            unique = unique && (0 > pr.ports.indexOf(ixTag, tag, ix +1));  // Több találat ugyanarra a cimkére
            ppr = pr.ports.at(ix)->reconvert<cPPort>();
            rows << new cDPRow(q, this, rows.size(), unique, *ppl, *ppr);
            ++ix;
        }
    }

}

bool cDeducePatch::findLink(cPhsLink& pl)
{
    foreach (cDPRow *p, rows) {
        if (pl.compare(p->phsLink)) return true;
    }
    return false;
}

void cDeducePatch::changeNode(qlonglong id)
{
    nid = id;
    clearTable();
    setButtons();
}

void cDeducePatch::changeNode2(qlonglong id)
{
    nid2 = id;
    clearTable();
    setButtons();
}

void cDeducePatch::start()
{
    QSqlQuery q = getQuery();
    bool f = (nid != NULL_ID) && (methode != DPM_TAG || nid2 != NULL_ID);
    if (!f) {
        EXCEPTION(EPROGFAIL);
    }
    clearTable();
    patch.setById(q, nid);
    patch.fetchPorts(q);
    patch.sortPortsByIndex();
    switch (methode) {
    case DPM_LLDP:  byLLDP(q);  break;
    case DPM_MAC:   byMAC(q);   break;
    case DPM_TAG:   byTag(q);   break;
    default:        EXCEPTION(EPROGFAIL);
    }
    pUi->tableWidget->resizeColumnsToContents();
    setButtons();
}

void cDeducePatch::save()
{
    QString msg = trUtf8(
                "Valóban menti a kijelölt linkeket?\n"
                "Az ütköző linkek autómatikusan törlődnek."
                "A művelet visszavonására nincs legetőség.");
    if (QMessageBox::Ok != QMessageBox::warning(this, dcViewShort(DC_WARNING), msg, QMessageBox::Ok, QMessageBox::Cancel)) {
        return;
    }
    QSqlQuery q = getQuery();
    cError *pe = NULL;
    QString tr = "deduce_patch";
    try {
        sqlBegin(q, tr);
        foreach (cDPRow *pRow, rows) {
            QCheckBox *pCB = pRow->pCheckBox;
            if (pCB == NULL || pCB->isChecked() == false) continue;
            pRow->phsLink.replace(q);
        }
        sqlCommit(q, tr);
    }
    CATCHS(pe)
    if (pe != NULL) {
        sqlRollback(q, tr);
        cErrorMessageBox::messageBox(pe, this);
        delete pe;
    }
    start();
}
