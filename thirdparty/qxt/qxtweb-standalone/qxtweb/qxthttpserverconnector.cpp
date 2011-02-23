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
\class QxtHttpServerConnector

\inmodule QxtWeb

\brief The QxtHttpServerConnector class provides a built-in HTTP server for QxtHttpSessionManager

QxtHttpSessionManager does the work of managing sessions and state for the
otherwise stateless HTTP protocol, but it relies on QxtAbstractHttpConnector
subclasses to implement the protocol used to communicate with the web server.

QxtHttpServerConnector implements a complete HTTP server internally and does
not require an external web server to function. However, it provides very
little control over the behavior of the web server and may not suitable for
high traffic scenarios or virtual hosting configurations.

\sa QxtHttpSessionManager
*/
#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"
#include <QTcpServer>
#include <QHash>
#include <QTcpSocket>
#include <QString>

#ifndef QXT_DOXYGEN_RUN
class QxtHttpServerConnectorPrivate : public QxtPrivate<QxtHttpServerConnector>
{
public:
    QTcpServer* server;
};
#endif

/*!
 * Creates a QxtHttpServerConnector with the given \a parent.
 */
QxtHttpServerConnector::QxtHttpServerConnector(QObject* parent) : QxtAbstractHttpConnector(parent)
{
    QXT_INIT_PRIVATE(QxtHttpServerConnector);
    qxt_d().server = new QTcpServer(this);
    QObject::connect(qxt_d().server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
}

/*!
 * \reimp
 */
bool QxtHttpServerConnector::listen(const QHostAddress& iface, quint16 port)
{
    return qxt_d().server->listen(iface, port);
}

/*!
 * \internal
 */
void QxtHttpServerConnector::acceptConnection()
{
    QTcpSocket* socket = qxt_d().server->nextPendingConnection();
    addConnection(socket);
}

/*!
 * \reimp
 */
bool QxtHttpServerConnector::canParseRequest(const QByteArray& buffer)
{
    if (buffer.indexOf("\r\n\r\n") >= 0) return true; // 1.0+
    if (buffer.indexOf("\r\n") >= 0 && buffer.indexOf("HTTP/") == -1) return true; // 0.9
    return false;
}

/*!
 * \reimp
 */
QHttpRequestHeader QxtHttpServerConnector::parseRequest(QByteArray& buffer)
{
    int pos = buffer.indexOf("\r\n\r\n"), endpos = pos + 3;
    if (pos == -1)
    {
        pos = buffer.indexOf("\r\n"); // 0.9
        endpos = pos + 1;
    }

    QHttpRequestHeader header(QString::fromUtf8(buffer.left(endpos)));
    QByteArray firstLine = buffer.left(buffer.indexOf('\r'));
    if (firstLine.indexOf("HTTP/") == -1)
    {
        header.setRequest(header.method(), header.path(), 0, 9);
    }
    buffer.remove(0, endpos + 1);
    return header;
}

/*!
 * \reimp
 */
void QxtHttpServerConnector::writeHeaders(QIODevice* device, const QHttpResponseHeader& header)
{
    if (header.majorVersion() == 0) return; // 0.9 doesn't have headers
    device->write(header.toString().toUtf8());
}
