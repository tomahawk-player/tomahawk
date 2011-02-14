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

#include "qtweetgeoboundingbox.h"

QTweetGeoBoundingBox::QTweetGeoBoundingBox()
{
}

QTweetGeoBoundingBox::QTweetGeoBoundingBox(const QTweetGeoCoord &topLeft,
                                           const QTweetGeoCoord &topRight,
                                           const QTweetGeoCoord &bottomRight,
                                           const QTweetGeoCoord &bottomLeft)
{
    setTopLeft(topLeft);
    setTopRight(topRight);
    setBottomRight(bottomRight);
    setBottomLeft(bottomLeft);
}

bool QTweetGeoBoundingBox::isValid() const
{
    return m_topLeft.isValid() & m_topRight.isValid() &
            m_bottomRight.isValid() & m_bottomLeft.isValid();
}
