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

#ifndef QTWEETFAVORITESDESTROY_H
#define QTWEETFAVORITESDESTROY_H

#include "qtweetnetbase.h"

/**
 *   Un-favorites the status specified in the ID parameter as the authenticating user.
 *   Emits the un-favorited status when successful.
 */
class QTWEETLIBSHARED_EXPORT QTweetFavoritesDestroy : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetFavoritesDestroy(QObject *parent = 0);
    QTweetFavoritesDestroy(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void unfavorite(qint64 statusid, bool includeEntities = false);

signals:
    /** Emits the unfavorited status */
    void parsedStatus(const QTweetStatus& status);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETFAVORITESDESTROY_H
