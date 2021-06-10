#ifndef IMPORT_PARSER_H
#define IMPORT_PARSER_H

#include "lv2data.h"
#include "lv2user.h"
#include "srvdata.h"

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
EXT_ bool isBreakImportParser(bool __ex = false);

#define IPT_SHORT_WAIT  5000
#define IPT_LONG_WAIT  15000

/// @class cImportParseThread
/// Egy külön szálban indított ill. indítható input  parser objektum.
/// A parser csak egy példányban és csak egy szálon futatható.
/// Az objektum is csak egy példányban hozható létre.
class LV2SHARED_EXPORT cImportParseThread : public QThread {
    friend QString yygetline();
public:
    /// Létrehozza az input parser szálat. Nem indítja a parsert.
    /// @param _inicmd Egy végrehajtandó opcionális parancs a parser indításakor.
    /// Az itt megadott parancsot minden (újra)indításkor végrehajtja.
    /// @param par Parent
    cImportParseThread(const QString &_inicmd = QString(), QObject *par = nullptr);
    ~cImportParseThread();
    /// A parser szál indítása. Végrehajtja a konstruktorban megadott parancsot.
    /// Ezután a _pSrc értékétől függően, vagy végrehalytja az ott megadot parancsot, és leáll,
    /// vagy várakozik a további parancsokra a push() metóduson keresztül.
    /// @param pe Hiba pointer. Hiba esetén itt adja vissza a hiba leírását.
    /// @param _pSrc Opcionális, ha megadjuk, akkor a parser végrehajtja, és leáll. Ha nem, akkor vár a parancsokra.
    /// @return redmény: R_IN_PROGRESS vagy a hiba kód (eReason).
    int startParser(cError *&pe, const QString *pSrc = nullptr);
    int reStartParser(cError *&pe);
    void stopParser();
    int push(const QString& srv, cError *&pe, int _to = IPT_SHORT_WAIT);
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
    QString         src;
    enum eSrcType   { ST_SRING, ST_QUEUE } srcType;

    static cImportParseThread *pInstance;
public:
    static cImportParseThread& instance()       { if (pInstance == nullptr) EXCEPTION(EPROGFAIL); return *pInstance; }
};

#endif // IMPORT_PARSER_H
