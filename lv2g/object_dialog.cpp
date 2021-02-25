#include "record_table.h"
#include "object_dialog.h"
#include "lv2validator.h"
#include "popupreport.h"

#include <QComboBox>

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

cPPortTableLine::cPPortTableLine(int r, cPatchDialog *par, qlonglong _pid)
{
    parent = par;
    tableWidget = par->pUi->tableWidgetPorts;
    row    = r;
    pid = _pid;
    sharedPortRow = -1;
    if (par->readOnly) {
        comboBoxShare = nullptr;
        comboBoxPortIx = nullptr;
    }
    else {
        comboBoxShare  = new QComboBox();
        for (int i = ES_; i <= ES_NC; ++i) comboBoxShare->addItem(portShare(i));
        tableWidget->setCellWidget(row, CPP_SH_TYPE, comboBoxShare);
        comboBoxPortIx = new QComboBox();
        tableWidget->setCellWidget(row, CPP_SH_IX, comboBoxPortIx);

        connect(comboBoxShare,  SIGNAL(currentIndexChanged(int)), this, SLOT(changeShared(int)));
        connect(comboBoxPortIx, SIGNAL(currentIndexChanged(int)), this, SLOT(changePortIx(int)));

    }
}

int cPPortTableLine::getShare()
{
    if (comboBoxShare != nullptr) {
        return comboBoxShare->currentIndex();
    }
    else {
        QString ssh = _sNul;
        QTableWidgetItem *pi = tableWidget->takeItem(row, CPP_SH_TYPE);
        if (pi != nullptr) ssh = pi->text();
        return portShare(ssh);
    }
}

void cPPortTableLine::setShare(int sh)
{
    if (comboBoxShare != nullptr) {
        comboBoxShare->setCurrentIndex(sh);
    }
}


