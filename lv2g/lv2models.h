#ifndef LV2MODELS_H
#define LV2MODELS_H

#include "lv2g.h"
#include "cerrormessagebox.h"
#include <QAbstractTableModel>
#include <QStringListModel>

_GEX QVector<int> mil2rowsAsc(const QModelIndexList& mil);
_GEX QVector<int> mil2rowsDesc(const QModelIndexList& mil);


/// @class cStringListModel
/// Lista modell string lista megjelenítéséhez.
class LV2GSHARED_EXPORT cStringListModel: public QAbstractListModel {
public:
    /// Konstruktor
    /// Az elemek sorszámozását kikapcsolja.
    cStringListModel(QObject *__par = nullptr);
    /// A _stringList konténer adattag méretét adja vissza
    virtual int rowCount(const QModelIndex &parent) const;
    /// Egy oszlopos megjelenítés (esetleg egy sorszám oszlop, mint heder), mindíg 1-el tér vissza.
    virtual int columnCount(const QModelIndex &parent) const;
    /// Cella adatként a _stringList megfelelő elemét, az igazítás típusaként pedig balra igazítást, egyébb esetben 'NULL'-t ad vissza.
    virtual QVariant data(const QModelIndex &index, int role) const;
    /// Ha sorszámozás van megadva, akkor a kért sorszámot adja vissza.
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    /// Az aktuális string konténer referenciáját adja vissza.
    const QStringList& stringList() const       { return _stringList; }
    /// A megadott sring lista megjelenításe. A konténert bemásolja _stringList adattag konténerbe.
    bool setStringList(const QStringList& sl);
    /// Az aktuális lista (_stringList konténer) a végéhez ad egy sort, és az új lista lessz megjelenítve.
    cStringListModel& operator<<(const QString& s) {
        return insert(s);
    }
    /// Beszúr egy elemet a megadott indexű sor elé
    cStringListModel& insert(const QString& s, int i = -1) {
        if (i < 0) i = _stringList.size();
        else if (i > _stringList.size()) EXCEPTION(ENOINDEX);
        beginInsertRows(QModelIndex(), i, i);
        _stringList.insert(i, s);
        endInsertRows();
        return *this;
    }
    /// Módosítja a megadott indexű egy elemet
    cStringListModel& modify(const QString& s, int i) {
        if (isContIx(_stringList, i)) {
            beginResetModel();
            _stringList[i] = s;
            endResetModel();
        }
        else EXCEPTION(ENOINDEX);
        return *this;
    }
    /// Törli a megadott indexű sort
    cStringListModel& remove(int i) {
        if (isContIx(_stringList, i)) {
            beginRemoveRows(QModelIndex(), i, i);
            _stringList.removeAt(i);
            endRemoveRows();
        }
        else EXCEPTION(ENOINDEX);
        return *this;
    }
    /// Törli az utolsó sort, ha üres a lista, akkor nem csinál semmit.
    cStringListModel& pop_back() {
        if (_stringList.isEmpty()) return *this;
        return remove(_stringList.size() -1);
    }
    /// Törli az első sort, ha üres a lista, akkor nem csinál semmit.
    cStringListModel& pop_front() {
        if (_stringList.isEmpty()) return *this;
        return remove(0);
    }
    /// A megjelenítendő lista kiürítése, a táblázat újra rajzolása
    cStringListModel& clear() {
        beginResetModel();
        _stringList.clear();
        endResetModel();
        return *this;
    }
    /// Az index listában kijelölt elemeket törli.
    cStringListModel& remove(QModelIndexList& mil);
    cStringListModel& up(const QModelIndexList& mil);
    cStringListModel& down(const QModelIndexList& mil);

    /// A sorszámozás állapotát adja vissza
    bool rowNumbers() const                     { return _rowNumbers; }
    /// A sorszámozás kéreése, vagy tiltása
    /// @param b Ha kérünk sorszámokat, akkor teue, egyébként false.
    void setRowNumers(bool b)                   { _rowNumbers = b; }
    /// A lista aktuális mérete
    int size() const                            { return _stringList.size(); }
    /// Ha az aktuális lista üres, akkor true értékkel tér vissza.
    int isEmpty() const                         { return _stringList.isEmpty(); }
protected:
    /// Az aktuálisan megjelenített string lista konténer
    QStringList _stringList;
    /// A sorszámozás megjelenítésének az aktuális állpota.
    bool        _rowNumbers;
};

