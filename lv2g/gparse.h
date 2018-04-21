#ifndef GPARSE_H
#define GPARSE_H

#include "lv2g.h"
#include "import_parser.h"

#include "lv2g.h"
#if defined(LV2G_LIBRARY)
#include "ui_gparse.h"
#else
namespace Ui {
    class GParseWidget;
}
#endif

class  LV2GSHARED_EXPORT cParseWidget : public cIntSubObj
{
    Q_OBJECT
public:
    cParseWidget(QMdiArea *par);
    ~cParseWidget();
    static const enum ePrivilegeLevel rights;
private:
    Ui::GParseWidget *pUi;
private slots:
    void loadClicked();
    void saveClicked();
    void parseClicked();
    void debugLine();
    void exportLine();
    void localParseFinished();
    void localParseBreak();

    void loadQPClicked();
    void saveQPClicked();
    void parseQPClicked();
    void paramClicked();
    void on_checkBoxQP_toggled(bool checked);

    void on_pushButtonRepSave_clicked();

    void on_pushButtonLogSave_clicked();

private:
    void localParse(const QString &src);
    void remoteParse(const QString &src);
    void setParams();
    QSqlQuery   *pq;
    QString fileName;
    QString fileNamePQ;
    bool    isRuning;
    cImportParseThread *pLocalParser;
    cError             *pLocalError;
    cExportQueue       *pExportQueue;
    QString            *pLocalParsedStr;
    QStringList         debugLines;
    QStringList         qParseList;
    tStringMap          params;
};

#endif // GPARSE_H
