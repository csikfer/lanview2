#include "gparse.h"
#include "srvdata.h"
#include "import_parser.h"
#include "cerrormessagebox.h"
#include "lv2widgets.h"

#if defined(Q_CC_GNU)
#include <unistd.h>
#endif

const enum ePrivilegeLevel cParseWidget::rights = PL_OPERATOR;

cParseWidget::cParseWidget(QMdiArea *par)
: cIntSubObj(par)
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    pq  = newQuery();
    isRuning = false;
    pLocalParser = NULL;
    pLocalParsedStr = NULL;
    pUi = new Ui::GParseWidget();
    pUi->setupUi(this);

    pUi->labelQP->hide();
    pUi->pushButtonClearQP->hide();
    pUi->pushButtonLoadQP->hide();
    pUi->pushButtonParseQP->hide();
    pUi->pushButtonSaveQP->hide();
    pUi->textEditQP->hide();
    static const QString sql =
            "SELECT DISTINCT(service_name) FROM query_parsers JOIN services USING (service_id) WHERE parse_type = 'parse'";
    if (execSql(*pq, sql)) {
        do { qParseList << pq->value(0).toString(); } while (pq->next());
        pUi->comboBoxQP->addItems(qParseList);
    }
    else {
        pUi->checkBoxQP->setDisabled(true);
    }

    setParams();

    connect(pUi->pushButtonLoad,  SIGNAL(clicked()), this, SLOT(loadClicked()));
    connect(pUi->pushButtonSave,  SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(pUi->pushButtonParse, SIGNAL(clicked()), this, SLOT(parseClicked()));
    connect(pUi->pushButtonClose, SIGNAL(clicked()), this, SLOT(endIt()));
    connect(pUi->pushButtonBreak, SIGNAL(clicked()), this, SLOT(localParseBreak()));

    connect(pUi->pushButtonLoadQP,  SIGNAL(clicked()), this, SLOT(loadQPClicked()));
    connect(pUi->pushButtonSaveQP,  SIGNAL(clicked()), this, SLOT(saveQPClicked()));
    connect(pUi->pushButtonParseQP, SIGNAL(clicked()), this, SLOT(parseQPClicked()));
    connect(pUi->pushButtonMap,     SIGNAL(clicked()), this, SLOT(paramClicked()));

    DBGFNL();
}

cParseWidget::~cParseWidget()
{
    PDEB(OBJECT) << "delete (cParseWidget *)" << p2string(this) << endl;
    DBGFNL();
}

void cParseWidget::loadClicked()
{
    QString text;
    if (textFromFile(fileName, text, this)) {
        pUi->textEditSrc->setText(text);
    }
}

void cParseWidget::saveClicked()
{
    textToFile(fileName, pUi->textEditSrc->toPlainText(), this);
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

    imp.insert(*pq);
    QString msg = trUtf8("Végrehajtandó forrásszöveg kiírva az adatbázisba (ID = %1)\nVárakozás...").arg(imp.getId());
    sqlNotify(*pq, "import");
    pUi->pushButtonBreak->setEnabled(true);
    int lastStat = ES_WAIT;
    while (true) {
        if (msg.isEmpty() == false) {
            pUi->textEditResult->clear();
            pUi->textEditResult->setText(msg);
            msg.clear();
        }
        QThread::sleep(1);
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
    pUi->pushButtonBreak->setEnabled(false);
}

void cParseWidget::setParams()
{
    cNode *pNode = lanView::getInstance()->pSelfNode;
    const QString node = pNode == NULL ? _sNil : pNode->getName();
    static const QString _sHostService = "host_service";
    params[_sHostService]   = _sNul;
    params[_sService]       = "import";
    params[_sNode]          = node;
    params[_sHost]          = node;
    params[_sInterface]     = _sNul;
    params["selfName"]      = node;
    params[_sAddress]       = pNode == NULL ? _sNul : pNode->getIpAddress().toString();
    params[_sProtocol]      = _sNil;
    params[_sPort]          = _sNul;
    params[_sHostServiceId] = _sNULL;
    params[_sNodeId]        = pNode == NULL ? _sNULL : QString::number(pNode->getId());
    params[_sServiceId]     = "0";
}

/// A debug rendszertől jött sorokat írja ki feltételesen a textEditResult nevű QTextEdit widgetre
/// Az üzenetek elejéről leválasztja a maszk-ot, és csak azokat az üzeneteket irja a widget-be,
/// melyek hiba, figyelmeztető, vagy információs üzenetek.
void cParseWidget::debugLine()
{
    QString s = cDebug::getInstance()->dequeue();
    QRegExp  re("^([\\da-f]{8})\\s(.+)$");
    if (re.exactMatch(s)) {
        bool ok;
        QString sm = re.cap(1);
        qlonglong m = sm.toULongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL);
        if (m & (cDebug::INFO | cDebug::WARNING | cDebug::DERROR)) {
            s = re.cap(2).trimmed();
            pUi->textEditResult->append(s);
        }
    }
}

void cParseWidget::loadQPClicked()
{
    QString text;
    if (textFromFile(fileNamePQ, text, this)) {
        pUi->textEditQP->setText(text);
    }
}

void cParseWidget::saveQPClicked()
{
    textToFile(fileNamePQ, pUi->textEditQP->toPlainText(), this);
}

void cParseWidget::parseQPClicked()
{
    cError *pe = NULL;
    cQueryParser qp;
    try {
        qlonglong sid = cService::service(*pq, pUi->comboBoxQP->currentText())->getId();
        qp.load(*pq, sid, false, false);
        QString text = pUi->textEditQP->toPlainText();
        qp.setMaps(&params);
        qp.prep(pe);
        if (pe == NULL) {
            foreach (QString line, text.split('\n')) {
                qp.parse(line.trimmed(), pe);
                if (pe != NULL) break;
            }
            if (pe == NULL) {
                qp.post(pe);
            }
        }
    } CATCHS(pe);
    pUi->textEditSrc->setPlainText(qp.getText());
    if (pe != NULL) {
        cErrorMessageBox::messageBox(pe, this);
        pDelete(pe);
    }
}

void cParseWidget::paramClicked()
{
    cStringMapEdit d(true, params, this);
    d.dialog().exec();
}
