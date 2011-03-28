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

#include <QSharedData>
#include <QDateTime>
#include "qtweetuser.h"
#include "qtweetdmstatus.h"

class QTweetDMStatusData : public QSharedData
{
public:
    QDateTime createdAt;
    QString senderScreenName;
    QTweetUser sender;
    QString text;
    QString recipientScreenName;
    qint64 id;
    QTweetUser recipient;
    qint64 recipientId;
    qint64 senderId;
};

QTweetDMStatus::QTweetDMStatus() :
        d(new QTweetDMStatusData)
{
}

QTweetDMStatus::QTweetDMStatus(const QTweetDMStatus &other) :
        d(other.d)
{
}

QTweetDMStatus& QTweetDMStatus::operator =(const QTweetDMStatus &other)
{
    if (this != &other)
        d.operator =(other.d);
    return *this;
}

QTweetDMStatus::~QTweetDMStatus()
{
}

void QTweetDMStatus::setCreatedAt(const QString &twitterDate)
{
    d->createdAt = QTweetUser::twitterDateToQDateTime(twitterDate);
}

void QTweetDMStatus::setCreatedAt(const QDateTime &datetime)
{
    d->createdAt = datetime;
}

QDateTime QTweetDMStatus::createdAt() const
{
    return d->createdAt;
}

void QTweetDMStatus::setSenderScreenName(const QString &screenName)
{
    d->senderScreenName = screenName;
}

QString QTweetDMStatus::senderScreenName() const
{
    return d->senderScreenName;
}

void QTweetDMStatus::setSender(const QTweetUser &sender)
{
    d->sender = sender;
}

QTweetUser QTweetDMStatus::sender() const
{
    return d->sender;
}

void QTweetDMStatus::setText(const QString &text)
{
    d->text = text;
}

QString QTweetDMStatus::text() const
{
    return d->text;
}

void QTweetDMStatus::setRecipientScreenName(const QString &screenName)
{
    d->recipientScreenName = screenName;
}

QString QTweetDMStatus::recipientScreenName() const
{
    return d->recipientScreenName;
}

void QTweetDMStatus::setId(qint64 id)
{
    d->id = id;
}

qint64 QTweetDMStatus::id() const
{
    return d->id;
}

void QTweetDMStatus::setRecipient(const QTweetUser &recipient)
{
    d->recipient = recipient;
}

QTweetUser QTweetDMStatus::recipient() const
{
    return d->recipient;
}

void QTweetDMStatus::setRecipientId(qint64 id)
{
    d->recipientId = id;
}

qint64 QTweetDMStatus::recipientId() const
{
    return d->recipientId;
}

void QTweetDMStatus::setSenderId(qint64 id)
{
    d->senderId = id;
}

qint64 QTweetDMStatus::senderId() const
{
    return d->senderId;
}
