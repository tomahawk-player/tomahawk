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

#ifndef QTWEETLISTSHOWLIST_H
#define QTWEETLISTSHOWLIST_H

#include "qtweetnetbase.h"

/**
 *   Show the specified list.
 *   Private lists will only be shown if the authenticated user owns the specified list.
 */
class QTWEETLIBSHARED_EXPORT QTweetListShowList : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetListShowList(QObject *parent = 0);
    QTweetListShowList(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void show(qint64 id, qint64 list);

signals:
    /** Emits the list */
    void parsedList(const QTweetList& list);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETLISTSHOWLIST_H
