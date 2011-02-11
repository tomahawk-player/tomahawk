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

#ifndef QTWEETFRIENDSHIPDESTROY_H
#define QTWEETFRIENDSHIPDESTROY_H

#include "qtweetnetbase.h"

/**
 *   Allows the authenticating users to unfollow the user specified in the ID parameter.
 */
class QTWEETLIBSHARED_EXPORT QTweetFriendshipDestroy : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetFriendshipDestroy(QObject *parent = 0);
    QTweetFriendshipDestroy(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void unfollow(qint64 userid,
                  bool includeEntities = false);
    void unfollow(const QString& screenName,
                  bool includeEntities = false);

signals:
    /** Emits unfollowed user */
    void parsedUser(const QTweetUser& user);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETFRIENDSHIPDESTROY_H
