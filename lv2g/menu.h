#ifndef MENU_H
#define MENU_H

#include <QMainWindow>

#include "lv2g.h"

enum eMenuActionType {
    MAT_ERROR,          ///< Az objektum inicializálása sikertelen
    MAT_SHAPE,          ///< Adatbázis tábla/táblák megjelenítése/kezelése
    MAT_EXEC,           ///< egy parancs végrehajtása
    MAT_OWN,            ///< egy widget megjelenítése
    MAT_WIDGET,         ///< egy widget megjelenítése
    MAT_DIALOG          ///< egy dialogus ablak mejelenítése
};

enum eIntSubWin {          /// Egyedi GUI tab widget elemek
    INT_UNKNOWN = -1,
    INT_SETUP   =  0,   /// SETUP alap beállítások
    INT_GSETUP,         /// tobábbi beállítások a GUI-hoz
    INT_PARSER,         /// Parser hívása
    INT_EXPORT,         /// Export
    INT_OLALARM,        /// On-Line riasztások
    INT_ERRCODES,       /// Program hiba kód táblázat
    INT_HSOP,           /// a szervíz példányok (host_services) állpot manipuláció, riasztás tiltások
    INT_FINDMAC,        /// Keresés egy MAC-re
    INT_WORKSTATION,    /// Új munkaállomás, vagy modosítás űrlap
    INT_DEDUCEPATCH,    /// Falikábel felfedezés
    INT_SNMPDQUERY,     /// SNMP device query; insert/refresh
    INT_ENUMEDIT,
    INT_TRANSLATOR,
    INT_CHANGESWITCH,
    INT_REPORTWIDGET
};

class cRecordsViewBase;

/// @class cMenuAction
/// A menu pontok funkcióit megvalósító objektum.
class LV2GSHARED_EXPORT cMenuAction : public QObject {
    Q_OBJECT
public:
    /// Konstruktor
    /// Menüpont, a *pmi objektumban leírtak alapján inicializálj az objektumot.
    /// Az objektumot a menu_items rekordja alapján hozzuk létre.
    cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QMdiArea * par);
    /// Konstruktor
    /// A menüpont egy parancsot hajt végre. Az objektumot nem a menu_items rekordja alapján hozzuk létre.
    /// @param ps A végrehajtandó parancs (az objektum neveként lessz eltárolva)
    /// @param pa A menü elem QAction objektuma
    /// @param par A QTab objektum, melynek egy tab-jában történik a megjelenítés
    cMenuAction(const QString&  ps, QAction *pa, QMdiArea *par);
    /// Konstruktor
    /// A menüpont egy widget-et jelenít meg. Az objektumot nem a menu_items rekordja alapján hozzuk létre.
    /// @param ps A widget pointere
    /// @param t A ps típusa
    /// @param pa A menü elem QAction objektuma
    /// @param par A QTab objektum, melynek egy tab-jában történik a megjelenítés
    cMenuAction(cIntSubObj *po, enum eIntSubWin t, QAction *pa, QMdiArea *par);
    /// Destruktor (üres)
    ~cMenuAction();
    /// Típus
    const enum eMenuActionType  type;
    /// Az owner widget pointere
    QMdiArea     *pMdiArea;
    ///
    cMenuItem   *pMenuItem;
    /// Amennyiben egy belső widget/sub window, akkor a bázis objektum pointere
    cIntSubObj     *pIntSubObj;
    int cnt;
    enum eIntSubWin  intType;
    /// Amennyiben egy dialógus ablakként jelenik meg, akkor a dialogus ablak pointere
    QDialog         *pDialog;
    ///< A menü elemhez tartozó QAction objektum
    QAction         *pAction;
    static QMap<QString, QAction *>    actionsMap;
private:
    /// A type adattag beállítása
    void setType(enum eMenuActionType _t) {
        *const_cast<enum eMenuActionType *>(&type) = _t;
    }

    /// Tábla megjelenítése esetén az inicializáló metódus.
    void initRecordTable();
    /// Own megjelenítése esetén az inicializáló metódus.
    void initInt();
public slots:
    /// Az objektumot meg kell jeleníteni
    void displayIt();
    /// Az objektumot be kell zárni, ill. el kell tüntetni (megjelenítés vége)
    void removeIt();
    /// A gyerek (megjelenített) objektum megszűnt
    void destroyedChild();
    /// Ha a pDialog pointer nem NULL, akkor a dialogus objektumnak az exec() metódusa kerül végrehajtásra.
    /// Ha a pDialog értéke NULL, akkor feltételezi, hogy egy parancs van az objektumhoz rendelve, és a parancsot végre kell hajtani.
    /// A jelenleg végrehajtjató parancsok (a parancs az objektum neve):
    /// - "reset" hívja az appRestart() függvényt, vagyis újraindul a program.
    /// - "exit" A program kilép
    /// - minden egyébb esetben EBDATA koddal hibaüzenetet kapunk
    void executeIt();
};

class LV2GSHARED_EXPORT cTableSubWin : public cIntSubObj {
public:
    cTableSubWin(const QString& shape, QMdiArea * pMdiArea, const cFeatures& _f = cFeatures());
    /// Táblát jelenítünk meg, annak a leírója (több tábla esetén a fő tábláé)
    cTableShape     *pTableShape;
    /// A megjelenítést végző objektum
    cRecordsViewBase *pRecordsView;

};

#endif // MENU_H
