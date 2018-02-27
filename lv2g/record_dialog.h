#ifndef RECORD_DIALOG_H
#define RECORD_DIALOG_H

#include "lv2models.h"
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QLayout>
#include <QtWidgets/QDialog>
#include <QTableWidget>
#include <QEventLoop>
#include "lv2widgets.h"

_GEX int _setCurrentIndex(const QString& name, QComboBox * pComboBox, eEx __ex = EX_ERROR);
_GEX int _setCurrentIndex(const QString& name, QComboBox * pComboBox, cRecordListModel *pListModel, eEx __ex = EX_ERROR);
_GEX int _setPlaceComboBoxs(qlonglong pid, QComboBox *pComboBoxZone, QComboBox *pComboBoxPlace, bool refresh);


/// @enum eDialogButtons
/// @brief A dialógus ablakokban kezelt nyomógombok típusait reprezentáló enumerációs konstansok.
enum eDialogButtons {
    DBT_CLOSE = 0,      ///< Close
    DBT_OK,             ///< OK
    DBT_REFRESH,        ///< Ablak tartalmának a frissítése
    DBT_INSERT,         ///< Beszúrás
    DBT_SIMILAR,        ///< Beszúrás, az aktív rekordhoz hasonlót (azzal inicializált dialóg)
    DBT_MODIFY,         ///< modosítás
    DBT_SAVE,           ///< Mentés
    DBT_FIRST,          ///< Lapozás az elsőoldalra
    DBT_PREV,           ///< Előző oldalra lapozás, előző elem
    DBT_NEXT,           ///< Következő oldal/elem
    DBT_LAST,           ///< Utolsó oldal/elem
    DBT_DELETE,         ///< törlés
    DBT_RESTORE,        ///< Visszaállítás
    DBT_CANCEL,         ///< Elvet
    DBT_RESET,          ///< Alaphelyzet
    DBT_PUT_IN,         ///< Betesz
    DBT_TAKE_OUT,       ///< Kivesz
    DBT_EXPAND,         ///< FA: kibont
    DBT_ROOT,           ///< FA: rész fa megjelenítése
    DBT_COPY,           ///< Másolás a vágó lapra, vagy CVS export
    DBT_RECEIPT,        ///< Üzenet, esemény nyugtázása
    DBT_TRUNCATE,       ///< Az összes rekord törlése
    DBT_COMPLETE,       ///< Kiegészít, ...
//  DBT_TOOLS,          ///< Segédeszköz, egyedi dalogus megjelenítése
    DBT_BUTTON_NUMBERS, ///< Nem egy nyomógombot reprezentál, hanem azok számát
    DBT_SPACER,         ///< Nem nyomogomb, spacer
    DBT_BREAK           ///< Nem nyomógomb, "sortörés"
};

/// @class cDialogButtons
/// @brief A dialogus ablakokban nyomodombok kezelése
class LV2GSHARED_EXPORT cDialogButtons : public QButtonGroup {
public:
    cDialogButtons(qlonglong buttons, qlonglong buttons2 = 0, QWidget *par = NULL);
    cDialogButtons(const tIntVector& buttons, QWidget *par = NULL);
    QWidget& widget()   { return *_pWidget; }
    QWidget *pWidget()  { return _pWidget; }
    void enabeAll();
    void disable(qlonglong __idSet);
    void disableExcept(qlonglong __idSet = ENUM2SET2(DBT_CANCEL, DBT_CLOSE));
    QAbstractButton *addPB(int id, QWidget *par);
protected:
    void init(int buttons, QBoxLayout *pL);
    static void staticInit();
    QWidget     *_pWidget;
    QBoxLayout  *_pLayout;
    static int _buttonNumbers;
    static QStringList      buttonNames;
    static QList<QIcon>     icons;
    static QList<int>       keys;
};

