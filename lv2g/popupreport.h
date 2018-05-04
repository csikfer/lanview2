#ifndef POPUPREPORT_H
#define POPUPREPORT_H

#include "lv2g.h"
#include "report.h"

class LV2GSHARED_EXPORT cPopupReportWindow : public QWidget {
    Q_OBJECT
public:
    cPopupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString());
    QVBoxLayout *pVLayout;
    QTextEdit   *pTextEdit;
    QHBoxLayout *pHLayout;
    QPushButton *pButtonClose;
    QPushButton *pButtonSave;
private slots:
    void save();
};

_GEX QWidget * popupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString());

_GEX QWidget * popupReportNode(QWidget *par, QSqlQuery& q, qlonglong nid);
_GEX QWidget * popupReportNode(QWidget *par, QSqlQuery& q, cRecord *po);
_GEX QWidget * popupReportByIp(QWidget *par, QSqlQuery& q, const QString& sIp);
_GEX QWidget * popupReportByMAC(QWidget *par, QSqlQuery& q, const QString& sMAC);


#endif // POPUPREPORT_H
