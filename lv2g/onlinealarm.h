#ifndef CONLINEALARM_H
#define CONLINEALARM_H

#include "lv2g.h"
#include "record_table.h"
#include "QSound"

class  LV2GSHARED_EXPORT cOnlineAlarm : public cOwnTab
{
    Q_OBJECT
public:
    cOnlineAlarm(QWidget *par);
    ~cOnlineAlarm();
    static const enum ePrivilegeLevel rights;
private:
    void map();
    void clearMap() {
        pMapText->setText(_sNul);
        pMap->clearDraws();
        pMap->setText(_sNul);
    }

    QSqlQuery *     pq;
    bool            isAdmin;
    QVBoxLayout *   pMainLayout;    /// Az ablak(widget)-hez rendelt layout.
    QSplitter *     pMainSplitter;  /// Az ablak splittere: balra riasztások,  jobbra térkép
    QSplitter *     pAlarmSplitter; /// A bal oldali splitter, riasztások: fent a nem nyugtázott, lent a nyugtázott de aktív
    cRecordTable *  pRecTabNoAck;   /// A nem nyugtázott riasztások táblája (tree?)
    cRecordTable *  pRecTabAckAct;  /// A nyugtázott aktív riasztások táblála (tree?)
    QWidget *       pRightWidget;   /// A jobb oldali widget
    QVBoxLayout *   pRightVBLayout; /// A jobb oldali widget vertikális layout
    QHBoxLayout *   pButtonLayout;  /// A jobb oldali Widget gombsora (alul)
    QTextEdit *     pMapText;      /// A térkép/alaprajz cím sora
    cImageWidget *  pMap;           /// A térkép/alaprajz (image)
    QPushButton *   pAckButton;     /// A nyugtázás gomb
    QPushButton *   pAckAllButton;  /// Több sor nyugtázás gomb (Admin)
    cRecord  *      pActRecord;     /// A kiválasztott rekord a nem nyugtázott riasztások táblában
private slots:
    void curRowChgNoAck(QItemSelection, QItemSelection);
    void curRowChgAckAct(QItemSelection sel, QItemSelection);
    void acknowledge();
    void allAcknowledge();
    void actRecordDestroyed(QObject *pO);
    void noAckDataReloded(const tRecords& _recs);
};

#if defined(LV2G_LIBRARY)
#include "ui_acknowledge.h"

/// @class cAckDialog
/// @brief Egy riasztás nyugtázása dialógus ablak.
class  cAckDialog : public QDialog {
    Q_OBJECT
public:
    cAckDialog(const cRecord &__r, QWidget *par = NULL);
    ~cAckDialog();
    Ui_ackDialog *pUi;
private slots:
    /// A megjegyzés mező megváltozásához rendselt slot:
    /// Ga a megjegyzés mező ki van töltve, akkor engedélyezi a nyugtézás gonbot.
    void changed();
};
#endif // defined(LV2G_LIBRARY)

#endif // CONLINEALARM_H
