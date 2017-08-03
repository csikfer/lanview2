#ifndef EXPORTS_H
#define EXPORTS_H
#include "lv2g.h"
#include "import_parser.h"

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
    static const enum ePrivilegeLevel rights;
    QString fileName;
private:
    Ui::Exports *pUi;
protected slots:
    void _export();
    void save();
    void changed();
};

#endif // EXPORTS_H
