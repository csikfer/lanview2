#ifndef LV2MODELS_H
#define LV2MODELS_H

#include "lv2g.h"
#include <QAbstractTableModel>
#include <QStringListModel>

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
        beginResetModel();
        _stringList << s;
        endResetModel();
        return *this;
    }
    /// Beszúr egy elemet a megadott indexű sor elé
    cStringListModel& insert(const QString& s, int i = -1) {
        if (i < 0) i = _stringList.size();
        beginResetModel();
        _stringList.insert(i, s);
        endResetModel();
        return *this;
    }
    /// Törli a megadott indexű sort
    cStringListModel& remove(int i) {
        beginResetModel();
        _stringList.removeAt(i);
        endResetModel();
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
    cPolygonTableModel& remove(QModelIndexList& mil);
    bool rowNumbers() const                     { return _rowNumbers; }
    void setRowNumers(bool b)                   { _rowNumbers = b; /* ??? */ }
    bool viewHeader() const                     { return _viewHeader; }
    void setViewHeader(bool b)                   { _viewHeader = b; /* ??? */ }
    int size() const                            { return _polygon.size(); }
    int isEmpty() const                         { return _polygon.isEmpty(); }
private:
    /// A megjelenített polygon
    tPolygonF   _polygon;
    bool        _rowNumbers;
    bool        _viewHeader;
};


/// Model: QStringListModel leszármazottja. Egy megadott rekord típus neveinek a listája.
class LV2GSHARED_EXPORT cRecordListModel : public QStringListModel {
    Q_OBJECT
public:
    /// Konstruktor.
    /// Eltárolja a statikus descriptor objektumot (referencia).
    /// A szűrés típusát FT_BEGIN -re állítja, a pattern üres lessz, a megjelenített lista is üres lessz.
    /// Az objektum egy saját QSqlQuery objektumot allokál a lekérdezésekhez.
    /// A lista feltöltése a setPattern() metódussal lehetséges.
    /// @par __d A rekord leíró objektum
    /// @par _par Parent
    cRecordListModel(const cRecStaticDescr& __d, QObject * __par = NULL);
    ~cRecordListModel();
    /// Opcionális konstans szűrési feltétel megadása, nincs lista frissítés
    /// Az itt megadott feltétel és kapcsolatban lessz a setFilter() metódusban megadottnak.
    void setConstFilter(const QString& _par, eFilterType __f);
    /// Szűrési feltételek megadása, és a lista frissítése
    bool setFilter(const QString& _par = QString(), enum eOrderType __o = OT_DEFAULT, enum eFilterType __f = FT_DEFAULT);
    /// Az opcionális név konvertáló függvény megadása.
    /// Az SQL függvény a rekord ID-ből egy nevet ad vissza.
    void setToNameF(const QString& _fn) { toNameFName = _fn; }
    int indexOf(const QString __s)  { return stringList.indexOf(__s); }
    int indexOf(qlonglong __id)     { return idList.indexOf(__id); }
    QString at(int __i)             { return __i < 0 || __i >= stringList.size() ? QString() : stringList[__i]; }
    qlonglong atId(int __i)         { return __i < 0 || __i >= idList.size() ? NULL_ID : idList[__i]; }
    qlonglong idOf(const QString& __s);
    QString nameOf(qlonglong __id);
    bool                nullable;
private:
    QString where(QString s = QString());
    enum eOrderType     order;
    enum eFilterType    filter;
    QString             pattern;
    QString             cnstFlt;
    QStringList         stringList;
    QList<qlonglong>    idList;
    const cRecStaticDescr&  descr;
    QSqlQuery *         pq;
    bool                firstTime;
    QString             toNameFName;
public slots:
    void setPatternSlot(const QString& __pat);
};


#endif // LV2MODELS_H
