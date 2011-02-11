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

#ifndef QTWEETDIRECTMESSAGENEW_H
#define QTWEETDIRECTMESSAGENEW_H

#include "qtweetnetbase.h"

/**
 *   Sends a new direct message to the specified user from the authenticating user.
 */
class QTWEETLIBSHARED_EXPORT QTweetDirectMessageNew : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetDirectMessageNew(QObject *parent = 0);
    QTweetDirectMessageNew(OAuthTwitter *oauhtTwitter, QObject *parent = 0);
    void post(qint64 user,
              const QString& text,
              bool includeEntities = false);
    void post(const QString& screenName,
              const QString& text,
              bool includeEntities = false);

signals:
    /** Emits direct message who was sent */
    void parsedDirectMessage(const QTweetDMStatus& message);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETDIRECTMESSAGENEW_H
