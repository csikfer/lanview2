#include "translator.h"
#include "ui_translator.h"
#include "popupreport.h"
#include "record_dialog.h"

#define MAX_LANGUAGES   6
#define MIN_LANGUAGES   2

cTransLang::cTransLang(int ix, cTranslator *par)
    : QObject(par)
{
    parent    = par;
    langIndex = ix;
    enabled   = true;
    if (parent->selectLangs.size() != langIndex) EXCEPTION(EPROGFAIL);
    pModel    = new cRecordListModel(_language.descr());
    pModel->_setOrder(OT_ASC, _sLanguageName);
    _langId   = NULL_ID;
    switch (langIndex) {
    case 0: // #1 (static)
        pComboBox = parent->ui->comboBoxLang1;
        pLabel    = parent->ui->labelLang1;
        break;
    case 1: // #2 (static)
        pComboBox = parent->ui->comboBoxLang2;
        pLabel    = parent->ui->labelLang2;
        pModel->nullable = true;
        break;
    default:// #2+ (dynamic)
        pComboBox = new QComboBox;
        pLabel    = new QLabel;
        parent->ui->horizontalLayoutComboBoxs->addWidget(pLabel);
        parent->ui->horizontalLayoutComboBoxs->addWidget(pComboBox);
        pModel->nullable = true;
        break;
    }
    int langNum = langIndex + 1;
    pLabel->setText(trUtf8("Nyelv #%1").arg(langNum));
    parent->ui->toolButtonAddComboBox->setEnabled(langNum < MAX_LANGUAGES);
    pModel->joinWith(pComboBox);
    pModel->setFilter();
    int currentIx = 0;
    if (langIndex == 0) {
        cLanguage *pLang = lanView::getInstance()->pLanguage;
        if (pLang == nullptr) EXCEPTION(EPROGFAIL);
        _language.clone(*pLang);
        _langId = current().getId();
        currentIx = pModel->indexOf(_langId);
        if (currentIx < 0) EXCEPTION(EDATA);
    }
    pComboBox->setCurrentIndex(currentIx);
    pLineEdit = nullptr;
    connect(pComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_comboBox_currentIndexChanged(int)));
}

cTransLang::~cTransLang()
{
    if (parent->down) return;
    if (langIndex < MIN_LANGUAGES || parent->selectLangs.size() != langIndex) EXCEPTION(EPROGFAIL, langIndex);
    delete pLabel;
    delete pComboBox;
    if (pLineEdit != nullptr) delete pLineEdit;
}

void cTransLang::setEnable(bool f)
{
    if (enabled == f) return;
    enabled = f;
    QHBoxLayout *pLayout = parent->ui->horizontalLayoutComboBoxs;
    QLayoutItem *p;
    if (enabled) {
        p = pLayout->replaceWidget(pLineEdit, pComboBox, Qt::FindDirectChildrenOnly);
        pDelete(pLineEdit);
        pComboBox->show();
    }
    else {
        pLineEdit = new QLineEdit;
        pLineEdit->setText(pComboBox->currentText());
        p = pLayout->replaceWidget(pComboBox, pLineEdit, Qt::FindDirectChildrenOnly);
        pComboBox->hide();
    }
    if (p == nullptr) EXCEPTION(EPROGFAIL);
}


void cTransLang::on_comboBox_currentIndexChanged(int index)
{
    _langId = pModel->atId(index);
    if (_langId == NULL_ID) {
        _language.clear();
    }
    else {
        _language.setById(parent->q, _langId);
    }
}


enum {
    CIX_REC_ID, CIX_REC_NAME, CIX_TID, CIX_TID_NAME, CIX_FIRST_LANG
};

cTransRow::cTransRow(cTranslator *par, qlonglong _rid, const QString& _rnm, qlonglong _tid, const QList<QStringList>& _texts)
    : QObject(par)
{
    parent      = par;
    recId       = _rid;
    recName     = _rnm;
    textId      = _tid;
    texts       = _texts;
    langNum     = texts.size();
    height      = texts.first().size();
    firstRow    = parent->rows.size() * height;
    pTable      = parent->ui->tableWidget;
    pTextEnum   = parent->pTextNameEnumType;

    pTable->setRowCount(firstRow + height);
    pTable->setItem(firstRow, CIX_REC_ID,      numberItem(recId));
    pTable->setItem(firstRow, CIX_REC_NAME,    textItem(recName));
    pTable->setItem(firstRow, CIX_TID,         numberItem(textId));
    subRows(0);
    if (height > 1) {
        for (int i = 1; i < height; ++i) {
            subRows(i);
        }
        pTable->setSpan(firstRow, CIX_REC_ID, height, 1);
        pTable->setSpan(firstRow, CIX_REC_NAME, height, 1);
        pTable->setSpan(firstRow, CIX_TID, height, 1);
    }
}

