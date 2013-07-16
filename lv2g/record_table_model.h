#ifndef RECORD_TABLE_MODEL_H
#define RECORD_TABLE_MODEL_H

#include "lv2g.h"

typedef tRecordList<cAlternate>         tRecords;
class   cRecordTableColumn;
typedef tRecordList<cRecordTableColumn>  tRecordTableColumns;
class   cRecordTable;

class cRecordTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    cRecordTableModel(const cRecordTable &_rt);
    ~cRecordTableModel();
    virtual int rowCount(const QModelIndex &parent) const;
    virtual int columnCount(const QModelIndex &parent) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    /// Az aktuális record set  a végéhez ad egy rekordot
    virtual cRecordTableModel& operator<<(const cAlternate& _r);
    /// Beszúr egy elemet a megadott indexű pont elé
    virtual cRecordTableModel& insert(const cAlternate& _r, int i = -1);
    /// Törli a megadott indexű recordot a record set-ből
    virtual cRecordTableModel& remove(int i);
    /// A megjelenítendő record set kiürítése, a táblázat újra rajzolása
    virtual cRecordTableModel& clear();
    /// Törli az utolsó rekordot, ha üres a record set, akkor nem csinál semmit.
    cRecordTableModel& pop_back();
    /// Törli az első rekordot, ha üres a record set, akkor nem csinál semmit.
    cRecordTableModel& pop_front();
    /// Törli a kijelölt rekordokat
    cRecordTableModel& remove(QModelIndexList& mil);
    /// Az utolsó elemmel kezdve sorban töröl minden elemet
    cRecordTableModel& removeAll();
    /// A megjelenítendő record set megadása, a táblázat újra rajzolása
    /// @return Ha a megadott rekord set üres, akkor false, egyébként true
    bool setRecords(const tRecords& __recs, int _firstNm = 0);
    /// Az aktuális megjelenített polygon referenciáját adja vissza
    const tRecords& records() const             { return _records; }
    /// A megadott sorhoz tartozó rekord objektum pointert lecseréli. A megjelenített táblát frissíti.
    /// A régi pointer felszabadítja, a paraméterként megadottat, pedig szintén ő szabadítja fel, ha már nem kell.
    /// @return ha elvégezte a műveletet true, ha olyan sort adunk meg, ami nem létezik, akkor false. ekkor a paraméterként kapott pointert nem szabadítja fel.
    bool update(int row, cAlternate *pRec);
    bool rowNumbers() const                     { return _viewRowNumbers; }
    void setRowNumers(bool b)                   { _viewRowNumbers = b; }
    bool viewHeader() const                     { return _viewHeader; }
    void setViewHeader(bool b)                  { _viewHeader = b;  }
    int  maxRows() const                        { return _maxRows; }
    void setMaxRows(int _max)                   { _maxRows = _max;  }
    int  firstRowNumber() const                 { return _firstRowNumber; }
    void setFirstRowNumber(int _fn)             { _firstRowNumber = _fn;  }
    int size() const                            { return _records.size(); }
    int isEmpty() const                         { return _records.isEmpty(); }
    bool isExtRow(int row)                      { return extLines.contains(row); }
    const cRecordTable&         recordTable;
    const cRecStaticDescr&      recDescr;
    const cTableShape&          descriptor;
    const tRecordTableColumns&   columns;
    /// Kiemelt sorok
    tIntVector                  extLines;
protected:
    virtual void _removed(cAlternate *p);
    /// A record set
    tRecords                    _records;
    /// Kereszt indexek
    tIntVector                   _col2shape, _col2field;
    /// A rekord sorszámok kijelzése (default tue)
    bool                        _viewRowNumbers;
    /// A header megjelenítése (default tue)
    bool                        _viewHeader;
    /// Az első megfjeklenített sor sorszáma (1-től indúl, default 1)
    int                         _firstRowNumber;
    /// A maximálisan megjelenítendő sorok száma (default 100)
    int                         _maxRows;
    /// Egy áltatlánosan használlt lekérdező objektum.
    QSqlQuery                 * pq;
signals:
    /// Egy rekord törlése a rekord set-ből.
    /// A signal kiadása után a pointer törlődik, aszinkron kapcsolat nem lehet !!!
    void removed(cAlternate *_p);
};


class cRecordTableModelSql : public cRecordTableModel {
public:
    cRecordTableModelSql(const cRecordTable &_rt);
    ~cRecordTableModelSql();
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
    /// A megjelenítendő record set kiürítése, a táblázat újra rajzolása
    cRecordTableModel& clear();
protected:
    /// Shadow copy az aktuális lekérdezés eredményéről
    QSqlQuery   q;
};


#endif // RECORD_TABLE_MODEL_H
