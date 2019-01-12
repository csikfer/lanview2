#ifndef RECORD_TABLE_H
#define RECORD_TABLE_H

#include    "lv2g.h"
#include    "record_dialog.h"
#include    "record_table_model.h"
#include    "QTableWidget"

#if defined(LV2G_LIBRARY)
#include    "ui_column_filter.h"
#include    "ui_no_rights.h"
#include    "ui_edit_field.h"
#else
namespace Ui {
    class dialogTabFiltOrd;
    class noRightsForm;
}
#endif
#include    <QModelIndex>

/// Nincs jogosultsága form megjelenítése
Ui::noRightsForm * noRightsSetup(QWidget *_pWidget, ePrivilegeLevel _need, const QString& _obj, const QString& _html = QString());

class cRecordTable;
class cRecordTableColumn;
class cRecordTableHideRows;
class cRecordTableFODialog;

/*!
@class cRecordTableFilter
@brief
A tábla megjelenítésnél a szűrő funkciókat ellátó objektum.

Az objektum egy mezőre vonatkozó szűrési feltételt kezel.
 */
class LV2GSHARED_EXPORT cRecordTableFilter : public QObject {
    friend class cRecordTableFODialog;
    Q_OBJECT
public:
    cRecordTableFilter(cRecordTableFODialog &_par, cRecordTableColumn &_rtc);
    ~cRecordTableFilter();
    QVariant paramValue(QStringList &sl, bool &ok);
    QString             where(QVariantList &qparams);
    void setFilter(int i);
    int fieldType();                ///< A mező (oszlop) típusa
    const cEnumVal *pActFilterType();///< Filter type
    cRecordTableColumn& field;      ///< A megjelenítés mező (oszlop) leírója
    cRecordTableFODialog &dialog;   ///< A szűrési feltétel megadásának a dialógusa
    int                 iFilter;    ///< A kiválasztott szűrő leíró indexe vagy -1, ha nincs aktív szűrő
    QVariant            param1;     ///< Szűrés (opcionális) paramétere
    bool                closed1;    ///< Ha a szűrési paraméter egy értékhatár, akkor ha true. akkor zárt intervallumként kell értelmezni.
    QVariant            param2;     ///< Szűrés (opcionális) paramétere, ha két paraméter van
    bool                closed2;    ///< Ha a második szűrési paraméter egy értékhatár, akkor ha true. akkor zárt intervallumként kell értelmezni.
    int                 toType;
    qlonglong           filtTypesEx;///< A szűrés típusa, reláció a paraméter(ek)el, és típus konverzió (SET + SET)
    qlonglong           filtTypes;  ///< A szűrés típusa, reláció a paraméter(ek)el
    qlonglong           toTypes;
    bool                inverse;    ///< Inverse
    bool                any;        ///< ARRAY all/any
    bool                csi;        ///< Case insensitive
    qlonglong           setOn;      ///< SET típus esetén bekapcsolva
    qlonglong           setOff;     ///< SET típus esetén kikapcsolva
    QValidator *        pValidator1;
    QValidator *        pValidator2;
    QList<const cEnumVal *> filterTypeList;   ///< A választható szűrés típusok
    qlonglong   type2filter();      ///< A lehetséges szűrő típusok, és a TEXT-hez rendelt lehetséges típuskonverziók (SET + SET)
private:
    QString whereLike(const QString &n, bool isArray);
    QString whereLitle(const QString& n, bool af, bool inv, bool clo);
    QString whereEnum(const QString &n, QVariantList& qparams);
    QString whereSet(const QString &n, QVariantList& qparams);
    template <class V> void tSetValidator();
    void setValidatorRegExp(const QString& _re);
    void setValidator(int i);
public slots:
    void changedParam1(const QString& s);
    void changedParam2(const QString& s);
    void togledClosed1(bool f);
    void togledClosed2(bool f);
    void togledCaseSen(bool f);
    void togledInverse(bool f);
    void changedToType(int i);
    void changedAnyAll(int i);
    void changedText();
};

/*!
@class cRecordTableOrd
@brief
A tábla megjelenítésnél a rendezés funkciókat ellátó objektum.
 */
class LV2GSHARED_EXPORT cRecordTableOrd : public QObject {
    friend class cRecordTableFODialog;
    Q_OBJECT
public:
    cRecordTableOrd(cRecordTableFODialog &par, cRecordTableColumn &_rtc, qlonglong types);
    ~cRecordTableOrd();
    QString             ord();
    bool operator<(const cRecordTableOrd& _o) const { return sequence_number < _o.sequence_number; }
    bool operator>(const cRecordTableOrd& _o) const { return sequence_number > _o.sequence_number; }
    void disableUp(bool f = true)   { pUp->setDisabled(f); pDown->setDisabled(false);}
    void disableDown(bool f = true) { pDown->setDisabled(f); }
protected:
    cRecordTableColumn& field;
    int                 ordTypes;
    int                 act;
    int                 sequence_number;
    QLineEdit          *pRowName;
    QComboBox          *pType;
    QPushButton        *pUp;
    QPushButton        *pDown;
private slots:
    void up();
    void down();
signals:
    void moveUp(cRecordTableOrd *p);
    void moveDown(cRecordTableOrd *p);
};

class cEnumCheckBox : public QCheckBox {
    Q_OBJECT
public:
    cEnumCheckBox(int _e, qlonglong *_pOn, qlonglong *_pOff, const QString& t);
private:
    qlonglong  m;
    /// SET típusú maszk
    /// ENUM: ha a bit on, akkor az az enumerációs érték kiválasztva (1 : checked, 0; unchecked)
    /// SET:  Ha a bit on, akkor annak a bitnek a set-ben nem kell on-ba lennie (1 : partially; 0: unchecked)
    qlonglong *pOn;
    /// SET típusú maszk
    /// ENUM: = NULL
    /// SET:  Ha a bit on, akkor az a bit a set-ben nem lehet on-ba lennie (1 : unchecked; 0 : partially )
    qlonglong *pOff;
private slots:
    void _chageStat(int st);
};

class LV2GSHARED_EXPORT cRecordTableHideRow : public QObject {
    Q_OBJECT
public:
    cRecordTableHideRow(int row, cRecordTableHideRows * _par);
    QString     name;
    QString     title;
    QCheckBox   *pCheckBoxTab;
    QCheckBox   *pCheckBoxHTML;
    cRecordTableHideRows *pParent;
    cRecordTableColumn   *pColumn;
    static int columns();
    static QString colTitle(int i);
private slots:
    void on_checkBoxTab_togle(bool st);
    void on_checkBoxHTML_togle(bool st);
};

class LV2GSHARED_EXPORT cRecordTableHideRows : public QTableWidget {
    friend class cRecordTableFODialog;
    Q_OBJECT
public:
    cRecordTableHideRows(cRecordTableFODialog *_par);
    cRecordTableFODialog *pParent;
public slots:
    void refresh();
};


class cRecordsViewBase;
/// Szűrések és rendezés dialógus.
/// A rendezés és szűrések állpota
class LV2GSHARED_EXPORT cRecordTableFODialog : public QDialog {
    Q_OBJECT
public:
    cRecordTableFODialog(QSqlQuery *pq, cRecordsViewBase&_rt);
    ~cRecordTableFODialog();
    cRecordsViewBase&           recordView;
    /// A szűrési feltételeknek megfeleő SQL feltételek litáját adja vissza
    /// A szükslges paramétereket (bind) hozzáadja a qparams konténerhez.
    QStringList where(QVariantList &qparams);
    /// A rendezési utasítás az SQL select-ben
    QString                     ord();
    /// Az aktuálisan kivállasztott filter objektummal tér vissza (?)
    cRecordTableFilter&         filter() { if (pSelFilter == nullptr) EXCEPTION(EPROGFAIL); return *pSelFilter; }
    QList<cRecordTableFilter *> filters;
    cRecordTableFilter *        pSelFilter;
    /// A dialógusban a szűrésnél aktuálisan kiválasztott oszlop index
    int                         iSelFilterCol;
    /// A dialógusban a szűrésnél aktuálisan kiválasztott oszlophoz tartozó kiválasztott szűrés típus index
    int                         iSelFilterType;
    QList<cRecordTableOrd *>    ords;
    Ui::dialogTabFiltOrd *      pForm;  ///< Dialógus form
    QPlainTextEdit *            pTextEdit;
    QLineEdit *                 pLineEdit1;
    QLineEdit *                 pLineEdit2;
    cEnumListModel *            pToTypeModel;
private:
    int indexOf(cRecordTableOrd * _po);
    void setGridLayoutOrder();
    QComboBox * comboBoxAnyAll();
    void setFilterDialog();
    void setFilterDialogPattern(int fType, int dType);
    void setFilterDialogComp(int fType, int dType);
    void setFilterDialogEnum(const cColEnumType& et, bool _enum);
    static QString sCheckInverse;
private slots:
    void clickOk();
    void clickDefault();
    void ordMoveUp(cRecordTableOrd * _po);
    void ordMoveDown(cRecordTableOrd * _po);
    void filtCol(int _c);
    void filtType(int _t);
};

