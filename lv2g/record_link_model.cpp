#include "record_link_model.h"
#include "record_link.h"
#include "popupreport.h"

cRecordLinkModel::cRecordLinkModel(cRecordLink& _rt)
    : cRecordTableModel(_rt)
{
    ;
}

cRecordLinkModel::~cRecordLinkModel()
{
    ;
}

void cRecordLinkModel::removeRecords(const QModelIndexList &mil)
{
    QBitArray   rb = index2map(mil);
    if (rb.count(true) == 0) return;
    QString msg = tr("Valóban törölni akarja a kijelölt linke(ke)t ?\n") + sIrrevocable;
    if (!cMsgBox::yes(msg, recordView.pWidget())) return;
    int s = rb.size();    // Az összes rekord száma
    for (int i = s - 1; i >= 0; --i) {   // végigszaladunk a sorokon, visszafelé
        if (rb[i]) removeRec(index(i, 0));
    }

}


bool cRecordLinkModel::removeRec(const QModelIndex &mi)
{
    if (mi.isValid() && isContIx(_records, mi.row())) {
        cRecord * pView = _records.at(mi.row());
        QString viewTableName = pView->tableName();
        cRecordAny o;
        qlonglong id = pView->getId(0);
        if      (0 == viewTableName.compare("lldp_links_shape")) {
            o.setType(_sLldpLinksTable);
            o.setId(id);
        }
        else if (0 == viewTableName.compare("phs_links_shape")) {
            o.setType(_sPhsLinksTable);
            o.setId(id);
        }
        else {
            EXCEPTION(ENOTSUPP, id, viewTableName);
        }

        if (cErrorMessageBox::condMsgBox(o.tryRemove(*pq))) {
            return removeRow(mi);
        }
        else {
            recordView.refresh(true);
        }
    }
    return false;
}
