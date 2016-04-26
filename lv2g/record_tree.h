#ifndef RECORD_TREE_H
#define RECORD_TREE_H

#include    "QTreeWidget"
#include    "record_table.h"
#include    "record_tree_model.h"


/// @class cRecordTree
/// Egy adatbázis tábla megjelenítését végző objektum.
class LV2GSHARED_EXPORT cRecordTree : public cRecordsViewBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// Fő ill. önálló tábla megjelenítése, már beolvasott leíró
    /// @param pts A tábla megjelenítését leíró rekord
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordTree(cTableShape *pts, bool _isDialog, cRecordsViewBase * _upper, QWidget * par = NULL);

    ~cRecordTree();
    void init();

    bool queryNodeChildrens(QSqlQuery &q, cTreeNode *pn);

    virtual QModelIndexList selectedRows();
    virtual QModelIndex actIndex();
    virtual cRecord *actRecord(const QModelIndex &_mi);
    virtual cRecord *nextRow(QModelIndex *pMi, int _upRes = 1);
    virtual cRecord *prevRow(QModelIndex *pMi, int _upRes = 1);
    virtual void selectRow(const QModelIndex& mi);


    virtual void _refresh(bool all = false);
    virtual void setEditButtons();
    virtual void setButtons();
    virtual void prev();
    void setRoot();
    void restoreRoot();
    void expand();

    virtual void initSimple(QWidget * pW);

    cRecordTreeModel *  pTreeModel() const { return static_cast<cRecordTreeModel *>(pModel); }
    QTreeView *         pTreeView;
    virtual void buttonPressed(int id);
};


#endif // RECORD_TREE_H
