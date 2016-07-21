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
#include "imagedrv.h"

/// @class cImageWidget
/// Egy képet tartalmazó ablak objektum
class LV2GSHARED_EXPORT cImageWidget : public QScrollArea {
    Q_OBJECT
public:
    /// Konstruktor
    /// @par __par Szülő widget pointere
    cImageWidget(QWidget *__par = NULL);
    /// Destruktor
    ~cImageWidget();
    /// Az ablak tartalmának a betöltése egy kép fájlból, és az ablak megjelenítése
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
    /// Az ablak tartaémának a betöltése egy cImage objektumból
    /// @par __o A kép et tartalmazó frltöltött cImage objektum referenciája
    /// @par __t Opcionális paraméter, az ablak címe, nincs megadva, akkor az image_descr mező lessz a cím
    /// @return Ha sikerült a kép betöltése, akkor true, ha nem akkor false (ekkor az ablak nem kerül megjelenítésre).
    bool setImage(const cImage& __o, const QString& __t = QString());
    /// Egy szöveg megjelenítése a kép helyett.
    bool setText(const QString& _txt);
    /// Metódus az egér kattintásra
    virtual void mousePressEvent(QMouseEvent * ev);
    ///
    void center(QPoint p);
    /// Zoom szorzó
    double          scaleStep;
protected:
    bool resetImage();
    void draw();
    void draw(QPainter &painter, QVariant& d);
    QLabel *        pLabel;
    /// Az eredeti kép objektum
    QPixmap         image;
    int             scale;
    QVariantList    draws;
    QBrush          brush;
    QPen            pen;
public:
    cImageWidget& setBrush(const QBrush& b)     { brush = b;    return *this; }
    cImageWidget& setPen(const QPen& p)         { pen = p;      return *this; }
    cImageWidget& setDraws(const QVariantList&d){ draws = d;    return *this; }
    cImageWidget& addDraw(const QVariant& d)    { draws << d;   return *this; }
    cImageWidget& clearDraws()                  { draws.clear();return *this; }
signals:
    /// Szignál, ha kattintottak ez egérrel a képen.
    /// @param A kattintáskori egér pozíció a képen.
    void mousePressed(const QPoint& __p);
public slots:
    void zoomIn();
    void zoomOut();

};

enum eFieldWidgetType {
    FEW_UNKNOWN = -1,   ///< ismeretlen/inicializálatlan/hibajelzés
    FEW_SET     =  0,   ///< cSetWidget
    FEW_ENUM_COMBO,     ///< cEnumComboWidget
    FEW_ENUM_RADIO,     ///< cEnumRadioWidget
    FEW_LINE,           ///< cFieldLineWidget
    FEW_TEXT,           ///< cFieldLineWidget/long text
    FEW_ARRAY,          ///< cArrayWidget
    FEW_FKEY_ARRAY,     ///< cFKeyArrayWidget
    FEW_POLYGON,        ///< cPolygonWidget
    FEW_FKEY,           ///< cFKeyWidget
    FEW_DATE,           ///< cDateWidget
    FEW_TIME,           ///< cTimeWidget
    FEW_DATE_TIME,      ///< cDateTimeWidget
    FEW_INTERVAL,       ///< cIntervalWidget
    FEW_BINARY,         ///< cBinaryWidget
    FEW_NULL            ///< cNullWidget
};
/// Az enum eFieldWidgetType értékeket stringgé konvertálja.
/// Vissza konverzió nincs, ez is csak nyomkövetési céllal.
_GEX QString fieldWidgetType(int _t);

class cRecordDialogBase;

