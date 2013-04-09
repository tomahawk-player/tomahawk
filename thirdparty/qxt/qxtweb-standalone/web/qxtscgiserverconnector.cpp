
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
\class QxtScgiServerConnector

\inmodule QxtWeb

\brief The QxtScgiServerConnector class provides an SCGI connector for QxtHttpSessionManager


QxtScgiServerConnector implements the SCGI protocoll supported by almost all modern web servers.



\sa QxtHttpSessionManager
*/
#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"
#include <QTcpServer>
#include <QHash>
#include <QTcpSocket>
#include <QString>

#ifndef QXT_DOXYGEN_RUN
class QxtScgiServerConnectorPrivate : public QxtPrivate<QxtScgiServerConnector>
{
public:
    QTcpServer* server;
};
#endif

/*!
 * Creates a QxtHttpServerConnector with the given \a parent.
 */
QxtScgiServerConnector::QxtScgiServerConnector(QObject* parent) : QxtAbstractHttpConnector(parent)
{
    QXT_INIT_PRIVATE(QxtScgiServerConnector);
    qxt_d().server = new QTcpServer(this);
    QObject::connect(qxt_d().server, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
}

/*!
 * \reimp
 */
bool QxtScgiServerConnector::listen(const QHostAddress& iface, quint16 port)
{
    return qxt_d().server->listen(iface, port);
}

/*!
 * \reimp
 */
bool QxtScgiServerConnector::shutdown()
{
    if(qxt_d().server->isListening()){
	qxt_d().server->close();
	return true;
    }
    return false;
}

/*!
 * \reimp
 */
quint16 QxtScgiServerConnector::serverPort() const
{
    return qxt_d().server->serverPort();
}

/*!
 * \internal
 */
void QxtScgiServerConnector::acceptConnection()
{
    QTcpSocket* socket = qxt_d().server->nextPendingConnection();
    addConnection(socket);
}

/*!
 * \reimp
 */
bool QxtScgiServerConnector::canParseRequest(const QByteArray& buffer)
{
    if (buffer.size() < 10)
        return false;
    QString expectedsize;
    for (int i = 0;i < 10;i++)
    {
        if (buffer.at(i) == ':')
        {
            break;
        }
        else
        {
            expectedsize += buffer.at(i);
        }
    }

    if (expectedsize.isEmpty())
    {
        //protocoll error
        return false;
    }

    return (buffer.size() > expectedsize.toInt());
}

/*!
 * \reimp
 */
QHttpRequestHeader QxtScgiServerConnector::parseRequest(QByteArray& buffer)
{
    QString expectedsize_s;
    for (int i = 0;i < 20;i++)
    {
        if (buffer.at(i) == ':')
        {
            break;
        }
        else
        {
            expectedsize_s += buffer.at(i);
        }
    }

    if (expectedsize_s.isEmpty())
    {
        //protocoll error
        return QHttpRequestHeader();
    }


    buffer = buffer.right(buffer.size() - (expectedsize_s.count() + 1));


    QHttpRequestHeader request_m;

    QByteArray name;
    int i = 0;
    while ((i = buffer.indexOf('\0')) > -1)
    {
        if (name.isEmpty())
        {
            name = buffer.left(i);
        }
        else
        {
            request_m.setValue(QString::fromLatin1(name).toLower(), QString::fromLatin1(buffer.left(i)));
            name = "";
        }
        buffer = buffer.mid(i + 1);
    }


    request_m.setRequest(request_m.value("request_method"), request_m.value("request_uri"), 1, 0);


    foreach(const QString& key, request_m.keys())
    {
        if (key.startsWith(QString("http_")))
        {
            request_m.setValue(key.right(key.size() - 5), request_m.value(key));
        }
    }

    request_m.setValue("Connection", "close");


    buffer.chop(1);


    return request_m;
}

/*!
 * \reimp
 */
void QxtScgiServerConnector::writeHeaders(QIODevice* device, const QHttpResponseHeader& response_m)
{

    device->write(("Status:" + QString::number(response_m.statusCode()) + ' ' + response_m.reasonPhrase() + "\r\n").toLatin1());

    foreach(const QString& key, response_m.keys())
    {
        device->write((key + ':' + response_m.value(key) + "\r\n").toLatin1());
    }
    device->write("\r\n");
}
