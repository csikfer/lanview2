#include "setup.h"
#include <QMessageBox>
#include <QFileDialog>


const enum ePrivilegeLevel cSetupWidget::rights = PL_ADMIN;

cSetupWidget::cSetupWidget(QMdiArea *par)
: cIntSubObj(par)
, logFile()
, qset(*lanView::getInstance()->pSet)
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    pLl = nullptr;
    pSelLang = nullptr;
    pUi = new Ui::SetupWidget();
    pUi->setupUi(this);
    if (lanView::getInstance()->dbIsOpen()) {
        pSelLang = new cSelectLanguage(pUi->comboBoxLang, pUi->labelFlag, false, this);
    }

    bool forced = !lanView::dbIsOpen();

    connect(pUi->PBApplicateAndRestart,SIGNAL(clicked()),   this,   SLOT(applicateAndRestart()));
    connect(pUi->PBApplicateAndExit,   SIGNAL(clicked()),   this,   SLOT(applicateAndExit()));
    connect(pUi->PBCancel,             SIGNAL(clicked()),   this,   SLOT(endIt()));
    pUi->PBCancel->setDisabled(forced);
    connect(pUi->PBApplicateAndClose,  SIGNAL(clicked()),   this,   SLOT(applicateAndClose()));
    pUi->PBApplicateAndClose->setDisabled(forced);

    connect(pUi->logLevelMore,   SIGNAL(clicked()),      this,   SLOT(logLevelMoreClicked()));
    connect(pUi->logToStdOutRB,  SIGNAL(clicked(bool)),  this,   SLOT(logToStdOutClicked(bool)));
    connect(pUi->logToStdErrRB,  SIGNAL(clicked(bool)),  this,   SLOT(logToStdErrClicked(bool)));
    connect(pUi->logToFileRB,    SIGNAL(clicked(bool)),  this,   SLOT(logToFileClicked(bool)));
    connect(pUi->CheckSql,       SIGNAL(clicked(bool)),  this,   SLOT(checkSqlLogin()));
    connect(pUi->MibPathPlus,    SIGNAL(clicked(bool)),  this,   SLOT(mibPathPlus()));
    connect(pUi->MibPathMinus,   SIGNAL(clicked(bool)),  this,   SLOT(mibPathMinus()));
    connect(pUi->SelectHome,     SIGNAL(clicked(bool)),  this,   SLOT(homeSelect()));

    pUi->homeDirLE->setText(qset.value(_sHomeDir, lanView::homeDefault).toString());
    pUi->sqlHostLE->setText(qset.value(_sSqlHost, _sLocalHost).toString());
    pUi->sqlPortSB->setValue(qset.value(_sSqlPort, 5432).toInt());
    pUi->sqlUserLE->setText(scramble(qset.value(_sSqlUser, scramble(_sLanView2)) .toString()));
    pUi->sqlPassLE->setText(scramble(qset.value(_sSqlPass).toString()));
    pUi->dbNameLE-> setText(qset.value(_sDbName,  _sLanView2) .toString());

    QRegExp regExp("\\d+|0x[\\dA-Za-z]*");
    pUi->debugLevelLE->setValidator(new QRegExpValidator(regExp, pUi->debugLevelLE));
    pUi->debugLevelLE->setText(_sHex + QString::number(qset.value(_sDebugLevel, lanView::debugDefault).toLongLong(), 16));
    logFile = qset.value(_sLogFile, _sStdErr).toString();
    if (logFile == "-" || logFile == _sStdOut) {
        logFile = _sNul;
        pUi->logToStdOutRB->setChecked(true);
        logToStdOutClicked(true);
    }
    else if (logFile == _sStdErr) {
        logFile = _sNul;
        pUi->logToStdErrRB->setChecked(true);
        logToStdErrClicked(true);
    }
    else {
        pUi->logToFileRB->setChecked(true);
        logToFileClicked(true);
    }
    QStringList mibPathList = qset.value(_sMibPath).toString().split(QChar(','));
    foreach (QString dir, mibPathList) {
        pUi->MibPathLS->addItem(new QListWidgetItem(dir, pUi->MibPathLS));
    }
    DBGFNL();
}

