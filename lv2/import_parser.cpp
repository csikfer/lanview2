#include <math.h>
#include "lanview.h"
#include "others.h"
#include "ping.h"
#undef  __MODUL_NAME__
#define __MODUL_NAME__  PARSER
#include "import_parser.h"

QString      importFileNm;
unsigned int importLineNo = 0;
QTextStream* importInputStream = NULL;

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
    importInputStream = new QTextStream(&text);
    PDEB(INFO) << QObject::trUtf8("Start parser ...") << endl;
    initImportParser();
    int r = importParse();
    downImportParser();
    PDEB(INFO) << QObject::trUtf8("End parser.") << endl;
    pDelete(importInputStream);
    return r;
}

int importParseFile(const QString& fn)
{
    QFile in(fn);
    importFileNm = fn;
    if (fn == QChar('-') || fn == _sStdin) {
        importFileNm = _sStdin;
        importInputStream = new QTextStream(stdin, QIODevice::ReadOnly);
    }
    else {
        if (!importSrcOpen(in)) EXCEPTION(EFOPEN, -1, in.fileName());
        importInputStream = new QTextStream(&in);
        importFileNm = in.fileName();
    }
    PDEB(INFO) << QObject::trUtf8("Start parser ...") << endl;
    initImportParser();
    int r = importParse();
    downImportParser();
    PDEB(INFO) << QObject::trUtf8("End parser.") << endl;
    pDelete(importInputStream);
    return r;
}

cError *importGetLastError() {
    cError *r = importLastError;
    importLastError = NULL;
    return r;
}
