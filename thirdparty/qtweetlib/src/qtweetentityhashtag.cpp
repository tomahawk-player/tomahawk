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

#include "qtweetentityhashtag.h"
#include <QString>
#include <QSharedData>

class QTweetEntityHashtagData : public QSharedData {
public:
    QString hashtag;
};

QTweetEntityHashtag::QTweetEntityHashtag() : data(new QTweetEntityHashtagData)
{
}

QTweetEntityHashtag::QTweetEntityHashtag(const QTweetEntityHashtag &rhs) : data(rhs.data)
{
}

QTweetEntityHashtag &QTweetEntityHashtag::operator=(const QTweetEntityHashtag &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QTweetEntityHashtag::~QTweetEntityHashtag()
{
}

void QTweetEntityHashtag::setText(const QString &text)
{
    data->hashtag = text;
}

QString QTweetEntityHashtag::text() const
{
    return data->hashtag;
}
