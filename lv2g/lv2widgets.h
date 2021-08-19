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
#include <QTableWidget>

#include "lv2g.h"
#include "lv2models.h"
#include "guidata.h"
#include "imagedrv.h"
#include "lv2link.h"
#include "vardata.h"

_GEX bool pixmap(const cImage& o, QPixmap &_pixmap);
_GEX bool setPixmap(const cImage& im, QLabel *pw);
_GEX bool setPixmap(QSqlQuery& q, qlonglong iid, QLabel *pw);

static inline qlonglong bitByButton(QAbstractButton *p, qlonglong m)
{
    if (p == nullptr) EXCEPTION(EPROGFAIL);
    return p->isChecked() ? m : 0;
}

/// @class tBatchBlocker
/// Egy objektumban szabályozza a kimeneti signálok kiadását, megakadályozandó,
/// hogy egy több lépéses műveletsoron bellül azok kiadásra kerüljenek a részeredményekkel.
template <class C> class tBatchBlocker {
private:
    /// Számláló, egy több lépéses művelet közben
    int batchCount;
    /// A kimeneti ssignal-okat kiadó metódus pointer típusa
    typedef bool (C::*tPEmits)(bool);
    /// A kimeneti signal-okat kiadó metódus pointere
    tPEmits pf;
    /// A tárgy objektum pointere
    C *     po;
public:
    /// Konstruktor
    /// @param _po Az objektum pointere, meljen a kimeneti signál-(ok)at szabályozni kell.
    /// @param _pf A kimeneti signal-okat kiadó metódus.
    tBatchBlocker(C *_po, tPEmits _pf) {
        batchCount = 0; pf = _pf; po = _po;
    }
    /// Destruktor.
    /// Ha a számláló nem nulla, akkor kiküld a hiba csatornára egy hibaüzenetet.
    ~tBatchBlocker() {
        if (batchCount != 0) DERR() << QString("Class %1; batchCount is not zero : %2.")
                                       .arg(typeid(C).name()).arg(batchCount)
                                    << endl;
    }
    /// Egy műveletsor indítása, amik közben a kimeneti signal(-ok) hívása tiltott
    /// A batchCount -ot inkrementálja.
    void begin() { ++batchCount; }
    /// Egy műveletsor vége, amik közben a kimeneti signal(-ok) hívása tiltott
    /// A batchCount -ot dekrementálja.
    /// Ha batchCount nulla lett és f paraméter értéke is true (ez az alapértelmezett),
    /// akkor hívja utána a pf pointerű metódust is a po objektumra,
    /// ami kiadja a szignálokat.
    /// @return true, ha vége a blokkos feldolgozásnak, batchCount értéke nulla lett.
    bool end(bool f = true) {
        if (batchCount <= 0) EXCEPTION(EPROGFAIL);
        --batchCount;
        if (f && batchCount == 0) {
            (po->*pf)(true);
            return true;
        }
        return false;
    }
    /// Teszteli, hogy szignálokat letíltó műveletsoron kívül vagyunk-e.
    bool test() const { return batchCount == 0; }
};

class LV2GSHARED_EXPORT cSelectLanguage : public QObject {
    Q_OBJECT
public:
    cSelectLanguage(QComboBox *_pComboBox, QLabel *pFlag, bool _nullable, QObject *_pPar);
    int currentLangId()         { qlonglong id = pModel->atId(pComboBox->currentIndex()); return id == NULL_ID ? NULL_IX : int(id); }
    bool refresh()              { return pModel->refresh(); }
    bool setCurrent(int lid)    { return pModel->setCurrent(lid); }
    bool setCurrent(const QString& s)   { return pModel->setCurrent(s); }
protected:
    QComboBox *pComboBox;
    cRecordListModel *pModel;
    QLabel           *pFlag;
private slots:
    void _languageChanged(int ix);
signals:
    void languageIdChanged(int id);
};

/// \brief setFormEditWidget
/// Egy QFormLayout -ban megkeresi a _lw pointerű cimke widgetet, és ugyanebbe a sorban elhelyezi a _ew widgetet mint érték.
/// \param _fl A QFormLayout pointere, amiben el kell helyezni a _ew widgetet.
/// \param _lw A keresett cimke widget
/// \param _ew Az elhelyezendő mező widget
/// \param __ex Ha értéke nem EX_IGNORE, akkor hiba esetén, vagyis, ha nem találja az _lw widgetet, vagy az nem cimke (LabelRole), akkor kizárást dob.
/// \return Ha nincs hiba, akkor true.
_GEX bool setFormEditWidget(QFormLayout *_fl, QWidget *_lw, QWidget *_ew, eEx __ex = EX_ERROR);

/// \brief The cLineWidget class
/// Egy soros editor + egy NULL toolButton
class LV2GSHARED_EXPORT cLineWidget : public QWidget {
    Q_OBJECT
public:
    cLineWidget(QWidget *par = nullptr, bool _ro = false, bool _horizontal = true);
    QLayout     * const pLayout;
    QLineEdit   * const pLineEdit;
    QToolButton * const pNullButton;

    void setNullIcon (const QIcon& icon) { pNullButton->setIcon(icon); }
    void set(const QVariant& val);
    bool isNull()  { return pNullButton->isChecked(); }
    QVariant get() { return isNull() ? QVariant() : QVariant(pLineEdit->text()); }
private:
    QVariant val;
private slots:
    void on_NullButton_togled(bool f);
    void on_LineEdit_textChanged(const QString& s);
public slots:
    void setDisabled(bool f);
    void setEnabled(bool f) { setDisabled(!f); }
signals:
    void changed(const QVariant& val);
};

/// \brief The cComboLineWidget class
/// Egy comboBox + egy NULL toolButton
class LV2GSHARED_EXPORT cComboLineWidget : public QWidget {
    Q_OBJECT
public:
    cComboLineWidget(const cRecordFieldRef& _cfr, const QString& sqlWhere = QString(), bool _horizontal = true, QWidget *par = nullptr);
    QLayout     * const pLayout;
    QComboBox   * const pComboBox;
    QToolButton * const pNullButton;

    void setNullIcon (const QIcon& icon) { pNullButton->setIcon(icon); }
    void set(const QVariant& val, Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive));
    bool isNull()  { return pNullButton->isChecked(); }
    QVariant get() { return isNull() ? QVariant() : QVariant(pComboBox->currentText()); }
    void refresh() { pModel->refresh(); }
private:
    cRecFieldSetOfValueModel *pModel;
    QVariant val;