cTransRow::~cTransRow()
{
    ;
}

void cTransRow::save()
{
    QSqlQuery &q = parent->q;
    for (int i = 0; i < langNum; ++i) {
        QStringList texts;
        int col = CIX_FIRST_LANG;
        for (int h = 0; h < height; ++h) {
            texts << getText(firstRow + i, col);
        }
        cLangTexts::saveText(q, parent->sTableName, parent->pTextNameEnumType, textId, parent->languages.at(i)->getId(), texts);
    }
}

QTableWidgetItem *cTransRow::numberItem(qlonglong n)
{
    QString text;
    if (n >= 0) text = QString::number(n);
    QTableWidgetItem *pi = new QTableWidgetItem(text);
    pi->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFlags<Qt::ItemFlag> itf = pi->flags();
    itf.setFlag(Qt::ItemIsEditable, false);
    pi->setFlags(itf);
    return pi;
}

QTableWidgetItem *cTransRow::textItem(const QString& text)
{
    QTableWidgetItem *pi = new QTableWidgetItem(text);
    QFlags<Qt::ItemFlag> itf = pi->flags();
    itf.setFlag(Qt::ItemIsEditable, false);
    pi->setFlags(itf);
    return pi;
}

QTableWidgetItem *cTransRow::editItem(const QString& text)
{
    QTableWidgetItem *pi = new QTableWidgetItem(text);
    QFlags<Qt::ItemFlag> itf = pi->flags();
    itf.setFlag(Qt::ItemIsEditable, true);
    itf.setFlag(Qt::ItemIsDragEnabled, true);
    itf.setFlag(Qt::ItemIsDropEnabled, true);
    pi->setFlags(itf);
    return pi;
}

QString cTransRow::getText(int row, int col)
{
    QTableWidgetItem *pi = pTable->item(row, col);
    if (pi == nullptr) EXCEPTION(EPROGFAIL);
    return pi->text();
}

void cTransRow::subRows(int i)
{
    pTable->setItem(firstRow +i, CIX_TID_NAME, textItem(pTextEnum->enumValues.at(i)));
    for (int j = 0; j < langNum; ++j) {
        pTable->setItem(firstRow +i, CIX_FIRST_LANG + j, editItem(texts.at(j).at(i)));
    }
}

const enum ePrivilegeLevel cTranslator::rights = PL_ADMIN;

cTranslator::cTranslator(QMdiArea *par)
    : cIntSubObj(par)
    , ui(new Ui::cTranslator)
    , enumTableForText(*cColEnumType::fetchOrGet(q, toTypeName(_sTableForText)))
    , q(getQuery())
{
    int ix;
    init   = true;
    down   = false;
    viewed = false;
    pTextNameEnumType = nullptr;

    ui->setupUi(this);
    selectLangs << new cTransLang(0, this);
    selectLangs << new cTransLang(1, this);
    pTableListModel = new cEnumListModel(&enumTableForText);
    pTableListModel->joinWith(ui->comboBoxObj);
    ix = 0;
    ui->comboBoxObj->setCurrentIndex(ix);
    pTextNameEnumType = index2objEnum(ix);

    init = false;

    on_comboBoxObj_currentIndexChanged(ix);
}

cTranslator::~cTranslator()
{
    down = true;
    delete ui;
}


const cRecStaticDescr * cTranslator::index2TableDescr(int ix)
{
    if (ix < 0) ix = ui->comboBoxObj->currentIndex();
    if (ix < 0) return nullptr;
    QString tn = pTableListModel->atEnumVal(ix)->getName();
    const cRecStaticDescr *pDescr = cRecStaticDescr::get(tn);
    return pDescr;
}

const cColEnumType *cTranslator::table2objEnum(const cRecStaticDescr * _pd)
{
    if (_pd == nullptr) return nullptr;
    const cColEnumType *pTextNameEnum = _pd->colDescr(_pd->textIdIndex()).pEnumType;
    if (pTextNameEnum == nullptr) EXCEPTION(EDATA);
    return pTextNameEnum;
}

const cColEnumType *cTranslator::index2objEnum(int ix)
{
    const cRecStaticDescr *pDescr = index2TableDescr(ix);
    return table2objEnum(pDescr);
}

