/***************************************************************************
 *   Copyright (C) 2008 by Csiki Ferenc   *
 *   csikfer@indasoft.hu   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/*!
@file qsnmp.h
Objektumok az SNMP protokolhoz.
A net-snmp -n alapul: (unix:LIBS += -lsnmp). Csak linux alatt implementált (elvileg működhetne windows alatt is).
Az SNMP (MIB) névfeloldáshoz (Ubuntu 14.04):
    sudo apt-get snmp lib-snmp-dev snmp-mibs-downloader
    sudo download-mibs
A /etc/snmp/snmp.conf -ban meg kell adni a MIB-ek útvonalát (alapértelmezetten tiltva):
    # mibs :
    mibdirs /usr/share/mibs/iana:/usr/share/mibs/ietf:/usr/share/snmp/mibs
Egyébb használlt MIB-ek útvonalával is ki kell egészíteni!
*/

#ifndef QSNMP_H
#define QSNMP_H
#include <Qt>
#include "lv2_global.h"
#include "lv2types.h"

EXT_ QString snmpNotSupMsg();

// NET-SNMP EXISTS?
#ifdef SNMP_IS_EXISTS

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <string.h>

/// Hálózati interfész SNMP statuszok
enum eIfStatus {
    IF_UP           = 1,
    IF_DOWN         = 2,
    IF_TESTING      = 3,
    IF_UNKNOWN      = 4,
    IF_DORMAT       = 5,
    IF_NOTPRESENT   = 6,
    IF_LOWERLAYERDOWN=7
};

EXT_ const QString&  snmpIfStatus(int __i, enum eEx __ex = EX_ERROR);
EXT_ int             snmpIfStatus(const QString& __s, enum eEx __ex = EX_ERROR);

typedef QVector<QVariant> QVariantVector;
/// @class cTable
/// Egy táblázat konténer.
/// A táblázat oszlopai név szerint érhatőek el, a sorok pedig sorszám alapján.
/// A táblázat (mátrix) elemei QVariant-ok.
class LV2SHARED_EXPORT cTable : public QMap<QString, QVariantVector >
{
   public:
    /// A konstruktor egy üres táblát hoz létre
    cTable() : QMap<QString, QVector<QVariant> >() { ; }
    /// A táblában lévő oszlopok számával tér vissza
    int columns(void) const { return size(); }
    /// A tábla sorainak számával tér vissza, feltételezve, hogy minden oszlop azonos számú elemet tartalmaz.
    ///
    int rows(void) const    { return size() ? constBegin()->size() : 0; }
    /// Egy elem keresése
    /// Ha van találat, akkor az elem pointere, egyébként NULL
    /// @param __in Az index oszlop neve
    /// @param __ix Az index érték, az index oszlopban, megadja a keresett sort
    /// @param __com A keresett oszlop neve
    QVariant *find(const QString __in, QVariant __ix, const QString __col);
    /// Egy üres oszlop hozzáadása a konténerhez.
    /// Ha volt első oszlop, és abban voltak sorok, akkor az új oszlopban is
    /// ennyi üres sor lessz.
    /// @param __cn Az oszlop neve. Egyedi kell hogy legyen egy konténeren bellül.
    cTable& operator<<(const QString& __cn);
    /// A konténer tartalmár striggé konvertálja.
    QString toString(void) const;
};

/// Bit string konvertálása QBitArray-ba
/// @param __bs Bit string pointerek
/// @param __os A bit string mérete byte-ban (__os*8 == bitek száma)
EXT_ QBitArray   bitString2Array(u_char *__bs, size_t __os);

