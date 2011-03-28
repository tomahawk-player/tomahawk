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

#include "oauthtwitter.h"
#include <QtDebug>
#include <QUrl>
#include <QNetworkReply>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QDesktopServices>

#define TWITTER_REQUEST_TOKEN_URL "http://twitter.com/oauth/request_token"
#define TWITTER_ACCESS_TOKEN_URL "http://twitter.com/oauth/access_token"
#define TWITTER_AUTHORIZE_URL "http://twitter.com/oauth/authorize"
#define TWITTER_ACCESS_TOKEN_XAUTH_URL "https://api.twitter.com/oauth/access_token"

/**
 *   Constructor
 */
OAuthTwitter::OAuthTwitter(QObject *parent)
    :	OAuth(parent), m_netManager(0)
{
}

/**
 *  Constructor
 */
OAuthTwitter::OAuthTwitter(QNetworkAccessManager *netManager, QObject *parent) :
    OAuth(parent), m_netManager(netManager)
{
}

/**
 *   Sets network access manager
 *   @remarks Must be set to work properly
 */
void OAuthTwitter::setNetworkAccessManager(QNetworkAccessManager* netManager)
{
	m_netManager = netManager;
}

/**
 *   Gets network access manager
 */
QNetworkAccessManager* OAuthTwitter::networkAccessManager() const
{
	return m_netManager;
}

/**
 *   Gets oauth tokens using XAuth method (starts authorization process)
 *   @param username username
 *   @param password password
 *   @remarks Async, emits authorizeXAuthFinished or authorizeXAuthError when there is error
 */
void OAuthTwitter::authorizeXAuth(const QString &username, const QString &password)
{
    Q_ASSERT(m_netManager != 0);

    QUrl url(TWITTER_ACCESS_TOKEN_XAUTH_URL);
    url.addQueryItem("x_auth_username", username);
    url.addQueryItem("x_auth_password", password);
    url.addQueryItem("x_auth_mode", "client_auth");

    QByteArray oauthHeader = generateAuthorizationHeader(url, OAuth::POST);

    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = m_netManager->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), this, SLOT(finishedAuthorization()));
}

/**
 * Called when authorization is finished
 */
void OAuthTwitter::finishedAuthorization()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            parseTokens(response);

            emit authorizeXAuthFinished();
        } else {
            //dump error
            qDebug() << "Network Error: " << reply->error();
            qDebug() << "Response error: " << reply->readAll();
            emit authorizeXAuthError();

        }
        reply->deleteLater();
    }
}

/**
 *  Starts PIN based OAuth authorization
 */
void OAuthTwitter::authorizePin()
{
    Q_ASSERT(m_netManager != 0);

    QUrl url(TWITTER_REQUEST_TOKEN_URL);

    QByteArray oauthHeader = generateAuthorizationHeader(url, OAuth::POST);

    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    //enters event loop
    QEventLoop q;
    QTimer t;
    t.setSingleShot(true);
    connect(&t, SIGNAL(timeout()), &q, SLOT(quit()));

    QNetworkReply *reply = m_netManager->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), &q, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error()));

    t.start(5000);
    q.exec();

    if (t.isActive()) {
        t.stop();
        QByteArray response = reply->readAll();
        parseTokens(response);

        reply->deleteLater();
        requestAuthorization();

        int pin = authorizationWidget();
        if (pin) {
            requestAccessToken(pin);
        }
    } else {
        qDebug() << "Timeout";
    }
}

/**
 *  Opens authorization url
 *  @remarks Override if you want to show another browser
 */
void OAuthTwitter::requestAuthorization()
{
    QUrl authorizeUrl(TWITTER_AUTHORIZE_URL);
    authorizeUrl.addEncodedQueryItem("oauth_token", oauthToken());

    QDesktopServices::openUrl(authorizeUrl);
}

/**
 *  Gets access tokens for user entered pin number
 *  @param pin entered pin number
 */
void OAuthTwitter::requestAccessToken(int pin)
{
    Q_ASSERT(m_netManager != 0);

    QUrl url(TWITTER_ACCESS_TOKEN_URL);
    url.addEncodedQueryItem("oauth_verifier", QByteArray::number(pin));

    QByteArray oauthHeader = generateAuthorizationHeader(url, OAuth::POST);

    QEventLoop q;
    QTimer t;
    t.setSingleShot(true);

    connect(&t, SIGNAL(timeout()), &q, SLOT(quit()));

    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = m_netManager->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), &q, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error()));

    t.start(5000);
    q.exec();

    if(t.isActive()){
        QByteArray response = reply->readAll();
        parseTokens(response);
        reply->deleteLater();
    } else {
        qDebug() << "Timeout";
    }
}

/**
 *  Override to show the authorization widget where users enters pin number
 *  @return entered pin number by the user
 */
int OAuthTwitter::authorizationWidget()
{
    return 0;
}
