/***************************************************************************
 *   Copyright (C) 2007 by Csiki Ferenc   *
 *   csikfer@csikfer@gmail.com   *
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
#ifndef LV2TYPES_H_INCLUDED
#define LV2TYPES_H_INCLUDED
#include "lv2_global.h"
#include <QtCore>
#include <QtNetwork>
#include <QSqlRecord>

/*! @file lv2types.h
  Az LV2-ben használlt néhény alap adat típus, osztály, ill. makró definíciója.
 */


/*!
  @def __STR
Név string literállá konvertálása. Csak makró definícióban használható
 */
#define __STR(s)    #s
/*!
  @def _STR(s)
Név string literállá konvertálása.
 */
#define _STR(s)     __STR(s)

/// Hiba string
EXT_ QString sInvalidEnum();
/*!
A QProcess::ProcessError érték nevével tér vissza
 */
EXT_ QString ProcessError2String(QProcess::ProcessError __e);
/*!
A QProcess::ProcessError értékhez tartozó hiba üzenettel tér vissza
 */
EXT_ QString ProcessError2Message(QProcess::ProcessError __e);
/*!
A QProcess::ProcessState érték nevével tér vissza
 */
EXT_ QString ProcessState2String(QProcess::ProcessState __e);

EXT_ QString dump(const QByteArray& __a);

/* ============================================================================================= */
/// @class cNullable
/// @brief Egy típust a NULL értéllel kiegészítő template osztály.
///
/// Nem használt
template<class T> class cNullable {
protected:
    T       _v;
    bool    _null;
public:
    cNullable()             : _v()      { _null = true;  }
    cNullable(const T& __t) : _v(__t)   { _null = false; }
    bool isNull() const                 { return _null; }
    bool isEmpty() const                { return _null; }
    cNullable& clear(void)              { _null = true; return *this; }
    cNullable& operator=(const T& __o)          { _v = __o;    _null = false;     return *this; }
    cNullable& operator=(const cNullable& __o)  { _v = __o._v; _null = __o._null; return *this; }
    bool operator <(const cNullable& __o) const { return *this && __o && _v < __o._v;  }
    bool operator<=(const cNullable& __o) const { return *this && __o && _v <= __o._v; }
    bool operator >(const cNullable& __o) const { return *this && __o && _v > __o._v;  }
    bool operator>=(const cNullable& __o) const { return *this && __o && _v >= __o._v; }
    bool operator==(const cNullable& __o) const { return *this && __o && _v == __o._v; }
    bool operator!=(const cNullable& __o) const { return *this && __o && _v != __o._v; }
    bool operator !() const             { return _null; }
    operator T () const                 { if (isNull()) EXCEPTION(EDATA); return _v; }
    operator bool() const               { return !_null; }
};

/**
@class cMac
@brief MAC cím kezelő osztály

Fizikai hálózati cím tárolását, és kezelését végző objektum.A MAC-et egy qlonglong adattagban tárolja.
 */
