#ifndef TABLEEXPORTDIALOG_H
#define TABLEEXPORTDIALOG_H

#include <QDialog>

enum eTableExportTarget {
    TET_CLIP,   ///< Export to clipboard
    TET_FILE,   ///< Export to file
    TET_WIN     ///< Export to text window
};
enum eTableExportWhat {
    TEW_NAME,       ///< Export name(s) only
    TEW_SELECTED,   ///< Export selected rows
    TEW_VIEWED,     ///< Export viewed rows
    TEW_ALL         ///< Export all rows
};
enum eTableExportForm {
    TEF_CSV,        ///< Export to CSV
    TEF_HTML,       ///< Export to HTML
    TEF_INTER       ///< Export to import interpreter
};

namespace Ui {
    class cTableExportDialog;
}

class cTableExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit cTableExportDialog(QWidget *parent = nullptr, const QString& _objName = QString());
    ~cTableExportDialog();
    enum eTableExportTarget target() const;
    enum eTableExportWhat   what() const;
    enum eTableExportForm   form() const;
    QString path();
    QString getOtherPath();
    const QString objectName;

private slots:
    void on_radioButtonFile_toggled(bool checked);
    void on_lineEditFilePath_textChanged(const QString &text);
    void on_toolButtonFilePath_clicked();
    void on_radioButtonName_toggled(bool checked);

    void on_radioButtonAll_toggled(bool checked);

private:
    Ui::cTableExportDialog *ui;
    int isExportableObject;
};

#endif // TABLEEXPORTDIALOG_H
