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
#include "TomahawkSettings.h"

using namespace Tomahawk;

QMap< QString, PlaylistUpdaterFactory* > PlaylistUpdaterInterface::s_factories = QMap< QString, PlaylistUpdaterFactory* >();

void
PlaylistUpdaterInterface::registerUpdaterFactory( PlaylistUpdaterFactory* f )
{
    s_factories[ f->type() ] = f;
}



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

        if ( !s_factories.contains( type ) )
        {
            Q_ASSERT( false );
            // You forgot to register your new updater type with the factory....
            return 0;
        }

        updater = s_factories[ type ]->create( pl, key );
        return updater;
    }

    return 0;
}


PlaylistUpdaterInterface::PlaylistUpdaterInterface( const playlist_ptr& pl )
    : QObject( 0 )
    , m_playlist( pl )
{
    Q_ASSERT( !m_playlist.isNull() );

    m_playlist->addUpdater( this );

    QTimer::singleShot( 0, this, SLOT( save() ) );
}


void
PlaylistUpdaterInterface::save()
{
    TomahawkSettings* s = TomahawkSettings::instance();
    const QString key = QString( "playlistupdaters/%1" ).arg( m_playlist->guid() );
    if ( !s->contains( QString( "%1/type" ).arg( key ) ) )
    {
        s->setValue( QString( "%1/type" ).arg( key ), type() );
    }
    saveToSettings( key );
}

void
PlaylistUpdaterInterface::remove()
{
    if ( m_playlist.isNull() )
        return;

    TomahawkSettings* s = TomahawkSettings::instance();
    const QString key = QString( "playlistupdaters/%1" ).arg( m_playlist->guid() );
    removeFromSettings( key );
    s->remove( QString( "%1/type" ).arg( key ) );

    deleteLater();
}
