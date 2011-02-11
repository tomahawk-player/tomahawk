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

#ifndef QTWEETGEOBOUNDINGBOX_H
#define QTWEETGEOBOUNDINGBOX_H

#include "qtweetlib_global.h"
#include "qtweetgeocoord.h"

/**
 *  Stores geo bounding box
 *  Doesnt' do anything fancy/calculations just stores boundind box info from twitter api
 */
class QTWEETLIBSHARED_EXPORT QTweetGeoBoundingBox
{
public:
    QTweetGeoBoundingBox();
    QTweetGeoBoundingBox(const QTweetGeoCoord& topLeft,
                         const QTweetGeoCoord& topRight,
                         const QTweetGeoCoord& bottomRight,
                         const QTweetGeoCoord& bottomLeft);
    QTweetGeoCoord topLeft() const { return m_topLeft; }
    QTweetGeoCoord topRight() const { return m_topRight; }
    QTweetGeoCoord bottomRight() const { return m_bottomRight; }
    QTweetGeoCoord bottomLeft() const { return m_bottomLeft; }
    void setTopLeft(const QTweetGeoCoord& topLeft) { m_topLeft = topLeft; }
    void setTopRight(const QTweetGeoCoord& topRight) { m_topRight = topRight; }
    void setBottomRight(const QTweetGeoCoord& bottomRight) { m_bottomRight = bottomRight; }
    void setBottomLeft(const QTweetGeoCoord& bottomLeft) { m_bottomLeft = bottomLeft; }
    bool isValid() const;

private:
    QTweetGeoCoord m_topLeft;
    QTweetGeoCoord m_topRight;
    QTweetGeoCoord m_bottomRight;
    QTweetGeoCoord m_bottomLeft;
};

#endif // QTWEETGEOBOUNDINGBOX_H
