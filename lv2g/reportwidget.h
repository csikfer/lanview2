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
    /// A riport típusa
    enum eReportTypes {
        RT_TRUNK, RT_LINKS, RT_MACTAB, RT_UPLINK_VLANS
    };
    /// A riport szintje/szűrő
    enum eReportLevel {
        RL_ALL,     ///< Teljes riport
        RL_WARNING, ///< Hibék, és figelmeztetések
        RL_ERROR    ///< Csak a hibák
    };
    enum eReportLinkSubType {
        RLT_LOG_LINKS,
        RLT_LLDP_LINKS
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
    static QStringList     reportTypeName;
    static QStringList     reportTypeNote;
    static QStringList     reportLevels;
    static QStringList     reportLinkSubType;
    cReportThread * pThread;
};

class cTrunk;

class cReportThread : public QThread {
    friend class cReportWidget;
    friend class cTrunk;
    Q_OBJECT
protected:
    cReportThread(int _rt, int _l, int _st);
    QQueue<QString> queue;
    const int reportType;
    const int reportLevel;
    const int subType;
    void run();
    int tableCellPaddingPix;
    int recordNumber;
    cError * pLastError;
private:
    void trunkReport();
    void linksReport();
    void mactabReport();
    void uplinkVlansReport();
    void nodeHead(eTristate ok, const QString& nodeName, const QStringList &tableHead, const QList<QStringList>& matrix);
    void nodeHead(eTristate ok, const QString &nodeName, const QString &html);
    void sendMsg(const QString& msg) {
        queue.enqueue(msg);
        emit msgQueued();
    }
    static const QString sqlCount;
signals:
    void msgQueued();
    void progres(int percent);
};

#endif // REPORTWIDGET_H
