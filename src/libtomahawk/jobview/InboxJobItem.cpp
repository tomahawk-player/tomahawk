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

#include <QTimer>


InboxJobItem::InboxJobItem( const QString& sender,
                            const Tomahawk::track_ptr& track,
                            QObject* parent )
    : JobStatusItem()
    , m_track( track )
    , m_sender( sender )
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
    return tr( "%1 sent you %2 by %3." )
            .arg( m_sender )
            .arg( m_track->track() )
            .arg( m_track->artist() );
}


QPixmap
InboxJobItem::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Inbox, TomahawkUtils::Original, QSize( 64, 64 ) );
}
