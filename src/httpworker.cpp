#include <QtGlobal>
#include <QtDebug>

#include "httpworker.h"
#include "incomingconnectionevent.h"

#include <QFile>
#include <QFileInfo>

extern "C" {

static int onMessageBegin(http_parser *status);
static int onPath(http_parser *status, const char *at, size_t length);
static int onQueryString(http_parser *status, const char *at, size_t length);
static int onUrl(http_parser *status, const char *at, size_t length);
static int onFragment(http_parser *status, const char *at, size_t length);
static int onHeaderField(http_parser *status, const char *at, size_t length);
static int onHeaderValue(http_parser *status, const char *at, size_t length);
static int onHeadersComplete(http_parser *status);
static int onBody(http_parser *status, const char *at, size_t length);
static int onMessageComplete(http_parser *status);

}

static const http_parser_settings parser_settings = {
    0, // &onMessageBegin,
    0, // &onPath,
    0, // &onQueryString,
    &onUrl,
    0, // &onFragment,
    &onHeaderField,
    &onHeaderValue,
    &onHeadersComplete,
    &onBody,
    &onMessageComplete
};

HTTPWorker::HTTPWorker(const QString &baseDir, QObject *parent) :
    QObject(parent), baseDir(baseDir)
{
}

bool HTTPWorker::event(QEvent *event)
{
    if (event->type() == IncomingConnectionEvent::IncomingConnectionEventType) {
        IncomingConnectionEvent *icEvent = static_cast<IncomingConnectionEvent *>(event);
        int descriptor = icEvent->descriptor();

        QTcpSocket *socket = new QTcpSocket;
        socket->setSocketDescriptor(descriptor);

        connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        connect(socket, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));
        connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

        HTTPConnection *data = new HTTPConnection;
        // TODO move into ctor?
        data->socket = socket;
        data->baseDir = baseDir;

        connectionsMap.insert(socket, data);

        return true;
    }

    return QObject::event(event);
}

void HTTPWorker::onReadyRead()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    QByteArray data = socket->readAll();
    Q_ASSERT(!data.isEmpty());

    HTTPConnection *connection = connectionsMap.value(socket);

    http_parser *parser = &(connection->parser);

    int parsed = http_parser_execute(parser, &parser_settings, data.constData(), data.size());

    if (parser->upgrade) {
        connection->sendCode(501);
        connection->close();
    } else if (parsed != data.size()) {
        connection->sendCode(400);
        connection->close();
    }
}

void HTTPWorker::onDisconnected()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    delete connectionsMap.take(socket);
}

void HTTPWorker::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);

    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());

    socket->abort();
}


void HTTPWorker::onBytesWritten(qint64 bytes)
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
    HTTPConnection *connection = connectionsMap.value(socket);
    connection->bytesToWrite -= bytes;

    if (connection->file && connection->bytesToWrite < BUFFER_SIZE) {
        int bytesToRead = BUFFER_SIZE - connection->bytesToWrite;
        qint64 bytesRead = connection->file->read(buffer, bytesToRead);

        if (bytesRead > 0) {
            connection->bytesToWrite += socket->write(buffer, bytesRead);
        } else if (bytesRead == 0) {
            delete connection->file;
            connection->file = 0;
            connection->close();
        } else {
            socket->abort();
        }
    }

}


HTTPWorker::HTTPConnection::HTTPConnection()
    : socket(0), codeSent(false), headersSent(false), file(0), bytesToWrite(0)
{
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;
}

HTTPWorker::HTTPConnection::~HTTPConnection()
{
    delete file;
    socket->deleteLater();
}


void HTTPWorker::HTTPConnection::tryAddCurrentHeaderValue()
{
    if (!currentHeaderValue.isEmpty()) {
        request.setRawHeader(currentHeaderName, currentHeaderValue);
        currentHeaderName.clear();
        currentHeaderValue.clear();
    }
}

void HTTPWorker::HTTPConnection::sendCode(int code)
{
    Q_ASSERT(!codeSent);

    QByteArray statusCode;
    statusCode.reserve(16);
    statusCode.append("HTTP/1.1 ").append(QByteArray::number(code)).append("\r\n");

    bytesToWrite += socket->write(statusCode);

    codeSent = true;
}

