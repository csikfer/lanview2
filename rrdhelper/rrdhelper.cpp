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

    SETAPP();
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
    pSelfInspector = nullptr;  // Main service == APPNAME, standard init
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
}

cRrdHelper::~cRrdHelper()
{
    ;
}


void cRrdHelper::postInit(QSqlQuery &q, const QString &qs)
{
    cInspector::postInit(q, qs);
    static const QString _sBaseDir = "base_dir";
    QString sBaseDir = feature(_sBaseDir);
    QString sSubDir;
    if (sBaseDir.isEmpty()) {
        sBaseDir = cSysParam::getTextSysParam(q, _sBaseDir, lanView::getInstance()->homeDir);
        sSubDir  = "rrd";
        baseDir.setPath(sBaseDir);
    }
    else {
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
    if (!QFileInfo(baseDir.path()).isWritable()) {
        EXCEPTION(EFOPEN, 0, baseDir.path());
    }
    QString daemon = feature("daemon");
    if (!daemon.isEmpty()) {
        daemonOption << "-d";
        daemonOption << daemon;
    }
}

int cRrdHelper::run(QSqlQuery& q, QString &runMsg)
{
    (void)q;
    runMsg = tr("Enabled object %1/%2, update reauest %3, %4 Failed.")
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
    PDEB(VERBOSE) << runMsg << tr("State : %1").arg(notifSwitch(r, EX_IGNORE)) << endl;
    cntOk = cntFail = 0;
    return r;
}
enum ixRRDPar { RP_RRD, RP_HELPER_ID_IX, RP_TIMESTAMP, RP_VALUE_IX, RP_SERVICE_VAR_ID_IX, RP_FIELD_NUM };
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
        i = rrdFileMap.insert(serviceVarId, cRrdFile());
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
        QString node    = q.value(0).toString();
        QString port;
        bool port_is_null = q.isNull(1);
        if (!port_is_null) port = q.value(1).toString();
        QString service = q.value(2).toString();
        QString var     = q.value(3).toString();
        QDir dir = baseDir;
        // check <base_dir>/<node>
        if (!dir.exists(node)) {
            if (!dir.mkdir(node)) {
                APPMEMO(q, tr("I can't create the %2 (node) folder : '%1'").arg(payload, dir.filePath(node)), RS_CRITICAL);
                return;
            }
            PDEB(INFO) << tr("Created the %2 (node) folder : '%1'").arg(payload, dir.filePath(node)) << endl;
        }
        dir.cd(node);
        // check <base_dir>/<node>/<port> if port is not null
        if (!port_is_null) {
            if (!dir.exists(port)) {
                if (!dir.mkdir(port)) {
                    APPMEMO(q, tr("I can't create the %2 (port) folder : '%1'").arg(payload, dir.filePath(port)), RS_CRITICAL);
                    return;
                }
                PDEB(INFO) << tr("Created the %2 (port) folder : '%1'").arg(payload, dir.filePath(port)) << endl;
            }
            dir.cd(port);
        }
        // check <base_dir>/<node>[/<port>]/<service>
        if (!dir.exists(service)) {
            if (!dir.mkdir(service)) {
                APPMEMO(q, tr("I can't create the %2 (service) folder : '%1'").arg(payload, dir.filePath(service)), RS_CRITICAL);
                return;
            }
            PDEB(INFO) << tr("Created the %2 (service) folder : '%1'").arg(payload, dir.filePath(service)) << endl;
        }
        dir.cd(service);
        i.value().fileName = dir.filePath(var + ".rrd");
        if (dir.exists(var)) {
            i.value().enable();
        }
        else {
            if (createRrdFile(q, i)) {
                i.value().enable();
                PDEB(INFO) << tr("Created the %2 RRD file : '%1'").arg(payload, dir.filePath(var)) << endl;
            }
            else {
                APPMEMO(q, tr("I can't create the %2 RRD file : '%1'").arg(payload, dir.filePath(var)), RS_CRITICAL);
                return;
            }
        }
    }
    QStringList args;
    args << "update";
    args << daemonOption;
    args << i.value().fileName;
    args << QString("%1:%2").arg(sl.at(RP_TIMESTAMP), sl.at(RP_VALUE_IX));
    QString msg;
    int r = startProcessAndWait(sRrdTool, args, &msg);
    if (r != 0) {
        msg = tr("I can't update the %1 RRD file.\n %2").arg(i.value().fileName, msg);
        APPMEMO(q, msg, RS_CRITICAL);
        PDEB(WARNING) << msg << endl;
    }
    else {
        PDEB(VERBOSE) << tr("Updated the %1 RRD file.\n %2").arg(i.value().fileName, msg);
        --cntFail;
        ++cntOk;
    }
}

enum eRRA { RRA_DAILY, RRA_WEEKLY, RRA_MONTHLY, RRA_YEARLY, RRA_NUMBER };
enum eFXX { FXX_STEP, FXX_SIZE, FXX_AGREGATES, FXX_NUMBER };

bool cRrdHelper::createRrdFile(QSqlQuery& q, QMap<qlonglong, cRrdFile>::iterator i)
{
    cRrdBeat beat;
    static const int indexes[RRA_NUMBER][FXX_NUMBER] = {
        //             FXX_STEP                     FXX_SIZE                     FXX_AGREGATES
        { beat.toIndex(_sDailyStep),   beat.toIndex(_sDailyStep),   beat.toIndex(_sDailyAggregates)   },  // RRA_DAILY
        { beat.toIndex(_sWeeklyStep),  beat.toIndex(_sWeeklyStep),  beat.toIndex(_sWeeklyAggregates)  },  // RRA_WEEKLY
        { beat.toIndex(_sMonthlyStep), beat.toIndex(_sMonthlyStep), beat.toIndex(_sMonthlyAggregates) },  // RRA_MONTHLY
        { beat.toIndex(_sYearlyStep),  beat.toIndex(_sYearlyStep),  beat.toIndex(_sYearlyAggregates)  },  // RRA_YEARLY
    };
    cServiceRrdVar var;
    cServiceVarType type;
    cParamType ptyp;

    var.setById(q, i.key());
    type.setById(q, var.getId(_sServiceVarTypeId));
    beat.setById(q, var.getId(_sRrdBeatId));
    ptyp.setById(q, type.getId(_sRawParamTypeId));
    QString min = "0";
    QString max = "2000000000";
    eFilterType pft = eFilterType(type.getId(_sPlausibilityType));
    bool        pin = type.getBool(_sPlausibilityInverse);
    if (pft == FT_INTERVAL) {
        if (!pin) {
            min = type.getName(_sPlausibilityParam1);   // !! interval !!
            max = type.getName(_sPlausibilityParam2);   // !! interval !!
        }
    }
    else {
        switch (pft) {
        case FT_BIG:    if (pin) pft = FT_LITLE;    break;
        case FT_LITLE:  if (pin) pft = FT_BIG;      break;
        default:        break;
        }
        switch (pft) {
        case FT_BIG:    min = type.getName(_sPlausibilityParam1);    break; // !! interval !!
        case FT_LITLE:  max = type.getName(_sPlausibilityParam1);    break; // !! interval !!
        default:        break;
        }
    }

    QStringList args;
    args << "create" << i.value().fileName;
    args << daemonOption;
    args << "-s" << QString::number(beat.getId(_sStep) / 1000); // step [sec]
    args << QString("DS:%1:%2:%3:%4:%5")
            .arg(i.value().varName)                 // name
            .arg(type.getName(_sServiceVarType))    // Type
            .arg(beat.getId(_sHeartbeat) / 1000)    // heatrbeat [sec]
            .arg(min, max);                         // min, max
    for (int rra = 0; rra < RRA_NUMBER; rra++) {
        QStringList averages = beat.getStringList(indexes[rra][FXX_AGREGATES]);
        foreach (QString average, averages) {
            args << QString("RRA:%1:0.5:%2:%3")
                    .arg(average)
                    .arg(beat.getId(indexes[rra][FXX_STEP]))
                    .arg(beat.getId(indexes[rra][FXX_SIZE]));
        }
    }
    QString msg;
    int r = startProcessAndWait(sRrdTool, args, &msg);
    if (r != 0) {
        APPMEMO(q, tr("I can't create the %1 RRD file\n%2").arg(i.value().fileName, msg), RS_CRITICAL);
        return false;
    }
    return true;
}