void cTranslator::clearRows()
{
    foreach (cTransRow * pRow, rows) {
        delete pRow;
    };
    rows.clear();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(0);
    langNum = 0;
    langIds.clear();
    languages.clear();
}

void cTranslator::enableCombos(bool e)
{
    ui->comboBoxObj->setEnabled(e);
    foreach (cTransLang *p, selectLangs) {
        p->setEnable(e);
    }
    ui->pushButtonView->setEnabled(e);
    ui->pushButtonImport->setEnabled(e);

    int LangNum = selectLangs.size();
    ui->toolButtonAddComboBox->setEnabled(e && LangNum < MAX_LANGUAGES);
    ui->toolButtonDelComboBox->setEnabled(e && LangNum > MAX_LANGUAGES);

    ui->pushButtonCancel->setDisabled(e);
    ui->pushButtonSave->setDisabled(e);
    ui->pushButtonExport->setDisabled(e);
}


void cTranslator::on_comboBoxObj_currentIndexChanged(int index)
{
    if (init || !ui->comboBoxObj->isEnabled()) return;
    ui->lineEditObj->setText(cEnumVal::viewLong(enumTableForText, index, _sNul));
    pTableDescr  = index2TableDescr();
    sTableName   = pTableDescr->tableName();
    pTextNameEnumType = table2objEnum(pTableDescr);
}

void cTranslator::on_pushButtonView_clicked()
{
    QString idName   = pTableDescr->idName(EX_IGNORE);
    QString nameName = pTableDescr->nameName(EX_IGNORE);
    // Exceptional cases
    if (sTableName == _sAlarmMessages) {
        idName   = "-1";    // No ID
        nameName = "(SELECT service_type_name FROM service_types WHERE o.service_type_id = service_type_id) || '.' || o.status";
    }
    else if (sTableName == _sEnumVals) {
        if (idName.isEmpty()) EXCEPTION(EPROGFAIL);
        nameName = "o.enum_type_name || '.' || o.enum_vnal_name";
        idName.prepend("o.");
    }
    else {
        if (idName.isEmpty() || nameName.isEmpty()) EXCEPTION(EPROGFAIL);
        nameName.prepend("o.");
        idName.prepend("o.");
    }
    clearRows();
    foreach (cTransLang *p, selectLangs) {
        if (p->currentId() != NULL_ID) {
            ++langNum;
            langIds   <<  p->currentId();
            languages << &p->current();
        }
    }
    QString selectText;
    for (int i = 0; i < langNum; ++i) {
        selectText += QString(" (SELECT texts FROM l WHERE text_id = o.text_id AND language_id = %1) AS tx%2,")
                .arg(langIds.at(i)).arg(i);
    }
    selectText.chop(1); // Dropp last column
    QString sql =
        QString( "WITH l AS (SELECT * FROM localizations WHERE table_for_text = '%1')").arg(sTableName)
      + QString( "SELECT %1 AS id, %2 AS name, text_id,"                              ).arg(idName, nameName)
      + selectText
      + QString( " FROM %1 AS o"                                                      ).arg(sTableName);
    if (!execSql(q, sql)) {
        const QString msg = QObject::trUtf8("A %1 táblában nincs egy szerkeszthető sor sem, nincs mit fordítani.").arg(sTableName);
        cMsgBox::warning(msg, this);
        return;
    }
    int cols = CIX_FIRST_LANG + langNum;
    setHeader(cols);
    int size = pTextNameEnumType->enumValues.size();
    do {
        qlonglong recId   = q.value(0).toLongLong();
        QString   recName = q.value(1).toString();
        qlonglong textId  = q.value(2).toLongLong();
        QList<QStringList> texts;
        for (int i = 0; i < langNum; ++i) {
            QString s = q.value(3 + i).toString();
            QStringList txt;
            if (!s.isEmpty()) {
                txt = sqlToStringList(s);
                txt = txt.mid(0, size);
            }
            while (txt.size() < size) txt << _sNul;
            texts << txt;

        }
        rows << new cTransRow(this, recId, recName, textId, texts);
    } while (q.next());
    ui->tableWidget->resizeColumnsToContents();
    ui->tableWidget->resizeRowsToContents();
    enableCombos(false);
}

void cTranslator::on_pushButtonSave_clicked()
{
    foreach (cTransRow *p, rows) {
        p->save();
    }
}

