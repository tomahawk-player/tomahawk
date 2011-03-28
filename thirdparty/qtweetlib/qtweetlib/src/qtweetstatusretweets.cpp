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

#include <QNetworkReply>
#include <QNetworkRequest>
#include "qtweetstatusretweets.h"

QTweetStatusRetweets::QTweetStatusRetweets(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetStatusRetweets::QTweetStatusRetweets(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Start fetching
 *   @param id tweet ID
 *   @param count numbers of retweets to fetch
 */
void QTweetStatusRetweets::fetch(qint64 id, int count)
{
    //Q_ASSERT(oauthTwitter() != 0);

    QUrl url("http://api.twitter.com/1/statuses/retweets.json");

    url.addQueryItem("id", QString::number(id));

    if (count != 0)
        url.addQueryItem("count", QString::number(count));

    // ### TODO: Add trim_user parameter
    // ### TODO: Add include_entities parameter

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

// ### TODO json parsing, emiting signal