class LV2SHARED_EXPORT netSnmp {
   protected:
    /// Első híváskor (ha nincs több netSnmp példány) inicializálja az SNMP könyvtárat.
    /// Ha a lanView objektumban meg van adva MIB path, akkor azokat hozzáadja a MIB-ek keresési útvonalához.
    netSnmp();
    /// Az utolsó objektumnál hívja az snmp_shutdown() függvényt.
    ~netSnmp();
    /// Törli az snmp_errno globális változót, és a status, valamint az emsg adattagot
    void    clrStat(void);
    /// Ha snmp_errno értékét átmásolja a status adattagba, és ha értéke nem nulla, akkor a hozzá tartozó hiba üzenetet bemásolja az emsg adattagba.
    /// @return a staus adattag új értéke
    int     setStat(void);
    /// Hasonló a paraméter nélküli híváshoz, de a hiba tényét az __e paraméterrel jelezzük, és megadható egy opcionális string, amit a hibaüzenethez fűz.
    /// @param __e Ha értéke false, akkor egyenértékű a clrStat() hívással. Ha értéke true, és snmp_errno nem nulla, akkor az setStat()-nak megfelelő
    ///  műveletet hajtja végre, és hozzáfűzi a hibaüzenethez az __em tartalmát, szeparátorként egy perjelet használva. Ha értéke true, és snmp_errno
    ///  értéke zérus, akkor staus értékét 1-re állítja, és az "[ERROR]" hibaüzenetet (emsg) állítja be, ill ehez fűzi hozá a __em stringet hasonlóan mint
    ///  előbb.
    /// @param __em Egy opcionális string, amit hozzáfűz a hiba string-hez
    /// @return a staus adattag új értéke
    int     setStat(bool __e, const QString& __em);
    /// Igazzal tér vissza ha a staus értéke nulla.
    virtual operator bool() const   { return status == 0; }
    /// Igazzal tér vissza ha a staus értéke nem nulla.
    virtual bool operator !() const { return status != 0; }
   public:
    static void setMibDirs(const char * __dl);
    static void setMibDirs(const QString& __dl)     { setMibDirs(__dl.toStdString().c_str()); }
    static void setMibDirs(const QStringList& __dl) { setMibDirs(__dl.join(QChar(','))); }
    static void init(void);
    static void down(void);

    int status;
    QString emsg;
   private:
    static void implicitInit(void);
    static void implicitDown(void);
    static const char *type;
    static int      objCnt;
    static int      explicitInit;
    static bool     inited;
};

class LV2SHARED_EXPORT cOId : public QVector<oid>, public netSnmp {
   private:
    static const oid  zero;
    size_t  oidSize;
   public:
    cOId();
    cOId(const oid *__oid, size_t __len);
    cOId(const char *__oid);
    cOId(const QString& __oid);
    cOId(const cOId& __oid);
    ~cOId();
    /// Az objektum méretét (integer értékek száma) adja vissza
    size_t  size() const    { return oidSize; }
    /// Ha az objektum üres (hossza nulla) akkor true-val tér vissza.
    bool    isEmpty() const { return 0 == size(); }
    cOId&   set(const oid *__oid, size_t __len);
    cOId&   set(const char * __oid);
    cOId&   set(const QString& __oid)       { return set(__oid.toStdString().c_str()); }
    cOId&   set(const cOId& __oid)          { return set(__oid.data(), __oid.oidSize); }
    cOId&   clear()                         { return set(NULL, 0); }
    /// Ha az objektum mérete megfelelő, és a hiba státus nulla, akkor true-val tér vissza.
    /// Ha az objektum méretét jelző érték nagyobb, mint a konténer, akkor dob egy kizárást.
    virtual operator bool() const;
    virtual bool operator !() const;
    cOId& operator =(const char * __oid)    { return set(__oid); }
    cOId& operator =(const QString& __oid)  { return set(__oid); }
    cOId& operator =(const cOId& __oid)     { return set(__oid); }
    cOId& operator <<(int __i);
    cOId& operator <<(const QString& __s);
    cOId  operator +(int __i) const             { return cOId(*this) << __i; }
    cOId  operator +(const QString& __s) const  { return cOId(*this) << __s; }
    /// Összehasonlít két objektumot. Az összehasonlítés eredménye akkor igaz, ha a bal érték
    /// elemszáma kisebb mint a jobb érték, és a balérték, és a jobb érték elejének a balértéknek
    /// megfelelő hosszú darabja azonos, egyébbként az eredmény hamis. Nem sorrendiséget állapít meg.
    bool  operator  <(const cOId& __o) const;
    /// Összehasonlít két objektumot. Az összehasonlítés eredménye akkor igaz, ha a jobb érték
    /// elemszáma kisebb mint a bal érték, és a jobbérték, és a bal érték elejének a jobbértéknek
    /// megfelelő hosszú darabja azonos, egyébbként az eredmény hamis. Nem sorrendiséget állapít meg.
    bool  operator  >(const cOId& __o) const{ return __o < (*this); }
    /// Összehasonlít lét objektumot. Ha a kettő azonos, akkor igazzal tér vissza.
    bool  operator ==(const cOId& __o) const;
    bool  operator <=(const cOId& __o) const{ return *this == __o ||  *this < __o; }
    bool  operator >=(const cOId& __o) const{ return *this == __o ||  *this > __o; }
    oid last() const { return at(oidSize -1); }
    /// Összehasonlít lét objektumot. Ha a kettő azonos, akkor hamis tér vissza.
    bool  operator !=(const cOId& __o) const{ return !(*this == __o); }
    /// Ha a jobb érték kisebb (lsd < operator), akkor a jobb érték balértékkel azonos eleje törlődik.
    /// Ha a jobb érték nem azonos a bal érték elejével, akkor az eredmény egy üres objrektum lessz.
    cOId& operator -=(const cOId& __o);
    /// Ha a jobb oldali oerandus kisebb (lsd < operator), akkor az eredmény a baloldali operandus lessz a jobboldali
    /// operandussal azonos részt törölve.
    /// Ha a jobb oldali operandus nem azonos a bal oldali operandus elejével, akkor az eredmény egy üres objrektum lessz.
    cOId  operator  -(const cOId& __o) const{ cOId r(*this); return r -= __o; }