bool cPPortTableLine::setShared(const QStringList& sl)
{
    bool shOk = true;
    if (comboBoxPortIx != nullptr) {
        comboBoxPortIx->clear();
        comboBoxPortIx->addItem(_sNul);
        comboBoxPortIx->addItems(sl);
        if (sharedPortRow >= 0 && listPortIxRow.contains(sharedPortRow)) {
            int ix = listPortIxRow.indexOf(sharedPortRow);
            comboBoxPortIx->setCurrentIndex(ix +1);
        }
        else {
            sharedPortRow = -1;
            comboBoxPortIx->setCurrentIndex(0);
            shOk = false;   // Nincs megadva az elsődleges, az nem OK.
        }
    }
    return shOk;
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

// ro !!!
cPatchDialog::cPatchDialog(QWidget *parent, bool ro)
    : QDialog(parent)
    , pUi(new Ui::patchSimpleDialog)
    , readOnly(ro)
{
    pq = newQuery();
    if (sPortRefForm.isEmpty()) sPortRefForm = tr("#%1 (%2/%3)");
    pUi->setupUi(this);
    if (ro) {
        pSelectPlace = nullptr;
        pUi->buttonBox->button(QDialogButtonBox::Ok)->hide();
        pUi->buttonBox->button(QDialogButtonBox::Save)->hide();
        pUi->buttonBox->button(QDialogButtonBox::Cancel)->hide();
        pUi->toolButtonRefreshPlace->hide();
        pUi->toolButtonInfoPlace->hide();
        pUi->toolButtonNewPlace->hide();
        pUi->lineEditNamePattern->hide();
        pUi->comboBoxZone->hide();
        pUi->comboBoxPlace->hide();
        pUi->labelPlacePattern->hide();
        pUi->labelZone->hide();
        pUi->lineEditPlacePattern->hide();
        pUi->pushButtonPort1->setDisabled(true);
        pUi->pushButtonPort2->setDisabled(true);
        pUi->pushButtonPort2Shared->setDisabled(true);
        pUi->pushButtonShAB->setDisabled(true);
        pUi->pushButtonShDel->setDisabled(true);
        pUi->pushButtonAddPorts->setDisabled(true);
        pUi->lineEditName->setReadOnly(true);
        pUi->lineEditNote->setReadOnly(true);
        pUi->lineEditTagPattern->setDisabled(true);
        pUi->spinBoxTo->setDisabled(true);
        pUi->spinBoxFrom->setDisabled(true);
        pUi->spinBoxTagOffs->setDisabled(true);
        pUi->spinBoxNameOffs->setDisabled(true);
        pUi->tableWidgetPorts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
    else {
        pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
        pUi->buttonBox->button(QDialogButtonBox::Save)->setDisabled(true);
        pUi->buttonBox->button(QDialogButtonBox::Close)->hide();
        pUi->lineEditPlace->hide();
        pSelectPlace = new cSelectPlace(pUi->comboBoxZone, pUi->comboBoxPlace, pUi->lineEditPlacePattern, nullptr, this);
        pSelectPlace->setPlaceRefreshButton(pUi->toolButtonRefreshPlace);
        pSelectPlace->setPlaceEditButton(pUi->toolButtonInfoPlace);
        pSelectPlace->setPlaceInsertButton(pUi->toolButtonNewPlace);
        cIntValidator *intValidator = new cIntValidator(false);
        pUi->tableWidgetPorts->setItemDelegateForColumn(CPP_INDEX, new cItemDelegateValidator(intValidator));
        connect(pUi->tableWidgetPorts->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    }
    shOk = pNamesOk = pIxOk = true;
    lockSlot = false;
}

cPatchDialog::~cPatchDialog()
{
    delete pq;
}

cPatch * cPatchDialog::getPatch()
{
    if (readOnly) EXCEPTION(ECONTEXT);
    cPatch *p = new cPatch();
    p->setId(_sNodeType, ENUM2SET(NT_PATCH));
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
        pp->setId(rowsData.at(i)->pid);
        p->ports << pp;
    }
    for (i = 0; i < n && p != nullptr; i++) {
        int sh = int(p->ports[i]->getId(_sSharedCable));
        if (!(sh == ES_A || sh == ES_AA)) continue;
        int a = i, b = -1, ab = -1, bb  = -1;
        bool cd = false;
        bool ok = true;
        for (int j = 0; ok && j < n && p != nullptr; j++) {
            if (rowsData.at(j)->sharedPortRow == i) {
                sh = int(p->ports[j]->getId(_sSharedCable));
                switch (sh) {
                case ES_AB:
                    ok = ab == -1;
                    ab = j;
                    break;
                case ES_B:
                case ES_BA:
                    ok = b  == -1;
                    b  = j;
                    break;
                case ES_BB:
                    ok = bb == -1;
                    bb = j;
                    break;
                case ES_C:
                    ok = ab == -1;
                    ab = j;
                    cd = true;
                    break;
                case ES_D:
                    ok = b  == -1;
                    b  = j;
                    cd = true;
                    break;
                default:
                    break;
                }
            }
        }
        ok = ok && p->setShare(a, ab, b, bb, cd);
        if (!ok) {
            QString msg = tr("A megadott kábelmegosztás nem állítható be (bázis port #%1)").arg(i);
            cMsgBox::warning(msg, this);
            pDelete(p);
            break;
        }
    }
    return p;
}

void cPatchDialog::setPatch(const cPatch *pSample)
{
    qlonglong id = pSample->getId();
    if (id == NULL_ID)  pUi->lineEditID->clear();
    else                pUi->lineEditID->setText(QString::number(id));
    pUi->lineEditName->setText(pSample->getName());
    pUi->lineEditNote->setText(pSample->getNote());
    if (readOnly) {
        qlonglong placeId = pSample->getId(_sPlaceId);
        QString placeName = cPlace().getNameById(*pq, placeId, EX_IGNORE);
        pUi->lineEditPlace->setText(placeName);
    }
    else {
        _setPlaceComboBoxs(pSample->getId(_sPlaceId), pUi->comboBoxZone, pUi->comboBoxPlace, true);
    }
    clearRows();
    int n = pSample->ports.size();
    if (n > 0) {
        int i;
        setRows(n, &pSample->ports.list()); // sportok/sorok száma, és port id
        for (i = 0; i < n; i++) {
            const cPPort *pp = pSample->ports.at(i)->reconvert<cPPort>();
            setTableItemText(pp->getName(),              pUi->tableWidgetPorts, i, CPP_NAME);
            setTableItemText(pp->getNote(),              pUi->tableWidgetPorts, i, CPP_NOTE);
            setTableItemText(pp->getName(_sPortTag),     pUi->tableWidgetPorts, i, CPP_TAG);
            setTableItemText(pp->getName(_sPortIndex),   pUi->tableWidgetPorts, i, CPP_INDEX);
            int s = int(pp->getId(__sSharedCable));
            if (readOnly) {
                setTableItemText(portShare(s), pUi->tableWidgetPorts, i, CPP_SH_TYPE);
                qlonglong spid = pp->getId(__sSharedPortId);
                if (spid != NULL_ID) {
                    cPPort sp;
                    sp.setById(*pq, spid);
                    if (pp->getId(_sNodeId) == sp.getId(_sNodeId)) {
                        setTableItemText(sp.getName(), pUi->tableWidgetPorts, i, CPP_SH_IX);
                    }
                    else {
                        QTableWidgetItem *pi = new QTableWidgetItem(sp.getFullName(*pq));
                        QFont font;
                        font.setBold(true);
                        pi->setFont(font);
                        pi->setForeground(QBrush(Qt::red));
                        pUi->tableWidgetPorts->setItem(i, CPP_SH_IX, pi);
                    }
                }
            }
            else {
                setPortShare(i, s);
            }
        }
        if (readOnly) return;
        // Másodlagos megosztáshoz tartozó elsődleges portok
        // Elvileg adatbázisból származó objektum, így ki vannak töltve az ID-k
        // Ha mégse, akkor ezek a hivatkozások elvesznek,
        // ha a hivatkozás hibás, szintén.
        for (i = 0; i < n; i++) {
            const cPPort *pp = pSample->ports.at(i)->reconvert<cPPort>();
            if (pp->isNull(_sSharedPortId)) continue;
            qlonglong spid = pp->getId(_sSharedPortId);
            int ix = pSample->ports.indexOf(spid);  // Hivatkozott port index
            if (ix < 0) {       //
                QString msg = tr("A %1 nevű megosztott port a %2 ID-jű portra hivatkozik, amit nem találok.").
                        arg(pp->getName()).arg(spid);
                cMsgBox::warning(msg, this);
                continue;
            }
            ix = rowsData[i]->listPortIxRow.indexOf(ix);    // Kiválasztható ?
            if (ix < 0) {                   // nem találta
                QString msg = tr("A %1 nevű megosztott port a %2 ID-jű portra hivatkozik, ami nem választható ki.").
                        arg(pp->getName()).arg(spid);
                cMsgBox::warning(msg, this);
                continue;
            }
            rowsData[i]->comboBoxPortIx->setCurrentIndex(ix +1);
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

void cPatchDialog::setRows(int rows, const QList<cNPort *> *pports)
{
    int i = pUi->tableWidgetPorts->rowCount();
    if (i != rowsData.size()) EXCEPTION(EDATA);
    if (i == rows) return;
    if (i < rows) {
        pUi->tableWidgetPorts->setRowCount(rows);
        for (; i < rows; ++i) {
            qlonglong pid = NULL_ID;
            if (pports != nullptr && isContIx(*pports, i)) pid = pports->at(i)->getId();
            rowsData << new cPPortTableLine(i, this, pid);
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
    rowsData[row]->setShare(sh);
}

#define _LOCKSLOTS()    _lockSlotSave_ = lockSlot; lockSlot = true
#define LOCKSLOTS()     bool _LOCKSLOTS()
#define UNLOCKSLOTS()   lockSlot = _lockSlotSave_

void cPatchDialog::updateSharedIndexs()
{
    // Az összes elsödleges megosztás listája (sor indexek)
    shPrimRows.clear();
    foreach (cPPortTableLine *pRow, rowsData) {
        int sh = pRow->getShare();              // megosztás típusa
        if (sh == ES_A || sh == ES_AA) shPrimRows << pRow->row;// Ha elsődleges megosztás (amire hivatkozik a többi)
    }
    // MAP az elsődleges megosztásokra mutatók listái
    shPrimMap.clear();
    QList<int> secondIxs;
    foreach (cPPortTableLine *pRow, rowsData) {
        int sh = pRow->getShare();   // megosztás típusa
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
            int mysh = pRow->getShare(); // Az aktuális másodlagos megosztás típusa
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
                            break;
                        case ES_BB:
                            collision = ssh == ES_B;
                            break;
                        case ES_C:
                        case ES_D:
                            collision = ssh == ES_B || ssh == ES_AB;
                            break;
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
            shOk = pRow->setShared(sl);
        }
        else {                                  // Elsodleges megosztott, vagy nem megosztott
            pRow->sharedPortRow = -1;
            pRow->setShared(QStringList());
        }
    }
    UNLOCKSLOTS();
    on_lineEditName_textChanged(pUi->lineEditName->text());  // Menthető ?
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

QList<int>  cPatchDialog::selectedRows() const
{
    QModelIndexList mil = pUi->tableWidgetPorts->selectionModel()->selectedRows();
    QList<int>  rows;   // Törlendő sorok sorszámai
    foreach (QModelIndex mi, mil) {
        rows << mi.row();
    }
    // sorba rendezzük, mert majd a vége felül törlünk
    std::sort(rows.begin(), rows.end());
    return rows;
}

/* SLOTS */
void cPatchDialog::on_lineEditName_textChanged(const QString& name)
{
    bool f = name.isEmpty() || pUi->tableWidgetPorts->rowCount() == 0 || !shOk || !pNamesOk || !pIxOk;
    pUi->buttonBox->button(QDialogButtonBox::Ok)  ->setDisabled(f);
    pUi->buttonBox->button(QDialogButtonBox::Save)->setDisabled(f);
}

void cPatchDialog::on_pushButtonPort1_clicked()
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

void cPatchDialog::on_pushButtonPort2_clicked()
{
    on_pushButtonPort1_clicked();
    LOCKSLOTS();
    setRows(2);
    QTableWidgetItem *pItem;
    pItem = new QTableWidgetItem("J2");
    pUi->tableWidgetPorts->setItem(1, CPP_NAME, pItem);
    pItem = new QTableWidgetItem("2");
    pUi->tableWidgetPorts->setItem(1, CPP_INDEX, pItem);
    UNLOCKSLOTS();
}

void cPatchDialog::on_pushButtonPort2Shared_clicked()
{
    on_pushButtonPort2_clicked();
    setPortShare(0, ES_A);
    setPortShare(1, ES_B);
    rowsData[1]->comboBoxPortIx->setCurrentIndex(1);    // Az első portra mutat
}

void cPatchDialog::on_pushButtonAddPorts_clicked()
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
        QString tag  = nameAndNumber(tagPattern, i + tagOffs);
        pItem = new QTableWidgetItem(name);
        pUi->tableWidgetPorts->setItem(row, CPP_NAME, pItem);
        pItem = new QTableWidgetItem(QString::number(i));
        pUi->tableWidgetPorts->setItem(row, CPP_INDEX, pItem);
        pItem = new QTableWidgetItem(tag);
        pUi->tableWidgetPorts->setItem(row, CPP_TAG, pItem);
    }
    UNLOCKSLOTS();
    updatePNameIxOk();
    on_lineEditName_textChanged(pUi->lineEditName->text());
}

void cPatchDialog::on_pushButtonDelPort_clicked()
{
    QList<int>  rows = selectedRows();   // Törlendő sorok sorszámai, rendezett
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
    on_lineEditName_textChanged(pUi->lineEditName->text());
}

void cPatchDialog::on_spinBoxFrom_valueChanged(int i)
{
    bool f = (pUi->spinBoxTo->value() - i) >= 0;
    pUi->pushButtonAddPorts->setEnabled(f);
}

void cPatchDialog::on_spinBoxTo_valueChanged(int i)
{
    bool f = (i - pUi->spinBoxFrom->value()) >= 0;
    pUi->pushButtonAddPorts->setEnabled(f);
}

void cPatchDialog::on_tableWidgetPorts_cellChanged(int row, int col)
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
    on_lineEditName_textChanged(pUi->lineEditName->text());
}

void cPatchDialog::selectionChanged(const QItemSelection &, const QItemSelection &)
{
    QModelIndexList mil = pUi->tableWidgetPorts->selectionModel()->selectedRows();
    int n = mil.size();
    bool empty = n == 0;
    pUi->pushButtonDelPort->setDisabled(empty);
    pUi->pushButtonShDel->setDisabled(empty);
    pUi->pushButtonShAB->setDisabled(n < 2 || 0 != (n % 2));
    pUi->pushButtonShNC->setDisabled(empty);
}

void cPatchDialog::on_pushButtonShAB_clicked()
{
    QList<int>  rows = selectedRows();   // Kijelölt sorok sorszámai, rendezett
    int i, n = rows.size();
    for (i = 0; (i + 1) < n; i += 2) {
        int ixA = rows.at(i);
        int ixB = rows.at(i +1);
        setPortShare(ixA, ES_A);
        setPortShare(ixB, ES_B);
        cPPortTableLine * rowB = rowsData.at(ixB);
        int cix = rowB->listPortIxRow.indexOf(ixA);
        if (cix >= 0) rowB->comboBoxPortIx->setCurrentIndex(cix +1);    // First item is NULL
    }

}

void cPatchDialog::on_pushButtonShDel_clicked()
{
    QList<int>  rows = selectedRows();   // Kijelölt sorok sorszámai, rendezett
    foreach (int i, rows) {
        setPortShare(i   , ES_);
    }
}

void cPatchDialog::on_pushButtonShNC_clicked()
{
    QList<int>  rows = selectedRows();   // Kijelölt sorok sorszámai, rendezett
    foreach (int i, rows) {
        setPortShare(i   , ES_NC);
    }
}


void cPatchDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (pUi->buttonBox->button(QDialogButtonBox::Save) == button) {
        done(QDialogButtonBox::Save);
    }
}


cPatch * patchInsertDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample)
{
    cPatchDialog dialog(pPar, false);
    if (pSample != nullptr) {
        pSample->clearIdsOnly();
        dialog.setPatch(pSample);
    }
    cPatch *p;
    while (true) {
        int r = dialog.exec();
        if (r != QDialog::Accepted) return nullptr;
        p = dialog.getPatch();
        if (p == nullptr) continue;
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
    if (pSample == nullptr) EXCEPTION(EPROGFAIL);
    dialog.setPatch(pSample);
    qlonglong id = pSample->getId();
    if (id == NULL_ID) EXCEPTION(EDATA);
    cPatch *p = nullptr;
    while (true) {
        int r = dialog.exec();
        if (r != QDialog::Accepted) return nullptr;
        p = dialog.getPatch();
        if (p == nullptr) continue;
        p->setContainerValid(CV_PORTS | CV_PATCH_BACK_SHARE);
        p->ports.setsOwnerId(id);
        if (!cErrorMessageBox::condMsgBox(p->tryRewriteById(q))) {
            pDelete(p);
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
    po->setAddress(actAddress, pModelIpType->currentName());
    po->setId(_sSubNetId, pSelectSubNet->currentId());
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
    rec.setName(cEnumVal::ixEnumTypeName(), parent->enumTypeTypeName);
    rec.setName(cEnumVal::ixEnumValName(), _val);
    int r = rec.completion(q);
    QString t;
    switch (r) {
    case 1:     // Beolvasva
        rec.fetchText(q);
        break;
    case 0:     // Még nincs az adatbázisban  Set default
        rec.setName(cEnumVal::ixEnumTypeName(), parent->enumTypeTypeName);
        rec.setName(cEnumVal::ixEnumValName(), _val);
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

    pBgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sBgColor), rec[cEnumVal::ixBgColor()], false, nullptr);
    pTableWidget->setCellWidget(row, CEV_BG_COLOR, pBgColorWidget);

    pFgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFgColor), rec[cEnumVal::ixFgColor()], false, nullptr);
    pTableWidget->setCellWidget(row, CEV_FG_COLOR, pFgColorWidget);

    pFntFamWidget  = new cFontFamilyWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFontFamily), rec[cEnumVal::ixFontFamily()], nullptr);
    pTableWidget->setCellWidget(row, CEV_FNT_FAM, pFntFamWidget);

    pFntAttWidget  = new cFontAttrWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFontAttr), rec[cEnumVal::ixFontAttr()], false, nullptr);
    pTableWidget->setCellWidget(row, CEV_FNT_ATT, pFntAttWidget);

    pComboBoxIcon = new QComboBox;
    pModelIcon    = new cResourceIconsModel;
    pModelIcon->joinWith(pComboBoxIcon);
    pTableWidget->setCellWidget(row, CEV_ICON, pComboBoxIcon);
    QString s = rec.getName(_sIcon);
    pModelIcon->setCurrent(s);
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
    rec[cEnumVal::ixBgColor()]     = pBgColorWidget->get();
    rec[cEnumVal::ixFgColor()]     = pFgColorWidget->get();
    rec[cEnumVal::ixFontFamily()]  = pFntFamWidget->get();
    rec[cEnumVal::ixFontAttr()]    = pFntAttWidget->get();
    QString s;
    s = getTableItemText(pTableWidget, row, CEV_NOTE);
    if (s.isEmpty()) rec.clear(_sEnumValNote);
    else             rec[_sEnumValNote] = s;
    s = pComboBoxIcon->currentText();
    if (s.isEmpty()) rec.clear(_sIcon);
    else             rec[_sIcon] = s;

    rec.replace(q);
    rec.saveText(q);
}

