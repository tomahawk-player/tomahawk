/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#ifndef QTWEETSTATUS_H
#define QTWEETSTATUS_H

#include <QVariant>
#include <QSharedDataPointer>
#include "qtweetlib_global.h"

class QDateTime;
class QTweetUser;
class QTweetStatusData;
class QTweetPlace;
class QTweetEntityUrl;
class QTweetEntityHashtag;
class QTweetEntityUserMentions;

/**
 *   Stores tweet info
 */
class QTWEETLIBSHARED_EXPORT QTweetStatus
{
public:
    QTweetStatus();
    QTweetStatus(const QTweetStatus &other);
    QTweetStatus &operator=(const QTweetStatus &other);
    ~QTweetStatus();

    void setId(qint64 id);
    qint64 id() const;
    void setText(const QString& text);
    QString text() const;
    void setCreatedAt(const QString& twitterDate);
    void setCreatedAt(const QDateTime& dateTime);
    QDateTime createdAt() const;
    void setInReplyToUserId(qint64 id);
    qint64 inReplyToUserId() const;
    void setInReplyToScreenName(const QString& screenName);
    QString inReplyToScreenName() const;
    void setInReplyToStatusId(qint64 id);
    qint64 inReplyToStatusId() const;
    void setFavorited(bool fav);
    bool favorited() const;
    void setSource(const QString& source);
    QString source() const;
    void setUser(const QTweetUser& user);
    QTweetUser user() const;
    qint64 userid() const;  //usefull for getting quick userid when userinfo is trimmed
    void setRetweetedStatus(const QTweetStatus& status);
    QTweetStatus retweetedStatus() const;
    void setPlace(const QTweetPlace& place);
    QTweetPlace place() const;
    bool isRetweet() const;
    QList<QTweetEntityUrl> urlEntities() const;
    QList<QTweetEntityHashtag> hashtagEntities() const;
    QList<QTweetEntityUserMentions> userMentionsEntities() const;
    void addUrlEntity(const QTweetEntityUrl& urlEntity);
    void addHashtagEntity(const QTweetEntityHashtag& hashtagEntity);
    void addUserMentionsEntity(const QTweetEntityUserMentions& userMentionsEntity);

private:
    QSharedDataPointer<QTweetStatusData> d;
};

Q_DECLARE_METATYPE(QTweetStatus)

#endif // QTWEETSTATUS_H
