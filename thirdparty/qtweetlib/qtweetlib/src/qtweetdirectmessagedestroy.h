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

#ifndef QTWEETDIRECTMESSAGEDESTROY_H
#define QTWEETDIRECTMESSAGEDESTROY_H

#include "qtweetnetbase.h"

/**
 *   Destroys the direct message.
 *   The authenticating user must be the recipient of the specified direct message.
 */
class QTWEETLIBSHARED_EXPORT QTweetDirectMessageDestroy : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetDirectMessageDestroy(QObject *parent = 0);
    QTweetDirectMessageDestroy(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void destroyMessage(qint64 id, bool includeEntities = false);

signals:
    /** emits destroyed direct message */
    void parsedDirectMessage(const QTweetDMStatus& message);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETDIRECTMESSAGEDESTROY_H
