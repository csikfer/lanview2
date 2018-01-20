#include "record_table.h"
#include "object_dialog.h"
#include "lv2validator.h"

class pINetValidator;
/*
cObjectDialog::cObjectDialog(QWidget *parent, bool ro)
    : QDialog(parent)
{
    pq = newQuery();
    readOnly = ro;
    lockSlot = false;
}

cObjectDialog~cObjectDialog()
{
    delete pq;
}
*/

/* ************************************************************************* */

cPPortTableLine::cPPortTableLine(int r, cPatchDialog *par)
{
    parent = par;
    tableWidget = par->pUi->tableWidgetPorts;
    row    = r;
    sharedPortRow = -1;
    comboBoxShare  = new QComboBox();
    for (int i = ES_; i <= ES_NC; ++i) comboBoxShare->addItem(portShare(i));
    tableWidget->setCellWidget(row, CPP_SH_TYPE, comboBoxShare);
    comboBoxPortIx = new QComboBox();
    tableWidget->setCellWidget(row, CPP_SH_IX, comboBoxPortIx);

    connect(comboBoxShare,  SIGNAL(currentIndexChanged(int)), this, SLOT(changeShared(int)));
    connect(comboBoxPortIx, SIGNAL(currentIndexChanged(int)), this, SLOT(changePortIx(int)));
}

void cPPortTableLine::changeShared(int _sh)
{
    (void)_sh;
    if (parent->lockSlot) return;
    parent->updateSharedIndexs();
}

void cPPortTableLine::changePortIx(int ix)
{
    if (parent->lockSlot) return;
    if (ix == 0) sharedPortRow = -1;
    else {
        int i = ix -1;
        if (!isContIx(listPortIxRow, i)) EXCEPTION(EDATA, ix);
        sharedPortRow = listPortIxRow[i];
    }
    parent->updateSharedIndexs();
}

QString cPatchDialog::sPortRefForm;

cPatchDialog::cPatchDialog(QWidget *parent, bool ro)
    : QDialog(parent)
    , pUi(new Ui::patchSimpleDialog)
{
    pq = newQuery();
    if (sPortRefForm.isEmpty()) sPortRefForm = trUtf8("#%1 (%2/%3)");
    pUi->setupUi(this);
    // nem ciffrázzuk ro-nal le ven titva az ok gomb
    if (ro) pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    pSelectPlace = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, NULL, NULL, this);
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    cIntValidator *intValidator = new cIntValidator(false);
    pUi->tableWidgetPorts->setItemDelegateForColumn(CPP_INDEX, new cItemDelegateValidator(intValidator));

    connect(pUi->lineEditName,      SIGNAL(textChanged(QString)),       this, SLOT(changeName(QString)));
    connect(pUi->comboBoxZone,      SIGNAL(currentIndexChanged(int)),   this, SLOT(changeFilterZone(int)));
    connect(pUi->toolButtonNewPlace,SIGNAL(pressed()),          pSelectPlace, SLOT(insertPlace()));
    connect(pUi->spinBoxFrom,       SIGNAL(valueChanged(int)),          this, SLOT(changeFrom(int)));
    connect(pUi->spinBoxTo,         SIGNAL(valueChanged(int)),          this, SLOT(changeTo(int)));
    connect(pUi->pushButtonAddPorts,SIGNAL(pressed()),                  this, SLOT(addPorts()));
    connect(pUi->tableWidgetPorts,  SIGNAL(cellChanged(int,int)),       this, SLOT(cellChanged(int, int)));
    connect(pUi->tableWidgetPorts->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    connect(pUi->pushButtonDelPort, SIGNAL(pressed()),                  this, SLOT(delPorts()));
    connect(pUi->pushButtonPort1,   SIGNAL(pressed()),                  this, SLOT(set1port()));
    connect(pUi->pushButtonPort2,   SIGNAL(pressed()),                  this, SLOT(set2port()));
    connect(pUi->pushButtonPort2Shared, SIGNAL(pressed()),              this, SLOT(set2sharedPort()));
    shOk = pNamesOk = pIxOk = true;
    lockSlot = false;
}

cPatchDialog::~cPatchDialog()
{
    delete pq;
}

cPatch * cPatchDialog::getPatch()
{
    cPatch *p = new cPatch();
    p->setName(pUi->lineEditName->text());
    p->setNote(pUi->lineEditNote->text());
    p->setId(_sPlaceId, pSelectPlace->currentPlaceId());
    int n = pUi->tableWidgetPorts->rowCount();
    int i;
    for (i = 0; i < n; i++) {
        cPPort *pp = new cPPort();
        pp->setName(             getTableItemText(pUi->tableWidgetPorts, i, CPP_NAME));
        pp->setNote(             getTableItemText(pUi->tableWidgetPorts, i, CPP_NOTE));
        pp->setName(_sPortTag,   getTableItemText(pUi->tableWidgetPorts, i, CPP_TAG));
        pp->setName(_sPortIndex, getTableItemText(pUi->tableWidgetPorts, i, CPP_INDEX));
        int sh = getTableItemComboBoxCurrentIndex(pUi->tableWidgetPorts, i, CPP_SH_TYPE);
        pp->setId(_sSharedCable, sh);
        p->ports << pp;
    }
    for (i = 0; i < n; i++) {
        int sh = (int)p->ports[i]->getId(_sSharedCable);
        if (sh == ES_A || sh == ES_AA) {
            int a = i, b = -1, ab = -1, bb  = -1;
            bool cd = false;
            for (int j = 0; j < n; j++) {
                sh = (int)p->ports[j]->getId(_sSharedCable);
                switch (sh) {
                case ES_AB: ab = j; break;
                case ES_B:
                case ES_BA: b  = j; break;
                case ES_BB: bb = j; break;
                case ES_C:  ab = j; cd = true;  break;
                case ES_D:  b  = j; cd = true;  break;
                default:
                    break;
                }
            }
            bool r = p->setShare(a, ab, b, bb, cd);
            if (!r) {
                QString msg = trUtf8("A megadott kábelmegosztás nem állítható be (%1,%2,%3,%4,%5)").
                        arg(a).arg(b).arg(ab).arg(bb).arg(cd ? _sTrue : _sFalse);
                QMessageBox::warning(this, dcViewShort(DC_ERROR), msg,QMessageBox::Ok);
            }
        }
    }
    return p;
}

void cPatchDialog::setPatch(const cPatch *pSample)
{
    pUi->lineEditName->setText(pSample->getName());
    pUi->lineEditNote->setText(pSample->getNote());
    _setPlaceComboBoxs(pSample->getId(_sPlaceId), pUi->comboBoxZone, pUi->comboBoxPlace, true);
    clearRows();
    int n = pSample->ports.size();
    if (n > 0) {
        int i;
        setRows(n);
        for (i = 0; i < n; i++) {
            const cPPort *pp = pSample->ports.at(i)->reconvert<cPPort>();
            setTableItemText(pp->getName(),              pUi->tableWidgetPorts, i, CPP_NAME);
            setTableItemText(pp->getNote(),              pUi->tableWidgetPorts, i, CPP_NOTE);
            setTableItemText(pp->getName(_sPortTag),     pUi->tableWidgetPorts, i, CPP_TAG);
            setTableItemText(pp->getName(_sPortIndex),   pUi->tableWidgetPorts, i, CPP_INDEX);
            int s = (int)pp->getId(__sSharedCable);
            setPortShare(i, s);
        }
        // Másodlagos megosztáshoz tartozó elsődleges portok
        // Elvileg adatbázisból származó objektum, így ki vannak tültve az ID-k
        // Ha mégse, akkor ezek a hivatkozások elvesznek,
        // ha a hivatkozás hibás, szintén.
        for (i = 0; i < n; i++) {
            const cPPort *pp = pSample->ports.at(i)->reconvert<cPPort>();
            if (pp->isNull(_sSharedPortId)) continue;
            qlonglong spid = pp->getId(_sSharedPortId);
            int ix = pSample->ports.indexOf(spid);  // Hivatkozott port index
            if (ix < 0) {       //
                QString msg = trUtf8("A %1 nevű megosztott port a %2 ID-jű portra hivatkozik, amit nem találok.").
                        arg(pp->getName()).arg(spid);
                QMessageBox::warning(this, dcViewShort(DC_ERROR), msg,QMessageBox::Ok);
                continue;
            }
            ix = rowsData[i]->listPortIxRow.indexOf(ix);    // Kiválasztható ?
            if (ix < 0) {                   // nem találta
                QString msg = trUtf8("A %1 nevű megosztott port a %2 ID-jű portra hivatkozik, amit nem választható ki.").
                        arg(pp->getName()).arg(spid);
                QMessageBox::warning(this, dcViewShort(DC_ERROR), msg,QMessageBox::Ok);
                continue;
            }
            rowsData[i]->comboBoxPortIx->setCurrentIndex(ix);
        }
    }
}


void cPatchDialog::clearRows()
{
    while (!rowsData.isEmpty()) {
        delete rowsData.takeLast();
    }
    pUi->tableWidgetPorts->clearContents();
    pUi->tableWidgetPorts->setRowCount(0);
    /*
    int n1 = rowsData.size();
    int n2 = pUi->tableWidgetPorts->rowCount();
    if (n1 != n2) {
        EXCEPTION(EPROGFAIL);
    }
    */
}

void cPatchDialog::setRows(int rows)
{
    int i = pUi->tableWidgetPorts->rowCount();
    if (i != rowsData.size()) EXCEPTION(EDATA);
    if (i == rows) return;
    if (i < rows) {
        pUi->tableWidgetPorts->setRowCount(rows);
        for (; i < rows; ++i) {
            rowsData << new cPPortTableLine(i, this);
        }
    }
    else {
        while (rows < rowsData.size()) {
            delete rowsData.takeLast();
        }
        pUi->tableWidgetPorts->setRowCount(rows);
    }
}

void cPatchDialog::setPortShare(int row, int sh)
{
    if (!isContIx(rowsData, row)) EXCEPTION(ENOINDEX, row);
    rowsData[row]->comboBoxShare->setCurrentIndex(sh);
}

#define _LOCKSLOTS()    _lockSlotSave_ = lockSlot; lockSlot = true
#define LOCKSLOTS()     bool _LOCKSLOTS()
#define UNLOCKSLOTS()   lockSlot = _lockSlotSave_

void cPatchDialog::updateSharedIndexs()
{
    // Az összes elsödleges megosztás listája (sor indexek)
    shPrimRows.clear();
    foreach (cPPortTableLine *pRow, rowsData) {
        int sh = pRow->comboBoxShare->currentIndex();          // megosztás típusa
        if (sh == ES_A || sh == ES_AA) shPrimRows << pRow->row;// Ha elsődleges megosztás (amire hivatkozik a többi)
    }
    // MAP az elsődleges megosztásokra mutatók listái
    shPrimMap.clear();
    QList<int> secondIxs;
    foreach (cPPortTableLine *pRow, rowsData) {
        int sh = pRow->comboBoxShare->currentIndex();   // megosztás típusa
        if (sh == ES_AB || sh == ES_B || sh == ES_BA || sh == ES_BB || sh == ES_C || sh ==  ES_D) {
            int shrow = pRow->sharedPortRow;        // Mutató az elsődleges megosztott portra
            if (shrow >= 0 && !shPrimRows.contains(shrow)) {
                shrow = -1;  // megszűnt
                pRow->sharedPortRow = -1;
            }
            if (shrow >= 0) shPrimMap[shrow] << pRow->row;  // Van mutató, csoportosítéshoz a MAP-ba
            secondIxs << pRow->row;
        }
    }
    shOk = true;
    LOCKSLOTS();
    // Végigszaladunk az összes soron, és frissítjük az elsődlegesre hivatkozások comboBox-át
    foreach (cPPortTableLine *pRow, rowsData) {
        if (secondIxs.contains(pRow->row)) {    // Másodlagos megosztott port (lehet hivatkozás, a)
            QStringList sl;             // Az eredmény lista (ezekre lehet hivatkozni) a teljes lista leválogatásának a célja
            pRow->listPortIxRow.clear();// A táblázatbeli sor idexek (hogy tudjuk is mire hivatkozik a fenti lista)
            int mysh = pRow->comboBoxShare->currentIndex(); // Az aktuális másodlagos megosztás típusa
            // Elsödleges megosztott portokhoz rendelések vizsgálata
            foreach (int pix, shPrimRows) {     // Végivesszük a lehetőségeket (elsödleges megosztások listája)
                QList<int> secs = shPrimMap[pix];       // Ezek a másodlagosak hivatkoznak rá
                QString item = refName(pix);            // A comboBox-ban megjelenő string formátuma
                if (secs.isEmpty()) {   // Erre az elsődlegesre nem hivatkozik senki, akkor bárhol választható
                    sl                  << item;
                    pRow->listPortIxRow << pix;
                }
                else {                  // Van rá hivatkozás, akkor vizsgálódunk
                    int psh = rowsData[pix]->comboBoxShare->currentIndex(); // Az aktuális elsődleges megosztás
                    // ütközés az elsődlegessel
                    if (psh == ES_A && (mysh == ES_AB || mysh == ES_C || mysh == ES_D)) continue;
                    bool collision = false;
                    foreach (int six, secs) {
                        if (six == pRow->row) continue;     // Saját magával nincs értelme az ütközésvizsgálatnak
                        int ssh = rowsData[six]->comboBoxShare->currentIndex(); // akt, másodlagos megosztás
                        if (mysh == ssh) {                  // Azonos megosztás tuti nem lehetséges
                            collision = true;
                            break;
                        }
                        switch (mysh) { // tételesen az összeférhetetlen megosztások
                        case ES_AB:
                            collision = ssh == ES_C || ssh == ES_D;
                            break;
                        case ES_B:
                            collision = ssh == ES_BA || ssh == ES_BB || ssh == ES_C || ssh == ES_D;
                            break;
                        case ES_BA:
                            collision = ssh == ES_B || ssh == ES_C || ssh == ES_D;
                        case ES_BB:
                            collision = ssh == ES_B;
                        case ES_C:
                        case ES_D:
                            collision = ssh == ES_B || ssh == ES_AB;
                        default:
                            EXCEPTION(EDATA);
                            break;
                        }
                        if (collision) break;   // volt ütközés, nem kell tovább vizsgálódni
                    }
                    if (collision) continue;    // Ha ütközés van, nem kerülhet a választási lehetőségek közé.
                    sl                  << item;// Hozzáadjuk a lehetőségek listá(i)hoz
                    pRow->listPortIxRow << pix;
                }
            }
            pRow->comboBoxPortIx->clear();
            pRow->comboBoxPortIx->addItem(_sNul);
            pRow->comboBoxPortIx->addItems(sl);
            if (pRow->sharedPortRow >= 0 && pRow->listPortIxRow.contains(pRow->sharedPortRow)) {
                int ix = pRow->listPortIxRow.indexOf(pRow->sharedPortRow);
                pRow->comboBoxPortIx->setCurrentIndex(ix +1);
            }
            else {
                pRow->sharedPortRow = -1;
                pRow->comboBoxPortIx->setCurrentIndex(0);
                shOk = false;   // Nincs megadva az elsődleges, az nem OK.
            }
        }
        else {                                  // Elsodleges megosztott, vagy nem megosztott
            pRow->comboBoxPortIx->clear();      // Nem hivatkozik senkire
            pRow->sharedPortRow = -1;
        }
    }
    UNLOCKSLOTS();
    changeName(pUi->lineEditName->text());  // Menthető ?
}

void cPatchDialog::updatePNameIxOk()
{
    QStringList nameList;
    QList<int>  indexList;
    pNamesOk = pIxOk = true;
    int n = pUi->tableWidgetPorts->rowCount();
    for (int i = 0; i < n; ++i) {
        if (pNamesOk) {
            QString s = getTableItemText(pUi->tableWidgetPorts, i, CPP_NAME);
            if (s.isEmpty() || nameList.contains(s)) {
                pNamesOk = false;
                if (!pIxOk) break;
            }
            nameList << s;
        }
        if (pIxOk) {
            bool ok;
            int ix = getTableItemText(pUi->tableWidgetPorts, i, CPP_INDEX).toInt(&ok);
            if (!ok || indexList.contains(ix)) {
                pIxOk = false;
                if (!pNamesOk) break;
            }
            indexList << ix;
        }
    }
}

QString cPatchDialog::refName(int row)
{
    return refName(row, getTableItemText(pUi->tableWidgetPorts, row, CPP_NAME),
                        getTableItemText(pUi->tableWidgetPorts, row, CPP_INDEX));
}


/* SLOTS */
void cPatchDialog::changeName(const QString& name)
{
    bool f = name.isEmpty() || pUi->tableWidgetPorts->rowCount() == 0 || !shOk || !pNamesOk || !pIxOk;
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(f);
}

void cPatchDialog::set1port()
{
    LOCKSLOTS();
    clearRows();
    setRows(1);
    QTableWidgetItem *pItem;
    pItem = new QTableWidgetItem("J1");
    pUi->tableWidgetPorts->setItem(0, CPP_NAME, pItem);
    pItem = new QTableWidgetItem("1");
    pUi->tableWidgetPorts->setItem(0, CPP_INDEX, pItem);
    bool f = pUi->lineEditName->text().isEmpty();
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(f);
    UNLOCKSLOTS();
}

void cPatchDialog::set2port()
{
    set1port();
    LOCKSLOTS();
    setRows(2);
    QTableWidgetItem *pItem;
    pItem = new QTableWidgetItem("J2");
    pUi->tableWidgetPorts->setItem(1, CPP_NAME, pItem);
    pItem = new QTableWidgetItem("2");
    pUi->tableWidgetPorts->setItem(1, CPP_INDEX, pItem);
    UNLOCKSLOTS();
}

void cPatchDialog::set2sharedPort()
{
    set2port();
    setPortShare(0, ES_A);
    setPortShare(1, ES_B);
    rowsData[1]->comboBoxPortIx->setCurrentIndex(1);    // Az első portra mutat
}

void cPatchDialog::addPorts()
{
    int from = pUi->spinBoxFrom->value();
    int to   = pUi->spinBoxTo->value();
    int n = to - from +1;
    if (n <= 0) return;
    int row = pUi->tableWidgetPorts->rowCount();
    LOCKSLOTS();
    setRows(row + n);
    QString namePattern = pUi->lineEditNamePattern->text();
    QString tagPattern  = pUi->lineEditTagPattern->text();
    int nameOffs = pUi->spinBoxNameOffs->value();
    int tagOffs  = pUi->spinBoxTagOffs->value();
    QTableWidgetItem *pItem;
    for (int i = from; i <= to; i++, row++) {
        QString name = nameAndNumber(namePattern, i + nameOffs);
        QString tag;
        if (!tagPattern.isEmpty()) tag = nameAndNumber(tagPattern, i + tagOffs);
        pItem = new QTableWidgetItem(name);
        pUi->tableWidgetPorts->setItem(row, CPP_NAME, pItem);
        pItem = new QTableWidgetItem(QString::number(i));
        pUi->tableWidgetPorts->setItem(row, CPP_INDEX, pItem);
        pItem = new QTableWidgetItem(tag);
        pUi->tableWidgetPorts->setItem(row, CPP_TAG, pItem);
    }
    UNLOCKSLOTS();
    updatePNameIxOk();
    changeName(pUi->lineEditName->text());
}

