#include "translator.h"
#include "ui_translator.h"



cTransRow::cTransRow(cTranslator *par, qlonglong _rid, const QString& _rnm, qlonglong _tid, const QStringList& _sl, const QStringList& _el)
    : QObject(par)
{
    parent      = par;
    recId       = _rid;
    recName     = _rnm;
    textId      = _tid;
    sampleTexts = _sl;
    editTexts   = _el;
    height      = _el.size();
    firstRow    = parent->rows.size() * height;
    pModel      = parent->pTableModel;
    pTable      = parent->ui->tableView;
    pTextEnum   = parent->pTextNameEnumType;

    pModel->setItem(firstRow, CIX_REC_ID,     number(recId));
    pModel->setItem(firstRow, CIX_REC_NAME,   new QStandardItem(recName));
    pModel->setItem(firstRow, CIX_TID,        number(textId));
    pModel->setItem(firstRow, CIX_TID_NAME,    new QStandardItem(pTextEnum->enumValues.first()));
    pModel->setItem(firstRow, CIX_SAMPLE_TEXT, new QStandardItem(sampleTexts.first()));
    pModel->setItem(firstRow, CIX_EDITED_TEXT, new QStandardItem(editTexts.first()));
    if (height > 1) {
        for (int i = 1; i < height; ++i) {
            pModel->setItem(firstRow +i, CIX_TID_NAME,    new QStandardItem(pTextEnum->enumValues.at(i)));
            pModel->setItem(firstRow +i, CIX_SAMPLE_TEXT, new QStandardItem(sampleTexts.at(i)));
            pModel->setItem(firstRow +i, CIX_EDITED_TEXT, new QStandardItem(editTexts.at(i)));
        }
        pTable->setSpan(firstRow, CIX_REC_ID, height, 1);
        pTable->setSpan(firstRow, CIX_REC_NAME, height, 1);
        pTable->setSpan(firstRow, CIX_TID, height, 1);
    }
}

const enum ePrivilegeLevel cTranslator::rights = PL_ADMIN;

cTranslator::cTranslator(QMdiArea *par)
    : cIntSubObj(par)
    , ui(new Ui::cTranslator)
    , q(getQuery())
    , enumTableForText(*cColEnumType::fetchOrGet(q, toTypeName(_sTableForText)))
{
    int ix;

    pTextNameEnumType = nullptr;
    viewed = modifyed = false;
    sampleLangId = editedLangId = NULL_ID;

    ui->setupUi(this);
    pTableModel = new QStandardItemModel;
    ui->tableView->setModel(pTableModel);

    pTableListModel = new cEnumListModel(&enumTableForText);
    pTableListModel->joinWith(ui->comboBoxObj);
    ix = 0;
    ui->comboBoxObj->setCurrentIndex(ix);
    pTextNameEnumType = index2objEnum(ix);

    pSampleLangModel = new cRecordListModel(sampleLanguage.descr());
    pSampleLangModel->_setOrder(OT_ASC, _sLanguageName);
    pSampleLangModel->setFilter();
    sampleLangId = lv2g::getInstance()->languageId;
    sampleLanguage.setById(q, sampleLangId);
    pSampleLangModel->joinWith(ui->comboBoxSample);
    ix = pSampleLangModel->indexOf(sampleLangId);
    ui->comboBoxSample->setCurrentIndex(ix);

    pEditedLangModel = new cRecordListModel(editedLanguage.descr());
    pEditedLangModel->_setOrder(OT_ASC, _sLanguageName);
    pEditedLangModel->setFilter(QString("%1 <> %2").arg(_sLanguageId).arg(sampleLangId), OT_DEFAULT, FT_SQL_WHERE);
    pEditedLangModel->joinWith(ui->comboBoxEdit);
    ix = 0;
    ui->comboBoxEdit->setCurrentIndex(ix);
    editedLangId = pEditedLangModel->atId(ix);
    editedLanguage.setById(q, editedLangId);

}

cTranslator::~cTranslator()
{
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
    pTableModel->clear();
}

void cTranslator::on_comboBoxSample_currentIndexChanged(int index)
{
    if (viewed) {
        qlonglong id = pSampleLangModel->atId(index);
        if (sampleLangId == id
         && editedLangId == pEditedLangModel->currendId()
         && pTextNameEnumType   == index2objEnum()) {
            ui->label->setForegroundRole(QPalette::WindowText);
        }
        else {
            ui->label->setForegroundRole(QPalette::BrightText);
        }
    }
}

void cTranslator::on_comboBoxEdit_currentIndexChanged(int index)
{
    if (viewed) {
        qlonglong id = pEditedLangModel->atId(index);
        if (editedLangId == id
         && sampleLangId == pSampleLangModel->currendId()
         && pTextNameEnumType   == index2objEnum()) {
            ui->label->setForegroundRole(QPalette::WindowText);
        }
        else {
            ui->label->setForegroundRole(QPalette::BrightText);
        }
    }
}

