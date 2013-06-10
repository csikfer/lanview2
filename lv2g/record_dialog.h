#ifndef RECORD_DIALOG_H
#define RECORD_DIALOG_H

#include "lv2models.h"
#include <QtWidgets/QtWidgets>
#include <QtWidgets/QLayout>
#include <QtWidgets/QDialog>
#include <QTableWidget>
#include <QEventLoop>
#include "lv2widgets.h"

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
    DBT_BUTTON_NUMBERS
};

class cDialogButtons : public QButtonGroup {
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

class LV2GSHARED_EXPORT cRecordDialogBase : public QObject {
    Q_OBJECT
public:
    cRecordDialogBase(const cTableShape &__tm, int _buttons, bool dialog = true, QWidget *par = NULL);
    ~cRecordDialogBase();
    QWidget& widget()   { return *_pWidget; }
    QWidget *pWidget()  { return _pWidget; }
    QDialog& dialog()   { if (!isDialog) EXCEPTION(EPROGFAIL); return *(QDialog *)_pWidget; }
    bool close()        { return _pWidget->close(); }
    void show()         { _pWidget->show(); }
    void hide()         { _pWidget->hide(); }
    const QString& errMsg() const { return _errMsg; }
    int exec(bool _close = true);
    /// A tábla model rekord
    const cTableShape&      descriptor;
    const QString           name;
protected:
    QWidget                *_pWidget;
    bool                    isDialog;
    cDialogButtons         *_pButtons;
    QEventLoop             *_pLoop;
    QSqlQuery              *pq;
    bool                    isReadOnly;
    QString                 _errMsg;
private slots:
    void _pressed(int id);
 signals:
     void buttonPressed(int id);
};

class LV2GSHARED_EXPORT cRecordDialog : public cRecordDialogBase {
public:
    /// Konstruktor
    /// @param rec Az editálandó adat objktum referenciája
    /// @param __mn A leíró, table_models rekord neve. Ha a rekord egyértelmüen beazonosítható a rec-hez tartopzó tábla név alapján, akkor nem kell megadni, ill. lehet öres string
    /// @param parent A parent widget
    cRecordDialog(cRecord& rec, cTableShape &__tm, int _buttons, bool dialog = true, QWidget * parent = NULL);
    /// A megjelenített értékek beállítása
    void set(const cRecord& _r);
    /// A megjelenített értékek kiolvasása
    bool get(cRecord &_r);
    ///
    /// Az adat rekord referenciája (konstansnak kellene lennie, de macerás)
    cRecord&                record;
protected:
    QVBoxLayout            *pVBoxLayout;
    //QHBoxLayout            *pHBoxLayout;
    QBoxLayout             *pSplittLayout;
    QSplitter              *pSplitter;
    QFormLayout            *pFormLayout;
    QList<cFieldEditBase *> fields;
private:
    void init();
    void restore();
};

class LV2GSHARED_EXPORT cRecordDialogTab : public cRecordDialogBase {
public:
    cRecordDialogTab(const cTableShape &_tm, tRecordList<cTableShape>& _tms, int _buttons, qlonglong _oid, bool dialog = true, QWidget * parent = NULL);
    int actTab();
    void setActTab(int i);

    /// A megjelenített értékek beállítása az aktuális tab-on
    void set(const cRecord& _r) { tabs[actTab()]->set(_r); }
    ///
    bool get(cRecord& _r);
    const cRecStaticDescr& actType() { return recs[actTab()]->descr(); }
    void setTabEnabled(int index, bool enable) { pTabWidget->setTabEnabled(index, enable); }
    tRecordList<cTableShape>&tabDescriptors;
protected:
    QVBoxLayout            *pVBoxLayout;
    QTabWidget             *pTabWidget;
    tRecordList<cAlternate> recs;
    QList<cRecordDialog *>  tabs;
private:
    void init(qlonglong _oid);
};

#endif // RECORD_DIALOG_H
