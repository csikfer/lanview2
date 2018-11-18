#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QWidget>
#include "lv2g.h"
#include "lv2models.h"

namespace Ui {
class cTranslator;
}

class cTranslator;

class cTransRow : public QObject {
    Q_OBJECT
public:
    enum {
        CIX_REC_ID, CIX_REC_NAME, CIX_TID, CIX_TID_NAME, CIX_SAMPLE_TEXT, CIX_EDITED_TEXT,
        CIX_COLUMNS
    };
    cTransRow(cTranslator *par, qlonglong _rid, const QString& _rnm, qlonglong _tid, const QStringList& _sl, const QStringList& _el);

    cTranslator *parent;
    int         height;
    int         firstRow;
    qlonglong   recId;
    QString     recName;
    qlonglong   textId;
    QStringList sampleTexts;
    QStringList editTexts;
    QTableView *pTable;
    QStandardItemModel *pModel;
    const cColEnumType *pTextEnum;
private:
    QStandardItem *number(qlonglong n) {
        QString text;
        if (n >= 0) text = QString::number(n);
        QStandardItem *pi = new QStandardItem(text);
        pi->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        return pi;
    }
};

class cTranslator : public cIntSubObj
{
    friend class cTransRow;
    Q_OBJECT

public:
    explicit cTranslator(QMdiArea *par);
    ~cTranslator();
    static const enum ePrivilegeLevel rights;
protected:
    /*
    QList<qlonglong>    textIdList;
    QList<QString>      recNameList;
    QList<QStringList>  sampleTexts;
    QList<QStringList>  editedTexts; */
    QList<cTransRow *>  rows;

private slots:
    void on_comboBoxSample_currentIndexChanged(int index);

    void on_comboBoxEdit_currentIndexChanged(int index);

    void on_comboBoxObj_currentIndexChanged(int index);

    void on_pushButtonView_clicked();

    void on_pushButtonSave_clicked();

protected:
    const cRecStaticDescr * index2TableDescr(int ix = NULL_IX);
    const cColEnumType *table2objEnum(const cRecStaticDescr * _pd);
    const cColEnumType *index2objEnum(int ix = NULL_IX);
    void clearRows();

    Ui::cTranslator *ui;
    QStandardItemModel *pTableModel;
    cRecordListModel *pSampleLangModel;
    cRecordListModel *pEditedLangModel;
    cEnumListModel   *pTableListModel;
    QSqlQuery       q;
    bool            viewed;
    bool            modifyed;
    const cColEnumType *pTextNameEnumType;
    cLanguage       sampleLanguage;
    qlonglong       sampleLangId;
    cLanguage       editedLanguage;
    qlonglong       editedLangId;
    const cColEnumType& enumTableForText;
    const cRecStaticDescr *pTableDescr;
    QString         sTableName;

};

#endif // TRANSLATOR_H
