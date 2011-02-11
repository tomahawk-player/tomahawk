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

#ifndef QTWEETSEARCH_H
#define QTWEETSEARCH_H

#include "qtweetnetbase.h"

/**
 *  Gets tweets that match a specified query
 */
class QTWEETLIBSHARED_EXPORT QTweetSearch : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetSearch(QObject *parent = 0);
    QTweetSearch(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void start(const QString& query,
               const QString& lang = QString(),
               /* const QString& locale = QString(), */
               int rpp = 0,
               int page = 0,
               qint64 sinceid = 0
               //geocode ### TODO
               //resultType ### TODO
               );
    void startWithCustomQuery(const QByteArray& encodedQuery);

signals:
    /** Emits page of search results */
    void parsedPageResults(const QTweetSearchPageResults& pageResults);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETSEARCH_H