private slots:
    void on_NullButton_togled(bool f);
    void on_ComboBox_textChanged(const QString& s);
public slots:
    void setDisabled(bool f);
    void setEnabled(bool f) { setDisabled(!f); }
signals:
    void changed(const QVariant& val);
};

inline QFrame *line(int __w, int __mw, QWidget *par = nullptr)
{
    QFrame *line = new QFrame(par);
    line->setLineWidth(__w);
    line->setMidLineWidth(__mw);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

inline QFrame *line(QWidget *par = nullptr)
{
    QFrame *line = new QFrame(par);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    return line;
}

/// @class cImageWidget
/// Egy képet tartalmazó ablak objektum
class LV2GSHARED_EXPORT cImageWidget : public QScrollArea {
    Q_OBJECT
public:
    /// Konstruktor
    /// @par __par Szülő widget pointere
    cImageWidget(QWidget *__par = nullptr);
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
    /// @par __t Opcionális paraméter, az ablak címe, nincs megadva, akkor az image név mező lessz a cím
    /// @return Ha sikerült a kép betöltése, akkor true, ha nem akkor false (ekkor az ablak nem kerül megjelenítésre).
    bool setImage(QSqlQuery __q, qlonglong __id, const QString& __t = QString());
    /// Az ablak tartaémának a betöltése egy cImage objektumból
    /// @par __o A kép et tartalmazó frltöltött cImage objektum referenciája
    /// @par __t Opcionális paraméter, az ablak címe, nincs megadva, akkor az image neve lessz a cím
    /// @return Ha sikerült a kép betöltése, akkor true, ha nem akkor false (ekkor az ablak nem kerül megjelenítésre).
    bool setImage(const cImage& __o, const QString& __t = QString());
    /// Egy szöveg megjelenítése a kép helyett.
    bool setText(const QString& _txt);
    /// Metódus az egér kattintásra
    virtual void mousePressEvent(QMouseEvent * ev);
    ///
    void center(QPoint p);
    static double   scale;
protected:
    bool resetImage();
    void draw();
    void draw(QPainter &painter, QVariant& d);
    QLabel *        pLabel;
    /// Az eredeti kép objektum
    QPixmap         image;
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
    void zoom(double z);
};

/// @class cROToolButton
/// "Read only" QToolButton
class LV2GSHARED_EXPORT cROToolButton : public QToolButton {
public:
    cROToolButton(QWidget *par = nullptr) : QToolButton(par) { ; }
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
};


enum eFieldWidgetType {
    FEW_UNKNOWN = -1,   ///< ismeretlen/inicializálatlan/hibajelzés
    FEW_SET     =  0,   ///< cSetWidget
    FEW_ENUM_COMBO,     ///< cEnumComboWidget
    FEW_ENUM_RADIO,     ///< cEnumRadioWidget
    FEW_LINE,           ///< cFieldLineWidget
    FEW_LINES,          ///< cFieldLineWidget/long text
    FEW_HTML_LINES,     ///< cFieldLineWidget/long html text
    FEW_COMBO_BOX,      ///< cFieldLineWidget/combo box
    FEW_SPIN_BOX,       ///< cFieldSpinBoxWidget
    FEW_ARRAY,          ///< cArrayWidget
    FEW_FKEY_ARRAY,     ///< cFKeyArrayWidget
    FEW_POLYGON,        ///< cPolygonWidget
    FEW_FKEY,           ///< cFKeyWidget
    FEW_DATE,           ///< cDateWidget
    FEW_TIME,           ///< cTimeWidget
    FEW_DATE_TIME,      ///< cDateTimeWidget
    FEW_INTERVAL,       ///< cIntervalWidget
    FEW_BINARY,         ///< cBinaryWidget
    FEW_NULL,           ///< cNullWidget
    FEW_COLOR,          ///< cColorWidget
    FEW_FONT_FAMILY,    ///< cFontFamilyWidget
    FEW_FONT_ATTR,      ///< cFontAttrWidget
    FEW_LTEXT,          ///< cLTextWidget
    FEW_LTEXT_LONG,     ///< cLTextWidget/long
    FEW_LTEXT_HTML,     ///< cLTextWidget/html
    FEW_FEATURES,       ///< cFeatureWidget
    FEW_PARAM_VALUE     ///< cParamValueWidget
};
/// Az enum eFieldWidgetType értékeket stringgé konvertálja.
/// Vissza konverzió nincs, ez is csak nyomkövetési céllal.
_GEX QString fieldWidgetType(int _t);

class cRecordDialogBase;

enum eLV2WidgetFlags {
    FEB_EDIT      = 0,
    FEB_READ_ONLY = 1,
    FEB_INSERT    = 2,
    FEB_YET_EDIT  = 4
};

/// @class cFieldEditBase
/// @brief Bázis objekktum a mező megjelenítő/módosító widget-ekhez
/// Az objektum nem a QWidget leszármazotja, a konkrét megjelenítéshez létrehozott widget
/// pointere egy adattag.
class LV2GSHARED_EXPORT cFieldEditBase : public QWidget {
    friend class cRecordDialog;
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A rekord megjelenítését leíró objektum referenciája
    /// @param _tf A mező megjelenítését leíró objektum referenciája
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _fl Flag-ek : eLV2WidgetFlags
    /// @param parent A parent widget/dialógus objektum pointere
    cFieldEditBase(const cTableShape& _tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
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
    /// Egy megfelelő típusú 'widget' objektum létrehozása
    /// @param _tm A tábla megjelenítését/szerkesztését leíró objektum
    /// @param _tf A mező megjelenítését/szerkesztését leíró objektum
    /// @param _fr A mező referencia objektuma
    /// @param _fl
    /// @param _par Parent
    /// @return A létrehozott objektum pointere
    static cFieldEditBase *createFieldWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par);
    /// A Widget-ben megjelenített értéket adja vissza. ld.: get() metódust.
    operator QVariant() const   { return get(); }
    /// A Widget-ben megjelenített értéket adja vissza, qlonglong-á konvertálva, Konvertáló függvény a mező leíró objektum szerint.
    operator qlonglong() const  { return getId(); }
    /// A Widget-ben megjelenített értéket adja vissza, QString-é konvertálva, Konvertáló függvény a mező leíró objektum szerint.
    operator QString() const    { return getName(); }
    /// A widgethez rendelt mező objektum inexével a rekordban tér vissza, ha nincs mező rendelve a widgethez, akkor dob egy kizárást.
    int fieldIndex() const { return _colDescr.fieldIndex(); }
    /// A widget magassága ~sor. Az alap metódus 1-et ad vissza.
    int rowNumber() { return _height; }
    /// Parent objektum (opcionális, ha nincs dialógus parent, akkor NULL)
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
    /// Az érték beállítása a widget felöl
    /// @param v A widget-ban mgadott új érték
    void setFromWidget(QVariant v);
    /// Beállítja ai _isDisabledEdit flag-et, ha tsf értéke nem TS_NULL.
    /// Ezután, ha a pEditWidget nem null, akkor hívja a setDisabled metódusát az _isDisabledEdit -e.
    virtual void disableEditWidget(eTristate tsf = TS_NULL);
    /// Beállítja ai _isDisabledEdit flag-et f-re.
    /// Ezután, ha a pEditWidget nem null, akkor hívja a setDisabled metódusát az _isDisabledEdit -e.
    void disableEditWidget(bool f) { disableEditWidget(f ? TS_TRUE : TS_FALSE); }
    /// Ha _readOnly hamis, akkor opcionálisan létrehozza a NULL vagy default nyomógombot (QToolButton).
    /// Beállítja a nyomógomb kinézetét, és ha nem paraméterként adtuk meg a gomb pointerét, akkor
    /// hozzáadja a pLayout pointerű Layout-hoz is.
    /// Ha _readOnly igaz, akkor egy QLabel objektumot allokál. Ezt csak akkor adja hozá a pLayout
    /// pointerű Layout-hoz, ha p NULL volt.
    /// @param isNull Ha igaz akkor a nyomógomb checked állpotú lessz.
    /// @param p Ha értéke NULL, akkor a metódus allokálja meg a nyomógomb objektumot, ha nem null,
    /// akkor az a használandó objektumra mutat. Ha viszont _readOnly igaz, törli az objektumot.
    /// @return A nyomógomb, vagy a cimke pointerével tér vissza.
    QWidget *setupNullButton(bool isNull = false, QAbstractButton * p = nullptr);
    /// A parent dialógusban egy másik mező szerkesztő objektumot keresi meg a .
    /// @param __fn A keresett mező leíró neve (cTableShapeField.getName())
    /// @param __ex Ha értéke nem EX_IGNORE és nincs parent, vagy men találja az objektumot, akkor kizárást dob.
    /// @return A talált objektum pointere, vagy NULL.
    cFieldEditBase * anotherField(const QString& __fn, eEx __ex = EX_ERROR);
    QBoxLayout         *pLayout;        ///< Main layout
    QAbstractButton    *pNullButton;    ///< NULL button or default button or NULL pointer
    QWidget            *pEditWidget;
    int                 _height;        ///< A Widget hozzávetőleges magassága (karakter) sorokban
    bool                _readOnly;      ///< Ha nem szerkeszthető, akkor értéke true
    bool                _nullable;      ///< Amező értéke NULL is lehet
    bool                _hasDefault;    ///< Ha a mezó rendelkezik alapértelmezett értékkel, akkor true
    bool                _hasAuto;
    bool                _isInsert;      ///< Ha egy új rekord, akkor true, ha modosítás, akkor false. Az alapérték is false;
    bool                _actValueIsNULL;
    int                 _dcNull;        ///< Ha megadható NULL érték, akkor annak a megjelenése (NULL vagy Default)
    eFieldWidgetType    _wType;         ///< A widget típusa (a leszármazott objektumot azonosítja)
    QVariant            _value;         ///< A mező aktuális értéke
    QSize               iconSize;
    QSqlQuery          *pq;             ///< Amennyiben szükslges a megjelenítéshez adatbázis hozzáférés, akkor a QSqlQuery objektum pointere.
    cEnumVal            enumVal;
    QFont               font;
    QColor              bgColor;
    QColor              fgColor;
    cEnumVal            defEnumVal;
    QFont               defFont;
    QColor              defBgColor;
    QColor              defFgColor;
    QIcon               actNullIcon;
public:
    bool isText()   { return _wType == FEW_LTEXT || _wType == FEW_LTEXT_LONG; }
signals:
    void changedValue(cFieldEditBase * pSndr);
protected slots:
    virtual void togleNull(bool f);
    virtual void _setFromEdit();
};

/// @class cNullWidget
/// @brief Nincs elegendő jogosultság, fix szöveg megjelenítése.
class LV2GSHARED_EXPORT cNullWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A megjelenítő leíró objektum referenciája.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent widget pointere
    cNullWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase* _par);
    ~cNullWidget();
};

