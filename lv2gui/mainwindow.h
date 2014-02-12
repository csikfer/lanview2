#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "lv2g.h"
#include "record_table.h"
#include <QMainWindow>

class cMainWindow;
class cMenuAction;

class cOwnTab : public QWidget {
    Q_OBJECT
public:
    cOwnTab(cMainWindow& _mw);
    cMainWindow&    mainWindow;
    virtual QWidget *pWidget();
    virtual cOwnTab *closed(cMenuAction * pa);
signals:
    void closeIt();
};

/// @class cMenuAction
/// A menu pontok funkcióit megvalósító objektum.
class cMenuAction : public QObject {
    Q_OBJECT
public:
    cMenuAction(cTableShape *ps, QAction *pa, cMainWindow *par);
    cMenuAction(const QString&  ps, QAction *pa, cMainWindow * par);
    cMenuAction(cOwnTab *po, QAction *pa, cMainWindow * par);
    ~cMenuAction();
    cMainWindow&     mainWindow;    ///< A szülő objektum, fő ablakra mutató referencia.
    cOwnTab         *pOwnTab;       ///< ? A menüponthoz tartozó tab ?
    cTableShape     *pTableShape;   ///<
    cRecordTable    *pRecordTable;
    QWidget         *pWidget;
    QDialog         *pDialog;
    QAction         *pAction;       ///< A menü elemhez tartozó QAction objektum
private:
    void initRecordTable();
public slots:
    void openIt();
    void closeIt();
    void exec();
    void closed();
};

/// @class cMainWindow
/// A fő ablak objektum
/// Az objektumban nincs közvetlen hivatkozás a cMenuAction objektumokra, azoknak szülője.
class cMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /// Konstruktor
    /// Ha _setupOnly true, akkor csak a setup tab-ot jeleníti meg, egy minimális menüvel.
    /// Ha _setupOnly false, akkor üres üres tab mellett, felolvassa az adatbáziskól a menüt, és megjeleníti.
    cMainWindow(bool _setupOnly, QWidget *parent = 0);
    ~cMainWindow();
    /// A QTabWidget objektum pointere, a fő ablak munkaterülete.
    QTabWidget *pTabWidget;
private:
    ///
    void action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq = NULL);
    ///
    void setShapeAction(QSqlQuery *pq, const QString mp, cMenuItem &_mi, QAction *_pa);
    ///
    void setOwnAction(QSqlQuery *pq, const QString mp, cMenuItem &_mi, QAction *_pa, const tMagicMap& _mm);
    ///
    void setExecAction(const QString mp, QAction *pa);
};



#endif // MAINWINDOW_H
