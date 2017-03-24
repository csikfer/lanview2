#ifndef LV2LINK
#define LV2LINK
#include "lv2data.h"

/// Fizikai link típusa
enum ePhsLinkType {
    LT_INVALID = ENUM_INVALID,
    LT_FRONT   = 0,     ///< Patch oanel, fali csatlakozü előlapi/külső link
    LT_BACK,            ///< Patch oanel, fali csatlakozü játlapi/belső link
    LT_TERM             ///< Hálózati elem/végpont linkje
};

EXT_ int phsLinkType(const QString& n, enum eEx __ex = EX_ERROR);
EXT_ const QString& phsLinkType(int e, enum eEx __ex = EX_ERROR);

typedef QPair<qlonglong, ePhsLinkType> tPhsLinkPort;

/// @class cPhsLink
/// Fizikai link objektumok. Patch és fali kábeleket reprezentáló objektum.
/// Az objektumhoz tartozó adatbázis tábla a "phs_links_table" ill. a "phs_links" nézet tábla.
class LV2SHARED_EXPORT cPhsLink : public cRecord {
    CRECORD(cPhsLink);
public:
    /// @return Vigyázat a visszaadott érték értelmezése más, mint a többi replace metódusnál:
    /// Ha felvette az új rekordot, és nem kellet törölni egyet sem, akkor R_INSERT, ha törölni kellett rekordokat, akkor
    /// R_UPDATE, ha viszont nem sikerült felvenni ez új rekordot, akkor R_ERROR.
    /// A műveleteket tranzakcióba zárja, és hiba esetén R_ERROR visszagörgeti.
    /// @param __ex Ha értéke nem EX_IGNORE, akkor hiba esetén kizárást dob.
    virtual int replace(QSqlQuery &__q, enum eEx __ex = EX_ERROR);
    /// Nem támogatott, kizárást dob, ha __ex értéke nem EX_IGNORE, egyébként nem csinál semmit és false-val tér vissza.
    virtual bool rewrite(QSqlQuery &__q, enum eEx __ex = EX_ERROR);
    /// Törli a megadott fizikai linket
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __nn A host neve, amihez a link tartozik
    /// @param __pn A port neve, vagy egy minta, amihez a link trtozik
    /// @param __t  A link típusa, LT_INVALID (ez az alapérteémezés) esetén mindehyik típust töröljük,
    /// @param __pat Ha true (ez az alapérteémezés), akkor nem konkrét port név lett megadva, hanem egy minta.
    /// @return A törölt rekordok száma
    int unlink(QSqlQuery &q, const QString& __nn, const QString& __pn, ePhsLinkType __t = LT_INVALID, bool __pat = true);
    /// Törli a megadott fizikai linket
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __hn A host neve, amihez a link tartozik
    /// @param __ix A port indexe, amihez a link trtozik, vagy egy kezdő érték
    /// @param __ei Egy opcionális port index záró érték, vagy NULL_IX (alapértelmezett), ha nem egy tartományról van szó.
    /// @return A törölt rekordok száma
    int unlink(QSqlQuery &q, const QString& __nn, ePhsLinkType __t, int __ix, int __ei = NULL_IX);
    /// Törli a megadott fizikai linket
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __id A port ID, amihez a link trtozik.
    /// @return a törölt rekordok száma
    int unlink(QSqlQuery &q, qlonglong __pid, ePhsLinkType __t = LT_INVALID, ePortShare __s = ES_NC) const;
    /// Lekérdezi, hogy a megadott porthoz létezik-e fizikai link rekord.
    /// Ha a kereset link rekord létezik, akkor feltölti az objektumot a rekord alapján,
    /// ha nem, akkor az objektum tartalma nem változik.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __pid A port ID-je
    /// @param __t A link típusa
    /// @param __s Az esetleges megosztás típusa, ha nincs megadva, vagy ES_NC, akkor a link keresésekor
    ///            nincs szűrési feltétel a megosztás típusára. Több találat is lehetséges.
    /// @return False, ha nincs találat, true, ha van és betöltötte a talált (első) rekordot.
    int isExist(QSqlQuery &q, qlonglong __pid, ePhsLinkType __t, ePortShare __s = ES_NC);
    bool addIfCollision(QSqlQuery &q, tRecordList<cPhsLink>& list, qlonglong __pid, ePhsLinkType __t, ePortShare __s = ES_NC);
    int collisions(QSqlQuery& __q, tRecordList<cPhsLink> &list, qlonglong __pid, ePhsLinkType __t, ePortShare __s);
    /// Ütközö linkek törlése
    /// @param __pid Port ID
    /// @param __t a link típusa
    /// @param __s a megosztás típusa
    /// @return A törölt linkek száma
    int unxlinks(QSqlQuery& __q, qlonglong __pid, ePhsLinkType __t, ePortShare __s = ES_) const;
    /// Felcsréli a link irányát
    cPhsLink& swap();

};

