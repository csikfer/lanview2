#ifndef OTHERS_H
#define OTHERS_H

#include "lv2_global.h"
#include <QVariant>
#include <QMap>
#include <QBitArray>
#include <QFile>
#include <QBitArray>

inline bool isNumNull(const QVariant v)
{
    int t = v.userType();
    return (v.isNull()
         || (QMetaType::LongLong == t && NULL_ID == v.toLongLong())
         || (QMetaType::Int      == t && NULL_IX == v.toInt())
    );
}

/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak egy megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i Az 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
inline QBitArray _mask(int s, int i) { QBitArray m(s); m.setBit(i); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak két megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i1 Az egyik 1-be álítandó bit indexe
/// @param i2 Az másik 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
inline QBitArray _mask(int s, int i1, int i2) { QBitArray m = _mask(s, i1); m.setBit(i2); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak három megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i1 Az egyik 1-be álítandó bit indexe
/// @param i2 Az másik 1-be álítandó bit indexe
/// @param i3 A további 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
inline QBitArray _mask(int s, int i1, int i2, int i3) { QBitArray m = _mask(s, i1, i2); m.setBit(i3); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és annak négy megadot
/// bitjét 1-be állítja.
/// @param s A bit tömb mérete
/// @param i1 Az egyik 1-be álítandó bit indexe
/// @param i2 Az másik 1-be álítandó bit indexe
/// @param i3 Az harmadik 1-be álítandó bit indexe
/// @param i4 Az negyedik 1-be álítandó bit indexe
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
inline QBitArray _mask(int s, int i1, int i2, int i3, int i4) { QBitArray m = _mask(s, i1, i2, i3); m.setBit(i4); return m; }
/// Létrehoz egy megadott méretű csupa 0 elemű bit tömböt, és az utolsó
/// bitjét 1-be állítja.
/// @param i Az 1-be álítandó bit indexe. (Tömb méret i +1)
/// @return A megkreállt bit tömb objektum
/// @related cRecStaticDescr
inline QBitArray _bit(int i) { return _mask(i+1,i); }
/// Létrehoz egy max(i1,i2) +1 méretű csupa 0 elemű bit tömböt, és az
/// i1, i2 indexű bitjeit 1-be állítja.
/// @param i1 Az 1-be álítandó bit indexe.
/// @param i2 Az 1-be álítandó bit indexe.
/// @return A megkreállt bit tömb objektum
inline QBitArray _bits(int i1, int i2) { return _mask(qMax(i1, i2) +1,i1, i2); }

/// Az enumerációs értéket a set-ben reprezentáló bit visszaadása.
inline qlonglong enum2set(int e) { return (qlonglong(1)) << e; }
inline qlonglong enum2set(int e1, int e2) { return enum2set(e1) | enum2set(e2); }
inline qlonglong enum2set(int e1, int e2, int e3) { return enum2set(e1, e2) | enum2set(e3); }
inline qlonglong enum2set(int e1, int e2, int e3, int e4) { return enum2set(e1, e2, e3) | enum2set(e4); }
inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5) { return enum2set(e1, e2, e3, e4) | enum2set(e5); }
inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5, int e6) { return enum2set(e1, e2, e3, e4, e5) | enum2set(e6); }
inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5, int e6, int e7) { return enum2set(e1, e2, e3, e4, e5, e6) | enum2set(e7); }
inline qlonglong enum2set(int e1, int e2, int e3, int e4, int e5, int e6, int e7, int e8) { return enum2set(e1, e2, e3, e4, e5, e6, e7) | enum2set(e8); }

inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e, enum eEx __ex = EX_ERROR) {
    int i = f(e, __ex);
    if (i < 0) return -1;
    return enum2set(i);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, __ex) | enum2set(f, e2, __ex);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, const QString& e3, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, e2, __ex) | enum2set(f, e3, __ex);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, e2, e3, __ex) | enum2set(f, e4, __ex);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, e2, e3, e4, __ex) | enum2set(f, e5, __ex);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, const QString& e6, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, e2, e3, e4, e5, __ex) | enum2set(f, e6, __ex);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, const QString& e6, const QString& e7, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, e2, e3, e4, e5, e6, __ex) | enum2set(f, e7, __ex);
}
inline qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QString& e1, const QString& e2, const QString& e3, const QString& e4,
                                 const QString& e5, const QString& e6, const QString& e7, const QString& e8, enum eEx __ex = EX_ERROR) {
    return enum2set(f, e1, e2, e3, e4, e5, e6, e7, __ex) | enum2set(f, e8, __ex);
}
EXT_ qlonglong enum2set(int (&f)(const QString& e, eEx __ex), const QStringList& el, enum eEx __ex = EX_ERROR);
EXT_ QStringList set2lst(const QString&(&f)(int e, eEx __ex), qlonglong _set, enum eEx __ex = EX_ERROR);