/// @class cFieldEditBase
/// @brief Bázis objekktum a mező megjelenítő/módosító widget-ekhez
/// Az objektum nem a QWidget leszármazotja, a konkrét megjelenítéshez létrehozott widget
/// pointere egy adattag.
class LV2GSHARED_EXPORT cFieldEditBase : public QObject {
    friend class cRecordDialog;
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A rekord megjelenítését leíró objektum referenciája
    /// @param _tf A mező megjelenítését leíró objektum referenciája
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _ro Ha true nem szerkeszthető. Egyes leszármazottnál a true nem értelmezhető, ekkor kizárást dob a konstruktor.
    /// @param parent A parent widget pointere
    cFieldEditBase(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
    /// Destruktor
    ~cFieldEditBase();
    /// Az objektum típusát megadó enumerációs értékkel tér vissza (_wType adattag értéke)
    eFieldWidgetType wType() const { return _wType; }
    /// A megjelenített/editált érték kiolvasása
    virtual QVariant  get()     const;
    /// A megjelenített/editált érték kiolvasása
    virtual qlonglong getId()   const;
    /// A megjelenített/editált érték kiolvasása
    virtual QString   getName() const;
    /// A magjelenített érték megadása.
    /// @return Ha az értékadás nem sikeres, akkor -1, ha nincs változás, akkor 0, és ha van változás akkor 1.
    virtual int set(const QVariant& v);
    /// A magjelenített érték megadása. Az alapértelmezett metódus a QVarian-á kovertált paraméterrel
    /// hívja a int set(const QVariant& v) metódust.
    /// @return Ha az értékadás nem sikeres, akkor -1, ha nincs változás, akkor 0, és ha van változás akkor 1.
    virtual int setId(qlonglong v);
    /// A magjelenített érték megadása. Az alapértelmezett metódus a QVarian-á kovertált paraméterrel
    /// hívja a int set(const QVariant& v) metódust.
    /// @return Ha az értékadás nem sikeres, akkor -1, ha nincs változás, akkor 0, és ha van változás akkor 1.
    virtual int setName(const QString& v);
    /// Ha a widget readOnly true-vel, ha szerkeszthető, akkor false-val tér vissza
    bool isReadOnly() const { return _readOnly; }
    ///
    QWidget&    widget()    { return *_pWidget; }
    QWidget    *pWidget()   { return  _pWidget; }
    /// Egy megfelelő típusú widget objektum létrehozása
    /// @param _tm A tábla megjelenítését/szerkesztését leíró objektum
    /// @param _tf A mező megjelenítését/szerkesztését leíró objektum
    /// @param _fr A mező referencia objektuma
    /// @param _ro Ha a mező nem szerkeszthető, akkor true
    /// @param _par Parent
    /// @return A létrehozott objektum pointere
    static cFieldEditBase *createFieldWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, bool _ro, cRecordDialogBase *_par);
    /// A Widget-ben megjelenített értéket adja vissza. ld.: get() metódust.
    operator QVariant() const   { return get(); }
    /// A Widget-ben megjelenített értéket adja vissza, qlonglong-á konvertálva, Konvertáló függvény a mező leíró objektum szerint.
    operator qlonglong() const  { return getId(); }
    /// A Widget-ben megjelenített értéket adja vissza, QString-é konvertálva, Konvertáló függvény a mező leíró objektum szerint.
    operator QString() const    { return getName(); }
    // / A widgethez rendelt rekord objektum mező értékével tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    // / @return A mező értéke, ahogyan azt a rekord objektum tárolja.
//  QVariant fieldValue()                   { if (_pFieldRef == NULL) EXCEPTION(EPROGFAIL); return *_pFieldRef; }
    // / A widgethez rendelt rekord objektum mező értékével tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    // / @return A mező értéke, stringgé konvertálva, a mező leíró szerint stringgé konvertálva.
//  QString fieldToName()                   { if (_pFieldRef == NULL) EXCEPTION(EPROGFAIL); return (QString)*_pFieldRef; }
    /// A widgethez rendelt mező objektum inexével a rekordban tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    int fieldIndex() const { return _colDescr.fieldIndex(); }

    /// Parent objektum
    cRecordDialogBase *_pParentDialog;
    /// A mező leíró objektum referenciája
    const cColStaticDescr&  _colDescr;
    /// A tábla megjelenítését (és a dialog megjelenítését) leíró objektum referenciája
    const cTableShape&      _tableShape;
    /// A mezö megjelenítését (és a dialog megjelenítését) leíró objektum referenciája
    const cTableShapeField& _fieldShape;
    ///
    const cRecStaticDescr& _recDescr;
protected:
    void setFromWidget(QVariant v);
    cFieldEditBase * anotherField(const QString& __fn, eEx __ex = EX_ERROR);
//  cRecordFieldRef    *_pFieldRef;     ///< A mező referencia objektum pointere
    bool                _readOnly;      ///< Ha nem szerkeszthető, akkor értéke true
    bool                _nullable;      ///< Amező értéke NULL is lehet
    bool                _hasDefault;    ///< Ha a mezó rendelkezik alapértelmezett értékkel, akkor treu
    bool                _isInsert;      ///< Ha egy új rekord, akkor true, ha modosítás, akkor false
    QString             _nullView;      ///< Ha megadható NULL érték, akkor annak a megjelenése (NULL vagy Default)
    eFieldWidgetType    _wType;         ///< A widget típusa (a leszármazott objektumot azonosítja)
    QVariant            _value;         ///< A mező aktuális értéke
    QWidget            *_pWidget;       ///< A megjelenítéshez létrejozptt QWidget (valós widget objektum pointere)
    QSqlQuery          *pq;             ///< Amennyiben szükslges a megjelenítéshez adatbázis hozzáférés, akkor a QSqlQuery objektum pointere.
/*
protected slots:
    void modRec();                  ///< A rekord módosult, aktualizálandó a megjelenítés
    void modField(int ix);          ///< A mező módosult, aktualizálandó a megjelenítés
*/
signals:
    void changedValue(cFieldEditBase * pSndr);
};