class LV2SHARED_EXPORT cMac {
   private:
    /// A MAC cím mint 64bites bináris érték. Hiba esetén, vagy ha inicializálatlan, akkor értéke -1.
    qlonglong   val;
    /// Maszk a 64biten tárolt mac érték felesleges bitjeinek a törléséhez/maszkolásához
    static const qlonglong    mask;
    /// Regexp string a mac cím egy megengedett formájához: hexa szám elválasztó karakterek nélkül
    static const char _sMacPattern1[];
    /// Regexp string a mac cím egy megengedett formájához: hexa szám byte-onként kettőspontal elválasztva
    static const char _sMacPattern2[];
    /// Regexp string a mac cím egy megengedett formájához: hexa szám, két 3byre-os rész kötőjellel elválasztva
    static const char _sMacPattern3[];
    /// Regexp string a mac cím egy megengedett formájához: 6 decimális byte-onként ponttal tagolva
    static const char _sMacPattern4[];
    /// Regexp string a mac cím egy megengedett formájához: hexa szám byte-onként space karakterrel elválasztva
    static const char _sMacPattern5[];
   public:
    /// Konstruktor. Null MAC értékkel (-1LL) hozza létre az objektumot.
    /// Ha val értéke negatív, akkor az isValid metódus true értékkel tér vissza.
    cMac();
    /// Konstruktor.
    /// @param __mac A stringet megpróbálja MAC címként értelmezni, és ezzel inicializálja az objektumot.
    /// lásd: set(const QString& __mac) metódust.
    cMac(const QString& __mac);
    /// Konstruktor.
    /// @param __mac A byte sorozatot megpróbálja MAC címként értelmezni, és ezzel inicializálj az objektumot.
    /// lásd: set(const QByteArray& __mac) metódust.
    cMac(const QByteArray& __mac);
    /// Konstruktor.
    /// @param __mac Megpróbálja MAC címként értelmezni, és ezzel inicializálj az objektumot.
    /// lásd,: set(const QVariant& __mac) metódust.
    cMac(const QVariant& __mac);
    /// Konstruktor.
    /// @param[in] __mac ezzel inicializálj az objektumot.
    /// lásd,: set(const QLongLong __mac) metódust.
    cMac(qlonglong __mac);
    /// Destruktor. Gyakorlatilag üres, csak nyomkövetés.
    ~cMac();
    /// A megadott értékkel módosítja a val adattagot, ellenörzés nélkül.
    /// @param[in] __mac Az új MAC cím érték. 64 bites bináris számként ábrázolva.
    cMac& set(qlonglong __mac)               { val = __mac & mask; return *this; }
    /// A val adattagot módosítja
    /// @param[in] __mac A stringet megpróbálja MAC címként értelmezni, és ezzel inicializálj az objektumot.
    /// Ha nem értelmezhető, akkor az objektum értéke invalid lessz (val = -1,
    /// és ekkor az isValid() metódus false-val tér vissza)
    ///
    /// A string feldolgozását a _sMacPattern1, _sMacPattern2, _sMacPattern3 és _sMacPattern4 regexp kifelyezésekkel végzi,
    /// sorban végikpróbálva azokat a string paraméteren. Ha egyik sem illeszkedik, akkor val adattag értéke -1 lessz.
    /// Az értelmezhető cím formátumok (ahol az X karakter egy hexadecimális számjegyet reprezentál,
    /// a 9 pedig decimális számjegyet):
    ///   - "XXXXXXXXXXXX"  Ahol a vezető nullák elhagyhatóak.
    ///   - "XX:XX:XX:XX:XX:XX" A hexadecimális számblokkokból az első nulla számjegy elhagyható.
    ///   - "XXXXXX-XXXXXX" A hexadecimális számblokkokból a vezető nullák elhagyhatóak, de egy számjegynek maradnia kell.
    ///   - "999.999.999.999.999.999 A decimális számblokkokból a vezető nullák elhagyhatóak, de egy számjegynek maradnia kell.
    ///     Ha a negyedik változatnál a számblokkok közül valamelyik értéke 255-nél nagyobb, akkor val értéke invalid (-1) lessz.
    /// @exception cError* Ha a stringek értelmezésénél algoritmikus hiba lép fel, dob egy kizárást EPROGFAIL kóddal.
    cMac& set(const QString& __mac);
    /// A val adattagot módosítja
    /// @param[in] __mac Egy 6 byte hozzsü értéket vár, és ezzel inicializálj az objektumot. Ha nem 6 byte-os a tömb,
    /// akkor az objektum értéke invalid lessz (val = -1, és ekkor az isValid() metódus false-val tér vissza)
    cMac& set(const QByteArray& __mac);
    /// A val adattagot módosítja. Csak a string (String, ByteArray), és a 64 bites számokat értelmezi (LongLong, ULongLong)
    /// @param __mac Az értéket  megpróbálja MAC címként értelmezni, és ezzel inicializálj az objektumot.
    /// Ha nem értelmezhető, ill. nem támogatott az adat típus, akkor az objektum értéke invalid lessz (val = -1,
    /// és ekkor az isValid() metódus false-val tér vissza)
    cMac& set(const QVariant& __mac);
    void clear()                            { val = -1LL; }
    /// Az objektum értékét, a MAC címet stringgé konverálja. Hexa szám byte-onként kettősponttal tagolva.
    /// Nincs ellenörzés, invalid érték esetén a felesleges biteket figyelmen kívül hagyja.
    /// Az alapértelmezett inicializálatlan, vagy invalid érték esetén az "FF:FF:FF:FF:FF:FF" értéket adja vissza.
    QString   toString() const;
    /// A val adattag értékével tér vissza, mely a MAC bináris ábrázolása 64 bitre kiegészyítve 0 bitekkel.
    /// De a val értékét nem ellenörzi.
    qlonglong toLongLong() const            { return val; }
    /// A val adattag értékével tér vissza, mely a MAC bináris ábrázolása 64 bitre kiegészyítve 0 bitekkel.
    /// De a val értékét nem ellenörzi.
    qlonglong toULongLong() const           { return (qulonglong)val; }
    /// Ha a val adattag értéke 0, akkor true-val tér vissza
    bool isEmpty() const                    { return val == 0LL; }
    /// Ha a MAC 64 bites ábrázolásában (val adattag) a fölösleges bitek bármelyike nem nulla,
    /// akkor false-val tér vissza, egyébként true-val.
    bool isValid() const                    { return (val & ~mask) == 0LL; }
    ///
    static bool isValid(qlonglong v)        { return (v   & ~mask) == 0LL; }
    static bool isValid(const QString& v);
    static bool isValid(const QByteArray& v);
    static bool isValid(const QVariant& v);
    /// Ha a MAC 64 bites ábrázolásában (val adattag) a fölösleges bitek bármelyike nem nulla,
    /// akkor false-val tér vissza, egyébként true-val.
    operator bool() const                   { return isValid(); }
    /// Ha a MAC 64 bites ábrázolásában (val adattag) a fölösleges bitek bármelyike nem nulla,
    /// akkor true-val tér vissza, egyébként false-val.
    bool operator !() const                 { return !isValid(); }
    /// A val adattag értékével tér vissza, mely a MAC bináris ábrázolása 64 bitre kiegészyítve 0 bitekkel.
    /// A val adattagot nem ellenőrzi.
    operator qlonglong() const              { return toLongLong(); }
    /// A val adattag értékével tér vissza, mely a MAC bináris ábrázolása 64 bitre kiegészyítve 0 bitekkel.
    /// A val adattagot nem ellenőrzi.
    operator qulonglong() const             { return toULongLong(); }
    /// Az objektum értékét, a MAC címet stringgé konverálja. Hexa szám byte-onként kettősponttal tagolva.
    /// Nincs ellenörzés, invalid érték esetén a felesleges biteket figyelmen kívül hagyja. Lásd: toString() metódust.
    operator QString() const                { return toString(); }
    /// Ld.: set(qlonglong __mac)
    cMac& operator=(qlonglong __mac)        { return set(__mac); }
    /// Ld.: set(const QString& __mac)
    cMac& operator=(const QString& __mac)   { return set(__mac); }
    /// ld.: set(const QVariant& __mac)
    cMac& operator=(const QVariant& __mac)  { return set(__mac); }
    /// Ha a két MAC azonos (val adattag) true-val tér vissza. A teljes 64 bites értéket hasonlítja össza.
    /// Az értékeket nem ellenörzi.
    bool  operator==(const cMac& __mac) const { return val == __mac.val; }
    /// A teljes 64 bites értéket hasonlítja össza. ( Ha a QMap-ban kulcsként akarjuk használni, ez elég )
    /// Az értékeket nem ellenörzi.
    bool  operator<(const cMac& __mac) const  { return val < __mac.val; }
};

