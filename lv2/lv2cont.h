#ifndef LV2CONT_H
#define LV2CONT_H

/*!
@file lv2cont.h
Adatbázis kezelő objektum konténerek ill. template-k
*/

#include <QList>
#include <lv2datab.h>

/*!
@class tRecordList
Template osztály. Rekord lista konténer.

A konténer az objektumok pointereit tárolja, amik a konténer objektum „tulajdonába” kerülnek, vagyis azokat a destruktor felszabadítja.
A konténer másolásakor az összes elem klónozva lesz, ezért ez a művelet, főleg nagyobb listák esetén kerülendő, és lásd még a klónozásnál
leírtakat is cRecord::dup() .
Az ős QList konténer metódusain kívül mit nyújt a tRecordList konténer?
A pointer elemeket az elem törlésekor felszabadítja, továbbá ha referenciával adjuk meg a konténerbe elhelyezendő objektumot (pointer helyett),
akkor klónozza azt, és a klónt helyezi a konténerbe.
Egy elem kereshető, ill. elérhető egy mező értéke alapján: contains() és indexOff(), get() metódusok.
Lehetőség van egy mező értékének beállítására az összes elemnél: set(), setId(), setName().
Az osztály biztosít néhání adatbázis műveletet végző metódust: fetch(), insert()

Azokat a metódusokat, melyek a mező indexe alapján azonosítanak egy mezőt, csak akkor használhatjuk, ha a konténerben tárolt
objektumok valóban azonos típusuak, ill. ugyan az a cRecStaticDescr objektum a leírójuk.
*/
template <class T>
        class tRecordList : public QList<T *>
{
public:
    /// Konstruktor. Üres rekord listát hoz létre
    tRecordList() : QList<T *>() { ; }
    /// Konstruktor. Egy elemű rekord listát hoz létre (T a rekord objektum típus)
    tRecordList(T * __p) : QList<T *>()
    {
        this->append(__p);
    }
    /// Konstruktor. Egy elemű listát hoz létre
    /// A paraméterként megadott objektumról másolatot készít.
    tRecordList(const T& __o) : QList<T *>()
    {
        T * p =  dynamic_cast<T *>(__o.dup());
        this->append(p);
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
    ///
    QList<T *>& list()  { return *(QList<T *> *)this; }
    const QList<T *>& list() const { return *(const QList<T *> *)this; }
    /// Kiüríti a konténert, az összes elemet (pointerek) felszabadítja
    tRecordList& clear()
    {
        typename QList<T *>::iterator    i;
        for (i = QList<T *>::begin(); i != QList<T *>::end(); i++) delete *i;
        QList<T *>::clear();
        return *this;
    }
    /// Bővíti a listát egy elemmel. A megadott pointer helyezi el a konténerben, nem készít másolatot.
    tRecordList<T>& operator <<(T *p) {
        this->append(p);
        return *this;
    }
    /// Bővíti a listát egy elemmel. Az objektumról másolatot készít, és azt teszi be a konténerbe
    /// @note A két << operátor lehet, hogy egy kicsit "beugratós", változtatni kéne ?
    tRecordList<T>& operator <<(const T& _o) {
        T * p =  dynamic_cast<T *>(_o.dup());
        this->append(p);
        return *this;
    }
    /// Copy operátor.
    /// A konténer összes objektumát újra allokálja (a dup(() metódus hívásával), és ezek kerülnek a másolat konténer objektumba.
    tRecordList& operator=(const tRecordList& __o)
    {
        //DBGFN();
        clear();
        typename QList<T *>::const_iterator    i;
        for (i = __o.constBegin(); i != __o.constEnd(); i++) {
            T * p =  dynamic_cast<T *>((*i)->dup());
            this->append(p);
        }
        //DBGFNL();
        return *this;
    }
    /// Copy konstruktor.
    /// A konténer összes objektumát újra allokálja, és ezek kerülnek a másolat objektumba.
    tRecordList(const tRecordList& __o) : QList<T *>()
    {
        DBGFN();
        *this = __o;
        DBGFNL();
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
    /// objektum megadott mezőinek. Csak egyenlőségre szűr, kivébe, ha beállítjuk a *p objektumban a likeMask bitjeit.
    /// A metódus akkor is használható, ha a template osztály onmagában nem azonosítja a kezelt adattáblát,
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
            this->append(p);
            while (__q.next()) {
                p = dynamic_cast<T *>(p->newObj());
                p->set(__q.record());
                this->append(p);
            }
        }
        else {
            delete p;
        }
        return QList<T *>::count();
    }
    ///
    int fetchByNamePattern(QSqlQuery& __q, const QString& __pat, bool __only = false, T *p = NULL)
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
    int insert(QSqlQuery& __q, bool __ex = true)  {
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
    int remove(QSqlQuery& __q, bool __ex = true)  {
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
            if ((*i)->isNullName() == false && (*i)->getName() == __name) return i - QList<T *>::constBegin();
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
            if (__st >= (int)this->size()) return -1;
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
            if (__st >= (int)this->size()) return -1;
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
        QString s = QChar('[');
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            T *p = *i;
            s += QChar(' ') + p->toString() + QChar(',');
        }
        if (s.size() > 1) s.chop(1);
        return s + QChar(' ') + QChar(']');
    }
    /// Az összes elem ID mezőjének a törlése
    void clearId() {
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            (*i)->clear((*i)->idIndex());
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
    T *get(qlonglong __id, bool __ex = true) {
        int i = indexOf(__id);
        if (i < 0) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, __id, QObject::trUtf8("Keresés a rekord ID alapján: %1 = %2").arg(T().idName()).arg(__id));
        }
        return this->at(i);
    }
    /// A konténer egy elemének a lekérése az név alapján
    /// Ha nincs név mező, vagy nem ismert az indexe, akkor dob egy kizárást
    /// @param __nm a keresett név értéke
    /// @param __ex Ha értéke true, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatér.
    T *get(const QString& __nm, bool __ex = true) {
        int i = indexOf(__nm);
        if (i < 0) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, 0, QObject::trUtf8("Keresés a rekord név alapján: %1 = %2").arg(T().nameName()).arg(__nm));
        }
        return this->at(i);
    }
    /// A konténer egy elemének a lekérése amegadott nevű mező értéke alapján
    /// Ha nincs ilyen nevű mező, akkor dob egy kizárást
    /// @param __nm A mező neve, ami alapján keresünk
    /// @param __v A keresett érték
    /// @param __ex Ha értéke true, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatér.
    T *get(const QString& __fn, const QVariant& __v, bool __ex = true) {
        int i = indexOf(__fn, __v);
        if (i < 0) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, 0, QObject::trUtf8("Keresés a %1 nevű mezőérték alapján: %2").arg(__fn).arg(debVariantToString(__v)));
        }
        return this->at(i);
    }
    /// A konténer egy elemének a lekérése amegadott sorszámú mező értéke alapján
    /// Ha nincs ilyen mező, akkor dob egy kizárást
    /// @param __ix A mező sorszáma, ami alapján keresünk
    /// @param __v A keresett érték
    /// @param __ex Ha értéke true, akkor ha nem találja az elemet, akkor dob egy kizárást, egyébként NULL pointerrel visszatér.
    T *get(int __ix, const QVariant& __v, bool __ex = true) {
        int i = indexOf(__ix, __v);
        if (i < 0) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, __ix, QObject::trUtf8("Keresés a %1 indexű mezőérték alapján: %2").arg(__ix).arg(debVariantToString(__v)));
        }
        return this->at(i);
    }
    /// A konténer utolsó pointerét törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer üres, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    T *pop_back(bool __ex = true)  {
        if (list().size() < 1) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, 0);
        }
        T *p = QList<T *>::back();
        QList<T *>::pop_back();
        return p;
    }
    /// A konténer első pointerét törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer üres, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    T *pop_front(bool __ex = true) {
        if (list().size() < 1) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, 0);
        }
        T *p = QList<T *>::front();
        QList<T *>::pop_front();
        return p;
    }
    /// A megadott indexű pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott indexű elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __ix A kiválasztott elem indexe a konténerben
    /// @param __ex Opcionális flag, ha igaz (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pullAt(int __ix, bool __ex = true) {
        if (list().size() <= __ix) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, __ix);
        }
        T *p = QList<T *>::at(__ix);
        QList<T *>::removeAt(__ix);
        return p;
    }
    /// A megadott ID-jű objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __id A keresett elem rekord azobosító ID-je
    /// @param __ex Opcionális flag, ha igaz (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(qlonglong __id, bool __ex = true) {
        int ix = indexOf(__id);
        if (0 > ix) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
    /// A megadott nevű objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __id A keresett elem rekord nevee
    /// @param __ex Opcionális flag, ha igaz (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(const QString __name, bool __ex = true) {
        int ix = indexOf(__name);
        if (0 > ix) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
    /// A megadott nevű mező értékévlel egyezést mutató első objektumra mutató pointert törli a konténerből, de a pointert nem szabadítja föl, hanem azzal tér vissza.
    /// Ha a konténer nem tartalmazza a megadott elemet, akkor dob egy kizárást, ha __ex igaz, vagy NULL pointerrel tér vissza, ha __ex hamis.
    /// @param __fname Mező név
    /// @param __val A keresett rekordban a megadott nevű mező értéke.
    /// @param __ex Opcionális flag, ha igaz (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(const QString __fname, const QVariant& __val, bool __ex = true) {
        int ix = indexOf(__fname, __val);
        if (0 > ix) {
            if (__ex == false) return NULL;
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
    /// @param __ex Opcionális flag, ha igaz (ez az alapértelmezés) akkor ha nincs találat, dob egy kizárást.
    T *pull(int __fix, const QVariant& __val, bool __ex = true) {
        int ix = indexOf(__fix, __val);
        if (0 > ix) {
            if (__ex == false) return NULL;
            EXCEPTION(EFOUND, __ex);
        }
        T *p = QList<T *>::at(ix);
        QList<T *>::removeAt(ix);
        return p;
    }
};

