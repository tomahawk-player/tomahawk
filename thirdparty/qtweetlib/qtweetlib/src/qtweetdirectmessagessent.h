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

#ifndef QTWEETDIRECTMESSAGESSENT_H
#define QTWEETDIRECTMESSAGESSENT_H

#include "qtweetnetbase.h"

/**
 *   Fetches direct messages sent by the authenticating user.
 */
class QTWEETLIBSHARED_EXPORT QTweetDirectMessagesSent : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetDirectMessagesSent(QObject *parent = 0);
    QTweetDirectMessagesSent(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 sinceid = 0,
               qint64 maxid = 0,
               int count = 0,
               int page = 0,
               bool includeEntities = false);

signals:
    /** Emits direct messages list */
    void parsedDirectMessages(const QList<QTweetDMStatus>& messages);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETDIRECTMESSAGESSENT_H
