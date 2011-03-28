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

#ifndef QXTWEBEVENT_H
#define QXTWEBEVENT_H

#include <qxtglobal.h>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QPointer>
#include <QUrl>
#include <QMultiHash>
#include <QDateTime>
QT_FORWARD_DECLARE_CLASS(QIODevice)
class QxtWebContent;

class QXT_WEB_EXPORT QxtWebEvent
{
public:
    enum EventType
    {
        None = 0,
        Request,
        FileUpload,
        Page,
        StoreCookie,
        RemoveCookie,
        Redirect
    };

    QxtWebEvent(EventType type, int sessionID);
    virtual ~QxtWebEvent();

    inline EventType type() const
    {
        return m_type;
    }
    const int sessionID;

private:
    EventType m_type;
};

class QXT_WEB_EXPORT QxtWebRequestEvent : public QxtWebEvent
{
public:
    QxtWebRequestEvent(int sessionID, int requestID, const QUrl& url);
    ~QxtWebRequestEvent();

    const int requestID;

    QUrl url;
    const QUrl originalUrl;
    QString contentType;
    QPointer<QxtWebContent> content;
    QString method;
    QString remoteAddress;

    QMultiHash<QString, QString> cookies;
    QMultiHash<QString, QString> headers;
};

/* TODO: refactor and implement
class QXT_WEB_EXPORT QxtWebFileUploadEvent : public QxtWebEvent {
public:
    QxtWebFileUploadEvent(int sessionID);

    QString filename;
    int contentLength;
    QIODevice* content;
};
*/

class QxtWebRedirectEvent;
class QXT_WEB_EXPORT QxtWebPageEvent : public QxtWebEvent
{
public:
    QxtWebPageEvent(int sessionID, int requestID, QSharedPointer<QIODevice> source);
    QxtWebPageEvent(int sessionID, int requestID, QByteArray source);    // creates a QBuffer
    virtual ~QxtWebPageEvent();

    QSharedPointer<QIODevice> dataSource;          // data is read from this device and written to the client
    bool chunked;
    bool streaming;

    const int requestID;
    int status;
    QByteArray statusMessage;
    QByteArray contentType;

    QMultiHash<QString, QString> headers;

private:
    friend class QxtWebRedirectEvent;
    QxtWebPageEvent(QxtWebEvent::EventType typeOverride, int sessionID, int requestID, QByteArray source);
};

class QXT_WEB_EXPORT QxtWebErrorEvent : public QxtWebPageEvent
{
public:
    QxtWebErrorEvent(int sessionID, int requestID, int status, QByteArray statusMessage);
};

class QXT_WEB_EXPORT QxtWebStoreCookieEvent : public QxtWebEvent
{
public:
    QxtWebStoreCookieEvent(int sessionID, QString name, QString data, QDateTime expiration = QDateTime());

    QString name;
    QString data;
    QDateTime expiration;
};

class QXT_WEB_EXPORT QxtWebRemoveCookieEvent : public QxtWebEvent
{
public:
    QxtWebRemoveCookieEvent(int sessionID, QString name);

    QString name;
};

class QXT_WEB_EXPORT QxtWebRedirectEvent : public QxtWebPageEvent
{
public:
    QxtWebRedirectEvent(int sessionID, int requestID, const QString& destination, int statusCode = 302);

    QString destination;
};

#endif // QXTWEBEVENT_H
