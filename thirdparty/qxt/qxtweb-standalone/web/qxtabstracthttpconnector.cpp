
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
\class QxtAbstractHttpConnector

\inmodule QxtWeb

\brief The QxtAbstractHttpConnector class is a base class for defining 
HTTP-based protocols for use with QxtHttpSessionManager

QxtHttpSessionManager does the work of managing sessions and state for the
otherwise stateless HTTP protocol, but it relies on QxtAbstractHttpConnector
subclasses to implement the protocol used to communicate with the web server.

Subclasses are responsible for accepting new connections (by implementing
listen(const QHostAddress&, quint16) and invoking addConnection(QIODevice*)),
for informing the session manager when request headers are available (by
implementing canParseRequest(const QByteArray&)), for parsing the request
headers (by implementing parseRequest(QByteArray&)), and for writing response
headers (by implementing writeHeaders(QIODevice*, const QHttpResponseHeader&)).

\sa QxtHttpSessionManager
*/

#include "qxthttpsessionmanager.h"
#include "qxtwebcontent.h"
#include <QReadWriteLock>
#include <QHash>
#include <QIODevice>
#include <QByteArray>
#include <QPointer>

#ifndef QXT_DOXYGEN_RUN
class QxtAbstractHttpConnectorPrivate : public QxtPrivate<QxtAbstractHttpConnector>
{
public:
    QxtHttpSessionManager* manager;
    QReadWriteLock bufferLock, requestLock;
    QHash<QIODevice*, QByteArray> buffers;  // connection->buffer
    QHash<QIODevice*, QPointer<QxtWebContent> > contents;  // connection->content
    QHash<quint32, QIODevice*> requests;    // requestID->connection
    quint32 nextRequestID;

    inline quint32 getNextRequestID(QIODevice* connection)
    {
        QWriteLocker locker(&requestLock);
        do
        {
            nextRequestID++;
            if (nextRequestID == 0xFFFFFFFF) nextRequestID = 1;
        }
        while (requests.contains(nextRequestID)); // yeah, right
        requests[nextRequestID] = connection;
        return nextRequestID;
    }

    inline void doneWithBuffer(QIODevice* device)
    {
        QWriteLocker locker(&bufferLock);
        buffers.remove(device);
	contents.remove(device);
    }

    inline void doneWithRequest(quint32 requestID)
    {
        QWriteLocker locker(&requestLock);
        requests.remove(requestID);
    }

    inline QIODevice* getRequestConnection(quint32 requestID)
    {
        QReadLocker locker(&requestLock);
        return requests[requestID];
    }
};
#endif

/*!
 * Creates a QxtAbstractHttpConnector with the specified \a parent.
 *
 * Note that this is an abstract class and cannot be instantiated directly.
 */
QxtAbstractHttpConnector::QxtAbstractHttpConnector(QObject* parent) : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtAbstractHttpConnector);
    qxt_d().nextRequestID = 0;
}

/*!
 * \internal
 */
void QxtAbstractHttpConnector::setSessionManager(QxtHttpSessionManager* manager)
{
    qxt_d().manager = manager;
}

/*!
 * Returns the session manager into which the connector is installed.
 *
 * \sa QxtHttpSessionManager::setConnector()
 */
QxtHttpSessionManager* QxtAbstractHttpConnector::sessionManager() const
{
    return qxt_d().manager;
}

/*!
 * \internal
 * Returns the QIODevice associated with a \a requestID.
 *
 * The request ID is generated internally and used by the session manager.
 */
QIODevice* QxtAbstractHttpConnector::getRequestConnection(quint32 requestID)
{
    return qxt_d().getRequestConnection(requestID);
}

/*!
 * Starts managing a new connection from \a device.
 *
 * This function should be invoked by a subclass to attach incoming connections
 * to the session manager.
 */
void QxtAbstractHttpConnector::addConnection(QIODevice* device)
{
    if(!device) return;
    QWriteLocker locker(&qxt_d().bufferLock);
    qxt_d().buffers[device] = QByteArray();
    QObject::connect(device, SIGNAL(readyRead()), this, SLOT(incomingData()));
    QObject::connect(device, SIGNAL(aboutToClose()), this, SLOT(disconnected()));
    QObject::connect(device, SIGNAL(disconnected()), this, SLOT(disconnected()));
    QObject::connect(device, SIGNAL(destroyed()), this, SLOT(disconnected()));
}

/*!
 * \internal
 */
