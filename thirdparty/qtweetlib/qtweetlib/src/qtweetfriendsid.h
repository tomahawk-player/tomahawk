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

#ifndef QTWEETFRIENDSID_H
#define QTWEETFRIENDSID_H

#include "qtweetnetbase.h"

/**
 *   Returns an list of numeric IDs for every user the specified user is following.
 */
class QTWEETLIBSHARED_EXPORT QTweetFriendsID : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetFriendsID(QObject *parent = 0);
    QTweetFriendsID(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 user,
               const QString& cursor = QString("-1"));
    void fetch(const QString& screenName,
               const QString& cursor = QString("-1"));

signals:
    /** Emits one page of ids
     * @param ids list of ids user is following
     * @param nextCursor cursor for next page, "0" if there is no page
     * @param prevCursor cursor for prev page "0" if there is no page
     */
    void parsedIDs(const QList<qint64>& ids,
                   const QString& nextCursor,
                   const QString& prevCursor);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETFRIENDSID_H
