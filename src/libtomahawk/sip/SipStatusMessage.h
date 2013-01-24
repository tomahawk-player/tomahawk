/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIPSTATUSMESSAGE_H
#define SIPSTATUSMESSAGE_H

#include "jobview/JobStatusItem.h"
#include "DllMacro.h"

#include <QPixmap>
#include <QHash>

class QTimer;

class DLLEXPORT SipStatusMessage : public JobStatusItem
{
    Q_OBJECT
public:
    enum SipStatusMessageType
    {
        SipInviteSuccess,
        SipInviteFailure,
        SipAuthReceived,
        SipLoginFailure,
        SipConnectionFailure
    };

    explicit SipStatusMessage( SipStatusMessageType statusMessageType, const QString& contactId, const QString& message = QString() );

    QString type() const { return "sipstatusmessage"; }

    QPixmap icon() const;
    QString mainText() const;

    bool allowMultiLine() const { return true; }
private:
    QString m_contactId;
    SipStatusMessageType m_statusMessageType;
    QString m_message;

    QHash< SipStatusMessageType, QPixmap > s_typesPixmaps;

    QTimer* m_timer;
};

#endif // SIPSTATUSMESSAGE_H
