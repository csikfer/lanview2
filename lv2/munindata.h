#ifndef MUNINDATA
#define MUNINDATA

#include "lanview.h"
#include "lv2datab.h"

enum eVarAggregateType {
    VAT_INVALID = -1,
    VAT_AVARAGE =  0,
    VAT_MIN,
    VAT_MAX
};

EXT_ int varAggregateType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& varAggregateType(int _i, eEx __ex = EX_ERROR);


enum eServiceVarType {
    SVT_INVALID = -1,
    SVT_DERIVE  =  0,
    SVT_COUNTER,
    SVT_GAUGE,
    SVT_ABSOLUTE,
    SVT_COMPUTE
};

EXT_ int serviceVarType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& serviceVarType(int _i, eEx __ex = EX_ERROR);

enum eVarDrawType {
    VDT_INVALID = -1,
    VDT_LINE = 0,
    VDT_AREA,
    VDT_STACK,
    VDT_NONE
};

EXT_ int varDrawType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& varDrawType(int _i, eEx __ex = EX_ERROR);


/*!
@class cRrdBeats
@brief
*/
class LV2SHARED_EXPORT cRrdBeat : public cRecord {
    CRECORD(cRrdBeat);
    FEATURES(cRrdBeat)
public:
};

/*!
@class cServiceVarType
@brief
 */
class LV2SHARED_EXPORT cServiceVarType : public cRecord {
    CRECORD(cServiceVarType);
    FEATURES(cServiceVarType)
public:
};

/*!
@class cServiceVar
@brief
 */
class LV2SHARED_EXPORT cServiceVar : public cRecord {
    CRECORD(cServiceVar);
    FEATURES(cServiceVar)
public:
};

/*!
@class cGraph
@brief
 */
class LV2SHARED_EXPORT cGraph : public cRecord {
    CRECORD(cGraph);
    FEATURES(cGraph)
public:
};

/*!
@class cGraphVar
@brief
 */
class LV2SHARED_EXPORT cGraphVar : public cRecord {
    CRECORD(cGraphVar);
    FEATURES(cGraphVar)
public:
};



#endif // MUNINDATA

