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

#include "trackproxymodel.h"

#include <QDebug>
#include <QTreeView>

#include "album.h"
#include "query.h"


TrackProxyModel::TrackProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , PlaylistInterface( this )
    , m_model( 0 )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
    , m_showOfflineResults( true )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceTrackModel( 0 );
}


void
TrackProxyModel::setSourceModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setSourceTrackModel instead";
    Q_ASSERT( false );
}


void
TrackProxyModel::setSourceTrackModel( TrackModel* sourceModel )
{
    m_model = sourceModel;

    if ( m_model && m_model->metaObject()->indexOfSignal( "trackCountChanged(uint)" ) > -1 )
        connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

    QSortFilterProxyModel::setSourceModel( m_model );
}


void
TrackProxyModel::setFilter( const QString& pattern )
{
    qDebug() << Q_FUNC_INFO;
    PlaylistInterface::setFilter( pattern );
    setFilterRegExp( pattern );

    emit filterChanged( pattern );
    emit trackCountChanged( trackCount() );
}


QList< Tomahawk::query_ptr >
TrackProxyModel::tracks()
{
    QList<Tomahawk::query_ptr> queries;

    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        TrackModelItem* item = itemFromIndex( mapToSource( index( i, 0 ) ) );
        if ( item )
            queries << item->query();
    }

    return queries;
}


Tomahawk::result_ptr
TrackProxyModel::siblingItem( int itemsAway )
{
    return siblingItem( itemsAway, false );
}


bool
TrackProxyModel::hasNextItem()
{
    return !( siblingItem( 1, true ).isNull() );
}


Tomahawk::result_ptr
TrackProxyModel::siblingItem( int itemsAway, bool readOnly )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = index( 0, 0 );
    if( rowCount() )
    {
        if ( m_shuffled )
        {
            // random mode is enabled
            // TODO come up with a clever random logic, that keeps track of previously played items
            idx = index( qrand() % rowCount(), 0 );
        }
        else if ( currentIndex().isValid() )
        {
            idx = currentIndex();

            // random mode is disabled
            if ( m_repeatMode == PlaylistInterface::RepeatOne )
            {
                // repeat one track
                idx = index( idx.row(), 0 );
            }
            else
            {
                // keep progressing through the playlist normally
                idx = index( idx.row() + itemsAway, 0 );
            }
        }
    }

    if ( !idx.isValid() && m_repeatMode == PlaylistInterface::RepeatAll )
    {
        // repeat all tracks
        if ( itemsAway > 0 )
        {
            // reset to first item
            idx = index( 0, 0 );
        }
        else
        {
            // reset to last item
            idx = index( rowCount() - 1, 0 );
        }
    }

    // Try to find the next available PlaylistItem (with results)
    if ( idx.isValid() ) do
    {
        TrackModelItem* item = itemFromIndex( mapToSource( idx ) );
        if ( item && item->query()->playable() )
        {
            qDebug() << "Next PlaylistItem found:" << item->query()->toString() << item->query()->results().at( 0 )->url();
            if ( !readOnly )
                setCurrentIndex( idx );
            return item->query()->results().at( 0 );
        }

        idx = index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0 );
    }
    while ( idx.isValid() );

    if ( !readOnly )
        setCurrentIndex( QModelIndex() );
    return Tomahawk::result_ptr();
}


Tomahawk::result_ptr
TrackProxyModel::currentItem() const
{
    TrackModelItem* item = itemFromIndex( mapToSource( currentIndex() ) );
    if ( item && item->query()->playable() )
        return item->query()->results().at( 0 );
    return Tomahawk::result_ptr();
}

bool
TrackProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    TrackModelItem* pi = itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    const Tomahawk::query_ptr& q = pi->query();
    if( q.isNull() ) // uh oh? filter out invalid queries i guess
        return false;

    Tomahawk::result_ptr r;
    if ( q->numResults() )
        r = q->results().first();

    if ( !m_showOfflineResults && !r.isNull() && !r->isOnline() )
        return false;

    if ( filterRegExp().isEmpty() )
        return true;

    QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );
    foreach( QString s, sl )
    {
        s = s.toLower();
        if ( !r.isNull() )
        {
            if ( !r->artist()->name().toLower().contains( s ) &&
                 !r->album()->name().toLower().contains( s ) &&
                 !r->track().toLower().contains( s ) )
            {
                return false;
            }
        }
        else
        {
            if ( !q->artist().toLower().contains( s ) &&
                 !q->album().toLower().contains( s ) &&
                 !q->track().toLower().contains( s ) )
            {
                return false;
            }
        }
    }

    return true;
}


