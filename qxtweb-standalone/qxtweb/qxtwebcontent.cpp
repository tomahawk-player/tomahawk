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
\class QxtWebContent

\inmodule QxtWeb

\brief The QxtWebContent class provides and I/O device for data sent by the web browser

QxtWebContent is a read-only QIODevice subclass that encapsulates data sent
from the web browser, for instance in a POST or PUT request.

In order to avoid delays while reading content sent from the client, and to
insulate multiple pipelined requests on the same connection from each other,
QxtWeb uses QxtWebContent as an abstraction for streaming data.

\sa QxtAbstractWebService
*/

#include "qxtwebcontent.h"
#include <string.h>
#include <QUrl>

#ifndef QXT_DOXYGEN_RUN
class QxtWebContentPrivate : public QxtPrivate<QxtWebContent>
{
public:
    QxtWebContentPrivate() : ignoreRemaining(false) {}
    QXT_DECLARE_PUBLIC(QxtWebContent)

    void init(int contentLength, const QByteArray& start, QIODevice* device)
    {
        this->start = start;
        this->device = device;
        if (contentLength <= 0)
            bytesRemaining = -1;
        else
            bytesRemaining = contentLength - start.length();
        if (device)
        {
            QObject::connect(device, SIGNAL(readyRead()), &qxt_p(), SIGNAL(readyRead()));
            // QObject::connect(device, SIGNAL(aboutToClose()), this, SIGNAL(aboutToClose()));
            // QObject::connect(device, SIGNAL(destroyed()), this, SIGNAL(aboutToClose()));
            // ask the object if it has an error signal
            if (device->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(SIGNAL(error(QAbstractSocket::SocketError)))) >= 0)
            {
                QObject::connect(device, SIGNAL(error(QAbstractSocket::SocketError)), &qxt_p(), SLOT(errorReceived(QAbstractSocket::SocketError)));
            }
        }
        qxt_p().setOpenMode(QIODevice::ReadOnly);
    }

    qint64 bytesRemaining;
    QByteArray start;
    QIODevice* device;
    bool ignoreRemaining;
};
#endif

/*!
 * Constructs a QxtWebContent object.
 *
 * The content provided by this constructor is the first \a contentLength bytes
 * read from the provided \a device.
 *
 * The QxtWebContent object is parented to the \a device.
 */
QxtWebContent::QxtWebContent(int contentLength, QIODevice* device) : QIODevice(device)
{
    QXT_INIT_PRIVATE(QxtWebContent);
    qxt_d().init(contentLength, QByteArray(), device);
}

/*!
 * Constructs a QxtWebContent object.
 *
 * The content provided by this constructor is the data contained in \a start,
 * followed by enough data read from the provided \a device to fill the desired
 * \a contentLength.
 *
 * The QxtWebContent object is parented to the \a device.
 */
QxtWebContent::QxtWebContent(int contentLength, const QByteArray& start, QIODevice* device) : QIODevice(device)
{
    QXT_INIT_PRIVATE(QxtWebContent);
    qxt_d().init(contentLength, start, device);
}

/*!
 * Constructs a QxtWebContent object with the specified \a parent.
 *
 * The content provided by this constructor is exactly the data contained in
 * \a content.
 */
QxtWebContent::QxtWebContent(const QByteArray& content, QObject* parent) : QIODevice(parent)
{
    QXT_INIT_PRIVATE(QxtWebContent);
    qxt_d().init(content.size(), content, 0);
}

/*!
 * \reimp
 */
qint64 QxtWebContent::bytesAvailable() const
{
    qint64 available = QIODevice::bytesAvailable() + (qxt_d().device ? qxt_d().device->bytesAvailable() : 0) + qxt_d().start.count();
    if (available > qxt_d().bytesRemaining)
        return qxt_d().bytesRemaining;
    return available;
}

/*!
 * \reimp
 */
qint64 QxtWebContent::readData(char* data, qint64 maxSize)
{
    char* writePtr = data;
    // read more than 32k; TCP ideally handles 48k blocks but we need wiggle room
    if (maxSize > 32768) maxSize = 32768;

    // don't read more than the content-length
    int sz = qxt_d().start.count();
    if (sz > 0 && maxSize > sz)
    {
        memcpy(writePtr, qxt_d().start.constData(), sz);
        writePtr += sz;
        maxSize -= sz;
        qxt_d().start.clear();
    }
    else if (sz > 0 && sz > maxSize)
    {
        memcpy(writePtr, qxt_d().start.constData(), maxSize);
        qxt_d().start = qxt_d().start.mid(maxSize);
        return maxSize;
    }

    if (qxt_d().device == 0)
    {
        return sz;
    }
    else if (qxt_d().bytesRemaining >= 0)
    {
        qint64 readBytes = qxt_d().device->read(writePtr, (maxSize > qxt_d().bytesRemaining) ? qxt_d().bytesRemaining : maxSize);
        qxt_d().bytesRemaining -= readBytes;
        if (qxt_d().bytesRemaining == 0) QMetaObject::invokeMethod(this, "aboutToClose", Qt::QueuedConnection);
        return sz + readBytes;
    }
    else
    {
        return sz + qxt_d().device->read(writePtr, maxSize);
    }
}

/*!
 * Returns the number of bytes of content that have not yet been read.
 *
 * Note that not all of the remaining content may be immediately available for
 * reading. This function returns the content length, minus the number of
 * bytes that have already been read.
 */
qint64 QxtWebContent::unreadBytes() const
{
    return qxt_d().start.size() + qxt_d().bytesRemaining;
}

/*!
 * \reimp
 */
qint64 QxtWebContent::writeData(const char*, qint64)
{
    // always an error to write
    return -1;
}

/*!
 * \internal
 */
void QxtWebContent::errorReceived(QAbstractSocket::SocketError)
{
    setErrorString(qxt_d().device->errorString());
}

/*!
 * Blocks until all of the streaming data has been read from the browser.
 *
 * Note that this function will block events for the thread on which it is called.
 * If the main thread is blocked, QxtWeb will be unable to process additional
 * requests until the content has been received.
 */
void QxtWebContent::waitForAllContent()
{
    if (!qxt_d().device) return;
    QByteArray buffer;
    while (qxt_d().device && qxt_d().bytesRemaining > 0)
    {
        buffer = qxt_d().device->readAll();
        qxt_d().start += buffer;
        qxt_d().bytesRemaining -= buffer.size();
        if (qxt_d().bytesRemaining > 0) qxt_d().device->waitForReadyRead(-1);
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
    if (qxt_d().bytesRemaining <= 0 || !qxt_d().device) return;
    if (!qxt_d().ignoreRemaining)
    {
        qxt_d().ignoreRemaining = true;
        QObject::connect(qxt_d().device, SIGNAL(readyRead()), this, SLOT(ignoreRemainingContent()));
    }
}

#ifndef QXT_DOXYGEN_RUN
typedef QPair<QString, QString> QxtQueryItem;
#endif

/*!
 * Extracts the key/value pairs from application/x-www-form-urlencoded \a data,
 * such as the query string from the URL or the form data from a POST request.
 */
QHash<QString, QString> QxtWebContent::parseUrlEncodedQuery(const QString& data)
{
    QUrl post("/?" + data);
    QHash<QString, QString> rv;
    foreach(const QxtQueryItem& item, post.queryItems())
    {
        rv.insertMulti(item.first, item.second);
    }
    return rv;
}
