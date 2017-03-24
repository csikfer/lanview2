/*!
@file doxydoc.h
@author Csiki Ferenc <csikfer@gmail.com>
@brief Nem igazi fejállomány. Az API dokumentáció része, csak megjegyzéseket tartalmaz.
*/

/*!
@mainpage LanView2 API
@author Csiki Ferenc <csikfer@gmail.com>
@section A LanView API Inicializálása, a lanView osztály
Az API-nak hasonlóan a Qt-ben a QApplication osztályhoz van egy „globálisan” objektuma,
amit egy példányban mindig létre kell hozni. A példány létrehozásakor inicializáljuk az API-t,
és ez a példány teszi lehetővé pl. az adatbázis elérését is.
Bár a lanview osztály példányosítható, de praktikusan egy leszármazottját használjuk mely
tartalmazza a program inicializálását is.
@note
GUI applikáció esetén a lanView helyett az lv2g osztályt (vagy leszármazotját) kell példányosítni a
lanView helyett, mely a lanView leszármazottja, és tartalmaz néhány GUI-val kapcsolatos kiegészítést.

A lanView osztály példányosításakor a következő műveleteket hajta végre:
- Betölti a nyelvi fájlokat a LANG környezeti változó alapján, ha vannak.
- Betölti a config állományt a QSettings példányosításával.
- Inicializálja a debug rendszert (a cDebug példányosítása)
- Értelmezi a program paramétereket (ami rá vonatkozik):
- Unix típusú operációs rendszer esetén inicializálja a signal-ok fogadását.
- Megnyitja az adatbázist, ha a kértük (lásd később).
- Inicializálja az SNMP alrendszert, ha kértük (ez Windows alatt hiányzik az API-ból).

@section cError Hiba kezelés, a cError osztály.
Az API hiba esetén egy a hiba paramétereivel létrehozott cError objektum pionterével hibát dob. Ha el akarjuk kapni a hibát,
akkor az ilyenkor használatos try { … } blokkban használhatjuk a CATCHS() makrót, mely az összes kizárás típust lekezeli.
Program hiba esetén a következő makrók használhatóak:
- EXCEPTION()	Általános hiba eseténi kizárást generáló makró
- NEWCERROR()	Egy hiba objektum példány létrehozása, feltöltése.
- LSTXGET()	Egy listából egy érték lekérése, hiba estén kizárás dobása.
- LSTUXGET()	Egy listából egy érték lekérése, hiba estén kizárás dobása, az index előjel nélküli.
- SQLERR()      SQL hiba
- SQLOERR()     SQL megnyitási hiba
- SQLPREPERR()	SQL prepare hiba
- SQLQUERYERR()	SQL lekédezés hiba


@section cDebug Nyomkövetés, a cDebug osztály
A cDebug osztállyal közvetlenül nem találkozunk, azt a lanView osztály inicializálja ill. példányosítja.
Használata pedig makrókon keresztül történik.
Minden a cDebug-on keresztül kiírt üzenetet a cDebug egy fejléccel lát el, mely hasonló a syslog üzenetekhez,
továbbá ha a program több szálú, akkor a fejléc tartalmazza az üzenetet küldő szál nevét is.
A cDebug lehetőséget biztosít GUI program esetén az üzenetek elfogására egy signal-on keresztül, ha azt a GUI-ban meg akarjuk jeleníteni.
A cDebug az üzeneteket mindig a fő szálban írja ki, a mellék szálak üzeneteit egy QQueue soron keresztül kapja meg.
Minden üzenet csak akkor kerül kiírásra ill. a sorba, ha lezártuk az endl manipulátorral.

@section detemanip Adatkezelés
Az API fel van készítve a több szálú működésre, beleértve az adatbázis műveleteket is.
Az adatbázis eléréséhez a newQuery() vagy a getQuery() függvényekkel kérhetünk egy QSqlQuery objektumot.
Az igy kapott objektumnál csak arra kell vigyázni, hogy abban a szálban kell hívni a fenti függvényeket, amelyikben
használni is fogjuk a QSqlQuery objektumot.
Az adatbázis eléréséhez az API nem definiál külön osztályt a magasabb szintű adat manipulátorokon kívül. De a következő
függvényeket tartalmazza az adatkezelés megkönnyítésére (lásd még: lv2sql.h):
- QSqlDatabase *  getSqlDb(void);
- static inline QSqlQuery * newQuery(QSqlDatabase *pDb)
- static inline QSqlQuery getQuery(QSqlDatabase *pDb)
- void dropThreadDb(const QString& tn, enum eEx __ex)
- bool executeSqlScript(QFile& file, QSqlDatabase *pq, eEx __e)
- bool sqlBegin(QSqlQuery& q, enum eEx __ex)
- bool sqlEnd(QSqlQuery& q, enum eEx __ex)
- bool sqlRollback(QSqlQuery& q, enum eEx __ex)
- bool execSql(QSqlQuery& q, const QString& sql, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5);
- bool execSqlFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5);
- qlonglong execSqlIntFunction(QSqlQuery& q, bool *pOk, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5);
- QString execSqlTextFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5);
- bool execSqlRecFunction(QSqlQuery& q, const QString& fn, const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5);
- void sqlNotify(QSqlQuery& q, const QString& channel, const QString& payload);


Továbbá néhány string függvény:
- static inline QString mCat(const QString& a, const QString& b)
- static inline QString dQuoted(const QString& __s)
- static inline QString quoted(const QString& __s)
- static inline QString unQuoted(const QString& name)
- static inline QString unDQuoted(const QString& name)
- static inline QString dQuotedCat(const QString& a, const QString& b)
- static inline QString parentheses(const QString& __s)

@subsection crec A cRecord bázis osztály
A cRecord egy összetett bázis osztály, amely egy sablont biztosít az adatbázis adattáblái és a C++ objektumok
közötti megfeleltetéshez. Az osztály által biztosított egy-egy megfeleltetésnek természetesen csak egyes tábla
típusoknál van értelme, tipikusan azoknál, melyek maguk is objektum jellegűek, pl. konkrét tárgyakat reprezentálnak.

@subsection  cRecStaticDescr A cRecStaticDescr leíró objektum
Az adatkezelést vezérlő, a tábla tulajdonságait tartalmazó cRecStaticDescr osztály teszi lehetővé a cRecord leszármazott osztály
tábla specifikus működését.

@subsection alternate A cRecordAny osztály
A cRecordAny egyolyan cRecord leszármazott, melyhez dinamikusan rendelhető egy adattáblához, vagyis a leíró cRecStaticDescr
pointere nem statikus. Az osztály természetesen nem egyenértékű a konkrétan egy adattáblához definiált osztállyal, mivel a
specifikus tulajdonságai hiányoznak.

@subsection crectmpl További sablonok
- tRecordList   A cRcord leszármazottak konténere
- tOwnRecords   Egy cRecord leszármaott objektum tulajdonában lévő objektumok konténere
- tGroup    A csoport és tagság kapcsoló tábla kezelő sablon
- cGroupAny     Hasonló a tGroup osztályhoz, de a group - member objektumok típusa a cRecordAny

@section User Felhasználók kezelése.

@section location Helyek helyiségek

@section node Hálózati elemek, a "node"-ok
A hálózati elemeket az adatbázisban három tábla reprezentálja, melyek származási sora:
patchs -> nodes -> snmpdevices. A megfelelő API osztályok ugyan ezt a származási sorrendet követik:
cPatch -> cNode -> cSnmpDevice.

A portokat szintén három tábla irja le: nports, pports, interfaces, melyekkel megfeleltethető az
azonos származási sort alkotó három API osztály: cNPort, cPPort, cInterface.

A hálózati elemek és portok relációja viszont nem ennyire egyenes. A patchs rekord ill. cPatch osztálynak
(patch panelek és fali, vagy egyébb csatlakozók) csak pporsts ill. cPPort elemei lehetnek, és ez a port típus csak a
cPatch osztályhoz rendelhető. A másik 2-2 objektum típus szabadon összerendelhető.


@section topology A hálózati topológiák
- Fizikai topológia
- Logikai tokológia
- Felfríthető topológiák

@section service Szolgáltatások

@section inspector Szolgáltatások kezelése, a cInspector osztály.
A cInspector osztály lekérdező programokban a host-services rekordok alapján épít fel a lekérdezéshez
a szolgáltatáspéldányok fáját, és vezérli a lekérdezés részfolyamatait. A konkrét résztevékenységek
egy körét az alap osztály tartalmazza, illetve a cInspector származtatásával valósítható meg,
a virtuális metódusokon keresztül. Egy cIspector objektum működését mindíg egy cHostServices objektum
(vagyis egy hostt_services rekord) és az általuk hivatkozott cService objektumok (services rekordok)
határozzák meg. Kulcs szerepük ebben még a features mezőknek is.
@subsection windhcpconf Egy példa a windows szerver DHCP konfigurációjának a lekérdezéséhez.
A lekérdezést az lv2d nevű APP Windows-os változata hajthatja végre. A példában a lekérdezés a
netsh parancs hívásával valósul meg.
A sükséges services definíciók (lásd: import_srv.txt fájlt, a példa ezzel nem azonos, kimaradtak a példa
szempontjából irreleváns paraméterek):
~~~~~~~~~~
SERVICE lv2d "Lanview2 super server" {
    NIL;
    SUPERIOR SERVICE MASK "~."; // Semmire sem illeszkedik
    FEATURES ":superior:timing=passive:logrot=500M,8:";
    NORMAL CHECK INTERVAL  300;
}

SERVICE "win.dhcp.conf.parser" "DHCP konfiguráció a netsh-n keresztül; Fordító" {
    FEATURES ":timing=polling:method=parser:";
}

SERVICE "win.dhcp.conf" "DHCP konfiguráció a netsh-n keresztül" {
    FEATURES ":timing=polling:method=qparse:comment=#:";
    CMD "netsh dhcp server \\\\$node dump all";
}

QUERY PARSER "win.dhcp.conf.parser" {
    CASE NO
        $rexp$Dhcp\s+Server\s+\\+([\w\-\.]+)\sScope\s+[\d\.\:]+\s+Add\s+iprange\s+([\d\.\:]+)\s+([\d\.\:]+)$rexp$
        $ipar$REPLACE DYNAMIC ADDRESS RANGE $2 TO $3 "$1".dhcp;$ipar$;
    CASE NO
        $rexp$Dhcp\s+Server\s+\\+([\w\-\.]+)\sScope\s+[\d\.\:]+\s+Add\s+excluderange\s+([\d\.\:]+)\s+([\d\.\:]+)$rexp$
        $ipar$REPLACE DYNAMIC ADDRESS RANGE EXCLUDE $2 TO $3 "$1".dhcp;$ipar$;
    CASE NO
        $rexp$Dhcp\s+Server\s+\\+([\w\-\.]+)\sScope\s+[\d\.\:]+\s+Add\s+reservedip\s+([\d\.\:]+)\s+([\dA-F]+)\s+(.*)$rexp$
        $ipar$REPLACE ARP $2 MAC("$3") config $host_service_id $$note$$$4$$note$$;$ipar$;
}

HOST SERVICE "win-lv2".lv2d {
    FEATURES ":timing=polling:";
}
HOST SERVICE "win-lv2"."win.dhcp.conf.parser" {
    SUPERIOR SERVICE "win-lv2".lv2d;
}
HOST SERVICE "win-dhcp-1"."win.dhcp.conf" {
    SUPERIOR SERVICE "win-lv2"."win.dhcp.conf.parser";
    FEATURES ":timing=polling:";
}
HOST SERVICE "win-dhcp-2"."win.dhcp.conf" {
    SUPERIOR SERVICE "win-lv2"."win.dhcp.conf.parser";
    FEATURES ":timing=polling:";
}
~~~~~~~~~~
A fenti példában a "win-lv2" nevű gép futtatj az lv2d appot, időzítés nélkül (idítás után egyszer elvégzi a feladatát majd kilép).
A "win.dhcp.conf.parser" definiálja a módszert a lekérdezéshez a query_parser rekord definíciókkal együtt.
A konkrét lekérdezés az utolsó két bejegyzés, két DHCP serverre, a win-dhcp-1 és win-decp-2 nevű szerver lekérdezése.
Mivel a "win.dhcp.conf"-ben szerepel a gép név mint a netsh paraméter, ezért itt a lekérdezendő szerver nevének
anonosnak kell lennie a Windows host névvel.

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