/// @def ENUM2SET
///
#define ENUM2SET(n)                  (1LL << (n))
#define ENUM2SET2(n,n2)             ((1LL << (n)) | (1LL << (n2)))
#define ENUM2SET3(n,n2,n3)          ((1LL << (n)) | (1LL << (n2)) | (1LL << (n3)))
#define ENUM2SET4(n,n2,n3,n4)       ((1LL << (n)) | (1LL << (n2)) | (1LL << (n3)) | (1LL << (n4)))
#define ENUM2SET5(n,n2,n3,n4,n5)    ((1LL << (n)) | (1LL << (n2)) | (1LL << (n3)) | (1LL << (n4)) | (1LL << (n5)))

/// Megvizsgálja hogy a set ként értelmezett s bitmap-ban az e-vel reprezentált enumerációs érték be van-e állítva
inline bool isOn(qlonglong s, int e)   { return 0 != (s & enum2set(e)); }

EXT_ int onCount(qlonglong _set);

/// Egy pointer felszabadítása, és a pointer változó nullázása, feltéve ha a pointer nem NULL.
template <class T> void pDelete(T *& p) {
    if (p != nullptr) {
        delete p;
        p = nullptr;
    }
}

/// Megvizsgálja, hogy egy konténer típusú objektumnak lehet-e a méret alapján az ix.
/// @return Ha a cont objektumnak a mérete alapján lehet ix indexe, akkor true.
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

EXT_ QStringList splitBy(const QString& s, const QChar& sep = QChar(':'), const QChar& esc = QChar('\\'));

/// Egy listából eltávolitja az azonos stringeket, nem nagybetű érzékeny.
/// @param sl A modosítandó lista
/// @param s az eltávolítandó értékek
/// @return az eltávolított elemek száma
EXT_ int removeAll(QStringList& sl, const QString& s);

class cRecordFieldRef;

/// @class cFeatures
/// \brief A features mezőt kezelő osztály. A kulcs a metódusokban mindíg kisbetüssé van konvertálva.
class LV2SHARED_EXPORT cFeatures : public tStringMap
{
public:
    cFeatures()                    : tStringMap()  { }
    cFeatures(const tStringMap& m) : tStringMap(m) { }
    /// Egy magic stringet szétvág és objektumot feltölti, ill. összefésüli az elemeivel
    /// A magic string paramétereket és értékeket tartalmazó string, a kezdő és záró karakter, valamint a szaeparátor
    /// a kettőspont. A paraméter név és érték szeparátor az egyenlőségjel. Pl.:
    /// ':par1:par2=val2:par3:par4=val4:
    /// Ha a paraméter érték a felkiáltójel, akkor az adott kulcs törölve lessz.
    /// @param __ms A feldolgozandó paraméter string
    /// @param __ex Nem megfelelő formátumú string esetén kizárást dob.
    /// @return true, ha nincs hiba. Hiba esetén (ha __ex = EX_IGNORE) false.
    bool split(const QString& __ms, bool merge = false, enum eEx __ex = EX_ERROR);
    /// Kiemeli a megadott nevű paraméter ártéket.
    /// @param _nm A paraméter neve (kisbetüssé lesz konvertálva)
    /// @return Egy string, a paraméter érték, ha nincs ilyen paraméter, akkor a NULL string,
    ///         ha viszont nincs paraméternek értéke, akkor egy üres string
    QString value(const QString &_nm) const { return tStringMap::value(_nm.toLower()); }
    /// Kiemeli a megadott nevű paraméter ártéket, és string listaként értelmezi
    /// @param _nm A paraméter neve (kisbetüssé lesz konvertálva)
    /// @return Egy string lista, a paraméter érték, ha nincs ilyen paraméter, vagy üres, akkor üres lista.
    QStringList slValue(const QString &_nm) const { return value2list(value(_nm)); }
    /// Egy listát tartalmazó string darabolása. A lista szeparátor a vessző, ha a szeparátor elött, vagy után
    /// space karakter van, akkor azt a space karaktert eldobja a metódus.
    /// @param s A darabolandó string.
    /// @return A szét darabolt stringgek litája
    static QStringList value2list(const QString &s);
    static QString list2value(const QStringList& l);
    /// Egy lista konverziója egy értékké. A lista szeparátor a ',' karakter (list_sep), ha a lista elem tartalmaz
    /// ',' karaktert, akkor azt '\,' -el helyettesíti (lis_esep).
    /// @param l A listát tartalmazó konténer
    /// @param f Ha értéke igaz, és az l konténer csak egy elemű, akkor a ', ' karakterek elé nem szúrja be a '\' karaktert.
    static QString list2value(const QVariantList& l, bool f = false);
    /// Egy listát tartalmazó string darabolása. A lista szeparátor a vessző, ha a szeparátor elött, vagy után
    /// space karakter van, akkor azt a space karaktert eldobja a metódus.
    /// A kapott lista elemeket enumerációs értékeknek tekintve (a listát set-nek) összeállít egy maszk értéket.
    /// @param s A darabolandó string.
    /// @param enums Az enumerációs értékek, sorrendben.
    /// @return A maszk
    static qlonglong value2set(const QString &s, const QStringList& enums);
    qlonglong eValue(const QString &_nm, const QStringList& enums) const;
    static tStringMap value2map(const QString& _s);
    static QString map2value(const tStringMap& _map);
    static QMap<QString, QStringList> value2listMap(const QString &s);
    static QMap<int, qlonglong> mapEnum(QMap<QString, QStringList> smap, const QStringList& enums);
    QMap<QString, QStringList> sListMapValue(const QString &_nm) const;
    QMap<int, qlonglong> eMapValue(const QString &_nm, const QStringList& enums) const;
    /// Megkeresi a megadott nevű paramétert.
    /// @param _nm A paraméter neve (kisbetüssé lesz konvertálva)
    /// @return Ha létezik ilyen nevű paraméter (értéke lehet üres, akkor true
    bool contains(const QString &_nm) const { return tStringMap::contains(_nm.toLower()); }
    /// A features mező értékének az előállítása.
    /// @param _sm a forrás konténer
    /// @return Az eredmény string
    static QString join(const tStringMap& _sm);
    QString join() const { return join(*static_cast<const tStringMap *>(this)); }
    /// Összefésül két objektumot.
    /// Ha megadtuk az _cKey kulcsot, akkor csak azok a mezők kerülnek az objektumba, melyeknek a kulcsa
    /// a megadott kulcssal, és egy pont karakterrel kezdödnek, ekkor a kulcsból ez az előteag tölődik, és
    /// így kerül az objektumba. Ha a _o -ban egy (modosított)kulcsértékhez a "!" van rendelve,
    /// akkor az ez alatti érték törlődik az objektumban.
    cFeatures& merge(const cFeatures &_o, const QString& _cKey = QString());
    cFeatures& selfMerge(const QString& _cKey);
    void modifyField(cRecordFieldRef& _fr);
    static const QString list_sep;
    static const QString list_esep;
};

