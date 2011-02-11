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
#include "qtweetgeoreversegeocode.h"
#include "qtweetconvert.h"

/**
 *  Constructor
 */
QTweetGeoReverseGeoCode::QTweetGeoReverseGeoCode(QObject *parent) :
    QTweetNetBase(parent)
{
}

/**
 *  Constructor
 *  @param oauthTwitter OAuthTwitter object
 *  @param parent parent QObject
 */
QTweetGeoReverseGeoCode::QTweetGeoReverseGeoCode(OAuthTwitter *oauthTwitter, QObject *parent) :
    QTweetNetBase(oauthTwitter, parent)
{
}

/**
 *  Start geo reversing
 *  @param latLong latitutde and longitude
 *  @param accuracy a hint on the "region" in which to search in meters
 *  @param granularity minimal granularity of place types to return
 *  @param maxResults hint as to the number of results to return
 */
void QTweetGeoReverseGeoCode::getPlaces(const QTweetGeoCoord& latLong,
                                        int accuracy,
                                        QTweetPlace::Type granularity,
                                        int maxResults)
{
    QUrl url("http://api.twitter.com/1/geo/reverse_geocode.json");

    url.addQueryItem("lat", QString::number(latLong.latitude()));
    url.addQueryItem("long", QString::number(latLong.longitude()));

    if (accuracy != 0)
        url.addQueryItem("accuracy", QString::number(accuracy));

    switch (granularity) {
    case QTweetPlace::Poi:
        url.addQueryItem("granularity", "poi");
        break;
    case QTweetPlace::Neighborhood:
        url.addQueryItem("granularity", "neighborhood");
        break;
    case QTweetPlace::City:
        url.addQueryItem("granularity", "city");
        break;
    case QTweetPlace::Admin:
        url.addQueryItem("granularity", "admin");
        break;
    case QTweetPlace::Country:
        url.addQueryItem("granularity", "country");
        break;
    default:
        ;
    }

    if (maxResults != 0)
        url.addQueryItem("max_results", QString::number(maxResults));

    QNetworkRequest req(url);

    if (isAuthenticationEnabled()) {
        QByteArray oauthHeader = oauthTwitter()->generateAuthorizationHeader(url, OAuth::GET);
        req.setRawHeader(AUTH_HEADER, oauthHeader);
    }

    QNetworkReply *reply = oauthTwitter()->networkAccessManager()->get(req);
    connect(reply, SIGNAL(finished()), this, SLOT(reply()));
}

void QTweetGeoReverseGeoCode::parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg)
{
    if (ok) {
        QList<QTweetPlace> places = QTweetConvert::variantToPlaceList(json);

        emit parsedPlaces(places);
    } else {
        qDebug() << "QTweetGeoReverseGeoCode parser error: " << errorMsg;
        setLastErrorMessage(errorMsg);
        emit error(JsonParsingError, errorMsg);
    }
}