class LV2GSHARED_EXPORT cRecordTableColumn {
public:
    cRecordTableColumn(cTableShapeField& sf, cRecordsViewBase &table);
    bool colExpr(QString &_name, int *pEColType = nullptr);
    cRecordsViewBase       *parent;
    cTableShapeField&       shapeField;
    const cRecStaticDescr&  recDescr;
    int                     fieldIndex;
    int                     textIndex;
    const cColStaticDescr * pColDescr;
    const cColEnumType *    pTextEnum;
    int                     dataAlign;      // Qt::AlignmentFlag
    int                     headAlign;      // Qt::AlignmentFlag
    int                     dataCharacter;  // eDataCharacter
    QString                 enumTypeName;
    qlonglong               fieldFlags;
    static qlonglong        type2filter(int _type);
    enum eIsImage { IS_NOT_IMAGE, IS_IMAGE, IS_ICON, IS_ICON_NAME }  isImage;
    QVariant                headerText;
    QVariant                headerIcon;
    QVariant                headerToolTyp;
};

/// A tábla viszonyát meghatározó flag értékek
enum eRecordTableFlags  {
    RTF_SINGLE  = 0x0001,   ///< Egyedi tábla megjelenítése
    RTF_MASTER  = 0x0002,   ///< több tábla, az objektum a fő tábláé
    RTF_SLAVE   = 0x0004,   ///< több tábla, az objektum egy mellék tábláé
    RTF_OVNER   = 0x0008,
    RTF_CHILD   = 0x0010,
    RTF_TREE    = 0x0020,   ///< Fa szerkezet
    RTF_LINK    = 0x0040,   ///< Link táblák
    RTF_LEFT    = 0x0080,   ///< A bal oldali link tábla
    RTF_MEMBER  = 0x0100,   ///< Csoport tagság
    RTF_GROUP   = 0x0200,   ///< Csoprt
    RTF_IGROUP  = 0x1000,
    RTF_NGROUP  = 0x2000,
    RTF_IMEMBER = 0x4000,
    RTF_NMEMBER = 0x8000
};

class cRecordsViewBase;
typedef QList<cRecordsViewBase *>   tRecordsViewBaseList;

class LV2GSHARED_EXPORT cRecordsViewBase : public QObject {
    Q_OBJECT
public:
    cRecordsViewBase(bool _isDialog, QWidget *par);
    ~cRecordsViewBase();