cEnumValsEditWidget::cEnumValsEditWidget(QWidget *parent)
    : QWidget(parent)
{
    pEnumTypeType = nullptr;
    pEnumValType  = nullptr;
    pq = newQuery();
    pUi = new Ui::enumValsWidget;
    pUi->setupUi(this);
    // pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    // pUi->buttonBox->button(QDialogButtonBox::Save)->setDisabled(true);
    connect(pUi->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));

    pSelLang = new cSelectLanguage(pUi->comboBoxLang, pUi->labelFlag, false, this);
    langId = pSelLang->currentLangId();

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
    pWidgetValBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), val[_sBgColor], false, nullptr);
    pWidgetValBgColor->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValBgColor, pWidgetValBgColor);

    pWidgetValFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), val[_sFgColor], false, nullptr);
    pWidgetValFgColor->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFgColor, pWidgetValFgColor);

    pWidgetValFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), val[_sFontFamily], nullptr);
    pWidgetValFntFam->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFontFamily, pWidgetValFntFam);

    pWidgetValFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), val[_sFontAttr], false, nullptr);
    pWidgetValFntAtt->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFontAttr, pWidgetValFntAtt);

    pModelValIcon = new cResourceIconsModel;
    pModelValIcon->joinWith(pUi->comboBoxValIcon);

    connect(pUi->comboBoxValType,  SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumValType(QString)));
    connect(pUi->comboBoxValVal,   SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumValVal(QString)));
    pUi->comboBoxValType->addItems(typeList);


    // Type
    pModelTypeIcon = new cResourceIconsModel;
    pModelTypeIcon->joinWith(pUi->comboBoxTypeIcon);
    connect(pUi->comboBoxTypeType, SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumTypeType(QString)));
    pUi->comboBoxTypeType->addItems(typeList);

    // Boolean
    pModelBoolIcon = new cResourceIconsModel;
    pModelBoolIcon->joinWith(pUi->comboBoxBoolIcon);
        // true
    pWidgetTrueBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), boolTrue[_sBgColor], false, nullptr);
    pWidgetTrueBgColor->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueBgColor, pWidgetTrueBgColor);

    pWidgetTrueFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), boolTrue[_sFgColor], false, nullptr);
    pWidgetTrueFgColor->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFgColor, pWidgetTrueFgColor);

    pWidgetTrueFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), boolTrue[_sFontFamily], nullptr);
    pWidgetTrueFntFam->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFontFam, pWidgetTrueFntFam);

    pWidgetTrueFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), boolTrue[_sFontAttr], false, nullptr);
    pWidgetTrueFntAtt->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFntAtt, pWidgetTrueFntAtt);

    pModelTrueIcon = new cResourceIconsModel;
    pModelTrueIcon->joinWith(pUi->comboBoxTrueIcon);
        // false
    pWidgetFalseBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), boolFalse[_sBgColor], false, nullptr);
    pWidgetFalseBgColor->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseBgColor, pWidgetFalseBgColor);

    pWidgetFalseFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), boolFalse[_sFgColor], false, nullptr);
    pWidgetFalseFgColor->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFgColor, pWidgetFalseFgColor);

    pWidgetFalseFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), boolFalse[_sFontFamily], nullptr);
    pWidgetFalseFntFam->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFntFam, pWidgetFalseFntFam);

    pWidgetFalseFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), boolFalse[_sFontAttr], false, nullptr);
    pWidgetFalseFntAtt->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFntAtt, pWidgetFalseFntAtt);

    pModelFalseIcon = new cResourceIconsModel;
    pModelFalseIcon->joinWith(pUi->comboBoxFalseIcon);

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
    val[cEnumVal::ixBgColor()]      = pWidgetValBgColor->get();
    val[cEnumVal::ixFgColor()]      = pWidgetValFgColor->get();
    val.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditValShort->text());
    val.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditValLong->text());
    val.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditValValTolTip->toPlainText());
    val[cEnumVal::ixFontFamily()]   = pWidgetValFntFam->get();
    val[cEnumVal::ixFontAttr()]     = pWidgetValFntAtt->get();
    QString s;
    s = pUi->lineEditValValNote->text();
    if (s.isEmpty()) val.clear(_sEnumValName);
    else             val[_sEnumValNote]  = s;
    s = pUi->comboBoxValIcon->currentText();
    if (s.isEmpty()) val.clear(_sIcon);
    else             val[_sIcon] = s;

    return cErrorMessageBox::condMsgBox(val.tryReplace(*pq, TS_NULL, true));
}

