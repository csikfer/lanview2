#include "snmpvars.h"
#include "lv2daterr.h"


#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

int main (int argc, char * argv[])
{
    cLv2QApp app(argc, argv);

    SETAPP();
    lanView::snmpNeeded = true;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2snmpVars   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // A továbbiakban a timer ütemében történnek az események
    int r = app.exec();
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2snmpVars::lv2snmpVars() : lanView()
{
    pSelfInspector = nullptr;  // Main service == APPNAME, standard init
    if (lastError == nullptr) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            setup();
        } CATCHS(lastError)
    }
}

lv2snmpVars::~lv2snmpVars()
{
    down();
}

void lv2snmpVars::staticInit(QSqlQuery *pq)
{
    (void)pq;
}

void lv2snmpVars::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::tSetup<cSnmpVars>(_tr);
}


/******************************************************************************/

cSnmpVars::cSnmpVars(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    ;
}

cSnmpVars::~cSnmpVars()
{
    ;
}

cInspector * cSnmpVars::newSubordinate(QSqlQuery &q, qlonglong hsid, qlonglong hoid, cInspector *pid)
{
    return new cDeviceSV(q, hsid, hoid, pid);
}

/******************************************************************************/

cDeviceSV::cDeviceSV(QSqlQuery& __q, qlonglong __host_service_id, qlonglong __tableoid, cInspector * _par)
    : cInspector(__q, __host_service_id, __tableoid, _par)
    , snmp()
{
    // Csak SNMP device lehet
    if (__tableoid != cSnmpDevice().tableoid()) {
        EXCEPTION(EDATA, protoServiceId(), QObject::tr("Nem támogatott node típus!"));
    }
    first = true;
    next = TS_NULL;
}

cDeviceSV::~cDeviceSV()
{
    ;
}

void cDeviceSV::postInit(QSqlQuery &q, const QString &qs)
{
    cInspector::postInit(q, qs);
    snmpDev().open(q, snmp);
}

void cDeviceSV::variablesPostInit(QSqlQuery &q)
{
    if (!variablesListFeature(q, _sVariables)) {
        EXCEPTION(EDATA, 0, QObject::tr("Missing or empty variable list."));
    }
    varNames = varsFeatureMap.keys();
    foreach (QString vname, varNames) {
        QString soid = varsFeatureMap[vname];
        bool get = soid.endsWith(QChar('.'));
        if (get) soid.chop(1);
        if (next == TS_NULL)  next = bool2ts(!get);
        else if (toBool(next) == get) {
            EXCEPTION(EDATA, 0, QObject::tr("Invalid variable list (query methode) : '%1'").arg(feature(_sVariables)));
        }
        cOId oid = cOId(soid);
        if (oid.isEmpty()) {
            EXCEPTION(EDATA, 0, QObject::tr("Error OId : '%1'; %2").arg(soid, oid.emsg));
        }
        oidVector << oid;
    }
}

int cDeviceSV::queryInit(QSqlQuery &_q, QString& msg)
{
    if (next == TS_NULL) {
        EXCEPTION(EPROGFAIL);
    }
    int i, n;
    n = varNames.size();
    if (next == TS_TRUE) {
        QBitArray bits;
        int r = snmp.checkTableColumns(oidVector, bits);
        if (r != 0) {
            msg = tr("queryInit() : SNMP error : ") + snmp.emsg;
            return RS_UNREACHABLE;
        }
        if (bits.count(false)) {    // Do you have missing variables?
            for (i = n -1; i >= 0; --i) {
                if (bits[i] == false) {     // Drop, if missing
                    varNames.removeAt(i);
                    oidVector.removeAt(i);
                }
            }
        }
        n = varNames.size();
        if (n == 0) {
            msg = tr("There is no queryable data.");
            return RS_UNREACHABLE;
        }
    }
    if (n != oidVector.size()) {
        EXCEPTION(EPROGFAIL);
    }
    for (i = 0; i < n; ++i) {
        const QString& vName = varNames.at(i);
        variablePostInitCreateOrCheck(_q, vName);
    }
    return RS_ON;
}

int cDeviceSV::run(QSqlQuery& q, QString &runMsg)
{
    if (first) {
        if (RS_ON != queryInit(q, runMsg)) {
            return RS_UNREACHABLE;
        }
        first = false;
    }
    int rs = RS_ON;
    qlonglong parid = parentId(EX_IGNORE);
    int r;
    if (next == TS_TRUE) {
        r = snmp.getNext(oidVector);
    }
    else {
        r = snmp.get(oidVector);
    }
    if (r) {
        runMsg += tr("SNMP get error : %1 in %2; host : %3, from %4 parent service.\n")
                .arg(snmp.emsg)
                .arg(name())
                .arg(lanView::getInstance()->selfNode().getName())
                .arg(parid == NULL_ID ? "NULL" : cHostService::fullName(q, parid, EX_IGNORE));
        return RS_UNREACHABLE;
    }
    int i, n = varNames.size();
    if (n != oidVector.size()) {
        EXCEPTION(EPROGFAIL);
    }
    snmp.first();
    if (next == TS_TRUE) {
        for (i = 0; i < n; ++i) {
            const QString& vName = varNames.at(i);
            cInspectorVar& svar = *getInspectorVar(vName, EX_ERROR);
            if (oidVector.at(i) < snmp.name()) {
                QVariant value = snmp.value();
                svar.setValue(q, value, rs);
            }
            else {
                svar.setUnreachable(q, tr("There is no such data in the SNMP query result."));
                rs = RS_UNREACHABLE;
            }
            snmp.next();
        }
    }
    else {
        for (i = 0; i < n; ++i) {
            const QString& vName = varNames.at(i);
            cInspectorVar& svar = *getInspectorVar(vName, EX_ERROR);
            QVariant value = snmp.value();
            svar.setValue(q, value, rs);
            snmp.next();
        }
    }
    return rs;
}

