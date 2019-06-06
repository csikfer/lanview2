#include "qtelnet.h"
#include "lanview.h"

/* Befejezetlen !!!!! */

QTelnet::QTelnet(QObject *parent) : QObject(parent)
{
    init();
}

QTelnet::QTelnet(QHostAddress& host, int to, QObject *parent) : QObject(parent)
{
    init();
    open(host, to, EX_ERROR);
}

QTelnet::~QTelnet()
{
    delete socket;
}

void QTelnet::init()
{
    socket = new QTcpSocket(this);
    setTcpPort();
    connect(socket, SIGNAL(connected()),          this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()),       this, SLOT(disconnected()));
    connect(socket, SIGNAL(readyRead()),          this, SLOT(readyRead()));
    connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
}

bool QTelnet::open(QHostAddress &host, int to, eEx __ex)
{
    if (host.isNull()) {
        QString emsg = tr("Invalid host address.");
        if (__ex) EXCEPTION(EDATA, -1, emsg);
        DERR() << emsg << endl;
        return false;
    }
    PDEB(VERBOSE) << tr("Telnet connecting to %1,..").arg(host.toString()) << endl;
    socket->connectToHost(host, port);
    if (to <= 0) return true;
    return waitForConnected(to, __ex);
}

bool QTelnet::close(int to, eEx __ex)
{
    socket->disconnectFromHost();
    if (to <= 0) return true;
    return waitForDisconnected(to, __ex);
}

bool QTelnet::waitForConnected(int to, eEx __ex)
{
    bool r = socket->waitForConnected(to);
    if (__ex && !r) EXCEPTION(ESOCKET, socket->error(), socket->errorString());
    return r;
}

bool QTelnet::waitForDisconnected(int to, eEx __ex)
{
    bool r = socket->waitForDisconnected(to);
    if (__ex && !r) EXCEPTION(ESOCKET, socket->error(), socket->errorString());
    return r;
}

/*
void QTelnet::connected()
{
    qDebug() << "Connected!";

    socket->write("HEAD / HTTP/1.0\r\n\r\n\r\n\r\n");
}

void QTelnet::disconnected()
{
    qDebug() << "Disconnected!";
}

void QTelnet::bytesWritten(qint64 bytes)
{
    qDebug() << "We wrote: " << bytes;
}

void QTelnet::readyRead()
{
    qDebug() << "Reading...";
    qDebug() << socket->readAll();
}
*/
