#ifndef HSOPERATE_H
#define HSOPERATE_H

#include <QObject>
#include <QWidget>
#include <QSound>
#include "lv2g.h"
#include "ui_hsoperate.h"
#include "lv2models.h"

class LV2GSHARED_EXPORT cHSOperate : public cIntSubObj
{
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
    QList<qlonglong>    idList;     /// Beolvasott sorokhoz tartozó host_service_id értékek listája
    QMap<int,qlonglong> supIdMap;   /// A beolvasott al példányokkal rendelkező pédányok id-i, index a tábla sora
    QList<QList<QSqlRecord> > history; /// Lekérdezés history, mentett query-k eredménye
    int                 historyIx;  /// Lekérdezés history aktuális pozició
    QString         _sql;
    QString         _ord;

protected:
    /// találatok beolvasása/megjelenítése
    void refreshTable(QList<QSqlRecord>& recs);
    void fetch();
protected slots:
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
};

#endif // HSOPERATE_H

