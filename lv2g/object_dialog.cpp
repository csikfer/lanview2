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


cEnumValRow::cEnumValRow(QSqlQuery& q, const QString& _val, int _row, cEnumValsEditWidget *par)
    :QObject(par), row(_row)
{
    parent = par;
    pTableWidget = par->pUi->tableWidgetType;
    rec.setName(_sEnumTypeName, parent->enumTypeTypeName);
    rec.setName(_sEnumValName, _val);
    int r = rec.completion(q);
    switch (r) {
    case 1:     // Beolvasva
        break;
    case 0:     // Még nincs az adatbázisban  Set default
        rec.setName(_sEnumTypeName, parent->enumTypeTypeName);
        rec.setName(_sEnumValName, _val);
        rec.setName(_sViewShort,   _val);
        rec.setName(_sViewLong,    _val);
        break;
    default:
        EXCEPTION(AMBIGUOUS, r, mCat(parent->enumTypeTypeName, _val));
    }
    QTableWidgetItem *p = setTableItemText(_val, pTableWidget, row, CEV_NAME);
    p->setFlags(p->flags() & ~Qt::ItemIsEditable);
    setTableItemText(rec.getName(_sViewShort), pTableWidget, row, CEV_SHORT);
    setTableItemText(rec.getName(_sViewLong),  pTableWidget, row, CEV_LONG);
    pBgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sBgColor), rec[_sBgColor], false, NULL);
    pTableWidget->setCellWidget(row, CEV_BG_COLOR, pBgColorWidget->pWidget());
    pFgColorWidget = new cColorWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFgColor), rec[_sFgColor], false, NULL);
    pTableWidget->setCellWidget(row, CEV_FG_COLOR, pFgColorWidget->pWidget());
    pFntFamWidget  = new cFontFamilyWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFontFamily), rec[_sFontFamily], NULL);
    pTableWidget->setCellWidget(row, CEV_FNT_FAM, pFntFamWidget->pWidget());
    pFntAttWidget  = new cFontAttrWidget(*parent->pShape, *parent->pShape->shapeFields.get(_sFontAttr), rec[_sFontAttr], false, NULL);
    pTableWidget->setCellWidget(row, CEV_FNT_ATT, pFntAttWidget->pWidget());
    setTableItemText(rec.getName(_sEnumValNote),  pTableWidget, row, CEV_NOTE);
    setTableItemText(rec.getName(_sToolTip),  pTableWidget, row, CEV_TOOL_TIP);
}

