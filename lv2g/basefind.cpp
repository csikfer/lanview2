#include "basefind.h"
#include "scan.h"

#include "ui_basefind.h"


cBaseFindDiscover::cBaseFindDiscover(const cMac &_mac, const QHostAddress &_ip, cSnmpDevice& _st, cBaseFind * _par)
    : QThread(_par), pEQ(nullptr), mac(_mac), ip(_ip), st(_st)
{
    ;
}

void cBaseFindDiscover::run()
{
    pEQ = cExportQueue::init(false);
    connect(pEQ, SIGNAL(ready()), this, SLOT(readyLine()));
    exploreByAddress(mac, ip, st);
    QString s;
    while (!(s = pEQ->pop()).isEmpty()) emit expLine(s);
}

void cBaseFindDiscover::readyLine()
{
    emit expLine(pEQ->pop());
}

static const QString _sNMap = "nmap";

cPopUpNMap::cPopUpNMap(QWidget *par, const QString& sIp)
    : cPopupReportWindow(par, tr("Start nmap..."), tr("%1 nmap report").arg(sIp))
    , process(new QProcess(this))
{
    pButtonSave->setDisabled(true);
    QStringList args("-A");
    args << sIp;
    connect(process, SIGNAL(started()),     this, SLOT(processStarted()));
    connect(process, SIGNAL(finished(int)), this, SLOT(processFinished(int)));
    connect(process, SIGNAL(readyRead()),   this, SLOT(processReadyRead()));
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
    first = true;
    process->start(_sNMap, args);
}

void cPopUpNMap::processStarted()
{
    pTextEdit->setText(tr("nmap started ..."));
}

void cPopUpNMap::processFinished(int ec)
{
    if (ec == 0) {
        pTextEdit->append(htmlGrInf(_sOk));
    }
    else {
        pTextEdit->append(htmlError(tr("Exit code : %1").arg(ec)));
    }
    pButtonSave->setEnabled(true);
}

void cPopUpNMap::processReadyRead()
{
    QString out;
    out = process->readAllStandardOutput();
    if (!out.isEmpty()) {
        static QChar cSp(' ');
        static QChar cNl('\n');
        QStringList lines = out.split(cNl);
        foreach (QString line, lines) {
            if (line.isEmpty()) {
                if (!first) {
                    text += sHtmlBr;
                }
            }
            else {
                int n = line.indexOf(cSp);
                if (n >= 0 && n < 8) {
                    QString s;
                    if (n > 0) {
                        s = line.mid(0, n);
                        line.remove(0, n);
                    }
                    do {
                        s += sHtmlNbsp;
                        line = line.remove(0,1);
                    } while (line.startsWith(cSp));
                    line = s + toHtml(line);
                }
                text += line + sHtmlBr;
            }
            first = false;
        }
    }
    out = process->readAllStandardError();
    if (!out.isEmpty()) text += htmlError(out, true);
    pTextEdit->setHtml(text);
    resizeByText();
    first = false;
    // PDEB(VERBOSE) << "Text : \n" << text << endl;
}

void cPopUpNMap::processError(QProcess::ProcessError e)
{
    QString text;
    text = ProcessError2Message(e);
    pTextEdit->append(htmlError(text, true));
}

const enum ePrivilegeLevel cBaseFind::rights = PL_VIEWER;
eTristate cBaseFind::nmapExists = TS_NULL;

cBaseFind::cBaseFind(QMdiArea *parent) :
    cIntSubObj(parent),
    pUi(new Ui::BaseFind)
{
    pq = newQuery();
    pThread = nullptr;
    fMAC = fIP = fSw = false;
    pUi->setupUi(this);
    // RDP Windows-on, ha ez egy RDP kliens
#if defined(Q_OS_WINDOWS)
    rdpClientAddr = rdpClientAddress();
#endif  // defined(Q_OS_WINDOWS)

#ifdef SNMP_IS_EXISTS
    setSwitchs();
#else   // SNMP_IS_EXISTS
    fSw = false;
#endif  // SNMP_IS_EXISTS
    if (nmapExists == TS_NULL) {
        QProcess    nmapTest;
        nmapTest.start(_sNMap, QStringList("-V"));
        if (!nmapTest.waitForFinished(5000)) nmapTest.kill();
        nmapExists = nmapTest.exitCode() == 0 ? TS_TRUE : TS_FALSE;
    }
    connect(pUi->pushButtonSave,  SIGNAL(clicked()), this, SLOT(save_clicked()));
    connect(pUi->pushButtonClear, SIGNAL(clicked()), this, SLOT(clear_clicked()));
}

cBaseFind::~cBaseFind()
{
    delete pq;
}

void cBaseFind::setSwitchs()
{
#ifdef SNMP_IS_EXISTS
    static const QString sql = "SELECT node_name FROM snmpdevices WHERE 'switch' = ANY (node_type) ORDER BY node_name";
    QString centralSwitchName = cSysParam::getTextSysParam(*pq, "central_switch_name");
    centralSwitchIndex = 0;
    if (execSql(*pq, sql)) do {
        // pUi->comboBox->addItem(pq->value(0).toString()); // SLOW !!
        QString switchName = pq->value(0).toString();
        if (switchName == centralSwitchName) centralSwitchIndex = switchItems.size();
        switchItems << switchName;
    } while (pq->next());
    fSw = !switchItems.isEmpty();
#endif
}

void cBaseFind::appendReport(const QString& s)
{
    if (!sReport.isEmpty()) sReport += sHtmlBr;
    sReport += s;
    pUi->textEdit->setHtml(sHtmlHead + sReport + sHtmlTail);
}

void cBaseFind::discover(const QHostAddress& ip, const cMac& mac, const QString& swName)
{
#ifdef SNMP_IS_EXISTS
    QSqlQuery q = getQuery();
    cSnmpDevice sw;
    if (ip.isNull() || !mac.isValid() || !sw.fetchByName(q, swName)) {
        appendReport(htmlError(tr("Hibás adatok.")));
        return;
    }
    pThread = new cBaseFindDiscover(mac, ip, sw, this);
    connect(pThread, SIGNAL(expLine(QString)), this, SLOT(expLine(QString)));
    connect(pThread, SIGNAL(finished()), this, SLOT(finished()));
    pUi->pushButtonClear->setDisabled(true);
    pUi->pushButtonSave->setDisabled(true);
    pThread->start();
#endif
}

void cBaseFind::expLine(QString s)
{
    appendReport(s);
}

void cBaseFind::finished()
{
    pUi->pushButtonClear->setDisabled(false);
    pUi->pushButtonSave->setDisabled(false);
}

void cBaseFind::NMap(const QString& sIp)
{
    new cPopUpNMap(this, sIp);
}

void cBaseFind::save_clicked()
{
    static QString fileName;
    cFileDialog::textEditToFile(fileName, pUi->textEdit, this);
}

void cBaseFind::clear_clicked()
{
    pUi->textEdit->clear();
    sReport.clear();
    clearPostWork();
}
