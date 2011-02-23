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

#ifndef QTWEETGEOPLACECREATE_H
#define QTWEETGEOPLACECREATE_H

#include "qtweetnetbase.h"

class QTweetPlace;
class QTweetGeoCoord;

/**
 *  Creates a new place at the given latitude and longitude
 */
class QTWEETLIBSHARED_EXPORT QTweetGeoPlaceCreate : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetGeoPlaceCreate(QObject *parent = 0);
    QTweetGeoPlaceCreate(OAuthTwitter *oauthTwitter, QObject *parent = 0);

    void create(const QString& name,
                const QString& containedWithin,
                const QString& token,
                const QTweetGeoCoord& latLong);

signals:
    /** Emits geo place */
    void parsedPlace(const QTweetPlace& place);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETGEOPLACECREATE_H
