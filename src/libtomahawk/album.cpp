/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "album.h"

#include "artist.h"
#include "albumplaylistinterface.h"
#include "database/database.h"
#include "database/databaseimpl.h"
#include "database/databasecommand_alltracks.h"
#include "query.h"

#include "utils/logger.h"

using namespace Tomahawk;


Album::~Album()
{
}


album_ptr
Album::get( const Tomahawk::artist_ptr& artist, const QString& name, bool autoCreate )
{
    int albid = Database::instance()->impl()->albumId( artist->id(), name, autoCreate );
    if ( albid < 1 && autoCreate )
        return album_ptr();

    return Album::get( albid, name, artist );
}


album_ptr
Album::get( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist )
{
    static QHash< unsigned int, album_ptr > s_albums;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_albums.contains( id ) )
    {
        return s_albums.value( id );
    }

    album_ptr a = album_ptr( new Album( id, name, artist ) );
    if ( id > 0 )
        s_albums.insert( id, a );

    return a;
}


Album::Album( unsigned int id, const QString& name, const Tomahawk::artist_ptr& artist )
    : QObject()
    , m_id( id )
    , m_name( name )
    , m_artist( artist )
    , m_infoLoaded( false )
{
    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
}


void
Album::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    Tomahawk::AlbumPlaylistInterface* api = dynamic_cast< Tomahawk::AlbumPlaylistInterface* >( playlistInterface().data() );
    if ( api )
        api->addQueries( tracks );
    
    emit tracksAdded( tracks );
}


artist_ptr
Album::artist() const
{
    return m_artist;
}


QImage
Album::cover() const
{
    if ( !m_infoLoaded )
    {
        m_uuid = uuid();

        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = artist()->name();
        trackInfo["album"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_uuid;
        requestData.type = Tomahawk::InfoSystem::InfoAlbumCoverArt;
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.customData = QVariantMap();

        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    return m_cover;
}


void
Album::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_uuid ||
         requestData.type != Tomahawk::InfoSystem::InfoAlbumCoverArt )
    {
        return;
    }

    m_infoLoaded = true;
    if ( !output.isNull() && output.isValid() )
    {
        QVariantMap returnedData = output.value< QVariantMap >();
        const QByteArray ba = returnedData["imgbytes"].toByteArray();
        if ( ba.length() )
        {
            m_cover.loadFromData( ba );
        }
    }

    emit updated();
}


Tomahawk::playlistinterface_ptr
Album::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::AlbumPlaylistInterface( this ) );
    }
    
    return m_playlistInterface;
}