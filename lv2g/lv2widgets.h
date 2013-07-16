#ifndef LV2WIDGETS_H
#define LV2WIDGETS_H

#include <QLabel>
#include <QButtonGroup>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QTableView>
#include <QMouseEvent>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QFileDialog>
#include <QMessageBox>


#include "lv2g.h"
#include "lv2models.h"


/// @class cImageWindow Egy képet tartalmazó ablak objektum
class cImageWindow : public QLabel {
    Q_OBJECT
public:
    /// Konstruktor
    /// @par __par Szülő widget pointere
    cImageWindow(QWidget *__par = NULL);
    /// Destruktor
    ~cImageWindow();
    /// Az ablak tartaémának a betöltése egy kép fájlból, és az ablak megjelenítése
    /// @par __fn A kép fájl neve
    /// @par __t Opcionális paraméter, az ablak címe
    /// @return Ha sikerült a kép betöltése, akkor true, ha nem akkor false (ekkor az ablak nem kerül megjelenítésre).
    bool setImage(const QString& __fn, const QString& __t = QString());
    /// Az ablak tartaémának a betöltése az adatbázisból
    /// @par __q Az adatbázis lekérdezéshez használlt objektum.
    /// @par __id A kép rekord ID-je
    /// @par __t Opcionális paraméter, az ablak címe, nincs megadva, akkor az image_descr mező lessz a cím
    /// @return Ha sikerült a kép betöltése, akkor true, ha nem akkor false (ekkor az ablak nem kerül megjelenítésre).
    bool setImage(QSqlQuery __q, qlonglong __id, const QString& __t = QString());
    /// Az ablak tartaémának a betöltése ecFieldEditBasegy cImage objektumból
    /// @par __o A kép et tartalmazó frltöltött cImage objektum referenciája
    /// @par __t Opcionális param1c:6f:65:4c:84:1aéter, az ablak címe, nincs megadva, akkor az image_descr mező lessz a cím
    /// @return Ha sikerült a kép betöltése, akkor true, ha nem akkor false (ekkor az ablak nem kerül megjelenítésre).
    bool setImage(const cImage& __o, const QString& __t = QString());
    /// Metódus az egér kattintásra
    virtual void mousePressEvent(QMouseEvent * ev);
    /// A kép objektum pointere, a konstruktor NULL-ra inicializálja. A destruktor pedig felszabadítja, ha nemm NULL.
    QPixmap *   image;
signals:
    /// Szignál, ha kattintottak ez egérrel a képen.
    /// @par A kattintáskori egér pozíció a képen.
    void mousePressed(const QPoint& __p);
};

enum eSyncType {
    SY_NO       = 0,    ///< Nincs szinkrom a mező érték és a widgeten megjelenített érték között
    SY_TO_REC   = 1,    ///< A widget-en módosított adat megjelenik a rekord mezőben
    SY_FROM_REC = 2,    ///< A rekord mezőn a változások szinkronizálódnak a widgetre
    SY_FULL     = 3     ///< Bármely oldalon történik változás, az rekord mezőben és a widget-en is ervényesül.
};

enum eFieldWidgetType {
    FEW_UNKNOWN = -1,   /// ismeretlen
    FEW_SET     =  0,   /// cSetWidget
    FEW_ENUM_COMBO,     /// cEnumComboWidget
    FEW_ENUM_RADIO,     /// cEnumRadioWidget
    FEW_LINE,           /// cFieldLineWidget
    FEW_ARRAY,          /// cArrayWidget
    FEW_POLYGON,        /// cPolygonWidget
    FEW_FKEY,           /// cFKeyVideget
    FEW_DATE,
    FEW_TIME,
    FEW_DATE_TIME,
    FEW_INTERVAL,
    FEW_BINARY,
    FEW_RO_LINE
};
/// Az enum eFieldWidgetType értékeket stringgé konvertálja.
/// Vissza konverzió nincs, ez is csak nyomkövetési céllal.
QString fieldWidgetType(int _t);