class cRecordDialogInh;
class cRecordsViewBase;
class cFKeyWidget;
/// @class cRecordDialogBase
/// @brief Rekord szerkesztés dialógus alap objektum
/// Konkrét megjelenítést nem tartalmaz.
class LV2GSHARED_EXPORT cRecordDialogBase : public QObject {
    friend class cRecordDialogInh;
    friend class cFKeyWidget;
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __tm A rekord megjelenítését leíró objektum példány
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param ownTab Ha a parent egy táblázat (vagy tree) megjelenítő
    /// @param ownDialog Ha egy cRecordDialogInh része, akkor a szülő objektum pointere, vagy a hívó egy cRecoedDialog obj.
    /// @param par Az szülő widget opcionális parent pointere
    cRecordDialogBase(const cTableShape &__tm, qlonglong _buttons, bool dialog = true, cRecordDialogBase * ownDialog = NULL, cRecordsViewBase  * ownTab = NULL, QWidget *par = NULL);
    /// destruktor
    ~cRecordDialogBase();
    virtual cRecord& record();
    /// A dialógust megvalósító widget objektum referenciájával tér vissza
    QWidget& widget()   { return *_pWidget; }
    /// A dialógust megvalósító widget objektum pointerével tér vissza
    QWidget *pWidget()  { return _pWidget; }
    /// A dialógust megvalósító dialógus objektum referenciájával tér vissza, ha nem QDialóg objektum valósítja meg a dialógust, akkor dob egy kizárást.
    QDialog& dialog()   { if (!_isDialog) EXCEPTION(EPROGFAIL); return *(QDialog *)_pWidget; }
    /// A hiba string tartalmával tér vissza
    const QString& errMsg() const { return _errMsg; }
    bool disabled() const { return _isDisabled; }
    /// Ha a _close paraméter true. akkor feltételezi, hogy QDialógus objektum a dialógus ablakunk, és hívja a QDialog::exec() metódust.
    /// Ha viszont _close false, akkor indít a widgetre egy loop-ot (QEventLoop). Ekkor egy nyomógomb megnyomásakor nem csukódik be a dialógusunk.
    /// Annak becsukásáról, ill az exec() metódus újra hívásáról mi döntünk.
    /// Ha _close igaz, de az isDialog adattagunk hamis, akkor kapunk egy kizárást.
    /// @return A dialógus ablak kilépési kódja, vagyis az aktívált nyomógombot reprezentáló enumeráxiós konstans értéke.
    int exec(bool _close = true);
    // / Insert modal dialog
    // static cRecord *insertDialog(QSqlQuery &q, cTableShape *pTableShape, const cRecStaticDescr *pRecDescr, QWidget *_par = NULL);
    /// A tábla model rekord. A megjelenítés leírója, azonosítja a rekord decriptor-t.
    cTableShape      tableShape;
    /// Rekord descriptor
    const cRecStaticDescr&  rDescr;
    /// Owner dialógus pointere, ha van, vagy NULL
    cRecordDialogBase * _pOwnerDialog;
    /// Owner tábla pointere, ha van, vagy NULL
    cRecordsViewBase  * _pOwnerTable;
    /// Az objektum neve
    const QString           name;
    ///

    virtual cFieldEditBase * operator[](const QString& __fn) = 0;
    virtual void restore(const cRecord *_pRec = NULL) = 0;
    virtual bool accept() = 0;
public slots:
    /// Hiba ablakunk van, megnyomták a close gombot...
    void cancel()       { _pressed(DBT_CANCEL); }
    bool close()        { return _pWidget->close(); }
    void show()         { _pWidget->show(); }
    void hide()         { _pWidget->hide(); }
    /// Slot a megnyomtak egy gombot szignálra.
    /// @param id a megnyomott gomb kódja
    void _pressed(int id);
protected:
    /// A dialógust megvalósító QWidget, vagy QDialog objektum példány.
    QWidget                *_pWidget;
    /// Ha a dialógus ténylegesen egy QDialog objektumként lett létrehozva, akkor értéke true.
    bool                    _isDialog;
    /// A szerkesztendő objektum pointere. Az osztálydestruktor felszabadítja, ha nem NULL.
    cRecord                *_pRecord;
    /// A nyomógombokat kezelő objektum példány
    cDialogButtons         *_pButtons;
    ///
    QEventLoop             *_pLoop;
    /// Az adatbázis hozzáférést biztosító objektum példány
    QSqlQuery              *pq;
    /// Ha a dialógus nem szerkeszthető, akkor értéke true.
    bool                    _isReadOnly;
    /// A tábla nézet nem nyitható meg. (alternatív hiba üzenet az _errMsg -ben, ha van).
    bool                    _isDisabled;
    qlonglong               _viewRights;
    /// A szerkesztett értékek rekordba másolásakor a hiba üzenetek buffere
    QString                 _errMsg;
 signals:
     void buttonPressed(int id);
};

/// @class cRecordDialog
/// @brief Rekord szerkesztés dialógus objektum
/// Tartalmazza (felépíti) a rekord mezőinek a megjelenítését, ill. a szerkesztést megvalósító widget-eket is.
class LV2GSHARED_EXPORT cRecordDialog : public cRecordDialogBase {
    friend class cFKeyWidget;
public:
    /// Konstruktor
    /// @param rec Az editálandó adat objktum referenciája
    /// @param __tm A rekord/tábla megjelenítését ill szerkesztését vezérlő leíró
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param parent Az szülő widget opcionális parent pointere
    cRecordDialog(const cTableShape &__tm, qlonglong _buttons, bool dialog = true, cRecordDialogBase *ownDialog = NULL, cRecordsViewBase  * ownTab = NULL, QWidget * parent = NULL);
    /// A rekord adattag tartalmának a megjelenítése/megjelenítés visszaállítása
    virtual void restore(const cRecord *_pRec = NULL);
    /// A megjelenített értékek kiolvasása
    virtual bool accept();
    virtual cFieldEditBase * operator[](const QString& __fn);
protected:
    QVBoxLayout            *pVBoxLayout;
    //QHBoxLayout            *pHBoxLayout;
    QBoxLayout             *pSplittLayout;
    QSplitter              *pSplitter;
    QFormLayout            *pFormLayout;
    QList<cFieldEditBase *> fields;
private:
    void init();
};

