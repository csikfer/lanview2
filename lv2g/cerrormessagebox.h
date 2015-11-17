#ifndef CERRORMESSAGEBOX_H
#define CERRORMESSAGEBOX_H

#include <QDialog>
#include "lv2g.h"

#define NONFATALBOX(ec, ...) cErrorMessageBox::condMsgBox(new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ec, ##__VA_ARGS__))

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
