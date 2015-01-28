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
    void updatePassword(QSqlQuery &q, const QString &__passwd);
    void updatePassword(const QString &__passwd) { QSqlQuery q = getQuery(); updatePassword(q, __passwd); }
    /// Ellenörzi a jelszót. A felhasználót azonosító user_id-t az objektum tartalmazza, vagyis legalább ezt az egy mezőt
    /// ki kell tölteni az objektumban.
    /// @return Ha a jelszó helyes, és nincs letiltva a felhasználó, akkor true-val tér vissza, egyébként false értékkel
    /// @param q Adatbázis művelethez használlt objektum
    /// @param __passwd Az ellenörizendő jelszó.
    bool checkPassword(QSqlQuery &q, const QString &__passwd) const;
    bool checkPassword(const QString &__passwd) const { QSqlQuery q = getQuery(); return checkPassword(q, __passwd); }
    /// Ellenörzi a jelszót, ha megfelelő, akkor betölti az objektumot.
    /// A felhasználó azonosítása többféleképpen lehetséges:
    /// Megadjuk az opcionális __id paramétert, ami a user_id, vagy kitöltjük az user_id, vagy user_name mezőket az objektumban.
    /// Ha mind az user_id és user_name mező ki van töltve (és nincs megadba az __id patraméter), akkor az user_id az elsődleges.
    /// @return Ha a jelszó helyes, és nincs letiltva a felhasználó, akkor true-val tér vissza, egyébként false értékkel
    /// @param __q Adatbázis művelethez használlt objektum
    /// @param __passwd Az ellenörizendő jelszó.
    /// @param __id A felhasználó ID-je. Ha megadjuk ez azonosítja a felhasználót, az objektum tartalmátol függetlenül.
    bool checkPasswordAndFetch(QSqlQuery& q, const QString &__passwd, qlonglong __id = NULL_ID);
    bool checkPasswordAndFetch(const QString &__passwd, qlonglong __id = NULL_ID) { QSqlQuery q = getQuery(); return checkPasswordAndFetch(q, __passwd, __id); }
    bool checkPasswordAndFetch(QSqlQuery& q, const QString &__passwd, const QString& __nm)
    {
        clear();
        setName(__nm);
        return checkPasswordAndFetch(q, __passwd);
    }
    bool checkPasswordAndFetch(const QString &__passwd, const QString& __nm) { QSqlQuery q = getQuery(); return checkPasswordAndFetch(q, __passwd, __nm); }
};

/// @typedef cGroupUser
/// Az objektum típus a groups/users táblák közötti kapcsoló táblát kezeli
/// Nem cRecord típusú objektum. Da adattagként tartalmaza a cUser és cGroup objektumokat.
/// lsd.: tGroup deklarációját a lv2datab.h végén.
typedef tGroup<cGroup, cUser> cGroupUser;

#endif // LV2USER_H