/// @class cStringListDecModel
/// Lista modell string lista megjelenítéséhez. + dekoráció
class LV2GSHARED_EXPORT cStringListDecModel: public QAbstractListModel {
public:
    /// Konstruktor
    cStringListDecModel(QObject *__par = nullptr);
    ~cStringListDecModel();
    /// A _stringList konténer adattag méretét adja vissza
    virtual int rowCount(const QModelIndex &parent) const;
    /// Egy oszlopos megjelenítés (esetleg egy sorszám oszlop, mint heder), mindíg 1-el tér vissza.
    virtual int columnCount(const QModelIndex &parent) const;
    /// Cella adatként a _stringList megfelelő elemét, az igazítás típusaként pedig balra igazítást, egyébb esetben 'NULL'-t ad vissza.
    virtual QVariant data(const QModelIndex &index, int role) const;
    /// Ha sorszámozás van megadva, akkor a kért sorszámot adja vissza.
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    /// Az aktuális string konténer referenciáját adja vissza.
    const QStringList& stringList() const       { return _stringList; }
    /// A megadott sring lista megjelenításe. A konténert bemásolja _stringList adattag konténerbe.
    /// A dokoráció alapértelmezett lessz;
    cStringListDecModel &setStringList(const QStringList& sl);
    /// A megadott sring lista megjelenításe. A konténert bemásolja _stringList adattag konténerbe.
    cStringListDecModel & setLists(const QStringList& sl, const QList<const cEnumVal *> &decs);
    cStringListDecModel &setDecorationAt(int ix, const cEnumVal * pe);
    cStringListDecModel &setDefDecoration(const cEnumVal * pe) { defDecoration = pe; return *this; }
    const cEnumVal * getDefDecoration() const { return defDecoration; }
    const cEnumVal * getDecorationAt(int ix) const;
    int indexOf(const QString& s) const { return _stringList.indexOf(s); }
protected:
    /// Az aktuálisan megjelenített string lista konténer
    QStringList _stringList;
    /// Az alapértelmezett dekoráció
    const cEnumVal * defDecoration;
    /// A soronkénti dekorációk
    QList<const cEnumVal *> * decorations;
};

/// Model: polygon pontjainak megjelenítése egy QTableView objektummal
class LV2GSHARED_EXPORT cPolygonTableModel : public QAbstractTableModel {
public:
    cPolygonTableModel(QObject *__par = nullptr);
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    /// A megjelenítendő polygon megadása, a táblázat újra rajzolása
    /// @return Ha a megadott polygon üres, akkor false, egyébként true
    bool setPolygon(const tPolygonF &__pol);
    /// Az aktuális megjelenített polygon referenciáját adja vissza
    const tPolygonF& polygon() const;
    /// Az aktuális polygonnak a végéhez ad egy pontot
    cPolygonTableModel& operator<<(const QPointF& pt) {
        beginResetModel();
        _polygon << pt;
        endResetModel();
        return *this;
    }
    /// Beszúr egy elemet a megadott indexű pont elé
    cPolygonTableModel& insert(const QPointF& pt, int i = -1) {
        if (i < 0) i = _polygon.size();
        beginResetModel();
        _polygon.insert(i, pt);
        endResetModel();
        return *this;
    }
    /// Törli a megadott indexű pontot a polygonból
    cPolygonTableModel& remove(int i) {
        beginResetModel();
        _polygon.removeAt(i);
        endResetModel();
        return *this;
    }
    /// Törli az utolsó pontot, ha üres a polygon, akkor nem csinál semmit.
    cPolygonTableModel& pop_back() {
        if (_polygon.isEmpty()) return *this;
        return remove(_polygon.size() -1);
    }
    /// Törli az első pontot, ha üres a polygon, akkor nem csinál semmit.
    cPolygonTableModel& pop_front() {
        if (_polygon.isEmpty()) return *this;
        return remove(0);
    }
    /// A megjelenítendő polygon kiürítése, a táblázat újra rajzolása
    cPolygonTableModel& clear() {
        beginResetModel();
        _polygon.clear();
        endResetModel();
        return *this;
    }
    ///
    cPolygonTableModel& up(const QModelIndexList& mil);
    cPolygonTableModel& down(const QModelIndexList& mil);
    cPolygonTableModel& modify(int i, const QPointF& p) {
        if (isContIx(_polygon, i)) {
            beginResetModel();
            _polygon[i] = p;
            endResetModel();
        }
        return *this;
    }
    ///
    cPolygonTableModel& remove(QModelIndexList& mil);
    bool rowNumbers() const                     { return _rowNumbers; }
    void setRowNumers(bool b)                   { _rowNumbers = b; /* ??? */ }
    bool viewHeader() const                     { return _viewHeader; }
    void setViewHeader(bool b)                  { _viewHeader = b; /* ??? */ }
    int size() const                            { return _polygon.size(); }
    int isEmpty() const                         { return _polygon.isEmpty(); }
private:
    /// A megjelenített polygon
    tPolygonF   _polygon;
    bool        _rowNumbers;
    bool        _viewHeader;
};


