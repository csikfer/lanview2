#ifndef POPUPREPORT_H
#define POPUPREPORT_H

#include "lv2g.h"
#include "report.h"

_GEX QWidget * popupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString());

_GEX QWidget * popupReportNode(QWidget *par, QSqlQuery& q, qlonglong nid);
_GEX QWidget * popupReportNode(QWidget *par, QSqlQuery& q, cRecord *po);
_GEX QWidget * popupReportByIp(QWidget *par, QSqlQuery& q, const QString& sIp);
_GEX QWidget * popupReportByMAC(QWidget *par, QSqlQuery& q, const QString& sMAC);


#endif // POPUPREPORT_H
