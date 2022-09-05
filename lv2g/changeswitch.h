#ifndef CHANGESWITCH_H
#define CHANGESWITCH_H
#include "lv2g.h"
#include "lv2link.h"
#include "srvdata.h"
#include "lv2widgets.h"
#include <QStandardItemModel>

class cCheckBoxListLayout : public QBoxLayout
{
    Q_OBJECT
public:
    cCheckBoxListLayout(QBoxLayout::Direction dir = QBoxLayout::TopToBottom, QWidget *par = nullptr);
    void clear();
    QCheckBox * addCheckBox(QCheckBox * pcb);
    QCheckBox * addCheckBox(const QString& text, Qt::CheckState state = Qt::Unchecked, bool tristate = false);
    void addCheckBoxs(const QStringList& items, Qt::CheckState state = Qt::Unchecked, bool tristate = false);
    int size() { int r = count(); if (_endStretch) --r; return r; }
    bool setEndStrech();
    bool dropEndStrech();
    QLayout * layout() { return static_cast<QLayout *>(this); }
    QCheckBox * at(int ix);
    Qt::CheckState checkState(int ix) { return at(ix)->checkState(); }
    void setCheckState(int ix, Qt::CheckState state) { at(ix)->setCheckState(state); }
    void setAllCheckState(Qt::CheckState state);
protected:
    bool    _endStretch;
};

#if defined(LV2G_LIBRARY)
#include "ui_changeswitch.h"

class cChangeSwitch;
class LV2GSHARED_EXPORT cCSRow : public QObject {
    Q_OBJECT
public:
    /// Column indexs
    enum eColumnIx {
        CIX_INDEX_SRC,  ///< Source port index
        CIX_PORT_SRC,   ///< Source port name
        CIX_CPY_PAR,    ///< Source port parameter, copy check box
        CIX_CPY_LNK,    ///< Source port linked port, move check box
        CIX_CPY_SRV,    ///< Source port service, move check box
        CIX_INDEX_TRG,  ///< Target port index
        CIX_PORT_TRG,   ///< Target port name
        CIX_DEL_PAR,    ///< Target port param, delete check box
        CIX_DEL_LNK,    ///< Target port link, delete check box
        CIX_DEL_SRV,    ///< Target port service, delete check box
        COLUMN_NUMBER
    };
    cCSRow(QSqlQuery& q, cChangeSwitch *par, int _r, cNPort *_ps, cNPort *_pt);
    ~cCSRow();
    void setChecked(int col, bool f);
    void save(QSqlQuery& q);
    cChangeSwitch * pParent;
    QTableWidget *  pTableWidget;
    int             row;

    cNPort *            pSrc;       ///< source port object pointer
    cPhsLink *          pLnkCpy;    ///< Másolandó fizikai link
    cCheckBoxListLayout *checkBoxCpyPar;
    cCheckBoxListLayout *checkBoxCpyLnk;
    QList<qlonglong>    hsIdCpyList;
    cCheckBoxListLayout *checkBoxCpySrv;

    cNPort *            pTrg;       ///< Target port object pointer
    cPhsLink *          pLnkDel;    ///< Cél eszköz törlendő fizikai link
    cCheckBoxListLayout *checkBoxDelPar;
    cCheckBoxListLayout *checkBoxDelLnk;
    QList<qlonglong>    hsIdDelList;
    cCheckBoxListLayout *checkBoxDelSrv;
protected:
    void setItem(const QString& txt, int col);
    void setItem(qlonglong n, int col);
    // QCheckBox * checkBox(int col, const QString& s, bool checked, bool ro);
    cCheckBoxListLayout * checkBoxsLayout(int cix) {
        switch (cix) {
        case CIX_CPY_PAR:   return checkBoxCpyPar;
        case CIX_CPY_SRV:   return checkBoxCpySrv;
        case CIX_DEL_PAR:   return checkBoxDelPar;
        case CIX_DEL_SRV:   return checkBoxDelSrv;
        }
        EXCEPTION(EPROGFAIL);
    }
protected slots:
    void on_checkBoxCpyLnk_togled(bool f);
    void on_checkBoxDelLnk_togled(bool f);
    void setCheckBoxColor(int cix, int ix, int col);
};

class cCSSaver : public QThread {
    Q_OBJECT
public:
    cCSSaver(cChangeSwitch *par);
    ~cCSSaver();
    void run();
    cChangeSwitch *pParent;
    cError *pe;
};

#else
namespace Ui {
    class changeSwitch;
}
class cCSRow;
#endif

class cChangeSwitch : public cIntSubObj {
#if defined(LV2G_LIBRARY)
    friend class cCSRow;
    friend class cCSSaver;
#endif
    Q_OBJECT
public:
    cChangeSwitch(QMdiArea *par);
    ~cChangeSwitch();
    static const enum ePrivilegeLevel rights;
protected:
    enum eState { ES_INSUFF, ES_SRC_READY, ES_DATA_READY, ES_RUN_SAVER, ES_SAVED} state;
    enum eList  { EL_CPY_SRV, EL_CPY_PAR, EL_DEL_SRV, EL_DEL_PAR, LIST_NUMBER };
    void clear();
    void setButtons(bool chk = true);
    void refreshTables();
    static bool filteringPorts(QList<cNPort *> &ports);
    bool setHeaderIcon(int col);
    bool setListIcon(enum eList e);
    void setChecked(enum eList e);
    void save(QSqlQuery& q);
    Ui::changeSwitch *  pUi;
    cCheckBoxListLayout*checkBoxCpySrv;
    cCheckBoxListLayout*checkBoxDelSrv;
    cCheckBoxListLayout*checkBoxCpyPar;
    cCheckBoxListLayout*checkBoxDelPar;
    QSqlQuery *         pq;
    cSelectNode *       pSelNodeSrc;
    cSelectNode *       pSelNodeTrg;
    int                 offsSrc;
    int                 offsTrg;
    qlonglong           nidSrc;
    qlonglong           nidTrg;
    QList<cCSRow *>     rows;
    cNode *             pNodeSrc;
    cNode *             pNodeTrg;
    QList<qlonglong>    srvCpyIdList;
    QList<qlonglong>    srvDelIdList;
    int                 minIxSrc;
    int                 minIxTrg;
    int                 maxIxSrc;
    int                 maxIxTrg;
    static const QString sIxNote;
    static const QString iconUnchecked;
    static const QString iconChecked;
    QBitArray           headerIconState;
    QBitArray           listIconState;
    cCSSaver *          pSaver;
private slots:
    void on_selNodeSrc_nodeIdChanged(qlonglong nid);
    void on_selNodeTrg_nodeIdChanged(qlonglong nid);
    void on_tableWidgetHHeader_clicked(int sect);

    void on_spinBoxOffsetSrc_valueChanged(int arg1);
    void on_spinBoxOffsetTrg_valueChanged(int arg1);
    void on_pushButtonRefresh_clicked();
    void on_pushButtonCopy_clicked();
    void on_toolButtonCpySrv_clicked();
    void on_toolButtonDelSrv_clicked();
    void on_toolButtonCpyPar_clicked();
    void on_toolButtonDelPar_clicked();
    void on_saver_finished();
    void setTableCellColor(int row, int col, int ix, int ec);
    void setListCellColor(int lt, int row, int ec);
signals:
    void doSetTableCellColor(int row, int col, int ix, int ec);
    void doSetListCellColor(int lt, int row, int ec);
};


#endif // CHANGESWITCH_H
