#ifndef POPUPREPORT_H
#define POPUPREPORT_H

#include "lv2g.h"
#include "report.h"

class LV2GSHARED_EXPORT cPopupReportWindow : public QWidget {
    Q_OBJECT
public:
    cPopupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString(), bool isHtml = true);
    void resizeByText();
    QVBoxLayout *pVLayout;
    QTextEdit   *pTextEdit;
    QHBoxLayout *pHLayout;
    QPushButton *pButtonClose;
    QPushButton *pButtonSave;
    QPushButton *pButtonPrint;
    QSize  screenSize;
private slots:
    void save();
    void print();
};

inline cPopupReportWindow* popupReportWindow(QWidget* _par, const QString& _text, const QString& _title = QString())
{
    return new cPopupReportWindow(_par, _text, _title);
}

inline cPopupReportWindow* popupTextWindow(QWidget* _par, const QString& _text, const QString& _title = QString())
{
    return new cPopupReportWindow(_par, _text, _title, false);
}

_GEX cPopupReportWindow* popupReportNode(QWidget *par, QSqlQuery& q, qlonglong nid);
_GEX cPopupReportWindow* popupReportNode(QWidget *par, QSqlQuery& q, cRecord *po);
_GEX cPopupReportWindow* popupReportByIp(QWidget *par, QSqlQuery& q, const QString& sIp);
_GEX cPopupReportWindow* popupReportByMAC(QWidget *par, QSqlQuery& q, const QString& sMAC);

// Message box (popup window)

class LV2GSHARED_EXPORT cMsgBox : public QMessageBox {
public:
    cMsgBox(eDataCharacter _dc, QWidget *par = nullptr);
    void settingDetailedText(const QString& _dt);
    static void info(const QString& _m, QWidget *par = nullptr, const QString &_dt = _sNul);
    static void warning(const QString& _m, QWidget *par = nullptr, const QString &_dt = _sNul);
    static void error(const QString& _m, QWidget *par = nullptr, const QString &_dt = _sNul);
    static bool yes(const QString& _m, QWidget *par = nullptr, const QString &_dt = _sNul);
    static bool yesno(const QString& _m, QWidget *par = nullptr, const QString &_dt = _sNul);
    static eTristate tristate(const QString& _m, QWidget *par = nullptr, const QString &_dt = _sNul);
    /**/
    static bool tryOpenRead(QFile& f, QWidget *par = nullptr);
    static bool tryOpenWrite(QFile& f, QWidget *par = nullptr);
    static bool tryOpenReadWrite(QFile& f, QWidget *par = nullptr);
    static const QString sNotFOpen;
    static const QString sNotFRead;
    static const QString sNotFWrite;
    static const QString sNotFRdWr;
protected:
    void setStandardIcon(eDataCharacter _dc);
};

class LV2GSHARED_EXPORT cFileDialog {
public:
    static QFile *srcFile(QString &fileName, const QString& filter, QWidget *par = nullptr);
    static QFile *trgFile(QString &fileName, const QString& filter, QWidget *par = nullptr);
    static bool textToFile(QString &fileName, const QString& text, QWidget *par = nullptr);
    static bool textFromFile(QString &fileName, QString &text, QWidget * par = nullptr);
    static bool textEditToFile(QString &fileName, const QTextEdit *pTE, QWidget * par = nullptr);
    static QString dirName(const QString& fileName);

    static const QString sInpFileFilter;
    static const QString sHtmFileFilter;
    static const QString sCsvFileFilter;
    static const QString sTrgFileTitle;
    static const QString sSrcFileTitle;
};



#endif // POPUPREPORT_H
