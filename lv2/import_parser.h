#ifndef IMPORT_PARSER_H
#define IMPORT_PARSER_H

#include "lv2data.h"
#include "lv2user.h"

extern QString importFileNm;               ///< Source file name
extern unsigned int importLineNo;          ///< Source line counter
extern QTextStream* importInputStream;     ///< Source stream
extern cError *importLastError;            ///< Last error object, or NULL pointer

/// Parse string
/// Hiba esetén a importLastError pointer a hiba okát leíró objekumra mutat
/// @param text A feldolgozandó string
/// @return Ha nem volt hiba akkor 0, egyébként a hiba kód.
extern int importParseText(QString text);
/// Parse file
/// Hiba esetén a importLastError pointer a hiba okát leíró objekumra mutat
/// @param fn A feldolgozandó fájl neve lsd.: bool importSrcOpen(QFile& f);
/// @return Ha nem volt hiba akkor 0, egyébként a hiba kód.
extern int importParseFile(const QString& fn);
///
extern int importParse();

/// Megnyitja olvasásra a megadott nevű forrás fájlt.
/// Elöszőr az aktuális könyvtárban, ill. ha a fájl nem létezik, akkor a home könyvtárban próbállja megnyitni.
/// @return true, ha sikerült a fájlt megnyitni.
extern bool importSrcOpen(QFile& f);
/// Az import parse híváshoz alaphelyzezbe állítja a parser globális változóit.
/// (Nem azokat a globális pointereket, melyet a void initImportParser(); már felszabadított.)
extern void initImportParser();
/// A parser hívás után az esetleg (pl. hiba esetén) fel nem szabadított pointereket felszabadítja, ill nullázza.
extern void downImportParser();


#endif // IMPORT_PARSER_H
