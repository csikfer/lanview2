#ifndef LV2TOOLDATA_H
#define LV2TOOLDATA_H

#include "lv2data.h"

enum eToolType {
    TT_COMMAND,
    TT_PARSE,
    TT_URL,
};

EXT_ const QString& toolType(int e, eEx __ex);
EXT_ int toolType(const QString& n, eEx __ex);

class LV2SHARED_EXPORT cTool : public cRecord {
    CRECORD(cTool);
    FEATURES(cTool)
public:
    /// Szöveg azonosítók
    enum eTextIndex {
        LTX_TITLE = 0,
        LTX_TOOL_TIP,
    };
};

class LV2SHARED_EXPORT cToolObject : public cRecord {
public:
    CRECORD(cToolObject);
    FEATURES(cToolObject)
    QString substituted();
    cTool       tool;
    cRecordAny  object;
protected:
    cFeatures   _mergedFeatures;
};

#endif // LV2TOOLDATA_H