inline static uint qHash(const cMac& mac) { return (uint)qHash(mac.toULongLong()); }
/* ============================================================================================= */
/*!
  Két IPV6 címet bitenként össze és-sel. és az eredménnyel tér vissza
  @relates netAddress
 */
EXT_ Q_IPV6ADDR operator&(const Q_IPV6ADDR& __a, const Q_IPV6ADDR& __b);
/*!
  Két IPV6 címet összehasonlít, ha azonosak, akkor true-val tér vissza
  @relates netAddress
 */
EXT_ bool operator==(const Q_IPV6ADDR& __a, const Q_IPV6ADDR& __b);
/*!
  Két IPV6 címet összehasonlít, ha nem azonosak, akkor true-val tér vissza
 */
inline bool operator!=(const Q_IPV6ADDR& __a, const Q_IPV6ADDR& __b) { return !(__a == __b); }
/*!
  Egy IPV6 címben a felülről számolva az __n-edik bitet negálja.
  @param __a az IPV6 cím referenciája, aminek egy bitjét negálni kell.
  @param __n a bit sorszáma. Ha __n = 0, akkor az a legmagasabb helyiértékű bit.
  @relates netAddress
  Ha az __n értéke negatív, vagy 128 ill. annál nagyobb, akkor __a értéke változatlan marad.
  */
inline void complementOneBit(Q_IPV6ADDR& __a, int __n)
{
    if (__n >= 0 && __n < 128) __a[__n / 8] ^= 0x80 >> (__n % 8);
}

/* ============================================================================================= */
/// Megvizsgálja, hogy az IPV4 cím lehet-e egy maszk.
/// Ha maszk, akkor az 1-es bitek számával, ha nem, akkor -1-el tér vissza, vagy dob egy kizárást
/// @param __a Az IPV4 cím
/// @param __ex Ha értke true, vagy nincs megadva, és __a nem maszk, akkor dob egy kizárást
/// @return Ha a cím maszk, akkor az egyes bitek számával, ha nem, és __ex = false, akkor -1 -el tér vissza.
EXT_ int toIPv4Mask(const QHostAddress& __a, bool __ex = true);

EXT_ bool operator<(const QHostAddress& __a1, const QHostAddress& __a);

EXT_ QString hostAddressToString(const QHostAddress& a);

class netAddressList;
/*!

@class netAddress
@brief Hálózati címtartományt reprezentáló osztály

Az objektum adat tartalmát egy QPair objektum reprezentálja, melynek első eleme
egy QHostAddress objektum, és egy int, ami a hálózati maszkot reprezentálja.
Jelentése, a hálózati maszk hány db 1-es bitaet tartalmaz. Értéke 0-32, ill.
IPV6 cím esetén 0-128.
 */
