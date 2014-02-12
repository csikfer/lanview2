#include "lv2gui.h"


cSetupWidget::cSetupWidget(QSettings &__s, cMainWindow *par)
: cOwnTab(*par)
, Ui::SetupWidget()
, logFile()
, qset(__s)
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << _sSpace << _sComma << VDEBPTR(this) << endl;
    forced = false == ((lv2Gui *)lanView::getInstance())->dbIsOpen;
    pLl = NULL;
    setupUi(this);

    connect(PBApplicateAndRestart,SIGNAL(clicked()),    this,   SLOT(applicateAndRestart()));
    connect(PBApplicateAndClose,  SIGNAL(clicked()),    this,   SLOT(applicateAndClose()));
    connect(PBCancel,             SIGNAL(clicked()),    this,   SLOT(close()));
    PBCancel->setDisabled(forced);
    connect(PBClose,              SIGNAL(clicked()),    this,   SLOT(closeIt()));
    PBClose->setDisabled(forced);

    connect(logLevelMore,   SIGNAL(clicked()),      this,   SLOT(logLevelMoreClicked()));
    connect(logToStdOutRB,  SIGNAL(clicked(bool)),  this,   SLOT(logToStdOutClicked(bool)));
    connect(logToStdErrRB,  SIGNAL(clicked(bool)),  this,   SLOT(logToStdErrClicked(bool)));
    connect(logToFileRB,    SIGNAL(clicked(bool)),  this,   SLOT(logToFileClicked(bool)));
    connect(CheckSql,       SIGNAL(clicked(bool)),  this,   SLOT(checkSqlLogin()));
    connect(MibPathPlus,    SIGNAL(clicked(bool)),  this,   SLOT(mibPathPlus()));
    connect(MibPathMinus,   SIGNAL(clicked(bool)),  this,   SLOT(mibPathMinus()));
    connect(SelectHome,     SIGNAL(clicked(bool)),  this,   SLOT(homeSelect()));

    homeDirLE->setText(qset.value(_sHomeDir, lanView::homeDefault).toString());
    sqlHostLE->setText(qset.value(_sSqlHost, _sLocalHost).toString());
    sqlPortSB->setValue(qset.value(_sSqlPort, 5432).toInt());
    sqlUserLE->setText(qset.value(_sSqlUser, _sLanView2) .toString());
    sqlPassLE->setText(qset.value(_sSqlPass).toString());
    dbNameLE-> setText(qset.value(_sDbName,  _sLanView2) .toString());

    QRegExp regExp("\\d+|0x[\\dA-Za-z]*");
    debugLevelLE->setValidator(new QRegExpValidator(regExp, debugLevelLE));
    debugLevelLE->setText(_sHex + QString::number(qset.value(_sDebugLevel, lanView::debugDefault).toLongLong(), 16));
    logFile = qset.value(_sLogFile, _sStdErr).toString();
    if (logFile == _sMinus || logFile == _sStdOut) {
        logFile = _sNul;
        logToStdOutRB->setChecked(true);
        logToStdOutClicked(true);
    }
    else if (logFile == _sStdErr) {
        logFile = _sNul;
        logToStdErrRB->setChecked(true);
        logToStdErrClicked(true);
    }
    else {
        logToFileRB->setChecked(true);
        logToFileClicked(true);
    }
    QStringList mibPathList = qset.value(_sMibPath).toString().split(_sColon);
    foreach (QString dir, mibPathList) {
        MibPathLS->addItem(new QListWidgetItem(dir, MibPathLS));
    }
    DBGFNL();
}

cSetupWidget::~cSetupWidget()
{
    PDEB(OBJECT) << "delete (cSetupWidget *)" << p2string(this) << endl;
    if (pLl != NULL) delete pLl;
    DBGFNL();
}
void cSetupWidget::applicate()
{
    DBGFN();
    bool ok = false;
    qset.setValue(_sHomeDir, homeDirLE->text());
    qset.setValue(_sSqlHost, sqlHostLE->text());
    qset.setValue(_sSqlPort, sqlPortSB->value());
    qset.setValue(_sSqlUser, sqlUserLE->text());
    qset.setValue(_sSqlPass, sqlPassLE->text());
    qset.setValue(_sDbName,  dbNameLE->text());
    qset.setValue(_sDebugLevel, debugLevelLE->text().toLongLong(&ok, 0));
    if (!ok) DERR() << "Invalid log level : " << logFileNameLE->text() << endl;
    qset.setValue(_sLogFile, logFileNameLE->text());
    int i, n = MibPathLS->count();
    QString mibPath;
    for (i = 0; i < n; i++) {
        if (mibPath.count()) mibPath += _sColon;
        mibPath += MibPathLS->item(i)->text();
    }
    qset.setValue(_sMibPath, mibPath);
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
        QMessageBox::warning(this, _titleError, err);
    }
    else QMessageBox::information(this, _titleInfo, trUtf8("New settings accepted."));
    DBGFNL();
}

void cSetupWidget::applicateAndRestart()
{
    applicate();
    appReStart();
}

void cSetupWidget::applicateAndClose()
{
    applicate();
    qApp->exit(0);
}

