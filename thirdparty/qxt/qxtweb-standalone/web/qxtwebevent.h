
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
#include <QHostAddress>
#ifndef QT_NO_OPENSSL
#include <QSslCertificate>
#endif
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
    virtual ~QxtWebRequestEvent();

    const int requestID;

    QUrl url;
    const QUrl originalUrl;
    QString contentType;
    QPointer<QxtWebContent> content;
    QString method;
    QHostAddress remoteAddress;
    bool isSecure;
#ifndef QT_NO_OPENSSL
    QSslCertificate clientCertificate;
#endif

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
    QxtWebPageEvent(int sessionID, int requestID, QIODevice* source = 0);
    QxtWebPageEvent(int sessionID, int requestID, QByteArray source);    // creates a QBuffer
    virtual ~QxtWebPageEvent();

    QPointer<QIODevice> dataSource;          // data is read from this device and written to the client
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
    QString path;
};

class QXT_WEB_EXPORT QxtWebRemoveCookieEvent : public QxtWebEvent
{
public:
    QxtWebRemoveCookieEvent(int sessionID, QString name);

    QString name;
    QString path;
};

class QXT_WEB_EXPORT QxtWebRedirectEvent : public QxtWebPageEvent
{
public:
    QxtWebRedirectEvent(int sessionID, int requestID, const QString& destination, int statusCode = 302);

    QString destination;
};

#endif // QXTWEBEVENT_H