_GEX bool setWidgetAutoset(qlonglong& on, const QMap<int, qlonglong> autosets);
_GEX void nextColLayout(QHBoxLayout *pHLayout, QVBoxLayout *&pVLayout, int rows, int n);
/// @class cSetWidget
/// @brief Egy set típusú (enumerációs tömb) adatmező megjelenítése, ill. szerkesztésére
/// használlható QWidget osztály. A megjelenítés/szerkesztés radio-button-okon keresztül történik.
class LV2GSHARED_EXPORT cSetWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _tm A megjelenítő leíró objektum referenciája.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _fl
    /// @param parent A parent widget pointere
    cSetWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
    ~cSetWidget();
    void setChecked();
    virtual int set(const QVariant& v);
protected:
    /// A radio-button-okat kezelő obkeltum
    QButtonGroup   *pButtons;
    /// Az aktuális érték (aradio-button-hoz rendelt bit a radio-button állapota.
    qlonglong           _bits;
    int                 _nId;
    bool                _isNull;
    qlonglong           _defaults;
    qlonglong           _hiddens;
    QMap<int, qlonglong> _collisions;
    QMap<int, qlonglong> _autosets;

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
    /// @param _fl
    /// @param par A parent widget pointere
    cEnumRadioWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
    ~cEnumRadioWidget();
    virtual int set(const QVariant& v);
protected:
    /// A radio-button-okat kezelő obkeltum
    QButtonGroup   *pButtons;
    /// A radio-button-okat tartalamzó layer.
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
    virtual void togleNull(bool f);
    QComboBox *pComboBox;
    cEnumListModel *pModel;
    qlonglong  eval;
    qlonglong  evalDef;
//    eNullType  nulltype;
private slots:
    void setFromEdit(int id);
};

class cArrayWidget;

/// @class cFieldLineWidget
/// Egy adatmező megjelenítése és módosítása
/// Megjelenítés egy QLineEdit vagy QPlainTextEdit -tel
class LV2GSHARED_EXPORT cFieldLineWidget : public cFieldEditBase {
    friend class cArrayWidget;
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _fl
    /// @param parent A parent widget pointere
    cFieldLineWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase* _par);
    ~cFieldLineWidget();
    virtual int set(const QVariant& v);
