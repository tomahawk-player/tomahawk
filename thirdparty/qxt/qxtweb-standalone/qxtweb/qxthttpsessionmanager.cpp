/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtWeb module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/

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

\sa QxtAbstractWebService
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

#ifndef QXT_DOXYGEN_RUN
class QxtHttpSessionManagerPrivate : public QxtPrivate<QxtHttpSessionManager>
{
public:
    struct ConnectionState
    {
        QxtBoundFunction* onBytesWritten;
        bool readyRead;
        bool finishedTransfer;
        bool keepAlive;
        bool streaming;
        int httpMajorVersion;
        int httpMinorVersion;
        int sessionID;
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
 * \sa setInterface
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
 * \sa QxtAbstractHttpConnector::listen
 */
void QxtHttpSessionManager::setListenInterface(const QHostAddress& iface)
{
    qxt_d().iface = iface;
}

/*!
 * Returns the port on which the session manager will listen for incoming connections.
 * \sa setInterface
 */
quint16 QxtHttpSessionManager::port() const
{
    return qxt_d().port;
}

/*!
 * Sets the \a port on which the session manager will listen for incoming connections.
 *
 * The default value is to listen on port 80. This is an acceptable value when
 * using QxtHttpServerConnector, but it is not likely to be desirable for other
 * connectors.
 *
 * \sa port
 */
void QxtHttpSessionManager::setPort(quint16 port)
{
    qxt_d().port = port;
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
 * Returns the name of the HTTP cookie used to track sessions in the web browser.
 * \sa setSessionCookieName
 */
QByteArray QxtHttpSessionManager::sessionCookieName() const
{
    return qxt_d().sessionCookieName;
}

/*!
 * Sets the \a name of the HTTP cookie used to track sessions in the web browser.
 *
 * The default value is "sessionID".
 *
 * \sa sessionCookieName
 */
void QxtHttpSessionManager::setSessionCookieName(const QByteArray& name)
{
    qxt_d().sessionCookieName = name;
}

/*!
 * Sets the \a connector used to manage connections to web browsers.
 *
 * \sa connector
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
 * \sa connector
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
 * \sa setConnector
 */
QxtAbstractHttpConnector* QxtHttpSessionManager::connector() const
{
    return qxt_d().connector;
}

/*!
 * Returns \c true if sessions are automatically created for every connection
 * that does not already have a session cookie associated with it; otherwise
 * returns \c false.
 * \sa setAutoCreateSession
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
 * \sa autoCreateSession
 */
void QxtHttpSessionManager::setAutoCreateSession(bool enable)
{
    qxt_d().autoCreateSession = enable;
}

/*!
 * Returns the QxtAbstractWebService that is used to respond to requests from
 * connections that are not associated with a session.
 *
 * \sa setStaticContentService
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
 * \sa staticContentService
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
    postEvent(new QxtWebStoreCookieEvent(sessionID, qxt_d().sessionCookieName, key));
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

    QxtWebRequestEvent* event = new QxtWebRequestEvent(sessionID, requestID, QUrl(header.path()));
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(device);
    if (socket)
    {
        event->remoteAddress = socket->peerAddress().toString();
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
        session(sessionID)->pageRequestedEvent(event);
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
    if (qxt_d().connectionState.contains(device))
        delete qxt_d().connectionState[device].onBytesWritten;
    qxt_d().connectionState.remove(device);
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
            header.addValue("set-cookie", ce->name + "=; max-age=0; expires=" + QDateTime(QDate(1970, 1, 1)).toString("ddd, dd-MMM-YYYY hh:mm:ss GMT"));
            removeIDs.push_front(i);
        }
    }
    removeIDs.push_front(pagePos);

    QIODevice* device = connector()->getRequestConnection(requestID);
    QxtWebContent* content = qobject_cast<QxtWebContent*>(device);
    // TODO: This should only be invoked when pipelining occurs
    // In theory it shouldn't cause any problems as POST is specced to not be pipelined
    if (content) content->ignoreRemainingContent();

    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[connector()->getRequestConnection(requestID)];

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

    connector()->setRequestDataSource( pe->requestID, pe->dataSource );
    QSharedPointer<QIODevice> source( pe->dataSource );
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
        if (state.onBytesWritten) delete state.onBytesWritten;  // disconnect old handler
        if (!pe->chunked)
        {
            state.keepAlive = false;
            state.onBytesWritten = QxtMetaObject::bind(this, SLOT(sendNextBlock(int)),
                                                             Q_ARG(int, requestID));

            QxtMetaObject::connect(source.data(), SIGNAL(readyRead()),
                                   QxtMetaObject::bind(this, SLOT(blockReadyRead(int)),
                                                             Q_ARG(int, requestID)),
                                   Qt::QueuedConnection);

            QxtMetaObject::connect(source.data(), SIGNAL(aboutToClose()),
                                   QxtMetaObject::bind(this, SLOT(closeConnection(int)),
                                                             Q_ARG(int, requestID)),
                                   Qt::QueuedConnection);
        }
        else
        {
            header.setValue("transfer-encoding", "chunked");
            state.onBytesWritten = QxtMetaObject::bind(this, SLOT(sendNextChunk(int)),
                                                             Q_ARG(int, requestID));

            QxtMetaObject::connect(source.data(), SIGNAL(readyRead()),
                                   QxtMetaObject::bind(this, SLOT(chunkReadyRead(int)),
                                                             Q_ARG(int, requestID)),
                                   Qt::QueuedConnection);

            QxtMetaObject::connect(source.data(), SIGNAL(aboutToClose()),
                                   QxtMetaObject::bind(this, SLOT(sendEmptyChunk(int)),
                                                             Q_ARG(int, requestID)),
                                   Qt::QueuedConnection);
        }
        QxtMetaObject::connect(device, SIGNAL(bytesWritten(qint64)), state.onBytesWritten, Qt::QueuedConnection);

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
                sendNextChunk(requestID);
            else
                sendNextBlock(requestID);
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
void QxtHttpSessionManager::chunkReadyRead(int requestID)
{
    const QSharedPointer<QIODevice>& dataSource = connector()->getRequestDataSource( requestID );
    if (!dataSource->bytesAvailable()) return;
    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!device->bytesToWrite() || qxt_d().connectionState[device].readyRead == false)
    {
        qxt_d().connectionState[device].readyRead = true;
        sendNextChunk(requestID);
    }
}

/*!
 * \internal
 */
void QxtHttpSessionManager::sendNextChunk(int requestID)
{
    const QSharedPointer<QIODevice>& dataSource = connector()->getRequestDataSource( requestID );
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
        QMetaObject::invokeMethod(this, "sendEmptyChunk", Q_ARG(int, requestID));
}

/*!
 * \internal
 */
void QxtHttpSessionManager::sendEmptyChunk(int requestID)
{
    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!qxt_d().connectionState.contains(device)) return;  // in case a disconnect signal and a bytesWritten signal get fired in the wrong order
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    if (state.finishedTransfer) return;
    state.finishedTransfer = true;
    device->write("0\r\n\r\n");

    if (state.keepAlive)
    {
        delete state.onBytesWritten;
        state.onBytesWritten = 0;
        QSharedPointer<QIODevice>& dataSource = connector()->getRequestDataSource( requestID );
        dataSource.clear();
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
    if( !device ) return; // already closing/closed
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    state.finishedTransfer = true;
    state.onBytesWritten = NULL;
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(device);
    if (socket)
        socket->disconnectFromHost();
    else
        device->close();

    connector()->doneWithRequest( requestID );
}

/*!
 * \internal
 */
void QxtHttpSessionManager::blockReadyRead(int requestID)
{
    const QSharedPointer<QIODevice>& dataSource = connector()->getRequestDataSource( requestID );
    if (!dataSource->bytesAvailable()) return;

    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!device->bytesToWrite() || qxt_d().connectionState[device].readyRead == false)
    {
        qxt_d().connectionState[device].readyRead = true;
        sendNextBlock(requestID);
    }
}

/*!
 * \internal
 */
void QxtHttpSessionManager::sendNextBlock(int requestID)
{
    QSharedPointer<QIODevice>& dataSource = connector()->getRequestDataSource( requestID );
    QIODevice* device = connector()->getRequestConnection(requestID);
    if (!device)
        return;

    if (!qxt_d().connectionState.contains(device)) return;  // in case a disconnect signal and a bytesWritten signal get fired in the wrong order
    QxtHttpSessionManagerPrivate::ConnectionState& state = qxt_d().connectionState[device];
    if (state.finishedTransfer) return;
    if (!dataSource->bytesAvailable())
    {
        state.readyRead = false;
        return;
    }
    QByteArray chunk = dataSource->read(32768); // this is a good chunk size
    device->write(chunk);
    state.readyRead = false;
    if (!state.streaming && !dataSource->bytesAvailable())
    {
        closeConnection(requestID);
    }
}