cSetupWidget::~cSetupWidget()
{
    PDEB(OBJECT) << "delete (cSetupWidget *)" << p2string(this) << endl;
    if (pLl != nullptr) delete pLl;
    DBGFNL();
}

void cSetupWidget::applicate()
{
    DBGFN();
    bool ok = false;
    if (pSelLang != nullptr) {
        int langId = pSelLang->currentLangId();
        qset.setValue(_sLangId,  langId);
    }
    qset.setValue(_sHomeDir, pUi->homeDirLE->text());
    qset.setValue(_sSqlHost, pUi->sqlHostLE->text());
    qset.setValue(_sSqlPort, pUi->sqlPortSB->value());
    qset.setValue(_sSqlUser, scramble(pUi->sqlUserLE->text()));
    qset.setValue(_sSqlPass, scramble(pUi->sqlPassLE->text()));
    qset.setValue(_sDbName,  pUi->dbNameLE->text());
    qset.setValue(_sDebugLevel, pUi->debugLevelLE->text().toLongLong(&ok, 0));
    if (!ok) DERR() << "Invalid log level : " << pUi->logFileNameLE->text() << endl;
    qset.setValue(_sLogFile, pUi->logFileNameLE->text());
    int i, n = pUi->MibPathLS->count();
    QString mibPath;
    for (i = 0; i < n; i++) {
        if (mibPath.count()) mibPath += QChar(',');
        mibPath += pUi->MibPathLS->item(i)->text();
    }
    qset.setValue(_sMibPath, mibPath);
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
        QMessageBox::warning(this, dcViewShort(DC_ERROR), err);
    }
    else QMessageBox::information(this, dcViewShort(DC_INFO), tr("New settings accepted."));
    DBGFNL();
}

void cSetupWidget::applicateAndRestart()
{
    applicate();
    appReStart();
}

void cSetupWidget::applicateAndExit()
{
    applicate();
    qApp->exit(0);
}

void cSetupWidget::applicateAndClose()
{
    applicate();
    endIt();
}

void cSetupWidget::logToStdOutClicked(bool __b)
{
    DBGFN();
    if (__b) {
        pUi->logFileNameLE->setEnabled(false);
        pUi->logFileNameLE->setText(_sStdOut);
    }
    DBGFNL();
}
void cSetupWidget::logToStdErrClicked(bool __b)
{
    DBGFN();
    if (__b) {
        pUi->logFileNameLE->setEnabled(false);
        pUi->logFileNameLE->setText(_sStdErr);
    }
    DBGFNL();
}
void cSetupWidget::logToFileClicked(bool __b)
{
    DBGFN();
    if (__b) {
        pUi->logFileNameLE->setEnabled(true);
        pUi->logFileNameLE->setText(logFile);
    }
    DBGFNL();
}
void cSetupWidget::logLevelMoreClicked()
{
    DBGFN();
    bool ok;
    qlonglong l = pUi->debugLevelLE->text().toLongLong(&ok, 0);
    if (!ok) DERR() << "Invalid log level : " << pUi->logFileNameLE->text() << endl;
    if (pLl == nullptr) {
        pLl = new cLogLevelDialog(l, this);
        connect(pLl,   SIGNAL(accepted()),this, SLOT(setLogLevel()));
    }
    else pLl->setLogLevel(l);
    pLl->show();
    DBGFNL();
}

void cSetupWidget::setLogLevel(void)
{
    if (pLl == nullptr) {
        DERR() << "Program error: cSetupWidget::pLl is NULL, ignored signal to " << __PRETTY_FUNCTION__ << " slot" << endl;
        return;
    }
    pUi->debugLevelLE->setText(_sHex + QString::number(pLl->getLogLevel(), 16));
}

