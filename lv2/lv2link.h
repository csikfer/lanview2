#ifndef LV2LINK
#define LV2LINK
#include "lv2data.h"

/// Fizikai link típusa
enum ePhsLinkType {
    LT_INVALID = ENUM_INVALID,
    LT_FRONT   = 0,     ///< Patch panel, fali csatlakozü előlapi/külső link
    LT_BACK,            ///< Patch panel, fali csatlakozü játlapi/belső link
    LT_TERM             ///< Hálózati elem/végpont linkje
};

EXT_ int phsLinkType(const QString& n, enum eEx __ex = EX_ERROR);
EXT_ const QString& phsLinkType(int e, enum eEx __ex = EX_ERROR);

// typedef QPair<qlonglong, ePhsLinkType> tPhsLinkPort;

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
    /// @return A találatok száma.
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
    /// Az objektumot stringgé konvertálja, ami pl. egy riportban szerepelhet.
    /// Ha t igaz (alapértelmezetten hamis), akkor kiír egy objetum megnevezést is.
    virtual QString show(bool t = false) const;
    /// A patch porthoz tartozó, vagy köveetkező link
    /// @param pid Patch port ID
    /// @param type Link típusa (csak LT_BACK vagy LT_FRONT lehet, különben kizárást dob)
    /// @param Megosztás típusa
    bool nextLink(QSqlQuery &q, qlonglong pid, enum ePhsLinkType type, enum ePortShare sh);
    ///
    bool compare(const cPhsLink& _o, bool _swap = false) const;
private:
    QString show12(QSqlQuery &q, bool _12) const;
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

/// @class cLogLink
/// A logikai linkek tábláját (ill. a hozzátartozó szimmetrikus viewt) kezelő objektum.
/// A tábla csak olvasható. Azt az adatbázis logika folyamatosan aktualizálja, ha
/// a fizikai linkek változnak. A logikai link a végpontok közti kapcsolatot írja le,
/// végighaladva a fizikai linkek láncán végponttól végpontig.
class LV2SHARED_EXPORT cLogLink : public cRecord {
    CRECORD(cLogLink);
public:
    /// A tábla írása automatikus, az insert metódus tiltott, kizárást dob.
    virtual bool insert(QSqlQuery &, bool);
    /// A tábla írása automatikus, az insert metódus tiltott, kizárást dob.
    virtual int replace(QSqlQuery &, bool);
    /// A tábla írása automatikus, az update metódus tiltott, kizárást dob.
    virtual bool update(QSqlQuery &, bool, const QBitArray &, const QBitArray &, bool);
    /// A logikai link tábla alapján megadja, hogy a megadott id-jű port mely másik portal van összekötve
    /// Az aktuális link rekordot beolvassa, ha van találat.
    /// @param A port id, melyel linkelt portot keressük.
    /// @return A linkelt port id-je, vagy NULL_ID, ha nincs linkbe másik port a megadottal.
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    /// A logikai link tábla alapján megadja, hogy a két port linkelve ven-e.
    /// Ha igen, akkor beolvassa a link rekordot.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __pid1 Az egyik port ID-je (port_id1)
    /// @param __pid1 Az másik port ID-je (port_id2)
    /// @return ha a két port linkelve van, akkor true, egyébként false
    /// @exception Ha megadott portokra két találat van a táblában (ami elvileg lehetetlen).
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }
    virtual QString show(bool t = false) const;
    QStringList showChain() const;
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
    /// Az LLDP-vel federített link tábla alapján megadja, hogy a megadott id-jű port mely másik portal van összekötve
    /// Az aktuális link rekordot beolvassa, ha van találat.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param A port id, melyel linkelt portot keressük.
    /// @return A linkelt port id-je, vagy NULL_ID, ha nincs linkbe másik port a megadottal.
    qlonglong getLinked(QSqlQuery& q, qlonglong __pid) { return LinkGetLinked(q, *this, __pid); }
    /// Az LLDP-vel federített link tábla alapján megadja, hogy a két port linkelve ven-e.
    /// Ha igen, akkor beolvassa a link rekordot.
    /// @param q Az SQL lekérdezéshez használt objektum.
    /// @param __pid1 Az egyik port ID-je (port_id1)
    /// @param __pid1 Az másik port ID-je (port_id2)
    /// @return ha a két port linkelve van, akkor true, egyébként false
    /// @exception Ha megadott portokra két találat van a táblában (ami elvileg lehetetlen).
    bool isLinked(QSqlQuery& q, qlonglong __pid1, qlonglong __pid2) { return LinkIsLinked(q, *this, __pid1, __pid2); }
    virtual QString show(bool t = false) const;
};

enum eLinkResult {
    LINK_NOT_FOUND = 0, ///< Nincs se logikai se LLDP link
    LINK_LLDP_ONLY,     ///< Csak LLDP link van, logikai nincs
    LINK_LOGIC_ONLY,    ///< Csak logikai link van, LLDP nincs
    LINK_LLDP_AND_LOGIC,///< Van LLDP és Logikai link, és egyeznek
    LINK_CONFLICT,      ///< Van LLDP és Logikai link, de nem egyeznek
    LINK_RCONFLICT      ///< Csak az egyik Link van meg, de az ellenoldalon van (ütköző) másik típusú link.
};

/// A linkelt port keresése.
/// @param pid A port ID, amihez a linkelt portot keressük. LLDP és vagy logikai.
/// @param lpid Ha van hihető link, akkor a port id-adja vissza a referencia változóban, egyébként a NULL_ID-t.
/// @param _pLldp Ha megadjuk, akkor az LLDP link objektum ebbe lesz beolvasva. Ha a visszaadott érték LINK_RCONFLICT, akkor
///               a mutatott objektum az ütköző ellenoldali linket fogja tartalmazni.
/// @param _pLogl Ha megadjuk, akkor az logikai link objektum ebbe lesz beolvasva. Ha a visszaadott érték LINK_RCONFLICT, akkor
///               a mutatott objektum az ütköző ellenoldali linket fogja tartalmazni.
/// @return A keresés eredménye.
EXT_ eLinkResult getLinkedPort(QSqlQuery& q, qlonglong pid, qlonglong& lpid, cLldpLink *_pLldp = nullptr, cLogLink *_pLogl = nullptr);

EXT_ QString reportComparedLinks(QSqlQuery& q, eLinkResult r, qlonglong pid, const cLldpLink& lldp, const cLogLink& logl, bool details = false);

/// A linkelt port keresése.
/// @param pid A port ID, amihez a linkelt portot keressük. LLDP és vagy logikai.
/// @param lpid Ha van hihető link, akkor a port id-adja vissza a referencia változóban, egyébként a NULL_ID-t.
/// @param msg A keresés eredménye szövegesen.
/// @param details Ha igaz akkor a részleteket is kiírja (konkrét link(ek)), ha hamis, akkor csak hiba esetén.
/// @return A keresés eredménye.
EXT_ eLinkResult getLinkedPort(QSqlQuery& q, qlonglong pid, qlonglong& lpid, QString& msg, bool details = false);

#endif // LV2LINK

