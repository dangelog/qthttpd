#include <QCoreApplication>
#include <QThread>

#include "httpserver.h"
#include "incomingconnectionevent.h"
#include "httpworker.h"

HTTPServer::HTTPServer(int threadCount, const QString &baseDir, QObject *parent)
    : QTcpServer(parent), threadCount(threadCount), nextWorker(0)
{
    for (int i = 0; i < threadCount; ++i) {
        QThread *thread = new QThread(this);
        HTTPWorker *worker = new HTTPWorker(baseDir);
        worker->moveToThread(thread);
        workers.append(worker);
        thread->start();
    }
}

void HTTPServer::incomingConnection(int descriptor)
{
    IncomingConnectionEvent *event = new IncomingConnectionEvent(descriptor);
    QCoreApplication::postEvent(workers.at(nextWorker), event);
    nextWorker = (nextWorker + 1) % threadCount;
}
