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
#include "Source.h"

namespace Tomahawk {

bool
operator==( const SerializedUpdater& one, const SerializedUpdater& two )
{
    return one.type == two.type;
}

}

using namespace Tomahawk;

QMap< QString, PlaylistUpdaterFactory* > PlaylistUpdaterInterface::s_factories = QMap< QString, PlaylistUpdaterFactory* >();

void
PlaylistUpdaterInterface::registerUpdaterFactory( PlaylistUpdaterFactory* f )
{
    s_factories[ f->type() ] = f;
}



void
PlaylistUpdaterInterface::loadForPlaylist( const playlist_ptr& pl )
{
    TomahawkSettings* s = TomahawkSettings::instance();

    const SerializedUpdaters allUpdaters = s->playlistUpdaters();
    if ( allUpdaters.contains( pl->guid() ) )
    {
        // Ok, we have some we can try to load
        const SerializedUpdaterList updaters = allUpdaters.values( pl->guid() );
        foreach ( const SerializedUpdater& info, updaters )
        {
            if ( !s_factories.contains( info.type ) )
            {
                Q_ASSERT( false );
                // You forgot to register your new updater type with the factory....
                continue;
            }

            // Updaters register themselves in their constructor
            s_factories[ info.type ]->create( pl, info.customData );
        }
    }
}


PlaylistUpdaterInterface::PlaylistUpdaterInterface( const playlist_ptr& pl )
    : QObject( 0 )
    , m_playlist( pl )
{
    Q_ASSERT( !m_playlist.isNull() );

    m_playlist->addUpdater( this );

    QTimer::singleShot( 0, this, SLOT( save() ) );
}


PlaylistUpdaterInterface::~PlaylistUpdaterInterface()
{
    if ( !m_playlist.isNull() )
        m_playlist->removeUpdater( this );
}


void
PlaylistUpdaterInterface::save()
{
    if ( m_playlist.isNull() )
        return;

    TomahawkSettings* s = TomahawkSettings::instance();

    SerializedUpdaters allUpdaters = s->playlistUpdaters();
    if ( allUpdaters.contains( m_playlist->guid(), SerializedUpdater( type() ) ) )
        allUpdaters.remove( m_playlist->guid(), SerializedUpdater( type() ) );

    SerializedUpdater updater;
    updater.type = type();
    updater.customData = m_extraData;
    allUpdaters.insert( m_playlist->guid(), updater );
    s->setPlaylistUpdaters( allUpdaters );
}

void
PlaylistUpdaterInterface::remove()
{
    if ( m_playlist.isNull() )
        return;

    TomahawkSettings* s = TomahawkSettings::instance();
    SerializedUpdaters allUpdaters = s->playlistUpdaters();

    if ( allUpdaters.remove( m_playlist->guid(), SerializedUpdater( type() ) ) )
        s->setPlaylistUpdaters( allUpdaters );

    aboutToDelete();
    deleteLater();
}


QVariantHash
PlaylistUpdaterInterface::settings() const
{
    return m_extraData;
}


void
PlaylistUpdaterInterface::saveSettings( const QVariantHash& settings )
{
    m_extraData = settings;
    save();
}
