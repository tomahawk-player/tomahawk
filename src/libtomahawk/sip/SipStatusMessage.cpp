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

#include "SipStatusMessage.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QHash>
#include <QTimer>

SipStatusMessage::SipStatusMessage( SipStatusMessageType statusMessageType, const QString& contactId, const QString& message )
    : m_statusMessageType( statusMessageType )
    , m_contactId( contactId )
    , m_message( message )
{
    // make this temporary for now, as soon as i know how: add ack button
    m_timer = new QTimer( this );
    m_timer->setInterval( 8 * 1000 );
    m_timer->setSingleShot( true );

    connect( m_timer, SIGNAL( timeout() ), this, SIGNAL( finished() ) );
    m_timer->start();

    if( s_typesPixmaps.value( m_statusMessageType ).isNull() )
    {
        TomahawkUtils::ImageType imageType;
        switch( m_statusMessageType )
        {
            case SipLoginFailure:
            case SipInviteFailure:
                imageType = TomahawkUtils::ProcessStop;
                break;

            case SipInviteSuccess:
            case SipAuthReceived:
            default:
                imageType = TomahawkUtils::AddContact;
        }
        s_typesPixmaps.insert( m_statusMessageType, TomahawkUtils::defaultPixmap( imageType, TomahawkUtils::Original, QSize( 64, 64 ) ) );
    }
}


QPixmap
SipStatusMessage::icon() const
{
    return s_typesPixmaps.value( m_statusMessageType );
}


QString
SipStatusMessage::mainText() const
{
    QString text;
    switch( m_statusMessageType )
    {
        case SipInviteFailure:
            text = "Could not invite %1. Please check his/her id!";
            break;

        case SipInviteSuccess:
            text = "Invitation sent to %1!";
            break;

        case SipAuthReceived:
            text = "Received authorization from %1";
            break;

        case SipLoginFailure:
            text = "Could not login to %1. Please check your user credentials!";
            break;

        case SipConnectionFailure:
            text = "Could not connect to %1: %2";
            break;

        default:
            tLog() << Q_FUNC_INFO << "Not all status types handled";
            Q_ASSERT(false);
    }

    text = text.arg( m_contactId );
    if(text.contains( "%2") )
        text = text.arg( m_message );

    return text;
}

