#ifndef RECORD_TABLE_MODEL_H
#define RECORD_TABLE_MODEL_H

#include "lv2g.h"

typedef tRecordList<cRecord>         tRecords;
class   cRecordTableColumn;
typedef tRecordList<cRecordTableColumn>  tRecordTableColumns;
class cRecordsViewBase;
class   cRecordTable;

/// @class cRecordViewModelBase
/// Rekord lista megjelenítés modell bázis objektuma
/// Ebbül a bázis objektumból. és a megfelelő QModel objektumból származtatható a megfelelő model osztály.
class cRecordViewModelBase {
public:

    cRecordViewModelBase(cRecordsViewBase &_rt);
    virtual ~cRecordViewModelBase();

    QVariant _data(int fix, cRecordTableColumn& column, const cRecord *pr, int role, bool bTxt) const;

    cRecord *qGetRecord(QSqlQuery& q);

    /// Kereszt index: Tábla oszlop - adat mező index
    tIntVector                   _col2field;
    /// A rekord sorszámok kijelzése (default tue)
    bool                        _viewRowNumbers;
    /// A header megjelenítése (default tue)
    bool                        _viewHeader;
    /// Az első megfjeklenített sor sorszáma (0-tól indúl, default 0)
    int                         _firstRowNumber;
    /// A maximálisan megjelenítendő sorok száma (default 100)
    int                         _maxRows;
    /// Egy áltatlánosan használlt lekérdező objektum.
    QSqlQuery                 * pq;
    /// A megjelenító view objektum
    cRecordsViewBase&      recordView;
    /// A megjelenített (fő) tábla/rekord leíró
    const cRecStaticDescr&      recDescr;
    /// A megjelenítést vezérlő (fő) tábla leíró rekord
    const cTableShape&          tableShape;
    /// Megjelenített oszlokok vezérlő objektumai
    const tRecordTableColumns&  columns;
    /// A teljes sor háttérszíne egy enumeráció típusú mező alapján. Enum típus név:
    QString lineBgColorEnumType;
    /// A teljes sor háttérszíne egy enumeráció típusú mező alapján. Enum típus név:
    int     lineBgColorEnumIx;
    bool rowNumbers() const                     { return _viewRowNumbers; }
    void setRowNumers(bool b)                   { _viewRowNumbers = b; }
    bool viewHeader() const                     { return _viewHeader; }
    void setViewHeader(bool b)                  { _viewHeader = b;  }
    int  maxRows() const                        { return _maxRows; }
    void setMaxRows(int _max)                   { _maxRows = _max;  }
    int  firstRowNumber() const                 { return _firstRowNumber; }
    void setFirstRowNumber(int _fn)             { _firstRowNumber = _fn;  }

    /// A model hederData() virtuális metódus alap algoritmusa
    QVariant _headerData(int section, Qt::Orientation orientation, int role) const;

    virtual void removeRecords(const QModelIndexList& mil) = 0;
    virtual bool removeRec(const QModelIndex & mi) = 0;
    virtual bool removeRow(const QModelIndex & mi) = 0;
    virtual cRecord *record(const QModelIndex &mi) = 0;
    virtual void clear() = 0;

    /// A megadott sorhoz tartozó rekord objektum pointert lecseréli. A megjelenített táblát frissíti.
    /// A megadott új adattartalommal frissíti az adatbázis tábla megfelelő rekordját.
    /// A régi pointer felszabadítja, a paraméterként megadottat, pedig szintén ő szabadítja fel, ha már nem kell.
    /// @return ha elvégezte a műveletet true, ha olyan sort adunk meg, ami nem létezik, akkor false.
    ///  ekkor a paraméterként kapott pointert nem szabadítja fel.
    virtual int updateRec(const QModelIndex &mi, cRecord *pRec);
    virtual bool updateRow(const QModelIndex &mi, cRecord *pRec) = 0;

    virtual bool insertRec(cRecord *pRec);
    virtual bool insertRow(cRecord *pRec) = 0;
    static bool SqlInsert(QSqlQuery &q, cRecord *pRec);
    static QString sIrrevocable;
};

/// @class cRecordTableModel
/// Model osztály adatatbázis tábla táblázatos megjelenítéséhez
class cRecordTableModel : public QAbstractTableModel, public cRecordViewModelBase {
    Q_OBJECT
public:
    cRecordTableModel(cRecordTable &_rt);
    ~cRecordTableModel();
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual cRecord *record(const QModelIndex &index);

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
    virtual bool updateRow(const QModelIndex &mi, cRecord *pRec);

    /// Beszúr egy elemet a megadott indexű pont elé
    virtual bool insertRow(cRecord *pRec);

    /// A megjelenítendő record set kiürítése, az üres táblázat újra rajzolása
    virtual void clear();
    /// A megjelenítendő record set megadása, a táblázat újra rajzolása
    /// @return Ha a megadott rekord set üres, akkor false, egyébként true
    bool setRecords(const tRecords& __recs, int _firstNm = 0);
    /// Az aktuális megjelenített konténer referenciáját adja vissza
    const tRecords& records() const             { return _records; }

    int size() const                            { return _records.size(); }
    int isEmpty() const                         { return _records.isEmpty(); }
    QList<QStringList>  toStringTable();
    QString             toCSV();
    QString             toHtml();
    QList<QStringList>  toStringTable(QModelIndexList mil);
    QString             toCSV(QModelIndexList mil);
    QString             toHtml(QModelIndexList mil);
    tRecords                    _records;
public:
    /// A megjelenítendő record set megadása, a táblázat újra rajzolása
    /// @param _q A ,egjelenítendő lekérdezés eredménye, másolat készül róla a q adattagban.
    /// @return A record set méretével tér vissza
    int setRecords(QSqlQuery& _q, bool _first = true);
    int qView();
    int qFirst();
    int qNext();
    int qPrev();
    int qLast();
    /// Az aktuális lekérdezés teljes méretét adja vissza (az esetleges benfoglalt objektummal együtt).
    int qSize()         { return q.size(); }
    bool qIsPaged()     { return q.isActive() && _maxRows < qSize(); }
    bool qNextResult()  { return (_firstRowNumber + _maxRows) <= qSize(); }
    bool qPrevResult()  { return _firstRowNumber > 0; }
protected:
    QSqlQuery   q;
signals:
    void dataReloded(const tRecords& _recs);
};



#endif // RECORD_TABLE_MODEL_H
