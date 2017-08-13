#ifndef RECORD_TABLE_H
#define RECORD_TABLE_H

#include    <QModelIndex>

#include    "lv2g.h"
#include    "record_dialog.h"
#include    "record_table_model.h"

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

/// Nincs jogosultsága form megjelenítése
Ui::noRightsForm * noRightsSetup(QWidget *_pWidget, qlonglong _need, const QString& _obj, const QString& _html = QString());

class cRecordTable;
class cRecordTableColumn;
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
    QString             where(QVariantList &qparams);
    void setFilter(int i);
    int fieldType();                ///< A mező (oszlop) típusa
    cRecordTableColumn& field;      ///< A megjelenítés mező (oszlop) leírója
    cRecordTableFODialog &dialog;   ///< A szűrési feltétel megadásának a dialógusa
    int                 iFilter;    ///< A kiválasztott szűrő leíró indexe vagy -1, ha nincs aktív szűrő
    QVariant            param1;     ///< Szűrés (opcionális) paramétere
    bool                closed1;    ///< Ha a szűrési paraméter egy értékhatár, akkor ha true. akkor zárt intervallumként kell értelmezni.
    QVariant            param2;     ///< Szűrés (opcionális) paramétere, ha két paraméter van
    bool                closed2;    ///< Ha a második szűrési paraméter egy értékhatár, akkor ha true. akkor zárt intervallumként kell értelmezni.
    qlonglong           types;      ///< A szűrés típusa, reláció a paraméter(ek)el
    QList<const cEnumVal *> typeList;   ///< A választható szűrés típusok
    static QString      sNoFilter;
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
    cRecordTableOrd(cRecordTableFODialog &par, cRecordTableColumn &_rtc, int types);
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

class cRecordsViewBase;

/// Szűrések és rendezés dialógus.
/// A rendezés és szűrések állpota
class LV2GSHARED_EXPORT cRecordTableFODialog : public QDialog {
    Q_OBJECT
public:
    cRecordTableFODialog(QSqlQuery *pq, cRecordsViewBase&_rt);
    ~cRecordTableFODialog();
    const cRecordsViewBase&      recordView;
    /// A szűrési feltételeknek megfeleő SQL feltételek litáját adja vissza
    /// A szükslges paramétereket (bind) hozzáadja a qparams konténerhez.
    QStringList where(QVariantList &qparams);
    /// A rendezési utasítás az SQL select-ben
    QString                     ord();
    /// Az aktuálisan kivállasztott filter objektummal tér vissza (?)
    cRecordTableFilter&         filter() { if (pSelFilter == NULL) EXCEPTION(EPROGFAIL); return *pSelFilter; }
    QList<cRecordTableFilter *> filters;
    cRecordTableFilter *        pSelFilter;
    /// A dialógusban a szűrésnél aktuálisan kiválasztott oszlop index
    int                         iSelFilterCol;
    /// A dialógusban a szűrésnél aktuálisan kiválasztott oszlophoz tartozó kiválasztott szűrés típus index
    int                         iSelFilterType;
    QList<cRecordTableOrd *>    ords;
    Ui::dialogTabFiltOrd *      pForm;  ///< Dialógus form
private:
    int indexOf(cRecordTableOrd * _po);
    void setGridLayoutOrder();
    void setFilterDialog();
private slots:
    void clickOk();
    void clickDefault();
    void ordMoveUp(cRecordTableOrd * _po);
    void ordMoveDown(cRecordTableOrd * _po);
    void filtCol(int _c);
    void filtType(int _t);
    void changeParam(QString t);
    void changeParam();
    void changeParam1(int i);
    void changeParam2(int i);
    void changeParamF1(double d);
    void changeParamF2(double d);
    void changeParamDT1(QDateTime dt);
    void changeParamDT2(QDateTime dt);
    void changeClosed1(bool f);
    void changeClosed2(bool f);
    void changeBoolean(bool f);
};

class LV2GSHARED_EXPORT cRecordTableColumn {
public:
    cRecordTableColumn(cTableShapeField& sf, cRecordsViewBase &table);
    cTableShapeField&       shapeField;
    const cRecStaticDescr&  recDescr;
    int                     fieldIndex;
    const cColStaticDescr&  colDescr;
    const QVariant&         header;
    int                     dataAlign;      // Qt::AlignmentFlag
    int                     headAlign;      // Qt::AlignmentFlag
    int                     dataCharacter;  // eDataCharacter
    const cEnumVal&         defaultDc;
    QString                 enumTypeName;
    qlonglong               fieldFlags;
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
    /// Ha bem engedélyezett a szűrők használata (tree esetén nem müködnek a szűrők, mert nem bejárható a fa)
    bool            disableFilters;
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
    /// Egy új rekord neszúrása
    virtual void insert(bool _similar = false);
    /// Egy kijelölt rekord modosítása
    virtual void modify(enum eEx __ex = EX_ERROR);
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void first();
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void next();
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void prev();
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void last();
    /// Egy vagy több kijelölt rekord törlése
    virtual void remove();
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void reset();
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void putIn();
    /// Nem kötelezően implementálandó metódus. Ha nincs újraimplementálva, de mégis meghívjuk, akkor kizárást dob
    virtual void takeOut();
    /// Kijeleölt sorok neveinek a vágólapra másolása, mint lista
    virtual void copy();
    ///
    virtual void receipt();
    virtual void truncate();

    virtual void setEditButtons() = 0;
    /// Nem kötelezően implementálandü virtuális metódus. Alapértelmezetten nem csinál semmit.
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
    qlonglong   actId() { cRecord *p = actRecord(); return p == NULL ? NULL_ID : p->getId(); }

    void initView();
    void initShape(cTableShape *pts = NULL);
    void initMaster();
    void initGroup(QVariantList &vlids);
    /// Inicializálja az adattábla megjelenítését.
    ///
    /// \diafile    record_table.dia "" width=8cm
    virtual void initSimple(QWidget *pW) = 0;

    virtual void _refresh(bool all = true) = 0;

    bool enabledBatchEdit(const cTableShapeField& tsf);
    virtual bool batchEdit(int logicalindex);
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
    static cRecordsViewBase *newRecordView(cTableShape * pts, cRecordsViewBase * own = NULL, QWidget *par = NULL);
    static cRecordsViewBase *newRecordView(QSqlQuery &q, qlonglong shapeId, cRecordsViewBase * own = NULL, QWidget *par = NULL);
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
    cRecordTable(const QString& _mn, bool _isDialog, QWidget * par = NULL);
    /// Konstruktor
    /// Fő, al, vagy önálló tábla megjelenítése, már beolvasott leíró
    /// @param pts A tábla megjelenítését leíró objektum pointere
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = NULL, QWidget * par = NULL);
    /// destruktor
    ~cRecordTable();
    cRecordTableModel *pTableModel() const { return static_cast<cRecordTableModel *>(pModel); }
    cRecord *recordAt(int i, eEx __ex = EX_ERROR) const {
        if (!isContIx(pTableModel()->records(), i)) {
            if (__ex != EX_IGNORE) EXCEPTION(ENOINDEX, i);
            return NULL;
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


    void setEditButtons();
    void setPageButtons();

    ///

    /// A táblázatot megjelenítő widget
    QTableView     *pTableView;
    virtual void init();
    virtual void initSimple(QWidget *pW);
    virtual QStringList filterWhere(QVariantList& qParams);
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