/// @class cNullWidget
/// @brief Nincs elegendő jogosultság, fix szöveg megjelenítése.
class LV2GSHARED_EXPORT cNullWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A megjelenítő leíró objektum referenciája.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _ro Ha true nem szerkeszthető
    /// @param parent A parent widget pointere
    cNullWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
    ~cNullWidget();
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
    /// @param _ro Ha true nem szerkeszthető
    /// @param parent A parent widget pointere
    cSetWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
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
    void setFromEdit(int id);
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
    /// @param _ro Ha true nem szerkeszthető
    /// @param par A parent widget pointere
    cEnumRadioWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
    ~cEnumRadioWidget();
    virtual int set(const QVariant& v);
protected:
    /// A radio-button-okat kezelő obkeltum
    QButtonGroup   *pButtons;
    /// A radio-button-okat tartalamzó layer.
    QBoxLayout     *pLayout;
    qlonglong       eval;
private slots:
    void setFromEdit(int id);
};

/// @class cEnumComboWidget
/// @brief Egy enumerációs (vagy bool) adatmező megjelenítése, ill. szerkesztésére
/// használlható QWidget osztály.
/// A megjelenítés ComboBox segítségével történik
class LV2GSHARED_EXPORT cEnumComboWidget : public cFieldEditBase  {
    Q_OBJECT
public:
    /// Konstruktor (nincs csak olvasható mód)
    /// @param _tm A megjelenítő leíró objektum referenciája.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent
    cEnumComboWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par);
    ~cEnumComboWidget();
    virtual int set(const QVariant& v);
protected:
    void setWidget();
    qlonglong  eval;
private slots:
    void setFromEdit(int id);
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
    /// @param _ro Ha nem szerkeszthető, értéke true.
    /// @param parent A parent widget pointere
    cFieldLineWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, bool _ro, cRecordDialogBase* _par);
    ~cFieldLineWidget();
    virtual int set(const QVariant& v);
protected:
    /// Védett konstruktor. cArrayWidget osztályhoz.
    /// Nem lehet ReadOnly, adat szinkronizálás csak a valid slot-on keresztül.
    /// Csak olyan adat típus engedélyezett, aminél az Array típus kezelve vean.
    cFieldLineWidget(const cColStaticDescr& _cd, QWidget * par);
    /// Az eredeti widget pointerrel tér visszta
    QLineEdit *pLineEdit() {
        QLineEdit *pLE = qobject_cast<QLineEdit *>(pWidget());
        if (pLE == NULL) EXCEPTION(EPROGFAIL);
        return pLE;
    }
    QPlainTextEdit *pTextEdit() {
        QPlainTextEdit *pTE = qobject_cast<QPlainTextEdit *>(pWidget());
        if (pTE == NULL) EXCEPTION(EPROGFAIL);
        return pTE;
    }
    /// Ha  ez egy jelszó
    bool    isPwd;
signals:
    void valid();
private slots:
    void setFromEdit();
};

class Ui_arrayEd;
/// @class cArrayWidget
/// Egy tömb adatmező megjelenítése és módosítása
class LV2GSHARED_EXPORT cArrayWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _ro Ha nem szerkeszthető, értéke true
    /// @param _par A parent pointere
    cArrayWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase *_par);
    ~cArrayWidget();
    virtual int set(const QVariant& v);
protected:
    void setButtons();
    Ui_arrayEd       *pUi;
    cStringListModel *pModel;
    /// Az inzertálandó adat legutoljára elfogadott értéke.
    QString     last;
    QModelIndex actIndex;
    int         selectedNum;
private slots:
    void selectionChanged(QModelIndex cur, QModelIndex);
    void changed(QString _t);
    void addRow();
    void insRow();
    void modRow();
    void upRow();
    void downRow();
    void delRow();
    void clrRows();
    void doubleClickRow(const QModelIndex & index);
};

class Ui_polygonEd;
/// @class cPolygonWidget
/// Egy polygon típusú adatmező megjelenítése és módosítása
class LV2GSHARED_EXPORT cPolygonWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _ro Ha nem szerkeszthető, értéke true
    /// @param parent A parent widget pointere
    cPolygonWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
    ~cPolygonWidget();
    virtual int set(const QVariant& v);
