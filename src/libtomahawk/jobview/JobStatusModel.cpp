/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "JobStatusModel.h"

#include "JobStatusItem.h"
#include "utils/Logger.h"

#include <QPixmap>


JobStatusSortModel::JobStatusSortModel( QObject* parent )
    : QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( true );
}


JobStatusSortModel::~JobStatusSortModel()
{
}


void
JobStatusSortModel::setJobModel( JobStatusModel* model )
{
    setSourceModel( model );

    m_sourceModel = model;

    connect( m_sourceModel, SIGNAL( customDelegateJobInserted( int, JobStatusItem* ) ), this, SLOT( customDelegateJobInsertedSlot( int, JobStatusItem* ) ) );
    connect( m_sourceModel, SIGNAL( customDelegateJobRemoved( int ) ), this, SLOT( customDelegateJobRemovedSlot( int ) ) );
    connect( m_sourceModel, SIGNAL( refreshDelegates() ), this, SLOT( refreshDelegatesSlot() ) );
}


void
JobStatusSortModel::addJob( JobStatusItem* item )
{
    m_sourceModel->addJob( item );
}


void
JobStatusSortModel::customDelegateJobInsertedSlot( int row, JobStatusItem* item )
{
    emit customDelegateJobInserted( mapFromSource( m_sourceModel->index( row ) ).row(), item );
}


void
JobStatusSortModel::customDelegateJobRemovedSlot( int row )
{
    emit customDelegateJobRemoved( mapFromSource( m_sourceModel->index( row ) ).row() );
}


void
JobStatusSortModel::refreshDelegatesSlot()
{
    sort( 0 );
    emit refreshDelegates();
}


bool
JobStatusSortModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    const int leftSort = left.data( JobStatusModel::SortRole ).toInt();
    const int rightSort = right.data( JobStatusModel::SortRole ).toInt();

    if ( leftSort == rightSort )
        return left.data( JobStatusModel::AgeRole ).toUInt() > right.data( JobStatusModel::AgeRole ).toUInt();

    return leftSort < rightSort;
}



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
//    tLog() << Q_FUNC_INFO << "current jobs of item type: " << m_jobTypeCount[ item->type() ] << ", current queue size of item type: " << m_jobQueue[ item->type() ].size();
    if ( item->concurrentJobLimit() > 0 )
    {
        if ( m_jobTypeCount[ item->type() ] >= item->concurrentJobLimit() )
        {
            m_jobQueue[ item->type() ].enqueue( item );
            return;
        }
        int currentJobCount = m_jobTypeCount[ item->type() ];
        currentJobCount++;
        m_jobTypeCount[ item->type() ] = currentJobCount;
    }

//    tLog() << Q_FUNC_INFO << "new current jobs of item type: " << m_jobTypeCount[ item->type() ];

    connect( item, SIGNAL( statusChanged() ), SLOT( itemUpdated() ) );
    connect( item, SIGNAL( finished() ), SLOT( itemFinished() ) );

    if ( item->collapseItem() )
    {
        if ( m_collapseCount.contains( item->type() ) )
        {
            m_collapseCount[ item->type() ].append( item );
//             qDebug() << "Adding item:" << item << "TO COLLAPSE ONLY";
            return; // we're done, no new rows
        }
        else
        {
            m_collapseCount.insert( item->type(), QList< JobStatusItem* >() << item );
        }

    }
//    tLog() << Q_FUNC_INFO << "Adding item:" << item;

    int currentEndRow = m_items.count();
    beginInsertRows( QModelIndex(), currentEndRow, currentEndRow );
    m_items.append( item );
    endInsertRows();

    if ( item->hasCustomDelegate() )
    {
//        tLog() << Q_FUNC_INFO << "job has custom delegate";
        emit customDelegateJobInserted( currentEndRow, item );
    }

    emit refreshDelegates();
}


