#ifndef DEDUCEPATCH_H
#define DEDUCEPATCH_H
#include "lv2g.h"
#include "lv2models.h"

class cSelectNode : public QObject {
    Q_OBJECT
public:
    cSelectNode(QComboBox *_pZone, QComboBox *_pPLace, QComboBox *_pNode, QLineEdit *_pFilt = NULL, const QString& _cFilt = QString());
private:
    QComboBox *pComboBoxZone;
    QComboBox *pComboBoxPLace;
    QComboBox *pComboBoxNode;
    QLineEdit *pLineEditFilt;
    const QString    constFilter;
    cZoneListModel     *pZoneModel;
    cPlacesInZoneModel *pPlaceModel;
    cRecordListModel   *pNodeModel;
    bool                blockSignal;
private slots:
    void zoneChanged(int ix);
    void placeChanged(int ix);
    void patternChanged(const QString& s);
    void _nodeChanged(const QString& s);
signals:
    void nodeChanged(const QString& name);
    void nodeChanged(qlonglong id);
};

/* *** */

#if defined(LV2G_LIBRARY)
#include "ui_deducepatch.h"
class cDeducePatch;
class LV2GSHARED_EXPORT cDPRow : public QObject {
    Q_OBJECT
public:
    cDPRow(QSqlQuery& q, cDeducePatch *par, int _row);
    QTableWidget const * pTable;
    const int            row;
};


#else
namespace Ui {
    class class deducePatch;
}
class cDPRow;
#endif

class cDeducePatch : public cIntSubObj {
    friend class cDPRow;
    Q_OBJECT
public:
    cDeducePatch(QMdiArea *par);
    ~cDeducePatch();
    static const enum ePrivilegeLevel rights;
protected:
    Ui::deducePatch *pUi;
    cSelectNode *pSelNodeLeft;
    cSelectNode *pSelNodeRight;
    QList<cDPRow *> rows;
    qlonglong   nidLeft, nidRight;
    cPatch      patchLeft, patchRight;
    QList<QMap<int, qlonglong> > endsIdLeft, endsIdRight;   ///< [<patch port index>][<shared>] = <term port ID>
    void clearTable();
    void changedNode();
protected slots:
    void changeNodeLeft(qlonglong id);
    void changeNodeRight(qlonglong id);
    void start();
    void save();
};

#endif // DEDUCEPATCH_H