/// @class cFieldEditBase
/// @brief Bázis objekktum a mező megjelenítő/módosító widget-ekhez
class LV2GSHARED_EXPORT cFieldEditBase : public QObject {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _sy A szinkronizálás típusa, alapértelmezetten nincs szinkronizálás
    /// @param _ro Ha true nem szerkeszthető. Egyes leszármazottnál a true nem értelmezhető, ekkor kizárást dob a konstruktor
    ///            Ha értéke true, akkor a sync hívás csak értékadás a widget-re.
    cFieldEditBase(const cTableShape& _tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * _par = NULL);
    /// Destruktor
    ~cFieldEditBase();
    /// Az objektum típusát megadó enumerációs értékkel tér vissza (_wType adattag értéke)
    eFieldWidgetType wType() const { return _wType; }
    /// A rekord és a widget közötti szinkron típusa
    eSyncType syncType() const { return _syType; }
    /// A megjelenített/editált érték kiolvasása
    virtual QVariant  get()     const;
    /// A megjelenített/editált érték kiolvasása
    virtual qlonglong getId()   const;
    /// A megjelenített/editált érték kiolvasása
    virtual QString   getName() const;
    /// A magjelenített érték megadása
    /// @return Ha az értékadás nem sikeres, akkor -1, ha nincs változás, akkor 0, és ha van változás akkor 1.
    virtual int set(const QVariant& _v)   = 0;
    /// A magjelenített érték megadása
    /// @return Ha az értékadás nem sikeres, akkor -1, ha nincs változás, akkor 0, és ha van változás akkor 1.
    virtual int setId(qlonglong v);
    /// A magjelenített érték megadása
    /// @return Ha az értékadás nem sikeres, akkor -1, ha nincs változás, akkor 0, és ha van változás akkor 1.
    virtual int setName(const QString& v);
    /// Ha a widget readOnly true-vel, ha szerkeszthető, akkor false-val tér vissza
    bool isReadOnly() const { return _readOnly; }
    ///
    QWidget&    widget()    { return *_pWidget; }
    QWidget    *pWidget()   { return  _pWidget; }
    ///
    static cFieldEditBase *createFieldWidget(const cTableShape &_tm, cRecordFieldRef _fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * _par = NULL);
    /// A Widget-ben megjelenített értéket adja vissza. ld.: get() metódust.
    operator QVariant() const   { return get(); }
    /// A Widget-ben megjelenített értéket adja vissza, qlonglong-á konvertálva, Konvertáló függvény a mező leíró objektum szerint.
    operator qlonglong() const  { return getId(); }
    /// A Widget-ben megjelenített értéket adja vissza, QString-é konvertálva, Konvertáló függvény a mező leíró objektum szerint.
    operator QString() const    { return getName(); }
    /// A widgethez rendelt rekord objektum mező értékével tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    /// @return A mező értéke, ahogyan azt a rekord objektum tárolja.
    QVariant fieldValue()                   { if (_pFieldRef == NULL) EXCEPTION(EPROGFAIL); return *_pFieldRef; }
    /// A widgethez rendelt rekord objektum mező értékével tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    /// @return A mező értéke, stringgé konvertálva, a mező leíró szerint stringgé konvertálva.
    QString fieldToName()                   { if (_pFieldRef == NULL) EXCEPTION(EPROGFAIL); return (QString)*_pFieldRef; }
    /// A widgethez rendelt rekord objektum leíró objektumával tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    const cRecStaticDescr& recDescr() const { if (_pFieldRef == NULL) EXCEPTION(EPROGFAIL); return _pFieldRef->recDescr(); }

    /// A mező leíró objektum referenciája
    const cColStaticDescr&  _descr;
    /// A tábla megjelenítését (és a dialog megjelenítését) leíró objektum referenciája
    const cTableShape&      _tableShape;
protected:
    void _setv(QVariant v);
    /// Inicializálás csak olvasható widget esetén
    bool initIfRo(QWidget *par = NULL);
    /// Csak olvasható widget esetén a fó widget típusa QLineEdit ennek a pointerét adja vissza (konvertálva a _pWidget adattagot.
    /// Nem ellenörzi, hogy a _pWidget valóban ilyen típusú-e, ill. hogy valóban csak olvasható a widget.
    QLineEdit * pRoWidget() { return (QLineEdit *)_pWidget; }
    /// Csak olvasható widget esetén beállítja a megjelenítendő értéket. Nincs ellenörzés.
    void setRoLine()        { pRoWidget()->setText(_descr.toView(*pq, _value)); }

    /// A mező referencia objektuma pointere
    cRecordFieldRef       *_pFieldRef;
    /// Ha nem szerkeszthető, akkor értéke true
    bool                _readOnly;
    eFieldWidgetType    _wType;
    eSyncType           _syType;
    QVariant            _value;
    QWidget            *_pWidget;
    QSqlQuery          *pq;
protected slots:
    void modRec();
    void modField(int ix);
};

/// @class cSetWidget
/// @brief Egy set típusú (enumerációs tömb) adatmező megjelenítése, ill. szerkesztésére
/// használlható QWidget osztály. A megjelenítés/szerkesztés radio-button-okon keresztül történik.
class LV2GSHARED_EXPORT cSetWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A megjelenítő leíró objektum referenciája.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _sy A szinkronizálás típusa, alapértelmezetten nincs szinkronizálás
    /// @oaram _ro Ha true nem szerkeszthető
    /// @param parent A parent widget pointere
    cSetWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cSetWidget();
    virtual int set(const QVariant& v);
protected:
    /// A radio-button-okat kezelő obkeltum
    QButtonGroup   *pButtons;
    /// A radio-button-okat tartalamzó layer.
    QBoxLayout     *pLayout;
    /// Az aktuális érték (aradio-button-hoz rendelt bit a radio-button állapota.
    qlonglong       _bits;
private slots:
    /// A megadot id-jű (sorszám) radio-button értéke megváltozott
    void _set(int id);
};

/// @class cEnumRadioWidget
/// @brief Egy enumerációs (vagy bool) adatmező megjelenítése, ill. szerkesztésére
/// használlható QWidget osztály.
/// A megjelenítés RadioButtonok segítségével történik
class LV2GSHARED_EXPORT cEnumRadioWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A megjelenítő leíró objektum referenciája.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @oaram _ro Ha true nem szerkeszthető
    /// @param par A parent widget pointere
    cEnumRadioWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cEnumRadioWidget();
    virtual int set(const QVariant& v);
protected:
    /// A radio-button-okat kezelő obkeltum
    QButtonGroup   *pButtons;
    /// A radio-button-okat tartalamzó layer.
    QBoxLayout     *pLayout;
    qlonglong       eval;
private slots:
    void _set(int id);
};

/// @class cEnumComboWidget
/// @brief Egy enumerációs (vagy bool) adatmező megjelenítése, ill. szerkesztésére
/// használlható QWidget osztály.
/// A megjelenítés ComboBox segítségével történik
class LV2GSHARED_EXPORT cEnumComboWidget : public cFieldEditBase  {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @oaram _ro A true értéknek nincs értelme, kizárást dob
    /// @param parent A parent widget pointere
    cEnumComboWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * parent = NULL);
    ~cEnumComboWidget();
    virtual int set(const QVariant& v);
protected:
    void setWidget();
    qlonglong  eval;
private slots:
    void _set(int id);
};

class cArrayWidget;

/// @class cFieldLineWidget
/// Egy adatmező megjelenítése és módosítása
/// Megjelenítés egy QLineEdit -el
class LV2GSHARED_EXPORT cFieldLineWidget : public cFieldEditBase {
    friend class cArrayWidget;
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _sy Adat szinkronozálás típusa
    /// @param _ro Ha nem szerkeszthető, értéke true, ebben az esetben a sync csak értékadás a widget-re
    /// @param parent A parent widget pointere
    cFieldLineWidget(const cTableShape &_tm, cRecordFieldRef _fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cFieldLineWidget();
    virtual int set(const QVariant& v);
protected:
    /// Védett konstruktor. cArrayWidget osztályhoz.
    /// Nem lehet ReadOnly, adat szinkronizálás csak a valid slot-on keresztül.
    /// Csak olyan adat típus engedélyezett, aminél az Array típus kezelve vean.
    cFieldLineWidget(const cColStaticDescr& _cd, QWidget * par);
    QLineEdit *pLineEdit() { return (QLineEdit *)pWidget(); }
signals:
    void valid();
private slots:
    void _set();
};


/// @class cArrayWidget
/// Egy tömb adatmező megjelenítése és módosítása
class LV2GSHARED_EXPORT cArrayWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _ro Ha nem szerkeszthető, értéke true
    /// @param parent A parent widget pointere
    cArrayWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * parent = NULL);
    ~cArrayWidget();
    virtual int set(const QVariant& v);
