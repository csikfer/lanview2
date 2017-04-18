#include "record_table.h"
#include "object_dialog.h"
#include "lv2validator.h"

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

cPatchDialog::cPatchDialog(QWidget *parent)
    : QDialog(parent)
    , pUi(new Ui::patchSimpleDialog)
{
    pq = newQuery();
    if (sPortRefForm.isEmpty()) sPortRefForm = trUtf8("#%1 (%2/%3)");
    pUi->setupUi(this);
    pModelZone  = new cZoneListModel;
    pUi->comboBoxZone->setModel(pModelZone);
    pModelPlace = new cPlacesInZoneModel;
    pUi->comboBoxPlace->setModel(pModelPlace);
    pUi->comboBoxPlace->setCurrentIndex(0);
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    cIntValidator *intValidator = new cIntValidator(false);
    pUi->tableWidgetPorts->setItemDelegateForColumn(CPP_INDEX, new cItemDelegateValidator(intValidator));

    connect(pUi->lineEditName,      SIGNAL(textChanged(QString)),       this, SLOT(changeName(QString)));
    connect(pUi->comboBoxZone,      SIGNAL(currentIndexChanged(int)),   this, SLOT(changeFilterZone(int)));
    connect(pUi->toolButtonNewPlace,SIGNAL(pressed()),                  this, SLOT(newPlace()));
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
    int i = pUi->comboBoxPlace->currentIndex();
    if (i > 0) p->setId(_sPlaceId, pModelPlace->atId(i));
    int n = pUi->tableWidgetPorts->rowCount();
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
                QMessageBox::warning(this, design().titleError, msg,QMessageBox::Ok);
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
                QMessageBox::warning(this, design().titleError, msg,QMessageBox::Ok);
                continue;
            }
            ix = rowsData[i]->listPortIxRow.indexOf(ix);    // Kiválasztható ?
            if (ix < 0) {                   // nem találta
                QString msg = trUtf8("A %1 nevű megosztott port a %2 ID-jű portra hivatkozik, amit nem választható ki.").
                        arg(pp->getName()).arg(spid);
                QMessageBox::warning(this, design().titleError, msg,QMessageBox::Ok);
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

void cPatchDialog::changeFilterZone(int i)
{
    qlonglong id = pModelZone->atId(i);
    pModelPlace->setFilter(id);
}

void cPatchDialog::newPlace()
{
    cRecord *p = recordInsertDialog(*pq, _sPlaces, this);
    if (p != NULL) {
        changeFilterZone(pUi->comboBoxZone->currentIndex());
        QString placeName = p->getName();
        int ix = pUi->comboBoxPlace->findText(placeName);
        if (ix > 0) pUi->comboBoxPlace->setCurrentIndex(ix);
        delete p;
    }
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

cPatch * patchDialog(QSqlQuery& q, QWidget *pPar, cPatch * pSample)
{
    cPatchDialog dialog(pPar);
    if (pSample != NULL) {
        dialog.setPatch(pSample);
    }
    cPatch *p;
    while (true) {
        int r = dialog.exec();
        if (r != QDialog::Accepted) return NULL;
        p = dialog.getPatch();
        if (p == NULL) continue;
        if (!cErrorMessageBox::condMsgBox(p->tryInsert(q, true))) {
            delete p;
            continue;
        }
        break;
    }
    return p;
}

/* ********************************************************************************************** */


cEnumValRow::cEnumValRow(QSqlQuery& q, const QString& _val, int _row, cEnumValsDialog *par)
    :QObject(par), row(_row)
{
    parent = par;
    pTableWidget = par->pUi->tableWidget;
    rec.setName(_sEnumTypeName, parent->enumTypeName);
    rec.setName(_sEnumValName, _val);
    int r = rec.completion(q);
    if (1 < r) EXCEPTION(AMBIGUOUS, r, mCat(parent->enumTypeName, _val));
    if (r == 0) {   // Még nincs az adatbázisban
        // Set default
        rec.setName(_sEnumTypeName, parent->enumTypeName);
        rec.setName(_sEnumValName, _val);
        rec.setName(_sViewShort,   _val);
        rec.setName(_sViewLong,    _val);
    }
    setTableItemText(_val,                     pTableWidget, row, CEV_NAME);
    setTableItemText(rec.getName(_sViewShort), pTableWidget, row, CEV_SHORT);
    setTableItemText(rec.getName(_sViewLong),  pTableWidget, row, CEV_LONG);
    pBgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sBgColor), rec[_sBgColor], false, NULL);
    pTableWidget->setCellWidget(row, CEV_BG_COLOR, pBgColorWidget->pWidget());
    pFgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFgColor), rec[_sFgColor], false, NULL);
    pTableWidget->setCellWidget(row, CEV_FG_COLOR, pFgColorWidget->pWidget());
    // font ...
    setTableItemText(rec.getName(_sEnumValNote),  pTableWidget, row, CEV_NOTE);
}


cEnumValsDialog::cEnumValsDialog(QWidget *parent)
    : QDialog(parent)
{
    pEnumType = NULL;
    pq = newQuery();
    pUi = new Ui::enumValsDialog;
    pUi->setupUi(this);
    pUi->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    pUi->buttonBox->button(QDialogButtonBox::Save)->setDisabled(true);
    connect(pUi->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
    // Egy rekord (az első tab lesst)
    if (!pShape->fetchByName(*pq, _sEnumVals)) {
        EXCEPTION(EDATA, 0, _sEnumVals);
    }
    pShape->fetchFields(*pq);
    pSinge = new cRecordDialog(*pShape, 0, true, NULL, NULL, this);  // Az egy rekordot szerkesztő dialógus
    pUi->tabWidget->insertTab(0, pSinge->pWidget(), trUtf8("Egy enumerációs érték"));
    // Kellenek az enumerációs típusok listája
    QString sql = "SELECT typname FROM pg_catalog.pg_type WHERE typcategory = 'E' ORDER BY typname ASC";
    QStringList typeList;
    if (execSql(*pq, sql)) do {
        typeList << pq->value(0).toString();
    } while (pq->next());
    pUi->comboBox->addItems(typeList);
    connect(pUi->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(setEnumType(QString)));
    // ...

}

cEnumValsDialog::~cEnumValsDialog()
{
    delete pq;
    delete pShape;
}

void cEnumValsDialog::clicked(QAbstractButton *pButton)
{
    int sb = pUi->buttonBox->standardButton(pButton);
    switch (sb) {
    case QDialogButtonBox::Ok:
        // ...
        break;
    case QDialogButtonBox::Save:
        // ...
        break;
    case QDialogButtonBox::Cancel:
        reject();
        break;
    }
}

void cEnumValsDialog::setEnumType(const QString& etn)
{
    enumTypeName = etn;
    pDelete(pEnumType);
    pEnumType = cColEnumType::fetchOrGet(*pq, enumTypeName);

}
