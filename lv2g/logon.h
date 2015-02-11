#ifndef LOGON
#define LOGON

#include "lv2g.h"
#include "ui_logindialog.h"

enum eLogOnResult {
    LR_INITED = -1,
    LR_OK = 0,
    LR_INVALID,
    LR_CANCEL
};

class LV2GSHARED_EXPORT cLogOn : public QDialog
{
    Q_OBJECT
public:
    explicit    cLogOn(QWidget * parent = 0);
    ~cLogOn();
    eLogOnResult    getState() const { return _state; }
    static eLogOnResult logOn(QWidget *par = NULL);
    static int      _maxProbes;
private:
    eLogOnResult checkState();
    Ui::LoginDialog *ui;
    eLogOnResult    _state;
    int             _probes;
    bool            _change;
    QString         _changeTxt;
    QString         _unChangeTxt;
private slots:
    void cancel();
    void ok();
    void change();
};

#endif // LOGON