void
TrackProxyModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( !sourceModel() )
        return;
    if ( !index.isValid() )
        return;

    sourceModel()->removeIndex( mapToSource( index ) );
}


void
TrackProxyModel::removeIndexes( const QModelIndexList& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach( const QModelIndex& idx, indexes )
    {
        if ( idx.isValid() && idx.column() == 0 )
            pil << mapToSource( idx );
    }

    bool b = true;
    foreach( const QPersistentModelIndex& idx, pil )
    {
        if ( idx == pil.last() )
            b = false;

        qDebug() << "b is:" << b;
        sourceModel()->removeIndex( idx, b );
    }
}


void
TrackProxyModel::removeIndexes( const QList<QPersistentModelIndex>& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach( const QModelIndex& idx, indexes )
    {
        if ( idx.isValid() && idx.column() == 0 )
            pil << mapToSource( idx );
    }

    bool b = true;
    foreach( const QPersistentModelIndex& idx, pil )
    {
        if ( idx == pil.last() )
            b = false;

        qDebug() << "b is:" << b;
        sourceModel()->removeIndex( idx, b );
    }
}


bool
TrackProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    qDebug() << Q_FUNC_INFO;

    TrackModelItem* p1 = itemFromIndex( left );
    TrackModelItem* p2 = itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    const Tomahawk::query_ptr& q1 = p1->query();
    const Tomahawk::query_ptr& q2 = p2->query();

    QString artist1 = q1->artist();
    QString artist2 = q2->artist();
    QString album1 = q1->album();
    QString album2 = q2->album();
    QString track1 = q1->track();
    QString track2 = q2->track();
    unsigned int albumpos1 = 0, albumpos2 = 0;
    unsigned int bitrate1 = 0, bitrate2 = 0;
    unsigned int mtime1 = 0, mtime2 = 0;
    unsigned int id1 = 0, id2 = 0;
    unsigned int size1 = 0, size2 = 0;

    if ( q1->numResults() )
    {
        const Tomahawk::result_ptr& r = q1->results().at( 0 );
        artist1 = r->artist()->name();
        album1 = r->album()->name();
        track1 = r->track();
        albumpos1 = r->albumpos();
        bitrate1 = r->bitrate();
        mtime1 = r->modificationTime();
        id1 = r->dbid();
        size1 = r->size();
    }
    if ( q2->numResults() )
    {
        const Tomahawk::result_ptr& r = q2->results().at( 0 );
        artist2 = r->artist()->name();
        album2 = r->album()->name();
        track2 = r->track();
        albumpos2 = r->albumpos();
        bitrate2 = r->bitrate();
        mtime2 = r->modificationTime();
        id2 = r->dbid();
        size2 = r->size();
    }

    if ( left.column() == TrackModel::Artist ) // sort by artist
    {
        if ( artist1 == artist2 )
        {
            if ( album1 == album2 )
            {
                if ( albumpos1 == albumpos2 )
                    return id1 < id2;

                return albumpos1 < albumpos2;
            }

            return QString::localeAwareCompare( album1, album2 ) < 0;
        }

        return QString::localeAwareCompare( artist1, artist2 ) < 0;
    }
    else if ( left.column() == TrackModel::Album ) // sort by album
    {
        if ( album1 == album2 )
        {
            if ( albumpos1 == albumpos2 )
                return id1 < id2;

            return albumpos1 < albumpos2;
        }

        return QString::localeAwareCompare( album1, album2 ) < 0;
    }
    else if ( left.column() == TrackModel::Bitrate ) // sort by bitrate
    {
        if ( bitrate1 == bitrate2 )
            return id1 < id2;

        return bitrate1 < bitrate2;
    }
    else if ( left.column() == TrackModel::Age ) // sort by mtime
    {
        if ( mtime1 == mtime2 )
            return id1 < id2;

        return mtime1 < mtime2;
    }
    else if ( left.column() == TrackModel::Filesize ) // sort by file size
    {
        if ( size1 == size2 )
            return id1 < id2;

        return size1 < size2;
    }
    return QString::localeAwareCompare( sourceModel()->data( left ).toString(),
                                        sourceModel()->data( right ).toString() ) < 0;
}