void cSetupWidget::logToStdOutClicked(bool __b)
{
    DBGFN();
    if (__b) {
        logFileNameLE->setEnabled(false);
        logFileNameLE->setText(_sStdOut);
    }
    DBGFNL();
}
void cSetupWidget::logToStdErrClicked(bool __b)
{
    DBGFN();
    if (__b) {
        logFileNameLE->setEnabled(false);
        logFileNameLE->setText(_sStdErr);
    }
    DBGFNL();
}
void cSetupWidget::logToFileClicked(bool __b)
{
    DBGFN();
    if (__b) {
        logFileNameLE->setEnabled(true);
        logFileNameLE->setText(logFile);
    }
    DBGFNL();
}
void cSetupWidget::logLevelMoreClicked()
{
    DBGFN();
    bool ok;
    qlonglong l = debugLevelLE->text().toLongLong(&ok, 0);
    if (!ok) DERR() << "Invalid log level : " << logFileNameLE->text() << endl;
    if (pLl == NULL) {
        pLl = new cLogLevelDialog(l, this);
        connect(pLl,   SIGNAL(accepted()),this, SLOT(setLogLevel()));
    }
    else pLl->setLogLevel(l);
    pLl->show();
    DBGFNL();
}

void cSetupWidget::setLogLevel(void)
{
    if (pLl == NULL) {
        DERR() << "Program error: cSetupWidget::pLl is NULL, ignored signal to " << __PRETTY_FUNCTION__ << " slot" << endl;
        return;
    }
    debugLevelLE->setText(_sHex + QString::number(pLl->getLogLevel(), 16));
}

void cSetupWidget::mibPathPlus()
{
    DBGFN();
    QString dir = QFileDialog::getExistingDirectory(this, trUtf8("Adja meg a MIB fájlok könyvtárát!"));
    if (dir.isEmpty() == false) {
        QList<QListWidgetItem *> l = MibPathLS->findItems(dir, Qt::MatchFixedString);
        if (l.count() == 0) {
            MibPathLS->addItem(new QListWidgetItem(dir, MibPathLS));
        }
    }
    DBGFNL();
}

void cSetupWidget::mibPathMinus()
{
    DBGFN();
    QList<QListWidgetItem *>    selected =  MibPathLS->selectedItems();
    if (selected.count()) {
        QListWidgetItem *   p;
        foreach (p, selected) {
            int row = MibPathLS->row(p);
            PDEB(INFO) << "Remove #" << row << " row from list..." << endl;
            // MibPathLS->removeItemWidget(p);
            if (p != MibPathLS->takeItem(row)) EXCEPTION(EPROGFAIL);
            delete p;
        }
    }
    else PDEB(WARNING) << "Not selected any row" << endl;
    DBGFNL();
    return;
}

void cSetupWidget::homeSelect()
{
    PDEB(VVERBOSE) << VDEBPTR(homeDirLE) << _sSColon << _sSpace << " == " << homeDirLE->text() << endl;
    homeDirLE->setText(QFileDialog::getExistingDirectory(this, tr("Alap könyvtár kiválasztása"), homeDirLE->text()));
    PDEB(VVERBOSE) << VDEBPTR(homeDirLE) << _sSColon << _sSpace << " == " << homeDirLE->text() << endl;
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

    checkBoxLv2->       setCheckState(checkState(__ll, cDebug::LV2));
    checkBoxLv2d->      setCheckState(checkState(__ll, cDebug::LV2D));
    checkBoxLv2gui->    setCheckState(checkState(__ll, cDebug::LV2GUI));
    checkBoxIcontsrv->  setCheckState(checkState(__ll, cDebug::ICONTSRV));
    checkBoxImport->    setCheckState(checkState(__ll, cDebug::IMPORT));
    checkBoxPortstat->  setCheckState(checkState(__ll, cDebug::PORTSTAT));
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

    checkState(checkBoxLv2->       checkState(), l, cDebug::LV2);
    checkState(checkBoxLv2d->      checkState(), l, cDebug::LV2D);
    checkState(checkBoxLv2gui->    checkState(), l, cDebug::LV2GUI);
    checkState(checkBoxIcontsrv->  checkState(), l, cDebug::ICONTSRV);
    checkState(checkBoxImport->    checkState(), l, cDebug::IMPORT);
    checkState(checkBoxPortstat->  checkState(), l, cDebug::PORTSTAT);

    return l;
}
void cLogLevelDialog::allOnClicked()  { setLogLevel(-1); }
void cLogLevelDialog::allOffClicked() { setLogLevel(0);  }


QSqlDatabase * cSetupWidget::SqlOpen()
{
    _DBGFNL() << _sSpace << _sComma << VDEBPTR(this) << endl;
    QSqlDatabase *pDb = new  QSqlDatabase(QSqlDatabase::addDatabase(_sQPSql));
    if (!pDb->isValid()) {
        QSqlError le = pDb->lastError();
        QString msg = QString("SQL DB ERROR #") + QString::number(le.number()) + "\n"
                    + "driverText   : " + le.driverText() + "\n"
                    + "databaseText : " + le.databaseText();
        QMessageBox::warning(this,_titleError, msg);
        delete pDb;
        return NULL;
    }
    pDb->setHostName(   sqlHostLE->text());
    pDb->setPort(       sqlPortSB->value());
    pDb->setUserName(   sqlUserLE->text());
    pDb->setPassword(   sqlPassLE->text());
    pDb->setDatabaseName(dbNameLE->text());
    if (!pDb->open()) {
        QSqlError le = pDb->lastError();
        QString msg = QString("SQL open ERROR #") + QString::number(le.number()) + "\n"
                    + "driverText   : " + le.driverText() + "\n"
                    + "databaseText : " + le.databaseText();
        QMessageBox::warning(this, _titleError, msg);
        delete pDb;
        return NULL;
    }
    return pDb;
}

void cSetupWidget::checkSqlLogin()
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << _sSpace << _sComma << VDEBPTR(this) << endl;
    QSqlDatabase *pDb = SqlOpen();
    if (pDb == NULL) return;
    QMessageBox::information(this, _titleInfo, trUtf8("Database open is successful."));
    pDb->close();
    delete pDb;
}
