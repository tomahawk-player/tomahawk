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

#ifndef QTWEETACCOUNTVERIFYCREDENTIALS_H
#define QTWEETACCOUNTVERIFYCREDENTIALS_H

#include "qtweetnetbase.h"

/**
 *  Checks credentials of a authenticated user. Should emit parsedUser signal
 *  if authentication was successful, error if not. Use this to test if
 *  supplied user credentials are valid
 */
class QTWEETLIBSHARED_EXPORT QTweetAccountVerifyCredentials : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetAccountVerifyCredentials(QObject *parent = 0);
    QTweetAccountVerifyCredentials(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void verify(bool includeEntities = false);

signals:
    /** Emits parsed user when credentials are valid */
    void parsedUser(const QTweetUser& user);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETACCOUNTVERIFYCREDENTIALS_H
