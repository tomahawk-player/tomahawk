/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "PlaylistUpdaterInterface.h"
#include "tomahawksettings.h"
#include "XspfUpdater.h"

using namespace Tomahawk;

PlaylistUpdaterInterface*
PlaylistUpdaterInterface::loadForPlaylist( const playlist_ptr& pl )
{
    TomahawkSettings* s = TomahawkSettings::instance();
    const QString key = QString( "playlistupdaters/%1" ).arg( pl->guid() );
    if ( s->contains( QString( "%1/type" ).arg( key ) ) )
    {
        // Ok, we have one we can try to load
        const QString type = s->value( QString( "%1/type" ).arg( key ) ).toString();
        PlaylistUpdaterInterface* updater = 0;
        if ( type == "xspf" )
            updater = new XspfUpdater( pl );

        // You forgot to register your new updater type with the factory above. 00ps.
        if ( !updater )
        {
            Q_ASSERT( false );
            return 0;
        }
        updater->setAutoUpdate( s->value( QString( "%1/autoupdate" ).arg( key ) ).toBool() );
        updater->setInterval( s->value( QString( "%1/interval" ).arg( key ) ).toInt() );
        updater->loadFromSettings( key );

        return updater;
    }

    return 0;
}


PlaylistUpdaterInterface::PlaylistUpdaterInterface( const playlist_ptr& pl )
    : QObject( 0 )
    , m_timer( new QTimer( this ) )
    , m_autoUpdate( true )
    , m_playlist( pl )
{
    Q_ASSERT( !m_playlist.isNull() );

    m_playlist->setUpdater( this );
    connect( m_timer, SIGNAL( timeout() ), this, SLOT( updateNow() ) );

    QTimer::singleShot( 0, this, SLOT( doSave() ) );
}

void
PlaylistUpdaterInterface::doSave()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    const QString key = QString( "playlistupdaters/%1" ).arg( m_playlist->guid() );
    if ( !s->contains( QString( "%1/type" ).arg( key ) ) )
    {
        s->setValue( QString( "%1/type" ).arg( key ), type() );
        s->setValue( QString( "%1/autoupdate" ).arg( key ), m_autoUpdate );
        s->setValue( QString( "%1/interval" ).arg( key ), m_timer->interval() );
        saveToSettings( key );
    }
}

void
PlaylistUpdaterInterface::setAutoUpdate( bool autoUpdate )
{
    m_autoUpdate = autoUpdate;
    if ( m_autoUpdate )
        m_timer->start();
    else
        m_timer->stop();

    const QString key = QString( "playlistupdaters/%1/autoupdate" ).arg( m_playlist->guid() );
    TomahawkSettings::instance()->setValue( key, m_autoUpdate );
}

void
PlaylistUpdaterInterface::setInterval( int intervalMsecs )
{
    const QString key = QString( "playlistupdaters/%1/interval" ).arg( m_playlist->guid() );
    TomahawkSettings::instance()->setValue( key, intervalMsecs );

    m_timer->setInterval( intervalMsecs );
}

