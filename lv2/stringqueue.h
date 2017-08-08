#ifndef STRINGQUEUE_H
#define STRINGQUEUE_H

#include <QtCore>
#include <QObject>
#include <QMutex>
#include "lanview.h"


class LV2SHARED_EXPORT cStringQueue : public QObject
{
    Q_OBJECT
public:
    explicit cStringQueue(QObject *parent = nullptr);
    ~cStringQueue();
    bool isEmpty();
    void push(const QString& s);
    void move(QString& s);
    QString pop();
private:
    QString data;
    QMutex  mutex;
signals:
    void ready();
};

#endif // STRINGQUEUE_H
