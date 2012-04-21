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

#include "CollectionFlatModel.h"

#include "database/Database.h"
#include "SourceList.h"
#include "utils/logger.h"

using namespace Tomahawk;


CollectionFlatModel::CollectionFlatModel( QObject* parent )
    : TrackModel( parent )
{
}


CollectionFlatModel::~CollectionFlatModel()
{
}


void
CollectionFlatModel::addCollections( const QList< collection_ptr >& collections )
{
    foreach( const collection_ptr& col, collections )
    {
        addCollection( col );
    }

    // we are waiting for some to load
    if( !m_loadingCollections.isEmpty() )
        emit loadingStarted();
}


void
CollectionFlatModel::addCollection( const collection_ptr& collection, bool sendNotifications )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    if ( sendNotifications )
        emit loadingStarted();

    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( collection );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

    m_loadingCollections << collection.data();

    if ( collection->source()->isLocal() )
        setTitle( tr( "My Collection" ) );
    else
        setTitle( tr( "Collection of %1" ).arg( collection->source()->friendlyName() ) );
}


void
CollectionFlatModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllTracks::SortOrder order )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName()
                            << amount << order;

    DatabaseCommand_AllTracks* cmd = new DatabaseCommand_AllTracks( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr>, QVariant ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
CollectionFlatModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    qDebug() << Q_FUNC_INFO << tracks.count() << rowCount( QModelIndex() );

    if ( !m_loadingCollections.isEmpty() && sender() && qobject_cast< Collection* >( sender() ) )
    {
        // we are keeping track and are called as a slot
        m_loadingCollections.removeAll( qobject_cast< Collection* >( sender() ) );
    }

    append( tracks );

    if ( m_loadingCollections.isEmpty() )
        emit loadingFinished();
}


void
CollectionFlatModel::onTracksRemoved( const QList<Tomahawk::query_ptr>& tracks )
{
    QList<Tomahawk::query_ptr> t = tracks;
    for ( int i = rowCount( QModelIndex() ); i >= 0 && t.count(); i-- )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        TrackModelItem* item = itemFromIndex( idx );
        if ( !item )
            continue;

        int j = 0;
        foreach ( const query_ptr& query, t )
        {
            if ( item->query().data() == query.data() )
            {
                remove( idx );
                t.removeAt( j );
                break;
            }

            j++;
        }
    }
}
