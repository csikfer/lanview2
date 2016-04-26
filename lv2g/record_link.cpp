
#include "record_link.h"

cRecordLink::cRecordLink(const QString& _mn, bool _isDialog, QWidget * par)
    : cRecordTable(_mn, _isDialog, par)
{
}

cRecordLink::cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper, QWidget * par)
    : cRecordTable(pts, _isDialog, _upper, par)
{
}


cRecordLink::~cRecordLink()
{
    ;
}

