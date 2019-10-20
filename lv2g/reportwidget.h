#ifndef REPORTWIDGET_H
#define REPORTWIDGET_H

#include <QWidget>
#include "lv2g.h"

namespace Ui {
class reportWidget;
}
class cReportThread;

class cReportWidget : public cIntSubObj
{
    friend cReportThread;
    Q_OBJECT
public:
    explicit cReportWidget(QMdiArea *parent = nullptr);
    ~cReportWidget();
    static const enum ePrivilegeLevel rights;
protected:
    enum eReportTypes {
        RT_TRUNK, RT_LINKS, RT_MACTAB, RT_UPLINK_VLANS
    };
private slots:
    void on_pushButtonStart_clicked();
    void on_pushButtonSave_clicked();
    void on_pushButtonPrint_clicked();
    void on_comboBoxType_currentIndexChanged(int index);
    void on_pushButtonClear_clicked();
protected slots:
    void msgReady();
    void threadReady();
private:
    Ui::reportWidget *ui;
    QStringList     reportTypeName;
    QStringList     reportTypeNote;
    cReportThread * pThread;
};

class cReportThread : public QThread {
    Q_OBJECT
public:
    cReportThread(cReportWidget::eReportTypes _rt);
    QQueue<QString> queue;
protected:
    cReportWidget::eReportTypes reportType;
    void run();
private:
    void trunkReport();
    void linksReport();
    void mactabReport();
    void uplinkVlansReport();
    void sendMsg(const QString& msg) {
        queue.enqueue(msg);
        emit msgQueued();
    }
signals:
    void msgQueued();
};

#endif // REPORTWIDGET_H