void QxtAbstractHttpConnector::incomingData(QIODevice* device)
{
    if (!device)
    {
        device = qobject_cast<QIODevice*>(sender());
        if (!device) return;
    }
    // Scope things so we don't block access during incomingRequest()
    QHttpRequestHeader header;
    QxtWebContent *content = 0;
    {
	// Fetch the incoming data block
	QByteArray block = device->readAll();
	// Check for a current content "device"
	QReadLocker locker(&qxt_d().bufferLock);
	content = qxt_d().contents[device];
	if(content && (content->wantAll() || content->bytesNeeded() > 0)){
	    // This block (or part of it) belongs to content device
	    qint64 needed = block.size();
	    if(!content->wantAll() && needed > content->bytesNeeded())
		needed = content->bytesNeeded();
	    content->write(block.constData(), needed);
	    if(block.size() <= needed)
		return; // Used it all ...
	    block.remove(0, needed);
	}
	// The data received represents a new request (or start thereof)
	qxt_d().contents[device] = content = NULL;
	QByteArray& buffer = qxt_d().buffers[device];
	buffer.append(block);
	if (!canParseRequest(buffer)) return;
	// Have received all of the headers so we can start processing
	header = parseRequest(buffer);
	QByteArray start;
	int len = header.hasContentLength() ? int(header.contentLength()) : -1;
	if(len > 0)
	{
	    if(len <= buffer.size()){
		// This request is fully-received & excess is another request
		// Leave in buffer & we'll fake a following "readyRead()"
		start = buffer.left(len);
		buffer = buffer.mid(len);
		content = new QxtWebContent(start, this);
		if(buffer.size() > 0)
		    QMetaObject::invokeMethod(this, "incomingData",
			    Qt::QueuedConnection, Q_ARG(QIODevice*, device));
	    }
	    else{
		// This request isn't finished yet but may still have one to
		// follow it. Remember the content device so we can append to
		// it until we've got it all.
		start = buffer;
		buffer.clear();
		qxt_d().contents[device] = content =
		    new QxtWebContent(len, start, this, device);
	    }
	}
	else if (header.hasKey("connection") && header.value("connection").toLower() == "close")
	{
	    // Not pipelining so we want to pass all remaining data to the
	    // content device. Although 'len' will be -1, we're using an
	    // explict value for clarity. This causes the content device
	    // to indicate it wants all remaining data.
	    start = buffer;
	    buffer.clear();
	    qxt_d().contents[device] = content =
		new QxtWebContent(-1, start, this, device);
	} // else no content
	//
	// NOTE: Buffer lock goes out of scope after this point
    }
    // Allocate request ID and process it
    quint32 requestID = qxt_d().getNextRequestID(device);
    sessionManager()->incomingRequest(requestID, header, content);
}

/*!
 * \internal
 */
void QxtAbstractHttpConnector::disconnected()
{
    quint32 requestID=0;
    QIODevice* device = qobject_cast<QIODevice*>(sender());
    if (!device) return;

    requestID = qxt_d().requests.key(device);
    qxt_d().doneWithRequest(requestID);
    qxt_d().doneWithBuffer(device);
    sessionManager()->disconnected(device);
}

/*!
 *  Returns the current local server port assigned during binding. This will
 *  be 0 if the connector isn't currently bound or when a port number isn't
 *  applicable to the connector in use.
 *
 * \sa listen()
 */
quint16 QxtAbstractHttpConnector::serverPort() const
{
    return 0;
}

/*!
 * \fn virtual bool QxtAbstractHttpConnector::listen(const QHostAddress& interface, quint16 port)
 * Invoked by the session manager to indicate that the connector should listen
 * for incoming connections on the specified \a interface and \a port.
 *
 * If the interface is QHostAddress::Any, the server will listen on all
 * network interfaces.
 * If the port is explicitly set to 0, the underlying network subsystem will
 * assign a port in the dynamic range. In this case, the resulting port number
 * may be obtained using the serverPort() method.
 *
 * Returns true on success, or false if the server could not begin listening.
 *
 * \sa addConnection(), shutdown()
 */

/*!
 * \fn virtual bool QxtAbstractHttpConnector::shutdown()
 * Invoked by the session manager to indicate that the connector should close
 * it's listener port and stop accepting new connections. A shutdown may be
 * followed by a new listen (to switch ports or handle a change in the
 * machine's IP address).
 *
 * Returns true on success, or false if the server wasn't listening.
 *
 * \sa listen()
 */

/*!
 * \fn virtual bool QxtAbstractHttpConnector::canParseRequest(const QByteArray& buffer)
 * Returns true if a complete set of request headers can be extracted from the provided \a buffer.
 */

/*!
 * \fn virtual QHttpRequestHeader QxtAbstractHttpConnector::parseRequest(QByteArray& buffer)
 * Extracts a set of request headers from the provided \a buffer.
 *
 * Subclasses implementing this function must be sure to remove the parsed data from the buffer.
 */

/*!
 * \fn virtual void QxtAbstractHttpConnector::writeHeaders(QIODevice* device, const QHttpResponseHeader& header)
 * Writes a the response \a header to the specified \a device.
 */
