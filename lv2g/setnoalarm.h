#ifndef SETNOALARM
#define SETNOALARM

#include <QObject>
#include <QWidget>
#include <QSound>
#include "lv2g.h"
#include "ui_noalarm.h"

class LV2GSHARED_EXPORT cSetNoAlarm : public cOwnTab
{
    Q_OBJECT
public:
    cSetNoAlarm(QWidget *par);
    static const enum ePrivilegeLevel rights;
protected:
    Ui_setNoAlarm   *pUi;
    QButtonGroup   *pButtonGroupPlace;
    QButtonGroup   *pButtonGroupService;
    QButtonGroup   *pButtonGroupNode;
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
};

#endif // SETNOALARM

