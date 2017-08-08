#ifndef LV2SYNTAX_H
#define LV2SYNTAX_H

#include "lv2data.h"
#include "stringqueue.h"

class LV2SHARED_EXPORT cObjectSyntax : public cRecord {
    CRECORD(cObjectSyntax);
    FEATURES(cObjectSyntax)
};

EXT_ cError * exportRecords(const QString& __tn, cStringQueue& __sq);

class LV2SHARED_EXPORT cExportThread : public QThread {
    Q_OBJECT
public:
    cExportThread(QObject *par);
    ~cExportThread();
    void start(const QString& __tn, enum QThread::Priority __pr = QThread::InheritPriority) {
        sTableName = __tn;
        QThread::start(__pr);
    }
    virtual void run();
    cError        * pLastError;
private:
    QString         sTableName;
    cStringQueue  * pStrQueue;
private slots:
    void queue();
signals:
    void sReady(const QString& s);
};

#endif // LV2SYNTAX_H
