/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "AlbumPlaylistInterface.h"

#include "Artist.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "Pipeline.h"
#include "Query.h"
#include "Source.h"
#include "SourceList.h"

#include "utils/Logger.h"

using namespace Tomahawk;


AlbumPlaylistInterface::AlbumPlaylistInterface( Tomahawk::Album* album, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection )
    : Tomahawk::PlaylistInterface()
    , m_currentItem( 0 )
    , m_infoSystemLoaded( false )
    , m_databaseLoaded( false )
    , m_mode( mode )
    , m_collection( collection )
    , m_album( QWeakPointer< Tomahawk::Album >( album ) )
{
}


AlbumPlaylistInterface::~AlbumPlaylistInterface()
{
    m_album.clear();
}


void
AlbumPlaylistInterface::setCurrentIndex( qint64 index )
{
    PlaylistInterface::setCurrentIndex( index );

    m_currentItem = m_queries.at( index )->results().first();
}


qint64
AlbumPlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
{
    qint64 p = m_currentIndex;
    if ( rootIndex >= 0 )
        p = rootIndex;

    p += itemsAway;

    if ( p < 0 )
        return -1;

    if ( p >= m_queries.count() )
        return -1;

    return p;
}


result_ptr
AlbumPlaylistInterface::currentItem() const
{
    return m_currentItem;
}


bool
AlbumPlaylistInterface::setCurrentTrack( unsigned int albumpos )
{
    Q_UNUSED( albumpos );
    Q_ASSERT( false );
    return false;

/*    albumpos--;
    if ( ( int ) albumpos >= m_queries.count() )
        return false;

    m_currentTrack = albumpos;
    m_currentItem = m_queries.at( albumpos )->results().first();
    return true;*/
}


QList< Tomahawk::query_ptr >
AlbumPlaylistInterface::tracks() const
{
    if ( m_queries.isEmpty() && m_album )
    {
        if ( ( m_mode == Mixed || m_mode == InfoSystemMode ) && !m_infoSystemLoaded )
        {
            Tomahawk::InfoSystem::InfoStringHash artistInfo;
            artistInfo["artist"] = m_album.data()->artist()->name();
            artistInfo["album"] = m_album.data()->name();

            Tomahawk::InfoSystem::InfoRequestData requestData;
            requestData.caller = id();
            requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
            requestData.type = Tomahawk::InfoSystem::InfoAlbumSongs;
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
            cmd->setAlbum( m_album );
            cmd->setSortOrder( DatabaseCommand_AllTracks::AlbumPosition );

            connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                            SLOT( onTracksLoaded( QList<Tomahawk::query_ptr> ) ) );

            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
        }
    }

    return m_queries;
}


void
AlbumPlaylistInterface::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != id() )
        return;

    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoAlbumSongs:
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
                    if ( query.isNull() )
                        continue;

                    query->setAlbumPos( trackNo++ );
                    ql << query;
                }
                Pipeline::instance()->resolve( ql );

                m_queries << ql;
                checkQueries();
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
    {
        infoSystemFinished( id() );
    }
}


void
AlbumPlaylistInterface::infoSystemFinished( const QString& infoId )
{
    if ( infoId != id() )
        return;

    m_infoSystemLoaded = true;
    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ),
                this, SLOT( infoSystemFinished( QString ) ) );

    if ( m_queries.isEmpty() && m_mode == Mixed )
    {
        DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( m_collection );
        cmd->setAlbum( m_album );
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
AlbumPlaylistInterface::onTracksLoaded( const QList< query_ptr >& tracks )
{
    if ( m_collection.isNull() )
    {
        m_databaseLoaded = true;
        m_queries << filterTracks( tracks );
    }
    else
        m_queries << tracks;

    checkQueries();

    m_finished = true;
    emit tracksLoaded( m_mode, m_collection );
}


qint64
AlbumPlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
{
    int i = 0;
    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        if ( query->numResults() && query->results().contains( result ) )
            return i;

        i++;
    }

    return -1;
}


qint64
AlbumPlaylistInterface::indexOfQuery( const Tomahawk::query_ptr& query ) const
{
    int i = 0;
    foreach ( const Tomahawk::query_ptr& q, m_queries )
    {
        if ( q->equals( query ) )
            return i;

        i++;
    }

    return -1;
}


query_ptr
AlbumPlaylistInterface::queryAt( qint64 index ) const
{
    if ( index >= 0 && index < m_queries.count() )
    {
        return m_queries.at( index );
    }

    return Tomahawk::query_ptr();
}


result_ptr
AlbumPlaylistInterface::resultAt( qint64 index ) const
{
    Tomahawk::query_ptr query = queryAt( index );
    if ( query && query->numResults() )
        return query->results().first();

    return Tomahawk::result_ptr();
}


void
AlbumPlaylistInterface::checkQueries()
{
    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        connect( query.data(), SIGNAL( playableStateChanged( bool ) ), SLOT( onItemsChanged() ), Qt::UniqueConnection );
    }
}
