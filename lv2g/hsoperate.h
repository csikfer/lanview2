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

    cRecordListModel *pZoneModel;
    cRecordListModel *pPlaceModel;
    cRecordListModel *pNodeModel;
    cRecordListModel *pServiceModel;

    QSqlQuery       *pq;
    QSqlQuery       *pq2;
    QList<qlonglong>    idList;
protected slots:
    /// találatok beolvasása/megjelenítése a szűrési feltételek alapján.
    void fetch();
    /// A kijelölt szolgáltatás példányokon a megadott noalarm állpot beállítása
    void set();
    /// A táblázat összes sorának a kijelölése
    void all();
    /// A táblázat összes sorában a kijelülés törlése
    void none();
    void disable(bool f);
    void enable(bool f);
    void clrStat(bool f);
    void changePlacePattern(const QString& text);
    void changeNodePattern(const QString& text);
    void changeServicePattern(const QString& text);
};

#endif // HSOPERATE_H

