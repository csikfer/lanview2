#ifndef LV2CONT_H
#define LV2CONT_H
#include <typeinfo>
#include <lv2types.h>

class SqlQuery;

/*!
@file lv2cont.h
Adatbázis kezelő objektum konténerek ill. template-k
*/

#include <QList>
#include <lv2datab.h>

template <class C, class M> void appendCont(C& c, const M& m, int i) {
    if (c.size() != i) EXCEPTION(EPROGFAIL, i);
    c << m;
}

template <class C1, class M1, class C2, class M2> void appendCont(C1& c1, const M1& m1, C2& c2, const M2& m2, int i) {
    if (c1.size() != i || c2.size() != i) EXCEPTION(EPROGFAIL, i);
    c1 << m1;
    c2 << m2;
}

template <class C1, class M1, class C2, class M2, class C3, class M3>
    void appendCont(C1& c1, const M1& m1, C2& c2, const M2& m2, C3& c3, const M3& m3, int i) {
    if (c1.size() != i || c2.size() != i || c3.size() != i) EXCEPTION(EPROGFAIL, i);
    c1 << m1;
    c2 << m2;
    c3 << m3;
}

template <class C1, class M1, class C2, class M2, class C3, class M3, class C4, class M4>
    void appendCont(C1& c1, const M1& m1, C2& c2, const M2& m2, C3& c3, const M3& m3, C4& c4, const M4& m4, int i) {
    if (c1.size() != i || c2.size() != i || c3.size() != i || c4.size() != i) EXCEPTION(EPROGFAIL, i);
    c1 << m1;
    c2 << m2;
    c3 << m3;
    c4 << m4;
}

/*!
@class tRecordList
Template osztály. Rekord lista konténer.

A konténer az objektumok pointereit tárolja, amik a konténer objektum „tulajdonába” kerülnek, vagyis azokat a destruktor
felszabadítja.
A konténer másolásakor az összes elem klónozva lesz, ezért ez a művelet, főleg nagyobb listák esetén kerülendő, és lásd még
a klónozásnál leírtakat is cRecord::dup().\n
Az ős QList konténer metódusain kívül mit nyújt a tRecordList konténer? \n
A pointer elemeket az elem törlésekor felszabadítja, továbbá ha referenciával adjuk meg a konténerbe elhelyezendő objektumot
(pointer helyett), akkor klónozza azt, és a klónt helyezi a konténerbe.
Egy elem kereshető, ill. elérhető egy mező értéke alapján: contains() és indexOff(), get() metódusok.
Lehetőség van egy mező értékének beállítására az összes elemnél: sets(), setId(), setName().
Az osztály biztosít néhány adatbázis műveletet végző metódust: fetch(), insert(), remove()

Azokat a metódusokat, melyek a mező indexe alapján azonosítanak egy mezőt, csak akkor használhatjuk, ha a konténerben tárolt
objektumok valóban azonos típusuak, ill. ugyan az a cRecStaticDescr objektum a leírójuk.
Vegyes cRecordAny objektum, ha a hozzárendelt adattáblák nincsenek öröklési kapcsolatban, nem támogatott.
*/
template <class T>
        class tRecordList : public QList<T *>
{
public:
    typedef QListIterator<T *> listIterator;
    /// Konstruktor. Üres rekord listát hoz létre
    tRecordList() : QList<T *>() { ; }
    /// Konstruktor. Egy elemű rekord listát hoz létre (T a rekord objektum típus)
    /// A paraméterként megadott pointer a konténeré, annak destruktora (vagy a clean()) szabadítja fel.
    tRecordList(T * __p) : QList<T *>()
    {
        QList<T *>::append(__p);
    }
    /// Konstruktor. Egy elemű listát hoz létre
    /// A paraméterként megadott objektumról másolatot készít.
    tRecordList(const T& __o) : QList<T *>()
    {
        T * p =  dynamic_cast<T *>(__o.dup());
        QList<T *>::append(p);
    }
    /// Konstruktor.
    /// Beolvassa az összes olyan rekordot, melynek a megadott sorszámú numerikus mezője egyezik a megadott id-vel
    /// @param __q query objektum, amivel a lekérdezés elvégezhető.
    /// @param __only
    /// @param __fi A numerikus mező indexe.
    /// @param __id A keresett ID-k.
    tRecordList(QSqlQuery& __q, bool __only, int __fi, qlonglong __id) : QList<T *>()
    {
        fetch(__q, __only, __fi, __id);
    }
    /// Copy konstruktor
    /// Nem támogatott
    tRecordList(const tRecordList& __o) : QList<T *>()
    {
        EXCEPTION(EPROGFAIL, 0, typeid(T).name() + QString(" :: ") + __o.toString());
    }

    QList<T *>& list()             { return *static_cast<      QList<T *> *>(this); }
    const QList<T *>& list() const { return *static_cast<const QList<T *> *>(this); }
    /// Kiüríti a konténert, az összes elemet (pointerek) felszabadítja
    tRecordList& clear()
    {
        typename QList<T *>::iterator    i;
        for (i = QList<T *>::begin(); i != QList<T *>::end(); i++) delete *i;
        QList<T *>::clear();
        return *this;
    }
    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    /// Az objektumot amire a pointer mutat, a konténer szabadítja fel-
    tRecordList<T>& append(T *p) {
        QList<T *>::append(p);
        return *this;
    }
    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    /// Az objektumot amire a pointer mutat, a konténer szabadítja fel-
    tRecordList<T>& operator <<(T *p) {
        QList<T *>::append(p);
        return *this;
    }
    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    /// Az objektumot amire a pointer mutat, a konténer szabadítja fel-
    tRecordList<T>& operator +=(T *p) {
        QList<T *>::append(p);
        return *this;
    }
    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít, és azt teszi be a konténerbe
    tRecordList<T>& append(const T& _o) {
        T * p =  dynamic_cast<T *>(_o.dup());
        QList<T *>::append(p);
        return *this;
    }
    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít, és azt teszi be a konténerbe
    /// @note A két << operátor lehet, hogy egy kicsit "beugratós", változtatni kéne ?
    tRecordList<T>& operator <<(const T& __o) {
        return append(__o);
    }
    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít, és azt teszi be a konténerbe
    /// @note A két << operátor lehet, hogy egy kicsit "beugratós", változtatni kéne ?
    tRecordList<T>& operator +=(const T& __o) {
        return append(__o);
    }
    /// A konténerhez hozzáadja a paraméterként megadott konténer elemeit.
    /// A paraméterként megadott konténer összes objektumát újra allokálja (a dup(() metódus hívásával),
    /// és ezek kerülnek az objektum eredeti tartalma után azonos sorrendben.
    tRecordList& append(const tRecordList& __o)
    {
        //DBGFN();
        typename QList<T *>::const_iterator    i;
        for (i = __o.constBegin(); i != __o.constEnd(); i++) {
            T * p =  dynamic_cast<T *>((*i)->dup());
            QList<T *>::append(p);
        }
        //DBGFNL();
        return *this;
    }
    /// A konténerhez hozzáadja a paraméterként megadott konténer elemeit.
    /// A paraméterként megadott konténer összes objektumát újra allokálja (a dup(() metódus hívásával),
    /// és ezek kerülnek az objektum eredeti tartalma után azonos sorrendben.
    tRecordList& operator<<(const tRecordList& __o)
    {
        //DBGFN();
        return append(__o);
    }
    /// A konténerhez hozzáadja a paraméterként megadott konténer elemeit.
    /// A paraméterként megadott konténer összes objektumát újra allokálja (a dup(() metódus hívásával),
    /// és ezek kerülnek az objektum eredeti tartalma után azonos sorrendben.
    tRecordList& operator+=(const tRecordList& __o)
    {
        //DBGFN();
        return append(__o);
    }
    /// Copy operátor.
    /// Nem támogatott. Helyettesíthető a clear().append(__o) hívással.
    /// @note A tOwnRecords osztálynál a clear().append(__o); forma használata hibás,
    /// mert a tRecordList::append() lesz meghívva a tOwnRecords::append() helyett!
    /// Volt belőle komoly gond, inkább nem kell.
    tRecordList& operator=(const tRecordList& __o)
    {
        EXCEPTION(EPROGFAIL, 0, typeid(T).name() + QString(" :: ") + __o.toString());
        return *this;
    }
    /// Konténer destruktor. A konténer elemeit, a pointereket felszabadítja.
    ~tRecordList()
    {
        typename QList<T *>::iterator    i;
        for (i = QList<T *>::begin(); i != QList<T *>::end(); i++) delete *i;
    }

