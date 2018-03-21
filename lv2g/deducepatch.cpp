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

static eTristate checkMac(QSqlQuery& q, cNPort& p1, cNPort& p2, bool first = true)
{
    cMac mac = p1.getMac(_sHwAddress, EX_IGNORE);   // MAC, ha van
    if (mac.isValid()) {
        cMacTab mt;                                 // Melyik címtáblában
        if (mt.setMac(_sHwAddress, mac).completion(q) == 1) {
            if (mt.getId(_sPortId) == p2.getId()) return TS_TRUE;
            return  TS_FALSE;
        }
    }
    return first ? checkMac(q, p2, p1, false) : TS_NULL;    // És fordítva ?
}

static eTristate checkMac(QSqlQuery &q, qlonglong pid1, qlonglong pid2)
{
    cNPort *p1 = cNPort::getPortObjById(q, pid1);
    cNPort *p2 = cNPort::getPortObjById(q, pid2);
    eTristate r = checkMac(q, *p1, *p2, true);
    delete p1;
    delete p2;
    return r;
}

/* *** */

enum eFieldIx {
    CX_NPORT_LEFT, CX_SHARE_LEFT, CX_PPORT_LEFT, CX_TAG_LEFT,
    CX_STATE, CX_SAVE,
    CX_TAG_RIGHT, CX_PPORT_RIGHT, CX_SHARE_RIGHT, CX_NPORT_RIGHT,
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

    pi = new QTableWidgetItem(pptagl);
    pTable->setItem(row, CX_TAG_LEFT, pi);

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

