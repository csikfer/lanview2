#ifndef RECORD_LINK
#define RECORD_LINK

#include "record_table.h"
#include "record_link_model.h"

/// @class cRecordLink
/// Egy adatbázis tábla megjelenítését végző objektum.
class LV2GSHARED_EXPORT cRecordLink : public cRecordsViewBase {
    Q_OBJECT
public:
    /// Konstruktor
    /// @param _mn A tábla megjelenítését leíró rekord neve (table_shapes.table_shape_name)
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordLink(const QString& _mn, bool _isDialog, QWidget * par = NULL);
    /// Konstruktor már beolvasott leíró
    /// @param _mn A tábla megjelenítését leíró objektum pointere
    /// @param _isDialog Ha igaz, akkor a megjelenítés egy dialog ablakban.
    /// @param par A szülő widget pointere, vagy NULL
    cRecordLink(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = NULL, QWidget * par = NULL);
    /// destruktor
    ~cRecordLink();
    cRecordLinkModel *pTableModel() const { return static_cast<cRecordLinkModel *>(pModel); }
    const cRecordAny *recordAt(int i) const {
        if (!isContIx(pTableModel()->records(), i)) EXCEPTION(EDATA, i);
        return pTableModel()->records()[i];
    }
    virtual QModelIndexList selectedRows();
    virtual QModelIndex actIndex();
    virtual cRecordAny *actRecord(const QModelIndex& _mi = QModelIndex());
    virtual cRecordAny *nextRow(QModelIndex *pMi, int _upRes = 1);
    virtual cRecordAny *prevRow(QModelIndex *pMi, int _upRes = 1);
    virtual void selectRow(const QModelIndex& mi);
    void empty();

    void setEditButtons();
    /// A táblázatot megjelenítő widget
    QTableView     *pTableView;
    void init();
    virtual void initSimple(QWidget *pW);
    virtual QStringList filterWhere(QVariantList& qParams);
    void _refresh(bool all = true);
    // void refresh_lst_rev();
    cRecordAny * record(int i);
    cRecordAny * record(QModelIndex mi);
public:
    QTableView *tableView() { return pTableView; }
};


#endif // RECORD_LINK
