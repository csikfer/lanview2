#ifndef MUNINDATA
#define MUNINDATA

#include "lanview.h"
#include "guidata.h"
#include "srvdata.h"

enum eVarAggregateType {
    VAT_AVARAGE =  0,
    VAT_MIN,
    VAT_MAX,
    VAT_LAST
};

EXT_ int varAggregateType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& varAggregateType(int _i, eEx __ex = EX_ERROR);


enum eServiceVarType {
    SVT_GAUGE  =  0,
    SVT_COUNTER,
    SVT_DCOUNTER,
    SVT_DERIVE,
    SVT_DDERIVE,
    SVT_ABSOLUTE,
    SVT_COMPUTE
};

EXT_ int serviceVarType(const QString& _n, eEx __ex = EX_ERROR);
EXT_ const QString& serviceVarType(int _i, eEx __ex = EX_ERROR);

enum eVarDrawType {
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
    RECACHEHED(cServiceVarType, srvartype)
    STATICIX(cServiceVarType, ParamTypeId)
    STATICIX(cServiceVarType, RawParamTypeId)
};

/*!
@class cServiceVar
@brief
 */
class LV2SHARED_EXPORT cServiceVar : public cRecord {
    CRECORD(cServiceVar);
    FEATURES(cServiceVar)
public:
    virtual void clearToEnd();
    virtual void toEnd();
    virtual bool toEnd(int _ix);
    const cServiceVarType * varType(QSqlQuery& q, eEx __ex = EX_ERROR);
    const cParamType& dataType(QSqlQuery& q)    { return cParamType::paramType(varType(q)->getId(cServiceVarType::ixParamTypeId())); }
    const cParamType& rawDataType(QSqlQuery& q) { return cParamType::paramType(varType(q)->getId(cServiceVarType::ixRawParamTypeId())); }
    QString  valToString(QSqlQuery& q, const QVariant& val)  { return dataType(q).paramToString(val); }
    QVariant valFromString(QSqlQuery& q, const QString& val) { return dataType(q).paramFromString(val); }
    QString  rawValToString(QSqlQuery& q, const QVariant& val)  { return rawDataType(q).paramToString(val); }
    QVariant rawValFromString(QSqlQuery& q, const QString& val) { return rawDataType(q).paramFromString(val); }
    int setValue(QSqlQuery& q, const QVariant& _rawVal, int& state);
    int setValue(QSqlQuery& q, double val, int& state, eTristate rawChg = TS_NULL);
    int setValue(QSqlQuery& q, qlonglong val, int& state, eTristate rawChg = TS_NULL);
    static int setValue(QSqlQuery& q, qlonglong _hsid, const QString& _name, const QVariant& val, int &state);
    static int setValues(QSqlQuery& q, qlonglong _hsid, const QStringList& _names, const QVariantList& vals, int &state);
    static QString sInvalidValue;
    static QString sNotCredible;
    static QString sFirstValue;
    static QString sRawIsNull;
protected:
    /// Ha raw érték változott, akkor true-val tér vissza
    bool preSetValue(QSqlQuery &q, const QVariant &rawVal);
    int setCounter(QSqlQuery &q, qlonglong val, int svt, int& state);
    int setDerive(QSqlQuery &q, double val, int& state);
    int updateVar(QSqlQuery& q, eParamType pt, const QVariant&  val, int& state);
    int updateVar(QSqlQuery& q, qlonglong val, int& state);
    int updateVar(QSqlQuery& q, double val, int& state);
    int noValue(QSqlQuery& q, int& state, int _st = RS_UNREACHABLE);
    /// Egy egész típusú értékre a megadott feltétel alkalmazása
    /// @param val A viszgálandó érték
    /// @param ft A feltétel típusa Lsd.: eFilterType csak az egész számra értelmezhető feltételek adhatóak meg.
    /// @param _p1 A feltétel első paramétere.
    /// @param _p2 A feltétel második paramétere.
    /// @param _inverse Az eredményt invertálni kell, ha igaz.
    /// @return Ha nincs összehasonlítási feltétel, akkor TS_NULL. Egyébként az összehasonlítás eredménye.
    /// Szintén TS_NULL-al tér vissza, ha egy szükséges paraméter nem konvertálható egész számmá.
    eTristate checkIntValue(qlonglong val, qlonglong ft, const QString &_p1, const QString &_p2, bool _inverse);
    /// Egy valós típusú értékre a megadott feltétel alkalmazása
    /// @param val A viszgálandó érték
    /// @param ft A feltétel típusa Lsd.: eFilterType csak az egész számra értelmezhető feltételek adhatóak meg.
    /// @param _p1 A feltétel első paramétere.
    /// @param _p2 A feltétel második paramétere.
    /// @param _inverse Az eredményt invertálni kell, ha igaz.
    /// @return Ha nincs összehasonlítási feltétel, akkor TS_NULL. Egyébként az összehasonlítás eredménye.
    /// Szintén TS_NULL-al tér vissza, ha egy szükséges paraméter nem konvertálható számmá.
    eTristate checkRealValue(double val, qlonglong ft, const QString& _p1, const QString& _p2, bool _inverse);
    eTristate checkIntervalValue(qlonglong val, qlonglong ft, const QString& _p1, const QString& _p2, bool _inverse);
    void addMsg(const QString& _msg) {
        QString msg = getName(_ixStateMsg);
        if (!msg.isEmpty()) msg += "\n";
        setName(_ixStateMsg, msg + _msg);
    }
    static tRecordList<cServiceVar>     serviceVars;  ///< Cache
    static QMap<qlonglong, qlonglong>   heartbeats;   ///< Heartbeat cache
    const cServiceVarType *pVarType;
    double      lastValue;      ///< Derived esetén az előző érték
    qlonglong   lastCount;      ///< Ha számláló a lekérdezett érték, akkor az előző érték
    QDateTime   lastTime;       ///< Ha számláló a lekérdezett érték, akkor az előző érték időpontja
    QDateTime   lastLast;       ///< last_time mező előző értéke
    QDateTime   now;            ///< most
    eNotifSwitch state;
    STATICIX(cServiceVar, ServiceVarTypeId)
    STATICIX(cServiceVar, ServiceVarValue)
    STATICIX(cServiceVar, VarState)
    STATICIX(cServiceVar, LastTime)
    STATICIX(cServiceVar, RawValue)
    STATICIX(cServiceVar, StateMsg)
    STATICIX(cServiceVar, Rarefaction)
protected:
    static QBitArray updateMask;
    int    skeepCnt;
    bool   skeep();
public:
    static void resetCacheData() { serviceVars.clear(); heartbeats.clear(); }
    static cServiceVar * serviceVar(QSqlQuery&__q, qlonglong hsid, const QString& name, eEx __ex = EX_ERROR);
    static cServiceVar * serviceVar(QSqlQuery&__q, qlonglong svid, eEx __ex = EX_ERROR);
    /// A heartbeat értéket olvassa ki a host_services vagy services rekordból, az értéket csak az első alkalommal
    /// olvassa ki az adatbázisból, és letárolja a heartbeats konténerbe. ...
    qlonglong heartbeat(QSqlQuery&__q, eEx __ex = EX_ERROR);
    bool initSkeepCnt(int& delayCnt);
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