/// @class cRecordDialogInh
/// @brief Rekord szerkesztés dialógus objektum
/// Tartalmazza (felépíti) a rekord mezőinek a megjelenítését, ill. a szerkesztést megvalósító widget-eket is.
/// Akkor használlható a cRecordDialog helyett, ha olyan rekordot kell szerkeszteni, vagy megjeleníteni, mely egy öröklött, vagy ős példány is lehet.
/// Szerkeszteni mindíg az eredeti rekordot lehet, vaygis ős esetén hiányoznak mezők az alaptípushoz képest, leszámazott esetén pedig további mező kerülnek megjelenítésre.
/// Az objektum létrehozza az összes lehetséges rekord típushoz a cRecordDialog objektumot, és a konkrét rekord megadásakor választja ki azt amelyiket használni fogja.
class LV2GSHARED_EXPORT cRecordDialogInh : public cRecordDialogBase {
    friend class cRecordsViewBase;
public:
    /// Konstruktor
    /// @param _tm A rekord/tábla megjelenítését ill szerkesztését vezérlő leíró (alap tábla ill. rekord típusra)
    /// @param _tms A lehetséges rekord leírók. Mind, tartalmazza a _tm -et is!
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param _oid Tulajdonos rekord ID, ha nincs owner, akkor kötelezően NULL_ID.
    /// @param _pid Parent rekord ID , ha nincs parent, vagy nincs megadva, akkor értéke NULL_ID
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param parent Az szülő widget opcionális pointere
    cRecordDialogInh(const cTableShape &_tm, tRecordList<cTableShape>& _tms, qlonglong _buttons, bool dialog = true, cRecordDialogBase *ownDialog = NULL, cRecordsViewBase  * ownTab = NULL, QWidget * parent = NULL);
    ~cRecordDialogInh();
    ///
    virtual cRecord& record();
    int actTab();

    void setActTab(int i);
    cRecordDialog& actDialog()  { return *tabs[actTab()]; }
    cRecordDialog *actPDialog() { return  tabs[actTab()]; }

/*    /// A megjelenített értékek beállítása az aktuális tab-on
    void set(const cRecord& _r) { tabs[actTab()]->set(_r); }*/
    ///
    virtual bool accept();
    const cRecStaticDescr& actType() { return record().descr(); }
    void setTabEnabled(int index, bool enable) { pTabWidget->setTabEnabled(index, enable); }
    tRecordList<cTableShape>&tabDescriptors;
    virtual cFieldEditBase * operator[](const QString& __fn);
    virtual void restore(const cRecord *_pRec = NULL);
protected:
    QVBoxLayout            *pVBoxLayout;
    QTabWidget             *pTabWidget;
    QList<cRecordDialog *>  tabs;
private:
    void init();
};

/* ************************************************************************** */

_GEX cRecord * recordDialog(QSqlQuery& q, const QString& sn, QWidget *pPar = NULL, const cRecord *pSample = NULL, bool ro = false, bool edit = false);

static inline QString getTableItemText(QTableWidget *pW, int row, int col) {
    QString r;
    QTableWidgetItem *pItem = pW->item(row, col);
    if (pItem != NULL) r = pItem->text();
    return r;
}

static inline QTableWidgetItem * setTableItemText(const QString& text, QTableWidget *pW, int row, int col) {
    QTableWidgetItem *pItem = pW->item(row, col);
    if (pItem == NULL) {
        pItem = new QTableWidgetItem(text);
        pW->setItem(row, col, pItem);
    }
    else {
        pItem->setText(text);
    }
    return pItem;
}

static inline int getTableItemComboBoxCurrentIndex(QTableWidget *pW, int row, int col)
{
    QWidget *pWidget = pW->cellWidget(row, col);
    if (pWidget == NULL) return -1;
    if (!pWidget->inherits("QComboBox")) EXCEPTION(EDATA,0, QObject::trUtf8("A %1 objektum nem konvertálható QComboBox pointerré").
                                                   arg(typeid(pWidget).name()));
    QComboBox *pComboBox = qobject_cast<QComboBox *>(pWidget);
    return pComboBox->currentIndex();
}

/* ************************************************************************** */

_GEX cRecord *objectDialog(const QString& name, QSqlQuery& q, QWidget *pPar, cRecord * _pSample = NULL, bool ro = false, bool edit = false);

#endif // RECORD_DIALOG_H
