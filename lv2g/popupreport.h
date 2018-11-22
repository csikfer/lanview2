#ifndef POPUPREPORT_H
#define POPUPREPORT_H

#include "lv2g.h"
#include "report.h"

class LV2GSHARED_EXPORT cPopupReportWindow : public QWidget {
    Q_OBJECT
public:
    cPopupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString(), bool isHtml = true);
    void resizeByText();
    QVBoxLayout *pVLayout;
    QTextEdit   *pTextEdit;
    QHBoxLayout *pHLayout;
    QPushButton *pButtonClose;
    QPushButton *pButtonSave;
    QSize  screenSize;
private slots:
    void save();
};

static inline cPopupReportWindow* popupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString())
{
    return new cPopupReportWindow(_par, _text, _title);
}

static inline cPopupReportWindow* popupTextWindow(QWidget* _par, const QString& _text, const QString& _title = QString())
{
    return new cPopupReportWindow(_par, _text, _title, false);
}

_GEX cPopupReportWindow* popupReportNode(QWidget *par, QSqlQuery& q, qlonglong nid);
_GEX cPopupReportWindow* popupReportNode(QWidget *par, QSqlQuery& q, cRecord *po);
_GEX cPopupReportWindow* popupReportByIp(QWidget *par, QSqlQuery& q, const QString& sIp);
_GEX cPopupReportWindow* popupReportByMAC(QWidget *par, QSqlQuery& q, const QString& sMAC);


#endif // POPUPREPORT_H
