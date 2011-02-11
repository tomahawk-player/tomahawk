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

#include "qtweetgeocoord.h"

QTweetGeoCoord::QTweetGeoCoord()
    : m_validLatitude(false), m_validLongitude(false)
{
}

QTweetGeoCoord::QTweetGeoCoord(double latitude, double longitude)
{
    setLatitude(latitude);
    setLongitude(longitude);
}

bool QTweetGeoCoord::isValid() const
{
    return m_validLatitude & m_validLongitude;
}

void QTweetGeoCoord::setLatitude(double latitude)
{
    m_latitude = latitude;

    if (m_latitude > -90.0 || m_latitude < 90.0)
        m_validLatitude = true;
    else
        m_validLatitude = false;
}

void QTweetGeoCoord::setLongitude(double longitude)
{
    m_longitude = longitude;

    if (m_longitude > -180.0 || m_longitude < 180.0)
        m_validLongitude = true;
    else
        m_validLongitude = false;
}
