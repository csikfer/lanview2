#include "tool_table.h"

cToolTable::cToolTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{

}

int cToolTable::ixToForeignKey()
{
    int ix;
    return ix;
}
