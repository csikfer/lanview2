#include "lanview.h"
#include "lv2data.h"
#include "srvdata.h"
#include "lv2service.h"
#include "import_parser.h"
#include "guidata.h"
#include "scan.h"
#include "vardata.h"
#include "report.h"

/* ******************************  ****************************** */
DEFAULTCRECDEF(cServiceType, _sServiceTypes)

qlonglong cServiceType::insertNew(QSqlQuery& __q, const QString& __name, const QString& __note, bool _replace, eEx __ex)
{
    cServiceType    o;
    o.setName(__name);
    o.setName(_sServiceTypeNote, __note);
    if (_replace) {
        if (o.replace(__q, __ex)) {
            return o.getId();
        }
    }
    else {
        if (o.insert(__q, __ex)) {
            return o.getId();
        }
    }
    return NULL_ID;
}

/* -------------------------------------------------------------- */
DEFAULTCRECDEF(cAlarmMsg, _sAlarmMessages)

void cAlarmMsg::replace(QSqlQuery& __q, qlonglong __stid, const QString& __stat, const QString& __shortMsg, const QString& __msg)
{
    cAlarmMsg o;
    o.setId(_sServiceTypeId, __stid);
    o.setName(_sStatus, __stat);
    bool e = o.fetchQuery(__q, false, QBitArray(), tIntVector(), 0, 0, _sTextId);   // Read text_id if exists record
    o.setText(LTX_MESSAGE, __msg);
    o.setText(LTX_SHORT_MSG, __shortMsg);
    if (e) {
        // The record contains only a text link outside the primary key.
        o.set(_sTextId, __q.value(0));   // get text_id
    }
    else {
        o.insert(__q);
    }
    o.saveText(__q);
}

void cAlarmMsg::replaces(QSqlQuery& __q, qlonglong __stid, const QStringList& __stats, const QString& __shortMsg, const QString& __msg)
{
    foreach (QString st, __stats) {
        replace(__q, __stid, st, __shortMsg, __msg);
    }
}

/* -------------------------------------------------------------- */

cService::cService() : cRecord()
{
    _pFeatures = nullptr;
    _set(cService::descr());
}

cService::cService(const cService& __o) : cRecord()
{
    _fields = __o._fields;      // Nincs öröklés !
    _stat    = __o._stat;
    _pFeatures = __o._pFeatures == nullptr ? nullptr : new cFeatures(*__o._pFeatures);
}

cService::~cService()
{
    pDelete(_pFeatures);
}

int             cService::_ixFeatures   = NULL_IX;

const cRecStaticDescr&  cService::descr() const
{
    if (initPDescr<cService>(_sServices)) {
        _ixFeatures = _descr_cService().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}
void cService::toEnd()
{
    cService::toEnd(_ixFeatures);
    cRecord::toEnd();
}

bool cService::toEnd(int i)
{
    if (i == _ixFeatures) {
        if (_pFeatures != nullptr) {
            delete _pFeatures;
            _pFeatures = nullptr;
            return true;
        }
    }
    return false;
}

void cService::clearToEnd()
{
}
CRECDEF(cService)
RECACHEDEF(cService, service)

/* ----------------------------------------------------------------- */

int noalarmtype(const QString& _n, enum eEx __ex)
{
    if (0 == _n.compare(_sOff,    Qt::CaseInsensitive)) return NAT_OFF;
    if (0 == _n.compare(_sOn,     Qt::CaseInsensitive)) return NAT_ON;
    if (0 == _n.compare(_sTo,     Qt::CaseInsensitive)) return NAT_TO;
    if (0 == _n.compare(_sFrom,   Qt::CaseInsensitive)) return NAT_FROM;
    if (0 == _n.compare(_sFromTo, Qt::CaseInsensitive)) return NAT_FROM_TO;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, 0, _n);
    return ENUM_INVALID;
}

const QString& noalarmtype(int _e, enum eEx __ex)
{
    switch(_e) {
    case NAT_OFF:       return _sOff;
    case NAT_ON:        return _sOn;
    case NAT_TO:        return _sTo;
    case NAT_FROM:      return _sFrom;
    case NAT_FROM_TO:   return _sFromTo;
    default:
        if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _e);
    }
    return _sNul;
}


cHostService::cHostService() : cRecord()
{
    _pFeatures = nullptr;
    _set(cHostService::descr());
}

cHostService::cHostService(const cHostService& __o) : cRecord()
{
    (*this) = __o;
}

cHostService::~cHostService()
{
    pDelete(_pFeatures);
}

cHostService::cHostService(QSqlQuery& q, const QString& __h, const QString& __p, const QString& __s, const QString& __n)
{
    _pFeatures = nullptr;
    _set(cHostService::descr());
    setId(_sNodeId,    cNode().getIdByName   (q, __h));
    setId(_sServiceId, cService().getIdByName(q, __s));
    if (!__p.isEmpty()) setPort(q, __p);
    if (!__n.isEmpty()) setName(_sHostServiceNote, __n);
}


cHostService& cHostService::operator=(const cHostService& __o)
{
    __cp(__o);
    _fields = __o._fields;              // Nincs öröklés !
    _pFeatures = __o._pFeatures == nullptr ? nullptr : new cFeatures(*__o._pFeatures);
    return *this;
}

int cHostService::_ixFeatures  = NULL_IX;
const cRecStaticDescr&  cHostService::descr() const
{
    if (initPDescr<cHostService>(_sHostServices)) {
        CHKENUM(_sNoalarmFlag, noalarmtype)
        // Ez egy SWITCH tábla kell legyen ...
        if (_descr_cHostService().tableType() != TT_SWITCH_TABLE) EXCEPTION(EPROGFAIL, _descr_cHostService().tableType(), _descr_cHostService().toString());
        _ixFeatures = _descr_cHostService().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}

void cHostService::toEnd()
{
    toEnd(_ixFeatures);
    cRecord::toEnd();
}

bool cHostService::toEnd(int i)
{
    if (i == _ixFeatures) {
        if (_pFeatures != nullptr) {
            delete _pFeatures;
            _pFeatures = nullptr;
            return true;
        }
    }
    return false;
}

void cHostService::clearToEnd()
{
    ;
}

CRECDEF(cHostService)

int cHostService::replace(QSqlQuery &__q, eEx __ex)
{
    (void)__q;
    if (__ex) EXCEPTION(ENOTSUPP);
    return R_ERROR;
}

int cHostService::setStateMaxTry = NULL_IX;

cHostService&  cHostService::setState(QSqlQuery& __q, const QString& __st, const QString& __note, qlonglong __did, bool _resetIfDeleted)
{
    tIntVector readBackFieldIxs;    // Visszaolvasandó (változó) mezők indexei
    QString readBackFields;
    if (setStateMaxTry < 0) {
        setStateMaxTry = int(cSysParam::getIntegerSysParam(__q, "set_service_state_max_try", 5));
        // Visszaolvasandó mezők indexek feltöltése:
        readBackFieldIxs
                << toIndex(_sDisabled)  // Ha esetleg letiltották
                << toIndex(_sHostServiceState) << toIndex(_sHardState)  // Új statusok (soft_state = __st)
                << toIndex(_sCheckAttempts) << toIndex(_sLastChanged) << toIndex(_sLastTouched); // Csak a log. miatt
        foreach (int ix, readBackFieldIxs) {
            readBackFields += columnNameQ(ix) + _sCommaSp;
        }
        readBackFields.chop(_sCommaSp.size());
    }
    static const QString _sHostServiceId2Name = "host_service_id2name";
    if (sFulName.isEmpty()) {
        sFulName = execSqlTextFunction(__q, _sHostServiceId2Name, getId());
        if (sFulName.isEmpty()) {   // Törölték?
            APPMEMO(__q, tr("Host service record not found : %1").arg(identifying(false)), RS_CRITICAL);
            setBool(_sDeleted, true);
            if (_resetIfDeleted) lanView::getInstance()->reSet();
            return *this;
        }
    }
    _DBGFN() << sFulName << VDEB(__st) << VDEB(__note) << endl;
    QVariant did;
    QString sql;
    if (__did != NULL_ID) {
        did = __did;
        sql = QString("SELECT %1 FROM %2(?,?,?,?)");
    }
    else {
        sql = QString("SELECT %1 FROM %2(?,?,?)");
    }
    sql = sql.arg(readBackFields).arg(_sSetServiceStat);
    bool tf = trFlag(TS_NULL) == TS_TRUE;
    QString sTrFulName = toSqlName(sFulName);
    int cnt = 0;
    while (true) {
        if (tf) sqlBegin(__q, sTrFulName);
        int r = _execSql(__q, sql, getId(), __st, __note, did);
        if (r == 0) {   // Nincs adat ?!
            if (tf) sqlRollback(__q, sTrFulName, EX_IGNORE);
            EXCEPTION(EENODATA, 0, tr("SQL függvény: %1(%2,%3,%4,%5)")
                      .arg(_sSetServiceStat).arg(getId()).arg(__st, __note).arg(__did)
                      );
        }
        if (r < 0) {    // prepare or exec error
            QSqlError le = __q.lastError();
            if (tf) sqlRollback(__q, sTrFulName);
            QString s = le.databaseText().split('\n').first();  // első sor
            cnt++;
            // deadlock ?
            if (s.contains("deadlock", Qt::CaseInsensitive) || cnt <= setStateMaxTry) {
                DERR() << tr("Set stat %1 to %2 SQL PREPARE ERROR #%3 try #%4\n").arg(__st, sFulName).arg(le.nativeErrorCode()).arg(cnt)
                    << tr("driverText   : ") << le.driverText() << "\n"
                    << tr("databaseText : ") << le.databaseText() << endl;
                QThread::msleep(200);
                continue;   // retrying
            }
            LV2_SQLERR(le, EQUERY);    // no return
        }
        else if (r == 1) {
            setName(_sSoftState, __st); // Ennyinek kell lennie
            for (int i = 0; i < readBackFieldIxs.size(); ++i) {
                setq(readBackFieldIxs.at(i), __q, i);
            }
            if (tf) sqlCommit(__q, sTrFulName);
            _DBGFNL() << toString() << endl;
            if (getBool(_sDisabled) && _resetIfDeleted) lanView::getInstance()->reSet();
            break;
        }
        else {
            EXCEPTION(EPROGFAIL);
        }
    }
    return *this;
}

cHostService& cHostService::clearState(QSqlQuery& __q)
{
    QBitArray setMask = clearStateFields(TS_FALSE);
    _toReadBack = RB_NO_ONCE;
    update(__q, false, setMask, primaryKey());
    return *this;
}

QBitArray cHostService::clearStateFields(eTristate flag)
{
    QBitArray setMask;
    setMask = mask(_sActAlarmLogId, _sLastAlarmLogId, _sStateMsg);
    clear(setMask);
    setName(_sHostServiceState, _sUnknown);
    setName(_sHardState,        _sUnknown);
    setName(_sSoftState,        _sUnknown);
    setMask |= mask(_sHostServiceState, _sHardState, _sSoftState);
    setName(_sLastChanged, _sNOW);
    setName(_sLastTouched ,_sNOW);
    setId(_sCheckAttempts, 0);
    setMask |= mask(_sLastChanged, _sLastTouched, _sCheckAttempts);
    if (flag != TS_NULL) {
        static int ix = toIndex(_sFlag);
        setBool(ix, flag == TS_TRUE);
        setMask.setBit(ix);
    }
    return setMask;
}

int cHostService::fetchByNames(QSqlQuery& q, const QString &__hn, const QString& __sn, eEx __ex)
{
    set();
    static const QString sql =
            "SELECT host_services.* "
              "FROM host_services "
              "JOIN nodes USING(node_id) "
              "JOIN services USING(service_id) "
              "WHERE node_name LIKE ? "
                "AND service_name LIKE ?";
    int n = -1;
    int r = -1;
    if (execSql(q, sql, __hn, __sn)) {
        set(q);
        n = q.size();
        if (n == 1 || __ex < EX_WARNING) r = n;
    }
    else {
        if (__ex == EX_IGNORE) r = 0;
    }
    if (r < 0) {
        QString e = tr("HostService pattern: \"%1\".\"%2\"").arg(__hn).arg(__sn);
        switch (n) {
        case -1:    SQLQUERYERR(q);             LV2_FALLTHROUGH;
        case  0:    EXCEPTION(EFOUND, 0, e);    LV2_FALLTHROUGH;
        default:    EXCEPTION(AMBIGUOUS, n, e); LV2_FALLTHROUGH;
        }
    }
    return n;
}

int cHostService::fetchByNames(QSqlQuery& q, const QString& __hn, const QString& __sn, const QString& __pn, eEx __ex)
{
    set();
    QVariantList bind;
    QString sql =
            "SELECT hs.* "
              "FROM host_services AS hs "
              "JOIN nodes             USING(node_id) "
              "JOIN services AS s     USING(service_id) ";
    // host, service
    QString where =
            "WHERE node_name    LIKE ? "
              "AND service_name LIKE ? ";
    bind << __hn << __sn;
    // port
    if (__pn.isEmpty()) {
        where += "AND hs.port_id IS NULL ";
    }
    else {
        sql   += "LEFT OUTER JOIN nports USING(port_id) ";
        where += "AND port_name LIKE ? ";
        bind  += __pn;
    }
    // QUERY
    sql += where;
    int n = -1;
    int r = -1;
    if (execSql(q, sql, bind)) {
        set(q);
        n = q.size();
        if (n == 1 || __ex < EX_WARNING) r = n;
    }
    else {
        if (__ex == EX_IGNORE) r = 0;
    }
    if (r < 0) {
        QString e = tr("HostService : %1:%2.%3").arg(__hn).arg(__pn).arg(__sn);
        switch (n) {
        case -1:    SQLQUERYERR(q);             LV2_FALLTHROUGH
        case  0:    EXCEPTION(EFOUND, 0, e);    LV2_FALLTHROUGH
        default:    EXCEPTION(AMBIGUOUS, n, e); LV2_FALLTHROUGH
        }
    }
    return n;
}

int cHostService::fetchByNames(QSqlQuery& q, const QString &__hn, const QString& __sn, const QString& __pn, const QString& __pron, const QString& __prin, eEx __ex)
{
    set();
    QVariantList bind;
    QString sql =
            "SELECT hs.* "
              "FROM host_services AS hs "
              "JOIN nodes             USING(node_id) "
              "JOIN services AS s     USING(service_id) ";
    // host, service
    QString where = "WHERE node_name    LIKE ? "
                    "AND s.service_name LIKE ? ";
    bind << __hn << __sn;
    // port
    if (__pn.isEmpty()) {
        where += "AND    hs.port_id IS NULL ";
    }
    else {
        sql   += "LEFT OUTER JOIN nports USING(port_id) ";
        where += "AND port_name  LIKE ? ";
        bind  += __pn;
    }
    // protocol service
    if (__pron.isEmpty()) {
        where += QString("AND proto_service_id = %1 ").arg(NIL_SERVICE_ID);
    }
    else {
        sql   += "JOIN services AS prot  ON prot.service_id = hs.proto_service_id ";
        where += "AND prot.service_name LIKE ? ";
        bind  << __pron;
    }
    // prime service
    if (__prin.isEmpty()) {
        where += QString("AND prime_service_id = %1 ").arg(NIL_SERVICE_ID);
    }
    else {
        sql   += "JOIN services AS prim  ON prim.service_id = hs.prime_service_id ";
        where += "AND prim.service_name LIKE ? ";
        bind  << __prin;
    }
    sql += where;
    int n = -1;
    int r = -1;
    if (execSql(q, sql, bind)) {
        set(q);
        n = q.size();
        if (n == 1 || __ex < EX_WARNING) r = n;
    }
    else {
        if (__ex == EX_IGNORE) r = 0;
    }
    if (r < 0) {
        QString e = tr("HostService : %1:%2.%3(%4:%5)").arg(__hn).arg(__pn).arg(__sn).arg(__pron).arg(__prin);
        switch (n) {
        case -1:    SQLQUERYERR(q);             LV2_FALLTHROUGH
        case  0:    EXCEPTION(EFOUND, 0, e);    LV2_FALLTHROUGH
        default:    EXCEPTION(AMBIGUOUS, n, e); LV2_FALLTHROUGH
        }
    }
    return n;
}

bool cHostService::fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, eEx __ex)
{
    set();
    (*this)[_sNodeId]    = __hid;
    (*this)[_sServiceId] = __sid;
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (r != 1) {
        QString e = tr("HostService : #%1(%2):#%3(%4)")
                .arg(__hid).arg(cNode().   getNameById(__hid, EX_IGNORE))
                .arg(__sid).arg(cService().getNameById(__sid, EX_IGNORE));
        if (__ex) {
            if (r == 0) {
                EXCEPTION(EFOUND,    r, e);
            }
            else {
                EXCEPTION(AMBIGUOUS, r, e);
            }
        }
        return false;
    }
    return true;
}

