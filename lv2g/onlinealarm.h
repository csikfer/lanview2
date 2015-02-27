#ifndef CONLINEALARM_H
#define CONLINEALARM_H

#include "lv2g.h"
#include "record_table.h"

class  LV2GSHARED_EXPORT cOnlineAlarm : public cOwnTab
{
    Q_OBJECT
public:
    cOnlineAlarm(QWidget *par);
    ~cOnlineAlarm();
private:
    void map();
    QSqlQuery *     pq;
    QVBoxLayout *   pMainLayout;
    QSplitter *     pMainSplitter;
    QSplitter *     pAlarmSplitter;
    cRecordTable *  pRecTabNoAck;
    cRecordTable *  pRecTabAckAct;
    QWidget *       pRightWidget;
    QVBoxLayout *   pRightVBLayout;
    QLabel *        pMapLabel;
    QLabel *        pMap;
    QPushButton *   pAckButton;
    const cRecordAny *    pActRecord;
private slots:
    void curRowChgNoAck(QItemSelection sel, QItemSelection);
    void curRowChgAckAct(QItemSelection sel, QItemSelection);
};

#endif // CONLINEALARM_H
