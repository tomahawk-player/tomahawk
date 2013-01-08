
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

#ifndef QXTHTTPSESSIONMANAGER_H
#define QXTHTTPSESSIONMANAGER_H

#include "qxtabstractwebsessionmanager.h"
#include "qxtabstracthttpconnector.h"
#include <QHostAddress>
#include "qhttpheader.h"

class QxtWebEvent;
class QxtWebContent;

class QxtHttpSessionManagerPrivate;
class QXT_WEB_EXPORT QxtHttpSessionManager : public QxtAbstractWebSessionManager
{
    friend class QxtAbstractHttpConnector;
    Q_OBJECT
    Q_PROPERTY(QHostAddress listenInterface READ listenInterface WRITE setListenInterface)
    Q_PROPERTY(QByteArray sessionCookieName READ sessionCookieName WRITE setSessionCookieName)
    Q_PROPERTY(quint16 port READ port WRITE setPort)
    Q_PROPERTY(quint16 serverPort READ serverPort)
    Q_PROPERTY(bool autoCreateSession READ autoCreateSession WRITE setAutoCreateSession)
public:
    enum Connector { HttpServer, Scgi, Fcgi };

    QxtHttpSessionManager(QObject* parent = 0);

    virtual void postEvent(QxtWebEvent*);

    QHostAddress listenInterface() const;
    void setListenInterface(const QHostAddress& iface);

    quint16 port() const;
    void setPort(quint16 port);

    quint16 serverPort() const;

    QByteArray sessionCookieName() const;
    void setSessionCookieName(const QByteArray& name);

    bool autoCreateSession() const;
    void setAutoCreateSession(bool enable);

    QxtAbstractWebService* staticContentService() const;
    void setStaticContentService(QxtAbstractWebService* service);

    void setConnector(QxtAbstractHttpConnector* connector);
    void setConnector(Connector connector);
    QxtAbstractHttpConnector* connector() const;

    virtual bool start();
    virtual bool shutdown();

protected:
    virtual void sessionDestroyed(int sessionID);
    virtual int newSession();
    virtual void incomingRequest(quint32 requestID, const QHttpRequestHeader& header, QxtWebContent* device);

protected Q_SLOTS:
    virtual void processEvents();

private Q_SLOTS:
    void closeConnection(int requestID);
    void chunkReadyRead(int requestID, QObject* dataSource);
    void sendNextChunk(int requestID, QObject* dataSource);
    void sendEmptyChunk(int requestID, QObject* dataSource);
    void blockReadyRead(int requestID, QObject* dataSource);
    void sendNextBlock(int requestID, QObject* dataSource);

private:
    void disconnected(QIODevice* device);
    QXT_DECLARE_PRIVATE(QxtHttpSessionManager)
};

#endif