void cTranslator::on_comboBoxObj_currentIndexChanged(int index)
{
    if (viewed) {
        const cColEnumType *p = index2objEnum(index);
        if (pTextNameEnumType   == p
         && editedLangId == pEditedLangModel->currendId()
         && sampleLangId == pSampleLangModel->currendId()) {
            ui->label->setForegroundRole(QPalette::WindowText);
        }
        else {
            ui->label->setForegroundRole(QPalette::BrightText);
        }
    }
}

void cTranslator::on_pushButtonView_clicked()
{
    if (modifyed) {
        QString  msg = trUtf8("Menti az eddigi modosításokat ?");
        int r = QMessageBox::question(this, dcViewShort(DC_ERROR), msg,
                                      QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        switch (r) {
        case QMessageBox::Yes:  on_pushButtonSave_clicked();    break;
        case QMessageBox::No:   break;
        default:                return;
        }
        modifyed = false;
    }
    sampleLangId = pSampleLangModel->currendId();
    editedLangId = pEditedLangModel->currendId();
    pTableDescr  = index2TableDescr();
    sTableName   = pTableDescr->tableName();
    pTextNameEnumType   = table2objEnum(pTableDescr);
    pTextNameEnumType = table2objEnum(pTableDescr);
    if (sampleLangId == NULL_ID || editedLangId == NULL_ID || pTextNameEnumType == nullptr) return;
    QString idName   = pTableDescr->idName(EX_IGNORE);
    QString nameName = pTableDescr->nameName(EX_IGNORE);
    if (idName.isEmpty()) { // Exceptional cases
        if (sTableName == _sAlarmMessages) {
            idName   = "-1";    // No ID
            nameName = "(SELECT service_type_name FROM service_types WHERE o.service_type_id = service_type_id) || '.' || o.status";
        }
        else {
            EXCEPTION(EPROGFAIL);
        }
    }
    else {
        nameName.prepend("o.");
        idName.prepend("o.");
    }

    QString sql =
        QString( "WITH l AS (SELECT * FROM localizations WHERE table_for_text = '%1')"  ).arg(sTableName)
      + QString( "SELECT %1 AS id, %2 AS name, text_id,"                                ).arg(idName, nameName)
      + QString( " (SELECT texts FROM l WHERE text_id = o.text_id AND language_id =  ?) AS smpl,")  // Sample text list or NULL
      + QString( " (SELECT texts FROM l WHERE text_id = o.text_id AND language_id =  ?) AS edit ")  // Edit text list or NULL
      + QString( " FROM %1 AS o"                                                        ).arg(sTableName);
    if (!execSql(q, sql, sampleLangId, editedLangId)) {
        const QString msg = QObject::trUtf8("A %1 táblában nincs egy szerkeszthető sor sem, nincs mit fordítani.").arg(sTableName);
        QMessageBox::warning(this, dcViewShort(DC_ERROR), msg);
        return;
    }
    clearRows();
    pTableModel->setHorizontalHeaderItem(cTransRow::CIX_REC_ID,     new QStandardItem(trUtf8("ID")));
    pTableModel->setHorizontalHeaderItem(cTransRow::CIX_REC_NAME,   new QStandardItem(trUtf8("Név")));
    pTableModel->setHorizontalHeaderItem(cTransRow::CIX_TID,        new QStandardItem(trUtf8("T.ID")));
    pTableModel->setHorizontalHeaderItem(cTransRow::CIX_REC_ID,     new QStandardItem(trUtf8("T.név")));
    pTableModel->setHorizontalHeaderItem(cTransRow::CIX_SAMPLE_TEXT,new QStandardItem(sampleLanguage.getName()));
    pTableModel->setHorizontalHeaderItem(cTransRow::CIX_EDITED_TEXT,new QStandardItem(editedLanguage.getName()));
    ui->label->setText(trUtf8("A %1 tábla szövegeinek fordítása: %2 --> %3")
                       .arg(sTableName, sampleLanguage.getName(), editedLanguage.getName()));
    int size = pTextNameEnumType->enumValues.size();
    do {
        qlonglong recId   = q.value(0).toLongLong();
        QString   recName = q.value(1).toString();
        qlonglong textId  = q.value(2).toLongLong();
        QStringList sampleList;
        QString s = q.value(3).toString();
        if (!s.isEmpty()) {
            sampleList = sqlToStringList(s);
            sampleList = sampleList.mid(0, size);
        }
        while (sampleList.size() < size) sampleList << _sNul;
        QStringList editList;
        s = q.value(4).toString();
        if (!s.isEmpty()) {
            editList = sqlToStringList(s);
            editList = editList.mid(0, size);
        }
        while (editList.size() < size) editList << _sNul;
        rows << new cTransRow(this, recId, recName, textId, sampleList, editList);
    } while (q.next());
    ui->tableView->resizeColumnsToContents();
    ui->tableView->resizeRowsToContents();
}

void cTranslator::on_pushButtonSave_clicked()
{

}
