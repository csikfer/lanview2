#include "rrdhelper.h"
#include "lv2daterr.h"
#include "vardata.h"


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
    QString rrdCmd = payload.toString();
    static const QString rrd = "rrd ";
    if (rrdCmd.startsWith(rrd)) {   // Is RRD command
        cRrdHelper& o = dynamic_cast<cRrdHelper&>(selfHostService());
        o.execRrd(rrdCmd);
    }
    else {
        lanView::dbNotif(name, source, payload);
    }
}


/******************************************************************************/

cRrdHelper::cRrdHelper(QSqlQuery& q, const QString& __sn)
    : cInspector(q, __sn)
{
    ;
}

cRrdHelper::~cRrdHelper()
{
    ;
}


void cRrdHelper::postInit(QSqlQuery &q, const QString &qs)
{
    (void)qs;
    static const QString _sBaseDir = "base_dir";
    QString sBaseDir = feature(_sBaseDir);
    if (sBaseDir.isEmpty()) {
        sBaseDir = cSysParam::getTextSysParam(q, _sBaseDir, lanView::getInstance()->homeDir + "/rrd");
    }
    baseDir.setPath(sBaseDir);
    if (!baseDir.exists()) {
        if (!baseDir.mkdir(".")) {
            EXCEPTION(ENODIR, 0, baseDir.path());
        }
    }
    if (!QFileInfo(baseDir.path()).isWritable()) {
        EXCEPTION(EFOPEN, 0, baseDir.path());
    }
}

enum ixRRDPar { RP_HELPER_ID_IX, RP_VALUE_IX, RP_SERVICE_VAR_ID_IX };
void cRrdHelper::execRrd(const QString& rrdCmd)
{
    bool ok1, ok2;
    QSqlQuery q = getQuery();
    QStringList sl = rrdCmd.split(QChar(' '), QString::SkipEmptyParts);
    sl.pop_front(); // drop "rrd".
    qlonglong helperId = sl.at(RP_HELPER_ID_IX).toLongLong(&ok1);
    qlonglong serviceVarId  = sl.at(RP_SERVICE_VAR_ID_IX).toLongLong(&ok2);
    if (!ok1 || !ok2) {
        APPMEMO(q, tr("Invalid RRD notify (non numeric ID) : '%1'").arg(rrdCmd), RS_CRITICAL);
        return;
    }
    if (helperId != hostServiceId()) return;    // It wasn't for us
    QString rrdFileName;
    QMap<qlonglong, QString>::iterator i = rrdFilePathMap.find(serviceVarId);
    if (i != rrdFilePathMap.end()) {
        rrdFileName = i.value();
        if (rrdFileName.isEmpty()) {
            // Not found, reported
            return;
        }
    }
    else {
        i = rrdFilePathMap.insert(serviceVarId, _sNul);
        static const QString sql =
                "SELECT node_name, service_name, service_var_name "
                " FROM service_rrd_vars"
                " JOIN services USING(service_id)"
                " JOIN nodes USING(node_id)"
                " WHERE service_var_id = ?";
        if (execSql(q, sql, serviceVarId)) {
            APPMEMO(q, tr("The service_rrd_vars record not found (ID : %2) : '%1'").arg(rrdCmd).arg(serviceVarId), RS_CRITICAL);
            return;
        }
        QString node    = q.value(0).toString();
        QString service = q.value(1).toString();
        QString var     = q.value(2).toString();
        QDir dir = baseDir;
        // check <base_dir>/<node>
        if (!dir.exists(node)) {
            if (!dir.mkdir(node)) {
                APPMEMO(q, tr("I can't create the %2 (node) folder : '%1'").arg(rrdCmd, dir.filePath(node)), RS_CRITICAL);
                return;
            }
        }
        dir.cd(node);
        // check <base_dir>/<node>/<service>
        if (!dir.exists(service)) {
            if (!dir.mkdir(service)) {
                APPMEMO(q, tr("I can't create the %2 (service) folder : '%1'").arg(rrdCmd, dir.filePath(service)), RS_CRITICAL);
                return;
            }
        }
        dir.cd(service);
        if (!dir.exists(var)) {
            if (!createRrdFile(q, serviceVarId, dir.filePath(var))) {
                APPMEMO(q, tr("I can't create the %2 RRD file : '%1'").arg(rrdCmd, dir.filePath(var)), RS_CRITICAL);
                return;
            }
        }
        i.value() = dir.filePath(var);
    }
}

bool cRrdHelper::createRrdFile(QSqlQuery& q, qlonglong _id, const QString& _fn)
{
    cServiceRrdVar var;
    cServiceVarType type;
    cRrdBeat beat;
    cParamType ptyp;

    var.setById(q, _id);
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
    args << "create" << _fn;
    args << "-s" << QString::number(beat.getId(_sStep) / 1000); // step [sec]
    args << QString("DS:v:%1:%2:%3:%4")
            .arg(type.getName(_sServiceVarType))    // Type
            .arg(beat.getId(_sHeartbeat) / 1000)    // heatrbeat [sec]
            .arg(min, max);                         // min, max



    return true;
}

