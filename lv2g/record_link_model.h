#ifndef RECORD_LINK_MODEL
#define RECORD_LINK_MODEL

#include "record_table_model.h"

class cRecordLink;

/// @class cRecordLinkModel
/// Model osztály adatatbázis tábla táblázatos megjelenítéséhez
class cRecordLinkModel : public cRecordTableModel {
    Q_OBJECT
public:
    cRecordLinkModel(cRecordLink &_rt);
    ~cRecordLinkModel();
};

#endif // RECORD_LINK_MODEL

