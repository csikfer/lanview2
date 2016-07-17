#include "gsetupwidget.h"

const enum ePrivilegeLevel cGSetupWidget::rights = PL_VIEWER;

cGSetupWidget::cGSetupWidget(QSettings &__s, QWidget *par)
    : cOwnTab(par)
    , qset(__s)
{
    bool splOrient = lv2g::getInstance()->defaultSplitOrientation;
    pUi = new Ui_GSetup;
    pUi->setupUi(this);
    pUi->radioButtonHorizontal->setChecked(splOrient);
    pUi->radioButtonVertikal->setChecked(!splOrient);

    connect(pUi->PBApplicateAndRestart,SIGNAL(clicked()),    this,   SLOT(applicateAndRestart()));
    connect(pUi->PBApplicateAndExit,   SIGNAL(clicked()),    this,   SLOT(applicateAndExit()));
    connect(pUi->PBCancel,             SIGNAL(clicked()),    this,   SLOT(endIt()));
    connect(pUi->PBApplicateAndClose,  SIGNAL(clicked()),    this,   SLOT(applicateAndClose()));

}

void cGSetupWidget::applicate()
{
    DBGFN();
    QString v = pUi->radioButtonVertikal->isChecked() ?
                "Vertical" :
                "Horizontal" ;
    qset.setValue(lv2g::sDefaultSplitOrientation, v);
    qset.sync();
    QSettings::Status st = qset.status();
    if (QSettings::NoError != st) {
        QString err;
        switch (st) {
            case QSettings::AccessError:
                err = trUtf8("An access error occurred (e.g. trying to write to a read-only file).");
                break;
            case QSettings::FormatError:
                err = trUtf8("A format error occurred (e.g. loading a malformed INI file).");
                break;
            default:
                err = trUtf8("Invalid QSettings status : ") + QString::number((int)st);
                break;
        }
        QMessageBox::warning(this, design().titleError, err);
    }
    else QMessageBox::information(this, design().titleInfo, trUtf8("New settings accepted."));
    DBGFNL();
}

void cGSetupWidget::applicateAndRestart()
{
    applicate();
    appReStart();
}

void cGSetupWidget::applicateAndExit()
{
    applicate();
    qApp->exit(0);
}

void cGSetupWidget::applicateAndClose()
{
    applicate();
    endIt();
}
