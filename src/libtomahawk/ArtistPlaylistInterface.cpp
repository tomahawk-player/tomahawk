/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "ArtistPlaylistInterface.h"

#include "Artist.h"
#include "Collection.h"
#include "Query.h"
#include "database/Database.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "Source.h"
#include "Pipeline.h"

#include "utils/Logger.h"

using namespace Tomahawk;


ArtistPlaylistInterface::ArtistPlaylistInterface( Tomahawk::Artist* artist, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection )
    : Tomahawk::PlaylistInterface()
    , m_currentItem( 0 )
    , m_currentTrack( 0 )
    , m_infoSystemLoaded( false )
    , m_databaseLoaded( false )
    , m_mode( mode )
    , m_collection( collection )
    , m_artist( QWeakPointer< Tomahawk::Artist >( artist ) )
{
}


ArtistPlaylistInterface::~ArtistPlaylistInterface()
{
    m_artist.clear();
}


Tomahawk::result_ptr
ArtistPlaylistInterface::siblingItem( int itemsAway )
{
    int p = m_currentTrack;
    p += itemsAway;

    if ( p < 0 )
        return Tomahawk::result_ptr();

    if ( p >= m_queries.count() )
        return Tomahawk::result_ptr();

    if ( !m_queries.at( p )->numResults() )
        return siblingItem( itemsAway + 1 );

    m_currentTrack = p;
    m_currentItem = m_queries.at( p )->results().first();
    return m_currentItem;
}


bool
ArtistPlaylistInterface::hasNextItem()
{
    int p = m_currentTrack;
    p++;
    if ( p < 0 || p >= m_queries.count() )
        return false;

    return true;
}


result_ptr
ArtistPlaylistInterface::currentItem() const
{
    return m_currentItem;
}


QList<Tomahawk::query_ptr>
ArtistPlaylistInterface::tracks()
{
    if ( m_queries.isEmpty() && m_artist )
    {
        if ( ( m_mode == Mixed || m_mode == InfoSystemMode ) && !m_infoSystemLoaded )
        {
            Tomahawk::InfoSystem::InfoStringHash artistInfo;
            artistInfo["artist"] = m_artist.data()->name();

            Tomahawk::InfoSystem::InfoRequestData requestData;
            requestData.caller = id();
            requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
            requestData.type = Tomahawk::InfoSystem::InfoArtistSongs;
            requestData.timeoutMillis = 0;
            requestData.allSources = true;
            Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

            connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                    SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                    SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

            connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                    SIGNAL( finished( QString ) ),
                    SLOT( infoSystemFinished( QString ) ) );
        }
        else if ( m_mode == DatabaseMode && !m_databaseLoaded )
        {
            DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
            cmd->setArtist( m_artist );
            cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );

            connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                            SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ) );

            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
        }
    }

    return m_queries;
}


void
ArtistPlaylistInterface::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != id() )
        return;

    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoArtistSongs:
        {
            QVariantMap returnedData = output.value< QVariantMap >();
            if ( !returnedData.isEmpty() )
            {
                Tomahawk::InfoSystem::InfoStringHash inputInfo;
                inputInfo = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();

                QStringList tracks = returnedData[ "tracks" ].toStringList();
                QList<query_ptr> ql;

                //TODO: Figure out how to do this with a multi-disk album without breaking the
                //      current behaviour. I just know too little about InfoSystem to deal with
                //      it right now, I've only taken the liberty of adding Query::setDiscNumber
                //      which should make this easier. --Teo 11/2011
                unsigned int trackNo = 1;

                foreach ( const QString& trackName, tracks )
                {
                    query_ptr query = Query::get( inputInfo[ "artist" ], trackName, inputInfo[ "album" ] );
                    query->setAlbumPos( trackNo++ );
                    ql << query;
                }
                Pipeline::instance()->resolve( ql );

                m_queries << ql;
            }

            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }

    if ( !m_queries.isEmpty() )
        infoSystemFinished( id() );
}


void
ArtistPlaylistInterface::infoSystemFinished( const QString &infoId )
{
    if ( infoId != id() )
        return;

    m_infoSystemLoaded = true;

    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                this, SLOT( infoSystemFinished( QString) ) );

    if ( m_queries.isEmpty() && m_mode == Mixed )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
        cmd->setArtist( m_artist );
        //this takes discnumber into account as well
        cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );

        connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                        SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }
    else
    {
        m_finished = true;
        emit tracksLoaded( m_mode, m_collection );
    }
}


void
ArtistPlaylistInterface::onTracksLoaded( const QList< query_ptr >& tracks )
{
    if ( m_collection.isNull() )
    {
        m_databaseLoaded = true;
        m_queries << filterTracks( tracks );
    }
    else
        m_queries << tracks;

    m_finished = true;
    emit tracksLoaded( m_mode, m_collection );
}
