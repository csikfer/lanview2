#include "exports.h"
#include "cerrormessagebox.h"
#include <QRegExp>

const enum ePrivilegeLevel cExportsWidget::rights = PL_OPERATOR;

cExportsWidget::cExportsWidget(QMdiArea *par)
    : cIntSubObj(par)
    , pUi(new Ui::Exports)
{
//    pThread = NULL;
    isStop  = false;
    QSqlQuery q = getQuery();
    QStringList ol = cSysParam::getTextSysParam(q, "export_list").split(",");
    pUi->setupUi(this);
    pUi->comboBoxTable->addItems(ol);
    bool empty = ol.isEmpty();
    pUi->pushButtonSave->setDisabled(empty);
    if (!empty) changedName(ol.first());
    connect(pUi->pushButtonExport, SIGNAL(clicked()),     this, SLOT(start()));
    connect(pUi->pushButtonSave,   SIGNAL(clicked()),     this, SLOT(save()));
    connect(pUi->textEdit,         SIGNAL(textChanged()), this, SLOT(changedText()));
    connect(pUi->comboBoxTable,    SIGNAL(currentTextChanged(QString)), this, SLOT(changedName(QString)));
}

cExportsWidget::~cExportsWidget()
{

}

void cExportsWidget::disable(bool f)
{
    pUi->pushButtonClear->setDisabled(f);
    pUi->pushButtonExport->setDisabled(f);
    pUi->pushButtonSave->setDisabled(f);
    pUi->textEdit->setReadOnly(f);
    pUi->pushButtonStop->setEnabled(f);
}

void cExportsWidget::start()
{
}

void cExportsWidget::stop()
{
}


void cExportsWidget::save()
{
    textToFile(fileName, pUi->textEdit->toPlainText(), this);
}

void cExportsWidget::changedText()
{
    pUi->pushButtonSave->setDisabled(pUi->textEdit->toPlainText().isEmpty());
}

void cExportsWidget::changedName(const QString& tn)
{
    cTableShape ts;
    ts.setByName(tn);
    pUi->lineEditTableTitle->setText(ts.getText(cTableShape::LTX_TABLE_TITLE));
}

void cExportsWidget::text(const QString& _s)
{
}

void cExportsWidget::ready()
{
}
