/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#ifndef QTWEETNETBASE_H
#define QTWEETNETBASE_H

#include <QObject>
#include <QVariantMap>
#include <QByteArray>
#include "oauthtwitter.h"
#include "qtweetlib_global.h"

class QTweetStatus;
class QTweetUser;
class QTweetDMStatus;
class QTweetList;
class QTweetSearchResult;
class QTweetSearchPageResults;
class QTweetPlace;

/**
 *   Base class for Twitter API classes
 */
class QTWEETLIBSHARED_EXPORT QTweetNetBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(OAuthTwitter* oauthTwitter READ oauthTwitter WRITE setOAuthTwitter)
    Q_PROPERTY(bool jsonParsing READ isJsonParsingEnabled WRITE setJsonParsingEnabled)
    Q_PROPERTY(bool authenticaion READ isAuthenticationEnabled WRITE setAuthenticationEnabled)
public: 
    QTweetNetBase(QObject *parent = 0);
    QTweetNetBase(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    virtual ~QTweetNetBase();

    enum ErrorCode {
        JsonParsingError = 1,       /** JSON parsing error */
        UnknownError = 2,           /** Unknown error */
        NotModified = 304,          /** There was no new data to return. */
        BadRequest = 400,           /** The request was invalid. This is the status code will be returned during rate limiting. */
        Unauthorized = 401,         /** Authentication credentials were missing or incorrect. */
        Forbidden = 403,            /** The request is understood, but it has been refused, due to update limits. */
        NotFound = 404,             /** The URI requested is invalid or the resource requested, such as a user, does not exists. */
        NotAcceptable = 406,        /** Returned by the Search API when an invalid format is specified in the request. */
        EnhanceYourCalm = 420,      /** Returned by the Search and Trends API when you are being rate limited. */
        InternalServerError = 500,  /** Something is broken in Twitter */
        BadGateway = 502,           /** Twitter is down or being upgraded. */
        ServiceUnavailable = 503    /** The Twitter servers are up, but overloaded with requests. Try again later. */
    };

    void setOAuthTwitter(OAuthTwitter* oauthTwitter);
    OAuthTwitter* oauthTwitter() const;

    void setJsonParsingEnabled(bool enable);
    bool isJsonParsingEnabled() const;

    void setAuthenticationEnabled(bool enable);
    bool isAuthenticationEnabled() const;

    QByteArray response() const;
    QString lastErrorMessage() const;

signals:
    /**
     *   Emited when fetching is finished
     *   @param response contains the response in json
     */
    void finished(const QByteArray& response);

    /** Emited when there is error. You can check error message with lastErrorMessage().
     *  @param code error code
     *  @param errorMsg error message. If it's empty then error was not standard json twitter api message.
     *                   In that case check response.
     *  @remarks DOESN'T emit finished signal
     */
    void error(QTweetNetBase::ErrorCode code, const QString& errorMsg);

protected slots:
    virtual void parsingJsonFinished(const QVariant& json, bool ok, const QString& errorMsg) = 0;
    virtual void reply();

protected:
    void parseJson(const QByteArray& jsonData);
    void setLastErrorMessage(const QString& errMsg);

private:
    OAuthTwitter *m_oauthTwitter;
    QByteArray m_response;
    QString m_lastErrorMessage;
    bool m_jsonParsingEnabled;
    bool m_authentication;
};

#endif // QTWEETNETBASE_H
