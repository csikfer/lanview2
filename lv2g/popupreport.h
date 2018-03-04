#ifndef POPUPREPORT_H
#define POPUPREPORT_H

#include "lv2g.h"
#include "report.h"

_GEX QWidget * popupReportWindow(QObject* _par, const QString& _text, const QString& _title = QString());

_GEX QWidget * popupReportNode(QObject *par, QSqlQuery& q, qlonglong nid);
_GEX QWidget * popupReportNode(QObject *par, QSqlQuery& q, cRecord *po);
_GEX QWidget * popupReportByIp(QObject *par, QSqlQuery& q, const QString& sIp);
_GEX QWidget * popupReportByMAC(QObject *par, QSqlQuery& q, const QString& sMAC);


#endif // POPUPREPORT_H
