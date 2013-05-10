/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "TreeProxyModel.h"

#include "TreeProxyModelPlaylistInterface.h"
#include "Source.h"
#include "Query.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "collection/AlbumsRequest.h"
#include "collection/ArtistsRequest.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "PlayableItem.h"
#include "utils/Logger.h"

#include <QListView>

TreeProxyModel::TreeProxyModel( QObject* parent )
    : PlayableProxyModel( parent )
    , m_artistsFilterCmd( 0 )
    , m_model( 0 )
{
    setPlaylistInterface( Tomahawk::playlistinterface_ptr( new Tomahawk::TreeProxyModelPlaylistInterface( this ) ) );
}


void
TreeProxyModel::setSourcePlayableModel( TreeModel* model )
{
    if ( sourceModel() )
    {
        disconnect( m_model, SIGNAL( rowsInserted( QModelIndex, int, int ) ), this, SLOT( onRowsInserted( QModelIndex, int, int ) ) );
        disconnect( m_model, SIGNAL( modelReset() ), this, SLOT( onModelReset() ) );
    }

    PlayableProxyModel::setSourcePlayableModel( model );
    m_model = model;

    if ( sourceModel() )
    {
        connect( m_model, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onRowsInserted( QModelIndex, int, int ) ) );
        connect( m_model, SIGNAL( modelReset() ), SLOT( onModelReset() ) );
    }
}


void
TreeProxyModel::onRowsInserted( const QModelIndex& parent, int /* start */, int /* end */ )
{
    if ( m_filter.isEmpty() )
        return;
    if ( sender() != m_model )
        return;

    PlayableItem* pi = m_model->itemFromIndex( m_model->index( parent.row(), 0, parent.parent() ) );
    if ( pi->artist().isNull() )
        return;

    Tomahawk::AlbumsRequest* cmd = 0;
    if ( !m_model->collection().isNull() )
        cmd = m_model->collection()->requestAlbums( pi->artist() );
    else
        cmd = new DatabaseCommand_AllAlbums( Tomahawk::collection_ptr(), pi->artist() );

    cmd->setFilter( m_filter );

    connect( dynamic_cast< QObject* >( cmd ), SIGNAL( albums( QList<Tomahawk::album_ptr> ) ),
             SLOT( onFilterAlbums( QList<Tomahawk::album_ptr> ) ) );

    cmd->enqueue();
}


void
TreeProxyModel::onModelReset()
{
    m_cache.clear();
    m_artistsFilter.clear();
    m_albumsFilter.clear();
}


void
TreeProxyModel::setFilter( const QString& pattern )
{
    emit filteringStarted();

    m_filter = pattern;
    m_albumsFilter.clear();

    if ( m_artistsFilterCmd )
    {
        disconnect( dynamic_cast< QObject* >( m_artistsFilterCmd ), SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                    this, SLOT( onFilterArtists( QList<Tomahawk::artist_ptr> ) ) );

        m_artistsFilterCmd = 0;
    }

    if ( m_filter.isEmpty() )
    {
        filterFinished();
    }
    else
    {
        Tomahawk::ArtistsRequest* cmd = 0;
        if ( !m_model->collection().isNull() )
            cmd = m_model->collection()->requestArtists();
        else
            cmd = new DatabaseCommand_AllArtists(); //for SuperCollection, TODO: replace with a proper proxy-ArtistsRequest

        cmd->setFilter( pattern );
        m_artistsFilterCmd = cmd;

        connect( dynamic_cast< QObject* >( cmd ), SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                 SLOT( onFilterArtists( QList<Tomahawk::artist_ptr> ) ) );

        cmd->enqueue();
    }
}


QString
TreeProxyModel::filter() const
{
    return m_filter;
}


void
TreeProxyModel::onFilterArtists( const QList<Tomahawk::artist_ptr>& artists )
{
    bool finished = true;
    m_artistsFilter = artists;
    m_artistsFilterCmd = 0;

    foreach ( const Tomahawk::artist_ptr& artist, artists )
    {
        QModelIndex idx = m_model->indexFromArtist( artist );
        if ( m_model->rowCount( idx ) )
        {
            finished = false;

            Tomahawk::AlbumsRequest* cmd = m_model->collection()->requestAlbums( artist );

            cmd->setFilter( m_filter );

            connect( dynamic_cast< QObject* >( cmd ), SIGNAL( albums( QList<Tomahawk::album_ptr> ) ),
                     SLOT( onFilterAlbums( QList<Tomahawk::album_ptr> ) ) );

            cmd->enqueue();
        }
    }

    if ( finished )
        filterFinished();
}


void
TreeProxyModel::onFilterAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    foreach ( const Tomahawk::album_ptr& album, albums )
        m_albumsFilter << album->id();

    filterFinished();
}


void
TreeProxyModel::filterFinished()
{
    m_artistsFilterCmd = 0;

    setFilterRegExp( m_filter );
    emit filterChanged( m_filter );
    emit filteringFinished();
}


