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

#ifndef QXTABSTRACTHTTPCONNECTOR_H
#define QXTABSTRACTHTTPCONNECTOR_H

#include "qxtglobal.h"
#include <QObject>
#include <QHostAddress>
#include <QHttpHeader>
#include <QSharedPointer>

QT_FORWARD_DECLARE_CLASS(QIODevice)
class QxtHttpSessionManager;

class QxtAbstractHttpConnectorPrivate;
class QXT_WEB_EXPORT QxtAbstractHttpConnector : public QObject
{
    friend class QxtHttpSessionManager;
    Q_OBJECT
public:
    QxtAbstractHttpConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port) = 0;

protected:
    QxtHttpSessionManager* sessionManager() const;

    void addConnection(QIODevice* device);
    QIODevice* getRequestConnection(quint32 requestID);
    void setRequestDataSource( quint32 requestID, QSharedPointer<QIODevice>& dataSource );
    QSharedPointer<QIODevice>& getRequestDataSource(quint32 requestID);
    void doneWithRequest(quint32 requestID);
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
    QxtHttpServerConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port);

protected:
    virtual bool canParseRequest(const QByteArray& buffer);
    virtual QHttpRequestHeader parseRequest(QByteArray& buffer);
    virtual void writeHeaders(QIODevice* device, const QHttpResponseHeader& header);

private Q_SLOTS:
    void acceptConnection();

private:
    QXT_DECLARE_PRIVATE(QxtHttpServerConnector)
};

class QxtScgiServerConnectorPrivate;
class QXT_WEB_EXPORT QxtScgiServerConnector : public QxtAbstractHttpConnector
{
    Q_OBJECT
public:
    QxtScgiServerConnector(QObject* parent = 0);
    virtual bool listen(const QHostAddress& iface, quint16 port);

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

private:
    QXT_DECLARE_PRIVATE(QxtFcgiConnector)
};
*/

#endif // QXTABSTRACTHTTPCONNECTOR_H