void cPatchDialog::delPorts()
{
    QModelIndexList mil = pUi->tableWidgetPorts->selectionModel()->selectedRows();
    QList<int>  rows;   // Törlendő sorok sorszámai
    foreach (QModelIndex mi, mil) {
        rows << mi.row();
    }
    // sorba rendezzük, mert majd a vége felül törlünk
    std::sort(rows.begin(), rows.end());
    // A hivatkozásokat töröljük a törlendő sorokra
    for (int j = 0; j < rowsData.size(); ++j) {
        int i = rowsData[j]->sharedPortRow;
        if (rows.contains(i)) rowsData[j]->sharedPortRow = -1;  // Törölt sorra nem hivatkozhatunk
    }
    // Törlés a vége felöl
    while (rows.size() > 0) {
        int row = rows.takeLast();
        rowsData.removeAt(row);
        pUi->tableWidgetPorts->removeRow(row);
    }
    // Sor indexek ujra számolása:
    for (int i = 0; i < rowsData.size(); ++i) {
        int old = rowsData[i]->row;
        rowsData[i]->row = i;
        for (int j = 0; j < rowsData.size(); ++j) {
            if (rowsData[j]->sharedPortRow == old) rowsData[j]->sharedPortRow = i;
        }
    }
    updateSharedIndexs();
    // Mentés gomb enhedélyezés, ha ok
    changeName(pUi->lineEditName->text());
}

void cPatchDialog::changeFrom(int i)
{
    bool f = (pUi->spinBoxTo->value() - i) >= 0;
    pUi->pushButtonAddPorts->setEnabled(f);
}

void cPatchDialog::changeTo(int i)
{
    bool f = (i - pUi->spinBoxFrom->value()) >= 0;
    pUi->pushButtonAddPorts->setEnabled(f);
}

void cPatchDialog::cellChanged(int row, int col)
{
    if (lockSlot) return;
    QString text = getTableItemText(pUi->tableWidgetPorts, row, col);
    int n = pUi->tableWidgetPorts->rowCount();
    QString sRefName;
    switch (col) {
    case CPP_NAME: {
        if (text.isEmpty()) {
            pNamesOk = false;
        }
        else {
            pNamesOk = true;
            QStringList nameList;
            for (int i = 0; i < n; ++i) {
                QString s = getTableItemText(pUi->tableWidgetPorts, i, CPP_NAME);
                if (s.isEmpty() || nameList.contains(s)) {
                    pNamesOk = false;
                    break;
                }
                nameList << s;
            }
        }
        sRefName = refName(row, text, getTableItemText(pUi->tableWidgetPorts, row, CPP_INDEX));
        break;
    }
    case CPP_INDEX: {
        QList<int>  ixList;
        pIxOk = true;
        for (int i = 0; i < n; ++i) {
            bool ok;
            int ix = getTableItemText(pUi->tableWidgetPorts, i, CPP_INDEX).toInt(&ok);
            if (!ok || ixList.contains(ix)) {   // Nem szám, vagy nem egyedi
                pIxOk = false;
                break;
            }
            ixList << ix;
        }
        sRefName = refName(row, getTableItemText(pUi->tableWidgetPorts, row, CPP_NAME), text);
        break;
    }
    default:    return;     // Mással nem foglalkozunk
    }
    // Ha vannak index hivatkozások, akkor a generált hivatkozás nevek megváltoztak!
    if (shPrimRows.contains(row)) {
        foreach (cPPortTableLine *pRow, rowsData) {
            QList<int> refList = pRow->listPortIxRow;
            int ix = refList.indexOf(row);
            if (ix < 0) continue;
            pRow->comboBoxPortIx->setItemText(ix + 1, sRefName);
        }
    }
    changeName(pUi->lineEditName->text());
}

void cPatchDialog::selectionChanged(const QItemSelection &, const QItemSelection &)
{
    QModelIndexList mil = pUi->tableWidgetPorts->selectionModel()->selectedRows();
    pUi->pushButtonDelPort->setDisabled(mil.isEmpty());
}

cPatch * patchInsertDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample)
{
    cPatchDialog dialog(pPar, false);
    if (pSample != NULL) {
        dialog.setPatch(pSample);
    }
    cPatch *p;
    while (true) {
        int r = dialog.exec();
        if (r != QDialog::Accepted) return NULL;
        p = dialog.getPatch();
        if (p == NULL) continue;
        if (!cErrorMessageBox::condMsgBox(p->tryInsert(q))) {
            delete p;
            continue;
        }
        break;
    }
    return p;
}

cPatch * patchEditDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample, bool ro)
{
    cPatchDialog dialog(pPar, ro);
    if (pSample == NULL) EXCEPTION(EPROGFAIL);
    dialog.setPatch(pSample);
    qlonglong id = pSample->getId();
    if (id == NULL_ID) EXCEPTION(EDATA);
    cPatch *p;
    while (true) {
        int r = dialog.exec();
        if (r != QDialog::Accepted) return NULL;
        p = dialog.getPatch();
        if (p == NULL) continue;
        if (!cErrorMessageBox::condMsgBox(p->setId(id).tryUpdateById(q))) {
            delete p;
            continue;
        }
        break;
    }
    return p;
}

/* ********************************************************************************************** */

cIpEditWidget::cIpEditWidget(const tIntVector& _types, QWidget *_par) : QWidget(_par)
{
    disabled = disableSignals = false;
    pq = newQuery();
    pUi = new Ui::editIp;
    pUi->setupUi(this);
    pEditNote = new cLineWidget;
    setFormEditWidget(pUi->formLayout, pUi->labelIpNote, pEditNote);
    pModelIpType = new cEnumListModel("addresstype", NT_NOT_NULL, _types);
    pModelIpType->joinWith(pUi->comboBoxIpType);
    pSelectVlan = new cSelectVlan(pUi->comboBoxVLanId, pUi->comboBoxVLan);
    pSelectSubNet = new cSelectSubNet(pUi->comboBoxSubNetAddr, pUi->comboBoxSubNet);
    pINetValidator = new cINetValidator(true, this);
    pUi->lineEditAddress->setValidator(pINetValidator);
    showToolBoxs(false, false, false);    // Az Info és Query toolBox-ot alapértelmezetten eltüntetjük.
    pUi->toolButtonQuery->hide();
    connect(pSelectVlan,   SIGNAL(changedId(qlonglong)), pSelectSubNet, SLOT(setCurrentByVlan(qlonglong)));
    connect(pSelectSubNet, SIGNAL(changedId(qlonglong)), pSelectVlan,   SLOT(setCurrentBySubNet(qlonglong)));
    connect(pSelectSubNet, SIGNAL(changedId(qlonglong)), this,          SLOT(_subNetIdChanged(qlonglong)));
    _state = IES_IS_NULL;
}

