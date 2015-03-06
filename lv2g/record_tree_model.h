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
    ~cTreeNode();
    cTreeNode          *parent;
    cRecordAny         *pData;
    QList<cTreeNode *> *pChildrens;

    QString name() { return (pData == NULL) ? "//" : pData->getName(); }
    void clearChild();
    void addChild(cTreeNode *cn) { pChildrens->append(cn); }
    int row() const;
};

class cRecordTreeModel : public QAbstractItemModel, public cRecordViewModelBase {
    Q_OBJECT
public:
    cRecordTreeModel(const cRecordTree &_rt);
    ~cRecordTreeModel();
    cTreeNode * pRootNode;

    QSqlQuery   *pq;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& child) const;
    int         rowCount(const QModelIndex &parent) const;
    int         columnCount( const QModelIndex & parent = QModelIndex()) const;
    QVariant    data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    QVariant    headerData(int section, Qt::Orientation orientation, int role) const;
//    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool    canFetchMore(const QModelIndex &parent) const;
    void    fetchMore(const QModelIndex &parent);

    cTreeNode * nodeFromIndex(const QModelIndex& index) const;

    void fetchTree();
    void readChilds(cTreeNode *pNode);

};


#endif // RECORD_TREE_MODEL_H
