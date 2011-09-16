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

#include "JobStatusModel.h"
#include "JobStatusItem.h"

JobStatusModel::JobStatusModel( QObject* parent )
    : QAbstractListModel ( parent )
{

}

JobStatusModel::~JobStatusModel()
{
    qDeleteAll( m_items );
    m_collapseCount.clear();
}

void
JobStatusModel::addJob( JobStatusItem* item )
{
    if ( item->collapseItem() )
    {
        if ( m_collapseCount.contains( item->type() ) )
        {
            m_collapseCount[ item->type() ].append( item );
            return; // we're done, no new rows
        }
        else
        {
            m_collapseCount.insert( item->type(), QList< JobStatusItem* >() << item );
        }

    }

    connect( item, SIGNAL( statusChanged() ), this, SLOT( itemUpdated() ) );
    connect( item, SIGNAL( finished() ), this, SLOT( itemFinished() ) );

    beginInsertRows( QModelIndex(), 0, 0 );
    m_items.prepend( item );
    endInsertRows();
}

Qt::ItemFlags
JobStatusModel::flags( const QModelIndex& index ) const
{
    // Don't let the items be selectable
    return Qt::ItemIsEnabled;
}


QVariant
JobStatusModel::data( const QModelIndex& index, int role ) const
{
    if ( !hasIndex( index.row(), index.column(), index.parent() ) )
        return QVariant();

    JobStatusItem* item = m_items[ index.row() ];

    switch ( role )
    {
    case Qt::DecorationRole:
        return item->icon();
    case Qt::DisplayRole:
    {
        if ( m_collapseCount.contains( item->type() ) )
            return m_collapseCount[ item->type() ].last()->mainText();
        else
            return item->mainText();
    }
    case RightColumnRole:
    {
        if ( m_collapseCount.contains( item->type() ) )
            return m_collapseCount[ item->type() ].count();
        else
            return item->rightColumnText();
    }
    }

    return QVariant();
}

int
JobStatusModel::rowCount( const QModelIndex& parent ) const
{
    return m_items.count();
}


void
JobStatusModel::itemFinished()
{
    JobStatusItem* item = qobject_cast< JobStatusItem* >( sender() );
    Q_ASSERT( item );

    if ( m_collapseCount.contains( item->type() ) )
    {
        m_collapseCount[ item->type() ].removeAll( item );
        if ( m_collapseCount[ item->type() ].isEmpty() )
            m_collapseCount.remove( item->type() );
        else
        {
            // One less to count, but item is still there
            const QModelIndex idx = index( m_items.indexOf( m_collapseCount[ item->type() ].first() ), 0, QModelIndex() );
            emit dataChanged( idx, idx );
            return;
        }
    }

    // Remove row completely
    const int idx = m_items.indexOf( item );
    beginRemoveRows( QModelIndex(), idx, idx );
    m_items.removeAll( item );
    endRemoveRows();

    item->deleteLater();
}

void
JobStatusModel::itemUpdated()
{
    JobStatusItem* item = qobject_cast< JobStatusItem* >( sender() );
    Q_ASSERT( item );

    if ( m_collapseCount.contains( item->type() ) )
        item = m_collapseCount[ item->type() ].first();

    const QModelIndex idx = index( m_items.indexOf( item ), 0, QModelIndex() );
    emit dataChanged( idx, idx );
    return;
}
