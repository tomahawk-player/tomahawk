
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
\class QxtWebContent

\inmodule QxtWeb

\brief The QxtWebContent class provides an I/O device for data sent by the web browser

QxtWebContent is a QxtFifo subclass that encapsulates data sent from the web
browser, for instance in a POST or PUT request.

In order to avoid delays while reading content sent from the client, and to
insulate multiple pipelined requests on the same connection from each other,
QxtWeb uses QxtWebContent as an abstraction for streaming data.

\sa QxtAbstractWebService
*/

#include "qxtwebcontent.h"
#include <string.h>
#include <QUrl>
#include <QCoreApplication>
#include <QThread>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <QUrlQuery>
#endif

#ifndef QXT_DOXYGEN_RUN
class QxtWebContentPrivate : public QxtPrivate<QxtWebContent>
{
public:
    QxtWebContentPrivate() : bytesNeeded(0), ignoreRemaining(false) {}
    QXT_DECLARE_PUBLIC(QxtWebContent)

    void init(int contentLength, QIODevice* device)
    {
        if (contentLength < 0)
            bytesNeeded = -1;
        else{
            bytesNeeded = contentLength - qxt_p().bytesAvailable();
	    Q_ASSERT(bytesNeeded >= 0);
	}
	if(device){
	    // Connect a disconnected signal if it has one
	    if(device->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(SIGNAL(disconnected()))) >= 0){
		QObject::connect(device, SIGNAL(disconnected()), &qxt_p(), SLOT(sourceDisconnect()), Qt::QueuedConnection);
	    }
	    // Likewise, connect an error signal if it has one
	    if(device->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(SIGNAL(error(QAbstractSocket::SocketError)))) >= 0){
		QObject::connect(device, SIGNAL(error(QAbstractSocket::SocketError)), &qxt_p(), SLOT(errorReceived(QAbstractSocket::SocketError)));
	    }
	}
    }

    qint64 bytesNeeded;
    bool ignoreRemaining;
};
#endif

/*!
 * Constructs a QxtWebContent object with the specified \a parent.
 *
 * The content provided by this constructor is the data contained in \a prime,
 * followed by whatever data is subsequently written to this object from the
 * source device up to the specified \a contentLength. Note that the provided
 * \a sourceDevice is used solely to detect socket errors and does not specify
 * parentage. This variation is ReadWrite to permit incoming data but should
 * never be written to by the service handler.
 *
 */
QxtWebContent::QxtWebContent(int contentLength, const QByteArray& prime,
	QObject *parent, QIODevice* sourceDevice) : QxtFifo(prime, parent)
{
    QXT_INIT_PRIVATE(QxtWebContent);
    qxt_d().init(contentLength, sourceDevice);
}

/*!
 * Constructs a QxtWebContent object with the specified \a parent.
 *
 * The content provided by this constructor is exactly the data contained in
 * \a content. This variation is ReadOnly.
 */
QxtWebContent::QxtWebContent(const QByteArray& content, QObject* parent)
: QxtFifo(content, parent)
{
    QXT_INIT_PRIVATE(QxtWebContent);
    qxt_d().init(content.size(), 0);
    setOpenMode(ReadOnly);
}

/*!
 * \reimp
 */
qint64 QxtWebContent::readData(char* data, qint64 maxSize)
{
    int result = QxtFifo::readData(data, maxSize);
    if(bytesAvailable() == 0 && bytesNeeded() == 0)
        QMetaObject::invokeMethod(this, "aboutToClose", Qt::QueuedConnection);
    return result;
}

/*!
 *  Returns \bold true if the total content size is unknown and
 *  \bold false otherwise.
 */
bool QxtWebContent::wantAll() const
{
    return (qxt_d().bytesNeeded == -1);
}

/*!
 * Returns the total number of bytes of content expected. This will be \bold -1
 * if the total content size is unknown. This total includes both unread
 * data and that which has not been received yet. To obtain the number of
 * bytes available for reading, use bytesAvailable().
 * \sa bytesNeeded(), wantAll()
 */
qint64 QxtWebContent::unreadBytes() const
{
    if(wantAll())
	return -1;
    return bytesAvailable() + bytesNeeded();
}