class LV2SHARED_EXPORT netAddress : protected QPair<QHostAddress, int>
{
    friend class netAddressList;
protected:
    /*!
     Egy referenciát ad vissza az objektum IP cím részéte.
     */
    QHostAddress& addr()    { return first;  }
    /*!
    Egy referenciát ad vissza az objektum maszk részére.
     */
    int&          mask()    { return second; }
    /*!
    Egy referenciát ad vissza az objektum adat tartalmára
     */
    operator QPair<QHostAddress, int>&() { return *((QPair<QHostAddress, int> *)this); }
public:
    /*!
    Konstruktor.
    A cím értéket 0.0.0.0-ra a maszkot -1-re inicializálja. Lásd, a clear() metódust.
     */
    netAddress() : QPair<QHostAddress, int>() { mask() = -1;    }
    /*!
    Konstruktor.
    A megadott címmel, és maszk értékkel inicializálja az objektumot.
    @par[in] __a    IP cím
    @par[in] __m    A maszkban az 1-es bitek száma, opcionális.

    Ha nincs megadva a maszk (vagy értéke -1), akkor a címtartomány hoszt címként lessz definiálva (IPV4-nél 32, IPV6-nál 128).
    Ha a megadott cím nem IPV4 vagy IPV6 cím, akkor a mask -1 -re lessz inicializálva, vagyis ez egyenértékű azzal,
    mintha paraméter nélkül hívtuk volna a konstruktort.
    @see set(const QHostAddress& __a, int __m)
     */
    netAddress(const QHostAddress& __a, int __m = -1) : QPair<QHostAddress, int>() { set(__a, __m); }
    /*!
    Konstruktor.
    A QPair objektumban megadott címmel, és maszk értékkel inicializálja az objektumot.
    @see netAddress(const QHostAddress& __a, int __m)
    @see set(const QHostAddress& __a, int __m)
    */
    netAddress(const QPair<QHostAddress, int>& __p) : QPair<QHostAddress, int>() { set(__p); }
    /*!
    A megadott IPV4 címmel, és maszk értékkel inicializálja az objektumot.
    @par[in] __a IPV4 cím
    @par[in] __m opcionális maszk, alapértelmezett értéke 32
    @see set(quint32 __a, int __m)
     */
    netAddress(quint32 __a, int __m = 32) : QPair<QHostAddress, int>() { set(__a, __m); }
    /*!
    A megadott IPV6 címmel, és maszk értékkel inicializálja az objektumot.
    @par[in] __a IPV6 cím
    @par[in] __m opcionális maszk, alapértelmezett értéke 128
    @see set(const Q_IPV6ADDR& __a, int __m)
     */
    netAddress(const Q_IPV6ADDR& __a, int __m = 128) : QPair<QHostAddress, int>() { set(__a, __m); }
    /*!
    A megadott címmel, és maszk értékkel inicializálja az objektumot. Ha a QString paraméter tartalmazza a
    maszkot is, akkor a második paraméter figyelmen kívül lessz hagyva.
    A cím értelmezésekor cím név feloldás nincsen.
    @par[in] IP cím vagy IP cím tartomány szöveges formában.
    @par[in] opcionális maszk.
    @see set(const QString& __s, int __m)
    */
    netAddress(const QString& __s, int __m = -1) : QPair<QHostAddress, int>() { set(__s, __m); }
    /*!
    Törli az objektum tartalmát. A mask-ot -1 -re állítja.
    A hívás után a feltételvizsgáló metódusok a következő értékekkel fognak visszatérni:
        - isNull()    true
        - isValid()   false
        - isAny()     false
        - isIPV4()    false
        - isIPV6()    false
     */
    netAddress& clear() { mask() = -1; addr().clear(); return *this; }
    /*!
    A megadott címmel, és maszk értékkel újra inicializálja az objektumot.
    @par[in] __a    IP cím
    @par[in] __m    A maszkban az 1-es bitek száma, opcionális.

    Ha nincs megadva a maszk (vagy értéke -1), akkor a címtartomány hoszt címként lessz definiálva (IPV4-nél 32, IPV6-nál 128).
    Ha a megadott cím nem IPV4 vagy IPV6 cím, akkor a mask -1 -re lessz inicializálva, vagyis ez egyenértékű azzal,
    mintha hívtuk volna a clear() metódust.
    Az ip címben a maszk alapján felesleges bitek törlődnek.
     */
    netAddress& set(const QHostAddress& __a, int __m = -1);
    /*!
    A QPair objektumon keresztül megadott címmel, és maszk értékkel újra inicializálja az objektumot.
    @par[in]    __p A címet és maszkot tartalmazó objektum referenciája.

    Működése gyakorlatilag azonos a set(const QHostAddress& __a, int __m) metóduséval, beleértve a maszk érték kezelését is.
     */
    netAddress& set(const QPair<QHostAddress, int>& __p) { return set(__p.first, __p.second); }
    /*!
    IPV4 címtartomány beállítása.
    @par[in] __a Az IPV4 cím 32bites numerikus értékként ábrázolva ( ld.: QHostAddress:setAddress(q1int32) )
    @par[in] __m Opcionális maszk. Alapértelmezett érték 32.

    Az ip címben a maszk alapján felesleges bitek törlődnek.
     */
    netAddress& set(quint32 __a, int __m = 32);
    /*!
    IPV6 címtartomány beállítása.
    @par[in] __a Az IPV6 cím
    @par[in] __m Opcionális maszk. Alapértelmezett érték 128.

    Az ip címben a maszk alapján felesleges bitek törlődnek.
     */
    netAddress& set(const Q_IPV6ADDR& __a, int __m = 128);
    /*!
    A megadott címmel, és maszk értékkel inicializálja az objektumot. Ha a QString paraméter tartalmazza a
    maszkot is, akkor a második paraméter figyelmen kívül lessz hagyva.
    A cím értelmezésekor cím név feloldás nincsen.
    @param[in] IP cím vagy IP cím tartomány szöveges formában.
    @param[in] opcionális maszk.

    Az ip címben a maszk alapján felesleges bitek törlődnek.
     */
    netAddress& set(const QString& __n, int __m = -1);
    /*!
    A megadott címmel, és maszk értékkel inicializálja az objektumot. Ha a QString paraméter tartalmazza a
    maszkot is, akkor a második paraméter figyelmen kívül lessz hagyva.
    A cím értelmezésekor cím név feloldás is van, lsd.: netAddress& setNetByName(const QString& __nn) és;
    netAddress& setAddressByName(const QString& __hn); metódusokat

    @param[in] IP cím vagy IP cím tartomány szöveges formában, hoszt név, vagy net név.
    @param[in] opcionális maszk.

    Az ip címben a maszk alapján felesleges bitek törlődnek.
     */
    netAddress& setr(const QString& __n, int __m = -1);
    /*!
    Módosítja a maszk értéket.
    @param[in] __m Az új maszk érték.

    Ha __m értéke negatív, vagy az objektum nem tartalmaz valós IP címet, akkor a függvény nem csinál semmit.
    Ha __m értéke nagyobb mint 32 ill 128, akkor csökkenti azt az aktuális cím tipus bitszámára.
    Az ip címben az új maszk alapján felesleges bitek törlődnek.
     */
    netAddress& setMask(int __m);
    /*!
      true-val tér vissza, hogyha az IP cím isNull(), és a maszk is 0.
     */
    bool isAny() const      { return mask() == 0 && addr().isNull();  }
    /*!
      true-val tér vissza, ha a cím tartomány egy IPV4 címtartomány.
     */
    bool isIPV4() const     { return mask() >= 0 && mask() <=  32 && addr().protocol() == QAbstractSocket::IPv4Protocol; }
    /*!
      true-val tér vissza, ha a cím tartomány egy IPV6 címtartomány.
     */
    bool isIPV6() const     { return mask() >= 0 && mask() <= 128 && addr().protocol() == QAbstractSocket::IPv6Protocol ; }
    /*!
      true-val tér vissza, ha az IP cím valós IPV4 vagy IPV6 cím, és a maszk nem negytív, vagy nagyobb, mint az
      aktuális cím típus bitszáma.
     */
    bool isValid() const    { return isIPV4() || isIPV6(); }
    /*!
      Az isValid() negáltjával egyenértékü.
     */
    bool isNull() const     { return !isValid(); }
    /*!
      Ha a címtartomány egyetlen IP cím, akkor true-val tér vissza.
     */
    bool isAddress() const  { return (isIPV4() && second == 32) || (isIPV6() && second == 128); }
    /*!
      Ha a címtartomány nem egyetlen IP cím, akkor true-val tér vissza.
     */
    bool isSubnet() const   { return (isIPV4() && second < 32) || (isIPV6() && second < 128); }
    /*!
      Az unáris logikai negálás operátor az isNull() metódus által visszaadott értékel tér vissza.
     */
    bool operator!() const  { return isNull(); }
    /*!
      A logikai típussá konvertáló operátor az isValid() metódus által visszaadott értékel tér vissza.
     */
    operator bool() const   { return isValid(); }
    /*!
      Egy constans referenciát ad vissza az objektum IP cím adatára.
     */
    const QHostAddress& addr() const { return first;  }
    /*!
    Egy constans referenciát ad vissza az objektum maszk adatára.
     */
    const int&          mask() const { return second; }
    /*!
    Egy constans referenciát ad vissza az objektum adat tartalmára.
     */
        operator const QPair<QHostAddress, int>&() const { return *((QPair<QHostAddress, int> *)this); }
    /*!
      Az IPV4 címtartomány maszk értékéhez tartozó 32 bites maszkkal tér vissza.
     */
    static quint32 bitmask(int __m);
    /*!
      Az IPV6 címtartomány maszk értékéhez tartozó 128 bites maszkkal tér vissza (első byte az legnagyobb helyiérték).
     */
    static Q_IPV6ADDR bitmask6(int __m);
    /*!
      Az IPV4 címtartomány maszk értékéhez tartozó 32 bites maszkkal tér vissza.
      Nem viszgálja, hogy az objektum valóban egy IPV4 címaet tartalmaz-e. IPV6 címtartomány estén a visszaadott
      maszk nem értelmezhető.
     */
    quint32 bitmask() const { return bitmask(mask()); }
    /*!
      Az IPV6 címtartomány maszk értékéhez tartozó 128 bites maszkkal tér vissza.
      Nem viszgálja, hogy az objektum valóban egy IPV6 címaet tartalmaz-e. IPV4 címtartomány estén a visszaadott
      maszk 1-es bitekkel ki van egészítve 128 bitre.
     */
    Q_IPV6ADDR bitmask6() { return bitmask6(mask()); }
    /*!
      A két címtartomány metszetét számolja ki. A visszaadott objektum értéke invalid (lásd: isNull() és isValid() ),
      ha a két címtaeromány bármelyike invalid, nem mondkettő IPV4 vagy IPV6 címtartomány, ill. nincs közös metszetük.
     */
    netAddress operator&(const netAddress& __o) const;
    /*!
      A két címtartomány únióját számolja ki. A visszaadott objektum értéke invalid (lásd: isNull() és isValid() ),
      ha a két címtaeromány bármelyike invalid, nem mondkettő IPV4 vagy IPV6 címtartomány, ill. ha a két tartomány
      uniója nem egybefüggő, vagy nem ábrázolható egy netAddress objektummal.
     */
    netAddress operator|(const netAddress& __a) const;
    /*!
      Egy címtartományból kivon egy másik címtartományt. Az eredmény egy cím tartomány lista.
     */
    netAddressList operator-(const netAddress& __a) const;
    /*!
      Törli a IP cím adattagban azokat a biteket, melyek a maszk alapján feleslegesek.
     */
    netAddress& masking();
    /*!
      Stringgé konvertálja a cím tartományt. Ha a címtartomány invalid (lásd: isValid() metódust), akkor
      a föggvény null stringgel tér vissza.
     */
    QString toString() const;
    /*!
      A konverzióhoz a toString() metódust hívja.
     */
    operator QString() const   { return toString(); }
    /*!
      Értékadó operátor. A megfelelő set() metódust hívja.
     */
    netAddress& operator=(const netAddress& __o)            { return set(__o); }
    /*!
      Értékadó operátor. A megfelelő set() metódust hívja.
     */
    netAddress& operator=(const QPair<QHostAddress, int>& __o) { return set(__o); }
    /*!
      Értékadó operátor. A megfelelő set() metódust hívja.
     */
    netAddress& operator=(quint32 __o)                      { return set(__o); }
    /*!
      Értékadó operátor. A megfelelő set() metódust hívja.
     */
    netAddress& operator=(const QString& __o)               { return set(__o); }
    /*!
      A megadott név alapján megpróbálja feloldani a címet, a /etc/networks fájl alapján.
      Ha nincs ilyen nevű címtartomány, akkor hívja a clear() metódust.
     */
    netAddress& setNetByName(const QString& __nn);
    /*!
      Egy hoszt címet állít be a megadott hoszt név alapján. A DNS lekérdezést kivárja. Ha nincs ilyen hoszt név, ill.
      a lekérdezés eredménytelen, akkor hívja a clear() metódust.
     */
    netAddress& setAddressByName(const QString& __hn);
    /*!
      Két objektum adattartalmának a cserélye.
     */
    netAddress& swap(netAddress& __o)   { netAddress t = __o; __o = *this; return *this = t; }
};

