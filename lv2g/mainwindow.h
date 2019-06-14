#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "lv2g.h"
#include "record_table.h"
class cMenuAction;

/// @class cMainWindow
/// A fő ablak objektum
/// Az objektumban nincs közvetlen hivatkozás a cMenuAction objektumokra, azoknak szülője.
class LV2GSHARED_EXPORT cMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /// Konstruktor
    /// Ha _setupOnly true, akkor csak a setup tab-ot jeleníti meg, egy minimális menüvel.
    /// Ha _setupOnly false, akkor üres üres tab mellett, felolvassa az adatbáziskól a menüt, és megjeleníti.
    cMainWindow(QWidget *parent = nullptr);
    ~cMainWindow();
    /// Init
    void init(bool _setup = false);
    /// A QMdiArea objektum pointere, a fő ablak munkaterülete.
    QMdiArea   *pMdiArea;
private:
    ///
    void action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq = nullptr);
    /// Ha nincs menű, akkor a beállításokat lehetővétevő menu beállítása.
    void setSetupMenu();
// private slots:
//    void widgetSplitterOrientation(int index);
};



#endif // MAINWINDOW_H
