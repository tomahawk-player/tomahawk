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

#include "qtweetentityurl.h"
#include <QString>
#include <QSharedData>

class QTweetEntityUrlData : public QSharedData {
public:
    QTweetEntityUrlData() : empty(false) {}

    QString displayUrl;
    QString url;
    QString expandedUrl;
    bool empty;
};

QTweetEntityUrl::QTweetEntityUrl() : data(new QTweetEntityUrlData)
{
}

QTweetEntityUrl::QTweetEntityUrl(const QTweetEntityUrl &rhs) : data(rhs.data)
{
}

QTweetEntityUrl &QTweetEntityUrl::operator=(const QTweetEntityUrl &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QTweetEntityUrl::~QTweetEntityUrl()
{
}

void QTweetEntityUrl::setDisplayUrl(const QString &url)
{
    data->displayUrl = url;
}

QString QTweetEntityUrl::displayUrl() const
{
    return data->displayUrl;
}

void QTweetEntityUrl::setUrl(const QString &url)
{
    data->url = url;
}

QString QTweetEntityUrl::url() const
{
    return data->url;
}

void QTweetEntityUrl::setExpandedUrl(const QString &url)
{
    data->url = url;
}

QString QTweetEntityUrl::expandedUrl() const
{
    return data->url;
}
