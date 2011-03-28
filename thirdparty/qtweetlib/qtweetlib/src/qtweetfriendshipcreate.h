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

#ifndef QTWEETFRIENDSHIPCREATE_H
#define QTWEETFRIENDSHIPCREATE_H

#include "qtweetnetbase.h"

/**
 *   Allows the authenticating users to follow the user specified in the ID parameter.
 */
class QTWEETLIBSHARED_EXPORT QTweetFriendshipCreate : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetFriendshipCreate(QObject *parent = 0);
    QTweetFriendshipCreate(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void create(qint64 userid,
                bool follow = false,
                bool includeEntities = false);
    void create(const QString& screenName,
                bool follow = false,
                bool includeEntities = false);

signals:
    /** Emits the befriended user */
    void parsedUser(const QTweetUser& user);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETFRIENDSHIPCREATE_H
