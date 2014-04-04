#ifndef MENU_H
#define MENU_H

#include "lv2g.h"
#include "record_table.h"
#include <QMainWindow>

enum eMenuActionType {
    MAT_ERROR,          ///< Az objektum inicializálása sikertelen
    MAT_SHAPE,          ///< Adatbázis tábla/táblák megjelenítése/kezelése
    MAT_EXEC,           ///< egy parancs végrehajtása
    MAT_OWN,            ///< egy widget megjelenítése
    MAT_WIDGET,         ///< egy widget megjelenítése
    MAT_DIALOG          ///< egy dialogus ablak mejelenítése
};

/// @class cMenuAction
/// A menu pontok funkcióit megvalósító objektum.
class LV2GSHARED_EXPORT cMenuAction : public QObject {
    Q_OBJECT
public:
    /// Konstruktor
    cMenuAction(QSqlQuery *pq, cMenuItem * pmi, QAction * pa, QTabWidget * par, bool __ex = true);
    /// Konstruktor
    /// Egy tábla megjelenítése
    /// @param ps A megjelenítés leíró objektuma (a ps objektum ownere lessz a this objektum példány)
    /// @param pa A menü elem QAction objektuma
    /// @param par A QTab objektum, melynek egy tab-jában történik a megjelenítés
    cMenuAction(cTableShape *ps, const QString&  nm, QAction *pa, QTabWidget *par);
    /// Konstruktor
    /// A menüpont egy parancsot hajt végre
    /// @param ps A végrehajtandó parancs (az objektum neveként lessz eltárolva)
    /// @param pa A menü elem QAction objektuma
    /// @param par A QTab objektum, melynek egy tab-jában történik a megjelenítés
    cMenuAction(const QString&  ps, QAction *pa, QTabWidget *par);
    /// Konstruktor
    /// A menüpont egy widget-et jelenít meg
    /// @param ps A widget pointere
    /// @param pa A menü elem QAction objektuma
    /// @param par A QTab objektum, melynek egy tab-jában történik a megjelenítés
    cMenuAction(cOwnTab *po, QAction *pa, QTabWidget * par);
    /// Destruktor (üres)
    ~cMenuAction();
    /// Típus
    const enum eMenuActionType  type;
    /// Az owner tab widget pointere
    QTabWidget     *pTabWidget;
//    cOwnTab         *pOwnTab;       ///< ? A menüponthoz tartozó tab ?
    /// Amennyiben egy táblát jelenítünk meg, akkor annak a leírója (több tábla esetén a fő tábláé)
    cTableShape     *pTableShape;
    /// Amennyiben egy táblát jelenítünk meg, akkoe a megjelenítést végző objektum
    cRecordTable    *pRecordTable;
    /// A tab-ban a menüponthoz tartozó widget
    QWidget         *pWidget;
    /// Amennyiben egy dialógus ablakként jelenik meg, akkor a dialogus ablak pointere
    QDialog         *pDialog;
    ///< A menü elemhez tartozó QAction objektum
    QAction         *pAction;
private:
    /// A type adattag beállítása
    void setType(enum eMenuActionType _t) {
        *const_cast<enum eMenuActionType *>(&type) = _t;
    }

    /// Tábla megjelenítése esetén az inicializáló metódus.
    void initRecordTable();
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

#endif // MENU_H
