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

class cMenuAction : public QObject {
    Q_OBJECT
public:
    cMenuAction(cTableShape *ps, QAction *pa, cMainWindow *par);
    cMenuAction(const QString&  ps, QAction *pa, cMainWindow * par);
    cMenuAction(cOwnTab *po, QAction *pa, cMainWindow * par);
    ~cMenuAction();
    cMainWindow&     mainWindow;
    cOwnTab         *pOwnTab;
    cTableShape     *pTableShape;
    cRecordTable    *pRecordTable;
    QWidget         *pWidget;
    QDialog         *pDialog;
    QAction         *pAction;
private:
    void initRecordTable();
public slots:
    void openIt();
    void closeIt();
    void exec();
    void closed();
};

class cMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    cMainWindow(bool _setupOnly, QWidget *parent = 0);
    ~cMainWindow();
    QTabWidget *pTabWidget;
private:
    void action(QAction *pa, cMenuItem& _mi, QSqlQuery *pq = NULL);
    void setShapeAction(QSqlQuery *pq, const QString mp, cMenuItem &_mi, QAction *_pa);
    void setOwnAction(QSqlQuery *pq, const QString mp, cMenuItem &_mi, QAction *_pa, const tMagicMap& _mm);
    void setExecAction(const QString mp, QAction *pa);
};



#endif // MAINWINDOW_H