/*!
 @def NAL_NOMOD
  Egyes netAddressList metódusok visszatérési értékében jelentése: a lista nem módosult.
  @relates netAddressList
  */
#define NAL_NOMOD           -1
/*!
 @def NAL_APPEND
  Egyes netAddressList metódusok visszatérési értékében jelentése: a lista egy elemmel bővölt
  @relates netAddressList
  */
#define NAL_APPEND          -2
/*!
 @def NAL_MOD
  Egyes netAddressList metódusok visszatérési értékében jelentése: a lista módosult.
  @relates netAddressList
  */
#define NAL_MOD             -3
/*!
 @def NAL_FNDMSK
  Egyes netAddressList metódusok visszatérési értéken egy maszk.
  A visszatérési értékben a nem index jellegű részt vélasztja ki.
  @relates netAddressList
  */
#define NAL_FNDMSK  0x60000000L
/*!
 @def NAL_IDXMSK
  Egyes netAddressList metódusok visszatérési értéken egy maszk.
  A visszatérési értékben az index jellegű részt vélasztja ki.
  @relates netAddressList
  */
#define NAL_IDXMSK  0x1fffffffL
/*!
 @def NAL_FNDHIT
  Egyes netAddressList metódusok visszatérési értékben lelentése:
  A visszatérési értékbe kiválaszva a NAL_FNDMSK részt, keresett tartomány megeggyezik a találttal.
  @relates netAddressList
  */
