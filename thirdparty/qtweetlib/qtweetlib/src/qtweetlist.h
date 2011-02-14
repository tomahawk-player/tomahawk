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

#ifndef QTWEETLIST_H
#define QTWEETLIST_H

#include <QVariant>
#include <QSharedDataPointer>
#include "qtweetlib_global.h"

class QTweetUser;
class QTweetListData;

/**
 *  Stores list information
 */
class QTWEETLIBSHARED_EXPORT QTweetList
{
public:
    QTweetList();
    QTweetList(const QTweetList& other);
    QTweetList& operator=(const QTweetList& other);
    ~QTweetList();

    void setMode(const QString& mode);
    QString mode() const;
    void setDescription(const QString& desc);
    QString description() const;
    void setFollowing(bool following);
    bool following() const;
    void setMemberCount(int count);
    int memberCount() const;
    void setFullName(const QString& name);
    QString fullName() const;
    void setSubscriberCount(int count);
    int subscriberCount() const;
    void setSlug(const QString& slug);
    QString slug() const;
    void setName(const QString& name);
    QString name() const;
    void setId(qint64 id);
    qint64 id() const;
    void setUri(const QString& uri);
    QString uri() const;
    void setUser(const QTweetUser& user);
    QTweetUser user() const;

private:
    QSharedDataPointer<QTweetListData> d;
};

Q_DECLARE_METATYPE(QTweetList)

#endif // QTWEETLIST_H
