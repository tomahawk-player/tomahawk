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
#include <QNetworkReply>
#include <QNetworkRequest>
#include "qtweetlistmemberships.h"
#include "qtweetlist.h"
#include "qtweetconvert.h"

QTweetListMemberships::QTweetListMemberships(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetListMemberships::QTweetListMemberships(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   @param id user ID
 *   @param cursor breaks the results into pages. A single page contains 20 lists.
 *                 Provide a value of -1 to begin paging.
 *                 Provide values as returned next_cursor and previous_cursor attributes to page back and forth
 */
void QTweetListMemberships::fetch(qint64 id, const QString &cursor)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url(QString("http://api.twitter.com/1/%1/lists/memberships.json").arg(id));

    if (!cursor.isEmpty())
        url.addQueryItem("cursor", cursor);

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetListMemberships::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QVariantMap respMap = json.toMap();

        QVariant listsVar = respMap["lists"];

        QList<QTweetList> lists = QTweetConvert::variantToTweetLists(listsVar);

        QString nextCursor = respMap["next_cursor_str"].toString();
        QString prevCursor = respMap["previous_cursor_str"].toString();

        emit parsedList(lists, nextCursor, prevCursor);
    } else {
        qDebug() << "QTweetListMemberships json parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

