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

#include <QTreeView>

#include "trackproxymodelplaylistinterface.h"
#include "artist.h"
#include "album.h"
#include "query.h"
#include "utils/logger.h"


TrackProxyModel::TrackProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( 0 )
    , m_showOfflineResults( true )
{
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
        connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), playlistInterface().data(), SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

    QSortFilterProxyModel::setSourceModel( m_model );
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
TrackProxyModel::remove( const QModelIndex& index )
{
    if ( !sourceModel() )
        return;
    if ( !index.isValid() )
        return;

    sourceModel()->remove( mapToSource( index ) );
}


void
TrackProxyModel::remove( const QModelIndexList& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach ( const QModelIndex& idx, indexes )
    {
        if ( idx.isValid() )
            pil << mapToSource( idx );
    }

    sourceModel()->remove( pil );
}


void
TrackProxyModel::remove( const QList< QPersistentModelIndex >& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach ( const QPersistentModelIndex& idx, indexes )
    {
        if ( idx.isValid() )
            pil << mapToSource( idx );
    }

    sourceModel()->remove( pil );
}


bool
TrackProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    TrackModelItem* p1 = itemFromIndex( left );
    TrackModelItem* p2 = itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    const Tomahawk::query_ptr& q1 = p1->query();
    const Tomahawk::query_ptr& q2 = p2->query();

    QString artist1 = q1->artistSortname();
    QString artist2 = q2->artistSortname();
    QString album1 = q1->album();
    QString album2 = q2->album();
    QString track1 = q1->track();
    QString track2 = q2->track();
    unsigned int albumpos1 = 0, albumpos2 = 0;
    unsigned int discnumber1 = 0, discnumber2 = 0;
    unsigned int bitrate1 = 0, bitrate2 = 0;
    unsigned int mtime1 = 0, mtime2 = 0;
    unsigned int size1 = 0, size2 = 0;
    qint64 id1 = 0, id2 = 0;

    if ( q1->numResults() )
    {
        const Tomahawk::result_ptr& r = q1->results().at( 0 );
        artist1 = r->artist()->sortname();
        album1 = r->album()->name();
        track1 = r->track();
        albumpos1 = r->albumpos();
        discnumber1 = r->discnumber();
        bitrate1 = r->bitrate();
        mtime1 = r->modificationTime();
        id1 = r->trackId();
        size1 = r->size();
    }
    if ( q2->numResults() )
    {
        const Tomahawk::result_ptr& r = q2->results().at( 0 );
        artist2 = r->artist()->sortname();
        album2 = r->album()->name();
        track2 = r->track();
        albumpos2 = r->albumpos();
        discnumber2 = r->discnumber();
        bitrate2 = r->bitrate();
        mtime2 = r->modificationTime();
        id2 = r->trackId();
        size2 = r->size();
    }

    // This makes it a stable sorter and prevents items from randomly jumping about.
    if ( id1 == id2 )
    {
        id1 = (qint64)&q1;
        id2 = (qint64)&q2;
    }

    if ( left.column() == TrackModel::Artist ) // sort by artist
    {
        if ( artist1 == artist2 )
        {
            if ( album1 == album2 )
            {
                if( discnumber1 == discnumber2 )
                {
                    if ( albumpos1 == albumpos2 )
                        return id1 < id2;

                    return albumpos1 < albumpos2;
                }

                return discnumber1 < discnumber2;
            }

            return QString::localeAwareCompare( album1, album2 ) < 0;
        }

        return QString::localeAwareCompare( artist1, artist2 ) < 0;
    }
    else if ( left.column() == TrackModel::Album ) // sort by album
    {
        if ( album1 == album2 )
        {
            if( discnumber1 == discnumber2 )
            {
                if ( albumpos1 == albumpos2 )
                    return id1 < id2;

                return albumpos1 < albumpos2;
            }

            return discnumber1 < discnumber2;
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

    const QString& lefts = sourceModel()->data( left ).toString();
    const QString& rights = sourceModel()->data( right ).toString();
    if ( lefts == rights )
        return id1 < id2;

    return QString::localeAwareCompare( lefts, rights ) < 0;
}


Tomahawk::playlistinterface_ptr
TrackProxyModel::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::TrackProxyModelPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