bool cHostService::fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, qlonglong __pid, eEx __ex)
{
    set();
    (*this)[_sNodeId]    = __hid;
    (*this)[_sServiceId] = __sid;
    (*this)[_sPortId]    = __pid;
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (r != 1) {
        QSqlQuery qq = getQuery();
        QString e = tr("HostService : %1(%2):%3(%4).%5(%6)")
                .arg(__hid).arg(cNode().   getNameById(qq, __hid, EX_IGNORE))
                .arg(__pid).arg(cNPort().  getFullNameById(qq, __pid))
                .arg(__sid).arg(cService().getNameById(qq, __sid, EX_IGNORE));
        if (__ex) {
            if (r == 0) {
                EXCEPTION(EFOUND,    r, e);
            }
            else {
                EXCEPTION(AMBIGUOUS, r, e);
            }
        }
        return false;
    }
    return true;
}

int cHostService::delByNames(QSqlQuery& q, const QString& __nn, const QString& __sn, bool __npat, bool __spat)
{
    QString sql =
            "SELECT host_service_id FROM host_services JOIN nodes USING(node_id) JOIN services USING(service_id)"
            " WHERE node_name %1 ? AND service_name %2 ?";
    sql = sql.arg(__npat ? "LIKE" : "=");
    sql = sql.arg(__spat ? "LIKE" : "=");
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __nn);
    q.bindValue(1, __sn);
    _EXECSQL(q);
    QList<qlonglong>    idl;
    if (q.first()) {
        do {
            idl << variantToId(q.value(0));
        } while (q.next());
        foreach (qlonglong id, idl) {
            setId(id);
            if (!remove(q, false, primaryKey())) DERR() << "Nem sikerült törölni a host_services rekordot, ID = " << id << endl;
        }
    }
    return idl.size();
}

bool cHostService::fetchSelf(QSqlQuery& q, cNode& __h, const cService &__s, eEx __ex)
{
    if (!__h.fetchSelf(q, __ex)) return false;
    if (!fetchByIds(q, __h.getId(), __s.getId(), __ex)) return false;
    return true;
}

