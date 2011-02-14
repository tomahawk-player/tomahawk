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

#ifndef QTWEETUSERLOOKUP_H
#define QTWEETUSERLOOKUP_H

#include <QStringList>
#include "qtweetnetbase.h"

class QTweetUser;

/**
 *   Class for fetching up to 100 users and theirs most recent status
 */
class QTWEETLIBSHARED_EXPORT QTweetUserLookup : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetUserLookup(QObject *parent = 0);
    QTweetUserLookup(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void fetch(const QList<qint64>& useridList = QList<qint64>(),
               const QStringList& screenNameList = QStringList());

signals:
    /** Emits list of users */
    void parsedUserInfoList(const QList<QTweetUser>& userInfoList);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETUSERLOOKUP_H
