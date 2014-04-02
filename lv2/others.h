#ifndef OTHERS_H
#define OTHERS_H

#include "lv2_global.h"
#include <QVariant>
#include <QMap>
#include <QBitArray>

/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak egy megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i Az 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
static inline QBitArray _mask(int s, int i) { QBitArray m(s); m.setBit(i); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak két megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i1 Az egyik 1-be álítandó bit indexe
/// @param i2 Az másik 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
static inline QBitArray _mask(int s, int i1, int i2) { QBitArray m = _mask(s, i1); m.setBit(i2); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak három megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i1 Az egyik 1-be álítandó bit indexe
/// @param i2 Az másik 1-be álítandó bit indexe
/// @param i3 A további 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
static inline QBitArray _mask(int s, int i1, int i2, int i3) { QBitArray m = _mask(s, i1, i2); m.setBit(i3); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak négy megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i1 Az egyik 1-be álítandó bit indexe
/// @param i2 Az másik 1-be álítandó bit indexe
/// @param i3 Az harmadik 1-be álítandó bit indexe
/// @param i4 Az negyedik 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
static inline QBitArray _mask(int s, int i1, int i2, int i3, int i4) { QBitArray m = _mask(s, i1, i2, i3); m.setBit(i4); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és az utolsó
/// bitjét 1-be állítja.
/// @param i Az 1-be álítandó bit indexe. (Tömb méret i +1)
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
static inline QBitArray _bit(int i) { return _mask(i+1,i); }

/// Az enumerációs értéket a set-ben reprezentáló bit visszaadása.
static inline qlonglong enum2set(int e) { return ((qlonglong)1) << e; }
static inline qlonglong enum2set(int e1, int e2) { return enum2set(e1) | enum2set(e2); }
static inline qlonglong enum2set(int e1, int e2, int e3) { return enum2set(e1, e2) | enum2set(e3); }
static inline qlonglong enum2set(int e1, int e2, int e3, int e4) { return enum2set(e1, e2, e3) | enum2set(e4); }
static inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5) { return enum2set(e1, e2, e3, e4) | enum2set(e5); }
static inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5, int e6) { return enum2set(e1, e2, e3, e4, e5) | enum2set(e6); }
static inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5, int e6, int e7) { return enum2set(e1, e2, e3, e4, e5, e6) | enum2set(e7); }
static inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5, int e6, int e7, int e8) { return enum2set(e1, e2, e3, e4, e5, e6, e7) | enum2set(e8); }

static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e, bool __ex = true) {
    int i = f(e, __ex);
    if (i < 0) return -1;
    return enum2set(i);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, bool __ex = true) {
    return enum2set(f, e1, __ex) | enum2set(f, e2, __ex);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, const QString& e3, bool __ex = true) {
    return enum2set(f, e1, e2, __ex) | enum2set(f, e3, __ex);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4, bool __ex = true) {
    return enum2set(f, e1, e2, e3, __ex) | enum2set(f, e4, __ex);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, bool __ex = true) {
    return enum2set(f, e1, e2, e3, e4, __ex) | enum2set(f, e5, __ex);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, const QString& e6, bool __ex = true) {
    return enum2set(f, e1, e2, e3, e4, e5, __ex) | enum2set(f, e6, __ex);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, const QString& e6, const QString& e7, bool __ex = true) {
    return enum2set(f, e1, e2, e3, e4, e5, e6, __ex) | enum2set(f, e7, __ex);
}
static inline qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, const QString& e6, const QString& e7, const QString& e8, bool __ex = true) {
    return enum2set(f, e1, e2, e3, e4, e5, e6, e7, __ex) | enum2set(f, e8, __ex);
}
qlonglong enum2set(int (&f)(const QString& e, bool __ex), const QStringList& el, bool __ex = true);


/// @def ENUM2SET
///
#define ENUM2SET(n)              (1LL << (n))
#define ENUM2SET2(n,n2)         ((1LL << (n)) | (1LL << (n2)))
#define ENUM2SET3(n,n2,n3)      ((1LL << (n)) | (1LL << (n2)) | (1LL << (n3)))
#define ENUM2SET4(n,n2,n3,n4)   ((1LL << (n)) | (1LL << (n2)) | (1LL << (n3)) | (1LL << (n4)))

/// Megvizsgálja hogy a set ként értelmezett s bitmap-ban az e-vel reprezentált enumerációs érték be van-e állítva
inline static bool isOn(qlonglong s, int e)   { return 0 != (s & enum2set(e)); }

/// Egy pointer felszabadítása, és a pointer változó nullázása, feltéve ha a pointer nem NULL.
template <class T> void pDelete(T *& p) {
    if (p != NULL) {
        delete p;
        p = NULL;
    }
}

/// Megvizsgálja, hogy egy konténer típusú objektumnak lehet-e a méret alapján az ix.
/// @return Ha a cont objektumnak a mérete alapján lez ix indexe, akkor true.
template <class T> bool isContIx(const T& cont, qlonglong ix) {
    return ix >= 0 && ix < cont.size();
}

