#ifndef DEDUCEPATCH_H
#define DEDUCEPATCH_H
#include "lv2g.h"
#include "lv2link.h"
#include "srvdata.h"
#include "lv2models.h"

#if defined(LV2G_LIBRARY)
#include "ui_deducepatch.h"
class cDeducePatch;
class LV2GSHARED_EXPORT cDPRow : public QObject {
    Q_OBJECT
public:
    /// Konstruktor táblázat egy sora. Cím tábla szerinti keresés.
    /// @param par parent objektum
    /// @param _row sor index a táblában
    /// @param il Bal oldali interfész port (linkelt a bal oldali patch porttal)
    /// @param ppl Bal oldali patch port
    /// @param pll Fizikai link objektum il és ppl között.
    /// @param plr Jobb oldali patch porthoz tartozó végponti front link objektum
    cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, cMacTab& mt, cNPort& il, cPPort& ppl, cPhsLink& pll, cPhsLink& plr);
    /// Konstruktor táblázat egy sora. Cím tábla szerinti keresés.
    /// @param par parent objektum
    /// @param _row sor index a táblában
    /// @param unique
    /// @param ppl Bal oldali patch port objektum
    /// @param ppr Jobb oldali patch port objektum
    cDPRow(QSqlQuery& q, cDeducePatch *par, int _row, bool unique, cPPort& ppl, cPPort& ppr);
    QTextEdit *newTextEdit(const QString& txt, int *pw = NULL, const QString& tt = QString());
    cDeducePatch * parent;
    QTableWidget * pTable;
    const int      row;
    QCheckBox    * pCheckBox;
    cPhsLink       phsLink;
private slots:
    void checkBoxchange(bool f);
};


#else
namespace Ui {
    class class deducePatch;
}
class cDPRow;
#endif

class cSelectNode;

class cDeducePatch : public cIntSubObj {
    friend class cDPRow;
    Q_OBJECT
public:
    cDeducePatch(QMdiArea *par);
    ~cDeducePatch();
    static const enum ePrivilegeLevel rights;
protected:
    void setButtons();
    void clearTable();
    void byLLDP(QSqlQuery& q);
    void byMAC(QSqlQuery& q);
    void byTag(QSqlQuery& q);
    bool findLink(cPhsLink& pl);
    Ui::deducePatch *pUi;
    cSelectNode *pSelNode;
    cSelectNode *pSelNode2;
    QList<cDPRow *> rows;
    enum eDeducePatchMeth { DPM_LLDP, DPM_MAC, DPM_TAG } methode;
    qlonglong   nid, nid2;
    cPatch      patch;
    int         nleft, nright;  /// Az első és utolsó oszlop szélessége
private slots:
    void changeNode(qlonglong id);
    void changeNode2(qlonglong id);
    void on_radioButtonLLDP_pressed();
    void on_radioButtonMAC_pressed();
    void on_radioButtonTag_pressed();
    void on_pushButtonStart_clicked();
    void on_pushButtonSave_clicked();
};

#endif // DEDUCEPATCH_H
