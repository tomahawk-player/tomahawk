/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#include "PlayableProxyModel.h"

#include <QTreeView>

#include "PlayableProxyModelPlaylistInterface.h"
#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "Source.h"
#include "PlayableItem.h"
#include "utils/Logger.h"


PlayableProxyModel::PlayableProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( 0 )
    , m_showOfflineResults( true )
    , m_maxVisibleItems( -1 )
{
    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourcePlayableModel( 0 );
}


bool
PlayableProxyModel::isLoading() const
{
    if ( m_model )
    {
        return m_model->isLoading();
    }

    return false;
}


void
PlayableProxyModel::setSourceModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setSourcePlayableModel instead";
    Q_ASSERT( false );
}


void
PlayableProxyModel::setSourcePlayableModel( PlayableModel* sourceModel )
{
    if ( m_model )
    {
        if ( m_model->metaObject()->indexOfSignal( "trackCountChanged(uint)" ) > -1 )
            disconnect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), this, SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

        disconnect( m_model, SIGNAL( loadingStarted() ), this, SIGNAL( loadingStarted() ) );
        disconnect( m_model, SIGNAL( loadingFinished() ), this, SIGNAL( loadingFinished() ) );
    }

    m_model = sourceModel;

    if ( m_model )
    {
        if ( m_model->metaObject()->indexOfSignal( "trackCountChanged(uint)" ) > -1 )
            connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), playlistInterface().data(), SIGNAL( sourceTrackCountChanged( unsigned int ) ) );
        
        connect( m_model, SIGNAL( loadingStarted() ), SIGNAL( loadingStarted() ) );
        connect( m_model, SIGNAL( loadingFinished() ), SIGNAL( loadingFinished() ) );
    }

    QSortFilterProxyModel::setSourceModel( m_model );
}


bool
PlayableProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    PlayableItem* pi = itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;
    
    if ( m_maxVisibleItems >= 0 && sourceRow > m_maxVisibleItems - 1 )
        return false;

    if ( pi->query() )
    {
        const Tomahawk::query_ptr& q = pi->query()->displayQuery();
        if ( q.isNull() ) // uh oh? filter out invalid queries i guess
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
            if ( !q->artist().toLower().contains( s ) &&
                !q->album().toLower().contains( s ) &&
                !q->track().toLower().contains( s ) )
            {
                return false;
            }
        }
    }

    const Tomahawk::album_ptr& al = pi->album();
    if ( al )
    {
        QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );

        bool found = true;
        foreach( const QString& s, sl )
        {
            if ( !al->name().contains( s, Qt::CaseInsensitive ) && !al->artist()->name().contains( s, Qt::CaseInsensitive ) )
            {
                found = false;
            }
        }

        return found;
    }

    const Tomahawk::album_ptr& ar = pi->album();
    if ( ar )
    {
        QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );

        bool found = true;
        foreach( const QString& s, sl )
        {
            if ( !ar->name().contains( s, Qt::CaseInsensitive ) && !ar->artist()->name().contains( s, Qt::CaseInsensitive ) )
            {
                found = false;
            }
        }

        return found;
    }

    return true;
}


void
PlayableProxyModel::remove( const QModelIndex& index )
{
    if ( !sourceModel() )
        return;
    if ( !index.isValid() )
        return;

    sourceModel()->remove( mapToSource( index ) );
}


void
PlayableProxyModel::remove( const QModelIndexList& indexes )
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
PlayableProxyModel::remove( const QList< QPersistentModelIndex >& indexes )
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


void
PlayableProxyModel::setMaxVisibleItems( int items )
{
    if ( m_maxVisibleItems == items )
        return;

    m_maxVisibleItems = items;
    invalidateFilter();
}


