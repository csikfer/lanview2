Hálózat nyilvántartó, monitorozó és vagyonvédelmi szoftver Qt/C++ alapokon.

A program alapja egy adatbázis, ami leképezi a teljes hálózatot. Erre épül egy lekérdező rendszer, melynek kettős feladata van. Egyrészt adatot gyűjt, feltöltve az adatbázist a lekérdezhető elemekkel, másrészt támogatja a manuális adatbevitelt. Ezen felül a lekérdező rendszer hasonló feladatokat is elláthat, mint a Nagios rendszer. Végül a rendelkezésre álló állapot adatok lehetővé teszik, hogy vagyonvédelmi funkciókat is ellásson. A rendszer ki lett egészítve hardver elemekkel, a nem aktív hálózati elemek védelmére.

A program azoknak lehet hasznos, akik nagyobb kliens ill. végpont számú hálózatot menedzselnek, és nem csak a szerverek működésére kíváncsiak.

A dokumentáció folyamatosan készül, akár naponta változhat.

A rendszer dokumentációja a projekt fájlok között: LanView2.odt

Egy rövid leírás, és egy prezentáció: lv2pre.odt lanview.pptx

Az interpreter dokumentációja : import/import_man.odt 
Az API dokumentáció pedig : http://svn.kkfk.bgf.hu/lanview2.doc/LanView2_API/ címen.
Az adatbázis dokumentáció : http://svn.kkfk.bgf.hu/lanview2.doc/database/ címen.
(Sajnos az postgresql_autodoc csak egy bekezdést kezel egy megjegyzésnél, nálam meg több bekezdés is van, ezért ezek összefolynak.) (Úgy látom, hogy az autodoc mára már teljesen használhatatlan, hiányos állományt generál.)

A rendszer elemei:

lv2	Program könyvtár: a lanview2 rendszer API (a GUI nélkül)

lv2g	Program könyvtár: a lanview2 rendszer API a GUI-hoz.

lv2gui	A lanview2 alapértelmezett GUI-ja.

lv2d	A lanview2 rendszer szerver keret programja.

import	A rendszer interpretere, ill. interpreter távoli végrehajtás szerver program

portstat (pstat) Switch portok állapot lekérdező program

portvlan Switch portok VLAN kiosztásának a lekérdezése.

portmac Switch portok címtábláit lekérdező program

arpd	Az ARP táblákat, vagy DHCP konfigurációs állományokat lekérdező program

snmpvars Általános SNMP értékek lekérdezése.

rrdhelper RRD fájlok kezelése

icontsrv A vagyonvédelmi kiegészítő hardver elemeket lekérdező program.

updt_oui Az OUI rekordokat frissítő program.

database Mappa: az adatbázis

design	Mappa: a vagyonvédelmi hardver elemek gyártási dokumentációja

firmware Mappa: a vagyonvédelmi hardver elemek firmware

Megjegyzés: Az SNMP lekérdezések viszonylag kevés általában HP gyártmányú switch-eken lettek tesztelve.
Sajnos a gyártok az ajánlásokat elég lazán kezelik, így nincs garancia arra, hogy más gyártótól való
eszközökön is jól működnek a lekérdezések. Ez különösen igaz az LLDP felfedezésre.

Megjegyzés: Jelenleg én aktívan használom a programot, és a felmerülő igények szerint bővítem is. Sajnos a
fejlesztő 'csapat' létszáma (1 fő) nincs igazán összhangban a feladat méretével. Ezért idő hiányában, továbbá
a visszajelzések teljes hiányának
következtében viszonylag kevés figyelmet kap a módosítások problémamentes lekövetése, a dokumentációk frissítése.
Ezért, ha valaki használni kívánja a programot, az legyen szíves jelezzen!
A program honosítható, ehhez minden elemet tartalmaz, de a honosításhoz nekem sem időm, sem kellő nyelvtudásom
nincsen.
Mivel javarészt HP switch-ekkel van lehetőségem tesztelni a rendszert, így más gyártók termékeivel lehetnek
problémák. Sajnos az SNMP ajánlásokat elég lazán kezelik a gyártók, így lehetnek olyan kivételek, sajátosságok,
amiket nem kezelek.

