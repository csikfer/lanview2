#include "gexports.h"
#include "export.h"
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
    QStringList ol;
    ol << _sParamTypes << _sSysParams << _sServices << _sQueryParsers << _sIfTypes << _sTableShapes << _sMenuItems;
    pUi->setupUi(this);
    pUi->comboBoxTable->addItems(ol);
    bool empty = ol.isEmpty();
    pUi->pushButtonStop->setDisabled(true);
    if (!empty) changedName(ol.first());
    connect(pUi->pushButtonExport, SIGNAL(clicked()),     this, SLOT(start()));
    connect(pUi->pushButtonSave,   SIGNAL(clicked()),     this, SLOT(save()));
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
    pUi->plainTextEdit->setReadOnly(f);
    pUi->pushButtonStop->setEnabled(f);
}

void cExportsWidget::start()
{
    pUi->pushButtonExport->setDisabled(true);
    QString tn = pUi->comboBoxTable->currentText();
    QString r;
    cExport e;
    if      (0 == tn.compare(_sParamTypes))     r = e.paramType(EX_IGNORE);
    else if (0 == tn.compare(_sSysParams))      r = e.sysParams(EX_IGNORE);
    else if (0 == tn.compare(_sServices))       r = e.services(EX_IGNORE);
    else if (0 == tn.compare(_sQueryParsers))   r = e.queryParser(EX_IGNORE);
    else if (0 == tn.compare(_sIfTypes))        r = e.ifType(EX_IGNORE);
    else if (0 == tn.compare(_sTableShapes))    r = e.tableShapes(EX_IGNORE);
    else if (0 == tn.compare(_sMenuItems))      r = e.menuItems(EX_IGNORE);
    if (r.isEmpty()) r = trUtf8("// %1 is empty.").arg(tn);
    r.prepend(pUi->plainTextEdit->toPlainText());
    pUi->plainTextEdit->setPlainText(r);
}

void cExportsWidget::save()
{
    textToFile(fileName, pUi->plainTextEdit->toPlainText(), this);
}

void cExportsWidget::changedName(const QString& tn)
{
    QSqlQuery q = getQuery();
    cTableShape ts;
    ts.setByName(q, tn);
    ts.fetchText(q);
    pUi->lineEditTableTitle->setText(ts.getText(cTableShape::LTX_TABLE_TITLE));
    pUi->pushButtonExport->setDisabled(false);
}

