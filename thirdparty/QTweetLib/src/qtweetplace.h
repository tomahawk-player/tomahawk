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

#ifndef QTWEETPLACE_H
#define QTWEETPLACE_H

#include <QVariant>
#include <QString>
#include <QList>
#include "qtweetgeoboundingbox.h"
#include "qtweetlib_global.h"


/**
 *   Store place info
 */
class QTWEETLIBSHARED_EXPORT QTweetPlace
{
public:
    enum Type { Poi, Neighborhood, City, Admin, Country };

    QTweetPlace();
    void setName(const QString& name);
    QString name() const;
    void setCountry(const QString& country);
    QString country() const;
    void setCountryCode(const QString& code);
    QString countryCode() const;
    void setID(const QString& id);  // http://api.twitter.com/1/geo/id/<id>.json
    QString id() const;
    void setBoundingBox(const QTweetGeoBoundingBox& box);
    QTweetGeoBoundingBox boundingBox() const;
    void setContainedWithin(const QList<QTweetPlace>& places);
    QList<QTweetPlace> containedWithin() const;
    void setFullName(const QString& name);
    QString fullName() const;
    void setType(Type type);
    Type type() const;

private:
    QString m_name;
    QString m_country;
    QString m_countryCode;
    // ### TODO: Attributes
    QString m_id;
    QTweetGeoBoundingBox m_boundingBox;
    QList<QTweetPlace> m_containedWithin;
    QString m_fullName;
    Type m_type;
};

Q_DECLARE_METATYPE(QTweetPlace)

#endif // QTWEETPLACE_H
