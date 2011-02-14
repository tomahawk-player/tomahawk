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
#include "qtweetusersearch.h"
#include "qtweetuser.h"
#include "qtweetconvert.h"

QTweetUserSearch::QTweetUserSearch(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetUserSearch::QTweetUserSearch(OAuthTwitter *oauthTwitter, QObject *parent) :
    QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Starts a search
 *   @param query the search query to run against people search
 *   @param perPage the number of people per page to retrieve. Maxiumum of 20 allowed per page.
 *   @param page specifies the page of results to retrieve
 *   @param includeEntities when set to true each tweet will include a node called "entities".
 */
void QTweetUserSearch::search(const QString &query,
                              int perPage,
                              int page,
                              bool includeEntities)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url("http://api.twitter.com/1/users/search.json");

    url.addQueryItem("q", query);

    if (perPage)
        url.addQueryItem("per_page", QString::number(perPage));

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

void QTweetUserSearch::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QList<QTweetUser> userInfoList = QTweetConvert::variantToUserInfoList(json);

        emit parsedUserInfoList(userInfoList);
    } else {
        qDebug() << "QTweetUserSearch json parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