QVariant cHostService::value(QSqlQuery& q, const cService& s, const QString& f)
{
    if (isNull(f)) {
        if (s.isNull(f)) {
            QString e = toString() + QChar('/') + s.toString() + QChar('/') + f;
            q.clear();
            EXCEPTION(EFOUND, 0, e);
        }
        (*this)[f] = s[f];
        update(q, true, mask(f));
    }
    return get(f);
}

QString cHostService::fullName(QSqlQuery& q, eEx __ex) const
{
    return fullName(q, getId(), __ex);
}

QString cHostService::fullName(QSqlQuery& q, qlonglong __id, eEx __ex)
{
    QString r;
    if (execSqlFunction(q, "host_service_id2name", __id)) {
        r = q.value(0).toString();
    }
    if (r.isEmpty()) {
        if (__ex != EX_IGNORE) EXCEPTION(EFOUND, __id, tr("Ismeretlen host_service_id"));
        if (__id == NULL_ID) {
            r = cColStaticDescr::rNul;
        }
        else {
            r = tr("Invalid or deleted #%1").arg(__id);
        }
    }
    return r;
}

/* ----------------------------------------------------------------- */
int userEventType(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sNotice,      Qt::CaseInsensitive)) return UE_NOTICE;
    if (0 == _n.compare(_sView,        Qt::CaseInsensitive)) return UE_VIEW;
    if (0 == _n.compare(_sAcknowledge, Qt::CaseInsensitive)) return UE_ACKNOWLEDGE;
    if (0 == _n.compare(_sSendmessage, Qt::CaseInsensitive)) return UE_SENDMESSAGE;
    if (0 == _n.compare(_sSendmail,    Qt::CaseInsensitive)) return UE_SENDMAIL;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, 0, _n);
    return ENUM_INVALID;
}

const QString& userEventType(int _i, eEx __ex)
{
    switch(_i) {
    case UE_NOTICE:         return _sNotice;
    case UE_VIEW:           return _sView;
    case UE_ACKNOWLEDGE:    return _sAcknowledge;
    case UE_SENDMESSAGE:    return _sSendmessage;
    case UE_SENDMAIL:       return _sSendmail;
    default:
        if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    }
    return _sNul;
}

int userEventState(const QString& _n, eEx __ex)
{
    if (0 == _n.compare(_sNecessary,   Qt::CaseInsensitive)) return UE_NECESSARY;
    if (0 == _n.compare(_sHappened,    Qt::CaseInsensitive)) return UE_HAPPENED;
    if (0 == _n.compare(_sDropped,     Qt::CaseInsensitive)) return UE_DROPPED;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, 0, _n);
    return ENUM_INVALID;
}

const QString& userEventState(int _i, eEx __ex)
{
    switch(_i) {
    case UE_NECESSARY:      return _sNecessary;
    case UE_HAPPENED:       return _sHappened;
    case UE_DROPPED:        return _sDropped;
    default:
        if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, _i);
    }
    return _sNul;
}

CRECCNTR(cUserEvent)
CRECDEFD(cUserEvent)
const cRecStaticDescr& cUserEvent::descr() const
{
    if (initPDescr<cUserEvent>(_sUserEvents)) {
        CHKENUM(_sEventType,  userEventType);
        CHKENUM(_sEventState, userEventState);
    }
    return *_pRecordDescr;
}

qlonglong cUserEvent::insert(QSqlQuery &q, qlonglong _uid, qlonglong _aid, eUserEventType _et)
{
    cUserEvent ue;
    ue.setId(_sUserId,  _uid);
    ue.setId(_sAlarmId, _aid);
    ue.setId(_sEventType,_et);
    // ue.setId(_sEventState, UE_NECESSARY);    // DEFAULT
    ue.cRecord::insert(q);
    return ue.getId();
}

qlonglong cUserEvent::insertHappened(QSqlQuery &q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString& _m)
{
    cUserEvent ue;
    ue.setId(_sUserId,  _uid);
    ue.setId(_sAlarmId, _aid);
    ue.setId(_sEventType,_et);
    ue.setId(_sEventState, UE_HAPPENED);
    ue.setName(_sUserEventNote, _m);
    ue.cRecord::insert(q);
    return ue.getId();
}

bool cUserEvent::happened(QSqlQuery& q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString &_m)
{
    QString et = userEventType(_et);
    if (_m.isEmpty()) {
        static const QString sql =
                "UPDATE user_events SET event_state = 'happened'"
                " WHERE user_id = ? AND alarm_id = ? AND event_type = ?";
        execSql(q, sql, _uid, _aid, et);
    }
    else {
        static const QString sql =
                "UPDATE user_events SET event_state = 'happened', user_event_note = ?"
                " WHERE user_id = ? AND alarm_id = ? AND event_type = ?";
        execSql(q, sql, _m, _uid, _aid, et);
    }
    int n = q.numRowsAffected();
    if (n > 1) {
        EXCEPTION(EDATA, n, tr("Is not unique : alarm #%1, user #%2, %3").arg(_uid).arg(_aid).arg(et));
    }
    return n == 1;
}

bool cUserEvent::dropped(QSqlQuery& q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString &_m)
{
    QString et = userEventType(_et);
    if (_m.isEmpty()) {
        static const QString sql =
                "UPDATE user_events SET event_state = 'dropped'"
                " WHERE user_id = ? AND alarm_id = ? AND event_type = ?";
        execSql(q, sql, _uid, _aid, userEventType(_et));
    }
    else {
        static const QString sql =
                "UPDATE user_events SET event_state = 'dropped', user_event_note = ?"
                " WHERE user_id = ? AND alarm_id = ? AND event_type = ?";
        execSql(q, sql, _m, _uid, _aid, userEventType(_et));
    }
    int n = q.numRowsAffected();
    if (n > 1) {
        EXCEPTION(EDATA, n, tr("Is not unique : alarm #%1, user #%2, %3").arg(_uid).arg(_aid).arg(et));
    }
    return n == 1;
}


/* ----------------------------------------------------------------- */
DEFAULTCRECDEF(cAlarm, _sAlarms)