    /// A konténer insert metódusának az újra definiálása. A pointer ezután a konténer hatáskörébe tartozik, az szabadítja fel.
    void pushAt(int i, T *p)        { list().insert(i, p); }
    /// Hasonló a konténer insert metódusához, de az _o objektumot ujra allokálja a dup() metódus hívásával.
    void pushAt(int i, const T& _o) { pushAt(i, dynamic_cast<T *>(_o.dup())); }

    /// Beolvassa az összes olyan rekordot, mely megfelel a pointerként átadott
    /// objektum megadott mezőinek. Csak egyenlőségre szűr, kivéve, ha beállítjuk a *p objektumban a likeMask bitjeit.
    /// A metódus akkor is használható, ha a template osztály önmagában nem azonosítja a kezelt adattáblát,
    /// de csak olyan *p objektummal hívhatjuk, ami egyértelműen azonosítja azt. cRecordAny esetén
    /// ha a *p faceless, akkor hibát fog generálni a hívás.
    /// @param __q query objektum, amivel a lekérdezés elvégezhető.
    /// @param __only Ha a leszármazottakban nem kell keresni, akkor értéke true.
    /// @param fm Bit map a szűrésben réstvevő mezőkre.
    /// @param p Objektum pointer, mely a szűrési értékeket tartalmazza, ez az objektum lessz a
    ///        konténer első eleme, ill. ha a konténer üres, akkor a pointer fel lessz szabadítva.
    /// @return A konténer elem számával tér vissza
    int fetch(QSqlQuery& __q, bool __only, const QBitArray& fm, T *p)
    {
        if (p->fetch(__q, __only, fm)) {
            append(p);
            while (__q.next()) {
                cRecord *__p = p->newObj();
                p = dynamic_cast<T *>(__p);
                // p = dynamic_cast<T *>(p->newObj());
                p->set(__q.record());
                append(p);
            }
        }
        else {
            delete p;
        }
        return QList<T *>::count();
    }
    ///
    int fetchByNamePattern(QSqlQuery& __q, const QString& __pat, bool __only = false, T *p = nullptr)
    {
        if (!p) p = new T();
        else    p->clear();
        p->setName(__pat);
        p->_likeMask = p->mask(p->nameIndex());
        PDEB(VVERBOSE) << __PRETTY_FUNCTION__ << "<" << p->tableName() << "> : " << __pat << " , " << list2string(p->_likeMask) << endl;
        return fetch(__q, __only, p->_likeMask, p);

    }
    /// Beolvassa az összes olyan rekordot, melynek a megadott indexű numerikus mezője egyezik a megadott id-vel
    /// Az objektumot (konténert) elötte kiüríti. A T template osztálynak egyértelműen azonosítania kell a
    /// kezelt adattáblát, pl. a cRecordAny osztály esetén hibát fog generálni a metódus.
    /// @param __q query objektum, amivel a lekérdezés elvégezhető.
    /// @param __only
    /// @param __fn A numerikus mező indexe.
    /// @param __id A keresett ID-k.
    int fetch(QSqlQuery& __q, bool __only, int __fi, qlonglong __id)
    {
        T   *p = new T();
        QBitArray fm = _bit(__fi);
        p->set(__fi, QVariant(__id));
        return fetch(__q, __only, fm, p);
    }
    /// Beolvassa az összes olyan rekordot, melynek a megadott nevű numerikus mezője egyezik a megadott id-vel
    /// Az objektumot (konténert) elötte kiüríti. A T template osztálynak egyértelműen azonosítania kell a
    /// kezelt adattáblát, pl. a cRecordAny osztály esetén hibát fog generálni a metódus.
    /// @param __q query objektum, amivel a lekérdezés elvégezhető.
    /// @param __only
    /// @param __fn A numerikus mező neve.
    /// @param __id A keresett ID-k.
    int fetch(QSqlQuery& __q, bool __only, const QString& __fn, qlonglong __id)
    {
        T   *p = new T();
        QBitArray fm =  _bit(p->toIndex(__fn));
        p->set(__fn, QVariant(__id));
        return fetch(__q, __only, fm, p);
    }
    /// Kiírja az adatbázisba az összes elemet az INSERT utasítással ld.: cRecord::insert()
    /// Ha egy rekord kiírása sikertelen, akkor megszakítja a kiírásokat.
    /// @return A sikeresen kiírt rekordszámmal tér vissza, vagy hiba esetén -1 -el
    int insert(QSqlQuery& __q, eEx __ex = EX_ERROR)  {
        DBGFN();
        int r = 0;
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            PDEB(VERBOSE) << "Insert : " << (*i)->toString() << endl;
            if (!(*i)->insert(__q, __ex)) return -1;
            ++r;
        }
        _DBGFNL() << " = " << r << endl;
        return r;
    }
    /// Törli az adatbázisból az összes elemet az REMOVE utasítással ld.: cRecord::remove()
    /// @return A sikeresen törölt rekordszámmal tér vissza.
    int remove(QSqlQuery& __q, eEx __ex = EX_NOOP)  {
        int r = 0;
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            PDEB(VERBOSE) << "Remove from db : " << (*i)->toString() << endl;
            if (!(*i)->remove(__q, false, QBitArray(), __ex)) return -1;
            ++r;
        }
        return r;
    }
    /// Keresés ID alapján.
    /// @param __id A keresett id.
    /// @return A listában megtaéállt első lista elem sorszáma, vagy -1, ha nem talállt semmit.
    int indexOf(qlonglong __id) const {
        typename QList<T *>::const_iterator    i = QList<T *>::constBegin();
        for (; i < QList<T *>::constEnd(); i++) {
            if ((*i)->isNullId() == false && (*i)->getId() == __id) return i - QList<T *>::constBegin();
        }
        return -1;
    }
    /// Keresés név alapján.
    /// @param __name A keresett név.
    /// @return A listában megtaéállt első lista elem sorszáma, vagy -1, ha nem talállt semmit.
    int indexOf (const QString& __name) const {
        typename QList<T *>::const_iterator    i = QList<T *>::constBegin();
        for (; i < QList<T *>::constEnd(); i++) {
            if ((*i)->getName() == __name) return i - QList<T *>::constBegin();
        }
        return -1;
    }
    /// Keresés név alapján.
    /// @param __name   A keresett név (csak az owner id-vel együtt egyedi)
    /// @param _oid Az tulajdonos id
    /// @param _ixoid Opcionális mező index: a kulcsmező indexe
    /// @return A listában megtaéállt első lista elem sorszáma, vagy -1, ha nem talállt semmit.
    int indexOf (const QString& __name, qlonglong _oid, int _ixoid = NULL_IX) const {
        if (QList<T *>::isEmpty()) return -1;
        if (_ixoid < 0) _ixoid = QList<T *>::first()->descr().ixToOwner();
        typename QList<T *>::const_iterator    i = QList<T *>::constBegin();
        for (; i < QList<T *>::constEnd(); i++) {
            if ((*i)->getId(_ixoid) == _oid && (*i)->getName() == __name) return i - QList<T *>::constBegin();
        }
        return -1;
    }
    /// Keresés megadott mező érték alapján.
    /// @param __ix A mező indexe
    /// @param _val A keresett érték.
    /// @param __st Kezdő index a kereséshez, opcionális, ha nem adjuk meg az első (0.) elemmel kezdi a keresést.
    /// @return A listában megtalált első lista elem sorszáma, vagy -1, ha nem talállt semmit.
    int indexOf (int __ix, const QVariant& _val, int __st = 0) const {
        typename QList<T *>::const_iterator    i = QList<T *>::constBegin();
        if (__st != 0) {
            if (__st < 0) return -1;
            if (__st >= list().size()) return -1;
            i += __st;
        }
        for (; i < QList<T *>::constEnd(); i++, __st++) {
            if ((*i)->isNull(__ix) == false && (*i)->get(__ix) == _val) return __st;
        }
        return -1;
    }
    /// Keresés megadott mező érték alapján.
    /// @param __name A mező neve
    /// @param _val A keresett érték.
    /// @param __st Kezdő index a kereséshez, opcionális, ha nem adjuk meg az első (0.) elemmel kezdi a keresést.
    /// @return A listában megtalállt első lista elem sorszáma, vagy -1, ha nem talállt semmit.
    int indexOf (const QString&  __name, const QVariant& _val, int __st = 0) const {
        typename QList<T *>::const_iterator    i = QList<T *>::constBegin();
        if (__st != 0) {
            if (__st < 0) return -1;
            if (__st >= list().size()) return -1;
            i += __st;
        }
        for (; i < QList<T *>::constEnd(); i++, __st++) {
            if ((*i)->isNull(__name) == false && (*i)->get(__name) == _val) return __st;
        }
        return -1;
    }
    /// Keresés ID alapján.
    /// @param __id A keresett id.
    /// @return Ha van találat, akkor teue, egyébként false
    bool contains(qlonglong __id) const              { return indexOf(__id) >= 0; }
    /// Keresés név alapján.
    /// @param __name A keresett név.
    /// @return Ha van találat, akkor teue, egyébként false
    bool contains(const QString& __name) const      { return indexOf(__name) >= 0; }
    /// Keresés megadott mező érték alapján.
    /// @param __ix A mező indexe
    /// @param _val A keresett érték.
    /// @return Ha van találat, akkor teue, egyébként false
    bool contains(int __ix, const QVariant& _val) const { return indexOf(__ix, _val) >= 0; }
    /// Keresés megadott mező érték alapján.
    /// @param __name A mező neve
    /// @param _val A keresett érték.
    /// @return Ha van találat, akkor teue, egyébként false
    bool contains(const QString&  __name, const QVariant& _val) const { return indexOf(__name, _val) >= 0; }
    /// A lista tartalmának a stringé konvertálása
    QString toString() const {
        if (QList<T *>::isEmpty()) return QString("[ ]");
        QString s = QChar('[');
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            T *p = *i;
            s += QChar(' ') + p->toString() + ", ";
        }
        s.chop(2);
        return s + "]";
    }
    /// Az összes elem ID mezőjének a törlése
    void clearId() {
        typename QList<T *>::const_iterator    i;
        int ix = QList<T *>::first()->idIndex();
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->clear(ix);
        }
    }
    /// Egy mező értékének a beállítása az összes elemnél a megadott értékre
    /// @param __i A beállítandó mező indexe
    /// @param __v A mező új értéke
    void sets(int __i, const QVariant& __v) {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->set(__i, __v);
        }
    }
    /// Egy mező értékének a beállítása az összes elemnél a megadott értékre
    /// @param __n A beállítandó mező neve
    /// @param __v A mező új értéke
    void sets(const QString& __n, const QVariant& __v) {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->set(__n, __v);
        }
    }
    /// Egy ID típusú mező értékének a beállítása az összes elemnél a megadott értékre
    /// @param __i A beállítandó mező indexe
    /// @param __id A mező új értéke
    void setsId(int __i, qlonglong __id) {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->setId(__i, __id);
        }
    }
    /// Egy ID típusú mező értékének a beállítása az összes elemnél a megadott értékre
    /// @param __n A beállítandó mező neve
    /// @param __id A mező új értéke
    void setsId(const QString& __n, qlonglong __id) {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->setId(__n, __id);
        }
    }
    /// Egy text típusú mező értékének a beállítása az összes elemnél a megadott értékre
    /// @param __i A beállítandó mező indexe
    /// @param __v A mező új értéke
    void setsName(int __i, const QString& __v) {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->setName(__i, __v);
        }
    }
    /// Egy text típusú mező értékének a beállítása az összes elemnél a megadott értékre
    /// @param __n A beállítandó mező neve
    /// @param __v A mező új értéke
    void setsName(const QString& __n, const QString& __v) {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->setName(__n, __v);
        }
    }
    /// A konténer egy elemének a lekérése az ID alapján
    /// Ha nincs ID mező, vagy nem ismert az indexe, akkor dob egy kizárást
    /// @param __id a keresett ID értéke
    /// @param __ex Ha értéke true, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatár.
    T *get(qlonglong __id, eEx __ex = EX_ERROR) const {
        int i = indexOf(__id);
        if (i < 0) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __id, QObject::tr("Keresés a rekord ID alapján: %1 = %2").arg(T().idName()).arg(__id));
        }
        return QList<T *>::at(i);
    }
    /// A konténer egy elemének a lekérése az név alapján
    /// Ha nincs név mező, vagy nem ismert az indexe, akkor dob egy kizárást
    /// @param __nm a keresett név értéke
    /// @param __ex Ha értéke true, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatér.
    T *get(const QString& __nm, eEx __ex = EX_ERROR) const {
        int i = indexOf(__nm);
        if (i < 0) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, 0, QObject::tr("Keresés a rekord név alapján: %1 = %2").arg(T().nameName()).arg(__nm));
        }
        return QList<T *>::at(i);
    }
    /// A konténer egy elemének a lekérése amegadott nevű mező értéke alapján
    /// Ha nincs ilyen nevű mező, akkor dob egy kizárást
    /// @param __nm A mező neve, ami alapján keresünk
    /// @param __v A keresett érték
    /// @param __ex Ha értéke true, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatér.
    T *get(const QString& __fn, const QVariant& __v, eEx __ex = EX_ERROR) const {
        int i = indexOf(__fn, __v);
        if (i < 0) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, 0, QObject::tr("Keresés a %1 nevű mezőérték alapján: %2").arg(__fn).arg(debVariantToString(__v)));
        }
        return QList<T *>::at(i);
    }
    /// A konténer egy elemének a lekérése amegadott sorszámú mező értéke alapján
    /// Ha nincs ilyen mező, akkor dob egy kizárást
    /// @param __ix A mező sorszáma, ami alapján keresünk
    /// @param __v A keresett érték
    /// @param __ex Ha értéke nem EX_IGNORE, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatér.
    T *get(int __ix, const QVariant& __v, eEx __ex = EX_ERROR) const {
        int i = indexOf(__ix, __v);
        if (i < 0) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __ix, QObject::tr("Keresés a %1 indexű mezőérték alapján: %2").arg(__ix).arg(debVariantToString(__v)));
        }
        return QList<T *>::at(i);
    }
    /// A konténer utolsó pointerét törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer üres, akkor dob egy kizárást, ha __ex nem EX_IGNORE, vagy NULL pointerrel tér vissza, ha __ex hamis.
    T *pop_back(eEx __ex = EX_ERROR)  {
        if (list().size() < 1) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, 0);
        }
        T *p = QList<T *>::back();
        QList<T *>::pop_back();
        return p;
    }
    /// A konténer első pointerét törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer üres, akkor dob egy kizárást, ha __ex nem EX_IGNORE, vagy NULL pointerrel tér vissza, ha __ex hamis.
    T *pop_front(eEx __ex = EX_ERROR) {
        if (list().size() < 1) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, 0);
        }
        T *p = QList<T *>::front();
        QList<T *>::pop_front();
        return p;
    }
    /// A megadott indexű pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott indexű elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __ix A kiválasztott elem indexe a konténerben
    /// @param __ex Opcionális flag, ha nem EX_IGNORE (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pullAt(int __ix, eEx __ex = EX_ERROR) {
        if (list().size() <= __ix) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __ix);
        }
        T *p = QList<T *>::at(__ix);
        QList<T *>::removeAt(__ix);
        return p;
    }
    /// A megadott ID-jű objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __id A keresett elem rekord azobosító ID-je
    /// @param __ex Opcionális flag, ha nem EX_IGNORE (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(qlonglong __id, eEx __ex = EX_ERROR) {
        int ix = indexOf(__id);
        if (0 > ix) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
    /// A megadott nevű objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __id A keresett elem rekord nevee
    /// @param __ex Opcionális flag, ha nem EX_IGNORE (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(const QString __name, eEx __ex = EX_ERROR) {
        int ix = indexOf(__name);
        if (0 > ix) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
    /// A megadott nevű mező értékévlel egyezést mutató első objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex nem EX_IGNORE, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __fname Mező név
    /// @param __val A keresett rekordban a megadott nevű mező értéke.
    /// @param __ex Opcionális flag, ha igaz (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(const QString __fname, const QVariant& __val, eEx __ex = EX_ERROR) {
        int ix = indexOf(__fname, __val);
        if (0 > ix) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
    /// A megadott nevű mező értékévlel egyezést mutató első objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __fix Mező index
    /// @param __val A keresett rekordban a megadott sorszámú mező értéke.
    /// @param __ex Opcionális flag, ha nem EX_IGNORE (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(int __fix, const QVariant& __val, eEx __ex = EX_ERROR) {
        int ix = indexOf(__fix, __val);
        if (0 > ix) {
            if (__ex == EX_IGNORE) return nullptr;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
    /// Eltávolítja a duplikált elemeket. Az összehasonlítást a rekord ID-k alapján végzi! A rekord ID-nek léteznie kell, és az értékük nem lehet NULL.
    int removeDuplicates() {
        int r = 0;
        for (int i = 0; i < QList<T *>::size(); ++i) {
            const T& o = *QList<T *>::at(i);
            for (int j = i +1; j < QList<T *>::size(); ++j) {
                if (o == *QList<T *>::at(j)) {
                    delete pullAt(j);
                    --j;
                    ++r;
                }
            }
        }
        return r;
    }
    /// Egy lekérdezés alapján feltölti a konténert. Nem üríti a konténert, a beolvasott objektumok hozzáadódnak a meglévőkhöz.
    /// @return A konténerbe került objektumok száma.
    int set(QSqlQuery& q) {
        if (q.first() == false) return 0;
        int i = 0;
        do {
            ++i;
            T *p = new T;
            p->set(q);
            *this << p;
        } while (q.next());
        return i;
    }
};

