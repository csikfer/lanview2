#include "stringqueue.h"
#include "cerror.h"

cStringQueue::cStringQueue(QObject *parent)
    : QObject(parent)
    , data()
    , mutex()
{
    ;
}

cStringQueue::~cStringQueue()
{
    if (data.size() > 0) {
        DERR() << "Deleted cStringQueue object is not empty : \"" << data << "\"" << endl;
    }
}

bool cStringQueue::isEmpty()
{
    bool r;
    mutex.lock();
    r = data.isEmpty();
    mutex.unlock();
    return r;
}

void cStringQueue::push(const QString& s)
{
    if (!s.simplified().isEmpty()) {
        mutex.lock();
        data += s;
        mutex.unlock();
        emit ready();
    }
}

void cStringQueue::move(QString& s)
{
    if (!s.simplified().isEmpty()) {
        mutex.lock();
        data += s;
        mutex.unlock();
        emit ready();
    }
    s.clear();
}

QString cStringQueue::pop()
{
    mutex.lock();
    QString s = data;
    data.clear();
    mutex.unlock();
    return s;
}
