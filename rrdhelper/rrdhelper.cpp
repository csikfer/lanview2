#include "rrdhelper.h"
#include "lv2daterr.h"
#include "vardata.h"
#include "others.h"


#define VERSION_MAJOR   0
#define VERSION_MINOR   01
#define VERSION_STR     _STR(VERSION_MAJOR) "." _STR(VERSION_MINOR)

const QString& setAppHelp()
{
    return _sNul;
}

const static QString sRrdTool = "rrdtool";

int main (int argc, char * argv[])
{
    cLv2QApp app(argc, argv);

    SETAPP()
    lanView::snmpNeeded = false;
    lanView::sqlNeeded  = SN_SQL_NEED;

    lv2rrdHelper   mo;

    if (mo.lastError) {  // Ha hiba volt, vagy vége
        return mo.lastError->mErrorCode; // a mo destruktora majd kiírja a hibaüzenetet.
    }
    // A továbbiakban a timer/ db notif. ütemében történnek az események
    int r = app.exec();
    PDEB(INFO) << QObject::tr("Event loop is exited.") << endl;
    exit(mo.lastError == nullptr ? r : mo.lastError->mErrorCode);
}

lv2rrdHelper::lv2rrdHelper() : lanView()
{
    if (lastError == nullptr) {
        try {
            insertStart(*pQuery);
            subsDbNotif();
            setup();
        } CATCHS(lastError)
    }
}

lv2rrdHelper::~lv2rrdHelper()
{
    down();
}

void lv2rrdHelper::staticInit(QSqlQuery *pq)
{
    (void)pq;
}

void lv2rrdHelper::setup(eTristate _tr)
{
    staticInit(pQuery);
    lanView::tSetup<cRrdHelper>(_tr);
}

void    lv2rrdHelper::dbNotif(const QString & name, QSqlDriver::NotificationSource source, const QVariant & payload)
{
    PDEB(VERBOSE) << QString(tr("DB notification : %1, %2, %3.")).arg(name).arg(int(source)).arg(debVariantToString(payload)) << endl;
    QString cmd = payload.toString();
    static const QString rrd = "rrd ";
    if (cmd.startsWith(rrd)) {   // Is RRD command
        cRrdHelper *pi = dynamic_cast<cRrdHelper *>(pSelfInspector);
        if (pi == nullptr) EXCEPTION(EPROGFAIL);
        pi->execRrd(cmd);
    }
    else {
        lanView::dbNotif(name, source, payload);
    }
}


/******************************************************************************/

int  cRrdFile::cntAll = 0;
int  cRrdFile::cntEna = 0;

cRrdHelper::cRrdHelper(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    cntOk = cntFail = 0;
    accesType = RAT_LOCAL;
}

cRrdHelper::~cRrdHelper()
{

}

inline void dirCat(QDir& dir, const QString& _to) { dir.setPath(dir.filePath(_to)); }

