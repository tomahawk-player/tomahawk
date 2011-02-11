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
#include "qtweetsearch.h"
#include "qtweetsearchpageresults.h"
#include "qtweetconvert.h"

QTweetSearch::QTweetSearch(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetSearch::QTweetSearch(OAuthTwitter *oauthTwitter, QObject *parent) :
    QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *  Starts searching
 *  @param query serch query
 *  @param lang Restricts tweets to the given language, given by an ISO 639-1 code.
 *  @param rpp number of tweets to return per page, up to a max of 100.
 *  @param page page number (starting at 1) to return, up to a max of roughly 1500 results
 *  @param sinceid returns results with an ID greater than (that is, more recent than) the specified ID.
 */
void QTweetSearch::start(const QString &query,
                         const QString &lang,
                         /* const QString &locale, */
                         int rpp,
                         int page,
                         qint64 sinceid)
{
    QUrl url("http://search.twitter.com/search.json");

    url.addEncodedQueryItem("q", QUrl::toPercentEncoding(query));

    if (!lang.isEmpty())
        url.addQueryItem("lang", lang);

    // if (!locale.isEmpty())
    //     url.addQueryItem("locale", locale);

    if (rpp)
        url.addQueryItem("rpp", QString::number(rpp));

    if (page)
        url.addQueryItem("page", QString::number(page));

    if (sinceid)
        url.addQueryItem("since_id", QString::number(sinceid));

    QNetworkRequest req(url);

    if (isAuthenticationEnabled()) {
        QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
        req.setRawHeader(AUTH_HEADER, oauthHeader);
    }

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetSearch::startWithCustomQuery(const QByteArray &encodedQuery)
{
    QUrl url("http://search.twitter.com/search.json");

    //remove ?
    QByteArray query(encodedQuery);
    url.setEncodedQuery(query.remove(0, 1));

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetSearch::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QTweetSearchPageResults pageResults = QTweetConvert::variantToSearchPageResults(json);

        emit parsedPageResults(pageResults);
    } else {
        qDebug() << "QTweetSearch parsing error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}