/// Egy QVariant érték konvertálása numerikussá (qlonglong).
/// @param v A konvertálandó adat, ha nem konvertálható (és nem null) akkor dob egy kizárást.
/// @param _ex Ha értéke true, akkor a NULL érték hibát dob, egyébként a def vagy annak alapértelmezett értékével NULL:ID-vel tér vissza.
/// @param def Ha v null, akkor ezzel térvissza, ha nincs megadva, akkor NULL_ID-vel
/// @return A konvertált vagy alapértelmezett érték.
EXT_ qlonglong variantToId(const QVariant& v, enum eEx __ex = EX_ERROR, qlonglong def = NULL_ID);

/// Egy QString érték konvertálása numerikussá (qlonglong).
/// @param v A konvertálandó adat, ha nem konvertálható (és nem null, ill. empty) akkor dob egy kizárást.
/// @param _ex Ha értéke true, akkor a NULL érték hibát dob, egyébként a def vagy annak alapértelmezett értékével NULL:ID-vel tér vissza.
/// @param def Ha engedélyezett a null érték, akkor ezzel térvissza, ha nincs megadva, akkor NULL_ID-vel
/// @return A konvertált vagy alapértelmezett érték.
EXT_ qlonglong stringToId(const QString& v, enum eEx __ex = EX_ERROR, qlonglong def = NULL_ID);

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

template <typename T> struct PtrGreat
{
    bool operator()(const T* a, const T* b) const {
        return *a > *b;
    }
};

LV2_ATR_NORET_ EXT_ void appReStart();

EXT_ const QString _sSpace;
inline QString joinCmd(const QString& cmd, const QStringList& args)
{
    return cmd + _sSpace + args.join(_sSpace);
}
#define PROCESS_START_TO    2000
#define PROCESS_SOPP_TO    10000
enum eProcError {
    PE_ERROR = -1,
    PE_START_TIME_OUT = -2,
    PE_STOP_TIME_OUT = -3,
};
EXT_ int startProcessAndWait(QProcess& p, const QString& cmd, const QStringList& args, QString *pMsg = nullptr, int start_to = PROCESS_START_TO, int stop_to = PROCESS_SOPP_TO);
EXT_ int startProcessAndWait(const QString& cmd, const QStringList& args, QString *pMsg = nullptr, int start_to = PROCESS_START_TO, int stop_to = PROCESS_SOPP_TO);

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
EXT_ no_init_ _no_init_;

