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
#include "collection/Collection.h"
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
    , m_infoSystemLoaded( false )
    , m_databaseLoaded( false )
    , m_mode( mode )
    , m_collection( collection )
    , m_artist( QPointer< Tomahawk::Artist >( artist ) )
{
}


ArtistPlaylistInterface::~ArtistPlaylistInterface()
{
    m_artist = 0;
}


void
ArtistPlaylistInterface::setCurrentIndex( qint64 index )
{
    PlaylistInterface::setCurrentIndex( index );

   m_currentItem = m_queries.at( index )->results().first();
}


qint64
ArtistPlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
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
ArtistPlaylistInterface::currentItem() const
{
    return m_currentItem;
}


QList<Tomahawk::query_ptr>
ArtistPlaylistInterface::tracks() const
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
            cmd->setArtist( m_artist->weakRef() );
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
        cmd->setArtist( m_artist->weakRef() );
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

    checkQueries();

    m_finished = true;
    emit tracksLoaded( m_mode, m_collection );
}


qint64
ArtistPlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
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
ArtistPlaylistInterface::indexOfQuery( const Tomahawk::query_ptr& query ) const
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
ArtistPlaylistInterface::queryAt( qint64 index ) const
{
    if ( index >= 0 && index < m_queries.count() )
    {
        return m_queries.at( index );
    }

    return Tomahawk::query_ptr();
}


result_ptr
ArtistPlaylistInterface::resultAt( qint64 index ) const
{
    Tomahawk::query_ptr query = queryAt( index );
    if ( query && query->numResults() )
        return query->results().first();

    return Tomahawk::result_ptr();
}


void
ArtistPlaylistInterface::checkQueries()
{
    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        connect( query.data(), SIGNAL( playableStateChanged( bool ) ), SLOT( onItemsChanged() ), Qt::UniqueConnection );
    }
}
