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
    PDEB(INFO) << QObject::trUtf8("Event loop is exited.") << endl;
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
        EXCEPTION(EDATA, protoServiceId(), QObject::trUtf8("Nem támogatott node típus!"));
    }
    first = true;
}

cDeviceSV::~cDeviceSV()
{
    ;
}

void cDeviceSV::postInit(QSqlQuery &q, const QString &qs)
{
    cInspector::postInit(q, qs);
    QString sVars = feature("variables");
    QMap<QString, QStringList> listmap = cFeatures::value2listMap(sVars);
    varNames = listmap.keys();
    if (varNames.isEmpty()) {
        EXCEPTION(EDATA, 0, QObject::trUtf8("Missing or empty variable list."));
    }
    foreach (QString vname, varNames) {
        QStringList sl = listmap[vname];
        if (sl.size() != 2) {   /// OId, varTypeName
            EXCEPTION(EDATA, 0, QObject::trUtf8("Invalid variable list : '%1'").arg(sVars));
        }
        cOId oid = cOId(sl.first());
        if (oid.isEmpty()) {
            EXCEPTION(EDATA, 0, QObject::trUtf8("Invalid OId : '%1'").arg(sl.first()));
        }
        oidVector << oid;
        QString typeName = sl.at(1);
        varTypes << cServiceVarType::srvartype(q, typeName);
    }
    snmpDev().open(q, snmp);
}

int cDeviceSV::queryInit(QSqlQuery &_q, QString& msg)
{
    QBitArray bits;
    int r = snmp.checkTableColumns(oidVector, bits);
    if (r != 0) {
        msg = trUtf8("queryInit() : SNMP error : ") + snmp.emsg;
        return RS_UNREACHABLE;
    }
    int i;
    if (bits.count(false)) {    // Do you have missing variables?
        for (i = varNames.size() -1; i >= 0; --i) {
            if (bits[i] == false) {     // Drop, if missing
                varNames.removeAt(i);
                oidVector.removeAt(i);
                varTypes.removeAt(i);
            }
        }
    }
    int n = varNames.size();
    if (n == 0) {
        msg = trUtf8("There is no queryable data.");
        return RS_UNREACHABLE;
    }
    if (n != oidVector.size() || n != varTypes.size() || !serviceVars.isEmpty()) {
        EXCEPTION(EPROGFAIL);
    }
    for (i = 0; i < n; ++i) {
        qlonglong varTypeId = varTypes.at(i)->getId();
        // read or create variable
        cServiceVar *psv = new cServiceVar;
        psv->setId(_sHostServiceId, hostServiceId());
        psv->setName(_sServiceVarName, varNames.at(i));
        switch (psv->completion(_q)) {
        case 0:     // Not found, create (psv cleared)
            psv->setId(_sHostServiceId, hostServiceId());
            psv->setName(_sServiceVarName, varNames.at(i));
            psv->setId(_sServiceVarTypeId, varTypeId);
            psv->insert(_q);
            break;
        case 1:     // Found, check type
            if (psv->getId(_sServiceVarTypeId) != varTypeId) {  // if modifyed type
                varTypes[i] = cServiceVarType::srvartype(_q, psv->getId(_sServiceVarTypeId));
            }
            break;
        default:
            EXCEPTION(EDATA, _q.size());
        }
        serviceVars << psv;
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
    int r = snmp.getNext(oidVector);
    if (r) {
        runMsg += trUtf8("SNMP get error : %1 in %2; host : %3, from %4 parent service.\n")
                .arg(snmp.emsg)
                .arg(name())
                .arg(lanView::getInstance()->selfNode().getName())
                .arg(parid == NULL_ID ? "NULL" : cHostService::fullName(q, parid, EX_IGNORE));
        return RS_UNREACHABLE;
    }
    int i, n = varNames.size();
    if (n != oidVector.size() || n != varTypes.size() || n != serviceVars.size()) {
        EXCEPTION(EPROGFAIL);
    }
    snmp.first();
    for (i = 0; i < n; ++i) {
        if (oidVector.at(i) < snmp.name()) {
            cServiceVar& svar = *serviceVars.at(i);
            QVariant value = snmp.value();
            svar.setValue(q, value, rs);
        }
        else {
            serviceVars.at(i)->setUnreachable(q, trUtf8("There is no such data in the SNMP query result."));
            rs = RS_UNREACHABLE;
        }
        snmp.next();
    }

    return rs;
}

