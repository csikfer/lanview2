#ifndef LV2SYNTAX_H
#define LV2SYNTAX_H

#include "lv2data.h"

class LV2SHARED_EXPORT cObjectSyntax : public cRecord {
    CRECORD(cObjectSyntax);
    FEATURES(cObjectSyntax)
};

EXT_ cError * exportRecords(const QString& __tn, QString str);

#endif // LV2SYNTAX_H
