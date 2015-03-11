#ifndef RECORD_TREE_MODEL_H
#define RECORD_TREE_MODEL_H

#include "lv2g.h"
#include "record_table_model.h"



class   cRecordTree;

class cTreeNode;
class cRecordTreeModel;

class cTreeNode {
public:
    cTreeNode(cRecordAny *__po = NULL, cTreeNode *_parentNode = NULL);
    cTreeNode(const cTreeNode&);
    ~cTreeNode();
    cTreeNode          *parent;
    cRecordAny         *pData;
    QList<cTreeNode *> *pChildrens;

    QString name() { return (pData == NULL) ? "//" : pData->getName(); }
    void addChild(cTreeNode *cn) { pChildrens->append(cn); }
    int row() const;
    int rows() const { if (pChildrens == NULL) EXCEPTION(EPROGFAIL); return pChildrens->size(); }
};

class cRecordTreeModel : public QAbstractItemModel, public cRecordViewModelBase {
    Q_OBJECT
public:
    cRecordTreeModel(cRecordTree &_rt);
    ~cRecordTreeModel();
    cTreeNode * pActRootNode;
    cTreeNode * pRootNode;

    /// Egy node keresése az id alapján (rekurzív)
    /// @param pid a keresett ID
    ///  Az ID nem lehet NULL_ID, akkor a keresett index az invalid, ua. mintha nem találna semmit
    /// @param mi a rész fa gyökere, amiben keresünk (invalid esetén a teljes fán keresünk)
    /// @return A keresett node indexe, vagy invalid index, ha nincs találat
    QModelIndex findNode(qlonglong pid, const QModelIndex &mi = QModelIndex());

    QSqlQuery   *pq;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    int         rowCount(const QModelIndex &parent) const;
    int         columnCount( const QModelIndex & parent = QModelIndex()) const;
    QVariant    data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant    headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool    canFetchMore(const QModelIndex &parent) const;
    void    fetchMore(const QModelIndex &parent);

    cTreeNode * nodeFromIndex(const QModelIndex& index) const;

    void refresh(bool first);
    void fetchTree();
    /// Beolvassa a node laszármazottait (nem rekurzív)
    void readChilds(cTreeNode *pNode);
    /// Törli a rész fát és rekordjait
    bool removeNode(cTreeNode *pn);
    /// A kiválasztott sor lessz a gyökér
    void setRoot(const QModelIndex& mi);
    /// Vissalép a fán a pRootNode pointerrel, ha lehet
    /// @param _sing Ha igaz, akkor csak egyet lép vissza, ha hamis, akkor az eredeti gyökérig.
    void prevRoot(bool _sing);

    virtual void removeRecords(const QModelIndexList& mil);
    virtual bool removeRec(const QModelIndex & mi);
    virtual bool removeRow(const QModelIndex & mi);

    int checkUpdateRow(const QModelIndex& mi, cRecordAny * pRec, QModelIndex& new_parent);
    virtual bool updateRec(const QModelIndex& mi, cRecordAny * pRec);
    virtual bool updateRow(const QModelIndex &mi, cRecordAny *pRec);

    virtual bool insertRow(cRecordAny *pRec);

};


#endif // RECORD_TREE_MODEL_H
