#ifndef HSOPERATE_H
#define HSOPERATE_H

#include <QObject>
#include <QWidget>
#include <QSound>
#include "lv2g.h"
#include "ui_hsoperate.h"
#include "lv2models.h"
#include "record_table.h"

class cHSOState;
class cHSOperate;

class LV2GSHARED_EXPORT cHSORow : public QObject {
    friend class cHSOperate;
    Q_OBJECT
public:
    cHSORow(QSqlQuery& q, cHSOState *par, int _row);
    QCheckBox * getCheckBoxSet();
    QWidget *   getWidgetSub();
    QWidget *getButtonCmd();
    QTableWidgetItem * item(int vix);
    QTableWidgetItem * item(int ix, const cColEnumType *pType);
    QTableWidgetItem * item(int vix, int eix, const cColEnumType *pType);
    QTableWidgetItem * item(int ix, const cColStaticDescr& cd);
    QTableWidgetItem * boolItem(int ix, const QString& tn, const QString &fn);
    static bool serviceIsApp(const QVariant &suphsid, const cService *ps);
    static QString findAppName(QSqlQuery &q, qlonglong hsid);
    qlonglong   id;             /// A példány ID-je
    int         nsub;           /// Az al szolgáltatáspéldányok száma
    QSqlRecord  rec;            /// Adatok
    const int   row;
    QSqlQuery  *pq;
    bool        set, sub;
    QCheckBox  *pCheckBoxSub;
    QCheckBox  *pCheckBoxSet;
    QComboBox  *pComboBoxCmd;
    QToolButton*pToolButtonCmd;
    const cService *pService;
    bool _serviceIsApp;
protected:
    cHSOperate *pDialog;
    static void staticInit();
    static const cColEnumType    *pPlaceType;
    static const cColEnumType    *pNoAlarmType;
    static const cColEnumType    *pNotifSwitch;
protected slots:
    void togleSet(bool f);
    void togleSub(bool f) { sub = f; }
    void goSub();
    void pressCmd();
};


class LV2GSHARED_EXPORT cHSOState : public QObject {
    Q_OBJECT
public:
    cHSOState(QSqlQuery &q, const QString& _sql, const QVariantList _binds, cHSOperate *par);
    QStringList getSupIds();
    cHSORow * rowAtId(qlonglong);
    cHSOperate * pDialog;
    QString         sql;    /// Az SQL string
    QVariantList    binds;  /// Az SQL string paraméterei
    QList<cHSORow *> rows;  /// A sorok tartalma
    int             size;   /// Rekordok/sorok száma
    int             nsup;   /// Az al szolgáltatás pédányokat is tartalmazó szolgáltatás pédányok száma
    QSqlQuery      *pq;
};

class LV2GSHARED_EXPORT cHSOperate : public cIntSubObj
{
    friend class cHSOState;
    friend class cHSORow;
    Q_OBJECT
public:
    cHSOperate(QMdiArea *par);
    ~cHSOperate();
    static const enum ePrivilegeLevel rights;
    int queryNodeServices(qlonglong _nid);
protected:
    void setButtonsFromTo();
    bool    lockSetButton;
    Ui::hostServiceOp *pUi;
    QButtonGroup   *pButtonGroupPlace;
    QButtonGroup   *pButtonGroupService;
    QButtonGroup   *pButtonGroupNode;
    QButtonGroup   *pButtonGroupSup;
    QButtonGroup   *pButtonGroupAlarm;

    cZoneListModel   *pZoneModel;
    cPlacesInZoneModel*pPlaceModel;
    cRecordListModel *pNodeModel;
    cRecordListModel *pServiceModel;
    cRecordListModel *pSupModel;

    QSqlQuery       *pq;
    QSqlQuery       *pq2;
    QList<cHSOState *> states;     /// Lekérdezés history, mentett query-k eredménye
    int             stateIx;    /// Lekérdezés history aktuális pozició
    cHSOState *    actState(eEx __ex = EX_ERROR);

    static const QString _sql;
    static const QString _ord;