/*!
 * Returns the number of bytes of content that have not yet been written
 * from the source device. This will be \bold -1 if the total content size is
 * unknown and \bold zero once all data has been received from the source (the
 * readChannelFinished() signal will be emitted when that occurs).
 */
qint64 QxtWebContent::bytesNeeded() const
{
    return qxt_d().bytesNeeded;
}

/*!
 * \reimp
 */
qint64 QxtWebContent::writeData(const char *data, qint64 maxSize)
{
    if(!(openMode() & WriteOnly)){
	qWarning("QxtWebContent(): size=%lld but read-only", maxSize);
	return -1; // Not accepting writes
    }
    if(maxSize > 0) {
	// This must match the QxtFifo implementation for consistency
        if(maxSize > INT_MAX) maxSize = INT_MAX; // qint64 could easily exceed QAtomicInt, so let's play it safe
	if(qxt_d().bytesNeeded >= 0){
	    if(maxSize > qxt_d().bytesNeeded){
		qWarning("QxtWebContent(): size=%lld needed %lld", maxSize,
			qxt_d().bytesNeeded);
		maxSize = qxt_d().bytesNeeded;
	    }
	    qxt_d().bytesNeeded -= maxSize;
	    Q_ASSERT(qxt_d().bytesNeeded >= 0);
	}
	if(qxt_d().ignoreRemaining)
	    return maxSize;
	return QxtFifo::writeData(data, maxSize);
    }
    // Error
    return -1;
}

/*!
 * \internal
 */
void QxtWebContent::errorReceived(QAbstractSocket::SocketError)
{
    QIODevice *device = qobject_cast<QIODevice*>(sender());
    if(device)
	setErrorString(device->errorString());
}

/*!
 *  Blocks until all of the streaming data has been read from the browser.
 *
 *  Note that this method effectively runs a tight event loop. Although it
 *  will not block a thread's other activities, it may result in high CPU
 *  consumption and cause performance problems. It is suggested that you
 *  avoid use of this method and try to implement services using the
 *  readChannelFinished() signal instead.
 */
void QxtWebContent::waitForAllContent()
{
    while(qxt_d().bytesNeeded != 0 && !qxt_d().ignoreRemaining){
	// Still need data ... yield processing
	if(QCoreApplication::hasPendingEvents())
	    QCoreApplication::processEvents();
	if(this->thread() != QThread::currentThread())
	    QThread::yieldCurrentThread();
    }
}

/*!
 * Discards any data not yet read.
 *
 * After invoking this function, any further data received from the browser
 * is silently discarded.
 */
void QxtWebContent::ignoreRemainingContent()
{
    if (qxt_d().bytesNeeded == 0) return;
    if(!qxt_d().ignoreRemaining){
	qxt_d().ignoreRemaining = true;
	qxt_d().bytesNeeded = 0;
    }
}

/*!
 *  \internal
 *  This slot handles a disconnect notice from a source I/O device to handle
 *  cases where the source size is unknown. The internal state is adjusted
 *  ensuring a reader will unblock in waitForAllContent(). If unread
 *  data is present, a readyRead() signal will be emitted.
 *  The readChannelFinished() signal is emitted regardless.
 */
void QxtWebContent::sourceDisconnect()
{
    if (qxt_d().bytesNeeded == 0) return;
    if(!qxt_d().ignoreRemaining){
	qxt_d().ignoreRemaining = true;
	qxt_d().bytesNeeded = 0;
	if(bytesAvailable() != 0)
	    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
        QMetaObject::invokeMethod(this, "readChannelFinished", Qt::QueuedConnection);
    }
}

#ifndef QXT_DOXYGEN_RUN
typedef QPair<QString, QString> QxtQueryItem;
#endif

/*!
 *  Parses URL-encoded form data
 *
 *  Extracts the key/value pairs from \bold application/x-www-form-urlencoded
 *  \a data, such as the query string from the URL or the form data from a
 *  POST request.
 */
QHash<QString, QString> QxtWebContent::parseUrlEncodedQuery(const QString& data)
{
    QHash<QString, QString> rv;
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QUrl post("/?" + data);
#else
    QUrlQuery post("/?" + data);
#endif
    foreach(const QxtQueryItem& item, post.queryItems())
    {
        rv.insertMulti(item.first, item.second);
    }
    return rv;
}