cIpEditWidget::~cIpEditWidget()
{
    delete pq;
}

void cIpEditWidget::set(const cIpAddress *po)
{
    disableSignals = true;
    _state = 0;
    actAddress = po->address();
    if (actAddress.isNull()) _state |= IES_ADDRESS_IS_NULL;
    pUi->lineEditAddress->setText(po->getName(_sAddress));
    int ix = pModelIpType->indexOf(po->getName(_sIpAddressType));
    if (ix < 0) ix = 0; // ?!
    pUi->comboBoxIpType->setCurrentIndex(ix);
    if (ix == 0) _state |= IES_ADDRESS_TYPE_IS_NULL;
    qlonglong snid = po->getId(_sSubNetId);
    pSelectSubNet->setCurrentBySubNet(snid);
    if (snid == NULL_ID) _state |= IES_SUBNET_IS_NULL;
    pEditNote->set(po->get(po->noteIndex()));
    disableSignals = false;
}

void cIpEditWidget::set(const QHostAddress &a)
{
    pUi->lineEditAddress->setText(a.toString());
}

cIpAddress *cIpEditWidget::get(cIpAddress *po) const
{
    po->setAddress(actAddress, pUi->comboBoxIpType->currentText());
    po->setId(pSelectSubNet->currentId());
    po->set(po->noteIndex(), pEditNote->get());
    return po;
}

void cIpEditWidget::showToolBoxs(bool _info, bool _go, bool _query)
{
    pUi->toolButtonInfo->setVisible(_info);
    pUi->toolButtonGo->setVisible(_go);
    pUi->toolButtonQuery->setVisible(_query);
}

void cIpEditWidget::enableToolButtons(bool _info, bool _go, bool _query)
{
    disabled_info = !_info;
    disabled_go   = !_go;
    disabled_query= !_query;
    pUi->toolButtonInfo->setDisabled(disabled_info);
    pUi->toolButtonGo->setDisabled(disabled_go);
    pUi->toolButtonQuery->setDisabled(disabled_query);
}


// SLOTS
void cIpEditWidget::setAllDisabled(bool f)
{
    disableSignals = true;
    disabled = f;
    pSelectVlan->setDisable(f);
    if (f) {
        pUi->lineEditAddress->setText(_sNul);
        actAddress.clear();
    }
    pUi->lineEditAddress->setDisabled(f);
    pUi->comboBoxIpType->setDisabled(f);
    pEditNote->setDisabled(f);
    pUi->comboBoxSubNet->setDisabled(f);
    pUi->comboBoxSubNetAddr->setDisabled(f);
    pUi->toolButtonInfo->setDisabled(disabled_info || f);
    pUi->toolButtonGo->setDisabled(disabled_go || f);
    pUi->toolButtonQuery->setDisabled(disabled_query || f);
    disableSignals = false;
}

void cIpEditWidget::_subNetIdChanged(qlonglong _id)
{
    if (_state & IES_SUBNET_IS_NULL) {
        if (_id != NULL_ID) {
            _state &= ~ IES_SUBNET_IS_NULL;
        }
        else {
            return; // state unchanged
        }
    }
    else {
        if (_id == NULL_ID) {
            _state |=   IES_SUBNET_IS_NULL;
        }
        else {
            return; // state unchanged
        }
    }
    changed(actAddress, _state);
}

void cIpEditWidget::on_comboBoxIpType_currentIndexChanged(int index)
{
    (void)index;
    changed(actAddress, _state);
}

void cIpEditWidget::on_lineEditAddress_textChanged(const QString &arg1)
{
    int oldState = _state;
    QHostAddress oldAddress = actAddress;
    _state &= ~(IES_ADDRESS_IS_INVALID | IES_ADDRESS_IS_NULL);
    if (arg1.isEmpty()) {
        actAddress.clear();
        _state |= IES_ADDRESS_IS_NULL;
    }
    else {
        int dummy;
        QString sa = arg1;
        if (QValidator::Acceptable == pINetValidator->validate(sa, dummy)) {
            actAddress.setAddress(arg1);
            if (actAddress.isNull()) EXCEPTION(EPROGFAIL);
            pSelectSubNet->setCurrentByAddress(actAddress);
        }
        else {
            _state |= IES_ADDRESS_IS_INVALID;
            actAddress.clear();
        }
    }
    if (oldState != _state || oldAddress != actAddress) changed(actAddress, _state);
}

void cIpEditWidget::on_toolButtonQuery_clicked()
{
    emit query_clicked();
}

void cIpEditWidget::on_toolButtonInfo_clicked()
{
    emit info_clicked();
}

void cIpEditWidget::on_toolButtonGo_clicked()
{
    emit go_clicked();
}

/* ********************************************************************************************** */


cEnumValRow::cEnumValRow(QSqlQuery& q, const QString& _val, int _row, cEnumValsEditWidget *par)
    :QObject(par), row(_row)
{
    parent = par;
    pTableWidget = par->pUi->tableWidgetType;
    rec.setName(_sEnumTypeName, parent->enumTypeTypeName);
    rec.setName(_sEnumValName, _val);
    int r = rec.completion(q);
    QString t;
    switch (r) {
    case 1:     // Beolvasva
        rec.fetchText(q);
        break;
    case 0:     // Még nincs az adatbázisban  Set default
        rec.setName(_sEnumTypeName, parent->enumTypeTypeName);
        rec.setName(_sEnumValName, _val);
        break;
    default:
        EXCEPTION(AMBIGUOUS, r, mCat(parent->enumTypeTypeName, _val));
    }
    QTableWidgetItem *p = setTableItemText(_val, pTableWidget, row, CEV_NAME);
    p->setFlags(p->flags() & ~Qt::ItemIsEditable);
    t = rec.getText(cEnumVal::LTX_VIEW_SHORT, _val);
    setTableItemText(t, pTableWidget, row, CEV_SHORT);
    t = rec.getText(cEnumVal::LTX_VIEW_LONG, _val);
    setTableItemText(t,  pTableWidget, row, CEV_LONG);
    pBgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sBgColor), rec[_sBgColor], false, NULL);
    pTableWidget->setCellWidget(row, CEV_BG_COLOR, pBgColorWidget);
    pFgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFgColor), rec[_sFgColor], false, NULL);
    pTableWidget->setCellWidget(row, CEV_FG_COLOR, pFgColorWidget);
    pFntFamWidget  = new cFontFamilyWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFontFamily), rec[_sFontFamily], NULL);
    pTableWidget->setCellWidget(row, CEV_FNT_FAM, pFntFamWidget);
    pFntAttWidget  = new cFontAttrWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFontAttr), rec[_sFontAttr], false, NULL);
    pTableWidget->setCellWidget(row, CEV_FNT_ATT, pFntAttWidget);
    setTableItemText(rec.getName(_sEnumValNote),  pTableWidget, row, CEV_NOTE);
    t = rec.getText(cEnumVal::LTX_TOOL_TIP);
    setTableItemText(t,  pTableWidget, row, CEV_TOOL_TIP);
}

void cEnumValRow::save(QSqlQuery& q)
{
    rec.clearId();
    // !!!
    rec.setText(cEnumVal::LTX_VIEW_SHORT, getTableItemText(pTableWidget, row, CEV_SHORT));
    rec.setText(cEnumVal::LTX_VIEW_LONG,  getTableItemText(pTableWidget, row, CEV_LONG));
    rec.setText(cEnumVal::LTX_TOOL_TIP,   getTableItemText(pTableWidget, row, CEV_TOOL_TIP));
    rec[_sBgColor]     = pBgColorWidget->get();
    rec[_sFgColor]     = pFgColorWidget->get();
    rec[_sFontFamily]  = pFntFamWidget->get();
    rec[_sFontAttr]    = pFntAttWidget->get();
    rec[_sEnumValNote] = getTableItemText(pTableWidget, row, CEV_NOTE);
    rec.replace(q);
    rec.saveText(q);
}

cEnumValsEditWidget::cEnumValsEditWidget(QWidget *parent)
    : QWidget(parent)
{
    pEnumTypeType = NULL;
    pEnumValType  = NULL;
    pq = newQuery();
    pUi = new Ui::enumValsWidget;
    pUi->setupUi(this);
    // pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    // pUi->buttonBox->button(QDialogButtonBox::Save)->setDisabled(true);
    connect(pUi->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));

    pSelLangVal = new cSelectLanguage(pUi->comboBoxValLang, this);
    langIdVal = pSelLangVal->currentLangId();

    pShape = new cTableShape;
    if (!pShape->fetchByName(*pq, _sEnumVals)) {
        EXCEPTION(EDATA, 0, _sEnumVals);
    }
    pShape->fetchFields(*pq);
    // Get enumeration types list from database
    QString sql = "SELECT typname FROM pg_catalog.pg_type WHERE typcategory = 'E' ORDER BY typname ASC";
    QStringList typeList;
    execSql(*pq, sql);
    getListFromQuery(*pq, typeList);
    // List of tables with boolean type fields
    sql = "SELECT DISTINCT(table_name) FROM information_schema.columns WHERE table_schema = 'public'"
          " AND (data_type = 'boolean' OR (data_type = 'ARRAY' AND udt_name = 'boolean'))"
            " ORDER BY table_name ASC";
    QStringList tableList;
    execSql(*pq, sql);
    getListFromQuery(*pq, tableList);

    pShape = new cTableShape;
    pShape->setParent(this);
    pShape->setByName(*pq, _sEnumVals);
    pShape->fetchFields(*pq);

    // VAL
    pWidgetValBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), val[_sBgColor], false, NULL);
    pWidgetValBgColor->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValBgColor, pWidgetValBgColor);

    pWidgetValFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), val[_sFgColor], false, NULL);
    pWidgetValFgColor->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFgColor, pWidgetValFgColor);

    pWidgetValFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), val[_sFontFamily], NULL);
    pWidgetValFntFam->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFontFamily, pWidgetValFntFam);

    pWidgetValFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), val[_sFontAttr], false, NULL);
    pWidgetValFntAtt->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFontAttr, pWidgetValFntAtt);

    connect(pUi->comboBoxValType,  SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumValType(QString)));
    connect(pUi->comboBoxValVal,   SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumValVal(QString)));
    pUi->comboBoxValType->addItems(typeList);

    // Type
    connect(pUi->comboBoxTypeType, SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumTypeType(QString)));
    pUi->comboBoxTypeType->addItems(typeList);

    // Boolean
        // true
    pWidgetTrueBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), boolTrue[_sBgColor], false, NULL);
    pWidgetTrueBgColor->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueBgColor, pWidgetTrueBgColor);

    pWidgetTrueFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), boolTrue[_sFgColor], false, NULL);
    pWidgetTrueFgColor->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFgColor, pWidgetTrueFgColor);

    pWidgetTrueFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), boolTrue[_sFontFamily], NULL);
    pWidgetTrueFntFam->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFontFam, pWidgetTrueFntFam);

    pWidgetTrueFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), boolTrue[_sFontAttr], false, NULL);
    pWidgetTrueFntAtt->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFntAtt, pWidgetTrueFntAtt);
        // false
    pWidgetFalseBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), boolFalse[_sBgColor], false, NULL);
    pWidgetFalseBgColor->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseBgColor, pWidgetFalseBgColor);

    pWidgetFalseFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), boolFalse[_sFgColor], false, NULL);
    pWidgetFalseFgColor->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFgColor, pWidgetFalseFgColor);

    pWidgetFalseFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), boolFalse[_sFontFamily], NULL);
    pWidgetFalseFntFam->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFntFam, pWidgetFalseFntFam);

    pWidgetFalseFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), boolFalse[_sFontAttr], false, NULL);
    pWidgetFalseFntAtt->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFntAtt, pWidgetFalseFntAtt);

    connect(pUi->comboBoxBoolTable, SIGNAL(currentIndexChanged(QString)), this, SLOT(setBoolTable(QString)));
    connect(pUi->comboBoxBoolField, SIGNAL(currentIndexChanged(QString)), this, SLOT(setBoolField(QString)));
    pUi->comboBoxBoolTable->addItems(tableList);
}

cEnumValsEditWidget::~cEnumValsEditWidget()
{
    delete pq;
}

bool cEnumValsEditWidget::save()
{
    QWidget *pW =  pUi->tabWidget->currentWidget();
    if      (pW == pUi->tabEnumValue) return saveValue();
    else if (pW == pUi->tabEnumType)  return saveType();
    else if (pW == pUi->tabBoolean)   return saveBoolean();
    else                              EXCEPTION(EPROGFAIL);
    return false;
}

