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

GUI alkalmazásokban a lanView helyett annak leszármazotját az lv2Gui objektumot (vagy leszármazottját)
kell példányosítani, a program inicializálásához. Szintén érdemes lecserélni az alapértelmezett
QApplication::notify() metódust, mivel alapesetben a Qt nem támogatja a kizárások kezekését.
A fentiek miatt nem a QApplication példányosításával indítjuk a programunkat, hanem a cLv2GQAll osztály
példányosításával (ami a QApplication leszármazottja, és fellüldefiniálja a notify() metódust. Lássd: a lv2gui.cpp fájlt.

A könyvtár a következő elemeket valósítja meg:\n
@section modells Model osztályok (Fej állomány : lv2models.h )
- cStringListModel  String lista megjelenítése és kezelése
- cStringListDecModel Dekorált string lista model
- cPolygonTableModel Polygon táblázatos megjelenítése
- cRecordListModel Rekord lista megjelenítése, minden rekordot a neve azonosít
- cZoneListModel    Zónák megjelenítése, lista model
- cPlacesInZoneModel Helyiség nevek egy megadott zónában, lista model
- cEnumListModel Egy enumerációs típus elemeinek a dekorált lista modelje
- cRecFieldSetOfValueModel Lista model, egy mező eddigi értékei alapján
- cResourceIconsModel Lista modell
Adattáblák megjelenítése (külön fejállományben).
- cRecordTableModel Adattábla táblázatos megjelenítés a cTableShape alapján
- cRecordTreeModel  Adattábla fa megjelenítés a cTableShape alapján
@section validators Validátor osztályok
- cIntValidator Validator egy numerikus egész típusú mezőhöz.
- cRealValidator Validator egy numerikus típusú mezőhöz.
- cMacValidator Validator egy MAC típusú mezőhöz.
- cINetValidator Input validátor a INET adattípushoz. Csak a hoszt cím van engedélyezve.
- cCidrValidator Input validátor a CIDR adattípushoz. (hálózati cím tartomány)
- cItemDelegateValidator
@section widgets Widget-ek, ill. widget kezelő osztályok. Mezők meglenítése, szerkesztése
Widgetek:
- cImageWidget Egy képet megjelenítő widget.
- cLineWidget Egy soros editor + egy NULL toolButton.
- cComboLineWidget Egy comboBox + egy NULL toolButton
- cROToolButton Engedélyezetten inaktív toolButton
- cStringMapEdit

Widget(ek)et kezelő osztályok
- cSelectLanguage Nyelv kiválasztása
- cSelectPlace Helyiség kiválasztása
- cSelectNode Eszköz kiválasztása
- cSelectLinkedPort
- cSelectVlan
- cSelectSubNet

Az adat mező widgetek, cFieldEditBase és leszármazottai.
- cNullWidget "Nincs elegendő jogosultsága"
- cSetWidget Egy set típusú (enumerációs tömb)
- cEnumRadioWidget Egy enumerációs (vagy bool)
- cEnumComboWidget Egy enumerációs (vagy bool)
- cFieldLineWidget Szöveges mező, egy sorban.
- cFieldSpinBoxWidget spinBox
- cArrayWidget Egy tömb típusú adatmező
- cPolygonWidget Egy polygon
- cFKeyWidget Egy távoli kulcs mező
- cTimeWidget Egy időpont
- cDateWidget Egy dátum
- cDateTimeWidget Egy dátum, és idópont
- cIntervalWidget Egy idő intervallum
- cBinaryWidget Bináris adat
- cFKeyArrayWidget Távoli kulcsok tömb
- cColorWidget Szín
- cFontFamilyWidget Font család (név)
- cFontAttrWidget Font attributumok
- cLTextWidget  Nyelvi szöveg
- cFeatureWidget    Feature mező

@section guihigh Magasabb szintű GUI API osztályok
- cDialogButtons
- cRecordTable
- cRecordTree
- cRecordLink
- cRecordDialog
- cRecordDialogTab

A cIntSubObj bázisosztály és leszármazottai:
- cSetupWidget
- cGSetupWidget
- cParseWidget
- cExportsWidget
- cOnlineAlarm
- cErrcodesWidget
- cHSOperate
- cFindByMac
- cWorkstation
- cDeducePatch
- cSnmpDevQuery
- cEnumValsEdit
- cTranslator
- cChangeSwitch

@section enumdec Enumerációs típusokhoz kapcsolt dekorációt támogató függvények
Az enumerációs típusoknak kettős szerepe van a rendszerben. Azon túl, hogy egy adat típust reprezentálnak,
rajtuk keresztűl valósul meg a GUI dekorációs funkciói: háttér és karakter szín, font család és attributumok,
ikonok.

Az enumerációkat leíró ill. kezelő osztályok (nem a GUI könvtár része):
- cColEnumType  Az SQL enumerációs típus leírója
- cEnumVal  A dekorációs adatokat tartalmazó rekordot reprezentáló osztály

A rendszer használ egy típust, aminek csak dekorációs szerepe van, ez a datacharacter.
Ez egy az SQL-ben is megjelenő ill. deklarált típus, de mint adat típusnak nincs szerepe.
Továbbá a bool típusú mezőknél is megadhatóak dekorációs paraméterek. Ebben az esetben
egy hipotetikus enumerációs típuson keresztűl, melynek a neve a tábla és mező névből
képződik: <tábla név>.<mező név> pl.: 'services.disabled'. Ezek, mint SQL típusok nem léteznek,
így az ennek megfeleltethető cColEnumType típusú objektum sem.

Az enumerációs típusokhoz, illetve a típus értékeihez rendelt dekorációs paraméterek lekérdezését végzik.
Az adatokat a memóriába pufferelik, csökkentendő az adatbázis műveletek számát. Emiatt ha változás történik,
akkor az csak a program újraindítása után érvényesül.
- const QColor& bgColorByEnum(const QString& __t, int e)
- const QColor& bgColorByBool(const QString& _tn, const QString& _fn, bool f)
- const QColor& dcBgColor(int id)
- const QColor& fgColorByEnum(const QString& __t, int e)
- const QColor& fgColorByBool(const QString& _tn, const QString& _fn, bool f)
- const QColor& dcFgColor(int id)
- const QFont& fontByEnum(const QString& __t, int _e)
- const QFont& fontByEnum(const cEnumVal& ev)
- const QFont& dcFont(int id)
- void enumSetColor(QWidget *pW, const QString& _t, int id)
- void enumSetBgColor(QWidget *pW, const QString& _t, int id)
- void enumSetFgColor(QWidget *pW, const QString& _t, int id)
- void dcSetColor(  QWidget *pW, int id)
- void dcSetBgColor(QWidget *pW, int id)
- void dcSetFbColor(QWidget *pW, int id)
- void enumSetD(QWidget *pW, const cEnumVal& ev, int id = NULL_IX)
- void enumSetD(QWidget *pW, const QString& _t, int id)
- void enumSetD(QWidget *pW, const QString& _t, int id, qlonglong ff, int dcId)
- void dcSetD(QWidget *pW, int id)
- void enumSetShort(W *pW, const QString& __t, int id, const QString& _ds)
- void enumSetShort(W *pW, const QString& __t, int id, const QString& _ds, qlonglong ff, int dcId)
- void dcSetShort(W *pW, int id)
- QVariant enumRole(const cEnumVal& ev, int role, int e)
- QVariant enumRole(const QString& _t, int id, int role, const QString& dData)
- QVariant dcRole(int id, int role)
*/
#include "lanview.h"
#include "lv2g_global.h"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  LV2G
#undef ERCODES_H_DECLARE
#include "errcodes.h"
#include "lv2data.h"
#include "guidata.h"
#include <QtWidgets>

class lv2g;
class cMainWindow;

class LV2GSHARED_EXPORT lv2g : public lanView {
public:
    lv2g();
    ~lv2g();
    static lv2g*    getInstance(void) { return static_cast<lv2g *>(lanView::getInstance()); }
    static QMdiArea *pMdiArea();
    void            changeZone(QWidget * par = nullptr);
    static bool     logonNeeded;
    static bool     zoneNeeded;
    qlonglong       zoneId;
    int             maxRows;
    int             dialogRows;
    enum Qt::Orientation    defaultSplitOrientation;
    QString                 soundFileAlarm;
    bool                    nativeMenubar;
    // Identifier strings
    static const QString    sDefaultSplitOrientation;
    static const QString    sMaxRows;
    static const QString    sDialogRows;
    static const QString    sSoundFileAlarm;
    static const QString    sHorizontal;
    static const QString    sVertical;
    static const QString    sNativeMenubar;

    /// Main window object pointer
    static cMainWindow *    pMainWindow;
    static QSplashScreen *  pSplash;
    static void splashMessage(const QString& msg);

    static QIcon        iconNull;
    static QIcon        iconDefault;
};

// Icon from resouce
/// Az resource-ban elérhető ikonok mappa neve
_GEX const QString iconBaseName;
/// Az resource-ban elérhető ikonok listája
_GEX const QStringList& resourceIconList();
_GEX int indexOfResourceIcon(const QString& _s);
inline QString resourceIconPath(const QString& s) { return s.isEmpty() ? s : iconBaseName + s; }
inline QString resourceIconPath(int ix)           { const QStringList& il = resourceIconList(); return isContIx(il, ix) ? resourceIconPath(il.at(ix)) : _sNul; }
inline QIcon resourceIcon(const QString& s)       { return s.isEmpty() ? QIcon() : QIcon(resourceIconPath(s)); }
inline QIcon resourceIcon(int ix)                 { QString s = resourceIconPath(ix); return s.isEmpty() ? QIcon() : QIcon(s); }
template <typename K> QVariant resourceIcon2Variant(const K& k) { QString s = resourceIconPath(k); return s.isEmpty() ? QVariant() : QVariant(QIcon(s)); }
inline QVariant string2variant(const QString& s)  { return s.isEmpty() ? QVariant() : QVariant(s); }

/// A mező típusához (ID, név, távoli kulcs, stb. ) rendelhető datacharacter értéket adja meg
/// @param __d A rekord leíró objektum
/// @param __ix A mező indexe
_GEX int defaultDataCharacter(const cRecStaticDescr& __d, int __ix);


inline QWidget *newFrame(int _st, QWidget * p = nullptr)
{
    QFrame *pFrame = new QFrame(p);
    pFrame->setFrameStyle(_st);
    return pFrame;
}
inline QWidget *newHLine(QWidget * p = nullptr) { return newFrame(QFrame::HLine, p); }
inline QWidget *newVLine(QWidget * p = nullptr) { return newFrame(QFrame::VLine, p); }

class cMenuAction;
// A  megjelenítéshez egy segéd osztály,
class LV2GSHARED_EXPORT cIntSubObj : public QWidget {
    friend class cMenuAction;
    Q_OBJECT
public:
    cIntSubObj(QMdiArea *par);
    QWidget *pWidget() { return static_cast<QWidget *>(this); }
protected:
    QMdiSubWindow *pSubWindow;

signals:
    // A widget-et be kell csukni, el kell távolítani ...
    void closeIt();
public slots:
    // Egy closeIt() signál
    void endIt();
};

/// @class cLv2GQApp
/// Saját QApplication osztály, a hiba kizárások elkapásához (újra definiált notify() metódus.)
class LV2GSHARED_EXPORT cLv2GQApp : public QApplication {
public:
    /// Konstruktor. Nincs saját inicilizálás, csak a QApplication konstrujtort hívja.
    cLv2GQApp(int& argc, char ** argv);
    ~cLv2GQApp();
    /// Az újra definiált notify() metódus.
    /// Az esetleges kizárásokat is elkapja, az eredeti metódus hívását egy try blokkba helyezi.
    /// Hiba ill. kizárás esetén egyrészt a cError::mDropAll statikus adattagot igazra állítja,
    /// majd megjeleníti a hibát egy hiba ablakban, feltéve, hogy a hibakód nem eError::EOK ,
    /// Ezután hívja az QAplication::exit() metódust a hiba koddal.
    /// Ha az cError::mDropAll igazra van állítva, akkor a hiba esetén a cError objektum pointer helyett
    /// a kizárás a _no_init_ (dummy) objektummal történik (a hiba objektum el lessz dobva),
    /// A notify() a _no_init_ hiba objektum elkapásakor nem csinál semmit, mivel ekkor már egy
    /// megjelenített hiba után vagyunk, a kilépés fázisában.
    virtual bool notify(QObject * receiver, QEvent * event);
};

/// Egy tPoligonF típusú objektumnak QPolygonF objektummá konvertálása.
/// A QPolygonF osztályt a Qt a GUI modulban tartalmazza, így az az lv2
/// számára nem elérhető, igy helyette a tPoliginF típust használja.
_GEX QPolygonF convertPolygon(const tPolygonF& __pol);

/// Egy enumerációs típus egy értékéhez rendelt háttérszín lekérdezése.
/// A színeket puffereli, így változás esetén azok nem frissülnek.
/// Az alapértelmezett szín a fehér.
/// @param __t Az enumerációs típus neve
/// @param e Az enumerációs érték indexxe (0,1,...)
/// @return az értékhez rendelt szín
_GEX const QColor& bgColorByEnum(const QString& __t, int e);

/// Egy bool típusú mező értékeihez rendelt háttérszín lekérdezése.
/// A színeket puffereli, így változás esetén azok nem frissülnek.
/// Az alapértelmezett szín a fehér.
/// @param _tn A boolean típusú mezőt tartalmazó tábla neve
/// @param _fn A boolean típusú mező neve
/// @param f A boolean érték
/// @return az értékhez rendelt szín
inline const QColor& bgColorByBool(const QString& _tn, const QString& _fn, bool f)
{
    return bgColorByEnum(mCat(_tn, _fn), f ? 0 : 1);
}

/// Egy enumerációs típus egy értékéhez rendelt karakterszín lekérdezése.
/// A színeket puffereli, így változás esetén azok nem frissülnek.
/// Az alapértelmezett szín a fekete.
/// @param __t Az enumerációs típus neve
/// @param e Az enumerációs érték indexxe (0,1,...)
/// @return az értékhez rendelt szín
_GEX const QColor& fgColorByEnum(const QString& __t, int e);

/// Egy bool típusú mező értékeihez rendelt karakterszín lekérdezése.
/// A színeket puffereli, így változás esetén azok nem frissülnek.
/// Az alapértelmezett szín a fekete.
/// @param _tn A boolean típusú mezőt tartalmazó tábla neve
/// @param _fn A boolean típusú mező neve
/// @param e 0: false, 1: true
/// @return az értékhez rendelt szín
inline const QColor& fgColorByBool(const QString& _tn, const QString& _fn, bool f)
{
    return fgColorByEnum(mCat(_tn, _fn), f ? 0 : 1);
}

/// Font az enumerációs típus és (numerikus) érték alapján.
/// A függvény minden értéket egy belső táblában gyűjt, ami nem frissül.
/// @param __t Az enum típus neve
/// @param _e Az enumerációs érték numerikusan
_GEX const QFont& fontByEnum(const QString& __t, int _e);
inline const QFont& fontByEnum(const cEnumVal& ev) {
    return fontByEnum(ev.typeName(), ev.toInt());
}

inline const QColor&  dcBgColor(int id)   { return bgColorByEnum(_sDatacharacter, id); }
inline const QColor&  dcFgColor(int id)   { return fgColorByEnum(_sDatacharacter, id); }
inline const QFont&   dcFont(int id)      { return fontByEnum(   _sDatacharacter, id); }

static inline void enumSetColor(QWidget *pW, const QString& _t, int id) {
    const QColor& bgc = bgColorByEnum(_t, id);
    const QColor& fgc = fgColorByEnum(_t, id);
    if (bgc.isValid() || fgc.isValid()) {
        QPalette palette = pW->palette();
        if (bgc.isValid()) { palette.setColor(QPalette::Base, bgc); }
        if (fgc.isValid()) { palette.setColor(QPalette::Text, fgc); }
        pW->setPalette(palette);
    }
}

static inline void enumSetBgColor(QWidget *pW, const QString& _t, int id) {
    const QColor& bgc = bgColorByEnum(_t, id);
    if (bgc.isValid()) {
        QPalette palette = pW->palette();
        if (bgc.isValid()) { palette.setColor(QPalette::Base, bgc); }
        pW->setPalette(palette);
    }
}

static inline void enumSetFgColor(QWidget *pW, const QString& _t, int id) {
    const QColor& fgc = fgColorByEnum(_t, id);
    if (fgc.isValid()) {
        QPalette palette = pW->palette();
        if (fgc.isValid()) { palette.setColor(QPalette::Text, fgc); }
        pW->setPalette(palette);
    }
}

inline void dcSetColor(  QWidget *pW, int id) { enumSetColor(  pW, _sDatacharacter, id); }
inline void dcSetBgColor(QWidget *pW, int id) { enumSetBgColor(pW, _sDatacharacter, id); }
inline void dcSetFbColor(QWidget *pW, int id) { enumSetFgColor(pW, _sDatacharacter, id); }

/// Widget kinézetének a beállítása a megadott enumeráció típus és értékhez rendelt paraméterek alapján
/// @param pW A beállítandó wideget pointere.
/// @param ev Enumerációs értékhez tartozó líró objektum
/// @param id Az enumeráció numerikus értéke (opcionális, ev-böl kideríthető)
_GEX void enumSetD(QWidget *pW, const cEnumVal& ev, int id = NULL_IX);
/// Widget kinézetének a beállítása a megadott enumeráció típus és értékhez rendelt paraméterek alapján
/// @param pW A beállítandó wideget pointere.
/// @param _t Enumerációs típus neve
/// @param id Az enumeráció numerikus értéke
_GEX void enumSetD(QWidget *pW, const QString& _t, int id);

/// Widget kinézetének a beállítása a megadott enumeráció típus és értékhez rendelt paraméterek alapján
/// feltételesen, figyelembe véve a megadott field_flags értéket.
/// @param pW A beállítandó wideget pointere.
/// @param _t Enumerációs típus neve
/// @param id Az enumeráció numerikus értéke
_GEX void enumSetD(QWidget *pW, const QString& _t, int id, qlonglong ff, int dcId = DC_DATA);

/// Widget kinézetének a beállítása az alap data_character típus nevű enumerációhoz
/// rendelt paraméterek alapján.
/// @param pW A beállítandó wideget pointere.
/// @param id Az enumeráció numerikus értéke
inline void dcSetD(QWidget *pW, int id) { enumSetD(pW, _sDatacharacter, id); }

/// Widget kinézetének a beállítása a megadott enumeráció típus és értékhez rendelt paraméterek alapján,
/// Továbbá a widget szövegének  beállítása a viewShort mező alapján.
/// @param pW A beállítandó wideget pointere.
/// @param _t Enumerációs típus neve
/// @param id Az enumeráció numerikus értéke
/// @param _ds Az alapértelmezett szöveg, ha nics az enumerációhoz paraméter rekord (cEnumVal/"enum_vals").
template <class W> void enumSetShort(W *pW, const QString& __t, int id, const QString& _ds) {
    enumSetD(pW, __t, id);
    pW->setText(cEnumVal::viewShort(__t, id, _ds));
}

/// Widget kinézetének a beállítása a megadott enumeráció típus és értékhez rendelt paraméterek alapján
/// feltételesen, figyelembe véve a megadott field_flags értéket.
/// Továbbá a widget szövegének  beállítása a viewShort mező alapján.
/// @param pW A beállítandó wideget pointere.
/// @param _t Enumerációs típus neve
/// @param id Az enumeráció numerikus értéke
/// @param _ds Az alapértelmezett szöveg, ha nics az enumerációhoz paraméter rekord (cEnumVal/"enum_vals").
/// @param ff A mező szerkesztő leíróban a field_flags mező numerikus értéke
template <class W> void enumSetShort(W *pW, const QString& __t, int id, const QString& _ds, qlonglong ff, int dcId = DC_DATA) {
    enumSetD(pW, __t, id, ff, dcId);
    pW->setText(cEnumVal::viewShort(__t, id, _ds));
}

/// Widget kinézetének a beállítása az alap data_character típus nevű enumerációhoz
/// rendelt paraméterek alapján. Továbbá a widget szövegének  beállítása a viewShort mező alapján.
/// @param pW A beállítandó wideget pointere.
/// @param id Az enumeráció numerikus értéke
template <class W> void dcSetShort(W *pW, int id) {
    dcSetD(pW, id);
    pW->setText(cEnumVal::viewShort(_sDatacharacter, id, dataCharacter(id)));
}

/// Model data() metódusában használható függvény, az enumerációs objektum alapján adja vissza az adatot
/// @param ev Az enumerációs értékhez tartozó objektum.
/// @param role A data() metódus role paramétere
/// @param e Az enumeráció numerikus értéke, ha ismert (gyorsabb, ha meg van adva)
_GEX QVariant enumRole(const cEnumVal& ev, int role, int e = NULL_IX);
/// Model data() metódusában használható függvény, az enumerációs objektum alapján adja vissza az adatot
/// @param _t At enumerációs típus neve
/// @param id Az enumerációs érték (numerikus)
/// @param role A data() metódus role paramétere
/// @param dData Az alapértelmezett megjelenítendő szüveg
_GEX QVariant enumRole(const QString& _t, int id, int role, const QString& dData);
/// Model data() metódusában használható függvény, a datacharacter enumerációs objektum alapján adja vissza az adatot
/// @param id Az enumerációs érték (numerikus)
/// @param role A data() metódus role paramétere
_GEX QVariant dcRole(int id, int role);

_GEX QString condAddJoker(const QString& pat);

#endif // LV2G_H
