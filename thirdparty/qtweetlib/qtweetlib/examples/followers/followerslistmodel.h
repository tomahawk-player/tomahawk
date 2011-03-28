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

#ifndef FOLLOWERSLISTMODEL_H
#define FOLLOWERSLISTMODEL_H

#include <QAbstractListModel>

class OAuthTwitter;
class QTweetUser;

class FollowersListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ScreenNameRole,
        DescriptionRole,
        AvatarRole
    };

    FollowersListModel(QObject *parent = 0);
    FollowersListModel(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void setOAuthTwitter(OAuthTwitter *oauthTwitter);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    void fetchFollowers(const QString& cursor = QString("-1"));

private slots:
    void followersFinished(const QList<QTweetUser>& followers,
                           const QString& nextCursor);

private:
    OAuthTwitter *m_oauthTwitter;
    QList<QTweetUser> m_users;
};

#endif // FOLLOWERSLISTMODEL_H
