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

#ifndef QTWEETLISTDELETEMEMBER_H
#define QTWEETLISTDELETEMEMBER_H

#include "qtweetnetbase.h"

/**
 *   Removes the specified member from the list.
 *   The authenticated user must be the list's owner to remove members from the list.
 */
class QTWEETLIBSHARED_EXPORT QTweetListDeleteMember : public QTweetNetBase
{
    Q_OBJECT
public:
    QTweetListDeleteMember(QObject *parent = 0);
    QTweetListDeleteMember(OAuthTwitter *oauthTwitter, QObject *parent = 0);
    void remove(qint64 user,
                qint64 list,
                qint64 member);

signals:
    /** Emits list where member was deleted */
    void parsedList(const QTweetList& list);

protected slots:
    void parsingJsonFinished(const QVariant &json, bool ok, const QString &errorMsg);
};

#endif // QTWEETLISTDELETEMEMBER_H
