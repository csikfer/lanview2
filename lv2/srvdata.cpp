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

qlonglong cServiceType::insertNew(QSqlQuery& __q, const QString& __name, const QString& __note, eEx __ex)
{
    cServiceType    o;
    o.setName(__name);
    o.setName(_sServiceTypeNote, __note);
    if (o.insert(__q, __ex)) {
        return o.getId();
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
    bool e = o.fetchQuery(__q);
    o.setName(_sMessage, __msg);
    o.setName(_sShortMsg, __shortMsg);
    if (e) o.update(__q, false);
    else   o.insert(__q);
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
    _pFeatures = NULL;
    _set(cService::descr());
}

cService::cService(const cService& __o) : cRecord()
{
    _fields = __o._fields;      // Nincs öröklés !
    _stat    = __o._stat;
    _pFeatures = __o._pFeatures == NULL ? NULL : new cFeatures(*__o._pFeatures);
}

cService::~cService()
{
    pDelete(_pFeatures);
}

int             cService::_ixFeatures   = NULL_IX;
const qlonglong cService::nilId         = -1;

const cRecStaticDescr&  cService::descr() const
{
    if (initPDescr<cService>(_sServices)) {
        _ixFeatures = _descr_cService().toIndex(_sFeatures);
        qlonglong id = _pRecordDescr->getIdByName(_sNil);
        if (nilId != id) EXCEPTION(EDATA, id, _sNil);
    }
    return *_pRecordDescr;
}
void cService::toEnd()
{
    cService::toEnd(_ixFeatures);
}

bool cService::toEnd(int i)
{
    if (i == _ixFeatures) {
        if (_pFeatures != NULL) {
            delete _pFeatures;
            _pFeatures = NULL;
            return true;
        }
    }
    return false;
}

void cService::clearToEnd()
{
    ;
}
CRECDEF(cService)
RECACHEDEF(cService, service)

/*
tRecordList<cService> cService::services;
const cService * cService::service(QSqlQuery& __q, const QString& __nm, eEx __ex)
{
    int i = services.indexOf(_descr_cService().nameIndex(), QVariant(__nm));
    if (i < 0) {
        cService *p = new cService();
        if (!p->fetchByName(__q, __nm)) {
            if (__ex) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("Ismeretlen szolgáltatás név : %1")).arg(__nm));
            delete p;
            return NULL;
        }
        services << p;
        return p;
    }
    return services.at(i);
}

const cService *cService::service(QSqlQuery &__q, qlonglong __id, eEx __ex)
{
    int i = services.indexOf(_descr_cService().idIndex(), QVariant(__id));
    if (i < 0) {
        cService *p = new cService();
        if (!p->fetchById(__q, __id)) {
            if (__ex) EXCEPTION(EFOUND, __id, QObject::trUtf8("Ismeretlen szolgáltatás azonosító."));
            delete p;
            return NULL;
        }
        services << p;
        return p;
    }
    return services.at(i);
}
*/
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
    _pFeatures = NULL;
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
    _pFeatures = NULL;
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
    _pFeatures = __o._pFeatures == NULL ? NULL : new cFeatures(*__o._pFeatures);
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
}