protected:
    /// Védett konstruktor. cArrayWidget osztályhoz.
    /// Nem lehet ReadOnly, adat szinkronizálás csak a valid slot-on keresztül.
    /// Csak olyan adat típus engedélyezett, aminél az Array típus kezelve vean.
    cFieldLineWidget(const cColStaticDescr& _cd, QWidget * par);
    QLineEdit      *pLineEdit;          ///< One line edit
    QPlainTextEdit *pPlainTextEdit;     ///< Multi line edit (huge flag/features)
    QTextEdit      *pTextEdit;          ///< Multi line HTML edit (html_text flag/features)
    QComboBox      *pComboBox;          ///< ComboBox (setOfValues features)
    QAbstractItemModel *pModel;
    enum eModelType { NO_MODEL, SETOF_MODEL, ICON_MODEL }   modeltype;
    /// Ha  ez egy jelszó
    bool    isPwd;
protected slots:
    void _setFromEdit();
};

/// @class cFieldSpinBoxWidget
/// Egy adatmező megjelenítése és módosítása
/// Megjelenítés egy QSpinBox -szal
class LV2GSHARED_EXPORT cFieldSpinBoxWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _fl
    /// @param parent A parent widget pointere
    cFieldSpinBoxWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase* _par);
    ~cFieldSpinBoxWidget();
    virtual int set(const QVariant& v);
protected:
    QSpinBox *      pSpinBox;
    QDoubleSpinBox *pDoubleSpinBox;
protected slots:
    void _setFromEdit();
    void setFromEdit(int i);
    void setFromEdit(double d);
};

class Ui_arrayEd;
/// @class cArrayWidget
/// Egy tömb adatmező megjelenítése és módosítása
class LV2GSHARED_EXPORT cArrayWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _fl
    /// @param _par A parent pointere
    cArrayWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par);
    ~cArrayWidget();
    virtual int set(const QVariant& v);
protected:
    void disableEditWidget(eTristate tsf);
    void setButtons();
    Ui_arrayEd       *pUi;
    cStringListModel *pModel;
    /// Az inzertálandó adat legutoljára elfogadott értéke.
    QString     last;
    QModelIndex actIndex;
    int         selectedNum;
private slots:
    void _setFromEdit();
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
    /// @param _fl
    /// @param parent A parent widget pointere
    cPolygonWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
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
class Ui_placeEd;
class Ui_fKeyPlaceEd;
class Ui_fKeyPortEd;
class cSelectPlace;
class cSelectNode;

#define FKEY_BATCHEDIT  100

/// @class cFKeyWidget
/// Egy távoli kulcs mező megjelenítése, és szerkesztése
class LV2GSHARED_EXPORT cFKeyWidget : public cFieldEditBase {
    friend class cRecordsViewBase;
    Q_OBJECT
public:
    /// Konstruktor Nincs readOnly mód
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cFKeyWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef __fr, cRecordDialogBase *_par);
    ~cFKeyWidget();
    virtual int set(const QVariant& v);
    virtual QString getName() const;
protected:
    bool setWidget();
    void setButtons();
    void disableEditWidget(eTristate tsf);
    bool setConstFilter();
    void _refresh();
    Ui_fKeyEd          *pUi;            ///< Alap form távoli kulcshoz
    Ui_placeEd         *pUiPlace;       ///< Form egy hely kiválasztásához.
    Ui_fKeyPlaceEd     *pUiRPlace;      ///< Egy hely alapján szűrhető távoli kulcs
    Ui_fKeyPortEd      *pUiPort;        ///< Form egy port kiválasztásához.

    cRecordListModel   *pModel;         ///< record list model
    cSelectPlace       *pSelectPlace;   ///< Hely kiválasztása
    cSelectNode        *pSelectNode;    ///< eszköz kiválasztása
    QAbstractButton    *pButtonEdit;    ///< Hivatkozott objektum modosítása
    QAbstractButton    *pButtonAdd;     ///< Új hivatkozott objektum
    QAbstractButton    *pButtonRefresh; ///< Objektum lista frissítése
    QAbstractButton    *pButtonInfo;    ///< objektum info/riport
    QComboBox          *pComboBox;      ///< Objektum lista comboBox
    QDialog            *pParentBatchEdit;   // Ha batch editből történik a hívás, akkor egy pointer a dialógusra.
    /// A távoli kulcs által mutatott tábla leíró objektumára muatat
    const cRecStaticDescr *pRDescr;
    /// A távoli kulcs által mutatott tábla rekord dialógus leíró objektum.
    cTableShape *   pTableShape;
    qlonglong       actId;
    /// A távoli kulcs által mutatott táblában a saját ID-t tartalmazó mező indexe
    int             owner_ix;
    qlonglong       ownerId;
    int             ixRPlaceId; ///< Hivatkozott rekordban a place_id mező indexe.
    /// Típus
    enum eFilter {
        F_NO = 0,   ///< Nincs szűrési lehetőség, csak egy comboBox az objektumokról.
        F_SIMPLE,   ///< Egyszerű minta alapján szűrhető a lista
        F_PLACE,    ///< Egy hely kiválasztása (szűrés minta és zóna alapján)
        F_RPLACE,   ///< Az objektum lista szűrése zóna, hely, és minta alapján
        F_PORT      ///< port kiválasztása zóna, hely, eszköz és port név minta alapján.
    };
    enum sPortType { P_ALL, P_PATCH, P_NODE, P_SNMP } _pt;
    int            _filter;
protected slots:
    void setFromEdit(int i);
    void setFromEdit(qlonglong id);
    void _setFromEdit();
    void insertF();
    void modifyF();
    void modifyOwnerId(cFieldEditBase* pof);
    void setFilter(const QString &_s);
    void setPlace(qlonglong _pid);
    void setNode(qlonglong _nid);
    void refresh();
    void node2place();
    // Ráböktek az info (popup report) gombra.
    void info();
};


/// @class cDateWidget
/// Egy dátun megjelenítése ill. szerkesztése
class LV2GSHARED_EXPORT cDateWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cDateWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, cRecordDialogBase* _par);
    ~cDateWidget();
    virtual int set(const QVariant& v);
protected:
    QDateEdit * pDateEdit;
private slots:
    void _setFromEdit();
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
    cTimeWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, cRecordDialogBase* _par);
    ~cTimeWidget();
    virtual int set(const QVariant& v);
    virtual void disableEditWidget(eTristate tsf = TS_NULL);