QString cAlarm::htmlText(QSqlQuery& q, qlonglong _id)
{
    static const QString _sBr = "<br>";

    bool isTicket = false;
    cAlarm a;
    cRecord *pTargetRec = &a;
    a.setById(q, _id);
    QString text;   // HTML text

    if (TICKET_SERVICE_ID == a.getId(_sHostServiceId)
     && !a.isNull(_sSuperiorAlarmId)) {     // Riasztáshoz csatolt ticket
        // Az eredeti riasztás a ticket-hez
        isTicket = true;
        text = tr("Hiba jegy") + " :<br>";
        qlonglong id = a.getId(_sSuperiorAlarmId);  // Amire a ticket készült
        pTargetRec = a.newObj();                    // Az eredeti riasztás, és jelezzük, hogy ticket
        pTargetRec->setById(q, id);
    }
    cHostService hs; hs.   setById(q, pTargetRec->getId(_sHostServiceId));
    cNode  node;     node. setById(q, hs.         getId(_sNodeId));
    cPlace place;    place.setById(q, node.       getId(_sPlaceId));
    QString m;
    const cService *pSrv = cService::service(q,hs.getId(_sServiceId), EX_IGNORE);
    text += _sBr + tr("Riasztási állpot kezdete") + " : <b>" + pTargetRec->view(q, _sBeginTime) + "</b>";
    text += _sBr + tr("A hállózati elem helye")   + " : <b>" + place.getName() + "</b>, <i>" + place.getNote() + "</i>";
    text += _sBr + tr("A hállózati elem neve")    + " : <b>" + node.getName()  + "</b>, <i>" + node.getNote()  + "</i>";
    text += _sBr + tr("Szolgáltatás neve")        + " : <b>" + pSrv->getName() + "</b>, <i>" + pSrv->getNote() + "</i>";
    m = execSqlTextFunction(q, "alarm_message", hs.getId(), a.get(_sMaxStatus));
    text += _sBr + tr("Riasztás oka")          + " : <b><i>" + m + "</i></b>";
    m = pTargetRec->getName(_sEventNote);
    text += _sBr + tr("Csatolt üzenet")        + " : <b><i>" + _sBr
            + toHtml(m, true) + "</i></b>";
    static const QString sql =
            "SELECT s.service_var_id, s.service_var_name, s.service_var_note, s.service_var_type_id, s.host_service_id,"
                    " a.service_var_value, a.var_state, NULL AS last_time, s.features, s.deleted, a.raw_value,"
                    " s.delegate_service_state, a.state_msg, s.delegate_port_state, s.disabled, s.flag, s.rarefaction"
            " FROM service_vars AS s"
            " JOIN alarm_service_vars AS a USING(service_var_id)"
            " WHERE s.host_service_id = ? AND a.alarm_id = ?"
            " ORDER BY service_var_name ASC";
    if (execSql(q, sql, hs.getId(), _id)) {
        tRecordList<cServiceVar> vars;
        vars.set(q);
        static cTableShape *pShape = nullptr;
        if (pShape == nullptr) {
            pShape = new cTableShape;
            pShape->setByName(q, _sServiceVars);
        }
        text += _sBr + tr("Változók : ") + _sBr + list2html(q, vars, *pShape);
    }
    if (a.isNull(_sEndTime)) {
        text += _sBr + tr("A riasztási állapot az üzenetküdéskor még aktív volt.");
    }
    else {
        text += _sBr + tr("A riasztási állapot az üzenetküdéskor már nem volt aktív.");
        text += _sBr + tr("A riasztási állapot vége : ") + "<b>" + pTargetRec->view(q, _sEndTime) + "</i></b>";
    }

    if (isTicket) delete pTargetRec;
    return text;
}

void cAlarm::ticket(QSqlQuery& _q,eNotifSwitch _st, const QString& _msg, qlonglong _did, qlonglong _said, eNotifSwitch _fst, eNotifSwitch _lst)
{
    QVariantList args;
    QString sarg = "?, ?, ";
    args << notifSwitch(_st);           // stat (last_stat)
    args << _msg;                       // stat message (event_note)
    if (_did != NULL_ID) {              // daemon ID
        args << _did;
        sarg += "?, ";
    }
    else {
        sarg += "NULL, ";
    }
    if (_said != NULL_ID) {             // sup.alarm ID
        args << _said;
        sarg += "?, ";
    }
    else {
        sarg += "NULL, ";
    }
    if (_fst == RS_INVALID) {
        args << args.first();
    }
    else {
        args << notifSwitch(_fst);
    }
    if (_lst == RS_INVALID) {
        args << args.first();
    }
    else {
        args << notifSwitch(_lst);
    }
    sarg += " ?, ?";
    QString sql = "SELECT ticket_alarm(" + sarg + ")";
    execSql(_q, sql, args);
}

/* ----------------------------------------------------------------- */
DEFAULTCRECDEF(cOui, _sOuis)

int cOui::replace(QSqlQuery& __q, eEx __ex)
{
    (void)__ex;
    if (!execSqlFunction(__q, "replace_oui", toSql(_sOui), toSql(_sOuiName), toSql(_sOuiNote))) {
        EXCEPTION(EPROGFAIL);
    }
    QString r = __q.value(0).toString();
    return eReasons(reasons(r));
}

bool cOui::fetchByMac(QSqlQuery& q, const cMac& __mac)
{
    cMac mac(__mac.toLongLong() & 0x0ffffff000000);
    clear();
    setMac(_sOui, mac);
    return 1 == completion(q);
}

/* ----------------------------------------------------------------- */
CRECCNTR(cMacTab);

int cMacTab::_ixPortId    = NULL_IX;
int cMacTab::_ixHwAddress = NULL_IX;
int cMacTab::_ixSetType   = NULL_IX;
int cMacTab::_ixMacTabState=NULL_IX;


const cRecStaticDescr& cMacTab::descr() const
{
    if (initPDescr<cMacTab>(_sMacTab)) {
        _ixHwAddress = _descr_cMacTab().toIndex(_sHwAddress);
        _ixPortId    = _descr_cMacTab().toIndex(_sPortId);
        _ixSetType   = _descr_cMacTab().toIndex(_sSetType);
        _ixMacTabState=_descr_cMacTab().toIndex(_sMacTabState);
    }
    return *_pRecordDescr;
}
CRECDEFD(cMacTab)

int cMacTab::replace(QSqlQuery& __q, eEx __ex)
{
    (void)__ex;
    QString sql = "SELECT replace_mactab(?,?";
    if (!isNull(_ixSetType))     sql += _sCommaQ;
    if (!isNull(_ixMacTabState)) sql += _sCommaQ;
    sql += QChar(')');
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    bind(_ixPortId,    __q, 0);
    bind(_ixHwAddress, __q, 1);
    int i = 2;
    if (!isNull(_ixSetType))     bind(_ixSetType,     __q, i++);
    if (!isNull(_ixMacTabState)) bind(_ixMacTabState, __q, i);
    _EXECSQL(__q);
    __q.first();
    enum eReasons r = eReasons(reasons(__q.value(0).toString(), EX_IGNORE));
    return r;
}

