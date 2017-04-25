#ifndef HSOPERATE_H
#define HSOPERATE_H

#include <QObject>
#include <QWidget>
#include <QSound>
#include "lv2g.h"
#include "ui_hsoperate.h"
#include "lv2models.h"

class cHSOState;
class cHSOperate;

class LV2GSHARED_EXPORT cHSORow : public QObject {
    friend class cHSOperate;
    Q_OBJECT
public:
    cHSORow(QSqlQuery& q, cHSOState *par);
    QCheckBox * getCheckBoxSet();
    QCheckBox * getCheckBoxSub();
    QTableWidgetItem * item(int vix);
    QTableWidgetItem * item(int ix, const cColEnumType *pType);
    QTableWidgetItem * item(int vix, int eix, const cColEnumType *pType);
    QTableWidgetItem * item(int ix, const cColStaticDescr& cd);
    QTableWidgetItem * boolItem(int ix, const QString& tn, const QString &fn);
    qlonglong   id;             /// A példány ID-je
    int         nsub;           /// Az al szolgáltatáspéldányok száma
    QSqlRecord  rec;            /// Adatok
    QSqlQuery  *pq;
    bool        set, sub;
protected:
    static void staticInit();
    static const cColEnumType    *pPlaceType;
    static const cColEnumType    *pNoAlarmType;
    static const cColEnumType    *pNotifSwitch;
protected slots:
    void togleSet(bool f) { set = f; }
    void togleSub(bool f) { sub = f; }
};


class LV2GSHARED_EXPORT cHSOState : public QObject {
    Q_OBJECT
public:
    cHSOState(QSqlQuery &q, const QString& _sql, const QVariantList _binds, cHSOperate *par);
    QStringList getSupIds();
    cHSORow * rowAtId(qlonglong);
    QString         sql;    /// Az SQL string
    QVariantList    binds;  /// Az SQL string paraméterei
    QList<cHSORow *>   rows;   /// A sorok tartalma
    int             size;   /// Rekordok/sorok száma
    int             nsup;   /// Az al szolgáltatás pédányokat is tartalmazó szolgáltatás pédányok száma
    QSqlQuery      *pq;
};

class LV2GSHARED_EXPORT cHSOperate : public cIntSubObj
{
    friend class cHSOState;
    Q_OBJECT
public:
    cHSOperate(QMdiArea *par);
    ~cHSOperate();
    static const enum ePrivilegeLevel rights;
protected:
    void setButton();
    Ui_hostServiceOp *pUi;
    QButtonGroup   *pButtonGroupPlace;
    QButtonGroup   *pButtonGroupService;
    QButtonGroup   *pButtonGroupNode;

    cZoneListModel   *pZoneModel;
    cPlacesInZoneModel*pPlaceModel;
    cRecordListModel *pNodeModel;
    cRecordListModel *pServiceModel;

    QSqlQuery       *pq;
    QSqlQuery       *pq2;
    QList<cHSOState *> states;     /// Lekérdezés history, mentett query-k eredménye
    int             stateIx;    /// Lekérdezés history aktuális pozició
    cHSOState *    actState(eEx __ex = EX_ERROR);

    static const QString _sql;
    static const QString _ord;

protected:
    /// találatok megjelenítése
    void refreshTable();
    bool fetch(const QString& sql, const QVariantList& bind = QVariantList());
protected slots:
    void refresh();
    /// Al példányok megjelenítése
    void fetchSubs();
    /// találatok beolvasása/megjelenítése a beállított filterek alakján
    void fetchByFilter();
    /// A kijelölt szolgáltatás példányokon a megadott noalarm állpot beállítása
    void set();
    /// A táblázat összes sorának a kijelölése
    void all();
    /// A táblázat összes sorában a kijelölés törlése
    void none();
    void subAll();
    void subNone();
    void zoneChanged(int ix);
    void disable(bool f);
    void enable(bool f);
    void clrStat(bool f);
    void disableAlarm(bool f);
    void enableAlarm(bool f);
    void changePlacePattern(const QString& text);
    void changeNodePattern(const QString& text);
    void changeServicePattern(const QString& text);
    void doubleClickCell(const QModelIndex& mi);
    /// Egyyel visszalépés, az előző lekérdezésre (nem olvassa újra)
    void back();
    /// Egyyel előre lépés, a következő lekérdezésre (nem olvassa újra)
    void forward();
    /// A lekérdezések törlése az aktuális kivételével.
    void clear();
private:
    void setCell(int row, int col, QTableWidgetItem * pi) {
        if (pi != NULL) {
            pUi->tableWidget->setItem(row, col, pi);
        }
    }
    void setCell(int row, int col, QWidget * pw) {
        if (pw != NULL) {
            pUi->tableWidget->setCellWidget(row, col, pw);
        }
    }
};

#endif // HSOPERATE_H

