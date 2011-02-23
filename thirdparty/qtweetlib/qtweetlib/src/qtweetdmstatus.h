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

#ifndef QTWEETDMSTATUS_H
#define QTWEETDMSTATUS_H

#include <QVariant>
#include <QSharedDataPointer>
#include "qtweetlib_global.h"

class QTweetUser;
class QTweetDMStatusData;

/**
 *   Stores direct message info
 */
class QTWEETLIBSHARED_EXPORT QTweetDMStatus
{
public:
    QTweetDMStatus();
    QTweetDMStatus(const QTweetDMStatus& other);
    QTweetDMStatus& operator=(const QTweetDMStatus& other);
    ~QTweetDMStatus();

    void setCreatedAt(const QString& twitterDate);
    void setCreatedAt(const QDateTime& datetime);
    QDateTime createdAt() const;
    void setSenderScreenName(const QString& screenName);
    QString senderScreenName() const;
    void setSender(const QTweetUser& sender);
    QTweetUser sender() const;
    void setText(const QString& text);
    QString text() const;
    void setRecipientScreenName(const QString& screenName);
    QString recipientScreenName() const;
    void setId(qint64 id);
    qint64 id() const;
    void setRecipient(const QTweetUser& recipient);
    QTweetUser recipient() const;
    void setRecipientId(qint64 id);
    qint64 recipientId() const;
    void setSenderId(qint64 id);
    qint64 senderId() const;

private:
    QSharedDataPointer<QTweetDMStatusData> d;
};

Q_DECLARE_METATYPE(QTweetDMStatus)

#endif // QTWEETDMSTATUS_H
