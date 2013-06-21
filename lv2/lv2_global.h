#ifndef LV2_GLOBAL_H
#define LV2_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LV2_LIBRARY)
#  define LV2SHARED_EXPORT Q_DECL_EXPORT
// #  warning "LIB..."
#else
#  define LV2SHARED_EXPORT Q_DECL_IMPORT
// #  warning "APP..."
#endif

#define EXT_ extern LV2SHARED_EXPORT

/// @def NULL_ID
/// NULL ID-t reprezentáló konstans.
#define NULL_ID     LLONG_MIN
/// @def NULL_IX
/// NULL index-et-t reprezentáló konstans.
#define NULL_IX     INT_MIN


#if   defined(Q_CC_MSVC)

#elif defined(Q_CC_GNU)

#else
#error "Nem támogatott C++ fordító használata."
#endif

/*!
@mainpage Indalarm API
@author Csiki Ferenc
@section lanview Inicializálás, a lanView osztály
Az API-nak hasonlóan a Qt-ben a QApplication osztályhoz van egy „globálisan” használt objektuma,
amit egy példányban mindig létre kell hozni. A példány létrehozásakor inicializáljuk az API-t,
és ez a példány teszi lehetővé pl. az adatbázis elérését.
Bár a lanview osztály példányosítható, de praktikusan egy leszármazottját használjuk mely
tartalmazhatja a program inicializálását is.
A lanView osztály példányosításakor a következő műveleteket hajta végre:
- Betölti a nyelvi fájlokat a LANG környezeti változó alapján, ha vannak.
- Betölti a config állományt a Qsettings pédányosításával.
- Inicializálja a debug rendszert (a cDebug példányosítása)
- Értelmezi a program paramétereket (ami rá vonatkozik):
- Unix típusú operációs rendszer esetén inicializálja a sygnal-ok fogadását.
- Megnyitja az adatbázist, ha a kértük (lásd később).
- Inicializálja az SNMP alrendszert, ha kértük (ez Windows alatt hiányzik az API-ból).

@section cError Hiba kezelés, a cError osztály.
Az API hiba esetén egy a hiba paramétereivel létrehozott cError objektum pionterével hibát dob.

@section cDebug Nyomkövetés, a cDebug osztály
A cDebug osztállyal közvetlenül nem találkozunk, azt a lanView osztály inicializálja ill. példányosítja.
Használata pedig makrókon keresztül történik.
Minden a cDebug-on keresztül kiírt üzenetet a cDebug egy fejléccel lát el, mely hasonló a syslog üzenetekhez,
továbbá ha a program több szálú, akkor a fejléc tartalmazza az üzenetet küldő szál nevét is.
A cDebug lehetőséget biztosit GUI program esetén az üzenetek elfogására a signal-on keresztül, ha azt a GUI-ban meg akarjuk jeleníteni.
A cDebug az üzeneteket mindig a fő szálban írja ki, a mellék szálak üzeneteit egy QQueue soron keresztül kapja meg.
Minden üzenet csak akkor kerül kiírásra ill a sorba, ha lezártuk az endl manipulátorral.

@section detemanip Adatkezelés
Az API fel van készítve a több szálú működésre, beleértve az adatbázis műveleteket is.
Az adatbázis eléréséhez a newQuery() vagy a getQuery() függvényekkel kérhetünk egy QSqlQuery objektumot.
Az igy kapott objektumnál arra kell vigyázni, hogy abban a szálban kell hívni az előző függvényeket, amelyikben
használni is fogjuk a QSqlQuery objektumot.

@subsection cRecord A cRecord bázis osztály
A cRecord egy összetett bázis osztály, amely egy sablont biztosít az adatbázis adattáblái és a C++ objektumok közötti megfeleltetéshez.
Az osztály által biztosított egy-egy megfeleltetésnek természetesen csak egyes tábla típusoknál van értelme, tipikusan azoknál,
melyek maguk is objektum jellegűek, pl. konkrét tárgyakat reprezentálnak.
A cRekord-ban a legtöbb művelet egy tisztán virtuális függvényen keresztül valósul meg, mely egy leíró objektum referenciáját adja vissza:
@code
virtual const cRecStaticDescr&  descr() const = 0;
@endcode

@subsection  cRecStaticDescr A cRecStaticDescr leíró objektum
Az adatkezelést vezérlő a táblatulajdonságait tartalmazó, ill. az e szeríti működést lehetővé tevő osztály.
*/

#endif // LV2_GLOBAL_H
