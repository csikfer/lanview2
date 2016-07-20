#include "gparse.h"
#include "import_parser.h"
#include "cerrormessagebox.h"

const enum ePrivilegeLevel cParseWidget::rights = PL_OPERATOR;

cParseWidget::cParseWidget(QWidget *par)
: cOwnTab(par)
, fileFilter(trUtf8("Szöveg fájlok (*.txt *.src *.imp)"))
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    isRuning = false;
    pLocalParser = NULL;
    pLocalParsedStr = NULL;
    pUi = new Ui::GParseWidget();
    pUi->setupUi(this);

    connect(pUi->pushButtonLoad,  SIGNAL(clicked()), this, SLOT(loadClicked()));
    connect(pUi->pushButtonSave,  SIGNAL(clicked()), this, SLOT(saveCliecked()));
    connect(pUi->pushButtonParse, SIGNAL(clicked()), this, SLOT(parseClicked()));
    connect(pUi->pushButtonClose, SIGNAL(clicked()), this, SLOT(endIt()));
    connect(pUi->pushButtonBreak, SIGNAL(clicked()), this, SLOT(localParseBreak()));
    DBGFNL();
}

cParseWidget::~cParseWidget()
{
    PDEB(OBJECT) << "delete (cParseWidget *)" << p2string(this) << endl;
    DBGFNL();
}

QString cParseWidget::dirName()
{
    if (fileName.isEmpty()) return lanView::getInstance()->homeDir;
    QFileInfo   fi(fileName);
    return fi.dir().dirName();
}

void cParseWidget::loadClicked()
{
    QString fn;
    const QString& capt = trUtf8("Forrás fájl kiválasztása");
    fn = QFileDialog::getOpenFileName(this, capt, dirName(), fileFilter);
    if (fn.isEmpty()) return;
    fileName = fn;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString msg = trUtf8("A %1 forrás fájl nem nyitható meg.\n%2").arg(fileName).arg(file.errorString());
        QMessageBox::warning(this, trUtf8("Hiba"), msg);
        return;
    }
    pUi->textEditSrc->setText(QString::fromUtf8(file.readAll()));
}

void cParseWidget::saveCliecked()
{
    QString fn;
    const QString& capt = trUtf8("Cél fájl kiválasztása");
    fn = QFileDialog::getSaveFileName(this, capt, dirName(), fileFilter);
    if (fn.isEmpty()) return;
    fileName = fn;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        const QString msg = trUtf8("A %1 cél fájl nem nyitható meg.\n%2").arg(fileName).arg(file.errorString());
        QMessageBox::warning(this, trUtf8("Hiba"), msg);
        return;
    }
    file.write(pUi->textEditSrc->toPlainText().toUtf8());
}

void cParseWidget::parseClicked()
{
    QString src = pUi->textEditSrc->toPlainText();
    if (src.simplified().isEmpty()) {
        pUi->textEditResult->setText(trUtf8("Üres szöveget adott át feldolgozásra, nincs művelet."));
        return;
    }
    if (pUi->radioButtonLocal->isChecked()) localParse(src);
    else                                    remoteParse(src);
}

void cParseWidget::localParse(const QString& src)
{
    pUi->pushButtonClose->setDisabled(true);
    pUi->pushButtonParse->setDisabled(true);
    pUi->pushButtonClear->setDisabled(true);
    pUi->pushButtonLoad->setDisabled(true);
    pUi->textEditSrc->setReadOnly(true);
    pUi->pushButtonBreak->setDisabled(false);

    pUi->textEditResult->clear();

    pLocalError = NULL;
    debugStream *pDS = cDebug::getInstance()->pCout();
    connect(pDS, SIGNAL(readyDebugLine()), this, SLOT(debugLine()));
    cDebug::getInstance()->setGui();

    pLocalParser = new cImportParseThread(_sNul, pWidget());

    connect(pLocalParser, SIGNAL(finished()), this, SLOT(localParseFinished()));
    pLocalParsedStr = new QString(src);
    pLocalParser->startParser(pLocalError, pLocalParsedStr);   // A hiba pointert ebben a kontexusban csk kinullázza
}

void cParseWidget::localParseFinished()
{
    // Az esetleges hiba kód
    pLocalError = importGetLastError();
    // A debug kimenet leválasztása a GUI-ról.
    debugStream *pDS = cDebug::getInstance()->pCout();
    disconnect(pDS, SIGNAL(readyDebugLine()), this, SLOT(debugLine()));
    cDebug::getInstance()->setGui(false);
    // OK ?
    if (pLocalError == NULL) {
        pUi->textEditResult->append(trUtf8("<p><b> O.K."));
    }
    else {
        pUi->textEditResult->append(trUtf8("<p><b> A fordító kizárást dobott. <p> %1.").arg(pLocalError->msg()));
        cErrorMessageBox::messageBox(pLocalError, this);
        pDelete(pLocalError);
    }
    pDelete(pLocalParser);
    pDelete(pLocalParsedStr);

    pUi->pushButtonClose->setDisabled(false);
    pUi->pushButtonParse->setDisabled(false);
    pUi->pushButtonClear->setDisabled(false);
    pUi->pushButtonLoad->setDisabled(false);
    pUi->textEditSrc->setReadOnly(false);
    pUi->pushButtonBreak->setDisabled(true);
}

void cParseWidget::localParseBreak()
{
    breakImportParser();
    pUi->pushButtonBreak->setDisabled(true);
}

void cParseWidget::remoteParse(const QString &src)
{
    cImport imp;
    imp.setName(_sImportText, src);
    imp.setName(_sAppName, lanView::appName);
    imp.setId(_sUserId, lanView::user().getId());
    imp.setId(_sNodeId, lanView::selfNode().getId());

    QSqlQuery   *pq  = newQuery();
    imp.insert(*pq);
    QString msg = trUtf8("Végrehajtandó forrásszöveg kiírva az adatbázisba (ID = %1)\nVárakozás...").arg(imp.getId());
    sqlNotify(*pq, "import");
    int lastStat = ES_WAIT;
    while (true) {
        if (msg.isEmpty() == false) {
            pUi->textEditResult->clear();
            pUi->textEditResult->setText(msg);
            msg.clear();
        }
#if   defined(Q_CC_MSVC)
        Sleep(1000);
#elif defined(Q_CC_GNU)
        sleep(1);
#endif
        if (!imp.fetchById(*pq)) {
            msg = trUtf8("A kiírt imports rekordot nem tudom visszaolvasni (ID = %1).").arg(imp.getId());
            break;
        }
        int stat = imp.getId(_sExecState);
        if (stat == lastStat) continue;
        lastStat = stat;
        switch (stat) {
        case ES_EXECUTE:
            msg = trUtf8("Végrehajtás alatt (ID = %1, PID = %2).").arg(imp.getId()).arg(imp.getId(_sPid));
            if (!imp.isNull(_sStarted)) msg += trUtf8("Kezdete : %1\n").arg(imp.getName(_sStarted));
            continue;
        case ES_OK:
        case ES_FAILE:
        case ES_ABORTED:
            msg = trUtf8("Végrehajtás eredménye : %1\n").arg(execState(stat));
            if (!imp.isNull(_sStarted)) msg += trUtf8("Kezdete : %1\n").arg(imp.getName(_sStarted));
            if (!imp.isNull(_sEnded))   msg += trUtf8("Vége : %1\n").arg(imp.getName(_sEnded));
            if (!imp.isNull(_sResultMsg)) msg += trUtf8("Az értelmező üzenete : %1\n").arg(imp.getName(_sResultMsg));
            // if (!imp.isNull(_sAppLogId)) ...
            break;
        case ES_WAIT:       // Elvileg képtelenség
            EXCEPTION(EPROGFAIL);
        default:
            msg = trUtf8("A visszaolvasott állapotjelző értelmezhetetlen: %1").arg(stat);
            break;
        }
        break;
    }
    pUi->textEditResult->clear();
    pUi->textEditResult->setText(msg);
}

void cParseWidget::debugLine()
{
    pUi->textEditResult->append(cDebug::getInstance()->dequeue());
}