protected:
    QTimeEdit *pTimeEdit;
    QToolButton *pFirstButton;
    QToolButton *pLastButton;
private slots:
    void _setFromEdit();
    void setFromEdit(QTime d);
    void setFirst();
    void setLast();
};

/// @class cDateTimeWidget
/// Egy dátun, és idópont megjelenítése ill. szerkesztése
class LV2GSHARED_EXPORT cDateTimeWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cDateTimeWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, cRecordDialogBase* _par);
    ~cDateTimeWidget();
    virtual int set(const QVariant& v);
protected:
    QDateTimeEdit * pDateTimeEdit;
private slots:
    void _setFromEdit();
    void setFromEdit(QDateTime d);
};

/// @class cIntervalWidget
/// Time interval
class LV2GSHARED_EXPORT cIntervalWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cIntervalWidget(const cTableShape &_tm, const cTableShapeField &_tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase* _par);
    ~cIntervalWidget();
    virtual int set(const QVariant& v);
protected:
    void disableEditWidget(eTristate tsf);
    void view();
    qlonglong getFromWideget() const;
    QLineEdit      *pLineEditDay;
    QLabel         *pLabelDay;
    QTimeEdit      *pTimeEdit;
    QIntValidator  *pValidatorDay;
private slots:
    void _setFromEdit();
};

/// @class cBinaryWidget
///
class LV2GSHARED_EXPORT cBinaryWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param parent A parent widget pointere
    cBinaryWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
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
    QPushButton    *pUpLoadButton;
    QPushButton    *pDownLoadButton;
    QPushButton    *pViewButton;
    QDoubleSpinBox *pDoubleSpinBoxZoom;
    QCheckBox      *pCheckBoxDetach;
    cImageWidget   *pImageWidget;
    QByteArray      data;
    /// A flag értéke true, ha a mező a cImage objektum része
    bool            isCImage;
    cImage         *pCImage;
private slots:
    void loadDataFromFile();
    void saveDataToFile();
    void nullChecked(bool checked);
    void viewPic();
    void changedAnyField(cFieldEditBase * p);
    void destroyedImage(QObject *p);
};

class Ui_fKeyArrayEd;   ///< Form for cFKeyArrayWidget
/// @class cArrayWidget
/// Egy tömb adatmező megjelenítése és módosítása
/// A tömb elemek rekord ID-k, a rekord név kerül megjelenítésre.
class LV2GSHARED_EXPORT cFKeyArrayWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _fl
    /// @param _par A parent pointere
    cFKeyArrayWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase *_par);
    ~cFKeyArrayWidget();
    virtual int set(const QVariant& v);
protected:
    void setButtons();
    virtual void disableEditWidget(eTristate tsf);
    Ui_fKeyArrayEd   *pUi;
    QStringList       valueView;    ///< Adat/név lista
    cStringListModel *pArrayModel;  ///< lista model (Adat/név lista)
    QList<qlonglong>  ids;          ///< Adat/id lista
    const cRecStaticDescr *pRDescr; ///< Rekord leíró (hivatkozott objektum)
    cRecordListModel *pFRecModel;   ///< Model a ComboBox-hoz
    cTableShape *pTableShape;
    /// Az inzertálandó adat legutoljára elfogadott értéke.
    QString     last;
    cRecordAny  ra;
    int         actRow;       ///< A listában az aktuális elem/sor indexe
    int         selectedNum;    ///< A listában a kijelölt elemek száma
    bool        unique;         ///< A lista elemek egyediek (alapértelmezés)
protected slots:
    void selectionChanged(QItemSelection,QItemSelection);
    void on_comboBox_currentIndexChanged(int);
    void on_pushButtonAdd_pressed();
    void on_pushButtonIns_pressed();
    void on_pushButtonUp_pressed();
    void on_pushButtonDown_pressed();
    void on_pushButtonDel_pressed();
    void on_pushButtonClr_pressed();
    void on_pushButtonNew_pressed();
    void on_pushButtonEdit_pressed();
    void on_listView_doubleClicked(const QModelIndex & index);
    void _setFromEdit();
};

/// @class cColorWidget
/// Egy dátun megjelenítése ill. szerkesztése
class LV2GSHARED_EXPORT cColorWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cColorWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
    ~cColorWidget();
    virtual int set(const QVariant& v);
private:
    void setColor(const QString &cn);
    QLineEdit  *pLineEdit; // A szín neve (edit mező)
    QLabel     *pLabel;    // Szín minta
    QString     sTitle;
    QColor      color;
    QPixmap     pixmap;
private slots:
    void setFromEdit(const QString& text);
    void colorDialog();
};


/// @class cFontFamilyWidget
/// Font család kiválasztása
class LV2GSHARED_EXPORT cFontFamilyWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cFontFamilyWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, cRecordDialogBase* _par);
    ~cFontFamilyWidget();
    virtual int set(const QVariant& v);
protected:
    void _setFromEdit();
    QFontComboBox  *pFontComboBox;
private slots:
    void changeFont(const QFont&);
};

/// @class cFontAttrWidget
/// Font attributumok megadása
class LV2GSHARED_EXPORT cFontAttrWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param __fr A rekord egy mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _par A parent pointere
    cFontAttrWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef __fr, int _fl, cRecordDialogBase* _par);
    ~cFontAttrWidget();
    virtual int set(const QVariant& v);
    static const QString sEnumTypeName;
protected:
    void setupFlagWidget(bool f, const QIcon& icon, QToolButton *& pButton);
    void disableEditWidget(eTristate tsf);
    void _setFromEdit();
    const cColEnumType *pEnumType;
    static QIcon    iconBold;
    static QIcon    iconItalic;
    static QIcon    iconUnderline;
    static QIcon    iconStrikeout;
    QToolButton    *pToolButtonBold;
    QToolButton    *pToolButtonItalic;
    QToolButton    *pToolButtonUnderline;
    QToolButton    *pToolButtonStrikeout;
    QLineEdit      *pLine;
    qlonglong       m;
private slots:
    void togleBoold(bool f);
    void togleItalic(bool f);
    void togleUnderline(bool f);
    void togleStrikeout(bool f);
};

/// @class cLTextWidget
/// Egy text megjelenítése és módosítása
/// Megjelenítés egy QLineEdit -el
class LV2GSHARED_EXPORT cLTextWidget : public cFieldEditBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _fr A rekord text_id mezőjére mutató referencia objektum (nem objektum referencia!)
    /// @param _ti Text index
    /// @param _fl
    /// @param _par A parent dilog pointere
    cLTextWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _ti, int _fl, cRecordDialogBase* _par);
    ~cLTextWidget();
    virtual int set(const QVariant& v);
