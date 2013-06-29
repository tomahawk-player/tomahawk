/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Dominik Schmidt <domme@tomahawk-player.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "SipStatusMessage_p.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QHash>
#include <QTimer>

QHash< SipStatusMessage::SipStatusMessageType, QPixmap > SipStatusMessagePrivate::s_typesPixmaps = QHash< SipStatusMessage::SipStatusMessageType, QPixmap >();

SipStatusMessage::SipStatusMessage( SipStatusMessageType statusMessageType, const QString& contactId, const QString& message )
    : d_ptr( new SipStatusMessagePrivate( this, statusMessageType, contactId, message ) )
{
    Q_D( SipStatusMessage );

    // make this temporary for now, as soon as i know how: add ack button
    d->timer = new QTimer( this );
    d->timer->setInterval( 8 * 1000 );
    d->timer->setSingleShot( true );

    connect( d->timer, SIGNAL( timeout() ), this, SIGNAL( finished() ) );
    d->timer->start();

    if( SipStatusMessagePrivate::s_typesPixmaps.value( d->statusMessageType ).isNull() )
    {
        TomahawkUtils::ImageType imageType;
        switch( d->statusMessageType )
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
        SipStatusMessagePrivate::s_typesPixmaps.insert( d->statusMessageType, TomahawkUtils::defaultPixmap( imageType, TomahawkUtils::Original, QSize( 64, 64 ) ) );
    }
}


QPixmap
SipStatusMessage::icon() const
{
    Q_D( const SipStatusMessage );

    return SipStatusMessagePrivate::s_typesPixmaps.value( d->statusMessageType );
}


QString
SipStatusMessage::mainText() const
{
    Q_D( const SipStatusMessage );

    QString text;
    switch( d->statusMessageType )
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

    text = text.arg( d->contactId );
    if(text.contains( "%2") )
        text = text.arg( d->message );

    return text;
}

