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

#ifndef QTWEETACCOUNTRATELIMITSTATUS_H
#define QTWEETACCOUNTRATELIMITSTATUS_H

#include "qtweetnetbase.h"

/**
 *   Returns the remaining number of API requests available to the requesting user
 *   before the API limit is reached for the current hour.
 */
class QTweetAccountRateLimitStatus : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetAccountRateLimitStatus(QObject *parent = 0);
    QTweetAccountRateLimitStatus(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void check();

signals:
    /** Emits rate limit info
     *  @param remainingHits Remaining hits
     *  @param resetTime Reset time in seconds
     *  @param hourlyLimit
     */
    void rateLimitInfo(int remainingHits, int resetTime, int hourlyLimit);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETACCOUNTRATELIMITSTATUS_H
