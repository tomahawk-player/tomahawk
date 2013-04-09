
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

/*!
\class QxtHttpSessionManager

\inmodule QxtWeb

\brief The QxtHttpSessionManager class provides a session manager for HTTP-based protocols

QxtHttpSessionManager is a QxtWeb session manager that adds session management
support to the normally stateless HTTP model.

In addition to session management, QxtHttpSessionManager also supports a
static service, which can serve content that does not require session management,
such as static web pages. The static service is also used to respond to HTTP/0.9
clients that do not support cookies and HTTP/1.0 and HTTP/1.1 clients that are
rejecting cookies. If no static service is provided, these clients will only
see an "Internal Configuration Error", so it is recommended to supply a static
service, even one that only returns a more useful error message.

QxtHttpSessionManager attempts to be thread-safe in accepting connections and
posting events. It is reentrant for all other functionality.

\sa class QxtAbstractWebService
*/

#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"
#include "qxtwebcontent.h"
#include "qxtabstractwebservice.h"
#include <qxtboundfunction.h>
#include <QMutex>
#include <QList>
#include <QUuid>
#include <QIODevice>
#include <QByteArray>
#include <QPair>
#include <QMetaObject>
#include <QThread>
#include <qxtmetaobject.h>
#include <QTcpSocket>
#ifndef QT_NO_OPENSSL
#include <QSslSocket>
#endif

#ifndef QXT_DOXYGEN_RUN
class QxtHttpSessionManagerPrivate : public QxtPrivate<QxtHttpSessionManager>
{
public:
    struct ConnectionState
    {
        QxtBoundFunction *onBytesWritten, *onReadyRead, *onAboutToClose;
        bool readyRead;
        bool finishedTransfer;
        bool keepAlive;
        bool streaming;
        int httpMajorVersion;
        int httpMinorVersion;
        int sessionID;

        void clearHandlers() {
            delete onBytesWritten;
            delete onReadyRead;
            delete onAboutToClose;
            onBytesWritten = onReadyRead = onAboutToClose = 0;
        }
    };

    QxtHttpSessionManagerPrivate() : iface(QHostAddress::Any), port(80), sessionCookieName("sessionID"), connector(0), staticService(0), autoCreateSession(true),
                eventLock(QMutex::Recursive), sessionLock(QMutex::Recursive) {}
    QXT_DECLARE_PUBLIC(QxtHttpSessionManager)

    QHostAddress iface;
    quint16 port;
    QByteArray sessionCookieName;
    QxtAbstractHttpConnector* connector;
    QxtAbstractWebService* staticService;
    bool autoCreateSession;

    QMutex eventLock;
    QList<QxtWebEvent*> eventQueue;
    QHash<QPair<int,int>, QxtWebRequestEvent*> pendingRequests;

    QMutex sessionLock;
    QHash<QUuid, int> sessionKeys;                      // sessionKey->sessionID
    QHash<QIODevice*, ConnectionState> connectionState; // connection->state

    Qt::HANDLE mainThread;
};
#endif

/*!
 * Constructs a new QxtHttpSessionManager with the specified \a parent.
 */
QxtHttpSessionManager::QxtHttpSessionManager(QObject* parent) : QxtAbstractWebSessionManager(parent)
{
    QXT_INIT_PRIVATE(QxtHttpSessionManager);
    qxt_d().mainThread = QThread::currentThreadId();
}

/*!
 * Returns the interface on which the session manager will listen for incoming connections.
 * \sa setListenInterface()
 */
QHostAddress QxtHttpSessionManager::listenInterface() const
{
    return qxt_d().iface;
}

/*!
 * Sets the interface \a iface on which the session manager will listen for incoming
 * connections.
 *
 * The default value is QHostAddress::Any, which will cause the session manager
 * to listen on all network interfaces.
 *
 * \sa QxtAbstractHttpConnector::listen()
 */
void QxtHttpSessionManager::setListenInterface(const QHostAddress& iface)
{
    qxt_d().iface = iface;
}

/*!
 * Returns the port on which the session manager is expected to listen for
 * incoming connections. This is always whatever value was supplied in the
 * last setPort() and not neccessarily the port number actually being
 * used.
 * \sa setPort(), serverPort()
 */
