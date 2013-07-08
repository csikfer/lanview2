#ifndef IMPORT_PARSER_H
#define IMPORT_PARSER_H

#include "lv2data.h"
#include "lv2user.h"

extern QString importFileNm;
extern unsigned int importLineNo;          // Sor Számláló
extern QTextStream* importInputStream;     // Source stream
extern cError *importLastError;

extern int importParseText(QString text);
extern int importParseFile(const QString& fn);
extern int importParse();

extern bool importSrcOpen(QFile& f);
extern void initImportParser();
extern void downImportParser();

#endif // IMPORT_PARSER_H
