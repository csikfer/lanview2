#ifndef QTELNET_H
#define QTELNET_H

#include "lv2_global.h"
#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QRegExp>

class QTelnet : public QObject
{
    Q_OBJECT
public:
    explicit QTelnet(QObject *parent = nullptr);
    explicit QTelnet(QHostAddress& host, int to = -1, QObject *parent = nullptr);
    ~QTelnet();

    bool open(QHostAddress& host, int to = -1, enum eEx __ex = EX_ERROR);
    bool close(int to = -1, enum eEx __ex = EX_ERROR);
    bool waitForConnected(int to = 30000, enum eEx __ex = EX_ERROR);
    bool waitForDisconnected(int to = 30000, enum eEx __ex = EX_ERROR);
    void setTcpPort(int __p = 23) { port = __p; }

signals:

public slots:

//    void connected();
//    void disconnected();
//    void bytesWritten(qint64 bytes);
//    void readyRead();

private:
    void init();
    int         port;       ///< TCP port száma
    QRegExp     prompt;     ///< Command promt
    QRegExp     userPrompt; ///< User név elötti prompt
    QRegExp     pwdPrompt;  ///< Password elötti prompt
    QRegExp     frstMsg;    ///< Opcionális bevezető üzenet
    QString     frstAnsver; ///< Az esetleges bevezető üzenetre adott válasz.
    QTcpSocket *socket;     ///<
};

#endif // QTELNET_H