quint16 QxtHttpSessionManager::port() const
{
    return qxt_d().port;
}

/*!
 * Sets the \a port on which the session manager should listen for incoming
 * connections.
 *
 * The default value is to listen on port 80. This is an acceptable value when
 * using QxtHttpServerConnector, but it is not likely to be desirable for other
 * connectors. You may also use 0 to allow the network layer to dynamically
 * assign a port number. In this case, the serverPort() method will
 * return the actual port assigned once the session has been successfully
 * started.
 *
 * \note Setting the port number after the session has been started will
 * have no effect unless it is shutdown and started again.
 *
 * \sa port(), serverPort()
 */
void QxtHttpSessionManager::setPort(quint16 port)
{
    qxt_d().port = port;
}

/*!
 * Returns the port on which the session manager is listening for incoming
 * connections. This will be 0 if the session manager has not been started
 * or was shutdown.
 * \sa setInterface(), setPort()
 */
quint16 QxtHttpSessionManager::serverPort() const
{
    if(qxt_d().connector)
	return connector()->serverPort();
    return 0;
}

/*!
 * \reimp
 */
bool QxtHttpSessionManager::start()
{
    Q_ASSERT(qxt_d().connector);
    return connector()->listen(listenInterface(), port());
}

/*!
 * \reimp
 */
bool QxtHttpSessionManager::shutdown()
{
    Q_ASSERT(qxt_d().connector);
    return connector()->shutdown();
}

/*!
 * Returns the name of the HTTP cookie used to track sessions in the web browser.
 * \sa setSessionCookieName()
 */
QByteArray QxtHttpSessionManager::sessionCookieName() const
{
    return qxt_d().sessionCookieName;
}

/*!
 * Sets the \a name of the HTTP cookie used to track sessions in the web
 * browser.
 *
 * The default value is "sessionID".
 *
 * \sa sessionCookieName()
 */
void QxtHttpSessionManager::setSessionCookieName(const QByteArray& name)
{
    qxt_d().sessionCookieName = name;
}

/*!
 * Sets the \a connector used to manage connections to web browsers.
 *
 * \sa connector()
 */
void QxtHttpSessionManager::setConnector(QxtAbstractHttpConnector* connector)
{
    connector->setSessionManager(this);
    qxt_d().connector = connector;
}

/*!
 * Sets the \a connector used to manage connections to web browsers.
 *
 * This overload is provided for convenience and can construct the predefined
 * connectors provided with Qxt.
 *
 * \sa connector()
 */
void QxtHttpSessionManager::setConnector(Connector connector)
{
    if (connector == HttpServer)
        setConnector(new QxtHttpServerConnector(this));
    else if (connector == Scgi)
        setConnector(new QxtScgiServerConnector(this));
    /* commented out pending implementation

    else if(connector == Fcgi)
        setConnector(new QxtFcgiConnector(this));
    */
}

/*!
 * Returns the connector used to manage connections to web browsers.
 * \sa setConnector()
 */
QxtAbstractHttpConnector* QxtHttpSessionManager::connector() const
{
    return qxt_d().connector;
}

/*!
 * Returns \c true if sessions are automatically created for every connection
 * that does not already have a session cookie associated with it; otherwise
 * returns \c false.
 * \sa setAutoCreateSession()
 */
bool QxtHttpSessionManager::autoCreateSession() const
{
    return qxt_d().autoCreateSession;
}

/*!
 * Sets \a enabled whether sessions are automatically created for every connection
 * that does not already have a session cookie associated with it.
 *
 * Sessions are only created for clients that support HTTP cookies. HTTP/0.9
 * clients will never generate a session.
 *
 * \sa autoCreateSession()
 */
void QxtHttpSessionManager::setAutoCreateSession(bool enable)
{
    qxt_d().autoCreateSession = enable;
}

/*!
 * Returns the QxtAbstractWebService that is used to respond to requests from
 * connections that are not associated with a session.
 *
 * \sa setStaticContentService()
 */
QxtAbstractWebService* QxtHttpSessionManager::staticContentService() const
{
    return qxt_d().staticService;
}

