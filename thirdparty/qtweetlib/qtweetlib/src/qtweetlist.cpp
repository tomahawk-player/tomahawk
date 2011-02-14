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

#include <QSharedData>
#include "qtweetlist.h"
#include "qtweetuser.h"

class QTweetListData : public QSharedData
{
public:
    QString mode;
    QString description;
    bool following;
    int memberCount;
    QString fullname;
    int subscriberCount;
    QString slug;
    QString name;
    qint64 id;
    QString uri;
    QTweetUser user;
};

QTweetList::QTweetList() :
        d(new QTweetListData)
{
}

QTweetList::QTweetList(const QTweetList &other) :
        d(other.d)
{
}

QTweetList& QTweetList::operator =(const QTweetList& other)
{
   if (this != &other)
       d.operator =(other.d);
   return *this;
}

QTweetList::~QTweetList()
{
}

void QTweetList::setMode(const QString &mode)
{
    d->mode = mode;
}

QString QTweetList::mode() const
{
    return d->mode;
}

void QTweetList::setDescription(const QString &desc)
{
    d->description = desc;
}

QString QTweetList::description() const
{
    return d->description;
}

void QTweetList::setFollowing(bool following)
{
    d->following = following;
}

bool QTweetList::following() const
{
    return d->following;
}

void QTweetList::setMemberCount(int count)
{
    d->memberCount = count;
}

int QTweetList::memberCount() const
{
    return d->memberCount;
}

void QTweetList::setFullName(const QString &name)
{
    d->fullname = name;
}

QString QTweetList::fullName() const
{
    return d->fullname;
}

void QTweetList::setSubscriberCount(int count)
{
    d->subscriberCount = count;
}

int QTweetList::subscriberCount() const
{
    return d->subscriberCount;
}

void QTweetList::setSlug(const QString &slug)
{
    d->slug = slug;
}

QString QTweetList::slug() const
{
    return d->slug;
}

void QTweetList::setName(const QString &name)
{
    d->name = name;
}

QString QTweetList::name() const
{
    return d->name;
}

void QTweetList::setId(qint64 id)
{
    d->id = id;
}

qint64 QTweetList::id() const
{
    return d->id;
}

void QTweetList::setUri(const QString &uri)
{
    d->uri = uri;
}

QString QTweetList::uri() const
{
    return d->uri;
}

void QTweetList::setUser(const QTweetUser &user)
{
    d->user = user;
}

QTweetUser QTweetList::user() const
{
    return d->user;
}