void cSetupWidget::mibPathPlus()
{
    DBGFN();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Adja meg a MIB fájlok könyvtárát!"));
    if (dir.isEmpty() == false) {
        QList<QListWidgetItem *> l = pUi->MibPathLS->findItems(dir, Qt::MatchFixedString);
        if (l.count() == 0) {
            pUi->MibPathLS->addItem(new QListWidgetItem(dir, pUi->MibPathLS));
        }
    }
    DBGFNL();
}

void cSetupWidget::mibPathMinus()
{
    DBGFN();
    QList<QListWidgetItem *>    selected =  pUi->MibPathLS->selectedItems();
    if (selected.count()) {
        QListWidgetItem *   p;
        foreach (p, selected) {
            int row = pUi->MibPathLS->row(p);
            PDEB(INFO) << "Remove #" << row << " row from list..." << endl;
            // MibPathLS->removeItemWidget(p);
            if (p != pUi->MibPathLS->takeItem(row)) EXCEPTION(EPROGFAIL);
            delete p;
        }
    }
    else PDEB(WARNING) << "Not selected any row" << endl;
    DBGFNL();
    return;
}

void cSetupWidget::homeSelect()
{
    PDEB(VVERBOSE) << VDEBPTR(pUi->homeDirLE) << ": == " << pUi->homeDirLE->text() << endl;
    pUi->homeDirLE->setText(QFileDialog::getExistingDirectory(this, tr("Alap könyvtár kiválasztása"), pUi->homeDirLE->text()));
    PDEB(VVERBOSE) << VDEBPTR(pUi->homeDirLE) << ": == " << pUi->homeDirLE->text() << endl;
}

cLogLevelDialog::cLogLevelDialog(qlonglong __logLev, QWidget *parent)
: QDialog(parent)
, Ui::LogLevelDialog()
{
    PDEB(OBJECT) << "cLogLevelWidget::cLogLevelWidget(), this = " << p2string(this) << endl;
    setupUi(this);

    connect(Cancel, SIGNAL(clicked()), this, SLOT(reject()));
    connect(Ok,     SIGNAL(clicked()), this, SLOT(accept()));
    connect(allOn,  SIGNAL(clicked()), this, SLOT(allOnClicked()));
    connect(allOff, SIGNAL(clicked()), this, SLOT(allOffClicked()));
    setLogLevel(__logLev);
    DBGFNL();
}
cLogLevelDialog::~cLogLevelDialog() {
    PDEB(OBJECT) << "delete (cLogLevelWidget *)" << p2string(this) << endl;
}
inline Qt::CheckState checkState(qlonglong __m, qlonglong __b) { return __m & __b ? Qt::Checked : Qt::Unchecked; }
void cLogLevelDialog::setLogLevel(qlonglong __ll)
{
    checkBoxException-> setCheckState(checkState(__ll, cDebug::EXCEPT));
    checkBoxError->     setCheckState(checkState(__ll, cDebug::DERROR));
    checkBoxWarn->      setCheckState(checkState(__ll, cDebug::WARNING));
    checkBoxInfo->      setCheckState(checkState(__ll, cDebug::INFO));
    checkBoxVerb->      setCheckState(checkState(__ll, cDebug::VERBOSE));
    checkBoxVVerb->     setCheckState(checkState(__ll, cDebug::VVERBOSE));
    checkBoxEnterLeave->setCheckState(checkState(__ll, cDebug::ENTERLEAVE));
    checkBoxPArg->      setCheckState(checkState(__ll, cDebug::PARSEARG));
    checkBoxSQL->       setCheckState(checkState(__ll, cDebug::SQL));
    checkBoxObject->    setCheckState(checkState(__ll, cDebug::OBJECT));
    checkBoxAddress->   setCheckState(checkState(__ll, cDebug::ADDRESS));
    checkBoxSNMP->      setCheckState(checkState(__ll, cDebug::SNMP));

    checkBoxLv2->       setCheckState(checkState(__ll, cDebug::LV2));
    checkBoxLv2g->      setCheckState(checkState(__ll, cDebug::LV2G));
    checkBoxParser->    setCheckState(checkState(__ll, cDebug::PARSER));
    checkBoxApp->       setCheckState(checkState(__ll, cDebug::APP));
}
inline void checkState(Qt::CheckState __st, qlonglong& __m, qlonglong __b) { if (__st ==  Qt::Checked) __m |= __b; }
qlonglong cLogLevelDialog::getLogLevel()
{
    qlonglong l = 0;
    checkState(checkBoxException-> checkState(), l, cDebug::EXCEPT);
    checkState(checkBoxError->     checkState(), l, cDebug::DERROR);
    checkState(checkBoxWarn->      checkState(), l, cDebug::WARNING);
    checkState(checkBoxInfo->      checkState(), l, cDebug::INFO);
    checkState(checkBoxVerb->      checkState(), l, cDebug::VERBOSE);
    checkState(checkBoxVVerb->     checkState(), l, cDebug::VVERBOSE);
    checkState(checkBoxEnterLeave->checkState(), l, cDebug::ENTERLEAVE);
    checkState(checkBoxPArg->      checkState(), l, cDebug::PARSEARG);
    checkState(checkBoxSQL->       checkState(), l, cDebug::SQL);
    checkState(checkBoxObject->    checkState(), l, cDebug::OBJECT);
    checkState(checkBoxObject->    checkState(), l, cDebug::OBJECT);
    checkState(checkBoxAddress->   checkState(), l, cDebug::ADDRESS);
    checkState(checkBoxSNMP->      checkState(), l, cDebug::SNMP);

    checkState(checkBoxLv2->       checkState(), l, cDebug::LV2);
    checkState(checkBoxLv2g->      checkState(), l, cDebug::LV2G);
    checkState(checkBoxParser->    checkState(), l, cDebug::PARSER);
    checkState(checkBoxApp->       checkState(), l, cDebug::APP);

    return l;
}
void cLogLevelDialog::allOnClicked()  { setLogLevel(-1); }
void cLogLevelDialog::allOffClicked() { setLogLevel(0);  }


QSqlDatabase * cSetupWidget::SqlOpen()
{
    _DBGFNL() << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    QSqlDatabase *pDb = new  QSqlDatabase(QSqlDatabase::addDatabase(_sQPSql));
    if (!pDb->isValid()) {
        QSqlError le = pDb->lastError();
        QString msg = QString("SQL DB ERROR #") + QString::number(le.number()) + "\n"
                    + "driverText   : " + le.driverText() + "\n"
                    + "databaseText : " + le.databaseText();
        QMessageBox::warning(this, dcViewShort(DC_ERROR), msg);
        delete pDb;
        return nullptr;
    }
    pDb->setHostName(   pUi->sqlHostLE->text());
    pDb->setPort(       pUi->sqlPortSB->value());
    pDb->setUserName(   pUi->sqlUserLE->text());
    pDb->setPassword(   pUi->sqlPassLE->text());
    pDb->setDatabaseName(pUi->dbNameLE->text());
    QString msg;
    if (!pDb->open()) {
        QSqlError le = pDb->lastError();
        msg = QString("SQL open ERROR #") + QString::number(le.number()) + "\n"
                    + "driverText   : " + le.driverText() + "\n"
                    + "databaseText : " + le.databaseText();
        QMessageBox::warning(this, dcViewShort(DC_ERROR), msg);
        delete pDb;
        return nullptr;
    }
    QSqlQuery q(*pDb);
    if (!checkDbVersion(q, msg)) {
        QMessageBox::warning(this, dcViewShort(DC_ERROR), msg);
        delete pDb;
        return nullptr;
    }
    return pDb;
}

void cSetupWidget::checkSqlLogin()
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    QSqlDatabase *pDb = SqlOpen();
    if (pDb == nullptr) return;
    QMessageBox::information(this, dcViewShort(DC_INFO), tr("Database open is successful."));
    pDb->close();
    delete pDb;
}
