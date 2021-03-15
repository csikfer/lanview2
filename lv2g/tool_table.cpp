#include "tool_table.h"

cToolTable::cToolTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{

}

int cToolTable::ixToForeignKey()
{
    int ix = recDescr().toIndex(_sObjectId);
    return ix;
}

QStringList cToolTable::where(QVariantList& qParams)
{
    DBGFN();
    QStringList wl;
    if (flags & (RTF_IGROUP | RTF_NGROUP | RTF_IMEMBER | RTF_NMEMBER)
     || flags & RTF_CHILD) {
        EXCEPTION(ECONTEXT, flags, tr("Invalid table shape type."));
    }
    if (pUpper == nullptr) EXCEPTION(EPROGFAIL);
    if (owner_id == NULL_ID) {  // A tulajdonos/tag rekord nincs kiválasztva
        wl << _sFalse;      // Ezzel jelezzük, hogy egy üres táblát kell megjeleníteni
        return wl;          // Ez egy üres tábla lessz!!
    }
    int ofix = ixToForeignKey();
    wl << pUpper->recDescr().tableName() + " = " + quoted(_sObjectName);
    wl << dQuoted(recDescr().columnName(ofix)) + " = " + QString::number(owner_id);
    wl << filterWhere(qParams);
    wl << refineWhere(qParams);
    DBGFNL();
    return wl;
}
