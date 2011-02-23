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

#include <QtDebug>
#include <QNetworkRequest>
#include <QAuthenticator>
#include <QTimer>
#include <QThreadPool>
#include "oauthtwitter.h"
#include "qtweetuserstream.h"
#include "qtweetstatus.h"
#include "qtweetdmstatus.h"
#include "qtweetuser.h"
#include "qtweetconvert.h"
#include "qjson/parserrunnable.h"

#define TWITTER_USERSTREAM_URL "https://userstream.twitter.com/2/user.json"

/**
 *  Constructor
 */
QTweetUserStream::QTweetUserStream(QObject *parent) :
    QObject(parent), m_oauthTwitter(0), m_reply(0), m_backofftimer(new QTimer(this))
{
    m_backofftimer->setSingleShot(true);
    connect(m_backofftimer, SIGNAL(timeout()), this, SLOT(startFetching()));
}

/**
 *  Sets oauth twitter object
 */
void QTweetUserStream::setOAuthTwitter(OAuthTwitter *oauthTwitter)
{
    m_oauthTwitter = oauthTwitter;
}

/**
 *  Gets oauth twitter object
 */
OAuthTwitter* QTweetUserStream::oauthTwitter() const
{
    return m_oauthTwitter;
}

/**
 *  Called when there is network error
 */
void QTweetUserStream::replyError(QNetworkReply::NetworkError code)
{
    qDebug() << "Reply error: " << code;

    // ### TODO: determine network error codes, assumptions here

    if (code < 200) {
        //linear backoff
        if (m_backofftimer->interval() < 250) {
            m_backofftimer->setInterval(250);
        } else {
            int nextLinInterval = m_backofftimer->interval() + 250;

            if (nextLinInterval > 16000)    //cap
                nextLinInterval = 16000;

            m_backofftimer->setInterval(nextLinInterval);
        }

        m_backofftimer->start();
        return;
    }

    if (code > 200) {
        //exp. backoff
        if (m_backofftimer->interval() < 10000) {
            m_backofftimer->setInterval(10000);
        } else {
            int nextExpInterval = 2 * m_backofftimer->interval();

            if (nextExpInterval > 240000)
                nextExpInterval = 240000;

            m_backofftimer->setInterval(240000);
        }

        m_backofftimer->start();
    }
}

/**
 *   Starts fetching user stream
 */
void QTweetUserStream::startFetching()
{
    if (m_reply != 0) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = 0;
    }

    QNetworkRequest req;
    req.setUrl(QUrl(TWITTER_USERSTREAM_URL));

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(req.url(), OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    m_reply = m_oauthTwitter->networkAccessManager()->get(req);
    connect(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(replyReadyRead()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
}

/**
 *  Called when connection is finished. Reconnects.
 */
void QTweetUserStream::replyFinished()
{
    if (!m_reply->error()) { //no error, reconnect

        //sligh delay for reconnect
        m_backofftimer->setInterval(250);
        m_backofftimer->start();
        m_reply->deleteLater();
        m_reply = 0;
    } else {    //error, delete QNetworkReply
        m_reply->deleteLater();
        m_reply = 0;
    }
}

void QTweetUserStream::replyReadyRead()
{
    QByteArray response = m_reply->readAll();

    //reset timer
    m_backofftimer->setInterval(0);

    //split to response to delimited and not delimited part
    int lastCarrReturn = response.lastIndexOf('\r');
    QByteArray rightNotDelimitedPart = response.mid(lastCarrReturn + 1);
    QByteArray leftDelimitedPart = response.left(lastCarrReturn);

    //prepend to left previous not delimited response
    leftDelimitedPart = leftDelimitedPart.prepend(m_cashedResponse);

    QList<QByteArray> elements = leftDelimitedPart.split('\r');

    for (int i = 0; i < elements.size(); ++i) {
        if (elements.at(i) != QByteArray(1, '\n')) {
            emit stream(elements.at(i));
            parseStream(elements.at(i));
        }
    }

    if (rightNotDelimitedPart != QByteArray(1, '\n'))
        m_cashedResponse = rightNotDelimitedPart;
    else
        m_cashedResponse.clear();
}

void QTweetUserStream::parseStream(const QByteArray& data)
{
    QJson::ParserRunnable *jsonParser = new QJson::ParserRunnable();
    jsonParser->setData(data);
    connect(jsonParser, SIGNAL(parsingFinished(QVariant,bool,QString)),
        this, SLOT(parsingFinished(QVariant,bool,QString)));

    QThreadPool::globalInstance()->start(jsonParser);
}

void QTweetUserStream::parsingFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (!ok) {
        qDebug() << "JSON parsing error: " << errorMsg;
        return;
    }

    QVariantMap result = json.toMap();

    //find it what stream element is
    if (result.contains("friends")) {    //friends element
        parseFriendsList(result);
    } else if (result.contains("direct_message")) { //direct message element
        parseDirectMessage(result);
    } else if (result.contains("text")) {  //status element
        QTweetStatus status = QTweetConvert::variantMapToStatus(result);
        emit statusesStream(status);
    } else if (result.contains("delete")) {
        parseDeleteStatus(result);
    }
}

void QTweetUserStream::parseFriendsList(const QVariantMap& streamObject)
{
    QList<qint64> friends;

    QVariantList friendsVarList = streamObject["friends"].toList();

    foreach (const QVariant& idVar, friendsVarList)
        friends.append(idVar.toLongLong());

    emit friendsList(friends);
}

void QTweetUserStream::parseDirectMessage(const QVariantMap &streamObject)
{
    QVariantMap directMessageVarMap = streamObject["direct_message"].toMap();

    QTweetDMStatus directMessage = QTweetConvert::variantMapToDirectMessage(directMessageVarMap);

    emit directMessageStream(directMessage);
}

void QTweetUserStream::parseDeleteStatus(const QVariantMap &streamObject)
{
    QVariantMap deleteStatusVarMap = streamObject["delete"].toMap();
    QVariantMap statusVarMap = deleteStatusVarMap["status"].toMap();

    qint64 id = statusVarMap["id"].toLongLong();
    qint64 userid = statusVarMap["user_id"].toLongLong();

    emit deleteStatusStream(id, userid);
}
