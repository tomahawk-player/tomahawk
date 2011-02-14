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

#include "qtweetentityusermentions.h"
#include <QSharedData>
#include <QString>

class QTweetEntityUserMentionsData : public QSharedData {
public:
    QTweetEntityUserMentionsData() : userid(0) {}

    QString screenName;
    QString name;
    qint64 userid;
};

QTweetEntityUserMentions::QTweetEntityUserMentions() : data(new QTweetEntityUserMentionsData)
{
}

QTweetEntityUserMentions::QTweetEntityUserMentions(const QTweetEntityUserMentions &rhs) : data(rhs.data)
{
}

QTweetEntityUserMentions &QTweetEntityUserMentions::operator=(const QTweetEntityUserMentions &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QTweetEntityUserMentions::~QTweetEntityUserMentions()
{
}

void QTweetEntityUserMentions::setScreenName(const QString &screenName)
{
    data->screenName = screenName;
}

QString QTweetEntityUserMentions::screenName() const
{
    return data->screenName;
}

void QTweetEntityUserMentions::setName(const QString &name)
{
    data->name = name;
}

QString QTweetEntityUserMentions::name() const
{
    return data->name;
}

void QTweetEntityUserMentions::setUserid(qint64 id)
{
    data->userid = id;
}

qint64 QTweetEntityUserMentions::userid() const
{
    return data->userid;
}
