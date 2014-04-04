#ifndef RECORD_TABLE_H
#define RECORD_TABLE_H

#include    "record_dialog.h"
#include    "record_table_model.h"

#if defined(LV2G_LIBRARY)
#include    "ui_column_filter.h"
#else
namespace Ui { class dialogTabFiltOrd; }
#endif

class cRecordTable;
class cRecordTableColumn;
class cRecordTableFODialog;

class cRecordTableFilter : public QObject {
    friend class cRecordTableFODialog;
    Q_OBJECT
public:
    cRecordTableFilter(cRecordTableFODialog &_par, cRecordTableColumn &_rtc);
    ~cRecordTableFilter();
    QString             where(QVariantList &qparams);
    void setFilter(int i);
    cTableShapeFilter& shapeFilter() { if (pFilter == NULL) EXCEPTION(EDATA); return *pFilter; }
    int fieldType();
    cRecordTableColumn& field;
    cRecordTableFODialog &dialog;
    cTableShapeFilter * pFilter;
    int                 iFilter;
    QVariant            param1;
    bool                closed1;
    QVariant            param2;
    bool                closed2;
    qlonglong           types;
    QStringList         typeList;
};

class cRecordTableOrd : public QObject {
    friend class cRecordTableFODialog;
    Q_OBJECT
public:
    cRecordTableOrd(cRecordTableFODialog &par, cRecordTableColumn &_rtc, int types);
    ~cRecordTableOrd();
    QString             ord();
    bool operator<(const cRecordTableOrd& _o) const { return sequence_number < _o.sequence_number; }
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


class cRecordTableFODialog : public QDialog {
    Q_OBJECT
public:
    cRecordTableFODialog(QSqlQuery *pq, cRecordTable &_rt);
    ~cRecordTableFODialog();
    const cRecordTable&         recordTable;
    QString                     where(QVariantList &qparams);
    QString                     ord();
    cRecordTableFilter&         filter() { if (pSelFilter == NULL) EXCEPTION(EPROGFAIL); return *pSelFilter; }
    QList<cRecordTableFilter *> filters;
    cRecordTableFilter *        pSelFilter;
    int                         iSelFilterCol;
    int                         iSelFilterType;
    QList<cRecordTableOrd *>    ords;
    Ui::dialogTabFiltOrd *      pForm;
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
};

class cRecordTableColumn {
public:
    cRecordTableColumn(cTableShapeField& sf, cRecordTable &table);
    cTableShapeField&       shapeField;
    const cRecStaticDescr&  recDescr;
    int                     fieldIndex;
    const cColStaticDescr&  colDescr;
    const QVariant&         header;
    QColor                  fgHeadColor;
    QColor                  bgHeadColor;
    QFont                   headFont;
    int                     headAlign;      // Qt::AlignmentFlag
    QColor                  fgDataColor;
    QColor                  bgDataColor;
    QColor                  fgExDataColor;
    QColor                  bgExDataColor;
    QFont                   dataFont;
    int                     dataAlign;      // Qt::AlignmentFlag
    QColor                  fgNullColor;
    QFont                   nullFont;
};

/// A tábla viszonyát meghatározó flag értékek
enum eRecordTableFlags  {
    RTF_SINGLE = 0x0001,    ///< Egyedi tábla megjelenítése
    RTF_MASTER = 0x0002,    ///< több tábla, az objektum a fő tábláé
    RTF_SLAVE  = 0x0004,    ///< több tábla, az objektum egy mellék tábláé
    RTF_OVNER  = 0x0008,
    RTF_CHILD  = 0x0010,
    RTF_LINK   = 0x0020,    ///< Link táblák
    RTF_LEFT   = 0x0040     ///< A bal oldali link tábla
};

/// @class cRecordTable
/// Egy adatbázis objektum megjelenítését végző objektum.
class cRecordTable : public QObject {
    friend class cRecordTableColumn;
    friend class cRecordTableModel;
    friend class cRecordTableModelSql;
    friend class cRecordTableFODialog;
    Q_OBJECT
public:
    cRecordTable(const QString& _mn, bool _isDialog, QWidget * par = NULL);
    cRecordTable(qlonglong id, bool _isDialog, QWidget * par = NULL, cRecordTable *_master = NULL);
    cRecordTable(cTableShape *pts, bool _isDialog, QWidget * par = NULL);
    ~cRecordTable();
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
    /// Az alapértelmezett paletta referenciáját adja vissza.
    static const QPalette& defPalette() { return *pDefaultPalette; }
    /// Ha a megjelínítés egy dialog box-ban történik, akkor értéke true.
    const bool      isDialog;
protected:
    int         actRow();
    cAlternate *actRecord();
    qlonglong   actId() { cAlternate *p = actRecord(); return p == NULL ? NULL_ID : p->getId(); }
    void setActRow(int row) { pTableView->setCurrentIndex(pTableModel->index(row, 0)); }

