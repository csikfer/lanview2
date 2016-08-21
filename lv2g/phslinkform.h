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
    qlonglong getPortId() const;
    ePhsLinkType getLinkType() const;
    ePortShare   getPortShare() const;
protected:
    phsLinkWidget(cLinkDialog * par);
    ~phsLinkWidget();
    void placeFilter();
    bool nodeFilter();
    bool portFilter();
    void uiSetLinkType();
    void uiSetShared();

    bool firstGetPlace();
    bool firstGetNode();
    bool firstGetPort();

    enum eState { INIT, SET, READY } stat;
    Ui::phsLinkForm *   pUi;
    cLinkDialog *       parent;
    QSqlQuery *         pq;
    cPatch              node;
    cNPort *            pPrt;
    cPlaceGroup         pgrp;
    cPlace              plac;
    qlonglong           linkType;
    QString             shared;
    QButtonGroup *      pButtonsLinkType;
    cRecordListModel   *pModelZone;
    cRecordListModel   *pModelPlace;
    cRecordListModel   *pModelNode;
    cRecordListModel   *pModelPort;
    cRecordListModel   *pModelPortShare;
    phsLinkWidget  *    pOther;

    bool                first;
    QString             sPortIdX;
    QString             sNodeIdX;
    QString             sPhsLinkTypeX;
    QString             sPortSharedX;
private slots:
    void changeLinkType(int id, bool f);
    void toglePlaceEqu(bool f);
    void zoneCurrentIndex(int i);
    void placeCurrentIndex(int i);
    void nodeCurrentIndex(int i);
    void portCurrentIndex(int i);
    void portShareCurrentText(const QString &s);
signals:
    void changed();
};


#endif // PHSLINKFORM_H