    pi = new QTableWidgetItem(pptagr);
    pTable->setItem(row, CX_TAG_LEFT, pi);

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

static void portNameAndShare(QSqlQuery& q, QString& names, QString& shares, const cPhsLink *pp)
{
    qlonglong pid = pp->getId(_sPortId2);
    names  = cNPort::getFullNameById(q, pid);
    shares = pp->getName(_sPortShared);
}

static void portNameAndShare(QSqlQuery& q, QString& names, QString& shares, const cPhsLink *pp, eTristate f)
{
    QString n, s;
    portNameAndShare(q, n, s, pp);
    switch (f) {
    case TS_NULL:   names += htmlInfo(n);   break;
    case TS_TRUE:   names += htmlGrInf(n);  break;
    case TS_FALSE:  names += htmlError(n);  break;
    }
    shares += htmlInfo(s);
}

static void portNameAndShare(QSqlQuery& q, QString& names, QString& shares, const tRecordList<cPhsLink>& links)
{
    QListIterator<cPhsLink *> i(links);
    while (i.hasNext()) {
        const cPhsLink *ppl = i.next();
        QString n, s;
        portNameAndShare(q, n, s, ppl);
        names  += htmlInfo(n);
        shares += htmlInfo(s);
    }
}

static QTextEdit *newTextEdit(const QString& txt, const QString& tt = QString())
{
    QTextEdit *p = new QTextEdit(txt);
    p->setReadOnly(true);
    p->setLineWrapMode(QTextEdit::NoWrap);
    if (!tt.isEmpty()) p->setToolTip(tt);
    QSizeF sf = p->document()->size();
    QSize  s;
    s.setHeight((int)(sf.height() + 10));
    s.setWidth((int)sf.width() + 2);
    p->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    p->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    p->setFrameStyle(QFrame::NoFrame);
    p->setMaximumSize(s);
    p->resize(s);
    return p;
}

cDPRow::cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, bool unique, cPPort& ppl, cPPort& ppr)
    : QObject(par), parent(par), pTable(par->pUi->tableWidget), row(_row)
{
    pCheckBox = NULL;
    pTable->setRowCount(row +1);

    // A "felfedezett" link objektum
    phsLink.setId(_sPortId1, ppl.getId());
    phsLink.setId(_sPhsLinkType1, LT_BACK);
    phsLink.setId(_sPortId2, ppr.getId());
    phsLink.setId(_sPhsLinkType2, LT_BACK);
    phsLink.setId(_sPortShared, ES_);   // back-back nincs (nem lehet) megosztás a rekordban
    bool exists;    // igaz, ha már létezik
    QString colMsg; // Ha van ütközés, akkor az üzenet
    bool colision;  // igaz, ha van ütközés
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
    cPhsLink _pl;   // Mnka objektum
    tRecordList<cPhsLink> pll, plr;     // Bal és jobb oldali linkek
    // A MAC alapján?
    int fl, fr;                         // Bal és jobb oldali linkek száma
    _pl.setId(_sPortId1, ppl.getId()).completion(q);        // minta
    fl = pll.set(q);                                        // Bal oldali linkek
    int ix = pll.indexOf(_sPortId2, ppr.get(ppr.idIndex()));// Saját portra mutató link
    if (ix >= 0) {
        delete pll.pullAt(ix);
        --fl;
    }
    _pl.clear().setId(_sPortId1, ppr.getId()).completion(q);// minta
    fr = plr.set(q);                                        // Jobb oldali linkek
    ix = plr.indexOf(_sPortId2, ppr.get(ppl.idIndex()));// Saját portra mutató link
    if (ix >= 0) {
        delete plr.pullAt(ix);
        --fr;
    }
    QString slp, srp; // Jobb és bal oldali következő link teljes port név/nevek
    QString sls, srs; // Jobb és bal oldali következő link megosztás(ok)
    eTristate bMac = TS_FALSE;
    if (fl == 0 || fr == 0) {
        sToolTip += htmlInfo(QObject::trUtf8("További linkek hiányában a MAC táblában nincs találat."));
        portNameAndShare(q, slp, sls, pll);
        portNameAndShare(q, srp, srs, plr);
    }
    else if (fl > 2 || fr > 2) {
        sToolTip += htmlInfo(QObject::trUtf8("A további linkek száma túl magas (%1 <- ... -> %2), nics címtábla ellenörzés.").arg(fl).arg(fr), false, true);
        portNameAndShare(q, slp, sls, pll);
        portNameAndShare(q, srp, srs, plr);
    }
    else {
        QString s, n;
        eTristate r11, r12 = TS_NULL, r21 = TS_NULL, r22 = TS_NULL;
        r11 = checkMac(q, pll.first()->getId(_sPortId2), plr.first()->getId(_sPortId2));
        if (fl == 2) {
            r21 = checkMac(q, pll[1]->getId(_sPortId2), plr[0]->getId(_sPortId2));
        }
        if (fr == 2) {
            r12 = checkMac(q, pll[0]->getId(_sPortId2), plr[1]->getId(_sPortId2));
        }
        if (fl == 2 && fr == 2) {
            r22 = checkMac(q, pll[1]->getId(_sPortId2), plr[1]->getId(_sPortId2));
        }
        int ok = 0, nok = 0;    // counter OK, not OK
        if (r12 == TS_TRUE || r21 == TS_TRUE) {
            r11 = r22 = TS_NULL;
            if (r12 == TS_TRUE) ++ok;
            if (r21 == TS_TRUE) ++ok;
        }
        if (r11 == TS_TRUE || r22 == TS_TRUE) {
            r12 = r21 = TS_NULL;
            if (r11 == TS_TRUE) ++ok;
            if (r22 == TS_TRUE) ++ok;
        }
        if (r11 == TS_FALSE) ++nok;
        if (r12 == TS_FALSE) ++nok;
        if (r21 == TS_FALSE) ++nok;
        if (r22 == TS_FALSE) ++nok;
        if      (ok >  0 && nok == 0) {
            portNameAndShare(q, slp, sls, pll.first(), anyTrue(r11, r12));
            portNameAndShare(q, srp, srs, plr.first(), anyTrue(r21, r22));
            if (fl > 1) portNameAndShare(q, slp, sls, pll[1], anyTrue(r11, r21));
            if (fr > 1) portNameAndShare(q, srp, srs, plr[1], anyTrue(r12, r22));
            sToolTip += htmlInfo(QObject::trUtf8("Pozitív találat a MAC táblában (is)."));
            bMac = TS_TRUE;
        }
        else if (ok >  0 && nok >  0) {
            portNameAndShare(q, slp, sls, pll.first(), tsAnd(r11, r12));
            portNameAndShare(q, srp, srs, plr.first(), tsAnd(r21, r22));
            if (fl > 1) portNameAndShare(q, slp, sls, pll[1], tsAnd(r11, r21));
            if (fr > 1) portNameAndShare(q, srp, srs, plr[1], tsAnd(r12, r22));
            sToolTip += htmlInfo(QObject::trUtf8("Találat a MAC táblában, pozitív és negatív is."));
        }
        else if (ok == 0 && nok >  0) {
            portNameAndShare(q, slp, sls, pll.first(), anyFalse(r11, r12));
            portNameAndShare(q, srp, srs, plr.first(), anyFalse(r21, r22));
            if (fl > 1) portNameAndShare(q, slp, sls, pll[1], anyTrue(r11, r21));
            if (fr > 1) portNameAndShare(q, srp, srs, plr[1], anyTrue(r12, r22));
            sToolTip += htmlInfo(QObject::trUtf8("Találat a MAC táblában, de más linkre."));
            bMac = TS_FALSE;
        }
        else /*  ok == 0 && nok == 0 */ {
            portNameAndShare(q, slp, sls, pll.first(), TS_NULL);
            portNameAndShare(q, srp, srs, plr.first(), TS_NULL);
            if (fl > 1) portNameAndShare(q, slp, sls, pll[1], TS_NULL);
            if (fr > 1) portNameAndShare(q, srp, srs, plr[1], TS_NULL);
            sToolTip += htmlGrInf(QObject::trUtf8("A MAC táblában nincs találat."));
        }
    }
    // Van LLDP link a két végpont között?
/*    bool bLLDP = false;
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
*/
    QTableWidgetItem *pi;
    QTextEdit *pTextEdit;
    QString s;
    //...
    pTextEdit = newTextEdit(state, sToolTip);
    pTable->setCellWidget(row, CX_STATE, pTextEdit);

    pTextEdit = newTextEdit(slp);
    pTable->setCellWidget(row, CX_NPORT_LEFT, pTextEdit);

    pTextEdit = newTextEdit(sls);
    pTable->setCellWidget(row, CX_SHARE_LEFT, pTextEdit);

    s = ppl.getName();
    if (ppshl != ES_) {
        if (ppshl == ES_A) s += " (A)";
        else               s += " (" + ppl.view(q, __sSharedPortId) + " <- " + portShare(ppshl) + ")";
    }
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_PPORT_LEFT, pi);

    s = ppl.getName(_sPortTag);
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_TAG_LEFT, pi);
    pi = new QTableWidgetItem(s);
    pTable->setItem(row, CX_TAG_RIGHT, pi);

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

    pTextEdit = newTextEdit(srs);
    pTable->setCellWidget(row, CX_SHARE_RIGHT, pTextEdit);

    pTextEdit = newTextEdit(srp);
    pTable->setCellWidget(row, CX_NPORT_RIGHT, pTextEdit);
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
    connect(pSelNode2, SIGNAL(nodeIdChanged(qlonglong)), this, SLOT(changeNode2(qlonglong)));
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
    pUi->tableWidget->resizeColumnsToContents();
    pUi->tableWidget->resizeRowsToContents();
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

void cDeducePatch::on_radioButtonLLDP_pressed()
{
    methode = DPM_LLDP;
    clearTable();
    setButtons();
    pSelNode2->setDisabled();
}
void cDeducePatch::on_radioButtonMAC_pressed()
{
    methode = DPM_MAC;
    clearTable();
    setButtons();
    pSelNode2->setDisabled();
}
void cDeducePatch::on_radioButtonTag_pressed()
{
    methode = DPM_TAG;
    clearTable();
    setButtons();
    pSelNode2->setEnabled();
}

void cDeducePatch::on_pushButtonStart_clicked()
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

void cDeducePatch::on_pushButtonSave_clicked()
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
    on_pushButtonStart_clicked();   // Refresh
}
