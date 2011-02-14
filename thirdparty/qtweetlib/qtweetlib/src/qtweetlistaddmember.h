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

#ifndef QTWEETLISTADDMEMBER_H
#define QTWEETLISTADDMEMBER_H

#include "qtweetnetbase.h"

/**
 *   Add a member to a list.
 *   The authenticated user must own the list to be able to add members to it.
 *   Lists are limited to having 500 members.
 */
class QTWEETLIBSHARED_EXPORT QTweetListAddMember : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetListAddMember(QObject *parent = 0);
    QTweetListAddMember(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void add(qint64 user,
             qint64 list,
             qint64 memberid);

signals:
    /** Emits list in which user was added */
    void parsedList(const QTweetList& list);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETLISTADDMEMBER_H