bool cEnumValsEditWidget::saveType()
{
    QString s;
    type.clearId();
    type.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditTypeShort->text());
    type.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditTypeLong->text());
    type.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditTypeTypeToolTip->toPlainText());
    s  = pUi->lineEditTypeTypeNote->text();
    if (s.isEmpty()) type.clear(_sEnumValNote);
    else             type[_sEnumValNote] = s;
    s = pUi->comboBoxTypeIcon->currentText();
    if (s.isEmpty()) type.clear(_sIcon);
    else             type[_sIcon] = s;

    cError *pe = nullptr;
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
    if (pe != nullptr) {
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
    QString s;

    boolType.clear(cEnumVal::ixEnumValName());
    boolType[cEnumVal::ixEnumTypeName()] = type;
    boolType.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditBoolTypeShort->text());
    boolType.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditBoolTypeLong->text());
    boolType.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditBoolToolTip->toPlainText());
    s  = pUi->lineEditBoolTypeNote->text();
    if (s.isEmpty()) boolType.clear(_sEnumValNote);
    else             boolType[_sEnumValNote] = s;;
    s = pUi->comboBoxBoolIcon->currentText();
    if (s.isEmpty()) boolType.clear(_sIcon);
    else             boolType[_sIcon] = s;

    boolTrue[cEnumVal::ixEnumValName()]  = _sTrue;
    boolTrue[cEnumVal::ixEnumTypeName()] = type;
    boolTrue[cEnumVal::ixBgColor()]      = pWidgetTrueBgColor->get();
    boolTrue[cEnumVal::ixFgColor()]      = pWidgetTrueFgColor->get();
    boolTrue.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditTrueShort->text());
    boolTrue.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditTrueLong->text());
    boolTrue.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditTrueToolTip->toPlainText());
    boolTrue[cEnumVal::ixFontFamily()]   = pWidgetTrueFntFam->get();
    boolTrue[cEnumVal::ixFontAttr()]     = pWidgetTrueFntAtt->get();
    s  = pUi->lineEditTrueNote->text();
    if (s.isEmpty()) boolTrue.clear(_sEnumValNote);
    else             boolTrue[_sEnumValNote] = s;
    s = pUi->comboBoxTrueIcon->currentText();
    if (s.isEmpty()) boolTrue.clear(_sIcon);
    else             boolTrue[_sIcon] = s;

    boolFalse[cEnumVal::ixEnumValName()] = _sFalse;
    boolFalse[cEnumVal::ixEnumTypeName()]= type;
    boolFalse[cEnumVal::ixBgColor()]     = pWidgetFalseBgColor->get();
    boolFalse[cEnumVal::ixFgColor()]     = pWidgetFalseFgColor->get();
    boolFalse.setText(cEnumVal::LTX_VIEW_SHORT, pUi->lineEditFalseShort->text());
    boolFalse.setText(cEnumVal::LTX_VIEW_LONG,  pUi->lineEditFalseLong->text());
    boolFalse.setText(cEnumVal::LTX_TOOL_TIP,   pUi->textEditFalseToolTip->toPlainText());
    boolFalse[cEnumVal::ixFontFamily()]  = pWidgetFalseFntFam->get();
    boolFalse[cEnumVal::ixFontAttr()]    = pWidgetFalseFntAtt->get();
    s  = pUi->lineEditFalseNote->text();
    if (s.isEmpty()) boolFalse.clear(_sEnumValNote);
    else             boolFalse[_sEnumValNote] = s;
    s = pUi->comboBoxFalseIcon->currentText();
    if (s.isEmpty()) boolFalse.clear(_sIcon);
    else             boolFalse[_sIcon] = s;

    cError *pe = nullptr;
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
    if (pe != nullptr) {
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
    if (pEnumValType  == nullptr || enumValTypeName.isEmpty()) return;
    val.clear();
    QString ev = _ev;
    if (ev.isNull()) ev = QString("");  // Üres lehet, de NULL nem!
    val[cEnumVal::ixEnumValName()]  = ev;
    val[cEnumVal::ixEnumTypeName()] = enumValTypeName;
    int n = val.completion(*pq);
    QString t;
    switch (n) {
    case 1:     // Beolvasva
        break;
    case 0:     // Not found (az enumVal-t törölte a completion() metódus)
        val[cEnumVal::ixEnumValName()]  = ev;
        val[cEnumVal::ixEnumTypeName()] = enumValTypeName;
        val.setText(cEnumVal::LTX_VIEW_SHORT, ev);
        val.setText(cEnumVal::LTX_VIEW_LONG,  ev);
        break;
    default:    EXCEPTION(AMBIGUOUS, n, val.identifying());
    }
    pWidgetValBgColor->set(val[cEnumVal::ixBgColor()]);
    pWidgetValFgColor->set(val[cEnumVal::ixFgColor()]);
    t = val.getText(cEnumVal::LTX_VIEW_SHORT, ev);
    pUi->lineEditValShort->setText(t);
    t = val.getText(cEnumVal::LTX_VIEW_LONG, ev);
    pUi->lineEditValLong->setText(t);
    pUi->lineEditValValNote->setText(val.getNote());
    t = val.getText(cEnumVal::LTX_TOOL_TIP, ev);
    pUi->textEditValValTolTip->setText(t);
    pWidgetValFntFam->set(val[cEnumVal::ixFontFamily()]);
    pWidgetValFntAtt->set(val[cEnumVal::ixFontAttr()]);
    t = val.getName(_sIcon);
    pModelValIcon->setCurrent(t);
}


