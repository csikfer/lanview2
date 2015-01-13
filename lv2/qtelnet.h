#ifndef QTELNET_H
#define QTELNET_H

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QRegExp>

class QTelnet : public QObject
{
    Q_OBJECT
public:
    explicit QTelnet(QObject *parent = 0);
    explicit QTelnet(QHostAddress& host, int to = -1, QObject *parent = 0);
    ~QTelnet();

    bool open(QHostAddress& host, int to = -1, bool __ex = true);
    bool close(int to = -1, bool __ex = true);
    bool waitForConnected(int to = 30000, bool __ex = true);
    bool waitForDisconnected(int to = 30000, bool __ex = true);
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
