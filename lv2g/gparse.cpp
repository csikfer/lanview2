#include "gparse.h"
#include "import_parser.h"
#include "cerrormessagebox.h"


cParseWidget::cParseWidget(QWidget *par)
: cOwnTab(par)
, fileFilter(trUtf8("Szöveg fájlok (*.txt *.src *.imp)"))
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    pUi = new Ui::GParseWidget();
    pUi->setupUi(this);

    connect(pUi->pushButtonLoad,  SIGNAL(clicked()), this, SLOT(loadClicked()));
    connect(pUi->pushButtonSave,  SIGNAL(clicked()), this, SLOT(saveCliecked()));
    connect(pUi->pushButtonParse, SIGNAL(clicked()), this, SLOT(parseClicked()));
    connect(pUi->pushButtonClose, SIGNAL(clicked()), this, SLOT(endIt()));
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
    pUi->textEditResult->clear();
    cError *pe = NULL;
    debugStream *pDS = NULL;
    cDebug      *pD  = NULL;
    QSqlQuery   *pq  = newQuery();
    bool         transaction = false;
    try {
        pD  = cDebug::getInstance();
        pDS = pD->pCout();
        connect(pDS, SIGNAL(readyDebugLine()), this, SLOT(debugLine()));
        pD->setGui();
        transaction = sqlBegin(*pq);
        importParseText(src);
        transaction = !sqlEnd(*pq);
        delete pq;
        pq = NULL;
        pD->setGui(false);
        disconnect(pDS, SIGNAL(readyDebugLine()), this, SLOT(debugLine()));
    } CATCHS(pe);
    if (importLastError != NULL) {
        if (pe != NULL) {
            pUi->textEditResult->append(trUtf8("<p><b> Tbbszörös hiba. <p> %1.").arg(pe->msg()));
            delete pe;
        }
        pe = importLastError;
        importLastError = NULL;
    }
    if (pe != NULL) {
        if (pq != NULL) {
            if (transaction) sqlRollback(*pq);
            delete pq;
            pq = NULL;
        }
        if (pD != NULL) {
            pD->setGui(false);
            if (pDS != NULL) {
                disconnect(cDebug::getInstance()->pCout(), SIGNAL(readyDebugLine()), this, SLOT(debugLine()));
            }
            pUi->textEditResult->append(trUtf8("<p><b> A fordító kizárást dobott. <p> %1.").arg(pe->msg()));
            cErrorMessageBox::messageBox(pe, this);
        }
        delete pe;
    }
    else {
        pUi->textEditResult->append(trUtf8("<p><b> O.K."));
    }
}

void cParseWidget::remoteParse(const QString &src)
{
    cImport imp;
    imp.setName(_sImportText, src);
    imp.setName(_sAppName, lanView::appName);
    imp.setId(_sUserId, lanView::user().getId());

    QSqlQuery   *pq  = newQuery();
    imp.insert(*pq);
    QString msg = trUtf8("Végrehajtandó forrásszöveg kiírva az adatbázisba (ID = %1)\nVárakozás...").arg(imp.getId());
    int lastStat = ES_WAIT;
    while (true) {
        if (msg.isEmpty() == false) {
            pUi->textEditResult->clear();
            pUi->textEditResult->setText(msg);
            msg.clear();
        }
        sleep(1);
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
