Hálózat nyilvántartó, monitorozó és vagyonvédelmi szoftver Qt/C++ alapokon.

Vagyonvédelmi hardver eszközökkel kiegészítve.
A program azoknak lehet hasznos, akik nagyobb kliens ill. végpont számú hálózatot menedzselnek, és nem csak a szerverek működésére kiváncsiak.

A dokumentáció folyamatosan készül, akár naponta változhat.

A rendszer dokumentációja a project fájlok között: LanView2.odt

Egy rövide leírás, és egy prezentáció: lv2pre.odt lanview.pptx

Az interpreter dokumentációja : import/import_man.odt 
Az API dokumentáció pedig : http://svn.kkfk.bgf.hu/lanview2.doc/LanView2_API/ címen.
Az adatbázis dokumentáció : http://svn.kkfk.bgf.hu/lanview2.doc/database/ cimen.
(Sajnos az postgresql_autodoc csak egy bekezdést kezel egy megjegyzésnél, nálam meg több bekezdés is van, ezért ezek osszefolynak.)

A rendszer elemei:

lv2	Program könyvtár: a lanview2 rendszer API (a GUI nélkül)

lv2g	Program könyvtár: a lanview2 rendszer API a GUI-hoz.

lv2gui	A lanview2 alapértelmezett GUI-ja.

lv2d	A lanview2 rendszer szerver keret programja.

import	A rendszer interpretere, ill. interpreter távoli végrehajtás szerver pr.

portstat Switch portok állapot lekérdező pr.

portmac Switch portok címtábláit lekérdező pr.

arpd	Az ARP táblákat, vagy DHCP konfig állományokat lekérdező pr.

icontsrv A vagyonvédelmi kiegészítő hardever elemeket lekérdező program.

updt_oui Az OUI rekordokat frissítő program.

database Mappa: az adatbázis

design	Mappa: a vagyonvédelmi hardvare elemek gyártási dokumentációja

firmware Mappa: a vagyonvédelmi hw elemek fimware