int cMacTab::refresStats(QSqlQuery& __q)
{
    bool r = execSqlFunction(__q, QString("refresh_mactab"));
    int  n = __q.value(0).toInt();
    PDEB(VERBOSE) << "cMacTab::refresStats() : " << DBOOL(r) << VDEB(n) << endl;
    return n;
}

/* ----------------------------------------------------------------- */
CRECCNTR(cArp);

int cArp::_ixIpAddress = NULL_IX;
int cArp::_ixHwAddress = NULL_IX;
int cArp::_ixSetType   = NULL_IX;
int cArp::_ixHostServiceId = NULL_IX;
const cRecStaticDescr&  cArp::descr() const
{
    if (initPDescr<cArp>(_sArps)) {
        STFIELDIX(cArp, IpAddress);
        STFIELDIX(cArp, HwAddress);
        STFIELDIX(cArp, SetType);
        STFIELDIX(cArp, HostServiceId);
    }
    return *_pRecordDescr;
}
CRECDEFD(cArp)

cArp::operator QHostAddress() const
{
    if (isNull(_ixIpAddress)) return QHostAddress();
    const netAddress na = get(_ixIpAddress).value<netAddress>();
    return na.addr();
}
cArp::operator cMac() const
{
    if (isNull(_ixHwAddress)) return cMac();
    return get(_ixHwAddress).value<cMac>();
}

int cArp::replace(QSqlQuery& __q, eEx __ex)
{
    (void)__ex;
    QString sql = "SELECT replace_arp(?,?";
    if (!isNull(_ixSetType))       sql += _sCommaQ;
    if (!isNull(_ixHostServiceId)) sql += _sCommaQ;
    sql += QChar(')');
    if (!__q.prepare(sql)) SQLPREPERR(__q, sql);
    bind(_ixIpAddress, __q, 0);
    bind(_ixHwAddress, __q, 1);
    int i = 2;
    if (!isNull(_ixSetType))       bind(_ixSetType,       __q, i++);
    if (!isNull(_ixHostServiceId)) bind(_ixHostServiceId, __q, i);
    _EXECSQL(__q);
    __q.first();
    eReasons r = eReasons(reasons(__q.value(0).toString(), EX_IGNORE));
    return r;
}

#ifdef SNMP_IS_EXISTS
int cArp::replaces(QSqlQuery& __q, const cArpTable& __t, int setType, qlonglong hsid)
{
    int r = 0;
    cArp arp;
    for (cArpTable::const_iterator i = __t.constBegin(); i != __t.constEnd(); ++i) {
        arp.clear();
        arp = i.key();
        arp = i.value();
        arp.setId(_ixSetType, setType);
        arp.setId(_ixHostServiceId, hsid);
        if (R_INSERT == arp.replace(__q)) ++r;
    }
    return r;
}
#else  // SNMP_IS_EXISTS
int cArp::replaces(QSqlQuery&, const cArpTable&, int, qlonglong)
{
    EXCEPTION(ENOTSUPP);
    return 0;
}
#endif // SNMP_IS_EXISTS

int cArp::mac2arps(QSqlQuery& __q, const cMac& __m, tRecordList<cArp>& arps)
{
    int r = 0;
    cArp arp;
    arp = __m;
    if (arp.fetch(__q, false, arp.mask(_ixHwAddress))) do {
        arps << arp;
        ++r;
    } while (arp.next(__q));
    return r;
}

QList<QHostAddress> cArp::mac2ips(QSqlQuery& __q, const cMac& __m)
{
    QList<QHostAddress> r;
    cArp arp;
    arp = __m;
    if (arp.fetch(__q, false, arp.mask(_ixHwAddress))) {
        r << arp;
        while (arp.next(__q)) r << arp;
    }
    return r;
}

QHostAddress cArp::mac2ip(QSqlQuery& __q, const cMac& __m, eEx __ex)
{
    QList<QHostAddress> al = mac2ips(__q, __m);
    int n = al.size();
    if (n == 1) return al.first();
    QString em = __m.toString();
    if (n > 1) {
        em = tr("A %1 MAC alapján az IP nem egyértelmű.").arg(em);
        if (__ex != EX_IGNORE) EXCEPTION(AMBIGUOUS, n, em);
    }
    else {
        em = tr("A %1 MAC-hez nincs IP cím.").arg(em);
        if (__ex >= EX_WARNING) EXCEPTION(EFOUND,    n, em);
    }
    DERR() << em << endl;
    return QHostAddress();
}


cMac cArp::ip2mac(QSqlQuery& __q, const QHostAddress& __a, eEx __ex)
{
    QString em;
    if (__a.isNull()) {
        em = tr("MAC keresése érvénytelen IP címmel.");
        if (__ex != EX_IGNORE) EXCEPTION(EDATA, 0, em);
    }
    else {
        cArp arp;
        arp = __a;
        if (arp.fetch(__q, false, arp.mask(_ixIpAddress))) return arp;
        em = tr("A %1 IP címhez-hez nincs MAC.").arg(__a.toString());
        if (__ex >= EX_WARNING) EXCEPTION(EFOUND, 0, em);
    }
    DERR() << em << endl;
    return cMac();
}

int cArp::checkExpired(QSqlQuery& __q)
{
    execSqlFunction(__q, QString("refresh_arps"));
    return __q.value(0).toInt();
}

/* ----------------------------------------------------------------- */

DEFAULTCRECDEF(cDynAddrRange, _sDynAddrRanges)

eReasons cDynAddrRange::replace(QSqlQuery& q, const QHostAddress& b, const QHostAddress& e, qlonglong dhcpSrvId, bool exclude, qlonglong subnetId)
{
    QString sql = "SELECT replace_dyn_addr_range(?, ?";
    QVariantList vl;
    vl << b.toString() << e.toString();
    if (dhcpSrvId == NULL_ID) sql += _sCommaSp + _sNULL;
    else                    { sql += _sCommaQ; vl << dhcpSrvId; }
    sql += _sCommaSp + (exclude ? _sTrue : _sFalse);
    if (subnetId == NULL_ID)  sql += _sCommaSp + _sNULL;
    else                    { sql += _sCommaQ; vl << subnetId; }
    sql += ")";
    execSql(q, sql, vl);
    return eReasons(reasons(q.value(0).toString()));
}

