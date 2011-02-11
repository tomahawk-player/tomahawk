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

#include "followerslistmodel.h"
#include "oauthtwitter.h"
#include "qtweetuser.h"
#include "qtweetuserstatusesfollowers.h"

FollowersListModel::FollowersListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "nameRole";
    roles[ScreenNameRole] = "screenNameRole";
    roles[DescriptionRole] = "descriptionRole";
    roles[AvatarRole] = "avatarRole";
    setRoleNames(roles);
}

FollowersListModel::FollowersListModel(OAuthTwitter *oauthTwitter, QObject *parent) :
    QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "nameRole";
    roles[ScreenNameRole] = "screenNameRole";
    roles[DescriptionRole] = "descriptionRole";
    roles[AvatarRole] = "avatarRole";
    setRoleNames(roles);

    m_oauthTwitter = oauthTwitter;
}

void FollowersListModel::setOAuthTwitter(OAuthTwitter *oauthTwitter)
{
    m_oauthTwitter = oauthTwitter;
}

int FollowersListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_users.count();
}

QVariant FollowersListModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > m_users.count())
        return QVariant();

    const QTweetUser &user = m_users.at(index.row());

    if (role == NameRole)
        return user.name();
    else if (role == ScreenNameRole)
        return user.screenName();
    else if (role == DescriptionRole)
        return user.description();
    else if (role == AvatarRole)
        return user.profileImageUrl();

    return QVariant();
}

void FollowersListModel::fetchFollowers(const QString &cursor)
{
    if (cursor == "-1") {
        beginResetModel();
        m_users.clear();
        endResetModel();
    }

    QTweetUserStatusesFollowers *followers = new QTweetUserStatusesFollowers;
    followers->setOAuthTwitter(m_oauthTwitter);
    followers->fetch(0, cursor, false);
    connect(followers, SIGNAL(parsedFollowersList(QList<QTweetUser>,QString)),
            this, SLOT(followersFinished(QList<QTweetUser>,QString)));

}

void FollowersListModel::followersFinished(const QList<QTweetUser> &followers, const QString &nextCursor)
{
    QTweetUserStatusesFollowers *users = qobject_cast<QTweetUserStatusesFollowers*>(sender());

    if (users) {
        beginInsertRows(QModelIndex(), m_users.count(), m_users.count() + followers.count());
        m_users.append(followers);
        endInsertRows();

        if (nextCursor == "0")
            return;

        //continue fetchingg next page
        fetchFollowers(nextCursor);

        users->deleteLater();
    }
}
