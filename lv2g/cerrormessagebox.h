#ifndef CERRORMESSAGEBOX_H
#define CERRORMESSAGEBOX_H

#include <QDialog>
#include "lv2g.h"

#define NONFATALBOX(ec, ...) cErrorMessageBox::condMsgBox(new cError(__FILE__, __LINE__,__PRETTY_FUNCTION__,eError::ec, ##__VA_ARGS__))

class LV2GSHARED_EXPORT cErrorMessageBox : public QDialog
{
    Q_OBJECT
public:
    explicit cErrorMessageBox(cError * _pe, QWidget *parent = 0, const QString &sMainMsg = QString());
    static int messageBox(cError * _pe, QWidget *parent = 0, const QString &sMainMsg = QString()) {
        return cErrorMessageBox(_pe, parent, sMainMsg).exec();
    }
    static bool condMsgBox(cError * _pe, QWidget *parent = 0, const QString &sMainMsg = QString());
protected:
    void row(const QString &l, const QString& val, Qt::AlignmentFlag a = Qt::AlignLeft);
    QFormLayout *pForm;
    QString text;
public slots:
    void pushed(int id);
};

#endif // CERRORMESSAGEBOX_H
