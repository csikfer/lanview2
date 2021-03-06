#ifndef LOGON
#define LOGON

#include "lv2g.h"
#include "lv2user.h"
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
    explicit    cLogOn(bool __needZone = true, QWidget * parent = 0);
    ~cLogOn();
    eLogOnResult    getState() const { return _state; }
    qlonglong       getZoneId() const;
    static eLogOnResult logOn(qlonglong *pZoneId = nullptr, QWidget *par = nullptr);
    static int      _maxProbes;
private:
    eLogOnResult checkState();
    Ui::LoginDialog *   ui;
    eLogOnResult        _state;
    int                 _probes;
    bool                _change;
    QString             _changeTxt;
    QString             _unChangeTxt;
    QList<qlonglong>    _zoneIdList;
    bool                _needZone;
    QString             _myDomainName;
    QString             _myUserName;
    QString             _inUser;
    cUser              *_pMyUser;
private slots:
    void cancel();
    void ok();
    void change();
    void userNameEdit(const QString &_txt);
    void userNameEdited();
    void myUser();
};

#endif // LOGON