/// A cLogLink és cLldpLink objektumokkal használható. Figyelem, a hivás külön nem ellenörzi!
/// A cPhsLink objektum esetén csak akkor működik helyesen, ha a kiindulási port nem patch, ha a
/// visszaadott port_id egy patch port, akkor nincs infó annak típusáról.
/// cPhsLink objektum esetén, ha a kiindulási port patch port, akkor, mivel az
/// több porttel is lehet linkbe, a hívás kizárást dobhat, nem egyértelmű eredmény miatt.
/// Megadja, hogy az ID alapján megadott port, mely másik porttal van link-be
/// @param q Az SQL lekérdezéshez használt objektum.
/// @param o Az objektum, melyhez tartozó táblában keresni szeretnénk
/// @param __pid port ID
/// @return A talált port ID-je, vagy NULL_ID
EXT_ qlonglong LinkGetLinked(QSqlQuery& q, cRecord& o, qlonglong __pid);
/// A cLogLink és cLldpLink objektumokkal használható. Figyelem, a hivás külön nem ellenörzi!
/// A cPhsLink objektum esetén csak akkor működik helyesen, ha a kiindulási port nem patch, ha a
/// visszaadott port_id egy patch port, akkor nincs infó annak típusáról.
/// cPhsLink objektum esetén, ha a kiindulási port patch port, akkor, mivel az
/// több porttel is lehet linkbe, a hívás kizárást dobhat, nem egyértelmű eredmény miatt.
/// Megadja, hogy az ID alapján megadott port, mely másik porttal van link-be
/// @param q Az SQL lekérdezéshez használt objektum.
/// @param __pid port ID
/// @return A talált port ID-je, vagy NULL_ID
template <class L> qlonglong LinkGetLinked(QSqlQuery& q, qlonglong __pid) {
    L o;
    return LinkGetLinked(q, o, __pid);
}

/// Csak a cLogLink és cLldpLink objektumokka használható. Figyelem, a hivás külön nem ellenörzi!
/// Megadja, hogy az ID alapján megadott két port, link-be van-e
/// @param q Az SQL lekérdezéshez használt objektum.
/// @param o Az objektum, melyhez tartozó táblában keresni szeretnénk
/// @param __pid1 az egyik port ID
/// @param __pid2 a másik port ID
/// @return true, ha a portok linkben vannak, ekkor a link rekord beolvasásra kerül o-ba.
EXT_ bool LinkIsLinked(QSqlQuery& q, cRecord& o, qlonglong __pid1, qlonglong __pid2);

class LV2SHARED_EXPORT cLogLink : public cRecord {
    CRECORD(cLogLink);
public:
    /// A tábla írása automatikus, az insert metódus tiltott
    virtual bool insert(QSqlQuery &, bool);
    /// A tábla írása automatikus, az insert metódus tiltott
    virtual int replace(QSqlQuery &, bool);
    /// A tábla írása automatikus, az update metódus tiltott
    virtual bool update(QSqlQuery &, bool, const QBitArray &, const QBitArray &, bool);
    /// A logikai link tábla alapján megadja, hogy a megadott id-jű port mely másik portal van összekötve
    /// Az aktuális link rekordot beolvassa, ha van találat.
    /// @param A port id, melyel linkelt portot keressük.
    /// @return A linkelt port id-je, vagy NULL_ID, ha nincs linkbe másik port a megadottal.
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }

};

class LV2SHARED_EXPORT cLldpLink : public cRecord {
    CRECORD(cLldpLink);
public:
    /// A megadott porthoz tartozó linket törli.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param o Az objektum, melyhez tartozó táblából törölni szeretnénk (tartalma érdektelen, csak a táblát azonosítja)
    /// @param __pid Port id (port_id1)
    /// @return true, ha törölt egy rekordot, false, ha nem
    bool unlink(QSqlQuery &q, qlonglong __pid);
    /// A logikai link tábla alapján megadja, hogy a megadott id-jű port mely másik portal van összekötve
    /// Az aktuális link rekordot beolvassa, ha van találat.
    /// @param A port id, melyel linkelt portot keressük.
    /// @return A linkelt port id-je, vagy NULL_ID, ha nincs linkbe másik port a megadottal.
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }
};

#endif // LV2LINK

