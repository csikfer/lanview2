#ifndef EXPORTS_H
#define EXPORTS_H
#include "lv2g.h"
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
    /// Minimális jogosultsági szint az eléréséhez.
    static const enum ePrivilegeLevel rights;
    QString fileName;
private:
    void disable(bool f);
    Ui::Exports *pUi;
    bool isStop;
//    cExportThread *pThread;
    QString lineFragment;
protected slots:
    void start();
    void save();
    void changedName(const QString &tn);
private slots:
    void on_pushButtonClear_clicked();
};

#endif // EXPORTS_H