/// Egy számozott név előállítása
/// @param __pat Egy minta. Ha tartalmaz helyettesítő kataktereket, akkor az első és utolsó helyettesítő karakterrel lefedett
///            területre másolja a sorszámot. A számot kiegészsíti 0 karakterekkel.
///            Ha nem elég a hely, akkor is beillesztődik a teljes szám, és ekkor hosszabb lesz az eredmény string
///            Ha a mintában nincs helyettesítő karakter, akkor csak a minta után lessz másolva a szám
/// @param __num A stringben elhelyezendő szám
/// @param __c   Helyettesítő karakter. Opcionális alapértelmezése a '?' karakter.
/// @return Az eredmény stringgel tér vissza
EXT_ QString nameAndNumber(const QString& __pat, int __num, char __c = '?');

/// Konténer típus a properties mező értelmezéséhez
typedef QMap<QString, QString>                     tMagicMap;
typedef QMap<QString, QString>::iterator           tMagicMapIteraeor;
typedef QMap<QString, QString>::const_iterator     tMagicMapConstIteraeor;

/// Egy magic stringet szétvág és egy QMap-ot tölt föl az elemeivel
/// A konténer kiindulási értéke a második opcionális paraméter,
/// A magic string paramétereket és értékeket tartalmazó string, a kezdő és záró karakter, valamint a szaeparátor
/// a kettőspont. A paraméter név és érték szeparátor az egyenlőségjel. Pl.:
/// ':par1:par2=val2:par3:par4=val4:
/// @param __ms A feldolgozandó magic string
/// @param __map A konténer kiindulási értéke
/// @param __ex Nem megfelelő formátumú string esetén kizárást dob.
/// @return Az eredmény konténer
EXT_ tMagicMap splitMagic(const QString& __ms, const tMagicMap& __map, bool __ex = true);
/// Egy magic string elemból kiemeli a megadott nevű paraméter stringet, feltételezve, hogy csak egy adott nevű elem van
/// @param _nm A paraméter neve
/// @param __map A magic string szétvágásakor keletkező konténer.
/// @return Egy string, a paraméter érték, ha nincs ijen paraméter, akkor a NULL string, ha nincs paramétere, akkor egy üres string
EXT_ const QString& magicParam(const QString& _nm, const tMagicMap __map);
/// A megadott __map konténerben keres egy a __k kulcshoz tartozó attributum nevet.
/// A konténerben talállható tulajdonság nevekről az esetleges paramétereket leválasztja az összehasonlítás elött
/// @param _nm A tulajdonság név (paraméterek nélkül)
/// @param __map A konténer referenciája
/// @return true, ha megtalálta az adott tulajdonságú elemet, egyébként false.
static inline bool findMagic(const QString& _nm, const tMagicMap __map) { return __map.contains(_nm); }
/// A properties mező értékének az előállítása a megadott konténer alapján.
/// @param __map a forrás konténer
/// @return Az eredmény string objektummal tér vissza
EXT_ QString joinMagic(const tMagicMap __map);
/// Egy QVariant érték konvertálása numerikussá (qlonglong).
/// @param v A konvertálandó adat, ha nem konvertálható (és nem null) akkor dob egy kizárást.
/// @param _ex Ha értéke true, akkor a NULL érték hibát dob, egyébként a def vagy annak alapértelmezett értékével NULL:ID-vel tér vissza.
/// @param def Ha engedélyezett a null érték, akkor ezzrl térvissza, ha nincs megadva, akkor NULL:ID-vel
/// @return A konvertált vagy alapértelmezett érték.
EXT_ qlonglong variantToId(const QVariant& v, bool _ex = true, qlonglong def = NULL_ID);

/// Egy QString érték konvertálása numerikussá (qlonglong).
/// @param v A konvertálandó adat, ha nem konvertálható (és nem null, ill. empty) akkor dob egy kizárást.
/// @param _ex Ha értéke true, akkor a NULL érték hibát dob, egyébként a def vagy annak alapértelmezett értékével NULL:ID-vel tér vissza.
/// @param def Ha engedélyezett a null érték, akkor ezzrl térvissza, ha nincs megadva, akkor NULL:ID-vel
/// @return A konvertált vagy alapértelmezett érték.
EXT_ qlonglong stringToId(const QString& v, bool _ex = true, qlonglong def = NULL_ID);

/// Két szám legnagyobb közös osztója. Negatív paraméterek esetén dob egy kizárást.
EXT_ qlonglong lko(qlonglong a, qlonglong b);

EXT_ QString getEnvVar(const char * cnm);
EXT_ QString getSysError(int eCode);

template <typename T> struct PtrLess
{
    bool operator()(const T* a, const T* b) const {
        return *a < *b;
    }
};

EXT_ void appReStart();

/* ******************************  ****************************** */

/// @class no_init_
/// @brief Üres dummy osztály
/// Adattal és metódusokkal, hogy ne ugasson a fordító, amikor használom.
class LV2SHARED_EXPORT no_init_ {
public:
    no_init_() { dummy = 0; }
    no_init_& operator=(const no_init_& __o) { dummy = __o.dummy; return *this; }
private:
    char    dummy;
};
/// Egy dummy osztály dummy példány, hogy lehessen referencia az osztályra.
extern no_init_ _no_init_;


#endif // OTHERS_H
