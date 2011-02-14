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

#include <QSharedData>
#include <QDateTime>
#include "qtweetuser.h"
#include "qtweetstatus.h"

class QTweetUserData : public QSharedData
{
public:
    QTweetUserData() : id(0) {}

    bool contributorsEnabled;
    QDateTime createdAt;
    QString description;
    int favoritesCount;
    bool followRequestSent;
    int followersCount;
    int friendsCount;
    bool geoEnabled;
    qint64 id;
    QString lang;
    int listedCount;
    QString location;
    QString name;
    //QString profileBackgroundColor;
    //QString profileBackgroundImageUrl;
    //bool profileBackgroundTile;
    QString profileImageUrl;
    //QString profileLinkColor;
    //QString profileSidebarBorderColor;
    //QString profileSidebarFillColor;
    //QString profileTextColor;
    //bool profileUseBackgroundImage;
    bool accountProtected;
    QString screenName;
    //bool showAllInlineMedia;
    int statusesCount;
    QString timeZone;
    QString url;
    int utcOffset;
    bool verified;
    //avoid recursion with QTweetStatus
    qint64 statusId;
    QString statusText;
    QDateTime statusCreatedAt;
    qint64 statusInReplyToUserId;
    QString statusInReplyToScreenName;
    qint64 statusInReplyToStatusId;
    bool statusFavorited;
    QString statusSource;
    //QTweetUser user
    //QTweetStatus retweetedStatus; //check if there is retweeted status in user response
    //bool containsRetweetStatus;
    //QTweetStatus status;    //should be pointer?
};

QTweetUser::QTweetUser() :
        d(new QTweetUserData)
{
}

QTweetUser::QTweetUser(const QTweetUser &other) :
        d(other.d)
{
}

QTweetUser& QTweetUser::operator =(const QTweetUser& other)
{
    if (this != &other)
        d.operator =(other.d);
    return *this;
}

QTweetUser::~QTweetUser()
{
}

void QTweetUser::setContributorsEnabled(bool enabled)
{
    d->contributorsEnabled = enabled;
}

bool QTweetUser::isContributorsEnabled() const
{
    return d->contributorsEnabled;
}

void QTweetUser::setId(qint64 id)
{
    d->id = id;
}

qint64 QTweetUser::id() const
{
    return d->id;
}

void QTweetUser::setLang(const QString &lang)
{
    d->lang = lang;
}

QString QTweetUser::lang() const
{
    return d->lang;
}

void QTweetUser::setListedCount(int count)
{
    d->listedCount = count;
}

int QTweetUser::listedCount() const
{
    return d->listedCount;
}

void QTweetUser::setName(const QString &name)
{
    d->name = name;
}

QString QTweetUser::name() const
{
    return d->name;
}

//void QTweetUser::setProfileBackgroundColor(const QString &color)
//{
//    // ### TODO: prepend #?
//    // ### TODO: use QColor (QGui dependancy?) instead QString
//    d->profileBackgroundColor = color;
//}

//QString QTweetUser::profileBackgroundColor() const
//{
//    return d->profileBackgroundColor;
//}

void QTweetUser::setScreenName(const QString &screenName)
{
    d->screenName = screenName;
}

QString QTweetUser::screenName() const
{
    return d->screenName;
}

void QTweetUser::setLocation(const QString &location)
{
    d->location = location;
}

QString QTweetUser::location() const
{
    return d->location;
}

void QTweetUser::setDescription(const QString &desc)
{
    d->description = desc;
}

QString QTweetUser::description() const
{
    return d->description;
}

void QTweetUser::setprofileImageUrl(const QString &url)
{
    d->profileImageUrl = url;
}

QString QTweetUser::profileImageUrl() const
{
    return d->profileImageUrl;
}

void QTweetUser::setUrl(const QString &url)
{
    d->url = url;
}

QString QTweetUser::url() const
{
    return d->url;
}

void QTweetUser::setProtected(bool protected_)
{
    d->accountProtected = protected_;
}

bool QTweetUser::isProtected() const
{
    return d->accountProtected;
}

void QTweetUser::setFollowersCount(int count)
{
    d->followersCount = count;
}

int QTweetUser::followersCount() const
{
    return d->followersCount;
}

void QTweetUser::setFriendsCount(int count)
{
    d->friendsCount = count;
}

int QTweetUser::friendsCount() const
{
    return d->friendsCount;
}

void QTweetUser::setCreatedAt(const QString &twitterDate)
{
    d->createdAt = twitterDateToQDateTime(twitterDate);
}

void QTweetUser::setCreatedAt(const QDateTime &datetime)
{
    d->createdAt = datetime;
}

QDateTime QTweetUser::createdAt() const
{
    return d->createdAt;
}

void QTweetUser::setFavouritesCount(int count)
{
    d->favoritesCount = count;
}

int QTweetUser::favouritesCount() const
{
    return d->favoritesCount;
}

void QTweetUser::setUtcOffset(int sec)
{
    d->utcOffset = sec;
}

int QTweetUser::utcOffset() const
{
    return d->utcOffset;
}

void QTweetUser::setTimezone(const QString &timezone)
{
    d->timeZone = timezone;
}

QString QTweetUser::timezone() const
{
    return d->timeZone;
}

void QTweetUser::setGeoEnabled(bool isGeoEnabled)
{
    d->geoEnabled = isGeoEnabled;
}

bool QTweetUser::isGeoEnabled() const
{
    return d->geoEnabled;
}

void QTweetUser::setVerified(bool verified)
{
    d->verified = verified;
}

bool QTweetUser::isVerified() const
{
    return d->verified;
}

void QTweetUser::setStatusesCount(int count)
{
    d->statusesCount = count;
}

int QTweetUser::statusesCount() const
{
    return d->statusesCount;
}

void QTweetUser::setStatus(const QTweetStatus &lastStatus)
{
    //d->status = lastStatus;
    d->statusId = lastStatus.id();
    d->statusText = lastStatus.text();
    d->statusCreatedAt = lastStatus.createdAt();
    d->statusInReplyToScreenName = lastStatus.inReplyToScreenName();
    d->statusInReplyToStatusId = lastStatus.inReplyToStatusId();
    d->statusInReplyToUserId = lastStatus.inReplyToUserId();
    d->statusFavorited = lastStatus.favorited();
    d->statusSource = lastStatus.source();
}

QTweetStatus QTweetUser::status() const
{
    QTweetStatus lastStatus;
    lastStatus.setId(d->statusId);
    lastStatus.setText(d->statusText);
    //lastStatus.setCreatedAt();    // ### TODO FIX IT!
    lastStatus.setInReplyToScreenName(d->statusInReplyToScreenName);
    lastStatus.setInReplyToStatusId(d->statusInReplyToStatusId);
    lastStatus.setInReplyToUserId(d->statusInReplyToUserId);
    lastStatus.setFavorited(d->statusFavorited);
    lastStatus.setSource(d->statusSource);

    return lastStatus;
}

QDateTime QTweetUser::twitterDateToQDateTime(const QString &twitterDate)
{
    //Twitter Date Format: 'Wed Sep 01 11:27:25 +0000 2010'  UTC
    QString dateString = twitterDate.left(11) + twitterDate.right(4);
    QString timeString = twitterDate.mid(11, 8);

    QDate date = QDate::fromString(dateString);
    QTime time = QTime::fromString(timeString);

    if (date.isValid() && time.isValid())
        return QDateTime(date, time, Qt::UTC);
    else
        return QDateTime();
}