void cTranslator::on_toolButtonAddComboBox_clicked()
{
    if (MAX_LANGUAGES <= selectLangs.size()) EXCEPTION(EPROGFAIL, selectLangs.size());
    selectLangs << new cTransLang(selectLangs.size(), this);
    ui->toolButtonDelComboBox->setEnabled(true);
    ui->toolButtonAddComboBox->setEnabled(selectLangs.size() < MAX_LANGUAGES);
}

void cTranslator::on_toolButtonAddLanguage_clicked()
{
    cRecord *p = recordDialog(q, _sLanguages, this);
    if (p == nullptr) return;
    qlonglong id = p->getId();
    delete p;
    foreach (cTransLang *p, selectLangs) {
        p->refresh();
        if (p->currentId() == NULL_ID || id != NULL_ID) {
            p->setCurrent(id);
            id = NULL_ID;
        }
    }
}

void cTranslator::on_toolButtonDelComboBox_clicked()
{
    if (MIN_LANGUAGES >= selectLangs.size()) EXCEPTION(EPROGFAIL, selectLangs.size());
    delete selectLangs.takeLast();
    ui->toolButtonAddComboBox->setEnabled(true);
    ui->toolButtonDelComboBox->setEnabled(selectLangs.size() > MIN_LANGUAGES);
}

void cTranslator::on_pushButtonCancel_clicked()
{
    QString text = trUtf8("Valóban eldobja a táblázatot, és a modosításokat ?");
    int r = QMessageBox::warning(this, dcViewLong(DC_WARNING), text, QMessageBox::No, QMessageBox::Yes);
    if (r == QMessageBox::Yes) {
        clearRows();
        enableCombos(true);
    }
}

void cTranslator::on_pushButtonExport_clicked()
{
    QString fileName;
    QFile *pFile = cFileDialog::trgFile(fileName, cFileDialog::sCsvFileFilter, this);
    if (pFile == nullptr) return;
    cCommaSeparatedValues csv(pFile);

    static const QString sUp = "^";
    QTableWidget *pTable = ui->tableWidget;
    int cols   = pTable->columnCount();
    int rows   = pTable->rowCount();
    int height = pTable->rowSpan(0, CIX_REC_ID);
    if (pTextNameEnumType == nullptr
     || height != pTextNameEnumType->enumValues.size()
     || cols != (CIX_FIRST_LANG + langNum)) EXCEPTION(EPROGFAIL);

    // Header: CIX_REC_ID, CIX_REC_NAME, CIX_TID,   CIX_TID_NAME
    csv <<    sTableName << _sName   << _sTextId << "Text Name";
    for (int i = 0; i < langNum; ++i) csv << languages.at(i)->getName();
    csv << endl;
    for (int row = 0; row < rows; row += height) {
        for (int h = 0; h < height; ++h) {
            if (row + h >= rows) EXCEPTION(EPROGFAIL);
            csv << (h == 0 ? itemText(row, CIX_REC_ID, EX_IGNORE) : sUp);
            csv << itemText(row, CIX_REC_NAME);
            csv << itemId(row, CIX_TID);
            csv << itemText(row + h, CIX_TID_NAME);
            for (int i = 0; i < langNum; ++i) csv << itemText(row + h, CIX_FIRST_LANG + i);
            csv << endl;
        }
    }
    pFile->close();
    delete pFile;
}


QString cTranslator::itemText(int row, int col, eEx __ex) const
{
    QTableWidgetItem *pi = ui->tableWidget->item(row, col);
    if (pi == nullptr) {
        if (__ex == EX_IGNORE) return _sNul;
        EXCEPTION(EPROGFAIL);
    }
    return pi->text();
}

qlonglong cTranslator::itemId(int row, int col) const
{
    QString s = itemText(row, col);
    bool ok;
    qlonglong r = s.toLongLong(&ok);
    if (!ok) EXCEPTION(EPROGFAIL);
    return r;
}