    QWidget& widget() const { return *_pWidget; }
    QWidget *pWidget() const { return _pWidget; }
    /// A QDialog objektum referenciáját adja vissza, ha a megjelenítés nem dialog boxban történik, akkor dob egy kizárást
    QDialog& dialog();
    cTableShape& tableShape() { return *pTableShape; }
    const cTableShape& tableShape() const { return *pTableShape; }
    /// A megjelenítendő tábla rekord leíró objektum referenciáját adja vissza
    const cRecStaticDescr& recDescr() const { return *pRecDescr; }
    /// A megadott sorszámú (a megjelenítő leíróban) mező megjelenítő leíró referenciáját adja vissza.
    const cRecordTableColumn& field(int i) const { return *fields[i]; }
    /// Ha a megjelínítés egy dialog box-ban történik, akkor értéke true.
    const bool      isDialog;
    /// A megadott ID-ű nyomódomb tiltásának a beállítása
    /// @param id A nyomogom id-je (egy enum eDialogButtons érték).
    /// @param d Ha értéke true, akkor a nyomógom letiltásra kerül.
    void buttonDisable(int id, bool d = true);
    /// A tábla viszonyát meghatározó flag-ek
    int             flags;
    qlonglong       shapeType;
    /// Ha a táblázat csak olvasható, akkor értéke true
    bool            isReadOnly;
    /// Ha a táblázatba nincs joga sort törölni
    bool            isNoDelete;
    /// Ha a táblázatba nincs joga rekordot beszúrni
    bool            isNoInsert;
    /// A lekérdezésekhez (nem az alap lekérdezésekhez) használt query objektum.
    QSqlQuery      *pq;
    QSqlQuery      *pTabQuery;
    /// A megjelenítő leíró
    cTableShape    *pTableShape;
    /// A model (bázis objektum)-ra mutató pointer
    cRecordViewModelBase *pModel;
    /// Ha ez egy al táblázat, akkor a felette lévő tábla megjelenítőjének a pointere, egyébként NULL
    cRecordsViewBase   *pUpper;
    /// A fő tábla, vagy NULL. Két tábla esetén azonos pUpper-rel
    cRecordsViewBase   *pMaster;
    /// A megjelenítendő mezők
    tRecordTableColumns  fields;
    /// A megjelenítendő tábla leíró pointere
    const cRecStaticDescr*pRecDescr;
    /// A fő widget, vagy dialog box objektum pointere
    QWidget        *_pWidget;
    ///
    QSplitter      *pMasterSplitter;
    /// Nyomógomb ID-k sorrendben
    tIntVector  buttons;
    /// A nyomogombok objektuma
    cDialogButtons *pButtons;
    /// A saját táblázat megjelenítésének a layer-e
    QVBoxLayout    *pMainLayout;
    /// Ha több tábla megjelenítése szükséges, akkor a fő layer, egyébként NULL.
    QHBoxLayout    *pMasterLayout;
    /// Ha több megjelenített tábla van akkor a bal oldali és saját táblázatot tartalmazó widget pointere, vagy NULL.
    QWidget        *pLeftWidget;
    /// A (bal oldali) alárendelt tábla megjelenítő objektuma.
    tRecordsViewBaseList   *pRightTables;
    /// Ha több bal oldali tábla van, a tabWideget
    QTabWidget             *pRightTabWidget;
    /// Szűrők és rendezés dialog box
    cRecordTableFODialog *  pFODialog;
    /// Child tábla esetén a tulajdonos ID-je, ha ismert (csak egy rekord van kijelölve az owner táblázatban)
    qlonglong   owner_id;
    /// Ha van parent objektum (rekord) és ismert.
    qlonglong   parent_id;
    /// ID index
    QString         idName;
    /// A dialogus ablak megnyitásához allokál egy objektumot a megadott record tábla név alapján
    /// Hiba esetén dob egy kizárást.
    static cTableShape *   getInhShape(QSqlQuery &q, cTableShape *pTableShape, const QString& _tn, bool ro = false);
    /// A dialogus ablak megnyitásához allokál egy objektumot a megadott record descriptor alapján
    /// Hiba esetén dob egy kizárást.
    cTableShape *   getInhShape(cTableShape *pTableShape, const cRecStaticDescr& d) { return getInhShape(*pq, pTableShape, d.tableName(), isReadOnly); }
    QStringList         inheritTableList;
    QMap<qlonglong, const cRecStaticDescr *> * pInhRecDescr;
    QString             viewName;   // A TEMP VIEW neve, vagy NULL, ha nincs
    eTableInheritType   tableInhType;
    cRecordDialogBase  *pRecordDialog;

    const cRecStaticDescr& inhRecDescr(qlonglong i) const;
    const cRecStaticDescr& inhRecDescr(const QString& tn) const;

    virtual void init() = 0;

    // egy gomb megnyomására hívandó virtuális metódusok
    /// Dialogus ablak esetén hívja a done() metódust a paraméterrel.
    /// Egyébként a closeIt() hívja.
    virtual void close(int r = QDialog::Accepted);
    /// Újraolvassa az adatbázist
    virtual void refresh(bool all = true);
    /// Inser new row/record
    virtual void insert(bool _similar = false);
    /// Modify selected row/record
    virtual void modify(enum eEx __ex = EX_ERROR);
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void first();
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void next();
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void prev();
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void last();
    /// Egy vagy több kijelölt rekord törlése
    virtual void remove();
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void reset();
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void putIn();
    /// Optional method to implement. If it is not re-implemented, but it is still called, it drop exception
    virtual void takeOut();
    /// Kijeleölt sorok neveinek a vágólapra másolása, mint lista
    virtual void copy();
    ///
    virtual void receipt();
    virtual void truncate();
    virtual void report();

    virtual void setEditButtons() = 0;
    /// Optional method to implement. By default, you are not doing anything.
    virtual void setPageButtons();
    /// Alapértelmezetten a setEditButtons(); és setPageButtons(); metódusokat hívja
    virtual void setButtons();
    QStringList refineWhere(QVariantList& qParams);
    virtual QStringList where(QVariantList &qParams);

    virtual QStringList filterWhere(QVariantList& qParams);

    virtual QModelIndexList selectedRows() = 0;
    virtual QModelIndex actIndex() = 0;
    virtual cRecord *actRecord(const QModelIndex& _mi = QModelIndex()) = 0;
    virtual cRecord *nextRow(QModelIndex *pMi, int _upRes = 1)   = 0;
    virtual cRecord *prevRow(QModelIndex *pMi, int _upRes = 1)   = 0;
    virtual void selectRow(const QModelIndex& mi)   = 0;
    qlonglong   actId(eEx __ex = EX_ERROR);

    void initView();
    void initShape(cTableShape *pts = nullptr);
    void initMaster();
    void initGroup(QVariantList &vlids);
    /// Inicializálja az adattábla megjelenítését.
    ///
    /// \diafile    record_table.dia "" width=8cm
    virtual void initSimple(QWidget *pW) = 0;

    virtual void _refresh(bool all = true) = 0;

    bool enabledBatchEdit(const cTableShapeField& tsf);
    virtual bool batchEdit(int logicalindex);
    virtual void hideColumn(int ix, bool f = true) = 0;
    void hideColumns(void);
public slots:
    /// Megnyomtak egy gombot.
    /// @param id A megnyomott gomb azonosítója
    virtual void buttonPressed(int id);
    void clickedHeader(int logicalindex);
    /// Ha változott a kijelölés
    void selectionChanged(QItemSelection,QItemSelection);
    /// Indítja az editáló dialogust, a megadostt indexű rekordra.
    void modifyByIndex(const QModelIndex & index);
signals:
    void closeIt();
public:
    static cRecordsViewBase *newRecordView(cTableShape * pts, cRecordsViewBase * own = nullptr, QWidget *par = nullptr);
    static cRecordsViewBase *newRecordView(QSqlQuery &q, qlonglong shapeId, cRecordsViewBase * own = nullptr, QWidget *par = nullptr);
    /// Az owner-re mutató ID mező indexét keressük.
    /// Alapesetben ez kiderül a rekord idegen kulcsokból.
    /// De megadható a features mezőben is, ha ez az idegen kilcsmezőkkel nem lehetséges, nem egyértelmű:
    /// ekkor a <table shape name>.owner kulcs értéke a keresett mező neve. (nem a tábla, hanem a leíró neve van a kulcsban)
    int ixToOwner();

private:
    void rightTabs(QVariantList &vlids);
    void createRightTab();
    /// megallokálja a konténert.
    tRecordList<cTableShape>   *getShapes();
};


/// @class cRecordTable
/// Egy adatbázis tábla megjelenítését végző objektum.
class LV2GSHARED_EXPORT cRecordTable : public cRecordsViewBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// Fő ill. önálló tábla megjelenítése
    /// @param _mn A tábla megjelenítését leíró rekord neve (table_shapes.table_shape_name)
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordTable(const QString& _mn, bool _isDialog, QWidget * par = nullptr);
    /// Konstruktor
    /// Fő, al, vagy önálló tábla megjelenítése, már beolvasott leíró
    /// @param pts A tábla megjelenítését leíró objektum pointere
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = nullptr, QWidget * par = nullptr);
    /// destruktor
    ~cRecordTable();
    cRecordTableModel *pTableModel() const { return static_cast<cRecordTableModel *>(pModel); }
    cRecord *recordAt(int i, eEx __ex = EX_ERROR) const {
        if (!isContIx(pTableModel()->records(), i)) {
            if (__ex != EX_IGNORE) EXCEPTION(ENOINDEX, i);
            return nullptr;
        }
        return pTableModel()->records()[i];
    }
    virtual QModelIndexList selectedRows();
    virtual QModelIndex actIndex();
    virtual cRecord *actRecord(const QModelIndex& _mi = QModelIndex());
    virtual cRecord *nextRow(QModelIndex *pMi, int _upRes = 1);
    virtual cRecord *prevRow(QModelIndex *pMi, int _upRes = 1);
    virtual void selectRow(const QModelIndex& mi);
    virtual void refresh(bool all = true);
    virtual bool batchEdit(int logicalindex);
    void empty();

    void first();
    void next();
    void prev();
    void last();
    void putIn();
    void takeOut();
    void copy();
    void report();


    void setEditButtons();
    void setPageButtons();

    ///

    /// A táblázatot megjelenítő widget
    QTableView     *pTableView;
    virtual void init();
    virtual void initSimple(QWidget *pW);
    virtual void hideColumn(int ix, bool f = true);
    void _refresh(bool all = true);
    // void refresh_lst_rev();
    cRecord *record(int i);
    cRecord * record(QModelIndex mi);

    QTimer *pTimer;
public:
    QTableView *tableView() { return pTableView; }
private slots:
    void autoRefresh();
};


#endif // RECORD_TABLE_H
