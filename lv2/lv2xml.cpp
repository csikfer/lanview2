#include "lanview.h"

processXml::processXml(QObject * parent)
    : QObject(parent), QDomDocument(), cmd(), content()
{
    _init();
}

processXml::processXml(QString __cmd, QObject * parent)
    : QObject(parent), QDomDocument(), cmd(), content()
{
    DBGFN();
    _init();
    start(__cmd);
    DBGFNL();
}

void processXml::_init()
{
    pe          = nullptr;
    domStatus   = EMPTY;
    exitCode    = -1;
    proc        = nullptr;
}
#define MAXWAITSEC  10
void processXml::clear()
{
    int i = 0;
    if (proc) {
        while (proc->state() == QProcess::Running) {
            proc->kill();
            proc->waitForFinished(1000);
            if (i++ >= MAXWAITSEC) EXCEPTION(ETO);
        }
        delete proc;
        proc = nullptr;
    }
    if (pe) {
        delete pe;
        pe = nullptr;
    }
    domStatus   = EMPTY;
    exitCode    = -1;
    QDomDocument::clear();
}

void processXml::start(QString __cmd)
{
    if (proc) EXCEPTION(EPROGFAIL);
    proc = new QProcess(this);
    proc->setWorkingDirectory(lanView::getInstance()->homeDir);
    connect(proc, SIGNAL(finished(int, QProcess::ExitStatus)),  this, SLOT(procFinished(int, QProcess::ExitStatus)));
    connect(proc, SIGNAL(error(QProcess::ProcessError)),        this, SLOT(procError(QProcess::ProcessError)));
    connect(proc, SIGNAL(readyReadStandardOutput()),            this, SLOT(procReadyRead()));
    proc->start(__cmd);
}

void processXml::reStart(QString __cmd)
{
    clear();
    start(__cmd);
}

processXml::~processXml()
{
    DBGFN();
    clear();
    DBGFNL();
}

void processXml::procFinished(int __exitCode, QProcess::ExitStatus exitStatus)
{
    DBGFN();
    if (pe) return;
    if (exitStatus != QProcess::NormalExit) NEWCERROR(EPROCERR, 1, cmd);
    exitCode = __exitCode;
    domStatus = PROCESS;
    procReadyRead();
    QString errStr;
    int     errLine;
    if (!setContent(content, &errStr, &errLine)) {
        pe = NEWCERROR(EXML, errLine, errStr);
        domStatus = DERROR;
    }
    else {
        domStatus = PROCESSED;
        content.clear();
    }
    DBGFNL();
}

void processXml::procError(QProcess::ProcessError error)
{
    QString msg = QString(trUtf8("Command : '%1'; error : %2")).arg(cmd).arg(ProcessError2Message(error));
    pe = NEWCERROR(EPROCERR, (int)error, msg);
    PDEB(DERROR) << "processError = " << msg << endl;
}

void processXml::procReadyRead()
{
    DBGFN();
    content += proc->readAllStandardOutput();
    DBGFNL();
}

void    processXml::waitForReady(int __to) const
{
    DBGFN();
    if (!proc->waitForFinished(__to)) EXCEPTION(ETO, eError::EXML, cmd);
    if (pe) pe->exception();
    if (domStatus == DERROR) EXCEPTION(EXML);
    DBGFNL();
}

QDomElement processXml::operator[](const QString& __ns) const
{
    QStringList names = __ns.split(QChar('.'));
    QDomElement r = documentElement();
    foreach (QString name, names) {
        QDomNode n = r.namedItem(name);
        r = n.isElement() ? n.toElement() : n.nextSiblingElement(name);
        if (r.isNull()) break;
    }
    return r;
}