/*!
 * Sets the \a service that is used to respond to requests from
 * connections that are not associated with a session.
 *
 * If no static content service is set, connections that are not associated
 * with a session will receive an "Internal Configuration Error".
 *
 * \sa staticContentService()
 */
void QxtHttpSessionManager::setStaticContentService(QxtAbstractWebService* service)
{
    qxt_d().staticService = service;
}

/*!
 * \reimp
 */
void QxtHttpSessionManager::postEvent(QxtWebEvent* h)
{
    qxt_d().eventLock.lock();
    qxt_d().eventQueue.append(h);
    qxt_d().eventLock.unlock();
    // if(h->type() == QxtWebEvent::Page)
    QMetaObject::invokeMethod(this, "processEvents", Qt::QueuedConnection);
}

/*!
 * This method removes the session cookie value corresponding to a deleted
 * service.
 */
void QxtHttpSessionManager::sessionDestroyed(int sessionID)
{
    QMutexLocker locker(&qxt_d().sessionLock);
    QUuid key = qxt_d().sessionKeys.key(sessionID);
//    qDebug() << Q_FUNC_INFO << "sessionID" << sessionID << "key" << key;
    if(!key.isNull())
	qxt_d().sessionKeys.remove(key);
}

/*!
 * Creates a new session and sends the session key to the web browser.
 *
 * Subclasses may override this function to perform custom session initialization,
 * but they must call the base class implementation in order to update the internal
 * session database and fetch a new session ID.
 */
int QxtHttpSessionManager::newSession()
{
    QMutexLocker locker(&qxt_d().sessionLock);
    int sessionID = createService();
    QUuid key;
    do
    {
        key = QUuid::createUuid();
    }
    while (qxt_d().sessionKeys.contains(key));
    qxt_d().sessionKeys[key] = sessionID;
    postEvent(new QxtWebStoreCookieEvent(sessionID, qxt_d().sessionCookieName, key.toString()));
    return sessionID;
}

/*!
 * Handles incoming HTTP requests and dispatches them to the appropriate service.
 *
 * The \a requestID is an opaque value generated by the connector.
 *
 * Subclasses may override this function to perform preprocessing on each
 * request, but they must call the base class implementation in order to
 * generate and dispatch the appropriate events.
 *
 * To facilitate use with multi-threaded applications, the event will remain
 * valid until a response is posted.
 */
void QxtHttpSessionManager::incomingRequest(quint32 requestID, const QHttpRequestHeader& header, QxtWebContent* content)
{
    QMultiHash<QString, QString> cookies;
    foreach(const QString& cookie, header.allValues("cookie"))   // QHttpHeader is case-insensitive, thankfully
    {
        foreach(const QString& kv, cookie.split("; "))
        {
            int pos = kv.indexOf('=');
            if (pos == -1) continue;
            cookies.insert(kv.left(pos), kv.mid(pos + 1));
        }
    }

    int sessionID;
    QString sessionCookie = cookies.value(qxt_d().sessionCookieName);

    qxt_d().sessionLock.lock();
    if (qxt_d().sessionKeys.contains(sessionCookie))
    {
        sessionID = qxt_d().sessionKeys[sessionCookie];
        if(!sessionID && header.majorVersion() > 0 && qxt_d().autoCreateSession)
            sessionID = newSession();
    }
    else if (header.majorVersion() > 0 && qxt_d().autoCreateSession)
    {
        sessionID = newSession();
    }
    else
    {
        sessionID = 0;
    }

    QIODevice* device = connector()->getRequestConnection(requestID);
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    state.sessionID = sessionID;
    state.httpMajorVersion = header.majorVersion();
    state.httpMinorVersion = header.minorVersion();
    if (state.httpMajorVersion == 0 || (state.httpMajorVersion == 1 && state.httpMinorVersion == 0) || header.value("connection").toLower() == "close")
        state.keepAlive = false;
    else
        state.keepAlive = true;
    qxt_d().sessionLock.unlock();

    QxtWebRequestEvent* event = new QxtWebRequestEvent(sessionID, requestID, QUrl::fromEncoded(header.path().toUtf8()));
    qxt_d().eventLock.lock();
    qxt_d().pendingRequests.insert(QPair<int,int>(sessionID, requestID), event);
    qxt_d().eventLock.unlock();
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(device);
    if (socket)
    {
        event->remoteAddress = socket->peerAddress();
#ifndef QT_NO_OPENSSL
        QSslSocket* sslSocket = qobject_cast<QSslSocket*>(socket);
        if(sslSocket) {
            event->isSecure = true;
            event->clientCertificate = sslSocket->peerCertificate();
        }
#endif
    }
    event->method = header.method();
    event->cookies = cookies;
    event->url.setScheme("http");
    if (event->url.host().isEmpty())
        event->url.setHost(header.value("host"));
    if (event->url.port() == -1)
        event->url.setPort(port());
    event->contentType = header.contentType();
    event->content = content;
    typedef QPair<QString, QString> StringPair;
    foreach(const StringPair& line, header.values())
    {
        if (line.first.toLower() == "cookie") continue;
        event->headers.insert(line.first, line.second);
    }
    event->headers.insert("X-Request-Protocol", "HTTP/" + QString::number(state.httpMajorVersion) + '.' + QString::number(state.httpMinorVersion));
    if (sessionID && session(sessionID))
    {
        QxtAbstractWebService *service = session(sessionID);
        if(content)
            content->setParent(service); // Set content ownership to the service
        service->pageRequestedEvent(event);
    }
    else if (qxt_d().staticService)
    {
        qxt_d().staticService->pageRequestedEvent(event);
    }
    else
    {
        postEvent(new QxtWebErrorEvent(0, requestID, 500, "Internal Configuration Error"));
    }
}

