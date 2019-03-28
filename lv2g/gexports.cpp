#include "gexports.h"
#include "export.h"
#include "cerrormessagebox.h"
#include <QRegExp>
#include "popupreport.h"

const enum ePrivilegeLevel cExportsWidget::rights = PL_OPERATOR;

static const QString _sGUI = "GUI";

enum eExport {
    E_PRAM_TYPES, E_SYS_PARAMS, E_SERVICES, E_QUERY_PARSERS, E_IF_TYPES,
    E_TABLE_SHAPE, E_MENU_ITEMS, E_ENUM_VALS,
    E_SERVICE_VAR_TYPES, E_SERVICE_VARS,
    E_GUI
};

cExportsWidget::cExportsWidget(QMdiArea *par)
    : cIntSubObj(par)
    , pUi(new Ui::Exports)
{
//    pThread = NULL;
    isStop  = false;
    QSqlQuery q = getQuery();
    QStringList ol = cExport::exportableObjects();
    ol << _sGUI;
    pUi->setupUi(this);
    pUi->comboBoxTable->addItems(ol);
    pUi->pushButtonStop->hide();    // Not working
    changedName(ol.first());
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
    int ix = pUi->comboBoxTable->currentIndex();
    QString r;
    cExport e;
    if (ix < cExport::exportableObjects().size()) {
        r = e.exportTable(ix);
    }
    else {
        QString s = pUi->comboBoxTable->currentText();
        if (_sGUI == s) {
            r  = e.MenuItems(EX_IGNORE);
            r += e.EnumVals(EX_IGNORE);
            r += e.TableShapes(EX_IGNORE);
        }
        else {
            EXCEPTION(EPROGFAIL);
        }
    }
    if (r.isEmpty()) r = trUtf8("// %1 is empty.").arg(pUi->comboBoxTable->currentText());
    r.prepend(pUi->plainTextEdit->toPlainText());
    pUi->plainTextEdit->setPlainText(r);
}

void cExportsWidget::save()
{
    cFileDialog::textToFile(fileName, pUi->plainTextEdit->toPlainText(), this);
}

void cExportsWidget::changedName(const QString& tn)
{
    QString t;
    if (tn == _sGUI) {
        t = trUtf8("Teljes GUI export (MenuItems + EnumVals + TableShapes).");
    }
    else {
        QSqlQuery q = getQuery();
        cTableShape ts;
        ts.setByName(q, tn);
        ts.fetchText(q);
        t = ts.getText(cTableShape::LTX_TABLE_TITLE);
    }
    pUi->lineEditTableTitle->setText(t);
    pUi->pushButtonExport->setDisabled(false);
}


void cExportsWidget::on_pushButtonClear_clicked()
{
    pUi->plainTextEdit->clear();
    pUi->pushButtonExport->setEnabled(true);
}
