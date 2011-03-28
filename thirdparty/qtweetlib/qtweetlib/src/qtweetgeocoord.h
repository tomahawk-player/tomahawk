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

#ifndef QTWEETGEOCOORD_H
#define QTWEETGEOCOORD_H

#include "qtweetlib_global.h"

/**
 *  Stores latitude/longitude coordinates
 *  Does basic checking if latitude/longitude is in valid degrees
 */
class QTWEETLIBSHARED_EXPORT QTweetGeoCoord
{
public:
    QTweetGeoCoord();
    QTweetGeoCoord(double latitude, double longitude);
    bool isValid() const;
    double latitude() const { return m_latitude; }
    double longitude() const { return m_longitude; }
    void setLatitude(double latitude);
    void setLongitude(double longitude);

private:
    double m_latitude;
    double m_longitude;
    bool m_validLatitude;
    bool m_validLongitude;
};

#endif // QTWEETGEOCOORD_H
