#ifndef TOOLTABLE_H
#define TOOLTABLE_H

#include    "record_table.h"
#include    "tool_table_model.h"

class LV2GSHARED_EXPORT cToolTable : public cRecordTable {
    Q_OBJECT
public:
    cToolTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = nullptr, QWidget * par = nullptr);
    virtual int ixToForeignKey();
    virtual QStringList where(QVariantList& qParams);
};


#endif // TOOLTABLE_H
