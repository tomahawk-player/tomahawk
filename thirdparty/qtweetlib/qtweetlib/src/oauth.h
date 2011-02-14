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

#ifndef OAUTH_H
#define OAUTH_H

#include <QObject>
#include <QUrl>
#include "qtweetlib_global.h"

class QByteArray;

/**
 * Base OAuth class
 */
class QTWEETLIBSHARED_EXPORT OAuth : public QObject
{
	Q_OBJECT
    Q_ENUMS(HttpMethod)
    Q_PROPERTY(QByteArray oauthToken READ oauthToken WRITE setOAuthToken)
    Q_PROPERTY(QByteArray oauthTokenSecret READ oauthTokenSecret WRITE setOAuthTokenSecret)

public:
	OAuth(QObject *parent = 0);
    OAuth(const QByteArray& consumerKey, const QByteArray& consumerSecret, QObject *parent = 0);
	
	enum HttpMethod {GET, POST, PUT, DELETE};

	void parseTokens(const QByteArray& response);
	QByteArray generateAuthorizationHeader(const QUrl& url, HttpMethod method);
	void setOAuthToken(const QByteArray& token);
	void setOAuthTokenSecret(const QByteArray& tokenSecret);
    void clearTokens();
	QByteArray oauthToken() const;
	QByteArray oauthTokenSecret() const;
	
private:
	QByteArray generateSignatureHMACSHA1(const QByteArray& signatureBase);
	QByteArray generateSignatureBase(const QUrl& url, HttpMethod method, const QByteArray& timestamp, const QByteArray& nonce);

	QByteArray m_oauthToken;
    QByteArray m_oauthTokenSecret;
    QByteArray m_oauthConsumerSecret;
    QByteArray m_oauthConsumerKey;
};

#endif //OAUTH_H
