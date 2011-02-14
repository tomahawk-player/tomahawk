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

#ifndef QTWEETUSERSTATUSESFRIENDS_H
#define QTWEETUSERSTATUSESFRIENDS_H

#include "qtweetnetbase.h"

/**
 * Fetches a user's friends, each with current status inline.
 * They are ordered by the order in which the user followed them, most recently followed first, 100 at a time.
 *
 *  Use the cursor option to access older friends.
 */
class QTWEETLIBSHARED_EXPORT QTweetUserStatusesFriends : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetUserStatusesFriends(QObject *parent = 0);
    QTweetUserStatusesFriends(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 userid = 0,
               const QString& cursor = QString(),
               bool includeEntities = false);
    void fetch(const QString& screenName,
               const QString& cursor = QString(),
               bool includeEntities = false);

signals:
    /** Emits page of friends
     *  @param friendsList list of friends
     *  @param nextCursor cursor for next page, "0" if there is no next page, empty if there is no paging
     *  @param prevCursor cursor for prev page, "0" if there is no prev page, empty if there is no paging
     */
    void parsedFriendsList(const QList<QTweetUser>& friendsList,
                           const QString& nextCursor = QString(),
                           const QString& prevCursor = QString());

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);

private:
    bool m_usesCursoring;   // ### TODO: Remove it
};

#endif // QTWEETUSERSTATUSESFRIENDS_H