protected:
    QLineEdit      *pLineEdit;
    QPlainTextEdit *pPlainTextEdit;
    QTextEdit      *pTextEdit;
    int textIndex;
private slots:
    void _setFromEdit();
};

class cFeatureWidget;
class cFeatureWidgetRow : public QObject {
    friend class cFeatureWidget;
    Q_OBJECT
protected:
    enum { COL_B_LIST, COL_B_MAP, COL_KEY, COL_VAL, COL_NUMBER };
    cFeatureWidgetRow(cFeatureWidget *par, int row, const QString &key, const QString &val);
    virtual ~cFeatureWidgetRow();
    cFeatureWidget * _pParentWidget;
    QToolButton *pListButton;
    QToolButton *pMapButton;
    QTableWidgetItem * pItemKey;
    QTableWidgetItem * pItemVal;
    QDialog *       pDialog;
    QListWidget *   pListWidget;
    QTableWidget *  pTableWidget;
private slots:
    void clickButton(int id);
    void listDialog();
    void mapDialog();
};

class LV2GSHARED_EXPORT cFeatureWidget : public cFieldEditBase {
    friend class cFeatureWidgetRow;
    Q_OBJECT
public:
    cFeatureWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par);
    ~cFeatureWidget();
    virtual int set(const QVariant& v);
protected:
    void clearRows();
    QList<cFeatureWidgetRow *>   rowList;
    QVBoxLayout *   pVLayout;
    QHBoxLayout *   pHLayout;
    QTableWidget *  pTable;
    QPushButton *   pDelRow;
    QPushButton *   pInsRow;
    cFeatures       features;
    bool            busy;
private slots:
    void _setFromEdit();
    void onChangedCell(int, int);
    void onInsClicked();
    void onDelClicked();
};

class cParamValueWidget : public cFieldEditBase {
    Q_OBJECT
public:
    cParamValueWidget(const cTableShape &_tm, const cTableShapeField& _tf, cRecordFieldRef _fr, int _fl, cRecordDialogBase *_par);
    ~cParamValueWidget();
    virtual int set(const QVariant& _v);
private:
    void                refreshWidget();
    int                 typeIdIndex;
    bool                rawValue;       ///< Ha a rekord service_vars, akkor a szerkesztendő mező a raw_value.
    cParamType *        pParamType;     ///< Az aktuális paraméter típus rekord
    cServiceVarType *   pSVarType;      ///< Az aktuális service_var_types rekord (ha a service_vars rekord mezőről van szó)
    qlonglong           lastType;       ///< Az utolsó/aktuális paraméter típus
    QLineEdit *         pLineEdit;
    QPlainTextEdit *    pPlainTextEdit;
    QComboBox *         pComboBox;
private slots:
    void _changeType(cFieldEditBase *pTW);
    void _setFromEdit();
};

/* **************************************************************************************************** */

/// Az osztály egy zóna ás hely QComboBox párossal egy hely kiválasztását teszi lehetővé.
/// Opcionálisan megadható egy QLineEdit objektum is, a hely nevek szűrésének a megadásához.
/// A form elemeit paraméterként kell átadni a konstruktornak. Az opcionális elemek utána állíthatóak be.
class LV2GSHARED_EXPORT cSelectPlace : public QObject {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param _pZone   A QComboBox objektum pointere a zóna kiválasztáshoz.
    /// @param _pPlace  A QComboBox objektum pointere a zónán bellüli hely kiválasztáshoz.
    /// @param _pFilt   Opcionális QLineEdit objektum pointere, a hely név szűréséhez.
    /// @param _constFilt Egy opcionális konstans szűrő a helyek-hez (típus : FT_SQL_WHERE).
    /// @param _par Parent objektum
    /// A zóna kiválasztásánál nincs szűrési lehetőség, bármelyik zóna kiválasztható lessz,
    /// az aktuális kiválasztott zóna pedig az "all" lesz, vagy az aktuális zóna.
    /// A hely kiválasztásakor, a NULL megengedett, és ez lessz az aktuálisan kiválasztott.
    cSelectPlace(QComboBox *_pZone, QComboBox *_pPLace, QLineEdit *_pFilt = nullptr, const QString& _constFilt = QString(), QWidget *_par = nullptr);
    /// Az aktuálisan kiválasztott zóna neve
    QString currentZoneName()  const { return pModelZone->at(pComboBoxZone->currentIndex()); }
    /// Az aktuálisan kiválasztott zóna ID-je
    qlonglong currentZoneId()  const { return pModelZone->atId(pComboBoxZone->currentIndex()); }
    /// Az aktuálisan kiválasztott hely neve
    QString currentPlaceName() const { return pModelPlace->at(pComboBoxPLace->currentIndex()); }
    /// Az aktuálisan kiválasztott hely ID-je
    qlonglong currentPlaceId() const { return pModelPlace->atId(pComboBoxPLace->currentIndex()); }
    /// Átmásolja az aktuális értékeket a másik objektumból.
    void copyCurrents(const cSelectPlace& _o);
    /// Set slave. A megadott objektumon tiltva lesznek a widgetek, és
    /// minden változás szinkronízáva less a megadott objektum irányába.
    /// Ha a _pSlave egy NULL pointer, akkor az aktuális szinkronizálás törlődik,
    /// az edig szinkronizált objektum widgetjei engedélyezve lesznek.
    /// @param disabled Ha pSlave nem NULL, és értéke true (vagy nem adtuk meg), akkor letiltja a pSlave elemeit.
    void setSlave(cSelectPlace *_pSlave, bool disabled = true);
    /// Letiltja a widgeteket
    virtual void setDisableWidgets(bool f = true);
    /// Az aktuális hely, helyiség objektum modosításe.
    /// Egy dialógus ablakot jelenít meg, az objektum modosításához.
    /// @return Az objektum ID, vagy NULL_ID.
    qlonglong editCurrentPlace();
    QComboBox *comboBoxZone() const { return pComboBoxZone; }
    QComboBox *comboBoxPlace() const { return pComboBoxPLace; }
    QLineEdit *lineEditPlaceFilt() const { return pLineEditPlaceFilt; }
    void setPlaceInsertButton(QAbstractButton * p);
    void setPlaceEditButton(QAbstractButton * p);
    void setPlaceRefreshButton(QAbstractButton * p);
    void setPlaceInfoButton(QAbstractButton * p);
protected:
    tBatchBlocker<cSelectPlace>   bbPlace;
    /// A két kimeneti signal hívása az aktuális hely név ill. ID-vel.
    bool emitChangePlace(bool f = true);
    ///
    QComboBox *pComboBoxZone;
    QComboBox *pComboBoxPLace;
    QLineEdit *pLineEditPlaceFilt;
    const QString    constFilterPlace;
    cZoneListModel     *pModelZone;
    cPlacesInZoneModel *pModelPlace;
    cSelectPlace *      pSlave;
    QAbstractButton *   pButtonPlaceInsert;
    QAbstractButton *   pButtonPlaceEdit;
    QAbstractButton *   pButtonPlaceRefresh;
    QAbstractButton *   pButtonPlaceInfo;
public slots:
    /// Egy hely, helyiség objektum beillesztése az adatbázisba, és a listába.
    /// Egy dialógus ablakot jelenít meg, az objektum felvételéhez, a minta név azonos a tábla névvel : "places".
    /// Az aktuális hely az új objektum lessz, vagy ha cancel-t nyom, akkor nincs változás.
    /// @return Az új objektum ID, vagy NULL_ID.
    qlonglong insertPlace();
    void editPlace();
    /// Az objektumhoz rendelt widgetek engedélyezése, vagy tiltása.
    void setEnabled(bool f = true);
    /// Az objektumhoz rendelt widgetek tiltása vagy engedélyezése.
    void setDisabled(bool f = true);
    /// Frissíti a lekérdezett listákat.
    /// A kimeneti signal-t csak akkor küldi el, ha az aktuális
    /// hely azonosítü place_id megváltozott, és ha f értéke true (ez az alapértelmezett).
    virtual void refresh(bool f = true);
    /// Az aktuális zóna beállítása
    void setCurrentZone(qlonglong _zid);
    /// Az aktuális hely beállítása
    void setCurrentPlace(qlonglong _pid);
    /// popup report
    void placeReport();
private slots:
    void on_comboBoxZone_currentIndexChanged(int ix);
    void on_comboBoxPlace_currentIndexChanged(int ix);
    void lineEditPlaceFilt_textChanged(const QString& s);
signals:
    void placeNameChanged(const QString& name);
    void placeIdChanged(qlonglong id);
};

