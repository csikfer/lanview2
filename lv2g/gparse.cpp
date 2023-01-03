#include "gparse.h"
#include "srvdata.h"
#include "import_parser.h"
#include "cerrormessagebox.h"
#include "lv2widgets.h"
#include "popupreport.h"

#if defined(Q_CC_GNU)
#include <unistd.h>
#endif

const enum ePrivilegeLevel cParseWidget::rights = PL_OPERATOR;

cParseWidget::cParseWidget(QMdiArea *par)
: cIntSubObj(par)
{
    PDEB(OBJECT) << __PRETTY_FUNCTION__ << QChar(' ') << QChar(',') << VDEBPTR(this) << endl;
    pExportQueue = nullptr;
    pq  = newQuery();
    isRuning = false;
    pLocalParser = nullptr;
    pLocalParsedStr = nullptr;
    pUi = new Ui::GParseWidget();
    pUi->setupUi(this);

    pHSListModel = new cRecordListModel(cHostService().descr(), this);
    static const QString whereHS = QString("service_id = %1").arg(cService::service(*pq, _sImport)->getId());
    pHSListModel->setConstFilter(whereHS, FT_SQL_WHERE);
    pHSListModel->joinWith(pUi->comboBoxTarget);
    pHSListModel->setDecorateByField(_sHostServiceState, ENUM2SET(FF_BG_COLOR));
    pHSListModel->setFilter();  // refresh
    pUi->comboBoxTarget->setCurrentIndex(0);
    pUi->radioButtonServer->setCheckable(pHSListModel->rowCount() > 0);

    pUi->labelQP->hide();
    pUi->pushButtonClearQP->hide();
    pUi->pushButtonLoadQP->hide();
    pUi->pushButtonParseQP->hide();
    pUi->pushButtonSaveQP->hide();
    pUi->textEditQP->hide();
    QList<int> sizes;
    sizes << 0 << 100 << 100;
    pUi->splitterMain->setSizes(sizes);

    static const QString whereS = QString("0 < (SELECT COUNT(*) FROM query_parsers WHERE query_parsers.service_id = services.service_id)");
    pSListModel = new cRecordListModel(cService().descr(), this);
    pSListModel->setConstFilter(whereS, FT_SQL_WHERE);
    pSListModel->joinWith(pUi->comboBoxQP);
    pSListModel->setFilter();
    pUi->checkBoxQP->setDisabled(pSListModel->rowCount() == 0);

    setParams();

    connect(pUi->pushButtonLoad,  SIGNAL(clicked()), this, SLOT(loadClicked()));
    connect(pUi->pushButtonSave,  SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(pUi->pushButtonParse, SIGNAL(clicked()), this, SLOT(parseClicked()));
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
    if (cFileDialog::textFromFile(fileName, text, this)) {
        pUi->textEditSrc->setText(text);
    }
}

void cParseWidget::saveClicked()
{
    cFileDialog::textToFile(fileName, pUi->textEditSrc->toPlainText(), this);
}

void cParseWidget::parseClicked()
{
    QString src = pUi->textEditSrc->toPlainText();
    if (src.simplified().isEmpty()) {
        pUi->textEditLog->setText(tr("Üres szöveget adott át feldolgozásra, nincs művelet."));
        return;
    }
    if (pUi->radioButtonLocal->isChecked()) localParse(src);
    else                                    remoteParse(src);
}

void cParseWidget::localParse(const QString& src)
{
    pUi->pushButtonParse->setDisabled(true);
    pUi->pushButtonClear->setDisabled(true);
    pUi->pushButtonLoad->setDisabled(true);
    pUi->textEditSrc->setReadOnly(true);
    pUi->pushButtonBreak->setDisabled(false);

    pUi->textEditResult->clear();
    pUi->textEditLog->clear();

    pLocalError = nullptr;
    debugStream *pDS = cDebug::getInstance()->pCout();
    connect(pDS, SIGNAL(readyDebugLine()), this, SLOT(debugLine()));
    cDebug::getInstance()->setGui();
    pExportQueue = cExportQueue::init(true);
    connect(pExportQueue, SIGNAL(ready()), this, SLOT(exportLine()));

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
    if (pLocalError == nullptr) {
        pUi->textEditLog->append(tr("<p><b> O.K."));
    }
    else {
        pUi->textEditLog->append(tr("<p><b> A fordító kizárást dobott. <p> %1.").arg(pLocalError->msg()));
        cErrorMessageBox::messageBox(pLocalError, this);
        pDelete(pLocalError);
    }
    pDelete(pLocalParser);
    pDelete(pLocalParsedStr);

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
    imp.setId(_sTargetId, pHSListModel->currendId());
    imp.setName(_sImportText, src);
    imp.setName(_sAppName, lanView::appName);
    imp.setId(_sUserId, lanView::user().getId());
    cNode *pSelfNode = lanView::getInstance()->pSelfNode;
    qlonglong selfNodeId = pSelfNode == nullptr ? NIL_NODE_ID : pSelfNode->getId();
    imp.setId(_sNodeId, selfNodeId);

    imp.insert(*pq);
    QString msg = tr("Végrehajtandó forrásszöveg kiírva az adatbázisba (ID = %1)\nVárakozás...\n").arg(imp.getId());
    sqlNotify(*pq, "import", QString::number(pHSListModel->currendId()));
    pUi->pushButtonBreak->setEnabled(true);
    int lastStat = ES_WAIT;
    QBitArray mapStat  = imp.mask(_sExecState);
    QBitArray mapOther = imp.mask(_sStarted, _sEnded, _sResultMsg, _sAppLogId) | imp.mask(_sOutMsg, _sExpMsg);
    QEventLoop  *pLoop = new QEventLoop;
    connect(pUi->pushButtonBreak, &QPushButton::clicked, [=] () {
        pLoop->exit(1);
    } );
    while (true) {
        if (msg.isEmpty() == false) {
            pUi->textEditLog->clear();
            pUi->textEditLog->setText(msg);
            msg.clear();
        }
        QTimer::singleShot(1000, pLoop, &QEventLoop::quit);
        int r = pLoop->exec();

        // Polling : Csak az állpotot olvassuk vissza
        if (!imp.fetchFieldsById(*pq, mapStat)) {
            msg = tr("A kiírt imports rekord állapotot nem tudom visszaolvasni (ID = %1).\n").arg(imp.getId());
            break;
        }
        int stat = int(imp.getId(_sExecState));
        if (stat == lastStat) continue;
        lastStat = stat;
        switch (stat) {
        case ES_EXECUTE:
            msg = tr("Végrehajtás alatt (ID = %1, PID = %2).").arg(imp.getId()).arg(imp.getId(_sPid));
            if (!imp.isNull(_sStarted)) msg += tr("Kezdete : %1\n").arg(imp.getName(_sStarted));
            if (r == 0) continue;
            // BREAK
            msg = tr("A válaszra való várakozás megszakítva. Az elküldött forrás szöveg értelmezését ez nem állítja meg.\n");
            break;
        case ES_OK:
        case ES_FAILE:
        case ES_ABORTED:
            // Vége, kell a többi adat is
            if (!imp.fetchFieldsById(*pq, mapOther)) {
                msg = tr("A kiírt imports rekordot nem tudom visszaolvasni (ID = %1).\n").arg(imp.getId());
            }
            else {
                msg = tr("Végrehajtás eredménye : %1\n").arg(execState(stat));
                if (!imp.isNull(_sStarted))   msg += tr("Kezdete : %1\n").arg(imp.getName(_sStarted));
                if (!imp.isNull(_sEnded))     msg += tr("Vége : %1\n").arg(imp.getName(_sEnded));
                if (!imp.isNull(_sResultMsg)) msg += tr("Az értelmező üzenete : %1\n").arg(imp.getName(_sResultMsg));
                if (!imp.isNull(_sAppLogId))  msg += tr("APP. log ID : /1").arg(imp.getId(_sAppLogId));
            }
            break;
        case ES_WAIT:       // Elvileg képtelenség
            EXCEPTION(EPROGFAIL);
        default:
            msg = tr("A visszaolvasott állapotjelző értelmezhetetlen: %1").arg(stat);
            break;
        }
        break;
    }
    pDelete(pLoop);
    pUi->textEditResult->clear();
    //pUi->textEditResult->setText(...);
    pUi->textEditLog->clear();
    pUi->textEditLog->setText(msg + imp.getName(_sOutMsg));
    pUi->textEditResult->setText(imp.getName(_sExpMsg));
    pUi->pushButtonBreak->setEnabled(false);
}

void cParseWidget::setParams()
{
    cNode *pNode = lanView::getInstance()->pSelfNode;
    const QString node = pNode == nullptr ? _sNil : pNode->getName();
    static const QString _sHostService = "host_service";
    params[_sHostService]   = _sNul;
    params[_sService]       = "import";
    params[_sNode]          = node;
    params[_sHost]          = node;
    params[_sInterface]     = _sNul;
    params["selfName"]      = node;
    params[_sAddress]       = pNode == nullptr ? _sNul : pNode->getIpAddress().toString();
    params[_sProtocol]      = _sNil;
    params[_sPort]          = _sNul;
    params[_sHostServiceId] = _sNULL;
    params[_sNodeId]        = pNode == nullptr ? _sNULL : QString::number(pNode->getId());
    params[_sServiceId]     = "0";
}

/// A debug rendszertől jött sorokat írja ki feltételesen a textEditResult nevű QTextEdit widgetre
/// Az üzenetek elejéről leválasztja a maszk-ot, és csak azokat az üzeneteket irja a widget-be,
/// melyek hiba, figyelmeztető, vagy információs üzenetek.
void cParseWidget::debugLine()
{
    QString s = cDebug::getInstance()->dequeue();
    static const QRegularExpression re(QRegularExpression::anchoredPattern(_sDebugLinePattern));
    QRegularExpressionMatch ma;
    if ((ma = re.match(s)).hasMatch()) {
        bool ok;
        QString sm = ma.captured(1);
        qulonglong m = sm.toULongLong(&ok, 16);
        if (!ok) EXCEPTION(EPROGFAIL);
        if (m & (cDebug::INFO | cDebug::WARNING | cDebug::DERROR)) {
            s = ma.captured(2).trimmed();
            pUi->textEditLog->append(s);
        }
    }
}

void cParseWidget::exportLine()
{
    QString s = pExportQueue->pop();
    if (s.isEmpty()) return;
    pUi->textEditResult->append(s);
}


void cParseWidget::loadQPClicked()
{
    QString text;
    if (cFileDialog::textFromFile(fileNamePQ, text, this)) {
        pUi->textEditQP->setText(text);
    }
}

void cParseWidget::saveQPClicked()
{
    cFileDialog::textToFile(fileNamePQ, pUi->textEditQP->toPlainText(), this);
}

void cParseWidget::parseQPClicked()
{
    cError *pe = nullptr;
    cQueryParser qp;
    try {
        qlonglong sid = cService::service(*pq, pUi->comboBoxQP->currentText())->getId();
        qp.load(*pq, sid, false, false);
        QString text = pUi->textEditQP->toPlainText();
        qp.setMaps(&params);
        qp.prep(pe);
        if (pe == nullptr) {
            foreach (QString line, text.split('\n')) {
                qp.captureAndParse(line.trimmed(), pe);
                if (pe != nullptr) break;
            }
            if (pe == nullptr) {
                qp.post(pe);
            }
        }
    } CATCHS(pe);
    pUi->textEditSrc->setPlainText(qp.getCommands());
    if (pe != nullptr) {
        cErrorMessageBox::messageBox(pe, this);
        pDelete(pe);
    }
}

void cParseWidget::paramClicked()
{
    cStringMapEdit d(true, params, this);
    d.dialog().exec();
}

void cParseWidget::on_checkBoxQP_toggled(bool checked)
{
    QList<int> sizes;
    sizes << (checked ? 100 : 0) << 100 << 100;
    pUi->splitterMain->setSizes(sizes);
}

void cParseWidget::on_pushButtonRepSave_clicked()
{
    cFileDialog::textEditToFile(fileName, pUi->textEditResult, this);
}

void cParseWidget::on_pushButtonLogSave_clicked()
{
    cFileDialog::textEditToFile(fileName, pUi->textEditLog, this);
}
