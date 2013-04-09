
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

#ifndef QXTABSTRACTHTTPCONNECTOR_H
#define QXTABSTRACTHTTPCONNECTOR_H

#include "qxtglobal.h"
#include <QObject>
#include <QHostAddress>
#include "qhttpheader.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)
QT_FORWARD_DECLARE_CLASS(QTcpServer)
class QxtHttpSessionManager;
class QxtSslServer;

class QxtAbstractHttpConnectorPrivate;
class QXT_WEB_EXPORT QxtAbstractHttpConnector : public QObject
{
    friend class QxtHttpSessionManager;
    Q_OBJECT
public:
    QxtAbstractHttpConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port) = 0;
    virtual bool shutdown() = 0;
    virtual quint16 serverPort() const;

protected:
    QxtHttpSessionManager* sessionManager() const;

    void addConnection(QIODevice* device);
    QIODevice* getRequestConnection(quint32 requestID);
    virtual bool canParseRequest(const QByteArray& buffer) = 0;
    virtual QHttpRequestHeader parseRequest(QByteArray& buffer) = 0;
    virtual void writeHeaders(QIODevice* device, const QHttpResponseHeader& header) = 0;

private Q_SLOTS:
    void incomingData(QIODevice* device = 0);
    void disconnected();

private:
    void setSessionManager(QxtHttpSessionManager* manager);
    QXT_DECLARE_PRIVATE(QxtAbstractHttpConnector)
};

class QxtHttpServerConnectorPrivate;
class QXT_WEB_EXPORT QxtHttpServerConnector : public QxtAbstractHttpConnector
{
    Q_OBJECT
public:
    QxtHttpServerConnector(QObject* parent = 0, QTcpServer* server = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port = 80);
    virtual bool shutdown();
    virtual quint16 serverPort() const;

    QTcpServer* tcpServer() const;

protected:
    virtual bool canParseRequest(const QByteArray& buffer);
    virtual QHttpRequestHeader parseRequest(QByteArray& buffer);
    virtual void writeHeaders(QIODevice* device, const QHttpResponseHeader& header);

private Q_SLOTS:
    void acceptConnection();

private:
    QXT_DECLARE_PRIVATE(QxtHttpServerConnector)
};

#ifndef QT_NO_OPENSSL
class QXT_WEB_EXPORT QxtHttpsServerConnector : public QxtHttpServerConnector
{
    Q_OBJECT
public:
    QxtHttpsServerConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port = 443);

    QxtSslServer* tcpServer() const;

protected Q_SLOTS:
    virtual void peerVerifyError(const QSslError &error);
    virtual void sslErrors(const QList<QSslError> &errors);
};
#endif

class QxtScgiServerConnectorPrivate;
class QXT_WEB_EXPORT QxtScgiServerConnector : public QxtAbstractHttpConnector
{
    Q_OBJECT
public:
    QxtScgiServerConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port);
    virtual bool shutdown();
    virtual quint16 serverPort() const;

protected:
    virtual bool canParseRequest(const QByteArray& buffer);
    virtual QHttpRequestHeader parseRequest(QByteArray& buffer);
    virtual void writeHeaders(QIODevice* device, const QHttpResponseHeader& header);

private Q_SLOTS:
    void acceptConnection();

private:
    QXT_DECLARE_PRIVATE(QxtScgiServerConnector)
};
/* Commented out pending implementation

class QxtFcgiConnectorPrivate;
class QXT_WEB_EXPORT QxtFcgiConnector : public QxtAbstractHttpConnector {
Q_OBJECT
public:
    QxtFcgiConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port);
    virtual bool shutdown();

private:
    QXT_DECLARE_PRIVATE(QxtFcgiConnector)
};
*/

#endif // QXTABSTRACTHTTPCONNECTOR_H
