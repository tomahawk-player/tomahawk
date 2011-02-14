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

#ifndef QTWEETGEOSIMILARPLACES_H
#define QTWEETGEOSIMILARPLACES_H

#include "qtweetnetbase.h"

class QTweetPlace;
class QTweetGeoCoord;

/**
 *  Locates places near the given coordinates which are similar in name
 *  @see http://dev.twitter.com/doc/get/geo/similar_places
 */
class QTWEETLIBSHARED_EXPORT QTweetGeoSimilarPlaces : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetGeoSimilarPlaces(QObject *parent = 0);
    QTweetGeoSimilarPlaces(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void get(const QTweetGeoCoord& latLong,
             const QString& name,
             const QString& containedWithin = QString());
    // ### TODO: Atributes, lack of documentation

signals:
    /** Emits places
      * @param places list of places
      * @param token token to create a new place
      */
    void parsedPlaces(const QList<QTweetPlace>& places, const QString& token);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETGEOSIMILARPLACES_H
