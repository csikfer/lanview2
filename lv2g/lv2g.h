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
- cRecordTreeModel
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
class lv2gDesign;
class cMainWindow;


// inline static const lv2gDesign& design();

class LV2GSHARED_EXPORT lv2g : public lanView {
//    friend const lv2gDesign& design();
public:
    lv2g();
    ~lv2g();
    static lv2g*    getInstance(void) { return (lv2g *)lanView::getInstance(); }
    static bool     logonNeeded;
    static bool     zoneNeeded;
    qlonglong       zoneId;
    int             maxRows;
    int             dialogRows;
    enum Qt::Orientation    defaultSplitOrientation;
    QString                 soundFileAlarm;
    // Identifier strings
    static const QString    sDefaultSplitOrientation;
    static const QString    sMaxRows;
    static const QString    sDialogRows;
    static const QString    sSoundFileAlarm;
    static const QString    sHorizontal;
    static const QString    sVertical;
    /// Main window object pointer
    static cMainWindow *    pMainWindow;
protected:
//    const lv2gDesign *pDesign;
};

/*
inline static const lv2gDesign& design()
{
    return *(lv2g::getInstance()->pDesign);
}

class LV2GSHARED_EXPORT colorAndFont {
    friend class lv2gDesign;
public:
    static QPalette& palette();
    QFont   font;
    QColor  fg;
    QColor  bg;
protected:
    colorAndFont();
    static QPalette *pal;
};

enum eDesignRole {
    GDR_INVALID = -1,   ///< Csak hibajelzésre
    GDR_HEAD = 0,       ///< Táblázat fejléce
    GDR_DATA,           ///< egyébb adat mező
    GDR_ID,             ///< ID mező
    GDR_NAME,           ///< Név mező
    GDR_PRIMARY,        ///< elsődleges kulcs (de nem ID vagy Név)
    GDR_KEY,            ///< kulcs mező (de nem elsődleges, ID vagy Név)
    GDR_FNAME,          ///< távoli kulcs, a hivatkozott rekord név mezője
    GDR_DERIVED,        ///< távoli kulcs, a hivatkozott rekordból származtatott név jellegű adat
    GDR_TREE,           ///< saját rekordra mutató távoli kulcs (gráf vagy fa)
    GDR_FOREIGN,        ///< távoli kulcs érték
    GDR_NULL,           ///< NULL adat
    GDR_WARNING,        ///< Figyelmeztető üzenet
    GDR_COLOR  = 0x100, ///< Egyedi szín
    GDR_FONT   = 0x200  ///< Egyedi font
};


class LV2GSHARED_EXPORT lv2gDesign : public QObject {
    Q_OBJECT
protected:
    friend class lv2g;
    lv2gDesign(QObject *par);
    ~lv2gDesign();
public:
    QString titleWarning;
    QString titleError;
    QString titleInfo;

    QString valNull;
    QString valDefault;
    QString valAuto;

    colorAndFont    head;
    colorAndFont    data;
    colorAndFont    name;
    colorAndFont    id;
    colorAndFont    primary;
    colorAndFont    key;
    colorAndFont    fname;
    colorAndFont    derived;
    colorAndFont    tree;
    colorAndFont    foreign;
    colorAndFont    null;
    colorAndFont    warning;
    const colorAndFont&   operator[](int role) const;
    static eDesignRole desRole(const cRecStaticDescr& __d, int __ix);
};
*/

_GEX int defaultDataCharter(const cRecStaticDescr& __d, int __ix);

// typedef QList<QHBoxLayout *> hBoxLayoutList;
// typedef QList<QLabel *>     labelList;

static inline QWidget *newFrame(int _st, QWidget * p = NULL)
{
    QFrame *pFrame = new QFrame(p);
    pFrame->setFrameStyle(_st);
    return pFrame;
}
static inline QWidget *newHLine(QWidget * p = NULL) { return newFrame(QFrame::HLine, p); }
static inline QWidget *newVLine(QWidget * p = NULL) { return newFrame(QFrame::VLine, p); }

class cMenuAction;
// A  megjelenítéshez egy segéd osztály,
class LV2GSHARED_EXPORT cIntSubObj : public QWidget {
    friend class cMenuAction;
    Q_OBJECT
public:
    cIntSubObj(QMdiArea *par);
    QWidget *pWidget() { return (QWidget *)this; }
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
    /// Az esetleges kizárásokat is elkapja.
    virtual bool notify(QObject * receiver, QEvent * event);
};

_GEX QPolygonF convertPolygon(const tPolygonF __pol);

_GEX const QColor& bgColorByEnum(const QString& __t, int e);
inline const QColor& bgColorByBool(const QString& _tn, const QString& _fn, int e)
{
    return bgColorByEnum(mCat(_tn, _fn), e);
}

_GEX const QColor& fgColorByEnum(const QString& __t, int e);
inline const QColor& fgColorByBool(const QString& _tn, const QString& _fn, int e)
{
    return fgColorByEnum(mCat(_tn, _fn), e);
}

_GEX const QFont& fontByEnum(const QString& __t, int _e);

static inline const QColor&  dcBgColor(int id)   { return bgColorByEnum(_sDatacharacter, id); }
static inline const QColor&  dcFgColor(int id)   { return fgColorByEnum(_sDatacharacter, id); }
static inline const QFont&   dcFont(int id)      { return fontByEnum(_sDatacharacter, id); }

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

static inline void dcSetColor(QWidget *pW, int id) { enumSetColor(pW, _sDatacharacter, id); }
static inline void dcSetBgColor(QWidget *pW, int id) { enumSetBgColor(pW, _sDatacharacter, id); }
static inline void dcSetFbColor(QWidget *pW, int id) { enumSetFgColor(pW, _sDatacharacter, id); }

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
static inline void dcSetD(QWidget *pW, int id) { enumSetD(pW, _sDatacharacter, id); }

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

_GEX QVariant enumRole(const cEnumVal& ev, int role, int e);
_GEX QVariant enumRole(const QString& _t, int id, int role, const QString& dData);
_GEX QVariant dcRole(int id, int role);

#endif // LV2G_H
