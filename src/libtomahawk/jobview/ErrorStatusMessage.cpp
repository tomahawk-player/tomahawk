/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#include "ErrorStatusMessage.h"

#include "utils/TomahawkUtilsGui.h"

#include <QTimer>


ErrorStatusMessage::ErrorStatusMessage( const QString& message, int timeoutSecs )
  : JobStatusItem()
  , m_message( message )
{
    m_timer = new QTimer( this );
    m_timer->setInterval( timeoutSecs * 1000 );
    m_timer->setSingleShot( true );

    connect( m_timer, SIGNAL( timeout() ), this, SIGNAL( finished() ) );
    m_timer->start();
}


QPixmap
ErrorStatusMessage::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::ProcessStop, TomahawkUtils::Original, QSize( 64, 64 ) );
}


QString
ErrorStatusMessage::mainText() const
{
    return m_message;
}
