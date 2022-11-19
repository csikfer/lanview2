#ifndef TOOLTABLE_H
#define TOOLTABLE_H

#include    "record_table.h"
#include    "record_table_model.h"

#if defined(Q_CC_GNU)

#include <qtermwidget5/qtermwidget.h>
#include "ui_terminal.h"

class cTerminal : public QWidget {
    Q_OBJECT
    friend class cToolTable;
protected:
    cTerminal(const QString& stmt, cFeatures& __f, QString& msg);
    Ui::Terminal *pUi;
    QTermWidget  *pTerminal;
    QString       buffer;
    bool          bufConnected;
private slots:
    void appendBuffer(const QString& text);
    void save();
    void clearLog();
};

#else

class cTerminal;

#endif
class LV2GSHARED_EXPORT cToolTable : public cRecordTable {

    Q_OBJECT
public:
    cToolTable(cTableShape *pts, bool _isDialog, cRecordsViewBase *_upper = nullptr, QWidget * par = nullptr);
    virtual cRecordViewModelBase * newModel() override;
    virtual int ixToForeignKey() override;
    virtual QStringList where(QVariantList& qParams) override;
    virtual void buttonPressed(int id) override;
    virtual void insert(bool _similar) override;
    virtual void modify(eEx __ex) override;
    virtual void setEditButtons() override;

    bool execute(QString &msg);
    bool exec_command(const QString& stmt, cFeatures &__f, QString& msg);
    bool exec_url(const QString& _url, cFeatures& __f, QString& msg);
    bool exec_term(const QString& stmt, cFeatures& __f, QString& msg);
//    void exec_parse(const QString& stmt, cRecord& o, cFeatures& features);
    enum eSubType {
        ST_TOOL_OBJ_CHILD,
        ST_TOOL_OBJ_ALL
    }   subType;

    QString sProd;
    QString sKern;
private:
    QString getParValue(QSqlQuery& q, const QString& key);
    cRecord * pToolObj;     // Object -> tool kapcsoló rekord
    cRecordAny  tool;       // Tool rekoed
    cRecordAny  object;     // Objektum
    cFeatures   features;
    QString     eMsg;
};

class LV2GSHARED_EXPORT cToolTableModel : public cRecordTableModel {
public:
    cToolTableModel(cToolTable& par) : cRecordTableModel(static_cast<cRecordTable&>(par)) {
        cRecordAny o(_sToolObjects);
        ixObjectId   = o.toIndex(_sObjectId);
        ixObjectName = o.toIndex(_sObjectName);
    }
    virtual QVariant data(const QModelIndex &index, int role) const override;
    int ixObjectId;
    int ixObjectName;

};


#endif // TOOLTABLE_H