template<class T> QTextStream& operator<<(QTextStream& __t, const tRecordList<T>& __v)
{
    return __t << toString(__v);
}

#define RECACHEHED(R, n) \
    protected: \
        static tRecordList<R> n##s; \
    public: \
        static void resetCacheData() { n##s.clear(); } \
        static const R * n(QSqlQuery& __q, const QString& ident, eEx __ex = EX_ERROR); \
        static const R * n(QSqlQuery &__q, qlonglong      ident, eEx __ex = EX_ERROR);

#define RECACHEFN(R, n, T, ff) \
        const R * R::n(QSqlQuery& __q, T ident, eEx __ex) { \
            int i = n##s.indexOf(ident); \
            if (i >= 0) return n##s.at(i); \
            R *p = new R(); \
            if (!p->ff(__q, ident)) { \
                if (__ex != EX_IGNORE) { \
                    EXCEPTION(EFOUND, -1, QString(QObject::tr("%1 rekord azonosító : %2::%3")).arg(p->tableName()).arg(ident).arg(_STR(T))); \
                } \
                pDelete(p); \
            } \
            else { \
                n##s << p; \
            } \
            return p; \
        }

#define RECACHEDEF(R, n) \
        tRecordList<R> R::n##s; \
        RECACHEFN(R, n, const QString&, fetchByName) \
        RECACHEFN(R, n, qlonglong,      fetchById)


/// cRecoed típusú objektum részobjektumait tároló konténer.
/// A konténer C típusú objektumokat táro, és a tulajdonos objektum tíousa O.
/// A QList ösnek nem minden metódusa lett újra implementálva!
template <class C, class O>
        class tOwnRecords : public tRecordList<C>
{
public:
    /// Az owner rekord id (távoli kulcs) mező indexe.
    const int ixOwnerId;
protected:
    O  * pOwner;
public:
    QList<C *>& list()             { return *static_cast<      QList<C *> *>(this); }
    const QList<C *>& list() const { return *static_cast<const QList<C *> *>(this); }
    /// Konstruktor, üres konténert hoz létre
    /// @param __po Tulajdonos objektum pointere.
    /// @remark Az owner id indexét egy C().descr().ixToOwner(tableName) hívással állapitja meg.
    /// A tábla névet a megadott __po pointerű objektumból kérdezi le, ezért a konstruktort
    /// a __po pointerű objektum konstruktorából nem hívhatjuk, mert ekkor még a descr() virtuális metódus nem elérhető,
    /// vagyis az alkalmazás össze fog omlani.
    tOwnRecords(O *__po) : tRecordList<C>(), ixOwnerId(C().descr().ixToOwner(__po->descr().tableName())), pOwner(__po) {
        if (pOwner == nullptr) EXCEPTION(EPROGFAIL);
    }
    /// Konstruktor, üres konténert hoz létre
    /// @param __po Tulajdonos objektum pointere.
    /// @param sTableName A tulajdonos objektum tábla neve, ha üres stringet adunk meg, akkor az ixOwnerId értékét egy
    ///     paraméter nélküli C::ixToOwner() hívással fogja megállapítani.
    /// @remark Ez a konstruktor hívható a pOwner pointerű objektum konstruktorából is.
    tOwnRecords(O *__po, const QString sTableName) : tRecordList<C>(), ixOwnerId(C().descr().ixToOwner(sTableName)), pOwner(__po) {
        if (pOwner == nullptr) EXCEPTION(EPROGFAIL);
    }
    /// Konstruktor (copy konstruktor helyett)
    tOwnRecords(O *__po, const tOwnRecords& __c) : tRecordList<C>(), ixOwnerId(C().descr().ixToOwner(O::_pRecordDescr->tableName())), pOwner(__po) {
        if (pOwner == nullptr) EXCEPTION(EPROGFAIL);
        append(__c);
    }
    /// Copy konstruktor. Nem támogatott
    tOwnRecords(const tOwnRecords& __o) : tRecordList<C>(), ixOwnerId(NULL_IX) {
        EXCEPTION(EPROGFAIL, 0, typeid(C).name() + QString(" , ") + typeid(O).name() + " :: \n" + __o.toString());
    }
    /// Copy operator. Nem támogatott
    tOwnRecords<C, O>& operator =(const tOwnRecords& __o) {
        EXCEPTION(EPROGFAIL, 0, typeid(C).name() + QString(" , ") + typeid(O).name() + " :: \n" + __o.toString());
        return *this;   // warning
    }

    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    tOwnRecords<C, O>& append(C *p) {
        p->setParent(pOwner);
        QList<C *>::append(p);
        return *this;
    }
    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    tOwnRecords<C, O>& operator <<(C *p) { return append(p); }
    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    tOwnRecords<C, O>& operator +=(C *p) { return append(p); }

    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít.
    tOwnRecords<C, O>& append(const C& c) {
        C *p = dynamic_cast<C *>(c.dup());
        return append(p);
    }
    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít, és azt teszi be a konténerbe
    tOwnRecords<C, O>& operator <<(const C& _o) { return append(_o); }
    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít, és azt teszi be a konténerbe
    /// @note A két << operátor lehet, hogy egy kicsit "beugratós", változtatni kéne ?
    tOwnRecords<C, O>& operator +=(const C& _o) { return append(_o); }

    /// Bővíti a listát a konténer elemeivel. Minden elemről másolat készül.
    tOwnRecords<C, O>& append(const tOwnRecords<C, O>& __o) {
        if (ixOwnerId != __o.ixOwnerId) EXCEPTION(EDATA);
        typename QList<C *>::const_iterator    i;
        for (i = __o.QList<C *>::constBegin(); i < __o.QList<C *>::constEnd(); i++) {
            (*this) << **i;
        }
        return *this;
    }
    /// Bővíti a listát a konténer elemeivel. Minden elemről másolat készül.
    tOwnRecords<C, O>& operator<<(const tOwnRecords<C, O>& __o) { return append(__o); }
    /// Bővíti a listát a konténer elemeivel. Minden elemről másolat készül.
    tOwnRecords<C, O>& operator+=(const tOwnRecords<C, O>& __o) { return append(__o); }

    ///
    tOwnRecords&  setsOwnerId() {
        tRecordList<C>::setsId(ixOwnerId, pOwner->getId());
        return *this;
    }
    tOwnRecords&  setsOwnerId(qlonglong _oid) {
        tRecordList<C>::setsId(ixOwnerId, _oid);
        pOwner->setId(_oid);
        return *this;
    }
    int fetch(QSqlQuery &__q, bool __only = false) {
        int r = tRecordList<C>::fetch(__q, __only, ixOwnerId, pOwner->getId());
        if (r > 0) setOwner();
        return r;
    }
    int insert(QSqlQuery &__q, eEx __ex = EX_ERROR) {
        if (tRecordList<C>::size() == 0) {
            if (__ex == EX_NOOP) EXCEPTION(EDATA);
            return 0;
        }
        setsOwnerId();
        return tRecordList<C>::insert(__q, __ex);
    }
    int removeByOwn(QSqlQuery &__q, eEx __ex = EX_ERROR) const {
        C o;
        o.setId(ixOwnerId, pOwner->getId());
        return o.remove(__q, false, o.mask(ixOwnerId), __ex);
    }
    bool replace(QSqlQuery &__q, eEx __ex = EX_ERROR) {
        // Ha nincs új rekord, csak a régieket töröljük
        if (tRecordList<C>::size() == 0) {
            removeByOwn(__q, __ex);
            return true;
        }
        // Megjelöljük a régi rekordokat, ha egy sincs akkor csak beillesztjük az újakat
        if (mark(__q) == 0) {
            insert(__q, EX_NOOP);
            return true;
        }
        int r = 0;
        typename QList<C *>::const_iterator    i;
        for (i = QList<C *>::constBegin(); i < QList<C *>::constEnd(); i++) {
            PDEB(VERBOSE) << "Replace : " << (*i)->toString() << endl;
            (*i)->setFlag(false);
            if ((*i)->replace(__q, __ex) != R_ERROR) ++r;
        }
        removeMarked(__q, TS_FALSE);    // If flag is true then remove from database, list unchanged
        return r == QList<C *>::size();
    }
    /// Az ID-k alapján írja újra a rekordokat.
    /// Ha az objektumnak az ID-je NULL, akkor rewriteById() metódus helyett az insert() metódust hívja
    bool replaceById(QSqlQuery &__q, eEx __ex = EX_ERROR) {
        // Ha nincs új rekord, csak a régieket töröljük
        if (tRecordList<C>::size() == 0) {
            removeByOwn(__q, __ex);
            return true;
        }
        // Megjelöljük a régi rekordokat, ha egy sincs akkor csak beillesztjük az újakat
        if (mark(__q) == 0) {
            insert(__q, EX_NOOP);
            return true;
        }
        int r = 0;
        typename QList<C *>::const_iterator    i;
        for (i = QList<C *>::constBegin(); i < QList<C *>::constEnd(); i++) {
            PDEB(VERBOSE) << "ReplaceById : " << (*i)->toString() << endl;
            (*i)->setFlag(false);
            if ((*i)->isNullId()) {
                if ((*i)->insert(__q, __ex)) ++r;
            }
            else {
                if ((*i)->rewriteById(__q, __ex)) ++r;
            }
        }
        removeMarked(__q, TS_FALSE);    // If flag is true then remove from database, list unchanged
        return r == QList<C *>::size();
    }

    int mark(QSqlQuery &__q, bool flag = true) const {
        if (pOwner == nullptr) EXCEPTION(EPROGFAIL);
        qlonglong oid = pOwner->getId();
        C o;
        return o.setId(ixOwnerId, oid).mark(__q, o.mask(ixOwnerId), flag);
    }
    int removeMarked(QSqlQuery& __q, eTristate f = TS_FALSE) {
        if (pOwner == nullptr) EXCEPTION(EPROGFAIL);
        qlonglong oid = pOwner->getId();
        C o;
        // Delete from database
        int r = o.setId(ixOwnerId, oid).removeMarked(__q, o.mask(ixOwnerId));
        // Delete from list
        if (f != TS_FALSE && !list().isEmpty() && r > 0) {
            int i, n = list().size(), rn = 0;
            int fix = o.flagIndex();
            for (i = 0; i < n; ++i) {
                C *po = list().at(i);
                if (po->getBool(fix)) {
                    list().removeAt(i);
                    delete po;
                    --i; --n;
                    ++rn;
                }
            }
            if (f == TS_TRUE && r != rn) EXCEPTION(EDATA, n);   // Check
        }
        return r;
    }
    O& owner() { return *pOwner; }
protected:
    tOwnRecords<C,O>& setOwner() {
        typename QList<C *>::const_iterator    i;
        for (i = QList<C *>::constBegin(); i < QList<C *>::constEnd(); i++) {
            (*i)->setParent(pOwner);
        }
        return *this;
    }
};

/* ******************************  ****************************** */

/*!
@class tGroup
@brief Csoport kapcsoló tábla kezelése
Az objektum egy kapcsoló tábla rekordot reprezentál, ill. adattagként a hivatkozot group. és member rekordot
Nem egy cRecord típusú objektum a kapcsoló táblára.
*/
template <class G, class M>
    class tGroup
{
public:
    /// A group tábla egy eleme (G egy cRecord-ból származtatott osztály)
    G   group;
    /// A member tábla egy eleme (M egy cRecord-ból származtatott osztály)
    M   member;
    /// Egy üres objektum konstruktora, mind a group, mind a member ojektum üres lessz.
    tGroup() : group(), member() { ; }
    /// Konstruktor. A megadott ID-k alapján beolvassa a group, és member objektumokat.
    /// Ha a megadott ID-k nem egy létező objektumot jelentenek, akkor dob egy kizárást.
    /// A kapcsoló táblát nem ovassa, és azt nem is módosítja.
    /// @param __q Az objektumok beolvasásához használt QSqlQuery objektum
    /// @param __g A group objektum ID-je
    /// @param __m A member objektum ID-je
    tGroup(QSqlQuery& __q, qlonglong __g, qlonglong __m) : group(), member() {
        if (!group.fetchById(__q, __g))  EXCEPTION(EFOUND, __g, "Id of " + G().tableName());
        if (!member.fetchById(__q, __m)) EXCEPTION(EFOUND, __m, "Id of " + M().tableName());;
    }
    /// Konstruktor. A megadott nevek alapján beolvassa a group, és member objektumokat.
    /// Ha a megadott nevek nem egy létező objektumot jelentenek, akkor dob egy kizárást.
    /// A kapcsoló táblát nem ovassa, és azt nem is módosítja.
    /// @param __q Az objektumok beolvasásához használt QSqlQuery objektum
    /// @param __g A group objektum ID-je
    /// @param __m A member objektum ID-je
    tGroup(QSqlQuery& __q, const QString& __g, const QString& __m) : group(), member() {
        if (!group.fetchByName(__q, __g))  EXCEPTION(EFOUND, -1,QString("Name %1 of %2").arg(__g).arg(G().tableName()));
        if (!member.fetchByName(__q, __m)) EXCEPTION(EFOUND, -1,QString("Name %1 of %2").arg(__m).arg(M().tableName()));
    }
    /// Konstruktor. A megadott objektumokkal inicializálja a group, és member objektumokat.
    /// A kapcsoló táblát nem ovassa, és azt nem is módosítja.
    /// @param __g A group objektum referenciája
    /// @param __m A member objektum  referenciája
    tGroup(const G& __g, const M& __m) : group(__g), member(__m) { ; }
/*
    /// Értékadás a group adattagra.
    tGroup& operator =(const G& __g) { group  = __g; return *this; }
    /// Értékadás a member adattagra.
    tGroup& operator =(const M& __m) { member = __m; return *this; }
*/
    /// Értékadás a group és a member típusú adattagra.
    tGroup& operator =(const tGroup __gm) { group = __gm.group; member = __gm.member; return *this; }
    /// A kapcsoló tábla nevének előállításánál használt segéd függvény.
    /// Ha a megadott string 's' betüre végződik, akkor azt levágja, és az eredménnyel tér vissza.
    static QString drop_s(const QString& __n) {
        QString r = __n;
        if (r.endsWith(QChar('s'))) r.chop(1);
        return r;
    }
    /// A kapcsoló tábla nevével tér vissza. A kapcsoló tábla nevét a group és meber tábla nevéből képezi,
    /// az adatbázisban alkalmazott névkonvenció szerint.
    /// A group tábla nevének végéről levágja az 's' betűt, ha van, és hozzáfűz egy '_' karaktert, és a
    /// member tábla nevét.
    QString tableName() { return drop_s(group.tableName()) + "_" + member.tableName(); }
    /// A kapcsoló tábla ID mező nevével tér vissza. A mező nevét a group és meber tábla nevéből képezi,
    /// az adatbázisban alkalmazott névkonvenció szerint.
    /// A mindkét tábla nevének végéről levágja az 's' betűt, ha van, és összefűzi őket, előbb
    /// A group tábla nevéből képzett string, majd egy '_' karakter, ezután a member tábla nevéből képzett név
    /// végül az "_id' string.
    QString idName()    { return drop_s(group.tableName()) + "_" + drop_s(member.tableName()) + "_id"; }
    /// A group tábla ID mezőjének a nevével tér vissza.
    const QString& groupIdName()    { return group.idName() ; }
    /// A member tábla ID mezőjének a nevével tér vissza.
    const QString& memberIdName()   { return member.idName() ; }
    /// A metdus feltételezi, hogy a group és member objektumoknak ki van töltve az ID mezőjük, és ez alapján keresi a kapcsoló rekordot.
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @return Ha létezik a kapcsoló rekord (a member tagja a group-nak) akkor a kapcsoló rekord ID-vel tér vissza, ha nem akkor NULL_ID-vel.
    qlonglong test(QSqlQuery& __q) {
        QString sql = QString("SELECT %1 FROM %2 WHERE %3 = %4 AND %5 = %6")
                .arg(idName()).arg(tableName())
                .arg(groupIdName()).arg(group.getId())
                .arg(memberIdName()).arg(member.getId());
        if (!__q.exec(sql)) SQLPREPERR(__q, sql);
        if (__q.first()) return variantToId(__q.value(0));
        return NULL_ID;
    }
    /// Feltételezi, hogy a group objektumnak az ID mezője ki van töltve.
    /// Lekérdezi a megadott group meber rekordjait.
    /// Ha vannak memberek, akkor az elsőt beolvassa a member objektumba.
    /// Ha viszont nincsenek memberek, akkor törli a member adattagot.
    /// További member rekordokat a nextMember() metődussal olvashatunk be.
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @return True, ha volt legalább egy meber, false, ha nem.
    bool fetchFirstMember(QSqlQuery& __q) {
        QString sql = QString("SELECT * FROM %1 JOIN %2 USING(%3) WHERE %4 = %5")
                .arg(tableName())       // Kapcsoló tábla neve
                .arg(member.tableName())// Tagok tábla neve
                .arg(memberIdName())    // Tagok tábla ID mező neve
                .arg(groupIdName())     // Csoport tábla ID mező neve
                .arg(group.getId());    // Csoport tábla, kiválasztott csoport ID
        if (!__q.exec(sql)) SQLPREPERR(__q, sql);
        bool r = __q.first();
        if (r) member.set(__q);
        else   member.set();
        return r;
    }
    /// A fetchMembers(QSqlQuery& __q) metódus hívása után a további member rekordok beolvasása
    /// Egy példa a metódus használatára:
/*! @code
    // A "mindenki" nevű felhasználói csoport összes tagja nevének a kiírása a debug -ra:
    tGroup<cGroup, cUser>   ugSw;
    QSqlQuery   q = getQuery();
    if (!ugSw.group.fetchByName(q, "mindenki")) DERR("Nincs ,imdenki nevű csoport.");
    else {
        if (ugSw.fetchFirstMember(q)) {
            do {
                PDEB(VERBOSE) << ugSw.user.getName() << endl;
            } while (ugSw.fetchNextMember(q));
        }
        else {
            DERR("Nincs egyetlen tagja sem a, midenki nevű csoportnak.");
        }
    }
    @endcode
 **/
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum, amit a fetchMeber() metődusnál is használltunk,
    ///          ill. ami au ott kezdeményezett lekérdezés eredményét tartalmazza.
    /// @return Ha be tudott olvasni további member rekordot, akkor true, egyébként false.
    bool fetchNextMember(QSqlQuery& __q) {
        bool r = __q.next();
        if (r) member.set(__q);
        else   member.set();
        return r;
    }
    /// Feltételezi, hogy a member objektumnak az ID mezője ki van töltve.
    /// Lekérdezi a megadott meber group rekordjait.
    /// Ha vannak groupok, akkor az elsőt beolvassa a group objektumba.
    /// Ha viszont nincsenek groupok, akkor törli a group adattagot.
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @return True, ha volt legalább egy group, false, ha nem.
    bool fetchFirstGroup(QSqlQuery& __q) {
        QString sql = QString("SELECT * FROM %1 JOIN %2 USING(%3) WHERE %4 = %5")
                .arg(tableName()).arg(group.tableName).arg(groupIdName())
                .arg(groupIdName()).arg(member.getId());
        if (!__q.exec(sql)) SQLPREPERR(__q, sql);
        bool r = __q.first();
        if (r) group.set(__q);
        else   group.set();
        return r;
    }
    /// A fetchGroup(QSqlQuery& __q) metódus hívása után a további group rekordok beolvasása
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @return Ha be tudott olvasni további group rekordot, akkor true, egyébként false.
    bool fetchNextGroup(QSqlQuery& __q) {
        bool r = __q.next();
        if (r) group.set(__q);
        else   group.set();
        return r;
    }
    /// A metődus feltételezi, hogy a group és member objektumoknak ki van töltve az ID mezőjük, és ez alapján beinzertálja a kapcsoló rekordot.
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param __ex Hiba kapcsoló, __ex = EX_IGNORE esetén nincs hiba kezelés.
    /// @return Ha sikerült az insert, akkor true, ha hiba történt vagy nem volt változás, akkor false
    /// @exception cError* Hiba esetén, ha __ex értéke nem EX_IGNORE, ill. ha nincs művelet (a kapcsoló rekord már létezik),
    ///     és __ex értéke EX_NOOP.
    bool insert(QSqlQuery& __q, enum eEx __ex = EX_NOOP) {
        QString sql = "INSERT INTO " + tableName() +
                QChar('(') + group.idName() + QChar(',') + member.idName() + QChar(')') +
                " VALUES(" + QString::number(group.getId()) + QChar(',') + QString::number(member.getId()) + QChar(')');
        if (__ex < EX_NOOP) {
            sql += QString("ON CONFLICT (%1,%2) DO NOTHING").arg(group.idName(), member.idName());
        }
        _DBGFNL() << "SQL:" << sql << endl;
        if (!__q.exec(sql)) {
            if (__ex == EX_IGNORE) {
                SQLQUERYERRDEB(__q);
                return false;
            }
            else SQLQUERYERR(__q)
        }
        return __q.numRowsAffected() == 1;
    }
    /// Egy kapcsoló rekord törlése.
    /// Ha megadjuk az __id opcionális paramétert, és annak értéke nem NULL_ID, akkor a megadott azonosítójú kapcsoló rekordot törli.
    /// Ha __id -t nem adjuk meg, ill. értéke NULL_ID, akkor feltételezi, hogy a group és member adattagok ID mezője ki van töltve, és az így
    /// azonosítható kapcsoló rekordot fogja törölni.
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param __id opcionális kapcsoló rekord ID
    /// @param __ex Hiba kapcsoló, __ex = EX_IGNORE esetén nincs hiba kezelés.
    /// @return Ha sikerült a törlés, akkor true, ha hiba történt vagy nem volt változás, akkor false
    /// @exception cError* Hiba esetén, ha __ex értéke nem EX_IGNORE, ill. ha nincs művelet (a kapcsoló rekord nem létezik),
    ///     és __ex értéke EX_NOOP.
    bool remove(QSqlQuery& __q, qlonglong __id = NULL_ID, enum eEx __ex = EX_NOOP) {
        QString sql = "DELETE FROM " + tableName() + " WHERE ";
        if (__id == NULL_ID) {
            sql +=  group.idName()  + " = " + QString::number(group.getId()) + " AND " +
                    member.idName() + " = " + QString::number(member.getId());
        }
        else {
            sql += idName() + " = " + QString::number(__id);
        }
        if (!__q.exec(sql)) {
            if (__ex == EX_IGNORE) {
                SQLQUERYERRDEB(__q);
                return false;
            }
            else SQLQUERYERR(__q)
        }
        bool r = __q.numRowsAffected() == 1;
        if (!r && __ex == EX_NOOP) EXCEPTION(EFOUND);
        return true;
    }
    /// Beolvassa a megadott group objektum összes tagját
    /// @param q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param g A group tíousú objektum lsd,: fetchFirstMember() metódust.
    static tRecordList<M> fetchMembers(QSqlQuery& q, const G& g) {
        tRecordList<M>  r;
        tGroup<G, M>    gm;
        gm = g;
        if (gm.fetchFirstMember(q)) {
            do {
                r << gm.member;
            } while (gm.fetchNextMember(q));
        }
        return r;
    }
    /// Beolvassa a megadott group objektum összes tagját
    /// @param q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param gid A group tíousú objektum ID-je.
    static tRecordList<M> fetchMembers(QSqlQuery& q, qlonglong gid)     { return fetchMembers(q, G().setById(q, gid)); }
    /// Beolvassa a megadott group objektum összes tagját
    /// @param q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param gn A group tíousú objektum neve.
    static tRecordList<M> fetchMembers(QSqlQuery& q, const QString& gn) { return fetchMembers(q, G().setByname(q, gn)); }
    /// Beolvassa a megadott member objektum összes csoportját, aminek tagja
    /// @param q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param m A member tíousú objektum lsd.: fetchFirstGroup() metüdust.
    static tRecordList<G> fetchGroups(QSqlQuery& q, const M& m) {
        tRecordList<G>  r;
        tGroup<G, M>    gm;
        gm = m;
        if (gm.fetchFirstGroup(q)) {
            do {
                r << gm.group;
            } while (gm.fetchNextGroup(q));
        }
        return r;
    }
    /// Beolvassa a megadott member objektum összes csoportját, aminek tagja
    /// @param q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param mid A member tíousú objektum ID-je
    static tRecordList<M> fetchGroups(QSqlQuery& q, qlonglong mid)     { return fetchGroups(q, G().setById(q, mid)); }
    /// Beolvassa a megadott member objektum összes csoportját, aminek tagja
    /// @param q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param mn A member tíousú objektum neve
    static tRecordList<M> fetchGroups(QSqlQuery& q, const QString& mn) { return fetchGroups(q, G().setByname(q, mn)); }
    //
    int setMemberByGroup(QSqlQuery& q, const QString& gn, const QString& fn, const QVariant& v)
    {
        int r = 0;
        group.setByName(q, gn);
        if (fetchFirstMember(q)) do {
            member.set(fn, v);
            ++r;
        } while (fetchNextMember(q));
        return r;
    }
    //
    int disableMemberByGroup(QSqlQuery& q, const QString& gn, bool f = true) { return setMemberByGroup(q, gn, _sDisabled, QVariant(f)); }
};

class cGroupAny : public tGroup<cRecordAny, cRecordAny> {
public:
    cGroupAny(const cRecStaticDescr *_gd, const cRecStaticDescr *_md)
        : tGroup<cRecordAny, cRecordAny>()
    {
        group.setType(_gd);
        member.setType(_md);
    }
    cGroupAny(const cRecordAny& _g, const cRecordAny _m)
        : tGroup<cRecordAny, cRecordAny>(_g, _m)
    {
        ;
    }
};

/**********************************************************************************************************************/

template <class O> class tTreeItem {
protected:
    tTreeItem   *       _pParent;
    QList<tTreeItem *>  _childList;
public:
    O *                 pData;
    tTreeItem(O * __pd, tTreeItem * __par) : _pParent(__par), _childList(), pData(__pd) {
        if (__par != nullptr) {
            _pParent->_childList << this;
        }
    }
    ~tTreeItem() {
        clear();
    }
    void clear() {
        if (!_childList.isEmpty()) delete _childList.first();
        if (_pParent != 0) {
            int ix = indexOnParent();
            if (!isContIx(_pParent->_childList ,ix)) {
                EXCEPTION(EPROGFAIL);
            }
            _pParent->_childList.removeAt(ix);
        }
        pDelete(pData);
    }
    tTreeItem * addChild(O * p) { return new tTreeItem(p, this); }
    tTreeItem& operator << (O * p) { addChild(p); return *this; }
    bool isRoot() const { return _pParent == nullptr; }
    int childNumber() const { return _childList.size(); }
    tTreeItem * parent() { return _pParent; }
    tTreeItem * root() { return _pParent == nullptr ? this : _pParent->root(); }
    const QList<tTreeItem *> * siblings() const { return _pParent == nullptr ? nullptr : &_pParent->_childList; }
    int indexOnParent() { return _pParent == nullptr ? NULL_IX : _pParent->_childList.indexOf(this); }
    tTreeItem * siblingAt(int __ix) {
        const QList<tTreeItem *> * ps =siblings();
        if (ps != nullptr && isContIx(*ps, __ix)) return (*ps)[__ix];
        return nullptr;
    }
    tTreeItem * nextSibling() {
        int ix = indexOnParent();
        if (ix < 0) return nullptr;
        ++ix;
        return siblingAt(ix);
    }
    tTreeItem * prevSibling() {
        int ix = indexOnParent();
        if (ix < 0) return nullptr;
        --ix;
        return siblingAt(ix);
    }
};


#endif // LV2CONT_H
