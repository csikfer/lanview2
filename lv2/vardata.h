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
    int setValue(QSqlQuery& q, double val, int& state, QString *pMsg = nullptr);
    int setValue(QSqlQuery& q, qulonglong val, int& state, QString *pMsg = nullptr);
    static int setValue(QSqlQuery& q, qlonglong _hsid, const QString& _name, const QVariant& val);
    static int setValues(QSqlQuery& q, qlonglong _hsid, const QStringList& _names, const QVariantList& vals);
protected:
    void preSetValue(const QString& val);
    int setCounter(QSqlQuery &q, qulonglong val, int svt, int& state);
    int setDerive(QSqlQuery &q, double val, int& state);
    int updateVar(QSqlQuery& q, qulonglong val, int& state);
    int updateVar(QSqlQuery& q, double val, int& state);
    int noValue(QSqlQuery& q, int& state);
    /// Egy egész típusú értékre a megadott feltétel alkalmazása
    /// @param val A viszgálandó érték
    /// @param ft A feltétel típusa Lsd.: eFilterType csak az egész számra értelmezhető feltételek adhatóak meg.
    /// @param _p1 A feltétel első paramétere.
    /// @param _p2 A feltétel második paramétere.
    /// @param _inverse Az eredményt invertálni kell, ha igaz.
    /// @return Ha nincs összehasonlítási feltétel, akkor TS_NULL. Egyébként az összehasonlítás eredménye.
    /// Szintén TS_NULL-al tér vissza, ha egy szükséges paraméter nem konvertálható egész számmá.
    eTristate checkIntValue(qulonglong val, qlonglong ft, const QVariant &_p1, const QVariant &_p2, bool _inverse);
    /// Egy valós típusú értékre a megadott feltétel alkalmazása
    /// @param val A viszgálandó érték
    /// @param ft A feltétel típusa Lsd.: eFilterType csak az egész számra értelmezhető feltételek adhatóak meg.
    /// @param _p1 A feltétel első paramétere.
    /// @param _p2 A feltétel második paramétere.
    /// @param _inverse Az eredményt invertálni kell, ha igaz.
    /// @return Ha nincs összehasonlítási feltétel, akkor TS_NULL. Egyébként az összehasonlítás eredménye.
    /// Szintén TS_NULL-al tér vissza, ha egy szükséges paraméter nem konvertálható számmá.
    eTristate checkRealValue(qulonglong val, qlonglong ft, const QVariant& _p1, const QVariant& _p2, bool _inverse);
    void addMsg(const QString& _msg) {
        QString msg = getName(_ixStateMsg);
        if (!msg.isEmpty()) msg += "\n";
        setName(_ixStateMsg, msg + _msg);
    }
    static tRecordList<cServiceVar>     serviceVars;  ///< Cache
    static QMap<qlonglong, qlonglong>   heartbeats;   ///< Heartbeat cache
    const cServiceVarType *pVarType;
    double      lastValue;      ///< Derived esetén az előző érték
    qulonglong  lastCount;      ///< Ha számláló a lekérdezett érték, akkor az előző érték
    QDateTime   lastTime;       ///< Ha számláló a lekérdezett érték, akkor az előző érték időpontja
    QDateTime   lastLast;       ///< last_time mező előző értéke
    eNotifSwitch state;
    STATICIX(cServiceVar, ServiceVarTypeId)
    STATICIX(cServiceVar, ServiceVarValue)
    STATICIX(cServiceVar, StateMsg)
    STATICIX(cServiceVar, VarState)
protected:
    static QBitArray updateMask;
public:
    static void resetCacheData() { serviceVars.clear(); heartbeats.clear(); }
    static cServiceVar * serviceVar(QSqlQuery&__q, qlonglong hsid, const QString& name, eEx __ex = EX_ERROR);
    static cServiceVar * serviceVar(QSqlQuery&__q, qlonglong svid, eEx __ex = EX_ERROR);
    /// A heartbeat értéket olvassa ki a host_services vagy services rekordból, az értéket csak az első alkalommal
    /// olvassa ki az adatbázisból, és letárolja a heartbeats konténerbe. ...
    qlonglong heartbeat(QSqlQuery&__q, eEx __ex = EX_ERROR);
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