#define NAL_FNDHIT  0x00000000L
/*!
 @def NAL_FNDLIT
  Egyes netAddressList metódusok visszatérési értékben lelentése:
  A visszatérési értékbe kiválaszva a NAL_FNDMSK részt, keresett tartomány nagyobb/kisebb mint a talált.
  @relates netAddressList
  */
#define NAL_FNDLIT  0x40000000L
/*!
 @def NAL_FNDBIG
  Egyes netAddressList metódusok visszatérési értékben lelentése:
  A visszatérési értékbe kiválaszva a NAL_FNDMSK részt, keresett tartomány nagyobb/kisebb mint a talált.
  @relates netAddressList
  */
#define NAL_FNDBIG  0x20000000L
/*!
 @def NAL_REMREM
  Egyes netAddressList metódusok visszatérési értékben lelentése:
  A visszatérési értékbe kiválaszva a NAL_FNDMSK részt, a megadott indexű elem törölve lett.
  @relates netAddressList
  */
#define NAL_REMREM  0x40000000L
/*! @def NAL_REMMOD
  Egyes netAddressList metódusok visszatérési értékben lelentése:
  A visszatérési értékbe kiválaszva a NAL_FNDMSK részt, a megadott indexű elem módosítva lett.
  @relates netAddressList
  */
#define NAL_REMMOD  0x20000000L

/*! @def NAL_CHKINDEX
  Index értékének az ellenörzése egy netAddressList metódusban.
  @exception Ha a megadott index értéke nem megfelelő.
 */
#define NAL_CHKINDEX(ix)   if (0 > i || vec.count() <= i) { \
    cError *e = new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ENOINDEX, ix); \
    e->exception(); \
}

/*!

 @class netAddressList
  Cím tartomány lista / cím (tartomány) halmaz.
 */