    virtual void timerEvent(QTimerEvent *event);
    /// találatok megjelenítése
    void refreshTable();
    bool fetch(const QString& sql, const QVariantList& bind = QVariantList());
    void goSub(int row);
    ePrivilegeLevel privilegLevel;
    int permit;
    int refreshTime;
    int timerId;
    int lastAlramButtonId;
    QString sStart;
    QString sStop;
    qlonglong minIntervalMs;
    QDateTime now;
    cRecordTable *pSrvVarsTable;
    cRecordTable *pSrvLogsTable;
private slots:
    void on_pushButtonRefresh_clicked();
    /// Al példányok megjelenítése
    void on_pushButtonSub_clicked();
    /// A kijelölt szolgáltatás példányokon a megadott állpot beállítása vagy művelet végrehajtása
    void on_pushButtonSet_clicked();
    /// találatok beolvasása/megjelenítése a beállított filterek alakján
    void on_pushButtonFetch_clicked();
    /// A táblázat összes sorának a kijelölése
    void on_pushButtonAll_clicked();
    /// A táblázat összes sorában a kijelölés törlése
    void on_pushButtonNone_clicked();
    void on_pushButtonSubAll_clicked();
    void on_pushButtonSubNone_clicked();
    void on_comboBoxZone_currentIndexChanged(int ix);
    void on_checkBoxClrStat_toggled(bool f);
    void setAlarmButtons(int id);
    void on_lineEditPlacePattern_textChanged(const QString& text);
    void on_lineEditNodePattern_textChanged(const QString& text);
    void on_lineEditServicePattern_textChanged(const QString& text);
    void on_lineEditSupPattern_textChanged(const QString& text);
    /// Kettős klik a szervíz példányok táblázat egy celláján
    void on_tableWidget_doubleClicked(const QModelIndex& mi);
    /// Egyyel visszalépés, az előző lekérdezésre (nem olvassa újra)
    void on_toolButtonBack_clicked();
    /// Egyyel előre lépés, a következő lekérdezésre (nem olvassa újra)
    void on_toolButtonForward_clicked();
    /// A lekérdezések törlése az aktuális kivételével.
    void on_toolButtonClearHist_clicked();
    void on_pushButtonRoot_clicked();
    // refresh
    void on_pushButtonAutoRefresh_clicked();
    void on_spinBoxRefresh_valueChanged(int v);
    // ?!
    void on_textEditJustify_textChanged();

    void on_dateTimeEditFrom_dateTimeChanged(const QDateTime &dateTime);
    void on_dateTimeEditTo_dateTimeChanged(const QDateTime &dateTime);
    void on_toolButtonDateFrom_clicked();
    void on_toolButtonRstFrom_clicked();
    void on_toolButtonDateTo_clicked();
    void on_toolButtonRstTo_clicked();
    void on_toolButtonIntervalDef_clicked();
    void on_tableWidget_itemSelectionChanged();
    void on_toolButtonUnknown_clicked();
    void on_toolButtonCritical_clicked();
    void on_toolButtonWarning_clicked();

    void on_checkBoxSup_toggled(bool checked);
    void on_radioButtonSupPattern_toggled(bool checked);
    void on_toolButtonSupNull_toggled(bool checked);

    void on_checkBoxPlace_toggled(bool checked);
    void on_radioButtonPlacePattern_toggled(bool checked);

    void on_checkBoxNode_toggled(bool checked);
    void on_radioButtonNodePattern_toggled(bool checked);

    void on_checkBoxService_toggled(bool checked);
    void on_radioButtonServicePattern_toggled(bool checked);

    void on_checkBoxRemove_toggled(bool checked);

private:
    void setCell(int row, int col, QTableWidgetItem * pi) {
        if (pi != nullptr) {
            pUi->tableWidget->setItem(row, col, pi);
        }
    }
    void setCell(int row, int col, QWidget * pw) {
        if (pw != nullptr) {
            pUi->tableWidget->setCellWidget(row, col, pw);
        }
    }
    void enableSup(bool ena, bool pattern, bool zero) {
        pUi->radioButtonSupSelect-> setEnabled(ena);
        pUi->radioButtonSupPattern->setEnabled(ena);
        pUi->lineEditSupPattern->   setEnabled(ena &&  pattern && !zero);
        pUi->toolButtonSupNull->    setEnabled(ena &&  pattern);
        pUi->comboBoxSupSelect->    setEnabled(ena && !pattern);
    }
    void enablePlace(bool ena, bool pattern) {
        pUi->radioButtonPlaceSelect-> setEnabled(ena);
        pUi->radioButtonPlacePattern->setEnabled(ena);
        pUi->lineEditPlacePattern->   setEnabled(ena &&  pattern);
        pUi->comboBoxPlaceSelect->    setEnabled(ena && !pattern);
    }
    void enableNode(bool ena, bool pattern) {
        pUi->radioButtonNodeSelect-> setEnabled(ena);
        pUi->radioButtonNodePattern->setEnabled(ena);
        pUi->lineEditNodePattern->   setEnabled(ena &&  pattern);
        pUi->comboBoxNodeSelect->    setEnabled(ena && !pattern);
    }
    void enableService(bool ena, bool pattern) {
        pUi->radioButtonServiceSelect-> setEnabled(ena);
        pUi->radioButtonServicePattern->setEnabled(ena);
        pUi->lineEditServicePattern->   setEnabled(ena &&  pattern);
        pUi->comboBoxServiceSelect->    setEnabled(ena && !pattern);
    }
    void removeSelected();
};

#endif // HSOPERATE_H

