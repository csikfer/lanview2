#include "exports.h"
#include "cerrormessagebox.h"
#include <QRegExp>

const enum ePrivilegeLevel cExportsWidget::rights = PL_OPERATOR;

cExportsWidget::cExportsWidget(QMdiArea *par)
    : cIntSubObj(par)
    , pUi(new Ui::Exports)
{
    pThread = NULL;
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
    if (pThread != NULL) EXCEPTION(EPROGFAIL);
    disable(true);
    isStop = false;
    QString tn = pUi->comboBoxTable->currentText();
    pThread = new cExportThread(this);
    connect(pThread, SIGNAL(finished()),      this, SLOT(ready()));
    connect(pThread, SIGNAL(sReady(QString)), this, SLOT(text(QString)));
    pThread->start(tn);
}

void cExportsWidget::stop()
{
    if (pThread == NULL) EXCEPTION(EPROGFAIL);
    isStop = true;
    pDelete(pThread);
    disable(false);
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
    pUi->lineEditTableTitle->setText(ts.getName(_sTableTitle));
}

void cExportsWidget::text(const QString& s)
{
    if (s.isEmpty()) return;
    pUi->textEdit->moveCursor(QTextCursor::End);
    pUi->textEdit->insertPlainText(s);
    pUi->textEdit->moveCursor(QTextCursor::End);
}

void cExportsWidget::ready()
{
    if (pThread == NULL) EXCEPTION(EPROGFAIL);
    if (isStop) return;
    cError *pe = pThread->pLastError;
    if (pe != NULL) {
        cErrorMessageBox::messageBox(pe, this);
    }
    pDelete(pThread);
    QString text = pUi->textEdit->toPlainText();
    text.replace(QRegExp("\\s+;"), ";");
    text.replace(QRegExp("\\s\\s+\\{"), " {");
    pUi->textEdit->setPlainText(text);
    disable(false);
}