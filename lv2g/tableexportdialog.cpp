#include "tableexportdialog.h"
#include "ui_tableexportdialog.h"
#include "QPushButton"
#include "QFileDialog"

cTableExportDialog::cTableExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::cTableExportDialog)
{
    ui->setupUi(this);
}

cTableExportDialog::~cTableExportDialog()
{
    delete ui;
}

enum eTableExportTarget cTableExportDialog::target() const
{
    if (ui->radioButtonClip->isChecked()) return TET_CLIP;
    if (ui->radioButtonWin->isChecked())  return TET_WIN;
    return TET_FILE;
}
enum eTableExportWhat   cTableExportDialog::what() const
{
    if (ui->radioButtonName->isChecked())     return TEW_NAME;
    if (ui->radioButtonSelected->isChecked()) return TEW_SELECTED;
    if (ui->radioButtonViewed->isChecked())   return TEW_VIEWED;
    return TEW_ALL;
}

enum eTableExportForm   cTableExportDialog::form() const
{
    if (ui->radioButtonCSV->isChecked()) return TEF_CSV;
    return TEF_HTML;
}

QString cTableExportDialog::path()
{
    return ui->lineEditFilePath->text();
}

QString cTableExportDialog::getOtherPath()
{
    ui->groupBoxForm->setDisabled(true);
    ui->groupBoxTarget->setDisabled(true);
    ui->groupBoxWhat->setDisabled(true);
    if (exec() == Rejected) return QString();
    return path();
}

void cTableExportDialog::on_radioButtonFile_toggled(bool checked)
{
    bool f = !checked || !ui->lineEditFilePath->text().isEmpty();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(f);
}

void cTableExportDialog::on_lineEditFilePath_textChanged(const QString &text)
{
    bool f = !text.isEmpty() || !ui->radioButtonFile->isChecked();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(f);
}

void cTableExportDialog::on_toolButtonFilePath_clicked()
{
    QString path = QFileDialog::getSaveFileName(this);
    if (path.isEmpty()) return;
    ui->lineEditFilePath->setText(path);
}
