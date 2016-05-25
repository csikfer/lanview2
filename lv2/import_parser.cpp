#include <math.h>
#include "lanview.h"
#include "others.h"
#include "ping.h"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  PARSER
#include "import_parser.h"

QString      importFileNm;
unsigned int importLineNo = 0;
QTextStream* pImportInputStream = NULL;
enum eImportParserStat importParserStat = IPS_READY;

bool importSrcOpen(QFile& f)
{
    if (f.exists()) return f.open(QIODevice::ReadOnly);
    QString fn = f.fileName();
    QDir    d(lanView::getInstance()->homeDir);
    if (d.exists(fn)) {
        f.setFileName(d.filePath(fn));
        return f.open(QIODevice::ReadOnly);
    }
    EXCEPTION(ENOTFILE, -1, fn);
    return false;   // A fordító szerint ez kell
}

int importParseText(QString text)
{
    importFileNm = "[stream]";
    pImportInputStream = new QTextStream(&text);
    PDEB(INFO) << QObject::trUtf8("Start parser ...") << endl;
    initImportParser();
    int r = importParse();
    downImportParser();
    PDEB(INFO) << QObject::trUtf8("End parser.") << endl;
    pDelete(pImportInputStream);
    return r;
}

int importParseFile(const QString& fn)
{
    QFile in(fn);
    importFileNm = fn;
    if (fn == QChar('-') || fn == _sStdin) {
        importFileNm = _sStdin;
        pImportInputStream = new QTextStream(stdin, QIODevice::ReadOnly);
    }
    else {
        if (!importSrcOpen(in)) EXCEPTION(EFOPEN, -1, in.fileName());
        pImportInputStream = new QTextStream(&in);
        importFileNm = in.fileName();
    }
    PDEB(INFO) << QObject::trUtf8("Start parser ...") << endl;
    initImportParser();
    int r = importParse();
    downImportParser();
    PDEB(INFO) << QObject::trUtf8("End parser.") << endl;
    pDelete(pImportInputStream);
    return r;
}

cError *importGetLastError() {
    cError *r = pImportLastError;
    pImportLastError = NULL;
    return r;
}

cImportParseThread *cImportParseThread::pInstance = NULL;

cImportParseThread::cImportParseThread(const QString& _inicmd, QObject *par)
    : QThread(par)
    , queueAccess(1)    // A queue szabad
    , dataReady(0)      // A Queue üres
    , parseReady(0)     // A parser szabad, de nincs adat
    , queue()
    , iniCmd(_inicmd)
{
    DBGOBJ();
    pSrc = NULL;
    if (pInstance != NULL) EXCEPTION(EPROGFAIL);
    pInstance = this;
    setObjectName("Import parser");
}

#define IPT_SHORT_WAIT  5000
#define IPT_LONG_WAIT  15000
cImportParseThread::~cImportParseThread()
{
    DBGOBJ();
    stopParser();
    pInstance = NULL;
}

void cImportParseThread::reset()
{
    DBGFN();
    queue.clear();
    if (0 != dataReady.available()) {
        if (!dataReady.tryAcquire(dataReady.available())) EXCEPTION(ESEM);
    }
    switch (queueAccess.available()) {
    case 1:                          break;
    case 0: queueAccess.release();   break;
    default: EXCEPTION(ESEM);
    }
    if (0 != parseReady.available()) {
        if (!parseReady.tryAcquire(parseReady.available())) EXCEPTION(ESEM);
    }
}

void	cImportParseThread::run()
{
    DBGFN();
    // Alaphelyzetbe állítjuk a sorpuffert, és a szemforokat
    reset();
    importFileNm = "[queue]";
    if (importParserStat != IPS_READY || pImportInputStream != NULL) {
        if (pImportLastError == NULL) pImportLastError = NEWCERROR(ESTAT);
        DERR() << VDEB(importParserStat) << _sCommaSp << VDEBPTR(pImportInputStream) << endl;
        return;
    }
    PDEB(INFO) << QObject::trUtf8("Start parser (thread) ...") << endl;
    initImportParser();
    if (pSrc == NULL) {     // Fordítás a queue-n keresztül
        importParse(IPS_THREAD);
        if (parseReady.available() == 0) parseReady.release();
    }
    else {
        importFileNm = "[stream]";
        pImportInputStream = new QTextStream(pSrc);
        PDEB(INFO) << QObject::trUtf8("Start parsing ...") << endl;
        importParse();
        PDEB(INFO) << QObject::trUtf8("End parse.") << endl;
    }
    downImportParser();
    PDEB(INFO) << QObject::trUtf8("End parser ((thread)).") << endl;
    pDelete(pImportInputStream);
    DBGFNL();
}

int cImportParseThread::push(const QString& src, cError *& pe)
{
    _DBGFN() << src << endl;
    int r;
    if (isRunning()) {
        int i = parseReady.available();
        if (i) parseReady.tryAcquire(i);    // Nyit, ha kész a parser
        queueAccess.acquire();              // puffer hazzáférés foglalt
        queue.enqueue(src);                 // küld
        dataReady.release();                // pufferben adat
        queueAccess.release();              // puffer hazzáférés szabad
        if (parseReady.tryAcquire(1, IPT_SHORT_WAIT)) {    // Megváruk, hogy végezzen a parser
            pe = importGetLastError();
            r = pe == NULL ? REASON_OK : R_ERROR;
        }
        else {
            pe = importGetLastError();                      // Időtullépés
            if (NULL == pe) pe = NEWCERROR(ETO);
            r = REASON_TO;
        }
    }
    else {
        pe = importGetLastError();
        if (pe == NULL) pe = NEWCERROR(EUNKNOWN, -1, trUtf8("A parser szál nem fut."));
        r = R_DISCARD;
    }
    if (REASON_OK != r) {
        cError *rpe = NULL;
        reStartParser(rpe);
        if (rpe != NULL) {
            DERR() << rpe->msg() << endl;
            delete rpe;
        }
    }
    if (pe == NULL) {
        DBGFNL();
    }
    else {
        _DBGFNL() << "ÉAST ERROR : " << pe->msg() << endl;
    }
    return r;
}

QString cImportParseThread::pop()
{
    QString r;
    if (parseReady.available() == 0) parseReady.release();
    dataReady.acquire();
    queueAccess.acquire();
    if (!queue.isEmpty()) r = queue.dequeue();
    queueAccess.release();
    return r;
}

int cImportParseThread::startParser(cError *&pe, QString *_pSrc)
{
    DBGFN();
    pSrc = _pSrc;
    int r;
    start();
    if (iniCmd.isEmpty()) {
        pe = NULL;
        r = REASON_OK;
    }
    else {
        r = push(iniCmd, pe);
    }
    DBGFNL();
    return r;
}

int cImportParseThread::reStartParser(cError *&pe)
{
    stopParser();
    return startParser(pe);
}

void cImportParseThread::stopParser()
{
    DBGFN();
    if (isRunning()) {
        if (!queueAccess.tryAcquire(1, IPT_SHORT_WAIT)) {
            DERR() << "Ignore blocked access semaphor." << endl;
        }
        queue.clear();
        queueAccess.release();
        dataReady.release();
        if (!wait(IPT_LONG_WAIT)) {
            DERR() << "Import parser thread, nothing exited." << endl;
            terminate();
            if (!wait(IPT_LONG_WAIT)) {
                DERR() << "Import parser thread, nothing terminated." << endl;
            }
            downImportParser();
        }
    }
    DBGFNL();
}
