#ifndef EXPORTS_H
#define EXPORTS_H
#include "lv2g.h"
#include "lv2syntax.h"
#if defined(LV2G_LIBRARY)
#include "ui_exports.h"
#else
namespace Ui {
    class Exports;
}
#endif

class  LV2GSHARED_EXPORT cExportsWidget : public cIntSubObj
{
    Q_OBJECT
public:
    cExportsWidget(QMdiArea *par);
    ~cExportsWidget();
    static const enum ePrivilegeLevel rights;
    QString fileName;
private:
    void disable(bool f);
    Ui::Exports *pUi;
    bool isStop;
    cExportThread *pThread;
protected slots:
    void start();
    void stop();
    void save();
    void changedText();
    void changedName(const QString &tn);
    void text(const QString &s);
    void ready();
};

#endif // EXPORTS_H
