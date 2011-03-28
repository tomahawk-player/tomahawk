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

#ifndef QTWEETSEARCHRESULT_H
#define QTWEETSEARCHRESULT_H

#include <QVariant>
#include <QSharedDataPointer>
#include "qtweetlib_global.h"

class QTweetSearchResultData;

/**
 *  Stores search result
 */
class QTWEETLIBSHARED_EXPORT QTweetSearchResult
{
public:
    QTweetSearchResult();
    QTweetSearchResult(const QTweetSearchResult &);
    QTweetSearchResult &operator=(const QTweetSearchResult &);
    ~QTweetSearchResult();

    void setCreatedAt(const QString& twitterDate);
    void setCreatedAt(const QDateTime& dateTime);
    QDateTime createdAt() const;
    void setFromUser(const QString& screenName);
    QString fromUser() const;
    void setId(qint64 id);
    qint64 id() const;
    void setLang(const QString& lang);
    QString lang() const;
    void setProfileImageUrl(const QString& url);
    QString profileImageUrl() const;
    void setSource(const QString& source);
    QString source() const;
    void setText(const QString& text);
    QString text() const;
    void setToUser(const QString& screenName);
    QString toUser() const;

private:
    QSharedDataPointer<QTweetSearchResultData> data;
};

#endif // QTWEETSEARCHRESULT_H
