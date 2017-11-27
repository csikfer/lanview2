#ifndef CWORKSTATION_H
#define CWORKSTATION_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"
#include "lv2link.h"
#include "lv2widgets.h"
#include "object_dialog.h"

namespace Ui {
    class wstWidget;
}
class cDialogButtons;
class cWorkstation;

class LV2GSHARED_EXPORT cWorkstation : public cIntSubObj
{
    Q_OBJECT
public:
    cWorkstation(QMdiArea *parent = 0);
    ~cWorkstation();
    static const enum ePrivilegeLevel rights;
private:
    void setMessages(bool f = true);
    class cBatchBlocker {
    public:
        typedef void (cWorkstation::*tPSet)(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
        cBatchBlocker(cWorkstation *_po, tPSet _pFSet, cBatchBlocker * _par = NULL);
        ~cBatchBlocker();
        void        begin() { ++counter; }
        bool        end(bool f = true);
        bool        test() const;
        bool        enforce(bool f = true);
        void        referState();
        void        setState(bool f = true);
        const QStringList getInfos() const;
        const QStringList getErrors() const;
        bool getStat() const;
    protected:
        int             counter;
        tPSet           pFSet;
        cWorkstation *  pOwner;
        cBatchBlocker * pParent;
        QList<cBatchBlocker *> childList;
        QStringList     sInfos;
        QStringList     sErrors;
        bool            isOk;
    };
    cBatchBlocker   bbNode;
      cBatchBlocker   bbNodeName;
      cBatchBlocker   bbNodeSerial;
      cBatchBlocker   bbNodeInventory;
      cBatchBlocker   bbNodeType;
      cBatchBlocker   bbPort;
        cBatchBlocker   bbPortName;
        cBatchBlocker   bbPortType;
          cBatchBlocker   bbIp;
        cBatchBlocker   bbPortMac;
     cBatchBlocker   bbLink;

    void msgEmpty(QLineEdit * pLineEdit, QLabel *pLabel, const QString &fn, QStringList& sErrs, QStringList& sInfs, bool &isOk);

    void setStatNode(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
      void setStatNodeName(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
      void setStatNodeSerial(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
      void setStatNodeInventory(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
      void setStatNodeType(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
      void setStatPort(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
        void setStatPortName(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
        void setStatPortType(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
          void setStatIp(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
        void setStatPortMac(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);
      void setStatLink(bool f, QStringList& sErrs, QStringList& sInfs, bool& isOk);

    void node2gui();

    bool isModify;
    Ui::wstWidget   *pUi;
    cSelectNode     *pSelNode;      /// A szerkesztendő, vagy minta eszköz kiválasztása
    cSelectPlace    *pSelPlace;     /// Az eszköz helyének a megadása
    cSelectLinkedPort *pSelLinked;  /// A linkelt port, ill. az eszköz kiválasztáse
    QButtonGroup    *pModifyButtons;
    QButtonGroup    *pLinkTypeButtons;
    cIpEditWidget   *pIpEditWidget;

    QSqlQuery *pq;
    /// Kiválasztott workstation objektum (modosítandó eredetije vagy minta)
    cNode      *pSample;
    /// A szerkesztett workstation objektum
    cNode       node;   ///< A munkaállomás objektum
    cNPort *    pnp;    ///< A munkaállomás egy portjára pointer
    cInterface *pif;    ///< Ha a munkaállomás portja egy interface, akkor arra egy pointer
    cIpAddress *pip;    ///< Ha a munkaállomás portja egy interface, akkor az ip cím objektumra pointer
    bool        portIsLinkage;  ///< Ha a port linkelhető (fizikailag )satlakoztatható
    cPhsLink    pl;     ///< A fizikai link
private slots:
    void on_radioButtonMod_toggled(bool checked);           // connected
    void selectedNode(qlonglong id);
    void on_checkBoxPlaceEqu_toggled(bool checked);         // connected
    void linkChanged(qlonglong _pid, int _lt, int _sh);
    void on_lineEditPName_textChanged(const QString &arg1);
    void on_comboBoxPType_currentIndexChanged(const QString &arg1);
};

#endif // CWORKSTATION_H