void cRrdHelper::postInit(QSqlQuery &q)
{
    cInspector::postInit(q);
    // RRD access: Local or by daemon
    QString daemon = feature("daemon");
    if (!daemon.isEmpty()) {
        daemonOption << "-d";
        daemonOption << daemon;
        if (daemon.startsWith("unix:")) accesType = RAT_LOCAL_BY_DAEMON;
        else                            accesType = RAT_REAMOTE_BY_DAEMON;
    }
    // Base dir ...
    static const QString _sBaseDir = "base_dir";    // Feature OR sys_param name
    QString sBaseDir = feature(_sBaseDir);          // base dir in feature?
    QString sSubDir  = "rrd";
    if (accesType == RAT_REAMOTE_BY_DAEMON) {
        // No check base directory
        if (sBaseDir.isEmpty()) {                   // no feature, get sys_param or home dir
            sBaseDir = cSysParam::getTextSysParam(q, _sBaseDir, lanView::getInstance()->homeDir);
            baseDir.setPath(sBaseDir);
            dirCat(baseDir, sSubDir);
        }
        else {  // In the feature, the name of the RRD folder is specified
            baseDir.setPath(sBaseDir);
        }
    }
    else {
        // Check base directory, and change current
        if (sBaseDir.isEmpty()) {                       // no feature, get sys_param or home dir
            sBaseDir = cSysParam::getTextSysParam(q, _sBaseDir, lanView::getInstance()->homeDir);
            baseDir.setPath(sBaseDir);
        }
        else {  // In the feature, the name of the RRD folder is specified
            baseDir.setPath(sBaseDir);
            sSubDir  = baseDir.dirName();
            baseDir.cdUp();
        }
        if (!baseDir.exists(sSubDir)) {
            if (!baseDir.mkdir(sSubDir)) {
                EXCEPTION(ENODIR, 0, baseDir.path());
            }
            PDEB(INFO) << tr("The RRD Base Folder (%1) was created").arg(sBaseDir) << endl;
        }
        baseDir.cd(sSubDir);
        if (!QDir::setCurrent(baseDir.path()) || !QFileInfo(baseDir.path()).isWritable()) {
            EXCEPTION(EFOPEN, 0, baseDir.path());
        }
    }
}

int cRrdHelper::run(QSqlQuery& q, QString &runMsg)
{
    (void)q;
    runMsg = tr("Enabled object %1/%2, update request %3 Ok, %4 Failed.")
            .arg(cRrdFile::cntEna)
            .arg(cRrdFile::cntAll)
            .arg(cntOk)
            .arg(cntFail);
    int r = RS_ON;
    if (cRrdFile::cntEna != cRrdFile::cntAll) {
        r = RS_CRITICAL;
    }
    if (cntFail != 0) {
        r = RS_WARNING;
    }
    msgAppend(&runMsg, getErrorMessages());
    PDEB(VERBOSE) << runMsg << tr("State : %1").arg(notifSwitch(r, EX_IGNORE)) << endl;
    cntOk = cntFail = 0;
    return r;
}

/// Notify parameter inxdexes
enum ixRRDPar { RP_RRD, RP_HELPER_ID_IX, RP_TIMESTAMP_IX, RP_VALUE_IX, RP_SERVICE_VAR_ID_IX, RP_FIELD_NUM };
void cRrdHelper::execRrd(const QString& payload)
{
    cntFail++;  // it's not good yet
    bool ok1, ok2;
    QSqlQuery q = getQuery();
    QStringList sl = payload.split(QChar(' '), QString::SkipEmptyParts);
    if (sl.size() != RP_FIELD_NUM) {
        APPMEMO(q, tr("Invalid RRD notify (invalid field number) : '%1'").arg(payload), RS_CRITICAL);
        return;
    }
    qlonglong helperId      = sl.at(RP_HELPER_ID_IX).toLongLong(&ok1);
    qlonglong serviceVarId  = sl.at(RP_SERVICE_VAR_ID_IX).toLongLong(&ok2);
    if (!ok1 || !ok2) {
        APPMEMO(q, tr("Invalid RRD notify (non numeric ID) : '%1'").arg(payload), RS_CRITICAL);
        return;
    }
    if (helperId != hostServiceId()) return;    // It wasn't for us
    QMap<qlonglong, cRrdFile>::iterator i = rrdFileMap.find(serviceVarId);
    if (i != rrdFileMap.end()) {
        if (!i.value().enabled) {   // Disable, reported problem
            return;
        }
    }
    else {
        // Not found cRrdFile objet, create
        i = rrdFileMap.insert(serviceVarId, cRrdFile());    // Empty disabled object
        // ID -> object names
        static const QString sql =
                "SELECT node_name, port_name, service_name, service_var_name "
                " FROM service_rrd_vars"
                " JOIN host_services USING(host_service_id)"
                " JOIN services USING(service_id)"
                " JOIN nodes USING(node_id)"
                " LEFT OUTER JOIN nports USING(port_id)"
                " WHERE service_var_id = ?";
        if (!execSql(q, sql, serviceVarId)) {
            APPMEMO(q, tr("The service_rrd_vars record not found (ID : %2) : '%1'").arg(payload).arg(serviceVarId), RS_CRITICAL);
            return;
        }
        const QString nodeName    = q.value(0).toString();
        const bool port_is_null   = q.isNull(1);
        const QString portName    = port_is_null ? _sNul : q.value(1).toString();
        const QString serviceName = q.value(2).toString();
        const QString varName     = q.value(3).toString();
        QString subDir;
        subDir = nodeName;
        if (!port_is_null) subDir += QDir::separator() + portName;
        subDir += QDir::separator() + serviceName;
        i.value().fileName = subDir + QDir::separator() + varName + ".rrd";
        if (accesType != RAT_REAMOTE_BY_DAEMON) {
            // Check dir
            if (!baseDir.mkpath(subDir)) {
                APPMEMO(q, tr("The service_rrd_vars record not found (ID : %2) : '%1'").arg(payload).arg(serviceVarId), RS_CRITICAL);
                return;
            }
        }
        // if (accesType == RAT_REAMOTE_BY_DAEMON) Az rrdcached-t eddig nem sikerült rávenni, hogy létrehozza a konyvtárakat hiába adtam meg a -B -R opciókat.
        if (accesType == RAT_LOCAL) {
            QFileInfo fi(i.value().fileName);
            if (fi.exists() && fi.isRelative() && fi.isWritable()) {
                i.value().enable();     // RRD file is exist, and accessible
            }
        }
        else {
            QStringList args;
            args << "info";
            args << daemonOption;
            args << i.value().fileName;
            QString msg;
            int r = startProcessAndWait(sRrdTool, args, &msg);
            if (r == 0) {   // File exists, and accessible
                i.value().enable();
            }
        }
        if (i.value().enabled != true) {
            if (createRrdFile(q, i)) {
                i.value().enable();
                PDEB(INFO) << tr("Created the %2 RRD file : '%1'").arg(payload, i.value().fileName) << endl;
            }
            else {
                APPMEMO(q, tr("I can't create the %2 RRD file : '%1'").arg(payload, i.value().fileName), RS_CRITICAL);
                return;
            }
        }
    }
    QStringList args;
    args << "update";
    args << daemonOption;
    args << i.value().fileName;
    args << QString("%1:%2").arg(sl.at(RP_TIMESTAMP_IX), sl.at(RP_VALUE_IX));
    QString msg;
    int r = startProcessAndWait(sRrdTool, args, &msg);
    if (r != 0) {
        msgAppend(&msg, tr("I can't update the %1 RRD file.").arg(i.value().fileName));
        i->addErrorMsg(msg);
        PDEB(DERROR) << msg << endl;
    }
    else {
        msgAppend(&msg, tr("Updated the %1 RRD file.").arg(i.value().fileName));
        PDEB(VERBOSE) << msg << endl;
        --cntFail;
        ++cntOk;
    }
}

enum eRRA { RRA_DAILY, RRA_WEEKLY, RRA_MONTHLY, RRA_YEARLY, RRA_NUMBER };
enum eFXX { FXX_STEP, FXX_SIZE, FXX_AGREGATES, FXX_NUMBER };

