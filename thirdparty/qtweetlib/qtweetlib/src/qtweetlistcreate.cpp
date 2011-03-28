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
#include <QThreadPool>
#include "qjson/parserrunnable.h"
#include "qtweetlistcreate.h"
#include "qtweetlist.h"
#include "qtweetconvert.h"

QTweetListCreate::QTweetListCreate(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetListCreate::QTweetListCreate(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/** Creates list
 *  @param user user id
 *  @param name the name of the list
 *  @param mode true for public list, false for private list
 *  @param description the description to give the list.
 */
void QTweetListCreate::create(qint64 user,
                              const QString &name,
                              bool mode,
                              const QString &description)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url(QString("http://api.twitter.com/1/%1/lists.json").arg(user));

    QUrl urlQuery(url);

    urlQuery.addEncodedQueryItem("name", QUrl::toPercentEncoding(name));

    if (!mode)
        urlQuery.addQueryItem("mode", "private");

    if (!description.isEmpty())
        urlQuery.addEncodedQueryItem("description", QUrl::toPercentEncoding(description));

    QNetworkRequest req(url);

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(urlQuery, OAuth::POST);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QByteArray postBody = urlQuery.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemovePath);
    postBody.remove(0, 1);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->post(req, postBody);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetListCreate::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QTweetList list = QTweetConvert::variantMapToTweetList(json.toMap());

        emit parsedList(list);
    } else {
        qDebug() << "QTweetListCreate json parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}
