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

#ifndef QTWEETSEARCHPAGERESULTS_H
#define QTWEETSEARCHPAGERESULTS_H

#include <QList>
#include <QVariant>
#include <QSharedDataPointer>
#include "qtweetlib_global.h"

class QTweetSearchPageResultsData;
class QTweetSearchResult;

/**
 *  Stores page of a search results
 */
class QTWEETLIBSHARED_EXPORT QTweetSearchPageResults
{
public:
    QTweetSearchPageResults();
    QTweetSearchPageResults(const QTweetSearchPageResults &);
    QTweetSearchPageResults &operator=(const QTweetSearchPageResults &);
    ~QTweetSearchPageResults();

    void setMaxId(qint64 maxid);
    qint64 maxid() const;
    void setNextPage(const QByteArray& nextPage);
    QByteArray nextPage() const;
    void setPage(int numPage);
    int page() const;
    void setQuery(const QByteArray& query);
    QByteArray query() const;
    void setRefreshUrl(const QByteArray& url);
    QByteArray refreshUrl() const;
    void setResults(const QList<QTweetSearchResult>& results);
    QList<QTweetSearchResult> results() const;
    void setResultsPerPage (int count);
    int resultsPerPage() const;
    void setSinceId(qint64 sinceid);
    qint64 sinceid() const;
    void setTotal(int total);
    int total() const;

private:
    QSharedDataPointer<QTweetSearchPageResultsData> data;
};

Q_DECLARE_METATYPE(QTweetSearchPageResults)

#endif // QTWEETSEARCHPAGERESULTS_H