    oid pop_front() {
        if (oidSize <= 0) EXCEPTION(EPROGFAIL, oidSize);
        oidSize--;
        oid r = at(0);
        QVector<oid>::pop_front();      // A vektorunk fix hosszú, de most rövidebb lett
        QVector<oid>::push_back(zero);  // Vissza a fix hossz.
        return r;
    }
    oid pop_back() {
        if (oidSize <= 0) EXCEPTION(EPROGFAIL, oidSize);
        oidSize--;
        oid r = at(oidSize);
        (*this)[oidSize] = zero;
        return r;
    }
    cOId mid(int _first, int _size);
    QVariant toVariant() const;
    QString toString() const;
    QString description() const;
    QString toNumString() const;
    /// Az utolsó hat elemet megpróbálja MAC-ként értelmezni.
    /// Ha nincs 6 eleme az objektumnak, vagy az utolsó 6 elem nem a 0-255 tartományba esik, akkor egy üres objektummal tér vissza.
    cMac toMac() const;
    /// Az utolsó négy elemet megpróbálja IPV4 címként értelmezni.
    /// Ha nincs 4 eleme az objektumnak, vagy az utolsó 4 elem nem a 0-255 tartományba esik, akkor egy üres objektummal tér vissza.
    /// @param _in Ha megadjuk, akkor az ODI végén ennyi elemet hagy figyelmen kívül a konverzió elött.
    QHostAddress toIPV4(uint _in = 0) const;
    // QString toFullString() const;
    bool    chkSize(size_t __len);
};

class LV2SHARED_EXPORT cOIdVector : public QVector<cOId> {
   public:
    cOIdVector() : QVector<cOId>() { ; }
    cOIdVector(int n) : QVector<cOId>(n) { ; }
    cOIdVector(const QStringList& __sl);
    QString toString();
};

/*!

@class cSnmp
@brief SNMP kliens osztály

 */
