/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#include <QtDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "qtweetdirectmessagessent.h"
#include "qtweetdmstatus.h"
#include "qtweetconvert.h"

/**
 *  Constructor
 */
QTweetDirectMessagesSent::QTweetDirectMessagesSent(QObject *parent) :
        QTweetNetBase(parent)
{
}

/**
 *  Constructor
 *  @param oauthTwitter OAuthTwitter object
 *  @param parent parent QObject
 */
QTweetDirectMessagesSent::QTweetDirectMessagesSent(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Starts fetching direct messages
 *   @param sinceid returns results with an ID greater than (that is, more recent than) the specified ID.
 *   @param maxid returns results with an ID less than (that is, older than) or equal to the specified ID.
 *   @param count specifies the number of records to retrieve. Must be less than or equal to 200.
 *   @param page specifies the page of results to retrieve.
 *   @param includeEntities When set to true, each tweet will include a node called "entities,".
 *   @remarks Setting parameters to default value will not be put in the query
 */
void QTweetDirectMessagesSent::fetch(qint64 sinceid,
                                     qint64 maxid,
                                     int count,
                                     int page,
                                     bool includeEntities)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url("http://api.twitter.com/1/direct_messages/sent.json");

    if (sinceid)
        url.addQueryItem("since_id", QString::number(sinceid));

    if (maxid)
        url.addQueryItem("max_id", QString::number(maxid));

    if (count)
        url.addQueryItem("count", QString::number(count));

    if (page)
        url.addQueryItem("page", QString::number(page));

    if (includeEntities)
        url.addQueryItem("include_entities", "true");

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetDirectMessagesSent::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QList<QTweetDMStatus> directMessages = QTweetConvert::variantToDirectMessagesList(json);

        emit parsedDirectMessages(directMessages);
    } else {
        qDebug() << "QTweetDirectMessagesSent parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