/// Model: QStringListModel leszármazottja. Egy megadott rekord típus neveinek a listája.
/// A model lekérdezi és tárolja a rekord ID-ket is. A megadott rekord típusnak
/// rendelkeznie kell id és név mezővel is.
class LV2GSHARED_EXPORT cRecordListModel : public QStringListModel {
    Q_OBJECT
public:
    /// Konstruktor.
    /// Eltárolja a statikus descriptor objektumot (referencia).
    /// A szűrés típusát FT_BEGIN -re állítja, a pattern üres lessz, a megjelenített lista is üres lessz.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista feltöltése a setPattern() metódussal lehetséges.
    /// @param __d A rekord leíró objektum
    /// @param _par Parent
    cRecordListModel(const cRecStaticDescr& __d, QObject * __par = nullptr);
    /// Konstruktor.
    /// Eltárolja a statikus descriptor objektumot (referencia).
    /// A szűrés típusát FT_BEGIN -re állítja, a pattern üres lessz, a megjelenített lista is üres lessz.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista feltöltése a setPattern() metódussal lehetséges.
    /// @param __t A rekord tábla név
    /// @param __s Az opcionális schema név
    /// @param _par Parent
    cRecordListModel(const QString& __t, const QString& __s = QString(), QObject * __par = nullptr);
    ~cRecordListModel();
//  void changeRecDescr(cRecStaticDescr * _descr);
    virtual QVariant data(const QModelIndex &index, int role) const;
    /// Beállítja az owner ID-re szűrést, nem frissít.
    /// @param _oid Az ID érték, a szőrés paramétere
    /// @param _fn A rekordban az id mező neve, amire szűrönk, ha ez kideríthető, pl. egy szabályos child rekordnál, ott nem kell megadni,
    ///            Továbbá csak az első híváskor kell megadni, a névet tárolja az objektum.
    /// @param _nullIsAll Mi a teendő, ha a megadott ID érték NULL: TS_TRUE : nincs szőrés. TS_FLSE: minden rekord kiszűrve, TS_NULL: az előző mód.
    /// @param _chkFn A szűrésnél alapértelmezett operátor az egyenlőség, de megadhatunk egy függvényt is, melnek két paramétere a rekordban
    ///             a megadott id mező értéke, a második paraméter pedig a megadott ID paraméter. A föggvény egy boolean értéket ad vissza.
    ///             Csak egyszer kell megadni, az objektum megjegyzi, ha törölni akarjuk, akkor az "=" stringet kell megadni.
    void _setOwnerId(qlonglong _oid, const QString& _fn = QString(), eTristate _nullIsAll = TS_NULL, const QString& _chkFn = QString());
    ///
    void _setOrder(eOrderType _ot, int _ix) { order = _ot; ordIndex = _ix; }
    void _setOrder(eOrderType _ot, const QString& _fn) { _setOrder(_ot, pDescr->toIndex(_fn)); }
    /// Beállítja az owner ID-re szűrést, és frissít.
    void setOwnerId(qlonglong _oid, const QString& _fn = QString(), eTristate _nullIsAll = TS_NULL, const QString& _chkFn = QString())
    {
        _setOwnerId(_oid, _fn, _nullIsAll, _chkFn);
        setFilter();
    }