class LV2SHARED_EXPORT cSnmp : public netSnmp {
   public:
/*    class async {
        async();
        ~async();
        clear(void);
        static int asyncResponse(init __op, snmp_session *__ss, int __id, snmp_pdu *__resp, voi * __magic);
       protected:
        _init(void);
        enum eState { READY, PENDING, EVENT }   state;
        netsnmp_session *ss;
        netsnmp_pdu *pdu;
        netsnmp_pdu *response;
    };*/
   protected:
    netsnmp_session session;
    netsnmp_session *ss;
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    netsnmp_variable_list * actVar;
    int count;
   public:
    /// Konstruktor. Inicializálja az objektumot, de egy SNMP lekérdezéshez előbb hvni kell az egyik open metódust.
    cSnmp();
    /// Konstruktor. Inicializálja az objektumot, majd a megadott paraméterekkel hívja az open metódust.
    cSnmp(const char * __host, const char * __com = defComunity, int __ver = SNMP_VERSION_2c);
    /// Konstruktor. Inicializálja az objektumot, majd a megadott paraméterekkel hívja az open metódust.
    cSnmp(const QString& __host, const QString& __com = defComunity, int __ver = SNMP_VERSION_2c);
    /// Copy konstruktor. Nem támogatott, dob egy kizárást.
    cSnmp(const cSnmp& __o) { EXCEPTION(ENOTSUPP); __o.type(); }
    /// Destruktor
    virtual ~cSnmp();
    /// Inicializál egy kapcsolatot a megadott hoszt-hoz. Ha volt inicializálva kapcsolat (open), akkor törli azt.
    /// @param __host Az SNMP szerver címe vagy neve.
    /// @param __com Community név. Opcionális, ha nincs megadva, akkor az alapértelmezése "public"
    /// @param __ver SNMP verzió: SNMP_VERSION_2c,SNMP_VERSION_2,SNMP_VERSION_1. Alapértelmezés: SNMP_VERSION_2c
    /// @return A hiba kód. A nem volt hiba, akkor 0.
    int open(const char * __host, const char * __com = defComunity, int __ver = SNMP_VERSION_2c);
    /// Inicializál egy kapcsolatot a megadott hoszt-hoz. Ha volt inicializálva kapcsolat (open), akkor törli azt.
    /// @param __host Az SNMP szerver címe vagy neve.
    /// @param __com Community név. Opcionális, ha nincs megadva, akkor az alapértelmezése "public"
    /// @param __ver SNMP verzió: SNMP_VERSION_2c,SNMP_VERSION_2,SNMP_VERSION_1. Alapértelmezés: SNMP_VERSION_2c
    /// @return A hiba kód. A nem volt hiba, akkor 0.
    int open(const QString& __host, const QString& __com = defComunity, int __ver = SNMP_VERSION_2c);
    /// A megadott snmp objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oid  A lekérdezendő objektum azonosítója (OID).
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int get(const cOId& __oid);
    /// A megadott snmp objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oids  A lekérdezendő objektumok azonosítói (OID).
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int get(const cOIdVector& __oids);
    /// A megadott snmp objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oid  A lekérdezendő objektum azonosítója (OID).
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int get(const QString& __o)                 { return get(cOId(__o)); }
    /// A megadott snmp objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oids  A lekérdezendő objektumok azonosítói (OID).
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int get(const QStringList __ol)             { return get(cOIdVector(__ol)); }
    /// A megadott snmp objektum utáni objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oid  A megadot objektum utáni objektumot kérdezi le.
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int getNext(const cOId& __oid);
    /// A megadott snmp objektum utáni objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oids Lekérdezi minden egyes a listában megadott objektum utáni objektumot.
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int getNext(const cOIdVector& __oids);
    /// A megadott snmp objektum utáni objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oid  A megadot objektum utáni objektumot kérdezi le.
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int getNext(const QString& __o)             { return getNext(cOId(__o)); }
    /// A megadott snmp objektum lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param __oids Lekérdezi minden egyes a listában megadott objektum utáni objektumot.
    /// @return A lekérdezés hibakódja. Ha nincs hiba, akkor 0.
    int getNext(const QStringList __ol)         { return getNext(cOIdVector(__ol)); }
    /// Az előző sikeres lekérdezésnél használt objektum(ok) utáni objektumo(ka)t kérdezi le.
    /// @exception Ha nem egy sikeres lekérdezés után hívtuk.
    int getNext(void);
    /// Táblázat lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param baseId A táblázat ezen az OID-n bellül található
    /// @param columns Az oszlopok nevei, ill. a baseId-hez képesti relatív azonosítói.
    /// @param result Cél objektum.
    /// @return A lekérdezés státusa. Ha minden lekérdezés sikeres, akkor 0.
    ///         Hiba esetén az első hibás lekérdezés státusa.
    int getTable(const cOId& baseId, const QStringList& columns, cTable& result);
    /// Táblázat lekérdezése, a legutóbbi open-el megnyitoot host-ról.
    /// @param baseId A táblázat ezen az OID-n bellül található
    /// @param columns Az oszlopok nevei, ill. a baseId-hez képesti relatív azonosítói.
    /// @param result Cél objektum.
    /// @return A lekérdezés státusa. Ha minden lekérdezés sikeres, akkor 0.
    ///         Hiba esetén az első hibás lejérdezés státusa.
    int getTable(const QString& baseId, const QStringList& columns, cTable& result);
    int getTable(const cOIdVector& ids, const QStringList& columns, cTable& result);
    /// Egy kereszt index tábla lekérdezése
    /// @param xoid Az OID, ami a keresztindexet tartalmazza: <xoid>.<új index>: <eredeti index>
    /// @param xix Kereszt indexek, ahhol a kulcs az új index, érték az eredeti.
    /// @param reverse Opcionális, ha megadjuk és true, akkor az eredmény QMap-ben felcseréli az érték kulcs párokat. Fordított konverzióhoz.
    /// @return Ha a lekérdezés eredményes, akkor 0.
    int getXIndex(const cOId& xoid, QMap<int, int> &xix, bool reverse = false);
    /// Bit map-ek lekérdezése
    /// @param __oid OID: <oid>.<ix> => <bitmap>
    /// @return Ha a lekérdezés eredményes, akkor 0.
    int getBitMaps(const cOId& _oid, QMap<int, QBitArray> &maps);
    const netsnmp_variable_list *first(void)    { return actVar = ((response == NULL) ? NULL : response->variables); }
    const netsnmp_variable_list *next(void)     { return actVar = ((actVar   == NULL) ? NULL : actVar->next_variable); }
    static QVariant value(const netsnmp_variable_list * __var);
    static cOId name(const netsnmp_variable_list * __var);
    static int type(const netsnmp_variable_list * __var);
    QVariant value(void) const                  { return value(actVar); }
    cOId name(void) const                       { return name(actVar); }
    int type(void) const                        { return actVar->type; }
    QVariantVector  values();
    cOIdVector names();
    bool isOpened() const                       { return ss != NULL;  }
    const netsnmp_variable_list *var() const    { return actVar; }
    operator bool() const                       { return status == 0; }
    bool operator!() const                      { return status != 0; }
    cSnmp& operator=(const cSnmp& __o) { EXCEPTION(ENOTSUPP); __o.type(); return *this; }
    static const char *defComunity;
   private:
/**
 *  Elvégzi az adattagok inicalzálását, pointerek nullázása. stb. snmpObjCnt értékét növeli eggyel, ha értéke
 *  nulla volt, akkor inicializálja a net-snmp rendszert is
 */
    void _init(void);
/**
 * Felszabadít minden adatblokkot, alaphelyzetbe állítja az objektumot (mint az _init() után)
 */
    void _clear(void);

    int     __get(void);
    int     _get(int __cmd, const cOId& __oid);
    int     _get(int __cmd, const cOIdVector& __oids);
    bool    checkOId(const cOId& __o);
    bool    checkOId(const cOIdVector& __o);
};

#else // SNMP_IS_EXISTS
enum eSnmp {
    SNMP_VERSION_1,
    SNMP_VERSION_2c,
    SNMP_VERSION_3
};
class LV2SHARED_EXPORT cSnmp {
public:
    cSnmp() { EXCEPTION(ENOTSUPP); }
    cSnmp(const QString&, const QString& = _sNul, int = 0) { EXCEPTION(ENOTSUPP); }
};

#endif // SNMP_IS_EXISTS
#endif // QSNMP_H
