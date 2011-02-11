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

#ifndef QTWEETFAVORITES_H
#define QTWEETFAVORITES_H

#include "qtweetnetbase.h"

/**
 *   Fetches favorite statuses for the authenticating user or
 *   user specified by the ID parameter
 */
class QTWEETLIBSHARED_EXPORT QTweetFavorites : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetFavorites(QObject *parent = 0);
    QTweetFavorites(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 id = 0,
               int page = 0,
               bool includeEntities = false);

signals:
    /** Emits list of favorited statuses */
    void parsedFavorites(const QList<QTweetStatus>& favorites);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETFAVORITES_H
