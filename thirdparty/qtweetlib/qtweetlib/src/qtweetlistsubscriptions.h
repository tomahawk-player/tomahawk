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

#ifndef QTWEETLISTSUBSCRIPTIONS_H
#define QTWEETLISTSUBSCRIPTIONS_H

#include "qtweetnetbase.h"

/**
 *   Fetch the lists the specified user follows.
 */
class QTWEETLIBSHARED_EXPORT QTweetListSubscriptions : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetListSubscriptions(QObject *parent = 0);
    QTweetListSubscriptions(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 user,
               const QString& cursor = QString());

signals:
    /*! Emits page of lists
     *  @param lists
     *  @param nextCursor cursor for next page, "0" if there is no next page
     *  @param prevCursor cursor for prev page, "0" if there is no prev page
     */
    void parsedLists(const QList<QTweetList>& lists,
                     const QString& nextCursor,
                     const QString& prevCursor);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETLISTSUBSCRIPTIONS_H
