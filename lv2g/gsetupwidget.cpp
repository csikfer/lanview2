#include "gsetupwidget.h"

const enum ePrivilegeLevel cGSetupWidget::rights = PL_VIEWER;

cGSetupWidget::cGSetupWidget(QSettings &__s, QWidget *par)
    : cOwnTab(par)
    , qset(__s)
{
    pSound = NULL;
    bool splOrient = lv2g::getInstance()->defaultSplitOrientation;
    QString sounFileAlarm = lv2g::getInstance()->sounFileAlarm;
    pUi = new Ui_GSetup;
    pUi->setupUi(this);
    pUi->radioButtonHorizontal->setChecked(splOrient);
    pUi->radioButtonVertikal->setChecked(!splOrient);
    pUi->lineEditAlarm->setText(sounFileAlarm);
    pUi->pushButtonAlarmTest->setDisabled(sounFileAlarm.isEmpty());

    connect(pUi->PBApplicateAndRestart,SIGNAL(clicked()),    this,   SLOT(applicateAndRestart()));
    connect(pUi->PBApplicateAndExit,   SIGNAL(clicked()),    this,   SLOT(applicateAndExit()));
    connect(pUi->PBCancel,             SIGNAL(clicked()),    this,   SLOT(endIt()));
    connect(pUi->PBApplicateAndClose,  SIGNAL(clicked()),    this,   SLOT(applicateAndClose()));
    connect(pUi->pushButtonAlarmFile,  SIGNAL(clicked()),    this,   SLOT(selectAlarmFile()));
    connect(pUi->pushButtonAlarmTest,  SIGNAL(clicked()),    this,   SLOT(testAlarmFile()));
}

void cGSetupWidget::applicate()
{
    DBGFN();
    QString v;
    if (pUi->radioButtonVertikal->isChecked()) {
        v = "Vertical";
        lv2g::getInstance()->defaultSplitOrientation = Qt::Vertical;
    }
    else {
        v = "Horizontal" ;
        lv2g::getInstance()->defaultSplitOrientation = Qt::Horizontal;
    }
    qset.setValue(lv2g::sDefaultSplitOrientation, v);
    lv2g::getInstance()->maxRows = pUi->spinBoxMaxRows->value();
    qset.setValue(lv2g::sMaxRows, lv2g::getInstance()->maxRows);
    lv2g::getInstance()->sounFileAlarm = pUi->lineEditAlarm->text();
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

void cGSetupWidget::selectAlarmFile()
{
    QString fp = QFileDialog::getOpenFileName(this, trUtf8("Hang fájl kiválasztása"), trUtf8("Hang fájlok (*.vaw)"));
    if (fp.isEmpty()) return;
    pUi->pushButtonAlarmTest->setEnabled(true);
    pUi->lineEditAlarm->setText(fp);
}

void cGSetupWidget::testAlarmFile()
{
    QString warning =trUtf8("Figyelmeztetés");
    QString fn = pUi->lineEditAlarm->text();
    if (fn.isEmpty()) {
        QMessageBox::warning(this, warning, trUtf8("Nincs megadva hang fájl."));
        return;
    }
    pDelete(pSound);
    pSound = new QSound(fn, this);
    pSound->setLoops(1);
    pSound->play();
}
