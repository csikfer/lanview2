#ifndef LV2G_H
#define LV2G_H

/*!
@file lv2g.h
@author Csiki Ferenc <csikfer@gmail.com>
@brief LanView2 GUI API
*/

/*!
@page GUIAPI A LanvWie API GUI modulja
Külön könyvtárba kerültek a LanView2 rendszer GUI-t támogató osztályai, ill. funkciói.
A könyvtár a következő osztályokat valósítja meg:\n
@section modells Model osztályok
- cStringListModel  String lista megjelenítése és kezelése
- cPolygonTableModel Polygon táblázatos megjelenítése
- cRecordListModel Rekord lista megjelenítése, minden rekordot egy string (név) azonosít
- cRecordTableModel
- cRecordTableModelSql
@section validators Validátor osztályok
- cIntValidator Validator egy numerikus egész típusú mezőhöz.
- cRealValidator Validator egy numerikus típusú mezőhöz.
- cMacValidator Validator egy MAC típusú mezőhöz.
- cINetValidator Input validátor a INET adattípushoz. Csak a hoszt cím van engedélyezve.
- cCidrValidator Input validátor a CIDR adattípushoz. (hálózati cím tartomány)
@section vidgets Widget-ek. Mezők meglenítése, szerkesztése
- cImageWindow Egy képet megjelenítő ablak.
- cFieldEditBase Bázis objektum, egy mező megjelenítéséhez, ill. adattartalmának a szerkesztéséhez.
- cSetWidget Egy set típusú (enumerációs tömb) adatmező megjelenítése, ill. szerkesztésére.
- cEnumRadioWidget Egy enumerációs (vagy bool) adatmező megjelenítése, ill. szerkesztésére.
- cEnumComboWidget Egy enumerációs (vagy bool) adatmező megjelenítése, ill. szerkesztésére.
- cFieldLineWidget Szöveges mező, egy sorban.
- cArrayWidget Egy tömb típusú adatmező megjelenítése és módosítása.
- cPolygonWidget Egy tömb adatmező megjelenítése és módosítása.
- cFKeyWidget Egy távoli kulcs mező megjelenítése, és szerkesztése.
- cTimeWidget Egy időpont megjelenítése ill. szerkesztése.
- cDateWidget Egy dátun megjelenítése ill. szerkesztése.
- cDateTimeWidget Egy dátun, és idópont megjelenítése ill. szerkesztése.
- cIntervalWidget Egy intervallum
- cBinaryWidget
@section guihigh Magasabb szintű GUI API osztályok
- cRecordTable
- cRecordTableColumn
- cRecordTableFODialog
- cRecordTableFilter
- cRecordTableOrd
- cRecordDialogBase
- cRecordDialog
- cRecordDialogTab
- cDialogButtons

*/

#include "lv2g_global.h"
#include "guidata.h"
#include <QtWidgets>

/// A GUI API inicializálása
/// Legalább egyszer meg kell hívni, ismételt hívása esetén nem csinál semmit.
_GEX void initLV2GUI();

/* Ez valami régebbi visszamaradt kód
/// A kiszelektált sorok listájával tér vissza
/// @param _sel A Qt objektum, melyből a kiszelektált sorok indexeit ki kell nyerni.
/// @return A kiszelektált sorok indexei, minden index csak egyszer szerepel.
_GEX QList<int> selection2rows(const QItemSelection& _sel);
/// A kiszelektált sorok listályának a modosítása
/// @param rows Az eredeti szelektált sorok listája.
/// @param _on Qt objektum, a új szelektált elemek
/// @param _off Qt objektum, a már nem szelektált elemek
/// @return egy a rows-ra mutató referencia.
_GEX QList<int>& modSelectedRows(QList<int>& rows, const QItemSelection& _on, const QItemSelection& _off);


/// Egy mező érték átmásolása a rekordba
/// @tparam T A mező azonosító típusa: QString, vagy const * (név), vagy egész szám (index)
/// @tparam V a mező érték típusa
/// @param __o a cél rekord objektum.
/// @param __i A mező azonosító
/// @param __v A mező új értéke
/// @return Hiba esetén false, egyébként true
template <class T, class V> static inline bool fromEdit(cRecord& __o, const T& __i, const V&  __v)
{
    if (!__o.isIndex(__i)) {
        if (__v.isEmpty()) return true;    // Nincs ilyen mező, és nem rakunk bele semmit, OK
        else               return false;   // Nincs ilyen mező, de raknánk bele valamit az már gáz
    }
    if (__v.isEmpty() && __o.isNullable(__i)) __o.clear(__i);   // Ures string esetén, ha lehet NULL, akkor NULL-ra állítjuk
    else                                      __o[__i] = __v;   // Egyébként megy bele az érték
    return true;
}
*/

typedef QList<QHBoxLayout *> hBoxLayoutList;
typedef QList<QLabel *>     labelList;

static inline QWidget *newFrame(int _st, QWidget * p = NULL)
{
    QFrame *pFrame = new QFrame(p);
    pFrame->setFrameStyle(_st);
    return pFrame;
}
static inline QWidget *newHLine(QWidget * p = NULL) { return newFrame(QFrame::HLine, p); }
static inline QWidget *newVLine(QWidget * p = NULL) { return newFrame(QFrame::VLine, p); }

_GEX void _setGUITitles();
_GEX QString _titleWarning;
_GEX QString _titleError;
_GEX QString _titleInfo;

// A tab-os megjelenítéshez egy segéd osztály, csak annyi a szerepe, hogy van egy removeTab() szignálja
class LV2GSHARED_EXPORT cOwnTab : public QWidget {
    Q_OBJECT
public:
    cOwnTab(QWidget *par = NULL);
signals:
    // A widget-et be kell csukni, el kell távolítani ...
    void closeIt();
public slots:
    // Egy closeIt() signál
    void endIt();
};


#endif // LV2G_H
