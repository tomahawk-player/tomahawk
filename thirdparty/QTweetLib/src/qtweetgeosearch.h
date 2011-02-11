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

#ifndef QTWEETGEOSEARCH_H
#define QTWEETGEOSEARCH_H

#include "qtweetplace.h"
#include "qtweetnetbase.h"


/**
 *  Search for places that can be attached to a statuses/update
 *  @see http://dev.twitter.com/doc/get/geo/search
 */
class QTWEETLIBSHARED_EXPORT QTweetGeoSearch : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetGeoSearch(QObject *parent = 0);
    QTweetGeoSearch(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void search(const QTweetGeoCoord& latLong = QTweetGeoCoord(),
                const QString& query = QString(),
                const QString& ip = QString(),
                QTweetPlace::Type granularity = QTweetPlace::Neighborhood,
                int accuracy = 0,
                int maxResults = 0,
                const QString& containedWithin = QString());
                // ### TODO Atributes, not enough documentation

signals:
    /** Emits list of places */
    void parsedPlaces(const QList<QTweetPlace>& places);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETGEOSEARCH_H
