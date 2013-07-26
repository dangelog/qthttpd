#ifndef HTTPWORKER_H
#define HTTPWORKER_H

#include <QObject>
#include <QFile>
#include <QHash>
#include <QString>
#include <QTcpSocket>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "http_parser.h"

#define BUFFER_SIZE 8192


class HTTPWorker : public QObject
{
    Q_OBJECT

public:
    HTTPWorker(const QString &baseDir, QObject *parent = 0);

protected:
    bool event(QEvent *event);

private slots:
    void onReadyRead();
    void onBytesWritten(qint64 bytesWritten);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);

public:
    struct HTTPConnection {
        HTTPConnection();
        ~HTTPConnection();

        QTcpSocket *socket;
        http_parser parser;
        QNetworkRequest request;
        QNetworkAccessManager::Operation method;
        QByteArray rawUrl;
        QByteArray body;
        QByteArray currentHeaderName;
        QByteArray currentHeaderValue;

        QString baseDir;

        void tryAddCurrentHeaderValue();
        void sendCode(int code);
        void sendHeader(const QByteArray &name, const QByteArray &value);
        void endHeaders();
        void close();

        bool codeSent;
        bool headersSent;

        QFile *file;

        qint64 bytesToWrite;
    };

private:
    QString baseDir;
    QHash<QTcpSocket *, HTTPConnection *> connectionsMap;
    char buffer[BUFFER_SIZE];
};

#endif // HTTPWORKER_H
