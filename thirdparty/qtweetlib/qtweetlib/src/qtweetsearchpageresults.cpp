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

#include "qtweetsearchpageresults.h"
#include <QSharedData>
#include "qtweetsearchresult.h"

class QTweetSearchPageResultsData : public QSharedData {
public:
    //float completedIn;        //who cares?
    qint64 maxid;
    QByteArray nextPage;
    int page;
    QByteArray query;
    QByteArray refreshUrl;
    QList<QTweetSearchResult> results;
    int resultsPerPage;
    qint64 sinceid;
    int total;
};

QTweetSearchPageResults::QTweetSearchPageResults() : data(new QTweetSearchPageResultsData)
{
}

QTweetSearchPageResults::QTweetSearchPageResults(const QTweetSearchPageResults &rhs) : data(rhs.data)
{
}

QTweetSearchPageResults &QTweetSearchPageResults::operator=(const QTweetSearchPageResults &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QTweetSearchPageResults::~QTweetSearchPageResults()
{
}

void QTweetSearchPageResults::setMaxId(qint64 maxid)
{
    data->maxid = maxid;
}

qint64 QTweetSearchPageResults::maxid() const
{
    return data->maxid;
}

void QTweetSearchPageResults::setNextPage(const QByteArray &nextPage)
{
    data->nextPage = nextPage;
}

QByteArray QTweetSearchPageResults::nextPage() const
{
    return data->nextPage;
}

void QTweetSearchPageResults::setPage(int numPage)
{
    data->page = numPage;
}

int QTweetSearchPageResults::page() const
{
    return data->page;
}

void QTweetSearchPageResults::setQuery(const QByteArray &query)
{
    data->query = query;
}

QByteArray QTweetSearchPageResults::query() const
{
    return data->query;
}

void QTweetSearchPageResults::setRefreshUrl(const QByteArray &url)
{
    data->refreshUrl = url;
}

QByteArray QTweetSearchPageResults::refreshUrl() const
{
    return data->refreshUrl;
}

void QTweetSearchPageResults::setResults(const QList<QTweetSearchResult> &results)
{
    data->results = results;
}

QList<QTweetSearchResult> QTweetSearchPageResults::results() const
{
    return data->results;
}

void QTweetSearchPageResults::setResultsPerPage(int count)
{
    data->resultsPerPage = count;
}

int QTweetSearchPageResults::resultsPerPage() const
{
    return data->resultsPerPage;
}

void QTweetSearchPageResults::setSinceId(qint64 sinceid)
{
    data->sinceid = sinceid;
}

qint64 QTweetSearchPageResults::sinceid() const
{
    return data->sinceid;
}

void QTweetSearchPageResults::setTotal(int total)
{
    data->total = total;
}

int QTweetSearchPageResults::total() const
{
    return data->total;
}
