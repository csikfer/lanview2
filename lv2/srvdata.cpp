#include "srvdata.h"
#include "lv2service.h"
#include "import_parser.h"

QString getParName(QString::const_iterator& i, const QString::const_iterator& e, bool __ex)
{
    QString r;
    if (i == e) return r;
    if (*i == QChar('{')) {
        for (++i; i != e && *i != QChar('}'); ++i) r += *i;
        if (i == e) {
            if (__ex) EXCEPTION(EDATA);
            return _sNul;
        }
        ++i;
        return r;
    }
    for (; i != e; ++i) {
        QChar c = *i;
        if (c.isLetterOrNumber() || c == QChar('.') || c == QChar('_')) {
            r += c;
        }
        else {
            break;
        }
    }
    return r;
}

/* ******************************  ****************************** */
DEFAULTCRECDEF(cIpProtocol, _sIpProtocols)
/* -------------------------------------------------------------- */
DEFAULTCRECDEF(cServiceType, _sServiceTypes)

qlonglong cServiceType::insertNew(QSqlQuery& __q, const QString& __name, const QString& __note, bool __ex)
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
    o.setId(__stid);
    o.setName(_sStatus, __stat);
    bool e = o.completion(__q);
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

cService::cService() : cRecord(), _protocol()
{
    _pFeatures = NULL;
    _set(cService::descr());
}

cService::cService(const cService& __o) : cRecord(), _protocol(__o._protocol)
{
    _fields = __o._fields;      // Nincs öröklés !
    _stat    = __o._stat;
    _pFeatures = __o._pFeatures == NULL ? NULL : new cFeatures(*__o._pFeatures);
}

cService::~cService()
{
    pDelete(_pFeatures);
}

int             cService::_ixProtocolId = NULL_IX;
int             cService::_ixFeatures = NULL_IX;
const cRecStaticDescr&  cService::descr() const
{
    if (initPDescr<cService>(_sServices)) {
        _ixProtocolId = _descr_cService().toIndex(_sProtocolId);
        _ixFeatures = _descr_cService().toIndex(_sFeatures);
    }
    return *_pRecordDescr;
}
void cService::toEnd()
{
    cService::toEnd(_ixProtocolId);
    cService::toEnd(_ixFeatures);
}

bool cService::toEnd(int i)
{
    if (i == _ixProtocolId) {
        if (get(i).isValid()) {
            _protocol.fetchById(getId(i));
        }
        else _protocol.set();
        return true;
    }
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
    _protocol.set();
}
CRECDEF(cService)


tRecordList<cService> cService::services;
cService *cService::pNull;
const cService& cService::service(QSqlQuery& __q, const QString& __nm, bool __ex)
{
    int i = services.indexOf(_nul().nameIndex(), QVariant(__nm));
    if (i < 0) {
        cService *p = new cService();
        if (!p->fetchByName(__q, __nm)) {
            if (__ex) EXCEPTION(EFOUND, -1, QString(QObject::trUtf8("Ismeretlen szolgáltatás név : %1")).arg(__nm));
            delete p;
            return _nul();
        }
        services << p;
        return *p;
    }
    return *services.at(i);
}

const cService& cService::service(QSqlQuery &__q, qlonglong __id, bool __ex)
{
    int i = services.indexOf(_nul().idIndex(), QVariant(__id));
    if (i < 0) {
        cService *p = new cService();
        if (!p->fetchById(__q, __id)) {
            if (__ex) EXCEPTION(EFOUND, __id, QObject::trUtf8("Ismeretlen szolgáltatás azonosító."));
            delete p;
            return _nul();
        }
        services << p;
        return *p;
    }
    return *services.at(i);
}

/* ----------------------------------------------------------------- */

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
        // Ez egy SWITCH tábla kell legyen ...
        if (_descr_cHostService().tableType() != SWITCH_TABLE) EXCEPTION(EPROGFAIL, _descr_cHostService().tableType(), _descr_cHostService().toString());
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

int cHostService::replace(QSqlQuery &__q, bool __ex)
{
    (void)__q;
    if (__ex) EXCEPTION(ENOTSUPP);
    return R_ERROR;
}

cHostService&  cHostService::setState(QSqlQuery& __q, const QString& __st, const QString& __note, qlonglong __did)
{
    QString sql = QString("SELECT * FROM set_service_stat(%1, '%2', '%3'%4)")
            .arg(getId())
            .arg(__st)
            .arg(__note)
            .arg(__did == NULL_ID ? _sNul : (_sCommaSp + QString::number(__did)) );
    PDEB(VVERBOSE) << "setState() : " << sql << endl;
    if (!__q.exec(sql)) SQLPREPERR(__q, sql);
    if (!__q.first()) SQLERR(__q, EQUERY);
    set(__q);
    // DBGFNL();
    return *this;
}

int cHostService::fetchByNames(QSqlQuery& q, const QString &__hn, const QString& __sn, bool __ex)
{
    set();
    (*this)[_sNodeId]    = cNode().descr().getIdByName(q, __hn, __ex);
    (*this)[_sServiceId] = cService().descr().getIdByName(q, __sn, __ex);
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (r != 1) {
        QString e = trUtf8("HostService : %1:*.%2(*:*)").arg(__hn).arg(__sn);
        if (__ex) {
            if (r == 0) {
                EXCEPTION(EFOUND,    r, e);
            }
            else {
                EXCEPTION(AMBIGUOUS, r, e);
            }
        }
        return r;
    }
    return 1;
}

int cHostService::fetchByNames(QSqlQuery& q, const QString& __hn, const QString& __sn, const QString& __pn, bool __ex)
{
    set();
    (*this)[_sNodeId]    = cNode().descr().getIdByName(q, __hn, __ex);
    (*this)[_sServiceId] = cService().descr().getIdByName(q, __sn, __ex);
    if (__pn.isEmpty() == false) {
        qlonglong pid = cNPort().getPortIdByName(q, __pn,  __hn, __ex);
        if (pid == NULL_ID) return 0;
        (*this)[_sPortId] = pid;
    }
    (*this)[_sDeleted]   = false;
    int r = 0;
    if (fetch(q, false, mask(_sNodeId, _sServiceId, _sPortId, _sDeleted))) r = q.size();
    if (r != 1) {
        QString e = trUtf8("HostService : %1:%2.%3(*:*)").arg(__hn).arg(__pn).arg(__sn);
        if (__ex) {
            if (r == 0) {
                EXCEPTION(EFOUND,    r, e);
            }
            else {
                EXCEPTION(AMBIGUOUS, r, e);
            }
        }
        return r;
    }
    return 1;
}

int cHostService::fetchByNames(QSqlQuery& q, const QString &__hn, const QString& __sn, const QString& __pn, const QString& __pron, const QString& __prin, bool __ex)
{
    set();
    (*this)[_sNodeId]    = cNode().descr().getIdByName(q, __hn, __ex);
    (*this)[_sServiceId] = cService().descr().getIdByName(q, __sn, __ex);
    qlonglong id;
    if (__pn.isEmpty() == false) {
        id = cNPort().getPortIdByName(q, __pn,  __hn, __ex);
        if (id == NULL_ID) return 0;
        (*this)[_sPortId] = id;
    }
    if (__pron.isEmpty() == false && _sNil != __pron) {
        id = cService().getIdByName(q, __pron, __ex);
        if (id == NULL_ID) return 0;
    }
    else id = -1;   //nil
    (*this)[_sProtoServiceId] = id;
    if (__prin.isEmpty() == false && _sNil != __prin) {
        qlonglong id = cService().getIdByName(q, __prin, __ex);
        if (id == NULL_ID) return 0;
    }
    else id = -1;   //nil
    (*this)[_sPrimeServiceId] = id;

    (*this)[_sDeleted]   = false;
    int r = 0;
    if (fetch(q, false, mask(_sNodeId, _sServiceId, _sPortId, _sDeleted) | mask(_sProtoServiceId, _sPrimeServiceId))) r = q.size();
    if (r != 1) {
        QString e = trUtf8("HostService : %1:%2.%3(%4:%5)").arg(__hn).arg(__pn).arg(__sn).arg(__pron).arg(__prin);
        if (__ex) {
            if (r == 0) {
                EXCEPTION(EFOUND,    r, e);
            }
            else {
                EXCEPTION(AMBIGUOUS, r, e);
            }
        }
        return r;
    }
    return 1;
}

