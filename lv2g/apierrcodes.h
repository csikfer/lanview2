#ifndef APIERRCODES
#define APIERRCODES

#include "lv2g.h"

#include "lv2g.h"
#if defined(LV2G_LIBRARY)
#include "ui_apierrcodes.h"
#else
namespace Ui {
    class apiErrorCodes;
}
#endif


class  LV2GSHARED_EXPORT cErrcodesWidget : public cIntSubObj
{
    Q_OBJECT
public:
    cErrcodesWidget(QMdiArea *par);
    ~cErrcodesWidget();
    static const enum ePrivilegeLevel rights;
private:
    Ui::apiErrorCodes *pUi;
};


#endif // APIERRCODES

