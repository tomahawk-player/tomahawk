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

#ifndef QTWEETLISTSTATUSES_H
#define QTWEETLISTSTATUSES_H

#include "qtweetnetbase.h"

/**
 *   Fetches tweet timeline for members of the specified list.
 */
class QTWEETLIBSHARED_EXPORT QTweetListStatuses : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetListStatuses(QObject *parent = 0);
    QTweetListStatuses(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 user,
               qint64 list,
               qint64 sinceid = 0,
               qint64 maxid = 0,
               int perPage = 0,
               int page = 0,
               bool includeEntities = false);

signals:
    /** Emits the statuses of a specified list */
    void parsedStatuses(const QList<QTweetStatus>& statuses);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETLISTSTATUSES_H
