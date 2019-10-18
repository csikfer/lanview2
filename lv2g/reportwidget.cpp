#include "reportwidget.h"
#include "ui_reportwidget.h"
#include "popupreport.h"
#include <QPrinter>
#include <QPrintDialog>

const enum ePrivilegeLevel cReportWidget::rights = PL_VIEWER;

cReportWidget::cReportWidget(QMdiArea *parent) :
    cIntSubObj(parent),
    ui(new Ui::reportWidget)
{
    pThread = nullptr;
    if (reportTypeName.isEmpty()) {
        appendCont(reportTypeName, tr("Trunks"), reportTypeNote, tr("Trunk port tagokok elleörzése"), RT_TRUNK);
        appendCont(reportTypeName, tr("Links"),  reportTypeNote, tr("Linkek összevetése"),            RT_LINKS);
        appendCont(reportTypeName, tr("Mactab"), reportTypeNote, tr("A MACTAB tébla és a linkek"),    RT_MACTAB);
        appendCont(reportTypeName, tr("VLAN"),   reportTypeNote, tr("Uplink és VLAN-ok"),             RT_UPLINK_VLANS);
    }
    ui->setupUi(this);
    ui->comboBoxType->addItems(reportTypeName);
    ui->comboBoxType->setCurrentIndex(0);
}

cReportWidget::~cReportWidget()
{
    delete ui;
}

void cReportWidget::on_pushButtonStart_clicked()
{
    int rt = ui->comboBoxType->currentIndex();
    if (pThread == nullptr && rt >= 0) {
        pThread = new cReportThread(eReportTypes(rt));
        connect(pThread, SIGNAL(finished()),       this, SLOT(threadReady()));
        connect(pThread, SIGNAL(senHtml(QString)), this, SLOT(appendHtml(QString)));
        ui->pushButtonStart->setDisabled(true);
        ui->pushButtonSave ->setDisabled(true);
        ui->pushButtonPrint->setDisabled(true);
        ui->pushButtonClear->setDisabled(true);
        pThread->start();
    }
    else {
        EXCEPTION(EPROGFAIL, rt);
    }
}

void cReportWidget::on_pushButtonSave_clicked()
{
    static QString fn;
    cFileDialog::textEditToFile(fn, ui->textEditRiport, this);
}

void cReportWidget::on_pushButtonPrint_clicked()
{
    QPrinter prn;
    QPrintDialog dialog(&prn, this);
    if (QDialog::Accepted != dialog.exec()) return;
    ui->textEditRiport->print(&prn);
}

void cReportWidget::on_comboBoxType_currentIndexChanged(int index)
{
    if (isContIx(reportTypeNote, index)) ui->lineEditType->setText(reportTypeNote.at(index));
    ui->pushButtonStart->setDisabled(false);
}

void cReportWidget::on_pushButtonClear_clicked()
{
    ui->textEditRiport->clear();
}

void cReportWidget::appendHtml(const QString& html)
{
    ui->textEditRiport->append(html);
}

void cReportWidget::threadReady()
{
    ui->pushButtonSave ->setDisabled(false);
    ui->pushButtonPrint->setDisabled(false);
    ui->pushButtonClear->setDisabled(false);
    pDelete(pThread);
}

cReportThread::cReportThread(cReportWidget::eReportTypes _rt)
    : QThread(), reportType(_rt)
{

}

void cReportThread::run()
{
    switch (reportType) {
    case cReportWidget::RT_TRUNK:       trunkReport();          break;
    case cReportWidget::RT_LINKS:       linksReport();          break;
    case cReportWidget::RT_MACTAB:      mactabReport();         break;
    case cReportWidget::RT_UPLINK_VLANS:uplinkVlansReport();    break;
    }

}

void cReportThread::trunkReport()
{

}

void cReportThread::linksReport()
{

}

void cReportThread::mactabReport()
{

}

void cReportThread::uplinkVlansReport()
{
    static const QString sql =
            "SELECT "
            ;

}