int cDynAddrRange::isDynamic(QSqlQuery &q, const QHostAddress& a)
{
    int n = cSubNet().getByAddress(q, a);
    if (n == 0) return AT_EXTERNAL;
    static const QString sql =
            "SELECT COUNT(*) FROM dyn_addr_ranges "
                "WHERE (begin_address <= :ip AND end_address >= :ip AND NOT exclude )"
              "AND NOT (begin_address <= :ip AND end_address >= :ip AND exclude)";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(":ip", a.toString());
    _EXECSQL(q);
    if (!q.first()) EXCEPTION(EDBDATA);
    n = q.value(0).toInt();
    return n > 0 ? AT_DYNAMIC : AT_FIXIP;
}

/* ----------------------------------------------------------------- */
CRECCNTR(cImport)
CRECDEFD(cImport)
const cRecStaticDescr& cImport::descr() const
{
    if (initPDescr<cImport>(_sImports)) {
        CHKENUM(_sExecState, execState);
    }
    return *_pRecordDescr;
}

/* ---------------------------------------------------------------------------------------------------- */

CRECDEF(cQueryParser)

const QString& parseType(int e, eEx __ex)
{
    switch (e) {
    case PT_PREP:   return _sPrep;
    case PT_PARSE:  return _sParse;
    case PT_POST:   return _sPost;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, e);
    return _sNul;

}

int parseType(const QString& s, eEx __ex)
{
    if (0 == s.compare(_sPrep,  Qt::CaseInsensitive)) return PT_PREP;
    if (0 == s.compare(_sParse, Qt::CaseInsensitive)) return PT_PARSE;
    if (0 == s.compare(_sPost,  Qt::CaseInsensitive)) return PT_POST;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, s);
    return ENUM_INVALID;
}

const QString& regexpAttr(int e, eEx __ex)
{
    switch (e) {
    case RA_CASESENSITIVE:  return _sCasesensitive;
    case RA_EXACTMATCH:     return _sExactmatch;
    case RA_LOOP:           return _sLoop;
    }
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, e);
    return _sNul;
}

int regexpAttr(const QString& s, eEx __ex)
{
    if (0 == s.compare(_sCasesensitive, Qt::CaseInsensitive)) return RA_CASESENSITIVE;
    if (0 == s.compare(_sExactmatch,    Qt::CaseInsensitive)) return RA_EXACTMATCH;
    if (0 == s.compare(_sLoop,          Qt::CaseInsensitive)) return RA_LOOP;
    if (__ex != EX_IGNORE) EXCEPTION(EENUMVAL, -1, s);
    return ENUM_INVALID;
}

const cRecStaticDescr&  cQueryParser::descr() const
{
    if (initPDescr<cQueryParser>(_sQueryParsers)) {
        CHKENUM(_sParseType, parseType)
        CHKENUM(_sRegexpAttr, regexpAttr)
    }
    return *_pRecordDescr;
}

cQueryParser::cQueryParser() : cRecord()
{
    pPostCmd      = pPrepCmd = nullptr;
    pListCmd      = nullptr;
    pListRExp     = nullptr;
    pListReAttr   = nullptr;
    pInspector    = nullptr;
    pVarMap       = nullptr;
    pCommands     = nullptr;
    pParserThread = nullptr;
    _set(cQueryParser::descr());
}

cQueryParser::cQueryParser(const cQueryParser& __o) : cRecord()
{
    pPostCmd      = pPrepCmd = nullptr;
    pListCmd      = nullptr;
    pListRExp     = nullptr;
    pListReAttr   = nullptr;
    pInspector    = nullptr;
    pVarMap       = nullptr;
    pCommands     = nullptr;
    pParserThread = nullptr;
    _cp(__o);
}

cQueryParser::~cQueryParser()
{
    pDelete(pPostCmd);
    pDelete(pPrepCmd);
    pDelete(pListCmd);
    pDelete(pListRExp);
    pDelete(pListReAttr);
    // pDelete(pParserThread);
}

void cQueryParser::_insert(QSqlQuery& q, qlonglong _sid, const QString& _ty, qlonglong _ra, const QString& _re, const QString& _cmd, const QString& _not, qlonglong _seq)
{
    cQueryParser o;
    o.setId(  _sServiceId,         _sid);
    o.setName(_sParseType,         _ty);
    o.setBool(_sRegexpAttr,        _ra);
    o.setName(_sRegularExpression, _re);
    o.setName(_sImportExpression,  _cmd);
    o.setName(_sQueryParserNote,   _not);
    o.setId(  _sItemSequenceNumber,_seq);
    o.insert(q);
}

void cQueryParser::setInspector(cInspector *pInsp)
{
    pInspector = pInsp;
    pVarMap    = nullptr;
}

void cQueryParser::setMaps(tStringMap *pVM)
{
    pVarMap    = pVM;
    pInspector = nullptr;
}

int cQueryParser::prep(cError *& pe)
{
    pe = nullptr;
    if (pParserThread != nullptr) EXCEPTION(EPROGFAIL);
    QString cmd;
    if (pPrepCmd != nullptr) cmd = substitutions(*pPrepCmd, QStringList());
    if (pInspector != nullptr) {
        pParserThread = new cImportParseThread(cmd, this);
        return pParserThread->startParser(pe);
    }
    else if (pVarMap != nullptr) {
        pDelete(pCommands);
        pCommands = new QString(cmd);
    }
    else EXCEPTION(EPROGFAIL);
    return REASON_OK;
}

int cQueryParser::parse(QString src,  cError *&pe)
{
    pe = nullptr;
    // Sanity check
    if (pListCmd == nullptr || pListRExp == nullptr || pListReAttr == nullptr
     || pListCmd->size() != pListRExp->size() || pListCmd->size() != pListReAttr->size()) EXCEPTION(EPROGFAIL);
    int i, n = pListCmd->size();
    // Foreach all regular expressions
    for (i = 0; i < n; i++) {
        QRegExp   rexp = pListRExp->at(i);      // Regular expr.
        qlonglong reat = pListReAttr->at(i);    // Atrributum (set)
        // PDEB(VERBOSE) << src << " ~ " << rexp.pattern() << " - " << reat << endl;
        const QString& cmd = pListCmd->at(i);
        if (ENUM2SET(RA_EXACTMATCH) & reat) {
            if (rexp.exactMatch(src)) {
                return execute(pe, cmd, rexp.capturedTexts());
            }
        }
        else {
            int ix = rexp.indexIn(src);
            if (ix == -1) continue;
            if (ENUM2SET(RA_LOOP) & reat) {
                int r;
                do {
                    r = execute(pe, cmd, rexp.capturedTexts());
                    if (R_ERROR == r) return R_ERROR;
                } while ((ix = rexp.indexIn(src, ix)) > -1);
                return r;
            }
            else {
                return execute(pe, cmd, rexp.capturedTexts());
            }
        }
    }
    PDEB(VVERBOSE) << "Nincs illeszkedes : " << src << endl;
    return R_NOTFOUND;
}

