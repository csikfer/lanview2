#ifndef TABLEEXPORTDIALOG_H
#define TABLEEXPORTDIALOG_H

#include <QDialog>

enum eTableExportTarget {
    TET_CLIP, TET_FILE
};
enum eTableExportWhat {
    TEW_NAME, TEW_SELECTED, TEW_VIEWED, TEW_ALL
};
enum eTableExportForm {
    TEF_CSV, TEF_HTML
};

namespace Ui {
class cTableExportDialog;
}

class cTableExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit cTableExportDialog(QWidget *parent = 0);
    ~cTableExportDialog();
    enum eTableExportTarget target() const;
    enum eTableExportWhat   what() const;
    enum eTableExportForm   form() const;
    QString path();
    QString getOtherPath();

private slots:

    void on_radioButtonFile_toggled(bool checked);

    void on_lineEditFilePath_textChanged(const QString &text);

    void on_toolButtonFilePath_clicked();

private:
    Ui::cTableExportDialog *ui;
};

#endif // TABLEEXPORTDIALOG_H