template<class T> QTextStream& operator<<(QTextStream& __t, const tRecordList<T>& __v) { return __t << toString(__v); }

template <class T>
        class tOwnRecords : public tRecordList<T>
{
public:
    /// Az owner rekord id (távoli kulcs) mező indexe.
    const int ixOwnerId;
    /// Konstruktor, üres konténert hoz létre
    /// @param _ix_owner_id Az owner rekord id (távoli kulcs) mező indexe.
    tOwnRecords(int _ix_owner_id) : tRecordList<T>(), ixOwnerId(_ix_owner_id) { ; }
    /// Konstruktor, Egy egy elemű konténert hoz létre
    /// @param _ix_owner_id Az owner rekord id (távoli kulcs) mező indexe.
    /// @param _p Az elem, amit a konténerbe elhelyet (a pointer az objektumhoz kerül, a destruktora felszabadítja a pointert).
    tOwnRecords(int _ix_owner_id, T *_p) : tRecordList<T>(_p), ixOwnerId(_ix_owner_id) { ; }
    /// Konstruktor, Egy egy elemű konténert hoz létre
    /// @param _ix_owner_id Az owner rekord id (távoli kulcs) mező indexe.
    /// @param _p Az objektumról másolata kerül a kontéberbe
    tOwnRecords(int _ix_owner_id, T& _o) : tRecordList<T>(_o), ixOwnerId(_ix_owner_id) { ; }
    /// Konstruktor.
    /// Beolvassa az összes vagy adott típusú owner-hez tatozó rekordot
    /// @param __q query objektum, amivel a lekérdezés elvégezhető.
    /// @param __only
    /// @param __fi Az owner rekord id (távoli kulcs) mező indexe.
    /// @param __id Az owner rekord ID.
    tOwnRecords(QSqlQuery& __q, bool __only, int __fi, qlonglong __id) : tRecordList<T>(__q, __only, __fi, __id), ixOwnerId(__fi) { ; }
    /// Copy konstruktor. A konténer elemeiről másolatot készít, és ezt helyezi el az új konténerbe
    tOwnRecords(const tOwnRecords& __o) : tRecordList<T>((const tRecordList<T>&)__o) , ixOwnerId(__o.ixOwnerId) { ; }
    /// Másoló operátor. Az elemekről másolatot készít.
    /// Ha a két objektumban az ixOwnerId mező nem egyenlő akkor kizárást dob.
    tOwnRecords& operator =(const tOwnRecords& __o) {
        if (ixOwnerId != __o.ixOwnerId) EXCEPTION(EDATA);
        return (tOwnRecords&)(this->tRecordList<T>::operator =(__o));
    }
    void setsOwnerId(qlonglong __owner_id = NULL_ID) { this->tRecordList<T>::setsId(ixOwnerId, __owner_id); }
    int fetch(QSqlQuery &__q, qlonglong __owner_id, bool __only = false) {
        return this->tRecordList<T>::fetch(__q, __only, ixOwnerId, __owner_id);
    }
    int insert(QSqlQuery &__q, qlonglong __owner_id, bool __ex) {
        if (tRecordList<T>::size() == 0) return 0;
        setsOwnerId(__owner_id);
        return this->tRecordList<T>::insert(__q, __ex);
    }
    int removeByOwn(QSqlQuery &__q, qlonglong __owner_id, bool __ex) const {
        T o;
        o.setId(ixOwnerId, __owner_id);
        return o.remove(__q, false, o.mask(ixOwnerId), __ex);
    }
    bool replace(QSqlQuery &__q, qlonglong __owner_id, bool __ex) {
        // Ha nincs új rekord, csak a régieket töröljük
        if (tRecordList<T>::size() == 0) {
            removeByOwn(__q, __owner_id, false);    // Ha nem töröl semmit, az nem hiba!
            return true;
        }
        // Megjelöljük a régi rekordokat, ha egy sincs akkor csak beillesztjük az újakat
        if (mark(__q, __owner_id) == 0) {
            return 0 < insert(__q, __owner_id, __ex);   // A nulla fura lenne
        }
        int r = 0;
        typename QList<T *>::const_iterator    i;
        for (i = QList<T *>::constBegin(); i < QList<T *>::constEnd(); i++) {
            PDEB(VERBOSE) << "Replace : " << (*i)->toString() << endl;
            (*i)->setFlag(false);
            if ((*i)->replace(__q, __ex) != R_ERROR) ++r;
        }
        removeMarked(__q, __owner_id, __ex);    // Ha flag = true maradt, akkor töröljük
        return r == QList<T *>::size();
    }
    int mark(QSqlQuery &__q, qlonglong __owner_id) const {
        T o;
        return o.setId(ixOwnerId, __owner_id).mark(__q, o.mask(__owner_id), true);
    }
    int removeMarked(QSqlQuery& __q, qlonglong __owner_id, bool __ex = true) const {
        T o;
        return o.setId(ixOwnerId, __owner_id).removeMarked(__q, o.mask(__owner_id, __ex));
    }
};