bool cEnumValsEditWidget::saveValue()
{
    val.clearId();
    val[_sEnumValNote]  = pUi->lineEditValValNote->text();
    val[_sBgColor]      = pWidgetValBgColor->get();
    val[_sFgColor]      = pWidgetValFgColor->get();
    val.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditValShort->text());
    val.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditValLong->text());
    val.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditValValTolTip->toPlainText());
    val[_sFontFamily]   = pWidgetValFntFam->get();
    val[_sFontAttr]     = pWidgetValFntAtt->get();

    return cErrorMessageBox::condMsgBox(val.tryReplace(*pq, TS_NULL, true));
}

bool cEnumValsEditWidget::saveType()
{
    type.clearId();
    type[_sEnumValNote]  = pUi->lineEditTypeTypeNote->text();
    type.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditTypeShort->text());
    type.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditTypeLong->text());
    type.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditTypeTypeToolTip->toPlainText());

    cError *pe = NULL;
    static const QString tn = "cEnumValsEdit";
    sqlBegin(*pq, tn);
    try {
        type.replace(*pq);
        type.saveText(*pq);
        foreach (cEnumValRow *p, rows) {
            p->save(*pq);
        }
        sqlCommit(*pq, tn);
    }
    CATCHS(pe);
    if (pe != NULL) {
        sqlRollback(*pq, tn);
        cErrorMessageBox::messageBox(pe, this);
        delete pe;
        return false;
    }
    return true;
}

bool cEnumValsEditWidget::saveBoolean()
{
    QString type = mCat(pUi->comboBoxBoolTable->currentText(), pUi->comboBoxBoolField->currentText());

    boolType[_sEnumValName]  = _sNul;
    boolType[_sEnumValNote]  = pUi->lineEditBoolTypeNote->text();
    boolType[_sEnumTypeName] = type;
    boolType.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditBoolTypeShort->text());
    boolType.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditBoolTypeLong->text());
    boolType.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditBoolToolTip->toPlainText());

    boolTrue[_sEnumValName]  = _sTrue;
    boolTrue[_sEnumValNote]  = pUi->lineEditTrueNote->text();
    boolTrue[_sEnumTypeName] = type;
    boolTrue[_sBgColor]      = pWidgetTrueBgColor->get();
    boolTrue[_sFgColor]      = pWidgetTrueFgColor->get();
    boolTrue.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditTrueShort->text());
    boolTrue.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditTrueLong->text());
    boolTrue.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditTrueToolTip->toPlainText());
    boolTrue[_sFontFamily]   = pWidgetTrueFntFam->get();
    boolTrue[_sFontAttr]     = pWidgetTrueFntAtt->get();

    boolFalse[_sEnumValName] = _sFalse;
    boolFalse[_sEnumValNote] = pUi->lineEditFalseNote->text();
    boolFalse[_sEnumTypeName]= type;
    boolFalse[_sBgColor]     = pWidgetFalseBgColor->get();
    boolFalse[_sFgColor]     = pWidgetFalseFgColor->get();
    boolFalse.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditFalseShort->text());
    boolFalse.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditFalseLong->text());
    boolFalse.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditFalseToolTip->toPlainText());
    boolFalse[_sFontFamily]  = pWidgetFalseFntFam->get();
    boolFalse[_sFontAttr]    = pWidgetFalseFntAtt->get();

    cError *pe = NULL;
    static const QString tn = "cEnumValsEdit";
    sqlBegin(*pq, tn);
    try {
        boolType.replace(*pq);
        boolType.saveText(*pq);
        boolTrue.replace(*pq);
        boolTrue.saveText(*pq);
        boolFalse.replace(*pq);
        boolFalse.saveText(*pq);
        sqlCommit(*pq, tn);
    }
    CATCHS(pe);
    if (pe != NULL) {
        sqlRollback(*pq, tn);
        cErrorMessageBox::messageBox(pe, this);
        delete pe;
        return false;
    }
    return true;
}


void cEnumValsEditWidget::clicked(QAbstractButton *pButton)
{
    int sb = pUi->buttonBox->standardButton(pButton);
    switch (sb) {
    case QDialogButtonBox::Ok:
        if (save()) closeWidget();
        break;
    case QDialogButtonBox::Save:
        save();
        break;
    case QDialogButtonBox::Cancel:
        closeWidget();
        break;
    }
}

void cEnumValsEditWidget::setEnumValType(const QString& etn)
{
    enumValTypeName = etn;
    pEnumValType = cColEnumType::fetchOrGet(*pq, enumValTypeName);
    pUi->comboBoxValVal->clear();
    pUi->comboBoxValVal->addItems(pEnumValType->enumValues);
}

void cEnumValsEditWidget::setEnumValVal(const QString& _ev)
{
    if (pEnumValType  == NULL || enumValTypeName.isEmpty()) return;
    val.clear();
    QString ev = _ev;
    if (ev.isNull()) ev = QString("");  // Üres lehet, de NULL nem!
    val[_sEnumValName]  = ev;
    val[_sEnumTypeName] = enumValTypeName;
    int n = val.completion(*pq);
    QString t;
    switch (n) {
    case 1:     // Beolvasva
        break;
    case 0:     // Not found (az enumVal-t törölte a completion() metódus)
        val[_sEnumValName]  = ev;
        val[_sEnumTypeName] = enumValTypeName;
        val.setText(cEnumVal::LTX_VIEW_SHORT, ev);
        val.setText(cEnumVal::LTX_VIEW_LONG,  ev);
        break;
    default:    EXCEPTION(AMBIGUOUS, n, val.identifying());
    }
    pWidgetValBgColor->set(val[_sBgColor]);
    pWidgetValFgColor->set(val[_sFgColor]);
    t = val.getText(cEnumVal::LTX_VIEW_SHORT, ev);
    pUi->lineEditValShort->setText(t);
    t = val.getText(cEnumVal::LTX_VIEW_LONG, ev);
    pUi->lineEditValLong->setText(t);
    pUi->lineEditValValNote->setText(val.getNote());
    t = val.getText(cEnumVal::LTX_TOOL_TIP, ev);
    pUi->textEditValValTolTip->setText(t);
    pWidgetValFntFam->set(val[_sFontFamily]);
    pWidgetValFntAtt->set(val[_sFontAttr]);
}


