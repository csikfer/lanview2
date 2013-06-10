#include "lv2g.h"


QList<int>  selection2rows(const QItemSelection& _sel)
{
    QModelIndex index;
    QList<int>  rows;
    foreach (index, _sel.indexes()) {
        if (!index.isValid()) continue;
        int row = index.row();
        if (rows.contains(row)) continue;
        rows << row;
    }
    return rows;
}

QList<int>&  modSelectedRows(QList<int>& rows, const QItemSelection& _on, const QItemSelection& _off)
{
    QModelIndex index;
    foreach (index, _on.indexes()) {    // +
        if (!index.isValid()) continue;
        int row = index.row();
        if (rows.contains(row)) continue;
        rows << row;
    }
    foreach (index, _off.indexes()) {    // -
        if (!index.isValid()) continue;
        int i = rows.indexOf(index.row());
        if (i < 0) continue;
        rows.removeAt(i);
    }
    return rows;
}