/*!
 * \internal
 */
void QxtHttpSessionManager::disconnected(QIODevice* device)
{
    QMutexLocker locker(&qxt_d().sessionLock);
    if (qxt_d().connectionState.contains(device)) {
        qxt_d().connectionState[device].clearHandlers();
    }
    qxt_d().connectionState.remove(device);
    device->deleteLater(); 
}

/*!
 * \reimp
 */
void QxtHttpSessionManager::processEvents()
{
    if (QThread::currentThreadId() != qxt_d().mainThread)
    {
        QMetaObject::invokeMethod(this, "processEvents", Qt::QueuedConnection);
        return;
    }
    QxtHttpSessionManagerPrivate& d = qxt_d();
    QMutexLocker locker(&d.eventLock);
    if (!d.eventQueue.count()) return;

    int ct = d.eventQueue.count(), sessionID = 0, requestID = 0, pagePos = -1;
    QxtWebRedirectEvent* re = 0;
    QxtWebPageEvent* pe = 0;
    for (int i = 0; i < ct; i++)
    {
        if (d.eventQueue[i]->type() != QxtWebEvent::Page && d.eventQueue[i]->type() != QxtWebEvent::Redirect) continue;
        pagePos = i;
        sessionID = d.eventQueue[i]->sessionID;
        if (d.eventQueue[pagePos]->type() == QxtWebEvent::Redirect)
        {
            re = static_cast<QxtWebRedirectEvent*>(d.eventQueue[pagePos]);
        }
        pe = static_cast<QxtWebPageEvent*>(d.eventQueue[pagePos]);
        requestID = pe->requestID;
        break;
    }
    if (pagePos == -1) return; // no pages to send yet

    QHttpResponseHeader header;
    QList<int> removeIDs;
    QxtWebEvent* e = 0;
    for (int i = 0; i < pagePos; i++)
    {
        if (d.eventQueue[i]->sessionID != sessionID) continue;
        e = d.eventQueue[i];
        if (e->type() == QxtWebEvent::StoreCookie)
        {
            QxtWebStoreCookieEvent* ce = static_cast<QxtWebStoreCookieEvent*>(e);
            QString cookie = ce->name + '=' + ce->data;
            if (!ce->path.isEmpty())
                cookie += "; path=" + ce->path;
            if (ce->expiration.isValid())
            {
                cookie += "; max-age=" + QString::number(QDateTime::currentDateTime().secsTo(ce->expiration))
                          + "; expires=" + ce->expiration.toUTC().toString("ddd, dd-MMM-YYYY hh:mm:ss GMT");
            }
            header.addValue("set-cookie", cookie);
            removeIDs.push_front(i);
        }
        else if (e->type() == QxtWebEvent::RemoveCookie)
        {
            QxtWebRemoveCookieEvent* ce = static_cast<QxtWebRemoveCookieEvent*>(e);
            QString path;
            if(!ce->path.isEmpty()) path = "path=" + ce->path + "; ";
            header.addValue("set-cookie", ce->name + "=; "+path+"max-age=0; expires=" + QDateTime(QDate(1970, 1, 1)).toString("ddd, dd-MMM-YYYY hh:mm:ss GMT"));
            removeIDs.push_front(i);
        }
    }
    removeIDs.push_front(pagePos);

    QIODevice* device = connector()->getRequestConnection(requestID);
    QxtWebContent* content = qobject_cast<QxtWebContent*>(device);
    // TODO: This should only be invoked when pipelining occurs
    // In theory it shouldn't cause any problems as POST is specced to not be pipelined
    if (content) content->ignoreRemainingContent();
    QHash<QPair<int,int>,QxtWebRequestEvent*>::iterator iPending =
	qxt_d().pendingRequests.find(QPair<int,int>(sessionID, requestID));
    if(iPending != qxt_d().pendingRequests.end()){
	delete *iPending;
	qxt_d().pendingRequests.erase(iPending);
    }

    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[connector()->getRequestConnection(requestID)];
    QIODevice* source;
    header.setStatusLine(pe->status, pe->statusMessage, state.httpMajorVersion, state.httpMinorVersion);

    if (re)
    {
        header.setValue("location", re->destination);
    }

    // Set custom header values
    for (QMultiHash<QString, QString>::iterator it = pe->headers.begin(); it != pe->headers.end(); ++it)
    {
        header.setValue(it.key(), it.value());
    }

    header.setContentType(pe->contentType);
    if (state.httpMajorVersion == 0 || (state.httpMajorVersion == 1 && state.httpMinorVersion == 0))
        pe->chunked = false;

    source = pe->dataSource;
    state.finishedTransfer = false;
    bool emptyContent = !source->bytesAvailable() && !pe->streaming;
    state.readyRead = source->bytesAvailable();
    state.streaming = pe->streaming;

    if (emptyContent)
    {
        header.setValue("connection", "close");
        connector()->writeHeaders(device, header);
        closeConnection(requestID);
    }
    else
    {
        pe->dataSource = 0;     // so that it isn't destroyed when the event is deleted
        state.clearHandlers();  // disconnect old handlers

        if (!pe->chunked)
        {
            state.keepAlive = false;
            state.onBytesWritten = QxtMetaObject::bind(this, SLOT(sendNextBlock(int, QObject*)), Q_ARG(int, requestID), Q_ARG(QObject*, source));
            state.onReadyRead = QxtMetaObject::bind(this, SLOT(blockReadyRead(int, QObject*)), Q_ARG(int, requestID), Q_ARG(QObject*, source));
            state.onAboutToClose = QxtMetaObject::bind(this, SLOT(closeConnection(int)), Q_ARG(int, requestID));
        }
        else
        {
            header.setValue("transfer-encoding", "chunked");
            state.onBytesWritten = QxtMetaObject::bind(this, SLOT(sendNextChunk(int, QObject*)), Q_ARG(int, requestID), Q_ARG(QObject*, source));
            state.onReadyRead = QxtMetaObject::bind(this, SLOT(chunkReadyRead(int, QObject*)), Q_ARG(int, requestID), Q_ARG(QObject*, source));
            state.onAboutToClose = QxtMetaObject::bind(this, SLOT(sendEmptyChunk(int, QObject*)), Q_ARG(int, requestID), Q_ARG(QObject*, source));
        }
        QxtMetaObject::connect(device, SIGNAL(bytesWritten(qint64)), state.onBytesWritten, Qt::QueuedConnection);
        QxtMetaObject::connect(source, SIGNAL(readyRead()), state.onReadyRead, Qt::QueuedConnection);
        QxtMetaObject::connect(source, SIGNAL(aboutToClose()), state.onAboutToClose, Qt::QueuedConnection);
        QObject::connect(device, SIGNAL(destroyed()), source, SLOT(deleteLater()));

        if (state.keepAlive)
        {
            header.setValue("connection", "keep-alive");
        }
        else
        {
            header.setValue("connection", "close");
        }
        connector()->writeHeaders(device, header);
        if (state.readyRead)
        {
            if (pe->chunked)
                sendNextChunk(requestID, source);
            else
                sendNextBlock(requestID, source);
        }
    }

    foreach(int id, removeIDs)
    {
        delete d.eventQueue.takeAt(id);
    }

    if (d.eventQueue.count())
        QMetaObject::invokeMethod(this, "processEvents", Qt::QueuedConnection);
}

/*!
 * \internal
 */
void QxtHttpSessionManager::chunkReadyRead(int requestID, QObject* dataSourceObject)
{
    QIODevice* dataSource = static_cast<QIODevice*>(dataSourceObject);
    if (!dataSource->bytesAvailable()) return;
    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!device->bytesToWrite() || qxt_d().connectionState[device].readyRead == false)
    {
        qxt_d().connectionState[device].readyRead = true;
        sendNextChunk(requestID, dataSourceObject);
    }
}

/*!
 * \internal
 */
void QxtHttpSessionManager::sendNextChunk(int requestID, QObject* dataSourceObject)
{
    QIODevice* dataSource = static_cast<QIODevice*>(dataSourceObject);
    QIODevice* device = connector()->getRequestConnection(requestID);
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    if (state.finishedTransfer)
    {
        // This is just the last block written; we're done with it
        return;
    }
    if (!dataSource->bytesAvailable())
    {
        state.readyRead = false;
        return;
    }
    QByteArray chunk = dataSource->read(32768); // this is a good chunk size
    if (chunk.size())
    {
        QByteArray data = QString::number(chunk.size(), 16).toUtf8() + "\r\n" + chunk + "\r\n";
        device->write(data);
    }
    state.readyRead = false;
    if (!state.streaming && !dataSource->bytesAvailable())
        QMetaObject::invokeMethod(this, "sendEmptyChunk", Q_ARG(int, requestID), Q_ARG(QObject*, dataSource));
}

