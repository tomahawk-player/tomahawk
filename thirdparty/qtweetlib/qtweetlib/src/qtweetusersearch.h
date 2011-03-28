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

#ifndef QTWEETUSERSEARCH_H
#define QTWEETUSERSEARCH_H

#include "qtweetnetbase.h"

/**
 *   Runs a search for users
 */
class QTWEETLIBSHARED_EXPORT QTweetUserSearch : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetUserSearch(QObject *parent = 0);
    QTweetUserSearch(OAuthTwitter* oauthTwitter, QObject *parent);
    void search(const QString& query,
                int perPage = 0,
                int page = 0,
                bool includeEntities = false);

signals:
    /** Emits list of users */
    void parsedUserInfoList(const QList<QTweetUser>& userInfoList);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETUSERSEARCH_H