Qt::ItemFlags
JobStatusModel::flags( const QModelIndex& index ) const
{
    Q_UNUSED( index );
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

        case Qt::ToolTipRole:

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

        case AllowMultiLineRole:
            return item->allowMultiLine();

        case JobDataRole:
            return QVariant::fromValue< JobStatusItem* >( item );

        case SortRole:
            return item->weight();

        case AgeRole:
            return item->age();

        default:
            return QVariant();
    }

    return QVariant();
}


int
JobStatusModel::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return m_items.count();
}


void
JobStatusModel::itemFinished()
{
//    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
    JobStatusItem* item = qobject_cast< JobStatusItem* >( sender() );
    Q_ASSERT( item );

//     tDebug() << "Got item finished:" << item->type() << item->mainText() << item;
    if ( !m_items.contains( item ) && !m_collapseCount.contains( item->type() ) )
        return;

//     foreach( JobStatusItem* item, m_items )
//     {
//         qDebug() << "ITEM #:" << item;
//     }
//     foreach( const QString& str, m_collapseCount.keys() )
//     {
//         tDebug() << "\t" << str;
//         foreach( JobStatusItem* chain, m_collapseCount[ str ] )
//             qDebug() << "\t\t" << chain;
//     }
    if ( m_collapseCount.contains( item->type() ) )
    {
        const int indexOf = m_items.indexOf( m_collapseCount[ item->type() ].first() );
//         tDebug() << "index in main list of collapsed irst item:" << indexOf;
        if ( m_collapseCount[ item->type() ].first() == item &&
             m_items.contains( m_collapseCount[ item->type() ].first() ) && m_collapseCount[ item->type() ].size() > 1 )
        {
            // the placeholder we use that links m_items and m_collapsecount is done, so choose another one
            m_items.replace( m_items.indexOf( m_collapseCount[ item->type() ].first() ), m_collapseCount[ item->type() ][ 1 ] );
//             qDebug() << "Replaced" << m_collapseCount[ item->type() ].first() << "with:" << m_collapseCount[ item->type() ][ 1 ] << m_items;
        }
        m_collapseCount[ item->type() ].removeAll( item );
//         tDebug() << "New collapse count list:" << m_collapseCount[ item->type() ];
        if ( m_collapseCount[ item->type() ].isEmpty() )
            m_collapseCount.remove( item->type() );
        else
        {
            // One less to count, but item is still there
            const QModelIndex idx = index( indexOf, 0, QModelIndex() );
            emit dataChanged( idx, idx );
            emit refreshDelegates();
            return;
        }
    }

    // Remove row completely
    const int idx = m_items.indexOf( item );
//     tDebug() << "Got index of item:" << idx;
    Q_ASSERT( idx >= 0 );

    beginRemoveRows( QModelIndex(), idx, idx );
    m_items.removeAll( item );
    endRemoveRows();

    if ( item->customDelegate() )
        emit customDelegateJobRemoved( idx );

    emit refreshDelegates();

//    tLog() << Q_FUNC_INFO << "current jobs of item type: " << m_jobTypeCount[ item->type() ] << ", current queue size of item type: " << m_jobQueue[ item->type() ].size();
    if ( item->concurrentJobLimit() > 0 )
    {
        int currentJobs = m_jobTypeCount[ item->type() ];
        currentJobs--;
        m_jobTypeCount[ item->type() ] = currentJobs;

        if ( !m_jobQueue[ item->type() ].isEmpty() )
        {
            JobStatusItem* item = m_jobQueue[ item->type() ].dequeue();
            QMetaObject::invokeMethod( this, "addJob", Qt::QueuedConnection, Q_ARG( JobStatusItem*, item ) );
        }
    }

    item->deleteLater();
}


void
JobStatusModel::itemUpdated()
{
//    tLog( LOGVERBOSE ) << Q_FUNC_INFO;
    JobStatusItem* item = qobject_cast< JobStatusItem* >( sender() );
    Q_ASSERT( item );

    if ( m_collapseCount.contains( item->type() ) )
        item = m_collapseCount[ item->type() ].first();

    const QModelIndex idx = index( m_items.indexOf( item ), 0, QModelIndex() );
    emit dataChanged( idx, idx );
}
