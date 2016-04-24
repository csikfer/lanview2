#ifndef RECORD_LINK_MODEL
#define RECORD_LINK_MODEL

#include "record_table_model.h"

class cRecordLink;

/// @class cRecordLinkModel
/// Model osztály adatatbázis tábla táblázatos megjelenítéséhez
class cRecordLinkModel : public QAbstractTableModel, public cRecordViewModelBase {
    Q_OBJECT
public:
    cRecordLinkModel(cRecordLink &_rt);
    ~cRecordLinkModel();
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    QBitArray index2map(const QModelIndexList &mil);
    /// Törli az index listában megjelült rekordokat.
    /// Elötte egy dialógusban megerősítést kér, ha ebből Cancel-el lépünk ki, akkor nincs törlés.
    /// A sorokat/rekordokat a removeRec() metódus hívásával törli.
    virtual void removeRecords(const QModelIndexList& mil);
    /// Eltávolítja az adatbázisból a megadott sorban megjelenített rekordot
    /// Majd ha ez sikerült, akkor törli a megjelenített sort is a widgeten is a removeRow() hívásával
    virtual bool removeRec(const QModelIndex & mi);
    /// Törli a megadott indexű recordot a record set-ből
    virtual bool removeRow(const QModelIndex & mi);

    /// Modosítja a widget megfelelő sorát a megadott rekord tartalommal.
    virtual bool updateRow(const QModelIndex &mi, cRecordAny *pRec);

    /// Beszúr egy elemet a megadott indexű pont elé
    virtual bool insertRow(cRecordAny *pRec);

    /// A megjelenítendő record set kiürítése, az üres táblázat újra rajzolása
    virtual cRecordLinkModel& clear();
    /// A megjelenítendő record set megadása, a táblázat újra rajzolása
    /// @return Ha a megadott rekord set üres, akkor false, egyébként true
    bool setRecords(const tRecords& __recs, int _firstNm = 0);
    /// Az aktuális megjelenített konténer referenciáját adja vissza
    const tRecords& records() const             { return _records; }

    int size() const                            { return _records.size(); }
    int isEmpty() const                         { return _records.isEmpty(); }
    tRecords                    _records;
public:
    /// A megjelenítendő record set megadása, a táblázat újra rajzolása
    /// @param _q A ,egjelenítendő lekérdezés eredménye, másolat készül róla a q adattagban.
    /// @return A record set méretével tér vissza
    int setRecords(QSqlQuery& _q, bool);
    int qView();
    /// Az aktuális lekérdezés teljes méretét adja vissza (az esetleges benfoglalt objektummal együtt).
    int qSize()         { return q.size(); }
protected:
    /// Shadow copy az aktuális lekérdezés eredményéről
    QSqlQuery   q;
};

#endif // RECORD_LINK_MODEL

