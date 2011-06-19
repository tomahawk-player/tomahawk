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

#include "treeproxymodel.h"

#include <QDebug>
#include <QListView>

#include "query.h"


TreeProxyModel::TreeProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , PlaylistInterface( this )
    , m_model( 0 )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceModel( 0 );
}


void
TreeProxyModel::setSourceModel( TreeModel* sourceModel )
{
    m_model = sourceModel;

    if ( m_model && m_model->metaObject()->indexOfSignal( "trackCountChanged(uint)" ) > -1 )
        connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), SIGNAL( sourceTrackCountChanged( unsigned int ) ) );


    QSortFilterProxyModel::setSourceModel( sourceModel );
}


void
TreeProxyModel::setFilter( const QString& pattern )
{
    qDebug() << Q_FUNC_INFO;
    setFilterRegExp( pattern );

    emit filterChanged( pattern );
}


bool
TreeProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( filterRegExp().isEmpty() )
        return true;

    TreeModelItem* pi = sourceModel()->itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );

    bool found = true;
    foreach( const QString& s, sl )
    {
        if ( !textForItem( pi ).contains( s, Qt::CaseInsensitive ) )
        {
            found = false;
        }
    }

    return found;
}


bool
TreeProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    TreeModelItem* p1 = sourceModel()->itemFromIndex( left );
    TreeModelItem* p2 = sourceModel()->itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    if ( !p1->result().isNull() )
    {
        if ( p1->result()->albumpos() != p2->result()->albumpos() )
            return p1->result()->albumpos() < p2->result()->albumpos();
    }

    return QString::localeAwareCompare( textForItem( p1 ), textForItem( p2 ) ) < 0;
}


void
TreeProxyModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( !sourceModel() )
        return;
    if ( index.column() > 0 )
        return;

    sourceModel()->removeIndex( mapToSource( index ) );
}


void
TreeProxyModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    if ( !sourceModel() )
        return;

    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


Tomahawk::result_ptr
TreeProxyModel::siblingItem( int itemsAway )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = currentItem();

    // Try to find the next available PlaylistItem (with results)
    if ( idx.isValid() ) do
    {
        idx = index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
        if ( !idx.isValid() )
            break;

        TreeModelItem* item = itemFromIndex( mapToSource( idx ) );
        if ( item && item->result()->isOnline() )
        {
            qDebug() << "Next PlaylistItem found:" << item->result()->url();
            setCurrentItem( idx );
            return item->result();
        }
    }
    while ( idx.isValid() );

    setCurrentItem( QModelIndex() );
    return Tomahawk::result_ptr();
}


QString
TreeProxyModel::textForItem( TreeModelItem* item ) const
{
    if ( !item->artist().isNull() )
    {
        return item->artist()->name();
    }
    else if ( !item->album().isNull() )
    {
        return item->album()->name();
    }
    else if ( !item->result().isNull() )
    {
        return item->result()->track();
    }

    return QString();
}