void cEnumValRow::save(QSqlQuery& q)
{
    rec.clearId();
    rec[_sViewShort]   = getTableItemText(pTableWidget, row, CEV_SHORT);
    rec[_sViewLong]    = getTableItemText(pTableWidget, row, CEV_LONG);
    rec[_sBgColor]     = pBgColorWidget->get();
    rec[_sFgColor]     = pFgColorWidget->get();
    rec[_sFontFamily]  = pFntFamWidget->get();
    rec[_sFontAttr]    = pFntAttWidget->get();
    rec[_sEnumValNote] = getTableItemText(pTableWidget, row, CEV_NOTE);
    rec[_sToolTip]     = getTableItemText(pTableWidget, row, CEV_TOOL_TIP);
    rec.replace(q);
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
    pShape = new cTableShape;
    if (!pShape->fetchByName(*pq, _sEnumVals)) {
        EXCEPTION(EDATA, 0, _sEnumVals);
    }
    pShape->fetchFields(*pq);
    // Kellenek az enumerációs típusok listája
    QString sql = "SELECT typname FROM pg_catalog.pg_type WHERE typcategory = 'E' ORDER BY typname ASC";
    QStringList typeList;
    if (execSql(*pq, sql)) do {
        typeList << pq->value(0).toString();
    } while (pq->next());
    // A bool mezőket tartalmazó táblák listáka
    QStringList tableList;
    sql = "SELECT DISTINCT(table_name) FROM information_schema.columns WHERE table_schema = 'public'"
          " AND (data_type = 'boolean' OR (data_type = 'ARRAY' AND udt_name = 'boolean'))"
            " ORDER BY table_name ASC";
    if (execSql(*pq, sql)) do {
        tableList << pq->value(0).toString();
    } while (pq->next());


    pShape = new cTableShape;
    pShape->setParent(this);
    pShape->setByName(*pq, _sEnumVals);
    pShape->fetchFields(*pq);

    // VAL
    pWidgetValBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), val[_sBgColor], false, NULL);
    pWidgetValBgColor->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValBgColor, pWidgetValBgColor->pWidget());

    pWidgetValFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), val[_sFgColor], false, NULL);
    pWidgetValFgColor->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFgColor, pWidgetValFgColor->pWidget());

    pWidgetValFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), val[_sFontFamily], NULL);
    pWidgetValFntFam->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFontFamily, pWidgetValFntFam->pWidget());

    pWidgetValFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), val[_sFontAttr], false, NULL);
    pWidgetValFntAtt->setParent(this);
    formSetField(pUi->formLayoutVal, pUi->labelValFontAttr, pWidgetValFntAtt->pWidget());

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
    formSetField(pUi->formLayoutTrue, pUi->labelTrueBgColor, pWidgetTrueBgColor->pWidget());

    pWidgetTrueFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), boolTrue[_sFgColor], false, NULL);
    pWidgetTrueFgColor->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFgColor, pWidgetTrueFgColor->pWidget());

    pWidgetTrueFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), boolTrue[_sFontFamily], NULL);
    pWidgetTrueFntFam->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFontFam, pWidgetTrueFntFam->pWidget());

    pWidgetTrueFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), boolTrue[_sFontAttr], false, NULL);
    pWidgetTrueFntAtt->setParent(this);
    formSetField(pUi->formLayoutTrue, pUi->labelTrueFntAtt, pWidgetTrueFntAtt->pWidget());
        // false
    pWidgetFalseBgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sBgColor), boolFalse[_sBgColor], false, NULL);
    pWidgetFalseBgColor->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseBgColor, pWidgetFalseBgColor->pWidget());

    pWidgetFalseFgColor = new cColorWidget(*pShape, *pShape->shapeFields.get(_sFgColor), boolFalse[_sFgColor], false, NULL);
    pWidgetFalseFgColor->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFgColor, pWidgetFalseFgColor->pWidget());

    pWidgetFalseFntFam  = new cFontFamilyWidget(*pShape, *pShape->shapeFields.get(_sFontFamily), boolFalse[_sFontFamily], NULL);
    pWidgetFalseFntFam->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFntFam, pWidgetFalseFntFam->pWidget());

    pWidgetFalseFntAtt  = new cFontAttrWidget(*pShape, *pShape->shapeFields.get(_sFontAttr), boolFalse[_sFontAttr], false, NULL);
    pWidgetFalseFntAtt->setParent(this);
    formSetField(pUi->formLayoutFalse, pUi->labelFalseFntAtt, pWidgetFalseFntAtt->pWidget());

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
    val[_sViewShort]    = pUi->lineEditValShort->text();
    val[_sViewLong]     = pUi->lineEditValLong->text();
    val[_sToolTip]      = pUi->textEditValValTolTip->toPlainText();
    val[_sFontFamily]   = pWidgetValFntFam->get();
    val[_sFontAttr]     = pWidgetValFntAtt->get();

    return cErrorMessageBox::condMsgBox(val.tryReplace(*pq, true));
}

