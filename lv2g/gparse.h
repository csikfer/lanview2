#ifndef GPARSE_H
#define GPARSE_H

#include "lv2g.h"
#include "import_parser.h"

#include "lv2g.h"
#if defined(LV2G_LIBRARY)
#include "ui_gparse.h"
#else
namespace Ui {
    class GParseWidget;
}
#endif

class  LV2GSHARED_EXPORT cParseWidget : public cIntSubObj
{
    Q_OBJECT
public:
    cParseWidget(QMdiArea *par);
    ~cParseWidget();
    static const enum ePrivilegeLevel rights;
private:
    Ui::GParseWidget *pUi;
private slots:
    void loadClicked();
    void saveClicked();
    void parseClicked();
    void debugLine();
    void localParseFinished();
    void localParseBreak();
private:
    void localParse(const QString &src);
    void remoteParse(const QString &src);
    QString fileName;
    bool    isRuning;
    cImportParseThread *pLocalParser;
    cError             *pLocalError;
    QString            *pLocalParsedStr;
};

#endif // GPARSE_H
