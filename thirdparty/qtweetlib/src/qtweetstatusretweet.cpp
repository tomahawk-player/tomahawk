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
#include <QNetworkReply>
#include <QNetworkRequest>
#include "qtweetstatusretweet.h"
#include "qtweetstatus.h"
#include "qtweetconvert.h"

QTweetStatusRetweet::QTweetStatusRetweet(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetStatusRetweet::QTweetStatusRetweet(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Retweets the tweet
 *   @param id tweet ID to retweet
 *   @param trimUser trims user info in response
 *   @param includeEntities true to include node entities in response
 */
void QTweetStatusRetweet::retweet(qint64 id,
                                  bool trimUser,
                                  bool includeEntities)
{
    QUrl url(QString("http://api.twitter.com/1/statuses/retweet/%1.json").arg(id));

    if (trimUser)
        url.addQueryItem("trim_user", "true");

    if (includeEntities)
        url.addQueryItem("include_entities", "true");

    QNetworkRequest req(url);

    if (isAuthenticationEnabled()) {    //Oh, Twitter API docs wrong?
        QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::POST);
        req.setRawHeader(AUTH_HEADER, oauthHeader);
    }

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetStatusRetweet::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QTweetStatus status = QTweetConvert::variantMapToStatus(json.toMap());

        emit postedRetweet(status);
    } else {
        qDebug() << "QTweetStatusRetweet JSON parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

