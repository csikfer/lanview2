#ifndef DEDUCEPATCH_H
#define DEDUCEPATCH_H
#include "lv2g.h"
#include "lv2link.h"
#include "srvdata.h"
#include "lv2models.h"
#include <QStandardItemModel>

#if defined(LV2G_LIBRARY)
#include "ui_deducepatch.h"

class cDeducePatch;
/// The object is a row of the table.
class LV2GSHARED_EXPORT cDPRow {
public:
    enum eFieldIxTag {
        CX_INDEX = 0
    };
    enum eColumnId {
        CID_INVALID = NULL_IX,
        CID_HAVE_NO = -1,
        CID_INDEX,
        CID_STATE_WARNING, CID_STATE_CONFLICT, CID_STATE_TAG, CID_STATE_MAC, CID_STATE_LLDP,
        CID_SAVE,
        CID_IF_LEFT, CID_SHARE_LEFT, CID_PPORT_LEFT, CID_TAG_LEFT,
        CID_TAG,
        CID_TAG_RIGHT, CID_PPORT_RIGHT, CID_SHARE_RIGHT, CID_IF_RIGHT,
        CID_NUMBER
    };

    cDPRow(cDeducePatch *par, int _row, int _save_col, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr);
    virtual ~cDPRow();
    void save(QSqlQuery& q);
    bool isChecked() { return pItemSave != NULL && pItemSave->isCheckable() && pItemSave->checkState() == Qt::Checked; }
    virtual QStringList exportHeader() = 0;
    virtual QList<tStringPair> exporter() = 0;
    int row() const { return pItemSave->row(); }
    cPhsLink            phsLink;    ///< Calculated Back-Back link
    static int ixSharedPortId;
    static const QString sIOk;
    static const QString sIWarning;
    static const QString sIError;
    static const QString sIReapeat;
    static const QString sIHelp;
    static const QString sIConflict;
    static const QString sITagSt;
    static const QString sIDbAdd;
    static const QString sILldp;
    static const QString sIMap;
protected:
    static void staticInit();
    typedef QVector<int>    intVector;
    template <intVector& v> static void addCid2Col(int cid, int col) {
        if (cid < v.size() || (v.isEmpty() == false && v.last() != (col -1))) EXCEPTION(EPROGFAIL, cid);
        while (cid > v.size()) v << CID_HAVE_NO;
        v << col;
    }
    static void colHead(int x, const QString& title, const QString& icon, const QString& tooltip);
    int cxSave()     { return pItemSave->column(); }
    void showText(int column, const QString& text, const QString& tool_tip = QString());
    void showCell(int column, const QString& text, const QString& icon = QString(), const QString& tool_tip = QString());
    void showIndex(const QString& _tt = _sNul) { showCell(CX_INDEX, ppl.getName(_sPortIndex).rightJustified(3), _sNul, _tt); }
    void setSaveCell();
    void calcLinkCheck(QSqlQuery& q);
    void checkAndShowTag(int _cx_st, int _cx_left, int _cx_right);
    void showWarning(int _cx);
    void showPatchPort(QSqlQuery& q, int _cx, cPPort& _pp, const QString& _name = QString());
    void showShare(int _ix, ePortShare sh, ePortShare msh);
    void showNodePort(QSqlQuery, int _cx, qlonglong _pid);
    void showConflict(int _cx);
    void checkAndShowSharing(int _cx_left, int _cx_right);
    void checkAndShowLLDP(QSqlQuery& q, int _cx);
    void checkAndShowMAC(QSqlQuery& q, int _cx);
    static QList<QString>      titles;
    static QList<QString>      icons;
    static QList<QString>      tooltips;
    cDeducePatch       *parent;     ///< Parent object
    QStandardItemModel &model;      ///< QTableView model
    QStandardItem      *pItemSave;  ///< Save cell item
    cPPort             &ppl;        ///< Local or left patch port
    cPPort             &ppr;        ///< Remote or right patch ports
    cPhsLink           &pll;        ///<
    cPhsLink           &plr;        ///<
    bool reapeat;       ///< Recurrence of the hit
    bool exists;        ///< Saved, existing hit
    bool colision;      ///< Conflicts with the existing ones
    int warning;        ///< 0: OK, 1: warning, 2: error
    eTristate stateMAC;
    eTristate stateTag;
    eTristate stateLLDP;
    QString colMsg;     ///< Collision info
    QString warnMsg;    ///< Warning info
    qlonglong pidl;     ///< Local or left patch port ID
    qlonglong pidr;     ///< Remote or right patch ports ID
    qlonglong pid;
    qlonglong spidl;    ///< Local or left patch port: shared_port_id
    qlonglong spidr;    ///< Remote or right patch port: shared_port_id
    qlonglong npidl;    ///< Local or left node port ID (or NULL_ID)
    qlonglong npidr;    ///< Remote or right node port ID (or NULL_ID)
    ePortShare  shl, shr, shel, sher;
    QStandardItem *pi;
    QString s, s2;
    static const QString& warningIconName(int _w) {
        switch (_w) {
        case 0: return sIOk;
        case 1: return sIWarning;
        default:break;
        }
        return sIError;
    }
    void setHeaderItem(int id, int column)
    {
        pi = new QStandardItem();
        if (icons.at(id).isEmpty()) pi->setText(titles.at(id));
        else                        pi->setIcon(QIcon(icons.at(id)));
        if (!tooltips.at(id).isEmpty()) pi->setToolTip(tooltips.at(id));
        model.setHorizontalHeaderItem(column, pi);
    }
    void setHeader(const QVector<int>& cid2col);
    QStringList exportHeader(const QVector<int> &cid2col);
    QList<tStringPair> exporter(const QVector<int> &cid2col);

private:
    eTristate checkMac(QSqlQuery& q, cNPort& p1, cNPort& p2);
    eTristate checkMac(QSqlQuery &q);
};

