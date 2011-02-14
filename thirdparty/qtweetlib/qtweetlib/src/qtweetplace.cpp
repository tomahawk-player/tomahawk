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

#include "qtweetplace.h"

QTweetPlace::QTweetPlace()
{
}

void QTweetPlace::setName(const QString &name)
{
    m_name = name;
}

QString QTweetPlace::name() const
{
    return m_name;
}

void QTweetPlace::setCountry(const QString &country)
{
    m_country = country;
}

QString QTweetPlace::country() const
{
    return m_country;
}

void QTweetPlace::setCountryCode(const QString &code)
{
    m_countryCode = code;
}

QString QTweetPlace::countryCode() const
{
    return m_countryCode;
}

void QTweetPlace::setID(const QString &id)
{
    m_id = id;
}

QString QTweetPlace::id() const
{
    return m_id;
}

void QTweetPlace::setBoundingBox(const QTweetGeoBoundingBox &box)
{
    m_boundingBox = box;
}

QTweetGeoBoundingBox QTweetPlace::boundingBox() const
{
    return m_boundingBox;
}

void QTweetPlace::setContainedWithin(const QList<QTweetPlace> &places)
{
    m_containedWithin = places;
}

QList<QTweetPlace> QTweetPlace::containedWithin() const
{
    return m_containedWithin;
}

void QTweetPlace::setFullName(const QString &name)
{
    m_fullName = name;
}

QString QTweetPlace::fullName() const
{
    return m_fullName;
}

void QTweetPlace::setType(Type type)
{
    m_type = type;
}

QTweetPlace::Type QTweetPlace::type() const
{
    return m_type;
}