void cEnumValsEditWidget::setEnumTypeType(const QString& etn)
{
    enumTypeTypeName = etn;
    while (!rows.isEmpty()) delete rows.takeLast();
    pUi->tableWidgetType->setRowCount(0);
    pEnumTypeType = cColEnumType::fetchOrGet(*pq, enumTypeTypeName);

    type.clear();
    type[_sEnumValName]  = _sNul;
    type[_sEnumTypeName] = enumTypeTypeName;
    int n = type.completion(*pq);
    QString t;
    switch (n) {
    case 1:     // Beolvasva
        break;
    case 0:     // Not found (törölte a completion() metódus)
        type[_sEnumValName]  = _sNul;
        type[_sEnumTypeName] = enumTypeTypeName;
        type.setText(cEnumVal::LTX_VIEW_SHORT, enumTypeTypeName);
        type.setText(cEnumVal::LTX_VIEW_LONG,  enumTypeTypeName);
        break;
    default:    EXCEPTION(AMBIGUOUS, n, type.identifying());
    }
    t = type.getText(cEnumVal::LTX_VIEW_SHORT, enumTypeTypeName);
    pUi->lineEditTypeShort->setText(t);
    t = type.getText(cEnumVal::LTX_VIEW_LONG, enumTypeTypeName);
    pUi->lineEditTypeLong->setText(t);
    pUi->lineEditTypeTypeNote->setText(type.getNote());
    t = type.getText(cEnumVal::LTX_TOOL_TIP);
    pUi->textEditTypeTypeToolTip->setText(t);

    int row = 0;
    foreach (QString val, pEnumTypeType->enumValues) {
        pUi->tableWidgetType->setRowCount(row +1);
        rows << new cEnumValRow(*pq, val, row, this);
        ++row;
    }
    pUi->tableWidgetType->resizeRowsToContents();
    pUi->tableWidgetType->resizeColumnsToContents();
}

void cEnumValsEditWidget::setBoolTable(const QString& tn)
{
    QString sql =
       "SELECT column_name FROM information_schema.columns WHERE table_schema = 'public' AND table_name = ?"
          " AND (data_type = 'boolean' OR (data_type = 'ARRAY' AND udt_name = 'boolean'))"
            " ORDER BY column_name ASC";
    QStringList fieldList;
    if (execSql(*pq, sql, tn)) do {
        fieldList << pq->value(0).toString();
    } while (pq->next());
    pUi->comboBoxBoolField->clear();
    pUi->comboBoxBoolField->addItems(fieldList);
}

void cEnumValsEditWidget::setBoolField(const QString& fn)
{
    QString typeName = mCat(pUi->comboBoxBoolTable->currentText(), fn);

    boolType.clear();
    boolType.setName(_sEnumTypeName, typeName);
    boolType.setName(_sEnumValName,  _sNul);
    QString t;
    int n = boolType.completion(*pq);
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolType.setName(_sEnumTypeName, typeName);
        boolType.setName(_sEnumValName,  _sNul);
    }
    t = boolType.getText(cEnumVal::LTX_VIEW_SHORT);
    pUi->lineEditBoolTypeShort->setText(t);
    t = boolType.getText(cEnumVal::LTX_VIEW_LONG);
    pUi->lineEditBoolTypeLong->setText(t);
    pUi->lineEditBoolTypeNote->setText(boolType.getNote());
    t = boolType.getText(cEnumVal::LTX_TOOL_TIP);
    pUi->textEditBoolToolTip->setText(t);

    boolTrue.clear();
    boolTrue.setName(_sEnumTypeName, typeName);
    boolTrue.setName(_sEnumValName,  _sTrue);
    n = boolTrue.completion(*pq);
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolTrue.setName(_sEnumTypeName, typeName);
        boolTrue.setName(_sEnumValName,  _sTrue);
        boolTrue.setText(cEnumVal::LTX_VIEW_SHORT, langBool(true));
        boolTrue.setText(cEnumVal::LTX_VIEW_LONG,  langBool(true));
    }
    t = boolTrue.getText(cEnumVal::LTX_VIEW_SHORT, _sTrue);
    pUi->lineEditTrueShort->setText(t);
    t = boolTrue.getText(cEnumVal::LTX_VIEW_LONG, _sTrue);
    pUi->lineEditTrueLong->setText(t);
    pUi->lineEditTrueNote->setText(boolTrue.getNote());
    t = boolTrue.getText(cEnumVal::LTX_TOOL_TIP);
    pUi->textEditBoolToolTip->setText(t);
    pWidgetTrueBgColor->set(boolTrue[_sBgColor]);
    pWidgetTrueFgColor->set(boolTrue[_sFgColor]);
    pWidgetTrueFntFam->set(boolTrue[_sFontFamily]);
    pWidgetTrueFntAtt->set(boolTrue[_sFontAttr]);

    boolFalse.clear();
    boolFalse.setName(_sEnumTypeName, typeName);
    boolFalse.setName(_sEnumValName,  _sFalse);
    n = boolFalse.completion(*pq);
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolFalse.setName(_sEnumTypeName, typeName);
        boolFalse.setName(_sEnumValName,  _sFalse);
        boolFalse.setText(cEnumVal::LTX_VIEW_SHORT, langBool(false));
        boolFalse.setText(cEnumVal::LTX_VIEW_LONG,  langBool(false));
    }
    t = boolFalse.getText(cEnumVal::LTX_VIEW_SHORT, _sFalse);
    pUi->lineEditFalseShort->setText(t);
    t = boolFalse.getText(cEnumVal::LTX_VIEW_LONG, _sFalse);
    pUi->lineEditFalseLong->setText(t);
    pUi->lineEditFalseNote->setText(boolFalse.getNote());
    t = boolFalse.getText(cEnumVal::LTX_TOOL_TIP, _sFalse);
    pUi->textEditBoolToolTip->setText(t);
    pWidgetFalseBgColor->set(boolFalse[_sBgColor]);
    pWidgetFalseFgColor->set(boolFalse[_sFgColor]);
    pWidgetFalseFntFam->set(boolFalse[_sFontFamily]);
    pWidgetFalseFntAtt->set(boolFalse[_sFontAttr]);
}

const enum ePrivilegeLevel cEnumValsEdit::rights = PL_ADMIN;

cEnumValsEdit::cEnumValsEdit(QMdiArea *par)
    : cIntSubObj(par)
{
    QHBoxLayout *pLayout = new QHBoxLayout;
    this->setLayout(pLayout);
    pEditWidget = new cEnumValsEditWidget;
    pLayout->addWidget(pEditWidget);
    connect(pEditWidget, SIGNAL(closeWidget()), this, SLOT(endIt()));
}

cEnumValsEdit::~cEnumValsEdit()
{

}

/* ****** */

