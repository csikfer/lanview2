/*!
@file doxydoc.h
@author Csiki Ferenc <csikfer@gmail.com>
@brief Nem igazi fejállomány. Az API dokumentáció része, csak megjegyzéseket tartalmaz.
*/

/*!
@mainpage LanView2 API
@author Csiki Ferenc <csikfer@gmail.com>
@section A LanView API Inicializálása, a lanView osztály
Az API-nak hasonlóan a Qt-ben a QApplication osztályhoz van egy „globálisan” használt objektuma,
amit egy példányban mindig létre kell hozni. A példány létrehozásakor inicializáljuk az API-t,
és ez a példány teszi lehetővé pl. az adatbázis elérését.
Bár a lanview osztály példányosítható, de praktikusan egy leszármazottját használjuk mely
tartalmazhatja a program inicializálását is.
A lanView osztály példányosításakor a következő műveleteket hajta végre:
- Betölti a nyelvi fájlokat a LANG környezeti változó alapján, ha vannak.
- Betölti a config állományt a QSettings példányosításával.
- Inicializálja a debug rendszert (a cDebug példányosítása)
- Értelmezi a program paramétereket (ami rá vonatkozik):
- Unix típusú operációs rendszer esetén inicializálja a signal-ok fogadását.
- Megnyitja az adatbázist, ha a kértük (lásd később).
- Inicializálja az SNMP alrendszert, ha kértük (ez Windows alatt hiányzik az API-ból).

@section cError Hiba kezelés, a cError osztály.
Az API hiba esetén egy a hiba paramétereivel létrehozott cError objektum pionterével hibát dob. Ha el akarjuk kapni a hibát, akkor az ilyenkor használatos try { … } blokkban használhatjuk a CATCHS() makrót, mely az összes kizárás típust lekezeli.
Program hiba esetén a következő makrók használhatóak:
- EXCEPTION()	Általános hiba eseténi kizárást generáló makró
- NEWCERROR()	Egy hiba objektum példány létrehozása, feltöltése.
- LSTXGET()	Egy listából egy érték lekérése, hiba estén kizárás dobása.
- LSTUXGET()	Egy listából egy érték lekérése, hiba estén kizárás dobása, az index előjel nélküli.
- SQLERR()	SQL hiba
- SQLOERR()	SQL megnyitási hiba
- SQLPREPERR()	SQL prepare hiba
- SQLQUERYERR()	SQL lekédezés hiba


@section cDebug Nyomkövetés, a cDebug osztály
A cDebug osztállyal közvetlenül nem találkozunk, azt a lanView osztály inicializálja ill. példányosítja.
Használata pedig makrókon keresztül történik.
Minden a cDebug-on keresztül kiírt üzenetet a cDebug egy fejléccel lát el, mely hasonló a syslog üzenetekhez,
továbbá ha a program több szálú, akkor a fejléc tartalmazza az üzenetet küldő szál nevét is.
A cDebug lehetőséget biztosít GUI program esetén az üzenetek elfogására a signal-on keresztül, ha azt a GUI-ban meg akarjuk jeleníteni.
A cDebug az üzeneteket mindig a fő szálban írja ki, a mellék szálak üzeneteit egy QQueue soron keresztül kapja meg.
Minden üzenet csak akkor kerül kiírásra ill. a sorba, ha lezártuk az endl manipulátorral.

@section detemanip Adatkezelés
Az API fel van készítve a több szálú működésre, beleértve az adatbázis műveleteket is.
Az adatbázis eléréséhez a newQuery() vagy a getQuery() függvényekkel kérhetünk egy QSqlQuery objektumot.
Az igy kapott objektumnál csak arra kell vigyázni, hogy abban a szálban kell hívni az előző függvényeket, amelyikben
használni is fogjuk a QSqlQuery objektumot.
Az adatbázis eléréséhez az API nem definiál külön osztályt a magasabb szintű adat manipulátorokon kívül. De a következő
függvényeket tartalmazza az adatkezelés megkönnyítésére:
- QSqlDatabase *  getSqlDb(void);
- static inline QSqlQuery * newQuery(QSqlDatabase *pDb)
- static inline QSqlQuery getQuery(QSqlDatabase *pDb)
- void dropThreadDb(const QString& tn, bool __ex)
- bool executeSqlScript(QFile& file, QSqlDatabase *pq, bool __e)
- bool sqlBegin(QSqlQuery& q, bool __ex)
- bool sqlEnd(QSqlQuery& q, bool __ex)
- bool sqlRollback(QSqlQuery& q, bool __ex)
Továbbá néhány string függvény:
- static inline QString  mCat(const QString& a, const QString& b)
- static inline QString dQuoted(const QString& __s)
- static inline QString quoted(const QString& __s)
- static inline QString unQuoted(const QString& name)
- static inline QString unDQuoted(const QString& name)
- static inline QString dQuotedCat(const QString& a, const QString& b)

@subsection crec A cRecord bázis osztály
A cRecord egy összetett bázis osztály, amely egy sablont biztosít az adatbázis adattáblái és a C++ objektumok
közötti megfeleltetéshez. Az osztály által biztosított egy-egy megfeleltetésnek természetesen csak egyes tábla
típusoknál van értelme, tipikusan azoknál, melyek maguk is objektum jellegűek, pl. konkrét tárgyakat reprezentálnak.

@subsection  cRecStaticDescr A cRecStaticDescr leíró objektum
Az adatkezelést vezérlő a tábla tulajdonságait tartalmazó cRecStaticDescr osztály teszi lehetővé a cRecord leszármazott osztály
tábla specifikus működését.

@subsection alternate A cAlternate osztály
A cAlternate egyolyan cRecord leszármazott, melyhez dinamikusan rendelhető egy adattáblához, vagyis a leíró cRecStaticDescr
pointere nem statikus. Az osztály természetesen nem egyenértékű a konkrétan egy adattáblához definiált osztállyal, mivel a
specifikus tulajdonságai hiányoznak.

@subsection crectmpl További sablonok
- tRecordList   A cRcord leszármazottak konténere
- tGroup    A csoport és tagság kapcsoló tábla kezelő sablon

@section User Felhasználók kezelése.

@section location Helyek helyiségek

@section node Hálózati elemek, a "node"-ok
A hálózati elemek az adatbázisban egy meglehetősen bonyolultra sikerült származási fát alkotnak a portokkal együtt.
A velük egyenértékű osztályok ezt a leszármazási vonalat csak részben követik. A patch és hub egy kicsit
jobban elkülönül ezen a szinten, a vagyis leszámítva a közös cRecord őst független osztályok.

@section topology A hálózati topológiák

@section service Szolgáltatások

@section inspector Szolgáltatások kezelése, a cInspector osztály.
A cInspector osztály lekérdező programokban a host-services rekordok alapján épít fel a lekérdezéshez
A szolgáltatáspéldányok fáját, és vezérli a lekérdezés részfolyamatait. A konkrét réstevékenységek a
cInspector származtatásával valósul meg, a virtuális metódusokon keresztül.

@section timeper Idő intervallumok

@section alarm A riasztási rendszer

@section GUI GUI elemek
Az lv2 modul csak az adatbázis elérését biztosító osztályokat tartalmazza. A konkrét GUI funkciók az lv2g modulban találhatóak.
A GUI-val kapcsolatos osztályok az lv2-ven:
- cMenuItem Egy GUI menüjét, ill annak egy elemét leíró osztály.
- cTableShape   Egy adattábla megjelenítését, ill. azok kapcsolatát írja le.
- cTableShapeField A tábla mezők megjelenítését leíró osztály (része a cTableShape osztálynak)
- cTableShapeFilter A mezőkre megadható filtereket írja le (része a cTableShapeField osztálynak)
- cEnumVal Enumerációs értékek megjelenítése ill. szótár

@section others Az API saját egyéb adat típusai.
- cMac  Ethernet cím (MAC)
- netAddress Hálózati címtartomány (IP/mask)
- netAddressList    Hálózati címtartományok konténere

*/