void cEnumValsEditWidget::setEnumTypeType(const QString& etn)
{
    enumTypeTypeName = etn;
    while (!rows.isEmpty()) delete rows.takeLast();
    pUi->tableWidgetType->setRowCount(0);
    pEnumTypeType = cColEnumType::fetchOrGet(*pq, enumTypeTypeName);

    type.clear();
    type[cEnumVal::ixEnumTypeName()] = enumTypeTypeName;
    type.fetch(*pq, false, type.mask(type.ixEnumTypeName(), type.ixEnumValName()));
    int n = pq->size();
    QString t;
    switch (n) {
    case 1:     // Readed one record
        break;
    case 0:     // Not found, init defaults
        type[cEnumVal::ixEnumTypeName()] = enumTypeTypeName;
        type.setText(cEnumVal::LTX_VIEW_SHORT, enumTypeTypeName);
        type.setText(cEnumVal::LTX_VIEW_LONG,  enumTypeTypeName);
        break;
    default:    // Database data error
        EXCEPTION(AMBIGUOUS, n, type.identifying());
    }
    t = type.getText(cEnumVal::LTX_VIEW_SHORT, enumTypeTypeName);
    pUi->lineEditTypeShort->setText(t);
    t = type.getText(cEnumVal::LTX_VIEW_LONG, enumTypeTypeName);
    pUi->lineEditTypeLong->setText(t);
    pUi->lineEditTypeTypeNote->setText(type.getNote());
    t = type.getText(cEnumVal::LTX_TOOL_TIP);
    pUi->textEditTypeTypeToolTip->setText(t);
    t = type.getName(_sIcon);
    pModelTypeIcon->setCurrent(t);

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
    boolType.setName(cEnumVal::ixEnumTypeName(), typeName);
    boolType.clear(cEnumVal::ixEnumValName());
    QString t;
    int n = boolType.fetch(*pq, false, boolType.mask(cEnumVal::ixEnumValName(), cEnumVal::ixEnumTypeName()));
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolType.setName(cEnumVal::ixEnumTypeName(), typeName);
        break;
    default:
        EXCEPTION(EPROGFAIL, n);
    }
    t = boolType.getText(cEnumVal::LTX_VIEW_SHORT);
    pUi->lineEditBoolTypeShort->setText(t);
    t = boolType.getText(cEnumVal::LTX_VIEW_LONG);
    pUi->lineEditBoolTypeLong->setText(t);
    pUi->lineEditBoolTypeNote->setText(boolType.getNote());
    t = boolType.getText(cEnumVal::LTX_TOOL_TIP);
    pUi->textEditBoolToolTip->setText(t);
    t = boolType.getName(_sIcon);
    pModelBoolIcon->setCurrent(t);

    boolTrue.clear();
    boolTrue.setName(cEnumVal::ixEnumTypeName(), typeName);
    boolTrue.setName(cEnumVal::ixEnumValName(),  _sTrue);
    n = boolTrue.completion(*pq);
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolTrue.setName(cEnumVal::ixEnumTypeName(), typeName);
        boolTrue.setName(cEnumVal::ixEnumValName(),  _sTrue);
        boolTrue.setText(cEnumVal::LTX_VIEW_SHORT, langBool(true));
        boolTrue.setText(cEnumVal::LTX_VIEW_LONG,  langBool(true));
        break;
    default:
        EXCEPTION(EPROGFAIL, n);
    }
    t = boolTrue.getText(cEnumVal::LTX_VIEW_SHORT, _sTrue);
    pUi->lineEditTrueShort->setText(t);
    t = boolTrue.getText(cEnumVal::LTX_VIEW_LONG, _sTrue);
    pUi->lineEditTrueLong->setText(t);
    pUi->lineEditTrueNote->setText(boolTrue.getNote());
    t = boolTrue.getText(cEnumVal::LTX_TOOL_TIP);
    pUi->textEditBoolToolTip->setText(t);
    pWidgetTrueBgColor->set(boolTrue[cEnumVal::ixBgColor()]);
    pWidgetTrueFgColor->set(boolTrue[cEnumVal::ixFgColor()]);
    pWidgetTrueFntFam->set(boolTrue[cEnumVal::ixFontFamily()]);
    pWidgetTrueFntAtt->set(boolTrue[cEnumVal::ixFontAttr()]);
    t = boolTrue.getName(_sIcon);
    pModelTrueIcon->setCurrent(t);

    boolFalse.clear();
    boolFalse.setName(cEnumVal::ixEnumTypeName(), typeName);
    boolFalse.setName(cEnumVal::ixEnumValName(),  _sFalse);
    n = boolFalse.completion(*pq);
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolFalse.setName(cEnumVal::ixEnumTypeName(), typeName);
        boolFalse.setName(cEnumVal::ixEnumValName(),  _sFalse);
        boolFalse.setText(cEnumVal::LTX_VIEW_SHORT, langBool(false));
        boolFalse.setText(cEnumVal::LTX_VIEW_LONG,  langBool(false));
        break;
    default:
        EXCEPTION(EPROGFAIL, n);
    }
    t = boolFalse.getText(cEnumVal::LTX_VIEW_SHORT, _sFalse);
    pUi->lineEditFalseShort->setText(t);
    t = boolFalse.getText(cEnumVal::LTX_VIEW_LONG, _sFalse);
    pUi->lineEditFalseLong->setText(t);
    pUi->lineEditFalseNote->setText(boolFalse.getNote());
    t = boolFalse.getText(cEnumVal::LTX_TOOL_TIP, _sFalse);
    pUi->textEditBoolToolTip->setText(t);
    pWidgetFalseBgColor->set(boolFalse[cEnumVal::ixBgColor()]);
    pWidgetFalseFgColor->set(boolFalse[cEnumVal::ixFgColor()]);
    pWidgetFalseFntFam->set(boolFalse[cEnumVal::ixFontFamily()]);
    pWidgetFalseFntAtt->set(boolFalse[cEnumVal::ixFontAttr()]);
    t = boolFalse.getName(_sIcon);
    pModelFalseIcon->setCurrent(t);
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
