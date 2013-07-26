#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QTcpServer>
#include <QVector>

class HTTPWorker;

class HTTPServer : public QTcpServer
{
public:
    HTTPServer(int threadCount, const QString &baseDir, QObject *parent = 0);

protected:
    void incomingConnection(int handle);

private:
    QVector<HTTPWorker *> workers;
    int threadCount;
    int nextWorker;
};

#endif // HTTPSERVER_H
