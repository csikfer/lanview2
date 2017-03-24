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
    cStringListModel(QObject *__par = NULL);
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

/// Model: polygon pontjainak megjelenítése egy QTableView objektummal
class LV2GSHARED_EXPORT cPolygonTableModel : public QAbstractTableModel {
public:
    cPolygonTableModel(QObject *__par = NULL);
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
    cRecordListModel(const cRecStaticDescr& __d, QObject * __par = NULL);
    /// Konstruktor.
    /// Eltárolja a statikus descriptor objektumot (referencia).
    /// A szűrés típusát FT_BEGIN -re állítja, a pattern üres lessz, a megjelenített lista is üres lessz.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista feltöltése a setPattern() metódussal lehetséges.
    /// @param __t A rekord tábla név
    /// @param __s Az opcionális schema név
    /// @param _par Parent
    cRecordListModel(const QString& __t, const QString& __s = QString(), QObject * __par = NULL);
    ~cRecordListModel();
    /// Opcionális konstans szűrési feltétel megadása, nincs lista frissítés
    /// Az itt megadott feltétel és kapcsolatban lessz a setFilter() metódusban megadottnak.
    /// @param _par A szűrő paramétere
    /// @param __f A szűrés típusa
    void setConstFilter(const QString& _par, eFilterType __f);
    /// Szűrési feltételek megadása, és a lista frissítése
    /// @param _par A szűrő paramétere
    /// @param __o A lista rendezésének a típusa
    /// @param __f A szűrés típusa
    virtual bool setFilter(const QVariant &_par = QVariant(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
    /// Az opcionális név konvertáló függvény megadása.
    /// Az SQL függvény a rekord ID-ből egy nevet ad vissza.
    void setToNameF(const QString& _fn) { toNameFName = _fn; }
    /// A megadott nevű elem indexe a listában, vagy -1 ha nincs ilyen elem
    int indexOf(const QString __s)  { return stringList.indexOf(__s); }
    /// A megadott ID-jű elem indexe a listában, vagy -1 ha nincs ilyen elem
    int indexOf(qlonglong __id)     { return idList.indexOf(__id); }
    QString at(int __i)             { return __i < 0 || __i >= stringList.size() ? QString() : stringList[__i]; }
    /// A megadott sorszámú elem ID-je, vagy NULL_ID, ha nincs ilyen sorszámú elem.
    qlonglong atId(int __i)         { return __i < 0 || __i >= idList.size() ? NULL_ID : idList[__i]; }
    /// A megadott sorszámú elem neve, vagy üres string, ha nincs ilyen sorszámú elem.
    qlonglong idOf(const QString& __s);
    /// Az ID alapján adja vissza a nevet a listából
    QString nameOf(qlonglong __id);
    ///
    cRecordListModel& copy(const cRecordListModel& _o);
    /// Ha értéke true, akkor a lista első eleme NULL (a név üres, az ID pedig NULL_ID)
    /// A konstruktorok false-re inicializálják.
    bool                nullable;
    /// Ha ID-re szűrés van megadva, és automatikusan nem megállapítható, hogy melyik mezőről van szó
    /// (nincs owner, vagy parant), akkor az id mező neve. NULL-ra van inicializálva.
    QString             sFkeyName;
    /// Ha Ha ID-re szűrés van megadva, és az ID értéke NULL, akkor ha értéke igaz, akkor az az
    /// összes rekordot jelenti szűrés nélkül. Ha értéke hamis, akkor nincs egyetlen rekord sem, ami illeszkedik.
    /// false-ra van inicializálva.
    bool                nullIdIsAll;
    ///
    bool                only;
    const cRecStaticDescr&  descr;  ///< A rekord leíró
protected:
    QString where(QString s = QString());
    QString select();
    void setPattern(const QVariant& _par);
    enum eOrderType     order;      ///< Az aktuális rendezés típusa
    enum eFilterType    filter;     ///< Az aktuális szűrés típusa
    QString             pattern;    ///< Minta, szűrés paramétere
    qlonglong           fkey_id;    ///< A szűrés paramétere FT_FKEY_ID esetén
    QString             cnstFlt;    ///< A konstans szűrő (SQL kifejezés)
    QStringList         stringList; ///< Az aktuális név lista (sorrend azonos mint az idList-ben
    QList<qlonglong>    idList;     ///< Az aktuális ID lista (sorrend azonos mint az stringList-ben
    QSqlQuery *         pq;
    QString             toNameFName;///< Név konvertáló SQL függvény neve, ha a rekordban nincs név mező
public slots:
    /// Hívja a setFilter() metódust, de csak a szűrés paramétere adható meg.
    void setPatternSlot(const QVariant& __pat);
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
    cZoneListModel(QObject * __par = NULL);
    /// Szűrési feltételek megadása, és a lista frissítése
    /// @param _par A szűrő paramétere, a zóna ID-je
    /// @param __o A lista rendezésének a típusa
    /// @param __f Nem használt paraméter.
    virtual bool setFilter(const QVariant &_par = QVariant(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
};

/// Model: cRecordListModel leszármazottja. A places táblát kérdezi le, a szűrési feltétel mindíg a zóna (place_group_id).
/// Egy helyiség akkor tartozik a zónába, ha tagja az a zónát reprezentáló place_groups csoportnak, vagy valamelyik
/// parentje tag. A nullable adattag alapértelmezése true. A listából mindíg kimarad a "root" és az "unknown" hely.
class LV2GSHARED_EXPORT cPlacesInZoneModel : public cRecordListModel {
public:
    /// Konstruktor.
    /// A szűrés típus nincs értelmezve, a pattern (zóna ill. place_group_id) az "ALL" ill ALL_PLACE_GROUP_ID lessz,
    /// a megjelenített lista az összes hely listája lessz.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista modosítása a setPattern() metódussal lehetséges.
    /// @param _par Parent
    cPlacesInZoneModel(QObject * __par = NULL);
    /// Szűrési feltételek megadása, és a lista frissítése
    /// @param _par A szűrő paramétere, a zóna ID-je
    /// @param __o A lista rendezésének a típusa
    /// @param __f Nem használt paraméter.
    virtual bool setFilter(const QVariant &_par = QVariant(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
protected:
    static const QString sqlSelect;
    static const QString sqlSelectAll;
};

#endif // LV2MODELS_H