bool cRrdHelper::createRrdFile(QSqlQuery& q, QMap<qlonglong, cRrdFile>::iterator i)
{
    PDEB(INFO) << "RRD file : " << i.value().fileName << endl;
    cRrdBeat rrdBeat;
    static const int indexes[RRA_NUMBER][FXX_NUMBER] = {
        //             FXX_STEP                     FXX_SIZE                     FXX_AGREGATES
        { rrdBeat.toIndex(_sDailyStep),   rrdBeat.toIndex(_sDailySize),   rrdBeat.toIndex(_sDailyAggregates)   },  // RRA_DAILY
        { rrdBeat.toIndex(_sWeeklyStep),  rrdBeat.toIndex(_sWeeklySize),  rrdBeat.toIndex(_sWeeklyAggregates)  },  // RRA_WEEKLY
        { rrdBeat.toIndex(_sMonthlyStep), rrdBeat.toIndex(_sMonthlySize), rrdBeat.toIndex(_sMonthlyAggregates) },  // RRA_MONTHLY
        { rrdBeat.toIndex(_sYearlyStep),  rrdBeat.toIndex(_sYearlySize),  rrdBeat.toIndex(_sYearlyAggregates)  },  // RRA_YEARLY
    };
    cServiceRrdVar  serviceRrdVar;
    cServiceVarType serviceVarType;
    cParamType      paramType;

    serviceRrdVar.setById(q, i.key());
    serviceVarType.setById(q, serviceRrdVar.getId(_sServiceVarTypeId));
    rrdBeat.setById(q, serviceRrdVar.getId(_sRrdBeatId));
    paramType.setById(q, serviceVarType.getId(_sRawParamTypeId));
    QString min = "0";
    QString max = "2000000000";
    eFilterType pft = eFilterType(serviceVarType.getId(_sPlausibilityType));
    bool        pin = serviceVarType.getBool(_sPlausibilityInverse);
    if (pft == FT_INTERVAL) {
        if (!pin) {
            min = serviceVarType.getName(_sPlausibilityParam1);   // !! interval !!
            max = serviceVarType.getName(_sPlausibilityParam2);   // !! interval !!
        }
    }
    else {
        switch (pft) {
        case FT_BIG:    if (pin) pft = FT_LITLE;    break;
        case FT_LITLE:  if (pin) pft = FT_BIG;      break;
        default:        break;
        }
        switch (pft) {
        case FT_BIG:    min = serviceVarType.getName(_sPlausibilityParam1);    break; // !! interval !!
        case FT_LITLE:  max = serviceVarType.getName(_sPlausibilityParam1);    break; // !! interval !!
        default:        break;
        }
    }

    QStringList args;
    args << "create" << i.value().fileName;
    args << daemonOption;
    args << "-s" << QString::number(rrdBeat.getId(_sStep) / 1000); // step [sec], getId(_sStep) returns millisec
    args << QString("DS:%1:%2:%3:%4:%5")
            .arg(i.value().varName, serviceVarType.getName(_sServiceVarType))  // name, Type
            .arg(rrdBeat.getId(_sHeartbeat) / 1000)         // heatrbeat [sec]
            .arg(min, max);                                 // min, max
    for (int rra = 0; rra < RRA_NUMBER; rra++) {
        QStringList averages = rrdBeat.getStringList(indexes[rra][FXX_AGREGATES]);
        qlonglong step = rrdBeat.getId(indexes[rra][FXX_STEP]);
        qlonglong size = rrdBeat.getId(indexes[rra][FXX_SIZE]);
        PDEB(VVERBOSE) << "RRA " << averages.join(_sCommaSp) << _sCommaSp << step << _sCommaSp << size << endl;
        foreach (QString average, averages) {
            args << QString("RRA:%1:0.5:%2:%3").arg(average).arg(step).arg(size);
        }
    }
    QString msg;
    int r = startProcessAndWait(sRrdTool, args, &msg);
    if (r != 0) {
        msg = tr("I can't create the %1 RRD file\n%2").arg(i.value().fileName, msg);
        APPMEMO(q, msg, RS_CRITICAL);
        PDEB(DERROR) << msg << endl;
        return false;
    }
    else {
        msg = tr("Create the %1 RRD file.").arg(i.value().fileName);
        APPMEMO(q, msg, RS_ON);
        PDEB(INFO) << msg << endl;
        return true;
    }
}

QString cRrdHelper::getErrorMessages()
{
    QString r;
    QList<qlonglong> keys = rrdFileMap.keys();
    foreach (qlonglong key, keys) {
        msgAppend(&r, rrdFileMap[key].getErrorMsg());
    }
    return r;
}
