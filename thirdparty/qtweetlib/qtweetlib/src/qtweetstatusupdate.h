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

#ifndef QTWEETSTATUSUPDATE_H
#define QTWEETSTATUSUPDATE_H

#include "qtweetnetbase.h"
#include "qtweetgeocoord.h"

/**
 *   Class for updating user status (posting tweet)
 */
class QTWEETLIBSHARED_EXPORT QTweetStatusUpdate : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetStatusUpdate(QObject *parent = 0);
    QTweetStatusUpdate(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void post(const QString& status,
              qint64 inReplyToStatus = 0,
              const QTweetGeoCoord& latLong = QTweetGeoCoord(),
              const QString& placeid = QString(),
              bool displayCoordinates = false,
              bool trimUser = false,
              bool includeEntities = false);

signals:
    /** Emits posted status */
    void postedStatus(const QTweetStatus& status);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETSTATUSUPDATE_H