/*!
 * \internal
 */
void QxtHttpSessionManager::sendEmptyChunk(int requestID, QObject* dataSource)
{
    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!qxt_d().connectionState.contains(device)) return;  // in case a disconnect signal and a bytesWritten signal get fired in the wrong order
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    if (state.finishedTransfer) return;
    state.finishedTransfer = true;
    device->write("0\r\n\r\n");
    dataSource->deleteLater();
    if (state.keepAlive)
    {
        delete state.onBytesWritten;
        state.onBytesWritten = 0;
        connector()->incomingData(device);
    }
    else
    {
        closeConnection(requestID);
    }
}

/*!
 * \internal
 */
void QxtHttpSessionManager::closeConnection(int requestID)
{
    QIODevice* device = connector()->getRequestConnection(requestID);
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    state.finishedTransfer = true;
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(device);
    if (socket)
        socket->disconnectFromHost();
    else
        device->close();
}

/*!
 * \internal
 */
void QxtHttpSessionManager::blockReadyRead(int requestID, QObject* dataSourceObject)
{
    QIODevice* dataSource = static_cast<QIODevice*>(dataSourceObject);
    if (!dataSource->bytesAvailable()) return;

    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!device->bytesToWrite() || qxt_d().connectionState[device].readyRead == false)
    {
        qxt_d().connectionState[device].readyRead = true;
        sendNextBlock(requestID, dataSourceObject);
    }
}

/*!
 * \internal
 */
void QxtHttpSessionManager::sendNextBlock(int requestID, QObject* dataSourceObject)
{
    QIODevice* dataSource = static_cast<QIODevice*>(dataSourceObject);
    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!qxt_d().connectionState.contains(device)) return;  // in case a disconnect signal and a bytesWritten signal get fired in the wrong order
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    if (state.finishedTransfer) return;
    if (!dataSource->bytesAvailable())
    {
        state.readyRead = false;
        return;
    }
    QByteArray chunk = dataSource->read(32768); // empirically determined to be a good chunk size
    device->write(chunk);
    state.readyRead = false;
    if (!state.streaming && !dataSource->bytesAvailable())
    {
        closeConnection(requestID);
        dataSource->deleteLater();
        state.clearHandlers();
    }
}
