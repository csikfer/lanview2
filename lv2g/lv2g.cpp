#include "lv2g.h"

void initLV2GUI()
{
    static bool inited = false;
    if (!inited) {
        _setGUITitles();
        inited = true;
    }
}

/* Ez valami régebbi visszamaradt kód
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
*/

void _setGUITitles()
{
    if (_titleWarning.isEmpty()) {
        _titleWarning       = QObject::trUtf8("Figyelmeztetés");
        _titleError         = QObject::trUtf8("Hiba");
        _titleInfo          = QObject::trUtf8("Megjegyzés");
    }
}


cOwnTab::cOwnTab(QWidget *par)
    : QWidget(par)
{
    setVisible(false);
}

void cOwnTab::endIt()
{
    closeIt();
}
