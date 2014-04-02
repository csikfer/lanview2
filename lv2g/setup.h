#ifndef SETUP_H
#define SETUP_H

#include "lv2g.h"
#include "ui_setup.h"
#include "ui_setup_logl.h"
#include <QDialog>

class cLogLevelDialog;

class cSetupWidget : public cOwnTab, public Ui::SetupWidget
{
    Q_OBJECT
public:
    cSetupWidget(QSettings &__s, QWidget *par);
    ~cSetupWidget();
private slots:
    void applicateAndRestart();
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
};

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

#endif // SETUP_H