protected:
    enum ePic {
        NO_ANY_PIC,
        IS_PLACE_REC,
        ID2IMAGE_FUN
    };
    bool getImage(bool refresh = false);
    void drawPolygon();
    void modPostprod(QModelIndex select = QModelIndex());
    void setButtons();
    QModelIndex index(int row) { return pModel->index(row, 0); }
    QModelIndex actIndex(eEx __ex = EX_ERROR);
    int actRow(enum eEx __ex = EX_ERROR);
    cPolygonTableModel *pModel;
    Ui_polygonEd    *pUi;
    cImagePolygonWidget*pMapWin;       ///< Az opcionális alaprajzot megjelenítő ablak
    cImage *     pCImage;       ///< Az alaprajz rekord

    tPolygonF   polygon;
    bool        xOk, yOk, xyOk;
    QString     xPrev, yPrev;
    double      x,y;
    enum ePic   epic;
    QString     id2imageFun;
    qlonglong   parentOrPlace_id;
    int         selectedRowNum;
    int         _actRow;
    QModelIndex _actIndex;
    QPointF     lastPos;
    static qreal stZoom;    ///< Global zoom

private slots:
    void tableSelectionChanged(const QItemSelection&, const QItemSelection&);
    void tableDoubleclicked(const QModelIndex& mi);
    void xChanged(QString _t);
    void yChanged(QString _t);
    void addRow();
    void insRow();
    void upRow();
    void downRow();
    void modRow();
    void delRow();
    void clearRows();
    void imageOpen();
    void zoom(double z);

    void destroyedImage(QObject *p);
    void changeId(cFieldEditBase * p);
    void setted(QPolygonF pol);
};

class Ui_fKeyEd;

/// @class cFKeyWidget
/// Egy távoli kulcs mező megjelenítése, és szerkesztése
class LV2GSHARED_EXPORT cFKeyWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor Nincs readOnly mód
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cFKeyWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par);
    ~cFKeyWidget();
    virtual int set(const QVariant& v);
protected:
    bool setWidget();
    Ui_fKeyEd          *pUi;
    cRecordListModel   *pModel;
    const cRecStaticDescr *pRDescr;
    cTableShape *   pTableShape;
private slots:
    void setFromEdit(int i);
//  void _edited(QString _txt);
    void insertF();
    void modifyF();
};

/// @class cDateWidget
/// Egy dátun megjelenítése ill. szerkesztése
class LV2GSHARED_EXPORT cDateWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cDateWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase* _par);
    ~cDateWidget();
    virtual int set(const QVariant& v);
private slots:
    void setFromEdit(QDate d);
};

/// @class cTimeWidget
/// Egy időpont megjelenítése ill. szerkesztése
class LV2GSHARED_EXPORT cTimeWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param par A parent widget pointere
    cTimeWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase* _par);
    ~cTimeWidget();
    virtual int set(const QVariant& v);
private slots:
    void setFromEdit(QTime d);
};

/// @class cDateTimeWidget
/// Egy dátun, és idópont megjelenítése ill. szerkesztése
class LV2GSHARED_EXPORT cDateTimeWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cDateTimeWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase* _par);
    ~cDateTimeWidget();
    virtual int set(const QVariant& v);
private slots:
    void setFromEdit(QDateTime d);
};

/// @class cIntervalWidget
/// Egy intervallum
class LV2GSHARED_EXPORT cIntervalWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cIntervalWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
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
    void setFromEdit();
};

/// @class cBinaryWidget
///
class LV2GSHARED_EXPORT cBinaryWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cBinaryWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, bool _ro, cRecordDialogBase* _par);
    ~cBinaryWidget();
    virtual int set(const QVariant& v);
protected:
    void _init();
    bool setCImage();
    void setNull();
    void setViewButtons();
    void closePic();
    void openPic();
    QHBoxLayout    *pLayout;
    QRadioButton   *pRadioButtonNULL;
    QPushButton    *pLoadButton;
    QPushButton    *pViewButton;
    QPushButton    *pZoomInButton;
    QPushButton    *pZoomOutButton;
    cImageWidget   *pImageWidget;
    QByteArray      data;
    /// A flag értéke true, ha a mező a cImage objektum része
    bool            isCImage;
    cImage         *pCImage;
private slots:
    void loadDataFromFile();
    void nullChecked(bool checked);
    void viewPic();
    void changedAnyField(cFieldEditBase * p);
    void destroyedImage(QObject *p);
};

#endif // LV2WIDGETS_H
