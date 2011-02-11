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

#ifndef QTWEETSTATUSRETWEETS_H
#define QTWEETSTATUSRETWEETS_H

#include "qtweetnetbase.h"

// ### TODO

/**
 *   Class to fetch up to 100 first retweets of a given tweet
 */
class QTWEETLIBSHARED_EXPORT QTweetStatusRetweets : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetStatusRetweets(QObject *parent = 0);
    QTweetStatusRetweets(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(qint64 id, int count = 0);
};

#endif // QTWEETSTATUSRETWEETS_H
