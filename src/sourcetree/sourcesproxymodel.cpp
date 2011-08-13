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

#include "sourcesproxymodel.h"

#include <QTreeView>

#include "sourcesmodel.h"
#include "sourcetree/items/collectionitem.h"

#include "utils/logger.h"


SourcesProxyModel::SourcesProxyModel( SourcesModel* model, QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( model )
    , m_filtered( false )
{
    setDynamicSortFilter( true );
    setSortRole( SourcesModel::SortRole );

    setSourceModel( model );


    if ( model && model->metaObject()->indexOfSignal( "expandRequest(QModelIndex)" ) > -1 )
        connect( model, SIGNAL( expandRequest( QModelIndex ) ), this, SLOT( expandRequested( QModelIndex ) ) );
    if ( model && model->metaObject()->indexOfSignal( "selectRequest(QModelIndex)" ) > -1 )
        connect( model, SIGNAL( selectRequest( QModelIndex ) ), this, SLOT( selectRequested( QModelIndex ) ) );
}


void
SourcesProxyModel::showOfflineSources( bool offlineSourcesShown )
{
    m_filtered = !offlineSourcesShown;
    invalidateFilter();
}

bool
SourcesProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( !m_filtered )
        return true;


    CollectionItem* sti = qobject_cast< CollectionItem* >( m_model->data( sourceModel()->index( sourceRow, 0, sourceParent ), SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >() );
    if ( sti )
    {
        if ( sti->source().isNull() || sti->source()->isOnline() )
            return true;
        else
            return false;
    }
    // accept rows that aren't sources
    return true;
}

void
SourcesProxyModel::selectRequested( const QModelIndex& idx )
{
    qDebug() << "selectRequested for idx" << idx << idx.data(Qt::DisplayRole).toString() << mapFromSource( idx );
    emit selectRequest( mapFromSource( idx ) );
}

void
SourcesProxyModel::expandRequested( const QModelIndex& idx )
{
    qDebug() << "emitting expand for idx" << idx << idx.data(Qt::DisplayRole).toString() << mapFromSource( idx );
    emit expandRequest( mapFromSource( idx ) );
}

