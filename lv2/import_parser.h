#ifndef IMPORT_PARSER_H
#define IMPORT_PARSER_H

#include "lv2data.h"
#include "lv2user.h"

enum eImportParserStat {
    IPS_READY,
    IPS_RUN,
    IPS_THREAD
};

EXT_ enum eImportParserStat importParserStat;

EXT_ QString importFileNm;               ///< Source file name
EXT_ unsigned int importLineNo;          ///< Source line counter
EXT_ QTextStream* pImportInputStream;     ///< Source stream
EXT_ cError *pImportLastError;            ///< Last error object, or NULL pointer
EXT_ cError *importGetLastError();
/// Parse string
/// Hiba esetén a importLastError pointer a hiba okát leíró objekumra mutat
/// @param text A feldolgozandó string
/// @return Ha nem volt hiba akkor 0, egyébként a hiba kód.
EXT_ int importParseText(QString text);
/// Parse file
/// Hiba esetén a importLastError pointer a hiba okát leíró objekumra mutat
/// @param fn A feldolgozandó fájl neve lsd.: bool importSrcOpen(QFile& f);
/// @return Ha nem volt hiba akkor 0, egyébként a hiba kód.
EXT_ int importParseFile(const QString& fn);
///
EXT_ int importParse(eImportParserStat _st = IPS_RUN);

/// Megnyitja olvasásra a megadott nevű forrás fájlt.
/// Elöszőr az aktuális könyvtárban, ill. ha a fájl nem létezik, akkor a home könyvtárban próbállja megnyitni.
/// @return true, ha sikerült a fájlt megnyitni.
EXT_ bool importSrcOpen(QFile& f);
/// Az import parse híváshoz alaphelyzezbe állítja a parser globális változóit.
/// (Nem azokat a globális pointereket, melyet a void initImportParser(); már felszabadított.)
EXT_ void initImportParser();
/// A parser hívás után az esetleg (pl. hiba esetén) fel nem szabadított pointereket felszabadítja, ill nullázza.
EXT_ void downImportParser();
/// A hívást követően, ha a parser beolvasna egy új sort, megszakítja a feldolgozást egy EBREAK hiba kóddal.
EXT_ void breakImportParser();
/// Visszaadja a parszert megszakító flag értékét ld.: void breakImportParser()
/// @param _except Ha az opcionális paraméter értéke true, akkor törli a flag-et, és dob egy kizárást EBREAK hibaküddal.
EXT_ bool isBreakImportParser(bool _except = false);

class LV2SHARED_EXPORT cImportParseThread : public QThread {
    friend QString yygetline();
public:
    cImportParseThread(const QString &_inicmd = QString(), QObject *par = NULL);
    ~cImportParseThread();

    int push(const QString& srv, cError *&pe);
    int startParser(cError *&pe, QString *_pSrc = NULL);
    int reStartParser(cError *&pe);
    void stopParser();
protected:
    virtual void	run();
    QString pop();
private:
    void reset();
    QSemaphore      queueAccess;    ///< Szemefor a queue eléréshez
    QSemaphore      dataReady;      ///< Szemafor: Van adat a queue-ban
    QSemaphore      parseReady;     ///< Szemafor: A parser szabad (és nincs adat)
    QQueue<QString> queue;
    QString         iniCmd;
    QString        *pSrc;
    static cImportParseThread *pInstance;
public:
    static cImportParseThread& instance()       { if (pInstance == NULL) EXCEPTION(EPROGFAIL); return *pInstance; }
};

#endif // IMPORT_PARSER_H
