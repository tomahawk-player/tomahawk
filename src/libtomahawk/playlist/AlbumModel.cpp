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

#include "AlbumModel.h"

#include <QListView>
#include <QMimeData>
#include <QNetworkReply>

#include "Artist.h"
#include "PlayableItem.h"
#include "Source.h"
#include "SourceList.h"
#include "database/Database.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;


AlbumModel::AlbumModel( QObject* parent )
    : PlayableModel( parent )
    , m_overwriteOnAdd( false )
{
}


AlbumModel::~AlbumModel()
{
}


void
AlbumModel::addCollection( const collection_ptr& collection, bool overwrite )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->nodeId();

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection );
    m_overwriteOnAdd = overwrite;
    m_collection = collection;

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( addAlbums( QList<Tomahawk::album_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    setTitle( tr( "All albums from %1" ).arg( collection->source()->friendlyName() ) );

    if ( collection.isNull() )
    {
        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ), Qt::UniqueConnection );

        QList<Tomahawk::source_ptr> sources = SourceList::instance()->sources();
        foreach ( const source_ptr& source, sources )
        {
            connect( source->dbCollection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
        }
    }
    else
    {
        connect( collection.data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
    }

    emit loadingStarted();
}


void
AlbumModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllAlbums::SortOrder order, bool overwrite )
{
/*    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->nodeId()
                            << amount << order;*/

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );
    m_overwriteOnAdd = overwrite;
    m_collection = collection;

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( addAlbums( QList<Tomahawk::album_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    if ( !collection.isNull() )
        setTitle( tr( "All albums from %1" ).arg( collection->source()->friendlyName() ) );
    else
        setTitle( tr( "All albums" ) );

    emit loadingStarted();
}


void
AlbumModel::addAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    emit loadingFinished();

    if ( m_overwriteOnAdd )
        clear();

    QList<Tomahawk::album_ptr> trimmedAlbums;
    foreach ( const album_ptr& album, albums )
    {
        if ( !album.isNull() && album->name().length() )
        {
            if ( findItem( album ) || trimmedAlbums.contains( album ) )
                continue;
            trimmedAlbums << album;
        }
    }

    if ( !trimmedAlbums.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + trimmedAlbums.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlayableItem* albumitem;
    foreach( const album_ptr& album, trimmedAlbums )
    {
        albumitem = new PlayableItem( album, rootItem() );
        albumitem->index = createIndex( rootItem()->children.count() - 1, 0, albumitem );

        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
AlbumModel::addArtists( const QList<Tomahawk::artist_ptr>& artists )
{
    emit loadingFinished();

    if ( m_overwriteOnAdd )
        clear();

    QList<Tomahawk::artist_ptr> trimmedArtists;
    foreach ( const artist_ptr& artist, artists )
    {
        if ( !artist.isNull() && artist->name().length() )
        {
            if ( findItem( artist ) || trimmedArtists.contains( artist ) )
                continue;
            trimmedArtists << artist;
        }
    }

    if ( !trimmedArtists.count() )
    {
        emit itemCountChanged( rowCount( QModelIndex() ) );
        return;
    }

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + trimmedArtists.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlayableItem* albumitem;
    foreach ( const artist_ptr& artist, trimmedArtists )
    {
        albumitem = new PlayableItem( artist, rootItem() );
        albumitem->index = createIndex( rootItem()->children.count() - 1, 0, albumitem );

        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
AlbumModel::addQueries( const QList<Tomahawk::query_ptr>& queries )
{
    emit loadingFinished();

    if ( m_overwriteOnAdd )
        clear();

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + queries.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlayableItem* albumitem;
    foreach ( const query_ptr& query, queries )
    {
        albumitem = new PlayableItem( query, rootItem() );
        albumitem->index = createIndex( rootItem()->children.count() - 1, 0, albumitem );

        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit itemCountChanged( rowCount( QModelIndex() ) );
}


void
AlbumModel::onSourceAdded( const Tomahawk::source_ptr& source )
{
    connect( source->dbCollection().data(), SIGNAL( changed() ), SLOT( onCollectionChanged() ), Qt::UniqueConnection );
}


void
AlbumModel::onCollectionChanged()
{
    addCollection( m_collection, true );
}


PlayableItem*
AlbumModel::findItem( const artist_ptr& artist ) const
{
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        PlayableItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
        if ( !item->artist().isNull() && item->artist() == artist )
        {
            return item;
        }
    }

    return 0;
}


PlayableItem*
AlbumModel::findItem( const album_ptr& album ) const
{
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        PlayableItem* item = itemFromIndex( index( i, 0, QModelIndex() ) );
        if ( !item->album().isNull() && item->album() == album )
        {
            return item;
        }
    }

    return 0;
}