protected:
    void setButtons();
    QHBoxLayout *pLayout;
    QVBoxLayout *pLeftLayout;
    QVBoxLayout *pRightLayout;

    QListView   *pList;
    cStringListModel *pModel;
    QLineEdit   *pLineEd;

    QPushButton *pAddButton;
    QPushButton *pDelButton;
    QPushButton *pClearButton;

//  QStringList array;
    /// Az inzertálandó adat legutoljára elfogadott értéke.
    QString     last;
private slots:
    void delRow();
    void addRow();
    void clearRows();
    void changed(QString _t);
    void selectionChanged(QItemSelection,QItemSelection);
};


/// @class cPolygonWidget
/// Egy tömb adatmező megjelenítése és módosítása
class LV2GSHARED_EXPORT cPolygonWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @oaram _ro Ha nem szerkeszthető, értéke true
    /// @param parent A parent widget pointere
    cPolygonWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cPolygonWidget();
    virtual int set(const QVariant& v);
protected:
    void setButtons();
    QHBoxLayout *pLayout;

    QVBoxLayout *pLeftLayout;
    QTableView  *pTable;
    cPolygonTableModel *pModel;
    QHBoxLayout *pXYLayout;
    QLabel      *pLabelX;
    QLineEdit   *pLineX;
    QLabel      *pLabelY;
    QLineEdit   *pLineY;

    QVBoxLayout *pRightLayout;
    QPushButton *pAddButton;
    QPushButton *pDelButton;
    QPushButton *pClearButton;

    tPolygonF   polygon;
    bool        xOk, yOk;
    QString     xPrev, yPrev;
    double      x,y;
private slots:
    void delRow();
    void addRow();
    void clearRows();
    void xChanged(QString _t);
    void yChanged(QString _t);
    void selectionChanged(QItemSelection,QItemSelection);
};

/// @class cFKeyWidget
/// Egy tömb adatmező megjelenítése és módosítása
class LV2GSHARED_EXPORT cFKeyWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cFKeyWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * parent = NULL);
    ~cFKeyWidget();
    virtual int set(const QVariant& v);
protected:
    QComboBox * pComboBox() { return (QComboBox *)_pWidget; }
    bool setWidget();
    cRecordListModel   *pModel;
    const cRecStaticDescr *pRDescr;
private slots:
    void _set(int i);
//  void _edited(QString _txt);
};

/// @class cDateWidget
/// Egy dátun
class LV2GSHARED_EXPORT cDateWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cDateWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cDateWidget();
    virtual int set(const QVariant& v);
private slots:
    void _set(QDate d);
};

/// @class cTimeWidget
/// Egy dátun
class LV2GSHARED_EXPORT cTimeWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cTimeWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cTimeWidget();
    virtual int set(const QVariant& v);
private slots:
    void _set(QTime d);
};

/// @class cDateTimeWidget
/// Egy dátun
class LV2GSHARED_EXPORT cDateTimeWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cDateTimeWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cDateTimeWidget();
    virtual int set(const QVariant& v);
private slots:
    void _set(QDateTime d);
};

/// @class cIntervalWidget
/// Egy intervallum
class LV2GSHARED_EXPORT cIntervalWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cIntervalWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * par = NULL);
    ~cIntervalWidget();
    virtual int set(const QVariant& v);
protected:
    void view();
    qlonglong getFromWideget() const;
    QHBoxLayout    *pLayout;
    QLineEdit      *pLineEdDay;
    QLabel         *pLabelDay;
    QTimeEdit      *pTimeEd;
    QIntValidator  *pValidatorDay;
private slots:
    void _set();
};

/// @class cBinaryWidget
/// Egy dátun
class LV2GSHARED_EXPORT cBinaryWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor, hívja a sync() metódust is.
    /// A widget-en megjelenő értékeket szinkronizálja a hivatkozott mezőben.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cBinaryWidget(const cTableShape &_tm, cRecordFieldRef __fr, eSyncType _sy = SY_NO, bool _ro = false, QWidget * parent = NULL);
    ~cBinaryWidget();
    virtual int set(const QVariant& v);
protected:
    void init();
    QHBoxLayout    *pLayout;
    QRadioButton   *pRadioButton;
    QPushButton    *pLoadButton;
    QByteArray      data;
private slots:
    void load();
    void setNull(bool checked);
};

#endif // LV2WIDGETS_H