bool cHostService::toEnd(int i)
{
    if (i == _ixFeatures) {
        if (_pFeatures != NULL) {
            delete _pFeatures;
            _pFeatures = NULL;
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

#define _MAX_TRY_   5
cHostService&  cHostService::setState(QSqlQuery& __q, const QString& __st, const QString& __note, qlonglong __did)
{
    QString sNames = names(__q);
    _DBGFN() << sNames << VDEB(__st) << VDEB(__note) << endl;
    QVariant did;
    QString sql;
    if (__did != NULL_ID) {
        did = __did;
        sql = QString("SELECT * FROM %1(?,?,?,?)");
    }
    else {
        sql = QString("SELECT * FROM %1(?,?,?)");
    }
    sql = sql.arg(_sSetServiceStat);
    bool tf = trFlag(TS_NULL) == TS_TRUE;
    sNames = toSqlName(sNames);
    int cnt = 0;
    while (true) {
        if (tf) sqlBegin(__q, sNames);
        int r = _execSql(__q, sql, getId(), __st, __note, did);
        switch (r) {
        case 0:         // Nincs adat ?!
            EXCEPTION(EENODATA, 0, trUtf8("SQL függvény: %1(%2,%3,%4,%5)")
                      .arg(_sSetServiceStat).arg(getId()).arg(__st, __note).arg(__did)
                      );
            break;
        case 1:         // OK
            set(__q);
            if (tf) sqlCommit(__q, sNames);
            _DBGFNL() << toString() << endl;
            return *this;
        case -1:    // prepare error
        case -2:    // exec error
            QSqlError le = __q.lastError();
            if (tf) sqlRollback(__q, sNames);
            QString s = le.databaseText().split('\n').first();  // első sor
            cnt++;
            // deadlock ?
            if (s.contains("deadlock", Qt::CaseInsensitive) || cnt <= _MAX_TRY_) {
                DERR() << trUtf8("Set stat %1 to %2 SQL PREPARE ERROR #%3 try #%4\n").arg(__st, sNames).arg(le.number()).arg(cnt)
                    << trUtf8("driverText   : ") << le.driverText() << "\n"
                    << trUtf8("databaseText : ") << le.databaseText() << endl;
                QThread::msleep(200);
                continue;   // retrying
            }
            _SQLERR(le, EQUERY);    // no return
        }
    }
    return *this;   // To avoid a warning message
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
    int n = 0;
    if (execSql(q, sql, __hn, __sn)) {
        set(q);
        n = q.size();
        if (n == 1 || __ex < EX_WARNING) return n;
    }
    else {
        if (__ex == EX_IGNORE) return 0;
    }
    QString e = trUtf8("HostService pattern: \"%1\".\"%2\"").arg(__hn).arg(__sn);
    if (n) { EXCEPTION(AMBIGUOUS, n, e); }
    else   { EXCEPTION(EFOUND, 0, e);    }
    return -1;
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
    int n = 0;
    if (execSql(q, sql, bind)) {
        set(q);
        n = q.size();
        if (n == 1 || __ex < EX_WARNING) return n;
    }
    else {
        if (__ex == EX_IGNORE) return 0;
    }
    QString e = trUtf8("HostService : %1:%2.%3").arg(__hn).arg(__pn).arg(__sn);
    if (n) { EXCEPTION(AMBIGUOUS, n, e); }
    else   { EXCEPTION(EFOUND, 0, e);    }
    return -1;
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
        where += QString("AND proto_service_id = %1 ").arg(cService::nilId);
    }
    else {
        sql   += "JOIN services AS prot  ON prot.service_id = hs.proto_service_id ";
        where += "AND prot.service_name LIKE ? ";
        bind  << __pron;
    }
    // prime service
    if (__prin.isEmpty()) {
        where += QString("AND prime_service_id = %1 ").arg(cService::nilId);
    }
    else {
        sql   += "JOIN services AS prim  ON prim.service_id = hs.prime_service_id ";
        where += "AND prim.service_name LIKE ? ";
        bind  << __prin;
    }
    sql += where;
    int n = 0;
    if (execSql(q, sql, bind)) {
        set(q);
        n = q.size();
        if (n == 1 || __ex < EX_WARNING) return n;
    }
    else {
        if (__ex == EX_IGNORE) return 0;
    }
    QString e = trUtf8("HostService : %1:%2.%3(%4:%5)").arg(__hn).arg(__pn).arg(__sn).arg(__pron).arg(__prin);
    if (n) { EXCEPTION(AMBIGUOUS, n, e); }
    else   { EXCEPTION(EFOUND, 0, e);    }
    return -1;
}

bool cHostService::fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, eEx __ex)
{
    set();
    (*this)[_sNodeId]    = __hid;
    (*this)[_sServiceId] = __sid;
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (r != 1) {
        QString e = trUtf8("HostService : #%1(%2):#%3(%4)")
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
        QString e = trUtf8("HostService : %1(%2):%3(%4).%5(%6)")
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

QString cHostService::names(QSqlQuery& q)
{
    execSqlFunction(q, "host_service_id2name", getId());
    QString r = q.value(0).toString();
    return r;
}

QString cHostService::names(QSqlQuery& q, qlonglong __id)
{
    execSqlFunction(q, "host_service_id2name", __id);
    QString r = q.value(0).toString();
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

void cUserEvent::happened(QSqlQuery& q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString &_m)
{
    if (_m.isEmpty()) {
        static const QString sql =
                "UPDATE user_events SET event_state = 'happened'"
                " WHERE user_id = ? AND alarm_id = ? AND event_type = ?";
        execSql(q, sql, _uid, _aid, userEventType(_et));
    }
    else {
        static const QString sql =
                "UPDATE user_events SET event_state = 'happened', user_event_note = ?"
                " WHERE user_id = ? AND alarm_id = ? AND event_type = ?";
        execSql(q, sql, _m, _uid, _aid, userEventType(_et));
    }
}

void cUserEvent::dropped(QSqlQuery& q, qlonglong _uid, qlonglong _aid, eUserEventType _et, const QString &_m)
{
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
}


/* ----------------------------------------------------------------- */
DEFAULTCRECDEF(cAlarm, _sAlarms)

QString cAlarm::htmlText(QSqlQuery& q, qlonglong _id)
{
    const qlonglong ticketSrvId  = cService::service(q, _sTicket, EX_IGNORE)->getId();
    static const QString _sBr = "<br>";

    bool isTicket = false;
    cAlarm a;
    cRecord *pTargetRec = &a;
    a.setById(q, _id);
    QString text;   // HTML text

    if (ticketSrvId != NULL_ID && ticketSrvId == a.getId(_sHostServiceId)) {    // Ticket
        isTicket = true;
        text = trUtf8("Hiba jegy") + " :<br>";
        qlonglong id = a.getId(_sSuperiorAlarmId);
        pTargetRec = a.newObj();
        pTargetRec->fetchById(q, id);
    }
    cHostService hs; hs.   setById(q, pTargetRec->getId(_sHostServiceId));
    cNode  node;     node. setById(q, hs.         getId(_sNodeId));
    cPlace place;    place.setById(q, node.       getId(_sPlaceId));
    const cService *pSrv = cService::service(q,hs.getId(_sServiceId));
    tOwnRecords<cServiceVar, cHostService> vars(&hs);
    int n = vars.fetch(q);
    QString aMsg = execSqlTextFunction(q, "alarm_message", hs.getId(), a.get(_sMaxStatus));
    text += _sBr + trUtf8("Riasztási állpot kezdete") + " : <b>" + pTargetRec->view(q, _sBeginTime) + "</b>";
    text += _sBr + trUtf8("A hállózati elem helye")   + " : <b>" + place.getName() + "</b>, <i>" + place.getNote() + "</i>";
    text += _sBr + trUtf8("A hállózati elem neve")    + " : <b>" + node.getName()  + "</b>, <i>" + node.getNote()  + "</i>";
    text += _sBr + trUtf8("Szolgáltatás neve")        + " : <b>" + pSrv->getName() + "</b>, <i>" + pSrv->getNote() + "</i>";
    text += _sBr + trUtf8("Riasztás oka")          + " : <b><i>" + aMsg + "</i></b>";
    text += _sBr + trUtf8("Csatolt üzenet")        + " : <b><i>" + pTargetRec->getName(_sEventNote) + "</i></b>";
    if (n)  {
        cTableShape *pShape = NULL;
        if (pShape == NULL) {
            pShape = new cTableShape;
            pShape->setByName(q, _sServiceVars);
        }
        text += _sBr + trUtf8("Változók : ") + _sBr + list2html(q, vars, *pShape);
    }
    if (a.isNull(_sEndTime)) {
        text += _sBr + trUtf8("A riasztási állapot az üzenetküdéskor még aktív volt.");
    }
    else {
        text += _sBr + trUtf8("A riasztási állapot az üzenetküdéskor már nem volt aktív.");
        text += _sBr + trUtf8("A riasztási állapot vége : ") + "<b>" + pTargetRec->view(q, _sEndTime) + "</i></b>";
    }

    if (isTicket) delete pTargetRec;
    return text;
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
    return (eReasons)reasons(r);
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
    if (!isNull(_ixSetType))     bind(_ixSetType, __q, i++);
    if (!isNull(_ixMacTabState)) bind(_ixMacTabState, __q, i);
    _EXECSQL(__q);
    __q.first();
    enum eReasons r = (enum eReasons) reasons(__q.value(0).toString(), EX_IGNORE);
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
    enum eReasons r = (enum eReasons) reasons(__q.value(0).toString(), EX_IGNORE);
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
        em = trUtf8("A %1 MAC alapján az IP nem egyértelmű.").arg(em);
        if (__ex) EXCEPTION(AMBIGUOUS, n, em);
    }
    else {
        em = trUtf8("A %1 MAC-hez nincs IP cím.").arg(em);
        if (__ex) EXCEPTION(EFOUND,    n, em);
    }
    DERR() << em << endl;
    return QHostAddress();
}


cMac cArp::ip2mac(QSqlQuery& __q, const QHostAddress& __a, eEx __ex)
{
    QString em;
    if (__a.isNull()) {
        em = trUtf8("MAC keresése érvénytelen IP címmel.");
        if (__ex) EXCEPTION(EDATA, 0, em);
    }
    else {
        cArp arp;
        arp = __a;
        if (arp.fetch(__q, false, arp.mask(_ixIpAddress))) return arp;
        em = trUtf8("A %1 IP címhez-hez nincs MAC.").arg(__a.toString());
        if (__ex) EXCEPTION(EFOUND, 0, em);
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
    QString r = execSqlTextFunction(q, "replace_dyn_addr_range", b.toString(), e.toString(), dhcpSrvId, exclude, subnetId == NULL_ID ? QVariant() : QVariant(subnetId));
    return (eReasons)reasons(r);
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

CRECDDCR(cQueryParser, _sQueryParsers)
CRECDEF(cQueryParser)

cQueryParser::cQueryParser() : cRecord()
{
    pPostCmd = pPrepCmd = NULL;
    pListCmd = NULL;
    pListRExp = NULL;
    pInspector = NULL;
    pParserThread = NULL;
    _set(cQueryParser::descr());
}
\
cQueryParser::cQueryParser(const cQueryParser& __o) : cRecord()
{
    pPostCmd = pPrepCmd = NULL;
    pListCmd = NULL;
    pListRExp = NULL;
    pInspector = NULL;
    pParserThread = NULL;
    _cp(__o);
}

cQueryParser::~cQueryParser()
{
    pDelete(pPostCmd);
    pDelete(pPrepCmd);
    pDelete(pListCmd);
    pDelete(pListRExp);
    // pDelete(pParserThread);
}

void cQueryParser::_insert(QSqlQuery& q, qlonglong _sid, const QString& _ty, bool _cs, const QString& _re, const QString& _cmd, const QString& _not, qlonglong _seq)
{
    cQueryParser o;
    o.setId(  _sServiceId,         _sid);
    o.setName(_sParseType,         _ty);
    o.setBool(_sCaseSensitive,     _cs);
    o.setName(_sRegularExpression, _re);
    o.setName(_sImportExpression,  _cmd);
    o.setName(_sQueryParserNote,   _not);
    o.setId(  _sItemSequenceNumber,_seq);
    o.insert(q);
}

void cQueryParser::setInspector(cInspector *pInsp)
{
    pInspector = pInsp;
}

int cQueryParser::prep(cError *& pe)
{
    pe = NULL;
    if (pParserThread != NULL) EXCEPTION(EPROGFAIL);
    QString cmd;
    if (pPrepCmd != NULL) cmd = substitutions(*pPrepCmd, QStringList());
    pParserThread = new cImportParseThread(cmd, this);
    return pParserThread->startParser(pe);
}

int cQueryParser::parse(QString src,  cError *&pe)
{
    pe = NULL;
    if (pListCmd == NULL || pListRExp == NULL || pListCmd->size() != pListRExp->size()) EXCEPTION(EPROGFAIL);
    int i, n = pListCmd->size();
    for (i = 0; i < n; i++) {
        QRegExp rexp = pListRExp->at(i);
        if (rexp.exactMatch(src)) {
            return execute(pe, pListCmd->at(i), rexp.capturedTexts());
        }
        else continue;
    }
    PDEB(VVERBOSE) << "Nincs illeszkedes : " << src << endl;
    return R_NOTFOUND;
}

int cQueryParser::post(cError *& pe)
{
    int r = REASON_OK;
    pe = NULL;
    // Záró parancs elküldése, ha van
    if (pPostCmd != NULL) r = execute(pe, *pPostCmd);
    if (pParserThread == NULL) EXCEPTION(EPROGFAIL);
    pParserThread->stopParser();
    // Töröljük a thread-et
    pDelete(pParserThread);
    return r;
}

int cQueryParser::load(QSqlQuery& q, qlonglong _sid, bool force, bool thread)
{
    int ixServiceId = toIndex(_sServiceId);
    if (_sid != NULL_ID) {
        if (pListCmd != NULL && !force && getId(ixServiceId) == _sid) return R_UNCHANGE;
        setId(ixServiceId, _sid);
    }
    pDelete(pPostCmd);
    pDelete(pPrepCmd);
    pDelete(pListCmd);
    pDelete(pListRExp);
    qlonglong id = getId(ixServiceId);

    clear().setId(ixServiceId, id).setName(_sParseType, _sParse);
    int n = completion(q);
    if (n == 0) return R_NOTFOUND;
    pListRExp = new QList<QRegExp>;
    pListCmd  = new QStringList;
    do {
        *pListCmd << getName(_sImportExpression);
        enum Qt::CaseSensitivity cs = getBool(_sCaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive ;
        QRegExp rexp(getName(_sRegularExpression),cs);
        if (!rexp.isValid()) EXCEPTION(EDATA, getId(), getName(_sRegularExpression));
        *pListRExp << rexp;
    } while(next(q));

    clear().setId(ixServiceId, id).setName(_sParseType, _sPrep);
    n = completion(q);
    switch(n) {
    case 0: break;
    case 1: if (thread) { pPrepCmd = new QString(getName(_sImportExpression)); break; }
    default:EXCEPTION(EDATA,n, _sPrep); break;
    }

    clear().setId(ixServiceId, id).setName(_sParseType, _sPost);
    n = completion(q);
    switch(n) {
    case 0:                                                     break;
    case 1: if (thread) { pPostCmd = new QString(getName(_sImportExpression)); break; }
    default:EXCEPTION(EDATA,n, _sPost);                         break;
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
    int i = name.toUInt(&ok);
    if (ok) {
        if (i >= args.size()) EXCEPTION(EDATA, i, args.join(_sCommaSp));
        return args.at(i);
    }
    else {
        QSqlQuery q = getQuery();
        if (pInspector == NULL) EXCEPTION(EPROGFAIL);
        return pInspector->getParValue(q, name);
    }
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
    if (pParserThread == NULL) {
        if (0 == importParseText(cmd)) return REASON_OK;
        pe = importGetLastError();
        return R_ERROR;
    }
    else {
        return pParserThread->push(cmd, pe);
    }
}