/// @class cSelectNode
/// Egy hálózati elem kiválasztását segítő objektum.
/// Egy hálózati elem kiválasztását a nodeNameChanged(); és nodeIdChanged();
/// signal-okkal jelzi.
class LV2GSHARED_EXPORT cSelectNode : public cSelectPlace {
    friend class cSnmpDevQuery;
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param _pZone   A QComboBox objektum pointere a zóna kiválasztáshoz.
    /// @param _pPlace  A QComboBox objektum pointere a zónán bellüli hely kiválasztáshoz.
    /// @param _pNode   A QComboBox objektum pointere a zónán és/vagy a  helységben található eszköz kiválasztáshoz.
    /// @param _pPlaceFilt Opcionális lineEdit objektum pointere, a hely név szűréséhez.
    /// @param _pNodeFilt  Opcionális lineEdit objektum pointere, az eszköz név szűréséhez.
    /// @param _placeConstFilt Egy opcionális konstans szűrő a helyek-hez.
    /// @param _nodeConstFilt  Egy opcionális konstans szűrő az eszközökhöz
    /// @param _par Parent objektum
    /// A zóna és helyekről lásd az ős cSelectPlace konstruktort.
    /// A hálózati elemek kiválasztásához a modell a cRecordListModel, a NULL megengedett.
    /// A konstruktorban a hálózati elemek lekérdezése nem történik meg, az esetleges nagy számú
    /// elem miatt. Ezért a konstruktor után vagy a modelt lecserélő setNodeModel(); metódust,
    /// vagy a refresh(); metódust még meg kell hívni.
    cSelectNode(QComboBox *_pZone, QComboBox *_pPlace, QComboBox *_pNode,
                QLineEdit *_pPlaceFilt = nullptr, QLineEdit *_pNodeFilt = nullptr,
                const QString& _placeConstFilt = QString(), const QString& _nodeConstFilt = QString(),
                QWidget *_par = nullptr);
    /// Lecseréli a node lista modelt. Lekérdezi a listát, nincs kimeneti signal.
    /// Ha _nullable értéke nem TS_NULL, akkor beállítja a nullable adattag értékét is.
    void setNodeModel(cRecordListModel *  _pNodeModel, eTristate _nullable = TS_NULL);
    void setNodeFilter(const QVariant& _par, eOrderType __o, eFilterType __f) {
        pModelNode->setFilter(_par, __o , __f);
    }

    /// Az összes QComboBox objektumon kiválasztja az első elemet.
    /// Ha az aktuális node megváltozott kiküldi a signal-okat, ha az f értéke true.
    void reset(bool f = true);
    /// A kurrens node-ot null-ra állítja. Feltételezi, hogy az lehet null, és az az első elem.
    /// Ha _sig értéke true, vagy nem adtuk meg, akkor a aktív node megváltozásakori szignál nem lessz meghívva.
    void nodeSetNull(bool _sig = false);
    /// Lekérdezi az aktuális node nevét
    QString currentNodeName() const { return pModelNode->at(pComboBoxNode->currentIndex()); }
    /// Lekérdezi az aktuális node ID-t
    qlonglong currentNodeId() const { return pModelNode->atId(pComboBoxNode->currentIndex()); }
    void setLocalityFilter();
    void setExcludedNode(qlonglong _nid = NULL_ID);
    void setPatchInsertButton(QAbstractButton * p);
    void setNodeRefreshButton(QAbstractButton * p);
    void setNodeInfoButton(QAbstractButton * p);
    void changeNodeConstFilter(const QString& _sql);
    /// Letiltja a widgeteket
    virtual void setDisableWidgets(bool f = true);
protected:
    tBatchBlocker<cSelectNode>  bbNode;
    bool emitChangeNode(bool f = true);
    QComboBox *         pComboBoxNode;
    QLineEdit *         pLineEditNodeFilt;
    const QString       constFilterNode;
    cRecordListModel *  pModelNode;
    QAbstractButton *   pButtonPatchInsert;
    QAbstractButton *   pButtonNodeRefresh;
    QAbstractButton *   pButtonNodeInfo;
public slots:
    qlonglong insertPatch(cPatch *pSample = nullptr);
    /// Frissíti a listákat, az ős objektumban is (zone, place).
    /// hely azonosító place_id megváltozott, és ha f értéke true (ez az alapértelmezett).
    virtual void refresh(bool f = true);
    eTristate setCurrentNode(qlonglong _nid);
    void setPlaceId(qlonglong pid, bool _sig = true);
    /// Az objektumhoz rendelt widgetek engedélyezése, vagy tiltása.
    void setEnabled(bool f = true);
    /// Az objektumhoz rendelt widgetek tiltása vagy engedélyezése.
    void setDisabled(bool f = true);
    /// Popup riport az aktuális node-ról
    void nodeReport();
private slots:
    void on_buttonPatchInsert_clicked();
    void on_lineEditNodeFilt_textChanged(const QString& s);
    void on_comboBoxNode_currenstIndexChanged(int ix);
    void on_comboBoxZone_currenstIndexChanged(int);
signals:
    void nodeNameChanged(const QString& name);
    void nodeIdChanged(qlonglong id);
};

class LV2GSHARED_EXPORT cSelectLinkedPort : public cSelectNode {
    Q_OBJECT
public:
    /// Konstruktor.
    /// @param _pZone   A comboBox objektum pointere a zóna kiválasztáshoz.
    /// @param _pPlace  A comboBox objektum pointere a zónán bellüli hely kiválasztáshoz.
    /// @param _pNode   A comboBox objektum pointere a zónán és/vagy a  helységben található eszköz kiválasztáshoz.
    /// @param _pPort   A comboBox objektum pointere a port kiválasztásához
    /// @param _pShare  A comboBox objekrum pointere a megosztás típusának a kiválasztásához
    /// @param _shList  A választható megosztás típusok, opcionális, ha üres listát adunk meg, akkor az összes érték kiválaszható.
    /// @param _pType   A buttonsGroup objektum pointere a link típusának a megadásához
    /// @param _pPlaceFilt Opcionális lineEdit objektum pointere, a hely név szűréséhez.
    /// @param _pNodeFilt  Opcionális lineEdit objektum pointere, az eszköz név szűréséhez.
    /// @param _placeConstFilt Egy opcionális konstans szűrő a helyek-hez.
    /// @param _nodeConstFilt  Egy opcionális konstans szűrő az eszközökhöz
    /// @param _par Parent objektum
    cSelectLinkedPort(QComboBox *_pZone, QComboBox *_pPlace, QComboBox *_pNode,
                QComboBox *_pPort, QButtonGroup *_Type, QComboBox *_pShare, const tIntVector& _shList = tIntVector(),
                QLineEdit *_pPlaceFilt = nullptr, QLineEdit *_pNodeFilt = nullptr,
                const QString& _placeConstFilt = QString(), const QString& _nodeConstFilt = QString(),
                QWidget *_par = nullptr);
    ~cSelectLinkedPort();
    qlonglong currentPortId() const { return pModelPort->atId(pComboBoxPort->currentIndex()); }
    int       currentType()   const { return lastLinkType; }
    int       currentShare()  const { return lastShare; }
    bool      isPatch()       const { return _isPatch; }
private:
    QSqlQuery *     pq;
    QComboBox *     pComboBoxPort;
    QComboBox *     pComboBoxShare;
    QButtonGroup *  pButtonGroupType;
    cEnumListModel *pModelShare;
    cRecordListModel *pModelPort;
    bool        _isPatch;
    int         lastLinkType;
    int         lastShare;
    qlonglong   lastPortId;
    int         lockSlots;
public slots:
    void setLink(cPhsLink& _lnk);
private slots:
    void setNodeId(qlonglong _nid);
    void setPortIdByIndex(int ix);
    void setLinkTypeByButtons(int _id);
    void changedShareType(int ix);
    void portChanged(int ix);
signals:
    void changedLink(qlonglong _pid, int _lt, int _sh);
};

class LV2GSHARED_EXPORT cSelectVlan : public QObject {
    Q_OBJECT
public:
    cSelectVlan(QComboBox *_pComboBoxId, QComboBox *_pComboBoxName, QWidget *_par = nullptr);
    ~cSelectVlan();
public slots:
    void setCurrentByVlan(qlonglong _vid);
    void setCurrentBySubNet(qlonglong _sid);
    void setDisable(bool f);
private:
    QSqlQuery   *pq;
    QComboBox   *pComboBoxId;
    QComboBox   *pComboBoxName;
    cStringListDecModel *pModelId;
    cStringListDecModel *pModelName;
    QStringList       nameList;
    QList<qlonglong>  idList;
    QStringList       sIdList;
    QString     actName;
    qlonglong   actId;
    bool        disableSignal;
    const cEnumVal *  pevNull;
private slots:
    void _changedId(int ix);
    void _changedName(int ix);
signals:
    void changedId(qlonglong _id);
    void changedName(const QString& _n);
};


class LV2GSHARED_EXPORT cSelectSubNet : public QObject {
    Q_OBJECT
public:
    cSelectSubNet(QComboBox *_pComboBoxNet, QComboBox *_pComboBoxName, QWidget *_par = nullptr);
    ~cSelectSubNet();
    qlonglong currentId();
public slots:
    void setCurrentByVlan(qlonglong _vid);
    void setCurrentBySubNet(qlonglong _sid);
    void setCurrentByAddress(QHostAddress& _a);
    void setDisable(bool f);
private:
    QSqlQuery   *pq;
    QComboBox   *pComboBoxNet;
    QComboBox   *pComboBoxName;
    cStringListDecModel *pModelNet;
    cStringListDecModel *pModelName;
    QHostAddress address;
    eAddressType addrType;
    QStringList  nameList;
    QList<qlonglong> idList;
    QStringList  sNetList;
    QList<netAddress> netList;
    QString     actName;
    qlonglong   actId;
    bool        disableSignal;
    const cEnumVal *  pevNull;
private slots:
    void _changedNet(int ix);
    void _changedName(int ix);
signals:
    void changedId(qlonglong _id);
    void changedName(const QString& _n);
};

class cDialogButtons;

class LV2GSHARED_EXPORT cStringMapEdit : public QObject {
    Q_OBJECT
public:
    cStringMapEdit(bool _isDialog, tStringMap& _map, QWidget *par = nullptr);
    ~cStringMapEdit();
    QWidget& widget() { return *_pWidget; }
    QWidget *pWidget() { return _pWidget; }
    QDialog& dialog();
    const bool isDialog;
    tStringMap& map;
private:
    QWidget *_pWidget;
    QDialog *pDialog;
    QTableWidget *pTableWidget;
    cDialogButtons  *pButtons;
private slots:
    void clicked(int id);
    void changed(int row, int column);
};

#endif // LV2WIDGETS_H
