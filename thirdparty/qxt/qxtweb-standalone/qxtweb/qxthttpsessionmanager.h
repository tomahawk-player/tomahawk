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

#ifndef QXTHTTPSESSIONMANAGER_H
#define QXTHTTPSESSIONMANAGER_H

#include "qxtabstractwebsessionmanager.h"
#include "qxtabstracthttpconnector.h"
#include <QHostAddress>
#include <QHttpHeader>
#include <QSharedPointer>

class QxtWebEvent;
class QxtWebContent;

class QxtHttpSessionManagerPrivate;
class QXT_WEB_EXPORT QxtHttpSessionManager : public QxtAbstractWebSessionManager
{
    friend class QxtAbstractHttpConnector;
    Q_OBJECT
public:
    enum Connector { HttpServer, Scgi, Fcgi };

    QxtHttpSessionManager(QObject* parent = 0);

    virtual void postEvent(QxtWebEvent*);

    QHostAddress listenInterface() const;
    void setListenInterface(const QHostAddress& iface);

    quint16 port() const;
    void setPort(quint16 port);

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

protected:
    virtual int newSession();
    virtual void incomingRequest(quint32 requestID, const QHttpRequestHeader& header, QxtWebContent* device);

protected Q_SLOTS:
    virtual void processEvents();

private Q_SLOTS:
    void closeConnection(int requestID);
    void chunkReadyRead(int requestID);
    void sendNextChunk(int requestID);
    void sendEmptyChunk(int requestID);
    void blockReadyRead(int requestID);
    void sendNextBlock(int requestID);

private:
    void disconnected(QIODevice* device);
    QXT_DECLARE_PRIVATE(QxtHttpSessionManager)
};

#endif