class LV2SHARED_EXPORT netAddressList {
protected:
    /*!
      A netAddress konténer.
     */
    QVector<netAddress>   vec;
    /*!
      A lista megadott indexű elemének a referenciájának a lekérése.
      @exception cError* Ha az index nem egy valós elemre mutat.
     */
    netAddress&    operator[](int i)       { NAL_CHKINDEX(i); return vec[i]; }
public:
    /*!
      Könstruktor. Üres lista ill. halmaz létrehozása.
     */
    netAddressList()                        : vec()       { ; }
    /*!
      Könstruktor. Egy elemű lista ill. eggy címtartományt tartalmazó halmaz létrehozása.
     */
    netAddressList(const netAddress& __a)   : vec(__a)    { ; }
    /*!
      Másolat konstruktor.
     */
    netAddressList(const netAddressList& __a)   : vec(__a)    { ; }
     /*!
      Hozzáad a listához egy címtartományt.
      @return
        - NAL_NOMOD  a lista nem módosult, egy megkévő elem lefedte a tartományt
        - NAL_APPEND a listához hozzá lett füzve az új elem
        - NAL_MOD a lista megváltozott (nem egy eleme, és nem bővült)

     Ha a visszatérési érték pozitív szám, ill. nulla. akkor az a módosult elem indexe.
     */
    int  add(const netAddress& __n);
    /*!
     Hozzáad a listához egy címtartományt, melyet a paraméterek alapján konstruált.
     @see add(const netAddress& __n)
     @see netAddress(const QHostAddress& __a, int __m)
     */
    int  add(const QHostAddress& __a, int __m = -1) { return add(netAddress(__a, __m)); }
    /*!
     Hozzáad a listához egy címtartományt, melyet a paraméterek alapján konstruált.
     @see add(const netAddress& __n)
     @see netAddress(const QString& __n)
     */
    int  add(const QString& __n)                    { return add(netAddress(__n)); }
    /*!
     Hozzáad a listához egy címtartományt, melyet a paraméterek alapján konstruált.
     @see add(const netAddress& __n)
     @see netAddress(const QPair<QHostAddress, int>& __p)
     */
    int  add(const QPair<QHostAddress, int>& __p)   { return add(netAddress(__p)); }
    /*!
     Hozzáad a listához egy címtartományt, melyet a paraméterek alapján konstruált.
     @see add(const netAddress& __n)
     @see netAddress(quint32 __a, int __m)
     */
    int  add(quint32 __a, int __m = 32)             { return add(netAddress(__a, __m)); }
    /*!
     Hozzáad a listához egy címtartományt, melyet a paraméterek alapján konstruált.
     @see add(const netAddress& __n)
     @see netAddress(const QHostAddress& __a, int __m)
     */
    bool add(const Q_IPV6ADDR& __a, int __m = 128)  { return add(netAddress(__a, __m)); }
    /*!
      Hozzáad a listához/halmazhoz egy IP cím listát
      A két összefűzött listára hívja a compress() metódust.
     */
    bool add(const QList<QHostAddress>& __al);
    /*!
      Hozzáad a listához/halmazhoz egy egy másik listát ill. halmazt.
      A két összefűzött listára hívja a compress() metódust.
     */
    bool add(const netAddressList& __al);
    /*!
      Megkusérli a listát minnél tömörebb formára hozni.
      Az összevonható tartományokat összevonja, azokat a tartományokat, melyek részei más listában szereplő
      tartománynak, azokat törli. Az eredmény lista nem tartalmaz egymást átfedő tartományt.
      @return Ha változott a lista, akkor true, ha nem, akkor false.

      Halmazelméketi szempontból a metódus nem változtatja meg a halmaz tartalmát, csak a listát módosítja/ egyszrűsíti.
     */
    bool compress();
    /*!
      A lista utolsó  netAddress elemével tér vissza.
      @exception cError* Ha üres a lista.
     */
    netAddress pop();
    /*!
      A listában szereplő hálózat cimek számával (lista elemszám) tér vissza
     */
    int size() const        { return vec.size(); }
    /*!
      Logikai értékkeé konvertálás. Értéke ha a lista ill. a halmaz üres, akkor false, egyébkét true.
     */
    operator bool() const   { return vec.size() != 0; }
    /*!
      Logikai operátor. Ha a halmaz ill. a lista üres, akkor true-val tér vissza, egyébként false-val.
     */
    bool operator !() const { return vec.size() == 0; }
    /*!
      A lista elemeket egyenként stringgé konvertálja, és a stringekből készült listával tér vissza.
     */
    QStringList toStringList() const;
    /*!
      A lista megadott indexű eleme constans referenciájának a lekérése.
      @exception cError* Ha az index nem egy valós elemre mutat.
     */
    const netAddress& operator[](int i) const { NAL_CHKINDEX(i); return vec[i]; }
    /*!
      Értékadó operátor.
     */
    netAddressList& operator=(const netAddressList& __o) { vec = __o.vec; return *this; }
    /*!
     Megkeres egy címtartományt.
     @param __a A keresett címtartomány.
     @return a visszadott érték -1, ha nincs találat, egíébként egy konstans + a taéűlt elem indexe. A konstans
     értékek:
        - NAL_FNDHIT  A kereset tartomány a talát tartománnyal megegyezik
        - NAL_FNDLIT  A kereset tartomány a talát tartománynál kisebb
        - NAL_FNDBIG  A kereset tartomány a talát tartománynál nagyobb, (index az első találatra!)
     */
    int find(const netAddress& __a) const;
    /*!
     A megadott indexű elemet törli a listából.
     */
    int remove(int i) { NAL_CHKINDEX(i); vec.remove(i); return NAL_REMREM | i; }
    /*!
     Elvesz a halmazból egy címtartományt.
     @return
       - NAL_NOMOD  a lista nem módosult, egy meglévő elemel sem volt nem üres metszete a megadott tartománynak
       - NAL_MOD a lista megváltozott (nem egy eleme)
       - NAL_REMREM + i csak egy listaelem lett törölve, annak indexe i volt.
       - NAL_REMMOD + i Csak egy elemet kellet módosítani, annak indexe i.
    */
    int remove(const netAddress& __a);
    ///
    QString toString() const;
    ///
    netAddressList& operator<<(const netAddress& a) { add(a); return *this; }
};

