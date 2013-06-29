/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "SourcesProxyModel.h"

#include "SourceList.h"
#include "SourcesModel.h"
#include "sourcetree/items/SourceItem.h"

#include "utils/Logger.h"

#include <QTreeView>

SourcesProxyModel::SourcesProxyModel( SourcesModel* model, QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( model )
    , m_filtered( false )
{
    setDynamicSortFilter( true );
    setSortRole( SourcesModel::SortRole );

    setSourceModel( model );

    connect( model, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onModelChanged() ) );
    connect( model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( onModelChanged() ) );

    if ( model && model->metaObject()->indexOfSignal( "toggleExpandRequest(QPersistentModelIndex)" ) > -1 )
        connect( model, SIGNAL( toggleExpandRequest( QPersistentModelIndex ) ), this, SLOT( toggleExpandRequested( QPersistentModelIndex ) ), Qt::QueuedConnection );
    if ( model && model->metaObject()->indexOfSignal( "expandRequest(QPersistentModelIndex)" ) > -1 )
        connect( model, SIGNAL( expandRequest( QPersistentModelIndex ) ), this, SLOT( expandRequested( QPersistentModelIndex ) ), Qt::QueuedConnection );
    if ( model && model->metaObject()->indexOfSignal( "selectRequest(QPersistentModelIndex)" ) > -1 )
        connect( model, SIGNAL( selectRequest( QPersistentModelIndex ) ), this, SLOT( selectRequested( QPersistentModelIndex ) ), Qt::QueuedConnection );
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
    // always filter empty top-level groups
    SourceTreeItem* item = m_model->data( sourceModel()->index( sourceRow, 0, sourceParent ), SourcesModel::SourceTreeItemRole ).value< SourceTreeItem* >();

    if ( item && item->type() != SourcesModel::Divider && item->parent()->parent() == 0 && !item->children().count() )
        return false;

    if ( !m_filtered )
        return true;

    SourceItem* sti = qobject_cast< SourceItem* >( item );
    if ( sti )
    {
        if ( sti->source().isNull() || sti->source()->isOnline() )
            return true;
        else if ( m_model->sourcesWithViewPage().contains( sti->source() ) )
            return true;
        else
            return false;
    }

    // accept rows that aren't sources
    return true;
}


void
SourcesProxyModel::selectRequested( const QPersistentModelIndex& idx )
{
    emit selectRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


void
SourcesProxyModel::expandRequested( const QPersistentModelIndex& idx )
{
    emit expandRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


void
SourcesProxyModel::toggleExpandRequested( const QPersistentModelIndex& idx )
{
    emit toggleExpandRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


bool
SourcesProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    if ( m_model->data( left, SourcesModel::SortRole ) != m_model->data( right, SourcesModel::SortRole ) )
        return ( m_model->data( left, SourcesModel::SortRole ).toInt() < m_model->data( right, SourcesModel::SortRole ).toInt() );

    const QString& lefts = left.data().toString().toLower();
    const QString& rights = right.data().toString().toLower();

    if ( lefts == rights )
        return ( m_model->data( left, SourcesModel::IDRole ).toInt() < m_model->data( right, SourcesModel::IDRole ).toInt() );
    else
        return QString::localeAwareCompare( lefts, rights ) < 0;
}


void
SourcesProxyModel::onModelChanged()
{
    invalidateFilter();
}
