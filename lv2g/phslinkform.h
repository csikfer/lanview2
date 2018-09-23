#ifndef PHSLINKFORM_H
#define PHSLINKFORM_H

#include <QWidget>
#include "lv2g.h"
#include "record_link.h"
#include "lv2link.h"

namespace Ui {
    class phsLinkForm;
}

class phsLinkWidget : public QWidget
{
    friend class cLinkDialog;
    Q_OBJECT
public:
    void setFirst(bool f);
    void init();
    qlonglong    getPortId() const   { return pSelectPort->currentPortId(); }
    ePhsLinkType getLinkType() const { return (ePhsLinkType)pSelectPort->currentType(); }
    ePortShare   getPortShare() const{ return (ePortShare)pSelectPort->currentShare(); }
    qlonglong    getPlaceId() const  { return pSelectPort->currentPlaceId(); }
    qlonglong    getNodeId() const   { return pSelectPort->currentNodeId(); }
    bool next();
    bool prev();
protected:
    phsLinkWidget(cLinkDialog * par);
    ~phsLinkWidget();
    Ui::phsLinkForm *   pUi;
    cLinkDialog *       parent;     // Link dalog object
    QSqlQuery *         pq;
    QButtonGroup *      pButtonsLinkType;   //Link típus : term, back, front
    cSelectLinkedPort * pSelectPort;        // Select port
    phsLinkWidget  *    pOther;             // A másik (linkelt) port
    bool                first;              // Primary
private slots:
    void toglePlaceEqu(bool f);
    void change(qlonglong, int _lt, int);
    void on_toolButtonInfo_clicked();
    void on_toolButtonStep_clicked();
    void on_toolButtonAdd_clicked();
signals:
    void changed();
};


#endif // PHSLINKFORM_H
