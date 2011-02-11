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

#ifndef QTWEETGEOREVERSEGEOCODE_H
#define QTWEETGEOREVERSEGEOCODE_H

#include "qtweetnetbase.h"
#include "qtweetplace.h"

/**
 *  Given a latitude and a longitude, searches up to 20 places that can be used
 *  as placeId when updating a status
 */
class QTWEETLIBSHARED_EXPORT QTweetGeoReverseGeoCode : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetGeoReverseGeoCode(QObject *parent = 0);
    QTweetGeoReverseGeoCode(OAuthTwitter *oauthTwitter, QObject *parent = 0);

    void getPlaces(const QTweetGeoCoord& latLong,
                   int accuracy = 0,
                   QTweetPlace::Type granularity = QTweetPlace::Neighborhood,
                   int maxResults = 0);

signals:
    /** Emits list of places */
    void parsedPlaces(const QList<QTweetPlace>& places);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETGEOREVERSEGEOCODE_H
