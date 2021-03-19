#ifndef TOOLTABLE_H
#define TOOLTABLE_H

#include    "record_table.h"
#include    "tool_table_model.h"

class LV2GSHARED_EXPORT cToolTable : public cRecordTable {
    Q_OBJECT
public:
    cToolTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = nullptr, QWidget * par = nullptr);
    virtual int ixToForeignKey();
    virtual QStringList where(QVariantList& qParams);
    virtual void buttonPressed(int id);
    virtual void insert(bool _similar);
    virtual void modify(eEx __ex);
    virtual void setEditButtons();

    bool execute(QString &msg);
    bool exec_command(const QString& stmt, cFeatures &__f, QString& msg);
//    void exec_url(const QString& stmt, cRecord& o, cFeatures& features);
//    void exec_parse(const QString& stmt, cRecord& o, cFeatures& features);
private:
    QString getParValue(QSqlQuery& q, const QString& key);
    cRecord * pToolObj;
    cRecordAny tool;
    cRecordAny object;
    cFeatures   features;
    QString     eMsg;
};


#endif // TOOLTABLE_H
