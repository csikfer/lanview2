#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QWidget>
#include "lv2g.h"
#include "lv2widgets.h"

namespace Ui {
class cTranslator;
}

class cTranslator;

class cTransLang : public QObject {
    Q_OBJECT
public:
    cTransLang(int ix, cTranslator *par);
    ~cTransLang();
    const cLanguage&    current() const { return _language; }
    qlonglong           currentId() const { return _langId; }
    const QString       currentName() const { return _language.getName(); }
    bool                refresh()        { return pSelectLanguage->refresh(); }
    bool                setCurrent(int _id) { return pSelectLanguage->setCurrent(_id); }
    bool                setCurrent(const QString& _n) { return pSelectLanguage->setCurrent(_n); }
    void setEnable(bool f);
private:
    cTranslator *       parent;
    int                 langIndex;
    bool                enabled;
    QComboBox *         pComboBox;
    QLabel *            pLabel;
    QLabel *            pFlag;
    QPixmap             flag;
    // cRecordListModel *  pModel;
    cSelectLanguage *   pSelectLanguage;
    QLineEdit *         pLineEdit;
    int                 _langId;
    cLanguage           _language;
protected slots:
    void on_languageIdChanged(int lid);
};

class cTransRow : public QObject {
    Q_OBJECT
public:
    cTransRow(cTranslator *par, qlonglong _rid, const QString& _rnm, qlonglong _tid, const QList<QStringList> &_texts);
    ~cTransRow();
    void save();
    bool isChanged();
    cTranslator *parent;
    int         height;
    int         firstRow;
    int         langNum;
    qlonglong   recId;
    QString     recName;
    qlonglong   textId;
    QList<QStringList> texts;
    QTableWidget *pTable;
    const cColEnumType *pTextEnum;
private:
    static QTableWidgetItem *numberItem(qlonglong n);
    static QTableWidgetItem *textItem(const QString& text);
    static QTableWidgetItem *editItem(const QString& text);
    QString getText(int row, int col);
    void subRows(int i);
};

class cTranslator : public cIntSubObj
{
    friend class cTransRow;
    friend class cTransLang;
    Q_OBJECT

public:
    explicit cTranslator(QMdiArea *par);
    ~cTranslator();
    static const enum ePrivilegeLevel rights;
protected:
    int                 langNum = 0;
    QList<qlonglong>    langIds;
    QList<cTransRow *>  rows;
    QList<const cLanguage *> languages;
private slots:
    void on_comboBoxObj_currentIndexChanged(int index);
    void on_pushButtonView_clicked();
    void on_pushButtonSave_clicked();
    void on_toolButtonAddComboBox_clicked();
    void on_toolButtonAddLanguage_clicked();
    void on_toolButtonDelComboBox_clicked();
    void on_pushButtonCancel_clicked();
    void on_pushButtonExport_clicked();
    void on_pushButtonImport_clicked();

protected:
    const cRecStaticDescr * index2TableDescr(int ix = NULL_IX);
    const cColEnumType *table2objEnum(const cRecStaticDescr * _pd);
    const cColEnumType *index2objEnum(int ix = NULL_IX);
    void clearRows();
    void enableCombos(bool e);
    Ui::cTranslator *ui;
    const cColEnumType&     enumTableForText;
    QList<cTransLang *>     selectLangs;
    cEnumListModel         *pTableListModel;
    QSqlQuery               q;
    bool                    init, down;
    bool                    viewed;
    bool                    fromImport;
    const cColEnumType     *pTextNameEnumType;
    const cRecStaticDescr  *pTableDescr;
    QString                 sTableName;
private:
    QString itemText(int row, int col, eEx __ex = EX_ERROR) const;
    qlonglong itemId(int row, int col) const;
    void setHeader(int cols);
    bool isChanged();
};

#endif // TRANSLATOR_H