/* ******************************  ****************************** */

EXT_ void writeRollLog(QFile& __log, const QByteArray& __data, qlonglong __size, int __arc);

template <class C, class T> T average(C c) {
    T r;
    foreach (T t, c) {
        r += t;
    }
    return r / c.size();
}

/* ******************************  ****************************** */

EXT_ QString hrmlFrame(const QString &title, const QString& body);

/// Paraméter név
EXT_ QString getParName(QString::const_iterator& i, const QString::const_iterator& e, bool _point = true, enum eEx __ex = EX_ERROR);

/// Két üzenet szövegének az összefűzése
inline QString msgCat(const QString msg1, const QString& msg2, const QString& sep = _sSpace)
{
    QString r = msg1;
    if ((!msg1.isEmpty()) && (!msg2.isEmpty())) r += sep;
    r += msg2;
    return r;
}

inline const QString& msgAppend(QString *pMsg, const QString& m)
{
    if (m.isEmpty()) return m;
    if (pMsg != nullptr) {
        if (!pMsg->isEmpty()) *pMsg += QChar('\n');
        *pMsg += m;
    }
    return m;
}
/* ******************************  ****************************** */
EXT_ QVariantList list_longlong2variant(const QList<qlonglong>& v);

/* ****************************** CSV ****************************** */

class QTextStream;
class cCommaSeparatedValues;
EXT_ cCommaSeparatedValues&  endl(cCommaSeparatedValues& __csv);
EXT_ cCommaSeparatedValues&  first(cCommaSeparatedValues& __csv);
EXT_ cCommaSeparatedValues&  next(cCommaSeparatedValues& __csv);

enum eErrorCSV {
    CSVE_OK                     = QTextStream::Ok,
    CSVE_STRM_ReadPastEnd       = QTextStream::ReadPastEnd,
    CSVE_STRM_ReadCorruptData   = QTextStream::ReadCorruptData,
    CSVE_STRM_WriteFailed       = QTextStream::WriteFailed,
    CSVE_EMPTY_LINE             = 0x0010,
    CSVE_PARSE_ERROR            = 0x0020,
    CSVE_IS_NOT_NUMBER          = 0x0030,
    CSVE_DROP_INVCH             = 0x0100,
    CSVE_DROP_CRLF              = 0x0200,
    CSVE_END_OF_LINE            = 0x1000,
    CSVE_EDD_OF_FILE            = 0x2000,
    CSVE_STRM                   = 0x000f,
    CSVE_FORM                   = 0x00f0,
    CSVE_DROP                   = 0x0f00,
    CSVE_END_OF                 = 0xf000,
    CSVE_CRIT                   = 0x00ff
};

class LV2SHARED_EXPORT cCommaSeparatedValues {
    friend LV2SHARED_EXPORT cCommaSeparatedValues&  endl(cCommaSeparatedValues& __csv);
    friend LV2SHARED_EXPORT cCommaSeparatedValues&  first(cCommaSeparatedValues& __csv);
    friend LV2SHARED_EXPORT cCommaSeparatedValues&  next(cCommaSeparatedValues& __csv);
public:
    cCommaSeparatedValues(const QString& _csv = QString());
    cCommaSeparatedValues(QIODevice *pIODev);
    ~cCommaSeparatedValues();
    void clear();
    const QString& toString() const;
    int lineNo() { return _lineNo; }
    int state(QString &msg);
    int state() { return _state; }
    static const QChar sep;
    static const QChar quo;
    cCommaSeparatedValues& operator <<(qlonglong i) { line += QString::number(i) + sep; return *this; }
    cCommaSeparatedValues& operator <<(double d)    { line += QString::number(d) + sep; return *this; }
    cCommaSeparatedValues& operator <<(const QString& _s);
    cCommaSeparatedValues& operator >>(QString &_v);
    cCommaSeparatedValues& operator >>(qlonglong &_v);
    QString msg;
    bool dropCrLf;
protected:
    void splitLine();
    int _state;
    int _lineNo;
    QString csv;
    QString line;
    QStringList values;
    QTextStream *pStream;
};

inline cCommaSeparatedValues& operator<< (cCommaSeparatedValues& __s, cCommaSeparatedValues&(*__pf)(cCommaSeparatedValues &))
{ return (*__pf)(__s); }

#endif // OTHERS_H

