#include <math.h>
#include "import.h"
#include "others.h"
#include "ping.h"
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
    PDEB(INFO) << "Start parser ..." << endl;
    initImportParser();
    int r = importParse();
    downImportParser();
    PDEB(INFO) << "End parser." << endl;
    pDelete(importInputStream);
    return r;
}

int importParseFile(const QString& fn)
{
    QFile in(fn);
    importFileNm = fn;
    if (fn == _sMinus || fn == _sStdin) {
        importFileNm = _sStdin;
        importInputStream = new QTextStream(stdin, QIODevice::ReadOnly);
    }
    else {
        if (!importSrcOpen(in)) EXCEPTION(EFOPEN, -1, in.fileName());
        importInputStream = new QTextStream(&in);
        importFileNm = in.fileName();
    }
    PDEB(INFO) << "Start parser ..." << endl;
    initImportParser();
    int r = importParse();
    downImportParser();
    PDEB(INFO) << "End parser." << endl;
    pDelete(importInputStream);
    return r;
}

