/* Copyright (c) 2010, Antonie Jovanoski
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact e-mail: Antonie Jovanoski <minimoog77_at_gmail.com>
 */

#ifndef QTWEETSTATUSSHOW_H
#define QTWEETSTATUSSHOW_H

#include "qtweetnetbase.h"

/**
 *   Class for fetching single tweet
 */
class QTWEETLIBSHARED_EXPORT QTweetStatusShow : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetStatusShow(QObject *parent = 0);
    QTweetStatusShow(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 id,
               bool trimUser = false,
               bool includeEntities = false);

signals:
    /** Emits specified tweet */
    void parsedStatus(const QTweetStatus& status);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETSTATUSSHOW_H