void cTranslator::on_pushButtonImport_clicked()
{
    clearRows();
    QString fileName;
    QFile *pFile = cFileDialog::srcFile(fileName, cFileDialog::sCsvFileFilter, this);
    if (pFile == nullptr) return;
    cCommaSeparatedValues csv(pFile);
    QString msg, s;
    int height  = 0;
    int langNum = 0;
    int row, col, h, i;
    qlonglong recId, textId;
    QString recName;
    QList<QStringList> texts;
    if (first(csv).state(msg) & CSVE_CRIT) goto import_error;   // Start CSV read
    // read header
    // CIX_REC_ID (tableName)
    row = col = 0;
    csv >> sTableName;
    if (csv.state(msg) != CSVE_OK) goto import_error;
    if (!enumTableForText.enumValues.contains(sTableName)) {
        msg = trUtf8("Invalid table name in header : %1").arg(sTableName);
        goto import_error;
    }
    ui->comboBoxObj->setCurrentIndex(pTableListModel->indexOf(sTableName));
    pTableDescr = cRecStaticDescr::get(sTableName);
    pTextNameEnumType = table2objEnum(pTableDescr);
    height = pTextNameEnumType->enumValues.size();
    // CIX_REC_NAME, CIX_TID, CIX_TID_NAME ignore
    csv >> s >> s >> s;
    col = CIX_TID_NAME;
    // CIX_FIRST_LANG ...
    while (true) {
        ++col;
        csv >> s;
        if (csv.state() == CSVE_END_OF_LINE) {
            if (langNum == 0) {
                msg = trUtf8("A fejlécben nincs egyetlen nyelv sem.");
                goto import_error;
            }
            break;
        }
        if (csv.state(msg) != CSVE_OK) goto import_error;
        langNum++;
        if (langNum > MAX_LANGUAGES) {
            msg = trUtf8("Tul sok nyelv a fejlécben.");
            goto import_error;
        }
        if (selectLangs.size() < langNum) on_toolButtonAddComboBox_clicked();
        if (!selectLangs.at(langNum -1)->setCurrent(s)) {
            msg = trUtf8("Ismeretlen nyelv a fejlécben : %1, #%2").arg(s).arg(langNum);
            goto import_error;
        }
    }
    while (selectLangs.size() > std::max(langNum, 2)) on_toolButtonDelComboBox_clicked();
    foreach (cTransLang *p, selectLangs) {
        languages << &p->current();
    }
    setHeader(CIX_FIRST_LANG + langNum);
    // read data
    row = 1; col = 0;
    while (true) {
        for (h = 0; h < height; ++h) {
            ++row; col = 0;
            if (next(csv).state() == CSVE_EDD_OF_FILE && h == 0) {
                // EXIT, OK
                delete pFile;
                ui->tableWidget->resizeColumnsToContents();
                ui->tableWidget->resizeRowsToContents();
                enableCombos(false);
                return;
            }
            if (csv.state(msg) & CSVE_CRIT) goto import_error;
            if (h == 0) {
                texts.clear();
                csv >> recId;
                ++col;
                if (csv.state(msg) != CSVE_OK && csv.state() != CSVE_IS_NOT_NUMBER) goto import_error;
                csv >> recName;
                ++col;
                if (csv.state(msg) != CSVE_OK) goto import_error;
                csv >> textId;
                ++col;
                if (csv.state(msg) != CSVE_OK) goto import_error;
            }
            else {
                csv >> s >> s >> s;
                col += 3;
                if (csv.state(msg) != CSVE_OK) goto import_error;
            }
            csv >> s;
            ++col;
            if (csv.state(msg) != CSVE_OK) goto import_error;
            if (s != pTextNameEnumType->enumValues.at(h)) {
                msg = trUtf8("Hibás szöveg azonosító : %1").arg(s);
                goto import_error;
            }
            for (i = 0; i < langNum; i++) {
                if (h == 0) texts << QStringList();
                csv >> s;
                ++col;
                if (csv.state(msg) != CSVE_OK) goto import_error;
                texts[i] << s;
            }
        }
        cTransRow *p = new cTransRow(this, recId, recName, textId, texts);
        rows << p;
    }
import_error:
    delete pFile;
    msg += QString("\n Table : row : %1; col : %2").arg(row).arg(col);
    cMsgBox::warning(msg, this);
    return;
}

void cTranslator::setHeader(int cols)
{
    ui->tableWidget->setColumnCount(cols);
    ui->tableWidget->setHorizontalHeaderItem(CIX_REC_ID,     new QTableWidgetItem(trUtf8("ID")));
    ui->tableWidget->setHorizontalHeaderItem(CIX_REC_NAME,   new QTableWidgetItem(trUtf8("Név")));
    ui->tableWidget->setHorizontalHeaderItem(CIX_TID,        new QTableWidgetItem(trUtf8("T.ID")));
    ui->tableWidget->setHorizontalHeaderItem(CIX_TID_NAME,   new QTableWidgetItem(trUtf8("T.név")));
    for (int i = CIX_FIRST_LANG; i < cols; ++i) {
        int ix = i - CIX_FIRST_LANG;
        QString s = languages.at(ix)->getName();
        ui->tableWidget->setHorizontalHeaderItem(i, new QTableWidgetItem(s));
    }
}