/* ------------------------------------------------------------------ */
/*!
  @def TSTREAMO(type)
  QTextStream objektumba való kiírás inline operátorát definiáló makró,
  A definiált inline fügvény a megadott típusú objektum toString() metódusával stringé konvertálja
  az objektumkéldány értékét, majd ez küldi a QTextStream objektumba.
  @param type Az objektum típus, amihez a '<<' operátort definiálni kell.
  */
#define TSTREAMO(type)  static inline QTextStream& operator<<(QTextStream& __t, const type& __v) { return __t << __v.toString(); }
/*!
  @def TSTREAMF(type)
  QTextStream objektumba való kiírás inline operátorát definiáló makró,
  A definiált inline fügvény a megadott típusú objektum paraméterként váró függvénnyel stringé konvertálja
  az objektumkéldány értékét, majd ez küldi a QTextStream objektumba.
  @param type Az objektum típus, amihez a '<<' operátort definiálni kell.
  @param fn A stringgé konvertálást végző függvény neve (vagy egy olyan konvertáló fügvényé, mely már közvetlenül kiíratható a \<\< operaátorral).
  */
#define TSTREAMF(type, fn)  static inline QTextStream& operator<<(QTextStream& __t, const type& __v) { return __t << fn(__v); }


TSTREAMO(netAddressList)
TSTREAMO(netAddress)
TSTREAMO(QHostAddress)
TSTREAMO(cMac)



typedef QPair<QString, QString>     QStringPair;
/// A QPolygonF osztály a GUI része, így nem használlható
/// Mert vagy nem GUI-val fordítjuk, és akkor elszállhat (pl. a QVariant-ból való kikonvertáláskor)
/// Vagy GUI-val fordítunk, de akkor meg nem fog elindulni tisztán konzolos módban
typedef QList<QPointF>  tPolygonF;

/// Egész szám (int/index) vektor
typedef QVector<int>        tIntVector;
/// Egész szám (qlonglong/id) vektor
typedef QVector<qlonglong>  tLongLongVector;

/// Egy egész szám (int) vektor előállítása.
/// A vektornak annyi eleme lesz, ahány paramétert megadtunk (max 8 elem), az első NULL_IX érték utáni elemek
/// figyelmen kívül lesznek hagyva. A vektor elemei a paraméterek értékeivel lesznek feltöltve.
/// @return egy maximum 8 elemű egész szám vektor. Ahol az elemek (ha vannak) első = a, második = b, stb.
/// @relates cRecord
EXT_ tIntVector   iTab(int a = NULL_IX, int b = NULL_IX, int c = NULL_IX, int d = NULL_IX, int e = NULL_IX, int f = NULL_IX, int g = NULL_IX, int h = NULL_IX);

/* ------------------------------------------------------------------ */

Q_DECLARE_METATYPE(tPolygonF)
Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(cMac)
Q_DECLARE_METATYPE(netAddress)
Q_DECLARE_METATYPE(netAddressList)

// EXT_ int _UMTID_QPoint;
// EXT_ int _UMTID_QPointF;
EXT_ int _UMTID_tPolygonF;
EXT_ int _UMTID_QHostAddress;
EXT_ int _UMTID_cMac;
EXT_ int _UMTID_netAddress;
EXT_ int _UMTID_netAddressList;

EXT_ void initUserMetaTypes();
EXT_ QString tIntVectorToString(const tIntVector& __iv);
EXT_ QString QBitArrayToString(const QBitArray& __ba);
EXT_ QString QSqlRecordToString(const QSqlRecord& __r);
EXT_ QString QStringListToString(const QStringList& _v);
EXT_ QString QVariantListToString(const QVariantList& _v, bool *pOk = NULL);
EXT_ QString QPointTosString(const QPoint& p);
EXT_ QString QPointFTosString(const QPointF& p);
EXT_ QString tPolygonFToString(const tPolygonF& pol);
EXT_ QString QVariantToString(const QVariant& _v, bool *pOk = NULL);

TSTREAMF(tIntVector,  tIntVectorToString)
TSTREAMF(QBitArray,   QBitArrayToString)
TSTREAMF(QSqlRecord,  QSqlRecordToString)
TSTREAMF(QStringList, QStringListToString)
TSTREAMF(QVariantList,QVariantListToString)
TSTREAMF(QPoint,      QPointTosString)
TSTREAMF(QPointF,     QPointFTosString)
TSTREAMF(tPolygonF,   tPolygonFToString)

EXT_ QString debVariantToString(const QVariant v);

#endif //LV2TYPES_H_INCLUDED