bool cEnumValsEditWidget::saveType()
{
    type.clearId();
    val[_sEnumValNote]  = pUi->lineEditTypeTypeNote->text();
    val[_sViewShort]    = pUi->lineEditTypeShort->text();
    val[_sViewLong]     = pUi->lineEditTypeLong->text();
    val[_sToolTip]      = pUi->textEditTypeTypeToolTip->toPlainText();

    cError *pe = NULL;
    static const QString tn = "cEnumValsEdit";
    sqlBegin(*pq, tn);
    try {
        val.replace(*pq);
        foreach (cEnumValRow *p, rows) {
            p->save(*pq);
        }
        sqlEnd(*pq, tn);
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
    boolType[_sViewShort]    = pUi->lineEditBoolTypeShort->text();
    boolType[_sViewLong]     = pUi->lineEditBoolTypeLong->text();
    boolType[_sToolTip]      = pUi->textEditBoolToolTip->toPlainText();

    boolTrue[_sEnumValName]  = _sTrue;
    boolTrue[_sEnumValNote]  = pUi->lineEditTrueNote->text();
    boolTrue[_sEnumTypeName] = type;
    boolTrue[_sBgColor]      = pWidgetTrueBgColor->get();
    boolTrue[_sFgColor]      = pWidgetTrueFgColor->get();
    boolTrue[_sViewShort]    = pUi->lineEditTrueShort->text();
    boolTrue[_sViewLong]     = pUi->lineEditTrueLong->text();
    boolTrue[_sToolTip]      = pUi->textEditTrueToolTip->toPlainText();
    boolTrue[_sFontFamily]   = pWidgetTrueFntFam->get();
    boolTrue[_sFontAttr]     = pWidgetTrueFntAtt->get();

    boolFalse[_sEnumValName] = _sFalse;
    boolFalse[_sEnumValNote] = pUi->lineEditFalseNote->text();
    boolFalse[_sEnumTypeName]= type;
    boolFalse[_sBgColor]     = pWidgetFalseBgColor->get();
    boolFalse[_sFgColor]     = pWidgetFalseFgColor->get();
    boolFalse[_sViewShort]   = pUi->lineEditFalseShort->text();
    boolFalse[_sViewLong]    = pUi->lineEditFalseLong->text();
    boolFalse[_sToolTip]     = pUi->textEditFalseToolTip->toPlainText();
    boolFalse[_sFontFamily]  = pWidgetFalseFntFam->get();
    boolFalse[_sFontAttr]    = pWidgetFalseFntAtt->get();

    cError *pe = NULL;
    static const QString tn = "cEnumValsEdit";
    sqlBegin(*pq, tn);
    try {
        boolType.replace(*pq);
        boolTrue.replace(*pq);
        boolFalse.replace(*pq);
        sqlEnd(*pq, tn);
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
    switch (n) {
    case 1:     // Beolvasva
        break;
    case 0:     // Not found (az enumVal-t törölte a completion() metódus)
        val[_sEnumValName]  = ev;
        val[_sEnumTypeName] = enumValTypeName;
        val[_sViewShort]    = ev;
        val[_sViewLong]     = ev;
        break;
    default:    EXCEPTION(AMBIGUOUS, n, val.identifying());
    }
    pWidgetValBgColor->set(val[_sBgColor]);
    pWidgetValFgColor->set(val[_sFgColor]);
    pUi->lineEditValShort->setText(val[_sViewShort].toString());
    pUi->lineEditValLong->setText(val[_sViewLong].toString());
    pUi->lineEditValValNote->setText(val[_sEnumValNote].toString());
    pUi->textEditValValTolTip->setText(val[_sToolTip].toString());
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
    switch (n) {
    case 1:     // Beolvasva
        break;
    case 0:     // Not found (törölte a completion() metódus)
        type[_sEnumValName]  = _sNul;
        type[_sEnumTypeName] = enumValTypeName;
        type[_sViewShort]    = enumValTypeName;
        type[_sViewLong]     = enumValTypeName;
        break;
    default:    EXCEPTION(AMBIGUOUS, n, type.identifying());
    }
    pUi->lineEditTypeShort->setText(type[_sViewShort].toString());
    pUi->lineEditTypeLong->setText(type[_sViewLong].toString());
    pUi->lineEditTypeTypeNote->setText(type[_sViewLong].toString());
    pUi->textEditTypeTypeToolTip->setText(type[_sToolTip].toString());

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
    int n = boolType.completion(*pq);
    switch (n) {
    case 1:     // beolvasva
        break;
    case 0:     // not found
        boolType.setName(_sEnumTypeName, typeName);
        boolType.setName(_sEnumValName,  _sNul);
    }
    pUi->lineEditBoolTypeShort->setText(boolType[_sViewShort].toString());
    pUi->lineEditBoolTypeLong->setText(boolType[_sViewLong].toString());
    pUi->lineEditBoolTypeNote->setText(boolType[_sViewLong].toString());
    pUi->textEditBoolToolTip->setText(boolType[_sToolTip].toString());

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
        boolTrue.setName(_sViewShort, langBool(true));
        boolTrue.setName(_sViewLong, langBool(true));
    }
    pUi->lineEditTrueShort->setText(boolTrue[_sViewShort].toString());
    pUi->lineEditTrueLong->setText(boolTrue[_sViewLong].toString());
    pUi->lineEditTrueNote->setText(boolTrue[_sViewLong].toString());
    pUi->textEditBoolToolTip->setText(type[_sToolTip].toString());
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
        boolFalse.setName(_sViewShort, langBool(false));
        boolFalse.setName(_sViewLong, langBool(false));
    }
    pUi->lineEditFalseShort->setText(boolFalse[_sViewShort].toString());
    pUi->lineEditFalseLong->setText(boolFalse[_sViewLong].toString());
    pUi->lineEditFalseNote->setText(boolFalse[_sViewLong].toString());
    pUi->textEditBoolToolTip->setText(boolFalse[_sToolTip].toString());
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