template <class R>
        class tOwnParams :public tRecordList<R>
{
public:
    /// Az owner rekord id (távoli kulcs) mező indexe.
    const int ixOwnerId;
    /// A típus rekord id (távoli kulcs) mező indexe.
    const int ixParamTypeId;
    /// Konstruktor, üres konténert hoz létre
    /// @param _ix_owner_id Az owner rekord id (távoli kulcs) mező indexe.
    tOwnParams(int _ix_owner_id) : tRecordList<R>(), ixOwnerId(_ix_owner_id), ixParamTypeId(R().toIndex(_sParamTypeId)) { ; }
    /// Konstruktor, Egy egy elemű konténert hoz létre
    /// @param _ix_owner_id Az owner rekord id (távoli kulcs) mező indexe.
    /// @param _p Az elem, amit a konténerbe elhelyet (a pointer az objektumhoz kerül, a destruktora felszabadítja a pointert).
    tOwnParams(int _ix_owner_id, R *_p) : tRecordList<R>(_p), ixOwnerId(_ix_owner_id), ixParamTypeId(_p->toIndex(_sParamTypeId)) { ; }
    /// Konstruktor, Egy egy elemű konténert hoz létre
    /// @param _ix_owner_id Az owner rekord id (távoli kulcs) mező indexe.
    /// @param _p Az objektumról másolata kerül a kontéberbe
    tOwnParams(int _ix_owner_id, R& _o) : tRecordList<R>(_o), ixOwnerId(_ix_owner_id), ixParamTypeId(_o.toIndex(_sParamTypeId)) { ; }
    /// Konstruktor.
    /// Beolvassa az összes vagy adott típusú owner-hez tatozó rekordot
    /// @param __q query objektum, amivel a lekérdezés elvégezhető.
    /// @param __only
    /// @param __fi Az owner rekord id (távoli kulcs) mező indexe.
    /// @param __id Az owner rekord ID.
    tOwnParams(QSqlQuery& __q, bool __only, int __fi, qlonglong __id) : tRecordList<R>(__q, __only, __fi, __id), ixOwnerId(__fi), ixParamTypeId(R().toIndex(_sParamTypeId)) { ; }
    /// Copy konstruktor. A konténer elemeiről másolatot készít, és ezt helyezi el az új konténerbe
    tOwnParams(const tOwnParams& __o) : tRecordList<R>((const tRecordList<R>&)__o) , ixOwnerId(__o.ixOwnerId), ixParamTypeId(__o.ixParamTypeId) { ; }
    /// Másoló operátor. Az elemekről másolatot készít.
    /// Ha a két objektumban az ixOwnerId mező nem egyenlő akkor kizárást dob.
    tOwnParams& operator =(const tOwnParams& __o) {
        if (ixOwnerId != __o.ixOwnerId) EXCEPTION(EDATA);
        if (ixParamTypeId != __o.ixParamTypeId) EXCEPTION(EDATA);
        return (tOwnParams &)(this->tRecordList<R>::operator =(__o));
    }
    void setsOwnerId(qlonglong __owner_id = NULL_ID) { this->tRecordList<R>::setsId(ixOwnerId, __owner_id); }
    int fetch(QSqlQuery &__q, qlonglong __owner_id, bool __only = false) {
        return this->tRecordList<R>::fetch(__q, __only, ixOwnerId, __owner_id);
    }
    int insert(QSqlQuery &__q, qlonglong __owner_id, bool __ex) {
        this->tRecordList<R>::setsId(ixOwnerId, __owner_id);
        return this->tRecordList<R>::insert(__q, __ex);
    }
    int ownRemove(QSqlQuery &__q, qlonglong __owner_id, bool __ex) {
        R o;
        o.setId(ixOwnerId, __owner_id);
        return o.remove(__q, false, o.mask(ixOwnerId), __ex);
    }
    int replace(QSqlQuery &__q, qlonglong __owner_id, bool __ex) {
        if (tRecordList<R>::size() == 0) return ownRemove(__q, __owner_id, __ex);
        untouch(__q, __owner_id);
        int r = 0;
        typename QList<R *>::const_iterator    i;
        for (i = QList<R *>::constBegin(); i < QList<R *>::constEnd(); i++) {
            PDEB(VERBOSE) << "Replace : " << (*i)->toString() << endl;
            (*i)->setBool(_sTouch, true);
            if ((*i)->replace(__q, __ex) != R_ERROR) ++r;
        }
        return r + removeUntouched(__q, __owner_id, __ex);
    }
    int untouch(QSqlQuery &__q, qlonglong __owner_id) {
        R o;
        QString sql = "UPDATE " + o.tableName() + " SET touch = false WHERE " + o.columnName(ixOwnerId) + " = ?";
        execSql(__q, sql, __owner_id);
        return __q.numRowsAffected();
    }
    int removeUntouched(QSqlQuery& __q, qlonglong __owner_id, bool __ex = true) {
        R o;
        int ixTouch = o.toIndex(_sTouch, __ex);
        o.setBool(ixTouch, false);
        o.setId(ixOwnerId, __owner_id);
        return o.remove(__q, false, o.mask(ixTouch, ixOwnerId), __ex);
    }
    /// Index operátor: egy elem a paraméter név alapján
    const R& operator[](const QString& __n) const
    {
        cRecordAny t(R().paramType.decr());
        if (!t.fetchByName(__n)) EXCEPTION(EFOUND, 1, __n);
        int i;
        i = tRecordList<R>::indexOf(_sParamTypeName, QVariant(__n));
        if (i < 0) EXCEPTION(EFOUND, 2, __n);
        return *tRecordList<R>::at(i);
    }
    /// Index operátor: egy elem a paraméter név alapján
    R& operator[](const QString& __n)
    {
        cRecordAny t(&R().paramType.descr());
        if (!t.fetchByName(__n)) EXCEPTION(EFOUND, 1, __n);
        int i = tRecordList<R>::indexOf(_sParamTypeName, QVariant(__n));
        if (i < 0) {
            R * pr = new R();
            pr->setType(t.getId());
            this->append(pr);
            return *pr;
        }
        return *tRecordList<R>::at(i);
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
    /// @param __ex Ha értéke true, akkor hiba esettén dob egy kizárást, egyébként hiba esetén false-val tér vissza.
    /// @return Ha sikerült az insert, akkor true, ha hiba történt, és __ex = false, akkor false.
    bool insert(QSqlQuery& __q, bool __ex = true) {
        QString sql = "INSERT INTO " + tableName() +
                QChar('(') + group.idName() + QChar(',') + member.idName() + QChar(')') +
                " VALUES(" + QString::number(group.getId()) + QChar(',') + QString::number(member.getId()) + QChar(')');
        _DBGFNL() << "SQL:" << sql << endl;
        if (!__q.exec(sql)) {
            // Pontosítani kéne, sajnos nincs hibakód!!
            if (__ex == false && __q.lastError().type() == QSqlError::StatementError) {
                SQLQUERYERRDEB(__q);
                return false;
            }
            else SQLQUERYERR(__q)
        }
        return true;
    }
    /// Egy kapcsoló rekord törlése.
    /// Ha megadjuk az __id opcionális paramétert, és annak értéke nem NULL_ID, akkor a megadott azonosítójú kapcsoló rekordot törli.
    /// Ha __id -t nem adjuk meg, ill. értéke NULL_ID, akkor feltételezi, hogy a group és member adattagok ID mezője ki van töltve, és az így
    /// azonosítható kapcsoló rekordot fogja törölni.
    /// @param __q Az adabázisművelethez használt QSqlQuery objektum.
    /// @param __id opcionális kapcsoló rekord ID
    /// @param __ex Ha értéke true, akkor hiba esettén dob egy kizárást, egyébként hiba esetén false-val tér vissza.
    /// @return Ha sikerült a törlés, akkor true, ha hiba történt, és __ex = false, akkor false.
    bool remove(QSqlQuery& __q, qlonglong __id = NULL_ID, bool __ex = true) {
        QString sql = "DELETE FROM " + tableName() + " WHERE ";
        if (__id == NULL_ID) {
            sql +=  group.idName()  + " = " + QString::number(group.getId()) + " AND " +
                    member.idName() + " = " + QString::number(member.getId());
        }
        else {
            sql += idName() + " = " + QString::number(__id);
        }
        if (!__q.exec(sql)) {
            // Pontosítani kéne, sajnos nincs hibakód!!
            if (__ex == false && __q.lastError().type() == QSqlError::StatementError) {
                SQLQUERYERRDEB(__q);
                return false;
            }
            else SQLQUERYERR(__q)
        }
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



#endif // LV2CONT_H
