#ifndef LV2USER_H
#define LV2USER_H
#include "lv2datab.h"

/// @class cGroup
/// Egy felhasználói csoport tábla rekordot reprezentáló osztály
/// @note
/// A host_notif_switchs és a serv_notif_switchs enumerációs tömböt még a cRecoed.tól örökölten
/// egy SET típusú adatként kezeli (mint a mysql-ben) Ha a getId-vel olvassuk ki az értéket, akkor egy bitmaszkot kapunk.
/// a notifswitch típusú mezőkhöz tartozó enumerációs típus az eNotifSwitch (lv2service.h -ban definiálva)
/// A set ben a bit sorszáma reprezentálja az enumerációs értéket:
/// pl.: a 'warning' hoz tartozó bitet az " 1 << notifSwitch(_sWarning) " vagy a " 1 << RS_WARNING " kifelyezés adja meg
/// A bitmask értéket a mező értékének a megadásánál is használhatjuk, a set() metüdus automatikusan konvertálja azt
/// az adatbázisban használható string tömbé, az objektumon bellül mindíg string tömbként jelennek meg a SET típusok
class LV2SHARED_EXPORT cGroup : public cRecord {
    CRECORD(cGroup);
};

/* ******************************  ****************************** */
class LV2SHARED_EXPORT cUser : public cRecord {
    CRECORD(cUser);
public:
    bool updatePassword(const QString &__passwd);
    bool checkPassword(const QString &__passwd) const;
};

/// @typedef cGroupUser
/// Az objektum típus a groups/users táblák közötti kapcsoló táblát kezeli
/// Nem cRecord típusú objektum. Da adattagként tartalmaza a cUser és cGroup objektumokat.
/// lsd.: tGroup deklarációját a lv2datab.h végén.
typedef tGroup<cGroup, cUser> cGroupUser;

#endif // LV2USER_H
