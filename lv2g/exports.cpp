#include "exports.h"
#include "lv2syntax.h"
#include "cerrormessagebox.h"

const enum ePrivilegeLevel cExportsWidget::rights = PL_OPERATOR;

cExportsWidget::cExportsWidget(QMdiArea *par)
    : cIntSubObj(par)
    , pUi(new Ui::Exports)
{
    QSqlQuery q = getQuery();
    QStringList ol = cSysParam::getTextSysParam(q, "export_list").split(",");
    pUi->setupUi(this);
    pUi->comboBoxTable->addItems(ol);
    pUi->pushButtonSave->setDisabled(ol.isEmpty());
    connect(pUi->pushButtonExport, SIGNAL(clicked()),     this, SLOT(_export()));
    connect(pUi->pushButtonSave,   SIGNAL(clicked()),     this, SLOT(save()));
    connect(pUi->plainTextEdit,    SIGNAL(textChanged()), this, SLOT(changed()));
}

cExportsWidget::~cExportsWidget()
{

}

void cExportsWidget::_export()
{
    QString tn = pUi->comboBoxTable->currentText();
    QString text = pUi->plainTextEdit->toPlainText();
    cError *pe = exportRecords(tn, text);
    if (pe == NULL) pUi->plainTextEdit->setPlainText(text);
    else {
        cErrorMessageBox::messageBox(pe, this);
        delete pe;
    }
}

void cExportsWidget::save()
{
    textToFile(fileName, pUi->plainTextEdit->toPlainText(), this);
}

void cExportsWidget::changed()
{
    pUi->pushButtonSave->setDisabled(pUi->plainTextEdit->toPlainText().isEmpty());
}