void HTTPWorker::HTTPConnection::sendHeader(const QByteArray &name, const QByteArray &value)
{
    Q_ASSERT(codeSent);
    Q_ASSERT(!headersSent);

    QByteArray headerLine;
    headerLine.reserve(name.length() + value.length() + 5);
    headerLine.append(name).append(": ").append(value).append("\r\n");

    bytesToWrite += socket->write(headerLine);
}

void HTTPWorker::HTTPConnection::endHeaders()
{
    Q_ASSERT(codeSent);

    bytesToWrite += socket->write("\r\n");

    headersSent = true;
}


void HTTPWorker::HTTPConnection::close()
{
    Q_ASSERT(codeSent);
    if (!headersSent)
        endHeaders();
    socket->close();
}



// static callbacks for http-parser

static int onMessageBegin(http_parser *status)
{
    qDebug() << Q_FUNC_INFO;
    return 0;
}

static int onPath(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);
    return 0;
}

static int onQueryString(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);
    return 0;
}

static int onUrl(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);

    HTTPWorker::HTTPConnection *connection = static_cast<HTTPWorker::HTTPConnection *>(status->data);
    connection->rawUrl.append(at, length);

    return 0;
}

static int onFragment(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);
    return 0;
}


static int onHeaderField(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);

    HTTPWorker::HTTPConnection *connection = static_cast<HTTPWorker::HTTPConnection *>(status->data);

    connection->tryAddCurrentHeaderValue();
    connection->currentHeaderName.append(at, length);

    return 0;
}

static int onHeaderValue(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);

    HTTPWorker::HTTPConnection *connection = static_cast<HTTPWorker::HTTPConnection *>(status->data);

    connection->currentHeaderValue.append(at, length);

    return 0;
}

static int onHeadersComplete(http_parser *status)
{
    qDebug() << Q_FUNC_INFO;

    HTTPWorker::HTTPConnection *connection = static_cast<HTTPWorker::HTTPConnection *>(status->data);

    connection->tryAddCurrentHeaderValue();

    switch (status->method) {
    case HTTP_GET:
        connection->method = QNetworkAccessManager::GetOperation;
        break;
    case HTTP_HEAD:
        connection->method = QNetworkAccessManager::HeadOperation;
        break;
        // TODO all the methods...
    default:
        break;
    }


    QUrl url = QUrl::fromEncoded(connection->rawUrl);
    connection->rawUrl.clear();

    if (!url.isValid()) {
        connection->sendCode(400);
        connection->close();
        return 1;
    } else if (connection->method != QNetworkAccessManager::GetOperation) {
        connection->sendCode(501);
        connection->close();
        return 1;
    }

    QString path = url.path();
    if (path.isEmpty()) {
        connection->sendCode(400);
        connection->close();
        return 1;
    }

    /* TODO remove this stuff. just for dumping profile data */
    if (path == QLatin1String("/exit")) {
        ::exit(0);
    }


    QString requestedFile = connection->baseDir + path;
    QFileInfo requestedFileInfo(requestedFile);
    requestedFile = requestedFileInfo.canonicalFilePath();

    if (requestedFile.isEmpty() || !requestedFile.startsWith(connection->baseDir)) {
        connection->sendCode(404);
        connection->close();
        return 1;
    }

    connection->file = new QFile(requestedFile);
    if (!connection->file->open(QIODevice::ReadOnly)) {
        connection->sendCode(500);
        connection->close();
        return 1;
    }

    qint64 size = connection->file->size();
    if (size < 0) {
        connection->sendCode(500);
        connection->close();
        return 1;
    }

    connection->sendCode(200);
    connection->sendHeader("Content-Length", QByteArray::number(size));
    connection->sendHeader("Content-Type", "application/octet-stream");
    connection->sendHeader("Connection", "close");
    connection->endHeaders();

    return 0;
}

static int onBody(http_parser *status, const char *at, size_t length)
{
    qDebug() << Q_FUNC_INFO << QByteArray::fromRawData(at, length);

    HTTPWorker::HTTPConnection *connection = static_cast<HTTPWorker::HTTPConnection *>(status->data);
    connection->body.append(at, length);

    return 0;
}

static int onMessageComplete(http_parser *status)
{
    qDebug() << Q_FUNC_INFO;

    return 0;
}