    /// Opcionális konstans szűrési feltétel megadása, nincs lista frissítés
    /// Az itt megadott feltétel és kapcsolatban lessz a setFilter() metódusban megadottnak.
    /// @param _par A szűrő paramétere
    /// @param __f A szűrés típusa
    void setConstFilter(const QVariant &_par, eFilterType __f);
    /// Szűrési feltételek megadása, és a lista frissítése
    /// @param _par A szűrő paramétere
    /// @param __o A lista rendezésének a típusa
    /// @param __f A szűrés típusa
    virtual bool setFilter(const QVariant &_par = QVariant(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
    /// Az opcionális név konvertáló függvény megadása.
    /// Az SQL függvény a rekord ID-ből egy nevet ad vissza.
    void setToNameF(const QString& _fn) { toNameFName = _fn; }
    /// Az opcionális név konvertáló kifejezés.
    /// Az SQL függvény a rekord ID-ből egy nevet ad vissza.
    void setViewExpr(const QString& _sve) { viewExpr = _sve; }
    /// A megadott nevű elem indexe a listában, vagy -1 ha nincs ilyen elem
    int indexOf(const QString __s)  { return nameList.indexOf(__s); }
    /// A megadott ID-jű elem indexe a listában, vagy -1 ha nincs ilyen elem
    int indexOf(qlonglong __id)     { return idList.indexOf(__id); }
    QString at(int __i) const       { return __i < 0 || __i >= nameList.size() ? QString() : nameList[__i]; }
    /// A megadott sorszámú elem ID-je, vagy NULL_ID, ha nincs ilyen sorszámú elem.
    qlonglong atId(int __i) const   { return __i < 0 || __i >= idList.size() ? NULL_ID : idList[__i]; }
    /// A megadott sorszámú elem neve, vagy üres string, ha nincs ilyen sorszámú elem.
    qlonglong idOf(const QString& __s);
    /// Az ID alapján adja vissza a nevet a listából
    QString nameOf(qlonglong __id);
    QString atView(int __i) const   { return __i < 0 || __i >= viewList.size() ? QString() : viewList[__i]; }
    ///
    cRecordListModel& copy(const cRecordListModel& _o);
    /// Ha értéke true, akkor a lista első eleme NULL (a név üres, az ID pedig NULL_ID)
    /// A konstruktorok false-re inicializálják.
    bool                nullable;
    /// Ha ID-re szűrés is van megadva, és automatikusan nem megállapítható, hogy melyik mezőről van szó
    /// (nincs owner, vagy parant), akkor az id mező neve. NULL-ra van inicializálva.
    QString             sFkeyName;
    /// Egy opcionális függvény név, az ID-re szűréshez, amit az egyenlőség operátor helyett lehet használni.
    QString             sOwnCheck;
    /// Ha Ha ID-re szűrés van megadva, és az ID értéke NULL, akkor ha értéke igaz, akkor az az
    /// összes rekordot jelenti szűrés nélkül. Ha értéke hamis, akkor nincs egyetlen rekord sem, ami illeszkedik.
    /// false-ra van inicializálva.
    bool                nullIdIsAll;
    /// Leszármazottak kizárva
    bool                only;
    eDataCharacter      dcData;
    const cRecStaticDescr * pDescr;  ///< A rekord leíró
    /// A model és egy QComboBox widget összekapcsolása. Lehetővé teszi, hogy a widget dekorációt a kiválasztott elem alapján
    /// állítsa be a modell.
    void joinWith(QComboBox *_pComboBox);
    /// A kurrens ID lekérdezése. Csa akkor hívható, ha a modelt a joinWith metódussal összerendeltük egy comboBox-al.
    qlonglong currendId();
    QString currendName();
protected:
    QString _where(QString s = QString());
    QString where(const QString &nameName);
    QString _order(const QString& nameName);
    QString select();
    void setPattern(const QVariant& _par);
    enum eOrderType     order;      ///< Az aktuális rendezés típusa
    int                 ordIndex;   ///< A rendezés erre az indexű mezőre történik (negatív, ha az alapértelmezett név a rendezés aléapja)
    enum eFilterType    filter;     ///< Az aktuális szűrés típusa
    QString             pattern;    ///< Minta, szűrés paramétere
    QString             cnstFlt;    ///< A konstans szűrő (SQL kifejezés)
    QString             ownerFlt;   ///< A szűrő az owner-re, vagy üres (SQL kifejezés)
    QStringList         nameList;   ///< Az aktuális név lista (sorrend azonos mint az idList-ben
    QStringList         viewList;   ///< A megjelenített stringek lista (alapértelmezetten azonos a nameList-el;
    QList<qlonglong>    idList;     ///< Az aktuális ID lista (sorrend azonos mint az stringList-ben
    QSqlQuery *         pq;
    QString             toNameFName;///< Név konvertáló SQL függvény neve, ha a rekordban nincs név mező
    QString             viewExpr;   ///< Opc. kifejezés, a megjelenített stringre (ha nincs, akkor a név jelenik meg)
public slots:
    /// Hívja a setFilter() metódust, de csak a szűrés paramétere adható meg.
    void setPatternSlot(const QVariant& __pat);
private:
    QComboBox  *pComboBox;
    QPalette    palette;
    QPalette    nullPalette;
    QFont       font;
    QFont       nullFont;
private slots:
    void changeCurrentIndex(int i);
};


/// Model: cRecordListModel leszármazottja. A zónákat  (a place_groups táblát) kérdezi le.
/// A konstans szűrési feltétel a típus, ami 'zone' lesz a konstruktorban beállítva.
/// Ha az eredmény string tartalmazza az 'all' zónát akkor az mindíg az első elem.
class LV2GSHARED_EXPORT cZoneListModel : public cRecordListModel {
public:
    /// Konstruktor.
    /// A szűrés alapértelmezése az összes zóna.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista modosítása a setPattern() metódussal lehetséges.
    /// @param _par Parent
    cZoneListModel(QObject * __par = nullptr);
    /// Szűrési feltételek megadása, és a lista frissítése
    /// @param _par A szűrő paramétere, a zóna ID-je
    /// @param __o A lista rendezésének a típusa
    /// @param __f Nem használt paraméter.
    virtual bool setFilter(const QVariant &_par = QVariant(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
};

/// Model: cRecordListModel leszármazottja. A places táblát kérdezi le, a konstans szűrési feltétel mindíg a zóna (place_group_id).
/// Egy helyiség akkor tartozik a zónába, ha tagja az a zónát reprezentáló place_groups csoportnak, vagy valamelyik
/// parentje tag. A nullable adattag alapértelmezése true. A listából mindíg kimarad a "root".
/// Az "unknown" hely akkor marad ki, ha a skipUnknown adattag értéke true. A konstruktor true-ra inicialozálja a
/// skipUnknown adattagot.
class LV2GSHARED_EXPORT cPlacesInZoneModel : public cRecordListModel {
public:
    /// Konstruktor.
    /// A szűrés típus nincs értelmezve, a pattern (zóna ill. place_group_id) az "ALL" ill ALL_PLACE_GROUP_ID lessz,
    /// a megjelenített lista az összes hely listája lessz.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista modosítása a setPattern() metódussal lehetséges.
    /// @param _par Parent
    cPlacesInZoneModel(QObject * __par = nullptr);
    /// Szűrési feltételek megadása, és/vagy a lista frissítése
    /// @param _par A szűrő paramétere, a zóna ID-je
    /// @param __o A lista rendezésének a típusa
    /// @param __f Nem használt paraméter.
    virtual bool setFilter(const QVariant &_par = QVariant(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
    /// Beállítja a zónát a konstans szűrési feltételként. Ha _par értéke NULL, akkor az "ALL" zóna lesz beállítva,
    /// ami azt jelenti, hogy nincs szűrés zónára.
    /// A metódus kiad egy setFilter() hívást paraméterek nélkül, és frissíti a listát.
    /// @param _par A zóna ID, vagy név. Ha a paraméter nem azonosítható zóna név vagy ID-ként, akkor kizárást dob a hívás.
    void setZone(qlonglong id);
    void setZone(const QString& name);
    ///
    bool    skipUnknown;
protected:
    static const QString sqlSelect;
    static const QString sqlSelectAll;
    qlonglong            zoneId;
};

enum eNullType {
    NT_NOT_NULL, NT_NULL = DC_NULL, NT_AUTO = DC_AUTO, NT_DEFAULT = DC_DEFAULT
};

class LV2GSHARED_EXPORT cEnumListModel : public QAbstractListModel {
    Q_OBJECT
public:
    cEnumListModel(QObject * __par = nullptr) : QAbstractListModel(__par) { pType = nullptr; pq = nullptr; pComboBox = nullptr, nullType = NT_NOT_NULL; }
    cEnumListModel(const QString& __t, eNullType _nullable = NT_NOT_NULL, const tIntVector &_eList = tIntVector(), QObject * __par = nullptr);
    cEnumListModel(const cColEnumType *_pType, eNullType _nullable = NT_NOT_NULL, const tIntVector &_eList = tIntVector(), QObject * __par = nullptr);
    ~cEnumListModel();
    int setEnum(const QString& __t, eNullType _nullable = NT_NOT_NULL, const tIntVector& _eList = tIntVector(), eEx __ex = EX_ERROR);
    int setEnum(const cColEnumType *_pType, eNullType _nullable = NT_NOT_NULL, const tIntVector& _eList = tIntVector(), eEx __ex = EX_ERROR);
    void joinWith(QComboBox *_pComboBox);
    void clear();
    virtual int rowCount(const QModelIndex & = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    const cEnumVal * atEnumVal(int ix, eEx __ex = EX_ERROR) const;
    int atInt(int ix) const;
    QString at(int ix) const;
    int indexOf(int e) const;
    int indexOf(const QString& s) const;
protected:
    QSqlQuery *         pq;
    eNullType           nullType;
    const cColEnumType *pType;
    QVector<const cEnumVal *> enumVals;
    QComboBox  *        pComboBox;
    QPalette palette;
private slots:
    void currentIndex(int i);
signals:
    void currentEnumChanged(int e);
};

#endif // LV2MODELS_H
