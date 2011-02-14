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

#ifndef QTWEETUSER_H
#define QTWEETUSER_H

#include <QVariant>
#include <QSharedDataPointer>
#include "qtweetlib_global.h"

class QTweetStatus;
class QTweetUserData;

/**
 *   Class for storing user info
 */
class QTWEETLIBSHARED_EXPORT QTweetUser
{
public:
    QTweetUser();
    QTweetUser(const QTweetUser& other);
    QTweetUser& operator=(const QTweetUser& other);
    ~QTweetUser();

    void setContributorsEnabled(bool enabled);
    bool isContributorsEnabled() const;
    void setId(qint64 id);
    qint64 id() const;
    void setLang(const QString& lang);
    QString lang() const;
    void setListedCount(int count);
    int listedCount() const;
    void setName(const QString& name);
    QString name() const;
    void setScreenName(const QString& screenName);
    QString screenName() const;
    void setLocation(const QString& location);
    QString location() const;
    void setDescription(const QString& desc);
    QString description() const;
    void setprofileImageUrl(const QString& url);
    QString profileImageUrl() const;
    void setUrl(const QString& url);
    QString url() const;
    void setProtected(bool isProtected);
    bool isProtected() const;
    void setFollowersCount(int count);
    int followersCount() const;
    void setFriendsCount(int count);
    int friendsCount() const;
    void setCreatedAt(const QString& twitterDate);
    void setCreatedAt(const QDateTime& datetime);
    QDateTime createdAt() const;
    void setFavouritesCount(int count);
    int favouritesCount() const;
    void setUtcOffset(int sec);
    int utcOffset() const;
    void setTimezone(const QString& timezone);
    QString timezone() const;
    void setGeoEnabled(bool isGeoEnabled);
    bool isGeoEnabled() const;
    void setVerified(bool verified);
    bool isVerified() const;
    void setStatusesCount(int count);
    int statusesCount() const;
    void setStatus(const QTweetStatus& lastStatus);
    QTweetStatus status() const;

    static QDateTime twitterDateToQDateTime(const QString& twitterDate);

private:
    QSharedDataPointer<QTweetUserData> d;
};

Q_DECLARE_METATYPE(QTweetUser)

#endif // QTWEETUSER_H
