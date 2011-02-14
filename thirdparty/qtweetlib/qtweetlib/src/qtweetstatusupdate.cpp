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
#include <QNetworkReply>
#include "qtweetstatusupdate.h"
#include "qtweetstatus.h"
#include "qtweetgeocoord.h"
#include "qtweetconvert.h"

QTweetStatusUpdate::QTweetStatusUpdate(QObject *parent) :
    QTweetNetBase(parent)
{
}

QTweetStatusUpdate::QTweetStatusUpdate(OAuthTwitter *oauthTwitter, QObject *parent) :
        QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *   Posts a tweet
 *   @param status text of the status update
 *   @param inReplyToStatus ID of a existing tweet is in reply to
 *   @param latLong latitude and longitude
 *   @param placeid a place in the world (use reverse geocoding)
 *   @param displayCoordinates whether or not to put a exact coordinates a tweet has been sent from
 */
void QTweetStatusUpdate::post(const QString &status,
                              qint64 inReplyToStatus,
                              const QTweetGeoCoord& latLong,
                              const QString &placeid,
                              bool displayCoordinates,
                              bool trimUser,
                              bool includeEntities)
{
    if (!isAuthenticationEnabled()) {
        qCritical("Needs authentication to be enabled");
        return;
    }

    QUrl url("http://api.twitter.com/1/statuses/update.json");

    QUrl urlQuery("http://api.twitter.com/1/statuses/update.json");

    urlQuery.addEncodedQueryItem("status", QUrl::toPercentEncoding(status));

    if (inReplyToStatus != 0)
        urlQuery.addQueryItem("in_reply_to_status_id", QString::number(inReplyToStatus));

    if (latLong.isValid()) {
        urlQuery.addQueryItem("lat", QString::number(latLong.latitude()));
        urlQuery.addQueryItem("long", QString::number(latLong.longitude()));
    }

    if (!placeid.isEmpty())
        urlQuery.addQueryItem("place_id", placeid);

    if (displayCoordinates)
        urlQuery.addQueryItem("display_coordinates", "true");

    if (trimUser)
        urlQuery.addQueryItem("trim_user", "true");

    if (includeEntities)
        urlQuery.addQueryItem("include_entities", "true");

    QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(urlQuery, OAuth::POST);
    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    //build status post array
    QByteArray statusPost = urlQuery.toEncoded(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemovePath);

    //remove '?'
    statusPost.remove(0, 1);

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->post(req, statusPost);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetStatusUpdate::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QTweetStatus status = QTweetConvert::variantMapToStatus(json.toMap());

        emit postedStatus(status);
    } else {
        qDebug() << "QTweetStatusUpdate JSON parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}

