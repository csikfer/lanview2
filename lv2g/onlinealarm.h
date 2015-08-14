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
    static const enum ePrivilegeLevel rights;
private:
    void map();
    QSqlQuery *     pq;
    QVBoxLayout *   pMainLayout;    /// Az ablak(widget)-hez rendelt layout.
    QSplitter *     pMainSplitter;  /// Az ablak splittere: jobbra riasztások, balra térkép
    QSplitter *     pAlarmSplitter; /// A bal oldali splitter, riasztások: fent a nem nyugtázott, lent a nyugtázott de aktív
    cRecordTable *  pRecTabNoAck;   /// A nem nyugtázott riasztások táblála (tree?)
    cRecordTable *  pRecTabAckAct;  /// A nyugtázott aktív riasztások táblála (tree?)
    QWidget *       pRightWidget;   /// A jobb oldali widget
    QVBoxLayout *   pRightVBLayout; /// A jobb oldali widget vertikális layout
    QLabel *        pMapLabel;      /// A térkép/alaprajz cím sora
    cImageWidget *  pMap;           /// A térkép/alaprajz (image)
    QPushButton *   pAckButton;     /// A nyugtázás gomb
    const cRecordAny *pActRecord;   /// A kiválasztott rekord a nem nyugtázott riasztások táblában
private slots:
    void curRowChgNoAck(QItemSelection sel, QItemSelection);
    void curRowChgAckAct(QItemSelection sel, QItemSelection);
    void acknowledge();
};

#if defined(LV2G_LIBRARY)
#include "ui_acknowledge.h"

/// @class cAckDialog
/// @brief Egy riasztás nyugtázása dialógus ablak.
class  cAckDialog : public QDialog {
    Q_OBJECT
public:
    cAckDialog(const cRecordAny& __r, QWidget *par = NULL);
    ~cAckDialog();
    Ui_ackDialog *pUi;
private slots:
    /// A megjegyzés mező megváltozásához rendselt slot:
    /// Ga a megjegyzés mező ki van töltve, akkor engedélyezi a nyugtézás gonbot.
    void changed();
};
#endif // defined(LV2G_LIBRARY)

#endif // CONLINEALARM_H
