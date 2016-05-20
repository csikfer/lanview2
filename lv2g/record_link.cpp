
#include "record_link.h"

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{
    ;
}


cRecordLink::~cRecordLink()
{
    ;
}

QStringList cRecordLink::where(QVariantList& qParams)
{
    QStringList wl;
    int ofix = recDescr().toIndex(_sNodeId1);
    wl << dQuoted(recDescr().columnName(ofix)) + " = " + QString::number(owner_id);
    wl << filterWhere(qParams);
    wl << refineWhere(qParams);
    return wl;
}

void cRecordLink::insert()
{
    /*
    if (flags & RTF_CHILD) {
        if (pUpper == NULL) EXCEPTION(EPROGFAIL);
        if (owner_id == NULL_ID) return;
    }
    if (pRecordDialog != NULL) {
        pDelete(pRecordDialog);
    }
    // A dialógusban megjelenítendő nyomógombok.
    int buttons = enum2set(DBT_OK, DBT_INSERT, DBT_CANCEL);
    cRecord rec(pRecDescr);
    // Ha CHILD, akkor a owner id adott
    if (flags & RTF_CHILD) {
        int ofix = rec.descr().ixToOwner();
        rec[ofix] = owner_id;
    }
    cRecordDialog   rd(*pTableShape, buttons, true, pWidget());  // A rekord szerkesztő dialógus
    pRecordDialog = &rd;
    rd.restore(&rec);
    while (1) {
        int r = rd.exec();
        if (r == DBT_INSERT || r == DBT_OK) {   // Csak az OK, és Insert gombra csinálunk valamit
            bool ok = rd.accept();
            if (ok) {
                cRecord *pRec = rd.record().dup();
                ok = pModel->insertRec(pRec);
                if (!ok) pDelete(pRec);
                else if (flags & RTF_IGROUP) {    // Group, tagja listába van a beillesztés?
                    ok = cGroupAny(*pRec, *(pUpper->actRecord())).insert(*pq, false);
                    if (!ok) {
                        QMessageBox::warning(pWidget(), trUtf8("Hiba"), trUtf8("A kijelölt tag felvétele az új csoportba sikertelen"),QMessageBox::Ok);
                        refresh();
                        break;
                    }
                }
            }
            if (ok) {
                if (r == DBT_OK) break; // Ha OK-t nyomott becsukjuk az dialóg-ot
                continue;               // Ha Insert-et, akkor folytathatja a következővel
            }
            else {
                //QMessageBox::warning(pWidget(), trUtf8("Az új rekord beszúrása sikertelen."), rd.errMsg());
                continue;
            }
        }
        break;
    }
    pRecordDialog = NULL;
*/
}

void cRecordLink::modify(eEx __ex)
{

}

void cRecordLink::remove()
{

}

/* ******************************************************************************************************************* *

const int cLinkDialog::buttons = ENUM2SET4(DBT_OK, DBT_CANCEL, DBT_NEXT, DBT_PREV);

cLinkDialog::cLinkDialog(cPhsLink rec, const cTableShape &__tm, bool dialog = true, QWidget * parent = NULL)
    : cRecordDialog(__tm, buttons, true, parent)
{

}
*/
