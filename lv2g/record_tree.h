#ifndef RECORD_TREE_H
#define RECORD_TREE_H

#include    "QTreeWidget"
#include    "record_table.h"
#include    "record_tree_model.h"


/// @class cRecordTree
/// Egy adatbázis tábla megjelenítését végző objektum.
class cRecordTree : public cRecordViewBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// Fő ill. önálló tábla megjelenítése, már beolvasott leíró
    /// @param _mn A tábla megjelenítését leíró rekord neve (table_shapes.table_shape_name)
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordTree(cTableShape *pts, bool _isDialog, cRecordViewBase * _upper, QWidget * par = NULL);

    ~cRecordTree();
    void init();

    bool queryNodeChildrens(QSqlQuery &q, cTreeNode *pn);

    virtual void refresh(bool first = true);
    virtual void modify();
    virtual void remove();
    virtual void insert();
    virtual void setEditButtons();
    virtual void setPageButtons();
    virtual void setButtons();

    virtual void initSimple(QWidget * pW);

    cRecordTreeModel *  pTreeModel;
    QTreeView *         pTreeView;
};


#endif // RECORD_TREE_H
