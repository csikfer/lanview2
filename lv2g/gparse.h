#ifndef GPARSE_H
#define GPARSE_H

#include "lv2g.h"

#include "lv2g.h"
#if defined(LV2G_LIBRARY)
#include "ui_gparse.h"
#else
namespace Ui {
    class GParseWidget;
}
#endif


class  LV2GSHARED_EXPORT cParseWidget : public cOwnTab
{
    Q_OBJECT
public:
    cParseWidget(QWidget *par);
    ~cParseWidget();
private:
    Ui::GParseWidget *pUi;
private slots:
    void loadClicked();
    void saveCliecked();
    void parseClicked();
    void debugLine();
private:
    void localParse(const QString &src);
    void remoteParse(const QString &src);
    const QString fileFilter;
    QString dirName();
    QString fileName;
};

#endif // GPARSE_H
