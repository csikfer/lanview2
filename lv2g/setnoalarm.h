#ifndef SETNOALARM
#define SETNOALARM

#include <QObject>
#include <QWidget>
#include <QSound>
#include "lv2g.h"
#include "ui_noalarm.h"
#include "lv2models.h"

class LV2GSHARED_EXPORT cSetNoAlarm : public cOwnTab
{
    Q_OBJECT
public:
    cSetNoAlarm(QWidget *par);
    ~cSetNoAlarm();
    static const enum ePrivilegeLevel rights;
protected:
    Ui_setNoAlarm   *pUi;
    QButtonGroup   *pButtonGroupPlace;
    QButtonGroup   *pButtonGroupService;
    QButtonGroup   *pButtonGroupNode;
    QButtonGroup   *pButtonGroupType;

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
    void off(bool f);
    void on(bool f);
    void to(bool f);
    void from(bool f);
    void fromTo(bool f);
    void editedPlacePattern(const QString& text);
    void editedNodePattern(const QString& text);
    void editedServicePattern(const QString& text);
};

#endif // SETNOALARM

