/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "InboxJobItem.h"

#include "Query.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "audio/AudioEngine.h"
#include "TrackData.h"

#include <QTimer>


InboxJobItem::InboxJobItem( Side side,
                            const QString& prettyName,
                            const Tomahawk::trackdata_ptr& track,
                            QObject* parent )
    : JobStatusItem()
    , m_track( track )
    , m_prettyName( prettyName )
    , m_side( side )
{
    m_timer = new QTimer( this );
    m_timer->setInterval( 8000 );
    m_timer->setSingleShot( true );

    connect( m_timer, SIGNAL( timeout() ), this, SIGNAL( finished() ) );
    m_timer->start();
}


InboxJobItem::~InboxJobItem()
{}


QString
InboxJobItem::mainText() const
{
    switch ( m_side )
    {
    case Sending:
        return tr( "Sent %1 by %2 to %3." )
                .arg( m_track->track() )
                .arg( m_track->artist() )
                .arg( m_prettyName );
    case Receiving:
        return tr( "%1 sent you %2 by %3." )
                .arg( m_prettyName )
                .arg( m_track->track() )
                .arg( m_track->artist() );
    }
    return QString();
}


QPixmap
InboxJobItem::icon() const
{
    switch ( m_side )
    {
    case Sending:
        return TomahawkUtils::defaultPixmap( TomahawkUtils::Outbox, TomahawkUtils::Original, QSize( 64, 64 ) );
    case Receiving:
        return TomahawkUtils::defaultPixmap( TomahawkUtils::Inbox, TomahawkUtils::Original, QSize( 64, 64 ) );
    }
    return QPixmap();
}
