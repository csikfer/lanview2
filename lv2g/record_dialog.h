#ifndef RECORD_DIALOG_H
#define RECORD_DIALOG_H

#include "lv2models.h"
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QLayout>
#include <QtWidgets/QDialog>
#include <QTableWidget>
#include <QEventLoop>
#include "lv2widgets.h"

/// @enum eDialogButtons
/// @brief A dialógus ablakokban kezelt nyomógombok típusait reprezentáló enumerációs konstansok.
enum eDialogButtons {
    DBT_OK = 0,
    DBT_CLOSE,
    DBT_REFRESH,
    DBT_INSERT,
    DBT_MODIFY,
    DBT_SAVE,
    DBT_FIRST,
    DBT_PREV,
    DBT_NEXT,
    DBT_LAST,
    DBT_DELETE,
    DBT_RESTORE,
    DBT_CANCEL,
    DBT_BUTTON_NUMBERS  ///< Nem egy nyomógombot reprezentál, hanem azok számát
};

/// @class cDialogButtons
/// @brief A dialogus ablakokban nyomodombok kezelése
class LV2GSHARED_EXPORT cDialogButtons : public QButtonGroup {
public:
    cDialogButtons(int buttons, int buttons2 = 0, QWidget *par = NULL);
    void init(int buttons, QHBoxLayout *pL);
    QWidget& widget()   { return *_pWidget; }
    QWidget *pWidget()  { return _pWidget; }
protected:
    QWidget *_pWidget;
    QVBoxLayout *_pLayout;
    QHBoxLayout *_pLayoutTop;
    QHBoxLayout *_pLayoutBotom;
    static QStringList      buttonNames;
    static QList<QIcon>     icons;
};

/// @class cRecordDialogBase
/// @brief Rekord szerkesztés dialógus alap objektum
/// Konkrét megjelenítést nem tartalmaz.
class LV2GSHARED_EXPORT cRecordDialogBase : public QObject {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param __tm A rekord megjelenítését leíró objektum példány
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param par Az szülő widget opcionális parent pointere
    cRecordDialogBase(const cTableShape &__tm, int _buttons, bool dialog = true, QWidget *par = NULL);
    /// destruktor
    ~cRecordDialogBase();
    virtual cRecord& record() = 0;
    /// A dialógust megvalósító widget objektum referenciájával tér vissza
    QWidget& widget()   { return *_pWidget; }
    /// A dialógust megvalósító widget objektum pointerével tér vissza
    QWidget *pWidget()  { return _pWidget; }
    /// A dialógust megvalósító dialógus objektum referenciájával tér vissza, ha nem QDialóg objektum valósítja meg a dialógust, akkor dob egy kizárást.
    QDialog& dialog()   { if (!isDialog) EXCEPTION(EPROGFAIL); return *(QDialog *)_pWidget; }
    /// A hiba string tartalmával tér vissza
    const QString& errMsg() const { return _errMsg; }
    /// Ha a _close paraméter true. akkor feltételezi, hogy QDialógus objektum a dialógus ablakunk, és hívja a QDialog::exec() metódust.
    /// Ha viszont _close false, akkor indít a widgetre egy loop-ot (QEventLoop). Ekkor egy nyomógomb megnyomásakor nem csukódik be a dialógusunk.
    /// Annak becsukásáról, ill az exec() metódus újra hívásáról mi döntünk.
    /// Ha _close igaz, de az isDialog adattagunk hamis, akkor kapunk egy kizárást.
    /// @return A dialógus ablak kilépési kódja, vagyis az aktívált nyomógombot reprezentáló enumeráxiós konstans értéke.
    int exec(bool _close = true);
    /// A tábla model rekord
    const cTableShape&      descriptor;
    /// Az objektum neve
    const QString           name;
public slots:
    bool close()        { return _pWidget->close(); }
    void show()         { _pWidget->show(); }
    void hide()         { _pWidget->hide(); }
protected:
    /// A dialógust megvalósító QWidget, vagy QDialog objektum példány.
    QWidget                *_pWidget;
    /// Ha a dialógus ténylegesen egy QDialog objektumként lett létrehozva, akkor értéke true.
    bool                    isDialog;
    /// A nyomógombokat kezelő objektum példány
    cDialogButtons         *_pButtons;
    ///
    QEventLoop             *_pLoop;
    /// Az adatbázis hozzáférést biztosító objektum példány
    QSqlQuery              *pq;
    /// Ha a dialógus nem szerkeszthető, akkor értéke true.
    bool                    isReadOnly;
    /// A szerkesztett értékek rekordba másolásakor a hiba üzenetek buffere
    QString                 _errMsg;
private slots:
    /// Slot a megnyomtak egy gombot szignálra.
    /// @param id a megnyomott gomb kódja
    void _pressed(int id);
 signals:
     void buttonPressed(int id);
};

/// @class cRecordDialog
/// @brief Rekord szerkesztés dialógus objektum
/// Tartalmazza (felépíti) a rekord mezőinek a megjelenítését, ill. a szerkesztést megvalósító widget-eket is.
class LV2GSHARED_EXPORT cRecordDialog : public cRecordDialogBase {
public:
    /// Konstruktor
    /// @param rec Az editálandó adat objktum referenciája
    /// @param __tm A rekord/tábla megjelenítését ill szerkesztését vezérlő leíró
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param parent Az szülő widget opcionális parent pointere
    cRecordDialog(cRecord& rec, cTableShape &__tm, int _buttons, bool dialog = true, QWidget * parent = NULL);
    ///
    virtual cRecord& record();
    /// A rekord adattag tartalmának a megjelenítése/megjelenítés visszaállítása
    void restore();
    /// A megjelenített értékek kiolvasása
    bool accept();
    /// Az adat rekord referenciája (konstansnak kellene lennie, de macerás)
    cRecord&                _record;
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
public:
    /// Konstruktor
    /// @param _tm A rekord/tábla megjelenítését ill szerkesztését vezérlő leíró (alap tábla ill. rekord típusra)
    /// @param _tms A további lehetséges rekord leírók (vagy az összes ?)
    /// @param _buttons A megjelenítendő nyomógombok bit maszkja
    /// @param _oid Tulajdonos rekord ID ?? Opcionális?
    /// @param dialog Ha a dialóus ablakot QDialog-ból kell létrehozni, akkor true, ha fals, akkor QWidget-ből.
    /// @param parent Az szülő widget opcionális parent pointere
    cRecordDialogInh(const cTableShape &_tm, tRecordList<cTableShape>& _tms, int _buttons, qlonglong _oid, bool dialog = true, QWidget * parent = NULL);
    ///
    virtual cRecord& record();
    int actTab();
    void setActTab(int i);
    cRecordDialog& actDialog()  { return *tabs[actTab()]; }
    cRecordDialog *actPDialog() { return  tabs[actTab()]; }

/*    /// A megjelenített értékek beállítása az aktuális tab-on
    void set(const cRecord& _r) { tabs[actTab()]->set(_r); }*/
    ///
    bool accept();
    const cRecStaticDescr& actType() { return recs[actTab()]->descr(); }
    void setTabEnabled(int index, bool enable) { pTabWidget->setTabEnabled(index, enable); }
    tRecordList<cTableShape>&tabDescriptors;
protected:
    QVBoxLayout            *pVBoxLayout;
    QTabWidget             *pTabWidget;
    tRecordList<cRecordAny> recs;
    QList<cRecordDialog *>  tabs;
private:
    void init(qlonglong _oid);
};

#endif // RECORD_DIALOG_H
