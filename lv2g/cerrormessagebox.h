#ifndef CERRORMESSAGEBOX_H
#define CERRORMESSAGEBOX_H

#include <QDialog>
#include "lv2g.h"

class LV2GSHARED_EXPORT cErrorMessageBox : public QDialog
{
    Q_OBJECT
public:
    explicit cErrorMessageBox(cError * _pe, QWidget *parent = 0);
    static int messageBox(cError * _pe, QWidget *parent = 0) { return cErrorMessageBox(_pe, parent).exec(); }
    static int condMsgBox(cError * _pe, QWidget *parent = 0);
public slots:
    void endIt();
};

#endif // CERRORMESSAGEBOX_H
