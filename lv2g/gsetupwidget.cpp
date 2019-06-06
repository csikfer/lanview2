#include "gsetupwidget.h"
#include "popupreport.h"

const enum ePrivilegeLevel cGSetupWidget::rights = PL_VIEWER;

cGSetupWidget::cGSetupWidget(QMdiArea *par)
    : cIntSubObj(par)
    , qset(*lanView::getInstance()->pSet)
{
    pSound = nullptr;
    enum Qt::Orientation splOrient = lv2g::getInstance()->defaultSplitOrientation;
    QString sounFileAlarm = lv2g::getInstance()->soundFileAlarm;
    int maxRows = lv2g::getInstance()->maxRows;
    int dialogRows = lv2g::getInstance()->dialogRows;
    pUi = new Ui_GSetup;
    pUi->setupUi(this);
    pUi->radioButtonHorizontal->setChecked(splOrient == Qt::Horizontal);
    pUi->radioButtonVertikal->setChecked(splOrient != Qt::Horizontal);
    pUi->lineEditAlarm->setText(sounFileAlarm);
    pUi->pushButtonAlarmTest->setDisabled(sounFileAlarm.isEmpty());
    pUi->spinBoxMaxRows->setValue(maxRows);
    pUi->spinBoxDialogRows->setValue(dialogRows);
    pUi->checkBoxNatMenu->setChecked(!lv2g::getInstance()->nativeMenubar);

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
        v = lv2g::sVertical;
        lv2g::getInstance()->defaultSplitOrientation = Qt::Vertical;
    }
    else {
        v = lv2g::sHorizontal;
        lv2g::getInstance()->defaultSplitOrientation = Qt::Horizontal;
    }
    lv2g::getInstance()->nativeMenubar = !pUi->checkBoxNatMenu->isChecked();

    qset.setValue(lv2g::sDefaultSplitOrientation, v);
    lv2g::getInstance()->maxRows = pUi->spinBoxMaxRows->value();
    qset.setValue(lv2g::sMaxRows, lv2g::getInstance()->maxRows);
    lv2g::getInstance()->dialogRows = pUi->spinBoxDialogRows->value();
    qset.setValue(lv2g::sDialogRows, lv2g::getInstance()->dialogRows);
    lv2g::getInstance()->soundFileAlarm = pUi->lineEditAlarm->text();
    qset.setValue(lv2g::sSoundFileAlarm, lv2g::getInstance()->soundFileAlarm);
    qset.setValue(lv2g::sNativeMenubar, QVariant(lv2g::getInstance()->nativeMenubar ? _sTrue : _sFalse));
    qset.sync();
    QSettings::Status st = qset.status();
    if (QSettings::NoError != st) {
        QString err;
        switch (st) {
            case QSettings::AccessError:
                err = tr("An access error occurred (e.g. trying to write to a read-only file).");
                break;
            case QSettings::FormatError:
                err = tr("A format error occurred (e.g. loading a malformed INI file).");
                break;
            default:
                err = tr("Invalid QSettings status : ") + QString::number((int)st);
                break;
        }
        cMsgBox::warning(err, this);
    }
    else cMsgBox::info(tr("New settings accepted."), this);
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
    QString fp = QFileDialog::getOpenFileName(this, tr("Hang fájl kiválasztása"), tr("Hang fájlok (*.wav)"));
    if (fp.isEmpty()) return;
    pUi->pushButtonAlarmTest->setEnabled(true);
    pUi->lineEditAlarm->setText(fp);
}

void cGSetupWidget::testAlarmFile()
{
    QString fn = pUi->lineEditAlarm->text();
    if (fn.isEmpty()) {
        cMsgBox::warning(tr("Nincs megadva hang fájl."), this);
        return;
    }
    pDelete(pSound);
    pSound = new QSound(fn, this);
    pSound->setLoops(1);
    pSound->play();
}