class LV2GSHARED_EXPORT cDPRowMAC : public cDPRow {
public:
    enum eFieldIxMAC {
        CX_STATE_WARNING = 1, CX_STATE_CONFLICT, CX_STATE_TAG, CX_STATE_LLDP,
        CX_SAVE,
        CX_IF_LEFT, CX_SHARE_LEFT, CX_PPORT_LEFT, CX_TAG_LEFT,
        CX_TAG_RIGHT, CX_PPORT_RIGHT, CX_SHARE_RIGHT, CX_IF_RIGHT
    };
    /// Construktor, find by address
    /// @param par parent objekt
    /// @param _row index of row
    /// @param mt Found mactab object
    /// @param ppl Left (local) patch port
    /// @param pll Left (local) phisical link object [1:Term, 2:Front]
    /// @param ppr Right (remote) patch port
    /// @param plr Right (remote) phisical link object (find link to remote interface) [1:Term, 2:Front]
    cDPRowMAC(QSqlQuery& q, cDeducePatch *par, int _row, cMacTab& mt, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr);
    ~cDPRowMAC();
    void setHeader();
    virtual QStringList exportHeader();
    virtual QList<tStringPair> exporter();
private:
    static QVector<int> cid2col;
};

class LV2GSHARED_EXPORT cDPRowTag : public cDPRow {
public:
    enum eFieldIxTag {
        CX_STATE_WARNING = 1, CX_STATE_CONFLICT, CX_STATE_MAC, CX_STATE_LLDP,
        CX_SAVE,
        CX_IF_LEFT, CX_SHARE_LEFT, CX_PPORT_LEFT, CX_TAG,
        CX_PPORT_RIGHT, CX_SHARE_RIGHT, CX_IF_RIGHT
    };
    /// Construktor, find by address
    /// @param par parent objekt
    /// @param row index of row
    /// @param mt Found mactab object
    /// @param ppl Left patch port
    /// @param pll Left (local) phisical link object [1:Term, 2:Front] or enmpty object
    /// @param ppr Right patch port
    /// @param plr Right (remote) phisical link object [1:Term, 2:Front] or enmpty object
    cDPRowTag(QSqlQuery& q, cDeducePatch *par, int _row, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr);
    ~cDPRowTag();
    void setHeader();
    virtual QStringList exportHeader();
    virtual QList<tStringPair> exporter();
private:
    static QVector<int> cid2col;
};

class LV2GSHARED_EXPORT cDPRowLLDP : public cDPRow {
public:
    enum eFieldIxLLDP {
        CX_STATE_WARNING = 1, CX_STATE_CONFLICT, CX_STATE_TAG, CX_STATE_MAC,
        CX_SAVE,
        CX_IF_LEFT, CX_SHARE_LEFT, CX_PPORT_LEFT, CX_TAG_LEFT,
        CX_TAG_RIGHT, CX_PPORT_RIGHT, CX_SHARE_RIGHT, CX_IF_RIGHT
    };
    /// Construktor, find by address
    /// @param par parent objekt
    /// @param _row index of row
    /// @param mt Found mactab object
    /// @param ppl Left (local) patch port
    /// @param pll Left (local) phisical link object [1:Term, 2:Front]
    /// @param ppr Right (remote) patch port
    /// @param plr Right (remote) phisical link object (find link to remote interface) [1:Term, 2:Front]
    cDPRowLLDP(QSqlQuery& q, cDeducePatch *par, int _row, cLldpLink& lldp, cPPort& _ppl, cPhsLink& _pll, cPPort& _ppr, cPhsLink& _plr);
    ~cDPRowLLDP();
    void setHeader();
    virtual QStringList exportHeader();
    virtual QList<tStringPair> exporter();
private:
    static QVector<int> cid2col;
};

#else
namespace Ui {
    class class deducePatch;
}
class cDPRow;
#endif

class cSelectNode;

class cDeducePatch : public cIntSubObj {
    Q_OBJECT
public:
    cDeducePatch(QMdiArea *par);
    ~cDeducePatch();
    static const enum ePrivilegeLevel rights;
    void setButtons();
    void clearTable();
    void byLLDP();
    void byMAC();
    void byTag();
    bool findLink(cPhsLink& pl);
    bool findLeftId(qlonglong lpid);
    Ui::deducePatch *pUi;
    QStandardItemModel  *pModel;
    cSelectNode *pSelNode;
    cSelectNode *pSelNode2;
    QList<cDPRow *> rows;
    enum eDeducePatchMeth { DPM_LLDP, DPM_MAC, DPM_TAG } methode;
    qlonglong   nidl, nidr;
    int         nleft, nright;  /// Az első és utolsó oszlop szélessége
    bool        tableSet;
private slots:
    void changeNode(qlonglong id);
    void changeNode2(qlonglong id);
    void on_radioButtonLLDP_pressed();
    void on_radioButtonMAC_pressed();
    void on_radioButtonTag_pressed();
    void on_pushButtonStart_clicked();
    void on_pushButtonSave_clicked();
    void on_tableItem_changed(QStandardItem * pi);
    void on_toolButtonCopyZone_clicked();
    void on_toolButtonCopyPlace_clicked();
    void on_pushButtonReport_clicked();
};

#endif // DEDUCEPATCH_H
