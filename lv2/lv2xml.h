#ifndef LV2XML_H
#define LV2XML_H

#include "lv2_global.h"
#include <QtCore>
#include <QtXml>
#include "cdebug.h"
#include "cerror.h"

class LV2SHARED_EXPORT processXml : public QObject, public QDomDocument {
    Q_OBJECT
public:
    /// Konstruktor
    processXml(QObject *parent = NULL);
    /// Konstruktor.
    /// létrehoz egy QProcess objektumot, és elindítja a __cmd parancsal.
    /// Csatlakoztat 3 slotot a Qprocess objektumra (finish, error, readyRead).
    /// A program kimenetet egy belső QByteArray változóba gyüjti, ha megkapta a finish
    /// szignált, akkor a teljes program kimenetre hívja a QDomDocument:setContent
    /// mentódusát, ha ez true-val tér vissza, akkor törli a belső változót.
    /// @param __cmd    A végrehajtandó parancs, és paraméterei
    /// @param parent   Szülő objektum
    processXml(QString __cmd, QObject *parent = NULL);
    /// destruktor
    /// @throw cError* Ha a parancs még fut, és nem sikerül leállítani (időkorlát = 1 sec)
    virtual ~processXml();
    /// Alaphelyzetbe állítja az objektumot, mint az üres konstruktor után.
    void clear();
    /// A metódus csak az üres konstruktor, ill. a clear metódus után hivható.
    /// Létrehoz egy QProcess objektumot, és elindítja a __cmd parancsal.
    /// Csatlakoztat 3 slotot a Qprocess objektumra (finish, error, readyRead).
    /// A program kimenetet egy belső QByteArray változóba gyüjti, ha megkapta a finish
    /// szignált, akkor a teljes program kimenetre hívja a QDomDocument:setContent
    /// mentódusát, ha ez true-val tér vissza, akkor törli a belső változót.
    /// @param __cmd    A végrehajtandó parancs, és paraméterei
    /// @exception *cError Ha nem az öres konstruktor, vagy a clear metódus után hívtuk.
    void start(QString __cmd);
    /// Hasonló a start() metódushoz, de elötte hívja a clear() metódust.
    void reStart(QString __cmd);
    /// Várakozik a parancs terminálására.
    /// @throw cError* Ha hibát észlel, vagy a program nem tér vissza a megadott időn bellül.
    /// @param __to Időkorlát mSec-ben.
    void waitForReady(int __to) const;
    /// A DOM dokumentum állapota
    enum eDomStat {
        EMPTY,      ///< A DomDocument üres, a program még fut
        PROCESS,    ///< A program lefutott, a kiment feldolgozása folyik
        PROCESSED,  ///< A program kimenete feldolgozva, a QDomDocument feltöltve
        DERROR       ///< Valamilyen hiba történt
    };
    /// Állapotinformáció lekérdezése
    /// @return az aktuálus állapot
    enum eDomStat getStat(void) const { return domStatus; }
    /// A program visszatérési értékének a lekérdezése.
    /// @return A program visszatérési értéke, vagy -1, ha a program mnég fut.
    int  getExitCode() const { return exitCode; }
    /// A program kimenetének belső pufferének a lekérdezése.
    /// A puffer üres, ha a feldolgozása eredményes volt.
    /// @return a buffer tartalma
    QByteArray  getContent() const { return content; }
    /// A QDomDocument objektum root elemének a lekérése
    QDomElement root(void) const { return QDomDocument::documentElement(); }
    /// A QDomDocument objektum egy elemének ill elemek részfájának a lekérdezése név alapján-
    /// @param __names Az elem neve, ill. a fán való haladásnál a nevek sorozata, elválasztó karakter a '.'.
    QDomElement operator[]( const QString& __names)  const;
private:
    void _init();
    QString     cmd;
    QByteArray  content;
    QProcess   *proc;
    cError     *pe;
    enum eDomStat domStatus;
    int  exitCode;
    QObject *parent;
private slots:
    void procFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void procError(QProcess::ProcessError error);
    void procReadyRead();
};

#endif // LV2XML_H
