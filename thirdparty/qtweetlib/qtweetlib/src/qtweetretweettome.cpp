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
#include "qtweetretweettome.h"
#include "qtweetstatus.h"
#include "qtweetconvert.h"

QTweetRetweetToMe::QTweetRetweetToMe(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetRetweetToMe::QTweetRetweetToMe(OAuthTwitter *oauthTwitter, QObject *parent) :
    QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Starts fetching retweets to user
 *   @param sinceid Fetches tweets with ID greater (more recent) then sinceid
 *   @param maxid Fetches tweets with ID less (older) then maxid
 *   @param count Number of tweets to fetch (up to 200)
 *   @param page Page number
 *   @remarks Setting parameter to default value, will not be put in the query
 */
void QTweetRetweetToMe::fetch(qint64 sinceid,
                              qint64 maxid,
                              int count,
                              int page,
                              bool trimUser,
                              bool includeEntities)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url("http://api.twitter.com/1/statuses/retweeted_to_me.json");

    if (sinceid != 0)
        url.addQueryItem("since_id", QString::number(sinceid));

    if (maxid != 0)
        url.addQueryItem("max_id", QString::number(maxid));

    if (count != 0)
        url.addQueryItem("count", QString::number(count));

    if (page != 0)
        url.addQueryItem("page", QString::number(page));

    if (trimUser)
        url.addQueryItem("trim_user", "true");

    if (includeEntities)
        url.addQueryItem("include_entities", "true");

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetRetweetToMe::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QList<QTweetStatus> statuses = QTweetConvert::variantToStatusList(json);

        emit parsedStatuses(statuses);
    } else {
        qDebug() << "QTweetRetweetToMe JSON parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

