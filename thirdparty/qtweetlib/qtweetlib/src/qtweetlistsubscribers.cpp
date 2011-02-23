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
#include "qtweetlistsubscribers.h"
#include "qtweetuser.h"
#include "qtweetconvert.h"

QTweetListSubscribers::QTweetListSubscribers(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetListSubscribers::QTweetListSubscribers(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   @param user user id
 *   @param list list id
 *   @param cursor breaks the results in pages. Set to "-1" to begin paging.
 *                 Use prevCursor and nextCursor to page back and forth
 *   @param includeEntities when set to true each tweet will include a node called "entities,"
 */
void QTweetListSubscribers::fetch(qint64 user,
                                  qint64 list,
                                  const QString &cursor,
                                  bool includeEntities)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url(QString("http://api.twitter.com/1/%1/%2/subscribers.json").arg(user).arg(list));

    if (!cursor.isEmpty())
        url.addQueryItem("cursor", cursor);

    if (includeEntities)
        url.addQueryItem("include_entities", "true");

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetListSubscribers::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QVariantMap respMap = json.toMap();

        QVariant userList = respMap["users"];

        QList<QTweetUser> users = QTweetConvert::variantToUserInfoList(userList);

        QString nextCursor = respMap["next_cursor_str"].toString();
        QString prevCursor = respMap["previous_cursor_str"].toString();

        emit parsedUsers(users, nextCursor, prevCursor);
    } else {
        qDebug() << "QTweetListSubscribers json parsing error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

