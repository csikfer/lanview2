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
    void acknowledge();
};

_GEX QPolygonF toQPolygonF(const tPolygonF _pol);

#if defined(LV2G_LIBRARY)
#include "ui_acknowledge.h"

class cAckDialog : public QDialog {
    Q_OBJECT
public:
    cAckDialog(const cRecordAny& __r, QWidget *par = NULL);
    ~cAckDialog();
    Ui_ackDialog *pUi;
private slots:
    void changed();
};
#endif // defined(LV2G_LIBRARY)

#endif // CONLINEALARM_H