    void empty();
    void close();
    void first();
    void next();
    void prev();
    void last();
    void refresh(bool first = true);
    void remove();
    void insert();
    void modify();
    void setEditButtons();
    void setPageButtons();
    void setButtons();

    /// A megadott ID-ű nyomódomb tiltásának a beállítása
    /// @param id A nyomogom id-je (egy enum eDialogButtons érték).
    /// @param d Ha értéke true, akkor a nyomógom letiltásra kerül.
    void buttonDisable(int id, bool d = true);
    ///
    const cRecStaticDescr& inhRecDescr(qlonglong i) const;
    const cRecStaticDescr& inhRecDescr(const QString& tn) const;

    /// A tábla viszonyát meghatározó flag-ek
    int             flags;
    /// Ha a táblázat csak olvasható, akkor értéke true
    bool            isReadOnly;
    /// A lekérdezésekhez (nem az alap lekérdezésekhez) használt query objektum.
    QSqlQuery      *pq;
    QSqlQuery      *pTabQuery;
    /// A megjelenítő leíró
    cTableShape    *pTableShape;
    /// Ha ez egy al táblázat, akkor a felette lévő tábla megjelenítősének a pointere, egyébként NULL
    cRecordTable   *pUpper;
    /// A fő tábla, vagy NULL. Két tábla esetén azonos pUpper-rel
    cRecordTable   *pMaster;
    /// A megjelenítendő mezők
    tRecordTableColumns  fields;
    /// A megjelenítendő tábla leíró pointere
    const cRecStaticDescr*pRecDescr;
    /// A tábla model pointere
    cRecordTableModelSql *pTableModel;
    /// A fő widget, vagy dialog box objektum pointere
    QWidget        *_pWidget;
    /// A nyomogombok objektuma
    cDialogButtons *pButtons;
    /// A saját táblázat megjelenítésének a layer-e
    QVBoxLayout    *pMainLayer;
    /// A táblázatot megjelenítő widget
    QTableView     *pTableView;
    /// Ha több tábla megjelenítése szükséges, akkor a fő layer, egyébként NULL.
    QHBoxLayout    *pMasterLayout;
    QSplitter      *pMasterFrame;
    /// Ha több megjelenített tábla van akkor a bal oldali és saját táblázatot tartalmazó widget pointere, vagy NULL.
    QWidget        *pLeftWidget;
    /// A (bal oldali) alárendelt tábla megjelenítő objektuma.
    cRecordTable   *pRightTable;
    /// Szűrők és rendezés dialog box
    cRecordTableFODialog *  pFODialog;
    /// Az alapértelmezett paletta objektum pointere (az első konstruktor inicializálja).
    static QPalette *pDefaultPalette;
    /// Child tábla esetén a tulajdonos ID-je, ha ismert (csak egy rekord van kijelölve az owner táblázatban)
    qlonglong   owner_id;
private:
    void init(QWidget *par);
    void initSimple(QWidget *pW);
    void initTree();
    void initOwner();
    QString where(QVariantList &qParams);
    void _refresh(bool first = true);
    void refresh_lst_rev();
    void initView();
    /// A dialogus ablak megnyitásához allokál egy objektumot a megadott record tábla név alapján
    /// Hiba esetén dob egy kizárást.
    cTableShape *   getInhShape(const QString& _tn);
    /// A dialogus ablak megnyitásához allokál egy objektumot a megadott record descriptor alapján
    /// Hiba esetén dob egy kizárást.
    cTableShape *   getInhShape(const cRecStaticDescr& d) { return getInhShape(d.tableName()); }
    QStringList         inheritTableList;
    QMap<qlonglong, const cRecStaticDescr *> * pInhRecDescr;
    QString             viewName;
    eTableInheritType   tit;
protected slots:
    /// Megnyomtak egy gombot.
    /// @param id A megnyomott gomb azonosítója
    void buttonPressed(int id);
    /// Ha eltávolításra került sor akkor itt jelez a model. A megadott objektum törlését fogja megkisérelni.
    void recordRemove(cAlternate * _pr);
    /// Ha változott a kijelölés
    void selectionChanged(QItemSelection,QItemSelection);
    void clickedHeader(int);
signals:
    void closeIt();
};


#endif // RECORD_TABLE_H
