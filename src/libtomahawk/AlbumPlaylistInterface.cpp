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

AlbumPlaylistInterface::AlbumPlaylistInterface() {}

AlbumPlaylistInterface::AlbumPlaylistInterface( Tomahawk::Album* album, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection )
    : Tomahawk::PlaylistInterface()
    , m_currentItem( 0 )
    , m_currentTrack( 0 )
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


Tomahawk::result_ptr
AlbumPlaylistInterface::siblingItem( int itemsAway )
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


result_ptr
AlbumPlaylistInterface::currentItem() const
{
    return m_currentItem;
}


bool
AlbumPlaylistInterface::hasNextItem()
{
    int p = m_currentTrack;
    p++;
    if ( p < 0 || p >= m_queries.count() )
        return false;

    return true;
}


QList< Tomahawk::query_ptr >
AlbumPlaylistInterface::tracks()
{
    if ( m_queries.isEmpty() && m_album )
    {
        if ( ( m_mode == Mixed || m_mode == InfoSystemMode ) && !m_infoSystemLoaded )
        {
            Tomahawk::InfoSystem::InfoStringHash artistInfo;
            artistInfo["artist"] = m_album.data()->artist()->name();
            artistInfo["album"] = m_album.data()->name();

            Tomahawk::InfoSystem::InfoRequestData requestData;
            requestData.caller = uuid();
            requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
            requestData.type = Tomahawk::InfoSystem::InfoAlbumSongs;
            requestData.timeoutMillis = 0;
            requestData.allSources = true;
            Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

            connect( Tomahawk::InfoSystem::InfoSystem::instance(),
                    SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                    SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
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
                    query->setAlbumPos( trackNo++ );
                    ql << query;
                    tDebug() << Q_FUNC_INFO << query->toString();
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

    m_infoSystemLoaded = true;
    disconnect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
                this, SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

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
        emit tracksLoaded( m_mode, m_collection );
    }
}


void
AlbumPlaylistInterface::onTracksLoaded( const QList< query_ptr >& tracks )
{
    m_databaseLoaded = true;

    if ( m_collection.isNull() )
        m_queries << filterTracks( tracks );
    else
        m_queries << tracks;

    emit tracksLoaded( m_mode, m_collection );
}


QList<Tomahawk::query_ptr>
AlbumPlaylistInterface::filterTracks( const QList<Tomahawk::query_ptr>& queries )
{
    QList<Tomahawk::query_ptr> result;

    for ( int i = 0; i < queries.count(); i++ )
    {
        bool picked = true;
        const query_ptr q1 = queries.at( i );

        for ( int j = 0; j < result.count(); j++ )
        {
            if ( !picked )
                break;

            const query_ptr& q2 = result.at( j );
            
            if ( q1->track() == q2->track() )
            {
                picked = false;
            }
        }

        if ( picked )
        {
            query_ptr q = Query::get( q1->artist(), q1->track(), q1->album(), uuid(), true );
            q->setAlbumPos( q1->results().first()->albumpos() );
            q->setDiscNumber( q1->discnumber() );
            result << q;
        }
    }
    
    foreach ( const query_ptr& q, result )
    {
        tDebug() << q->albumpos() << q->track();
    }
    return result;
}