int cQueryParser::post(cError *& pe)
{
    int r = REASON_OK;
    pe = nullptr;
    // Záró parancs elküldése, ha van
    if (pPostCmd != nullptr) r = execute(pe, *pPostCmd);
    if (pInspector != nullptr) {
        if (pParserThread == nullptr) EXCEPTION(EPROGFAIL);
        pParserThread->stopParser();
        // Töröljük a thread-et
        pDelete(pParserThread);
    }
    return r;
}

cQueryParser *cQueryParser::newChild(cInspector * _isp)
{
    if (pParserThread == nullptr) EXCEPTION(EPROGFAIL);
    QSqlQuery q = getQuery();
    cQueryParser *p = new cQueryParser;
    int r = p->load(q, _isp->serviceId(), false, false);
    if (R_NOTFOUND == r && nullptr != _isp->pPrimeService) r = p->load(q, _isp->primeServiceId(), false, false);
    if (R_NOTFOUND == r && nullptr != _isp->pProtoService) r = p->load(q, _isp->protoServiceId(), false, false);
    if (R_NOTFOUND == r) {
        if (pListCmd == nullptr || pListRExp == nullptr || pListReAttr) EXCEPTION(EDATA, 0, _isp->name());
        pDelete(p);
        return this;
    }
    p->pParserThread = pParserThread;
    p->setInspector(_isp);
    p->setParent(this);
    return p;
}

int cQueryParser::load(QSqlQuery& q, qlonglong _sid, bool force, bool thread)
{
    int ixServiceId = toIndex(_sServiceId);
    if (_sid != NULL_ID) {
        if (pListCmd != nullptr && !force && getId(ixServiceId) == _sid) return R_UNCHANGE;
        setId(ixServiceId, _sid);
    }
    pDelete(pPostCmd);
    pDelete(pPrepCmd);
    pDelete(pListCmd);
    pDelete(pListRExp);
    pDelete(pListReAttr);
    qlonglong id = getId(ixServiceId);

    clear().setId(ixServiceId, id).setName(_sParseType, _sParse);
    int n = completion(q);
    if (n == 0) return R_NOTFOUND;
    pListRExp = new QList<QRegExp>;
    pListReAttr = new QList<qlonglong>;
    pListCmd  = new QStringList;
    do {
        *pListCmd << getName(_sImportExpression);
        qlonglong reat = getId(_sRegexpAttr);
        if (reat == NULL_ID) reat = 0;
        enum Qt::CaseSensitivity cs = reat & ENUM2SET(RA_CASESENSITIVE) ? Qt::CaseSensitive : Qt::CaseInsensitive;
        QRegExp rexp(getName(_sRegularExpression),cs);
        if (!rexp.isValid()) EXCEPTION(EDATA, getId(), getName(_sRegularExpression));
        *pListRExp   << rexp;
        *pListReAttr << reat;
    } while(next(q));

    clear().setId(ixServiceId, id).setName(_sParseType, _sPrep);
    n = completion(q);
    if (n == 1 && thread) {
        pPrepCmd = new QString(getName(_sImportExpression));
    }
    else if (n != 0) {
        EXCEPTION(EDATA,n, _sPrep);
    }

    clear().setId(ixServiceId, id).setName(_sParseType, _sPost);
    n = completion(q);
    if (n == 1 && thread) {
        pPostCmd = new QString(getName(_sImportExpression));
    }
    else if (n != 0) {
        EXCEPTION(EDATA,n, _sPost);
    }
    return REASON_OK;
}

int cQueryParser::delByServiceName(QSqlQuery &q, const QString &__n, bool __pat)
{
    QString sql = QString(
            "DELETE FROM query_parsers WHERE service_id IN "
                "(SELECT service_id FROM services WHERE service_name %1 '%2')"
            ).arg(__pat ? "LIKE" : "=").arg(__n);
    EXECSQL(q, sql);
    int n = q.numRowsAffected();
    return  n;

}

QString cQueryParser::getParValue(const QString& name, const QStringList& args)
{
    bool ok;
    int i = int(name.toUInt(&ok));
    QString r;
    if (ok) {
        if (i >= args.size()) EXCEPTION(EDATA, i, args.join(_sCommaSp));
        r = args.at(i);
    }
    else {
        if (pInspector != nullptr) {
            QSqlQuery q = getQuery();
            r = pInspector->getParValue(q, name);
        }
        else if (pVarMap != nullptr) {
            r = (*pVarMap)[name];
        }
        else EXCEPTION(EPROGFAIL);
    }
    return r;
}

QString cQueryParser::substitutions(const QString& _cmd, const QStringList& args)
{
    QString r;
    QString::const_iterator i = _cmd.constBegin();
    while (i != _cmd.constEnd()) {
        char c = i->toLatin1();
        QChar qc = *i;
        ++i;
        if (c == '$') {
            if (i->toLatin1() == '$') { // $$ -> $
                ++i;
            }
            else {                      // $<name> -> <value>
                QString vname;
                vname = getParName(i, _cmd.constEnd());
                r += getParValue(vname, args);
                continue;
            }
        }
        r += qc;
    }
    return r;
}

int cQueryParser::execute(cError *&pe, const QString& _cmd, const QStringList& args)
{
    _DBGFN() << _cmd << "; " << args.join(_sCommaSp) << endl;
    QString cmd = substitutions(_cmd, args);
    int r = R_ERROR;
    if (pInspector != nullptr) {                            // Execute
        if (pParserThread == nullptr) {
            if (0 == importParseText(cmd)) return REASON_OK;
            pe = importGetLastError();
            r = R_ERROR;
        }
        else {
            r = pParserThread->push(cmd, pe);
        }
    }
    else if (pVarMap != nullptr && pCommands != nullptr) {  // Collecting rows to execute
        *pCommands += cmd + "\n";
        r = REASON_OK;
    }
    else {
        EXCEPTION(EPROGFAIL);
    }
    return r;
}