int cHostService::fetchFirstByNamePatterns(QSqlQuery& q, const QString& __hn, const QString& __sn, bool __ex)
{
    QString sql =
            "SELECT host_services.* "
              "FROM host_services "
              "JOIN nodes USING(node_id) "
              "JOIN services USING(service_id) "
              "WHERE node_name LIKE ? "
                "AND service_name LIKE ?";
    if (!q.prepare(sql)) SQLPREPERR(q, sql);
    q.bindValue(0, __hn);
    q.bindValue(1, __sn);
    if (!q.exec()) SQLQUERYERR(q);
    if (q.first()) {
        set(q);
        return q.size();
    }
    QString e = trUtf8("HostService not found, pattern: %1:%2").arg(__hn).arg(__sn);
    if (__ex) EXCEPTION(EFOUND, 0, e);
    DWAR() << e << endl;
    return 0;
}


bool cHostService::fetchByIds(QSqlQuery& q, qlonglong __hid, qlonglong __sid, bool __ex)
{
    set();
    (*this)[_sNodeId]    = __hid;
    (*this)[_sServiceId] = __sid;
    (*this)[_sDeleted]   = false;
    int r = completion(q);
    if (r != 1) {
        QString e = trUtf8("HostService : %1(%2):%3(%4)")
                .arg(__hid).arg(cNode().   getNameById(__hid, false))
                .arg(__sid).arg(cService().getNameById(__sid, false));
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
    if (!q.exec()) SQLQUERYERR(q);
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

bool cHostService::fetchSelf(QSqlQuery& q, cNode& __h, const cService &__s, bool __ex)
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
DEFAULTCRECDEF(cAlarm, _sAlarms)

/* ----------------------------------------------------------------- */
DEFAULTCRECDEF(cOui, _sOuis)

int cOui::replace(QSqlQuery& __q, bool __ex)
{
    (void)__ex;
    if (!execSqlFunction(__q, "replace_oui", toSql(_sOui), toSql(_sOuiName), toSql(_sOuiNote))) {
        EXCEPTION(EPROGFAIL);
    }
    QString r = __q.value(0).toString();
    return (eReasons)reasons(r);
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

int cMacTab::replace(QSqlQuery& __q, bool __ex)
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
    if (!__q.exec()) SQLQUERYERR(__q);
    __q.first();
    enum eReasons r = (enum eReasons) reasons(__q.value(0).toString(), false);
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

int cArp::replace(QSqlQuery& __q, bool __ex)
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
    if (!__q.exec()) SQLQUERYERR(__q);
    __q.first();
    enum eReasons r = (enum eReasons) reasons(__q.value(0).toString(), false);
    return r;
}

#ifdef MUST_SCAN
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
#else  // MUST_SCAN
int cArp::replaces(QSqlQuery&, const cArpTable&, int, qlonglong)
{
    EXCEPTION(ENOTSUPP);
    return 0;
}
#endif // MUST_SCAN

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

QHostAddress cArp::mac2ip(QSqlQuery& __q, const cMac& __m, bool __ex)
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


cMac cArp::ip2mac(QSqlQuery& __q, const QHostAddress& __a, bool __ex)
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
    if (!q.exec(sql)) SQLPREPERR(q, sql);
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