bool
TreeProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    PlayableItem* item = sourceModel()->itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    Q_ASSERT( item );

    if ( m_model->mode() == Tomahawk::DatabaseMode && !item->query().isNull() )
    {
        QList< Tomahawk::query_ptr > rl = m_cache.values( sourceParent );
        foreach ( const Tomahawk::query_ptr& cachedQuery, rl )
        {
            if ( cachedQuery.isNull() )
                continue;

            if ( cachedQuery->track() == item->query()->track() &&
               ( cachedQuery->albumpos() == item->query()->albumpos() || cachedQuery->albumpos() == 0 ) )
            {
                return ( cachedQuery.data() == item->query().data() );
            }
        }

        for ( int i = 0; i < sourceModel()->rowCount( sourceParent ); i++ )
        {
            if ( i == sourceRow )
                continue;

            PlayableItem* ti = sourceModel()->itemFromIndex( sourceModel()->index( i, 0, sourceParent ) );

            if ( ti && ti->name() == item->name() && !ti->query().isNull() )
            {
                if ( ti->query()->albumpos() == item->query()->albumpos() || ti->query()->albumpos() == 0 || item->query()->albumpos() == 0 )
                {
                    if ( item->result().isNull() )
                        return false;

                    if ( !ti->result().isNull() )
                    {
                        if ( !item->result()->isOnline() && ti->result()->isOnline() )
                            return false;

                        if ( ( item->result()->collection().isNull() || !item->result()->collection()->source()->isLocal() ) &&
                             !ti->result()->collection().isNull() && ti->result()->collection()->source()->isLocal() )
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    bool accepted = false;
    if ( m_filter.isEmpty() )
        accepted = true;
    else if ( !item->artist().isNull() )
        accepted = m_artistsFilter.contains( item->artist() );
    else if ( !item->album().isNull() )
        accepted = m_albumsFilter.contains( item->album()->id() );

    if ( !accepted )
    {
        QStringList sl = m_filter.split( " ", QString::SkipEmptyParts );
        foreach( const QString& s, sl )
        {
            if ( !item->name().contains( s, Qt::CaseInsensitive ) &&
                 !item->albumName().contains( s, Qt::CaseInsensitive ) &&
                 !item->artistName().contains( s, Qt::CaseInsensitive ) )
            {
                return false;
            }
        }
    }

    m_cache.insertMulti( sourceParent, item->query() );
    return true;
}


bool
TreeProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    PlayableItem* p1 = sourceModel()->itemFromIndex( left );
    PlayableItem* p2 = sourceModel()->itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

/*    if ( !p1->result().isNull() && p2->result().isNull() )
        return true;
    if ( p1->result().isNull() && !p2->result().isNull() )
        return false;*/

    unsigned int albumpos1 = 0;
    unsigned int albumpos2 = 0;
    unsigned int discnumber1 = 0;
    unsigned int discnumber2 = 0;
    if ( !p1->query().isNull() )
    {
        albumpos1 = p1->query()->albumpos();
        discnumber1 = p1->query()->discnumber();
    }
    if ( !p2->query().isNull() )
    {
        albumpos2 = p2->query()->albumpos();
        discnumber2 = p2->query()->discnumber();
    }
    if ( !p1->result().isNull() )
    {
        if ( albumpos1 == 0 )
            albumpos1 = p1->result()->albumpos();
        if ( discnumber1 == 0 )
            discnumber1 = p1->result()->discnumber();
    }
    if ( !p2->result().isNull() )
    {
        if ( albumpos2 == 0 )
            albumpos2 = p2->result()->albumpos();
        if ( discnumber2 == 0 )
            discnumber2 = p2->result()->discnumber();
    }
    discnumber1 = qMax( 1, (int)discnumber1 );
    discnumber2 = qMax( 1, (int)discnumber2 );

    if ( discnumber1 != discnumber2 )
    {
        return discnumber1 < discnumber2;
    }
    else
    {
        if ( albumpos1 != albumpos2 )
            return albumpos1 < albumpos2;
    }

    const QString& lefts = textForItem( p1 );
    const QString& rights = textForItem( p2 );
    if ( lefts == rights )
        return (qint64)&p1 < (qint64)&p2;

    return QString::localeAwareCompare( lefts, rights ) < 0;
}


QString
TreeProxyModel::textForItem( PlayableItem* item ) const
{
    if ( !item )
        return QString();

    if ( !item->artist().isNull() )
    {
        return item->artist()->sortname();
    }
    else if ( !item->album().isNull() )
    {
        return DatabaseImpl::sortname( item->album()->name() );
    }
    else if ( !item->result().isNull() )
    {
        return DatabaseImpl::sortname( item->result()->track() );
    }
    else if ( !item->query().isNull() )
    {
        return item->query()->track();
    }

    return QString();
}


QModelIndex
TreeProxyModel::indexFromArtist( const Tomahawk::artist_ptr& artist ) const
{
    return mapFromSource( m_model->indexFromArtist( artist ) );
}


QModelIndex
TreeProxyModel::indexFromAlbum( const Tomahawk::album_ptr& album ) const
{
    return mapFromSource( m_model->indexFromAlbum( album ) );
}


QModelIndex
TreeProxyModel::indexFromResult( const Tomahawk::result_ptr& result ) const
{
    return mapFromSource( m_model->indexFromResult( result ) );
}


QModelIndex
TreeProxyModel::indexFromQuery( const Tomahawk::query_ptr& query ) const
{
    return mapFromSource( m_model->indexFromQuery( query ) );
}
