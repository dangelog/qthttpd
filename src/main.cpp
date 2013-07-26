#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QFileInfo>

#include <QDebug>


#include "httpserver.h"
#include "incomingconnectionevent.h"

#define DEFAULT_PORT 8000

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QStringList arguments = app.arguments();

    QString baseDir;
    int port = -1;
    int threadCount = QThread::idealThreadCount();

    while (!arguments.isEmpty()) {
        QString arg = arguments.takeFirst();

        if (arg == QLatin1String("--port")) {
            if (arguments.isEmpty())
                qFatal("Error: option --port expects an argument");

            QString portArg = arguments.takeFirst();
            bool ok;
            port = portArg.toInt(&ok);

            if (!ok || port < 0 || port > 65535)
                qFatal("Error: wrong argument passed to --port");
        } else if (arg == QLatin1String("--threads")) {
            if (arguments.isEmpty())
                qFatal("Error: option --threads expects an argument");

            QString threadsArg = arguments.takeFirst();
            bool ok;
            threadCount = threadsArg.toInt(&ok);

            if (!ok || threadCount < 0 || threadCount > 1024)
                qFatal("Error: wrong argument passed to --threads");
        } else {
            baseDir = arg;
        }
    }

    if (port == -1)
        port = DEFAULT_PORT;

    QFileInfo baseDirInfo(baseDir);
    baseDir = baseDirInfo.canonicalFilePath();

    // rely on the exists() check done by canonicalFilePath
    if (baseDir.isEmpty() || !baseDirInfo.isDir())
        qFatal("Usage: qthttpd [--port port] [--threads threads] baseDir");

    IncomingConnectionEvent::IncomingConnectionEventType = QEvent::registerEventType();

    baseDir.append(QLatin1Char('/'));

    HTTPServer server(threadCount, baseDir);
    if (!server.listen(QHostAddress::Any, port))
        qFatal("Server cannot listen: %s", qPrintable(server.errorString()));

    qWarning() << "Starting httpd server on port" << port
            << "with" << threadCount << "threads"
            << "serving dir" << baseDir;

    return app.exec();
}
