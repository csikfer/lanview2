#ifndef SETUP_H
#define SETUP_H

#include "lv2g.h"
#if defined(LV2G_LIBRARY)
#include "ui_setup.h"
#include "ui_setup_logl.h"
#else
namespace Ui {
    class SetupWidget;
}
#endif
#include <QDialog>
#include "lv2widgets.h"

class cLogLevelDialog;

/// @class cSetupWidget
/// Program setup (tab) widget
class LV2GSHARED_EXPORT cSetupWidget : public cIntSubObj
{
    Q_OBJECT
public:
    cSetupWidget(QMdiArea *par);
    ~cSetupWidget();
    /// Minimális jogosultsági szint az eléréséhez.
    static const enum ePrivilegeLevel rights;
private:
    Ui::SetupWidget *pUi;
    cSelectLanguage *pSelLang;
private slots:
    void applicateAndRestart();
    void applicateAndExit();
    void applicateAndClose();
    void logToStdOutClicked(bool __b);
    void logToStdErrClicked(bool __b);
    void logToFileClicked(bool __b);
    void logLevelMoreClicked();
    void setLogLevel();
    void checkSqlLogin();
    void mibPathPlus();
    void mibPathMinus();
    void homeSelect();
private:
    void applicate();
    QSqlDatabase *  SqlOpen();
    QString         logFile;
    QSettings       &qset;
    cLogLevelDialog *pLl;
    bool            forced;
    QList<QPair<QString, QLineEdit *> >   others;
};

#if defined(LV2G_LIBRARY)

class cLogLevelDialog : public QDialog, Ui::LogLevelDialog
{
    Q_OBJECT
   public:
    cLogLevelDialog(qlonglong __logLev, QWidget *parent = 0);
    ~cLogLevelDialog();
    qlonglong getLogLevel();
    void setLogLevel(qlonglong __ll);
   private slots:
    void allOnClicked();
    void allOffClicked();
};
#endif

#endif // SETUP_H