bool
PlayableProxyModel::lessThan( int column, const Tomahawk::query_ptr& q1, const Tomahawk::query_ptr& q2 ) const
{
    const QString artist1 = q1->artistSortname();
    const QString artist2 = q2->artistSortname();
    const QString album1 = q1->albumSortname();
    const QString album2 = q2->albumSortname();
    const QString track1 = q1->trackSortname();
    const QString track2 = q2->trackSortname();
    const unsigned int albumpos1 = q1->albumpos();
    const unsigned int albumpos2 = q2->albumpos();
    const unsigned int discnumber1 = q1->discnumber();
    const unsigned int discnumber2 = q2->discnumber();
    unsigned int bitrate1 = 0, bitrate2 = 0;
    unsigned int mtime1 = 0, mtime2 = 0;
    unsigned int size1 = 0, size2 = 0;
    float score1 = 0, score2 = 0;
    qint64 id1 = 0, id2 = 0;

    if ( q1->numResults() )
    {
        const Tomahawk::result_ptr& r = q1->results().at( 0 );
        bitrate1 = r->bitrate();
        mtime1 = r->modificationTime();
        size1 = r->size();
        score1 = r->score();
        id1 = (qint64)&r;
    }
    if ( q2->numResults() )
    {
        const Tomahawk::result_ptr& r = q2->results().at( 0 );
        bitrate2 = r->bitrate();
        mtime2 = r->modificationTime();
        size2 = r->size();
        score2 = r->score();
        id2 = (qint64)&r;
    }

    // This makes it a stable sorter and prevents items from randomly jumping about.
    if ( id1 == id2 )
    {
        id1 = (qint64)&q1;
        id2 = (qint64)&q2;
    }

    if ( column == PlayableModel::Artist ) // sort by artist
    {
        if ( artist1 == artist2 )
        {
            if ( album1 == album2 )
            {
                if ( discnumber1 == discnumber2 )
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
    else if ( column == PlayableModel::Album ) // sort by album
    {
        if ( album1 == album2 )
        {
            if ( discnumber1 == discnumber2 )
            {
                if ( albumpos1 == albumpos2 )
                    return id1 < id2;

                return albumpos1 < albumpos2;
            }

            return discnumber1 < discnumber2;
        }

        return QString::localeAwareCompare( album1, album2 ) < 0;
    }
    else if ( column == PlayableModel::Bitrate ) // sort by bitrate
    {
        if ( bitrate1 == bitrate2 )
            return id1 < id2;

        return bitrate1 < bitrate2;
    }
    else if ( column == PlayableModel::Age ) // sort by mtime
    {
        if ( mtime1 == mtime2 )
            return id1 < id2;

        return mtime1 < mtime2;
    }
    else if ( column == PlayableModel::Filesize ) // sort by file size
    {
        if ( size1 == size2 )
            return id1 < id2;

        return size1 < size2;
    }
    else if ( column == PlayableModel::Score ) // sort by file score
    {
        if ( score1 == score2 )
            return id1 < id2;

        return score1 < score2;
    }
    else if ( column == PlayableModel::AlbumPos ) // sort by album pos
    {
        if ( discnumber1 != discnumber2 )
        {
            return discnumber1 < discnumber2;
        }
        else
        {
            if ( albumpos1 != albumpos2 )
                return albumpos1 < albumpos2;
        }
    }
    
    const QString& lefts = q1->track();
    const QString& rights = q2->track();
    if ( lefts == rights )
        return id1 < id2;

    return QString::localeAwareCompare( lefts, rights ) < 0;
}


bool
PlayableProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    PlayableItem* p1 = itemFromIndex( left );
    PlayableItem* p2 = itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    if ( p1->query() && p2->query() )
    {
        const Tomahawk::query_ptr& q1 = p1->query()->displayQuery();
        const Tomahawk::query_ptr& q2 = p2->query()->displayQuery();
        return lessThan( left.column(), q1, q2 );
    }

    return QString::localeAwareCompare( sourceModel()->data( left ).toString(), sourceModel()->data( right ).toString() ) < 0;
}


Tomahawk::playlistinterface_ptr
PlayableProxyModel::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::PlayableProxyModelPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
