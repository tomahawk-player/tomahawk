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

#ifndef QTWEETLISTMEMBERSHIPS_H
#define QTWEETLISTMEMBERSHIPS_H

#include "qtweetnetbase.h"

/**
 *   Fetches the lists the specified user has been added to.
 */
class QTWEETLIBSHARED_EXPORT QTweetListMemberships : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetListMemberships(QObject *parent = 0);
    QTweetListMemberships(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 id,
               const QString& cursor = QString());

signals:
    /** Emits page of lists
     *  @param lists
     *  @param nextCursor cursor for next page, "0" if there is no next page
     *  @param prevCursor cursor for prev page, "0" if there is no prev page
     */
    void parsedList(const QList<QTweetList>& lists,
                    const QString& nextCursor,
                    const QString& prevCursor);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETLISTMEMBERSHIPS_H
