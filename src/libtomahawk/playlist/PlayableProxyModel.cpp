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

#include "utils/Logger.h"

#include "Artist.h"
#include "Album.h"
#include "PlayableItem.h"
#include "PlayableProxyModelPlaylistInterface.h"
#include "Query.h"
#include "Result.h"
#include "Source.h"

#include <QTreeView>

PlayableProxyModel::PlayableProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( 0 )
    , m_showOfflineResults( true )
    , m_hideEmptyParents( true )
    , m_hideDupeItems( false )
    , m_maxVisibleItems( -1 )
    , m_style( Detailed )
{
    m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::PlayableProxyModelPlaylistInterface( this ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    PlayableProxyModel::setSourcePlayableModel( NULL );

    m_headerStyle[ SingleColumn ] << PlayableModel::Name;
    m_headerStyle[ Detailed ]     << PlayableModel::Artist << PlayableModel::Track << PlayableModel::Composer << PlayableModel::Album << PlayableModel::AlbumPos << PlayableModel::Duration << PlayableModel::Bitrate << PlayableModel::Age << PlayableModel::Year << PlayableModel::Filesize << PlayableModel::Origin << PlayableModel::Score;
    m_headerStyle[ Collection ]   << PlayableModel::Artist << PlayableModel::Track << PlayableModel::Composer << PlayableModel::Album << PlayableModel::AlbumPos << PlayableModel::Duration << PlayableModel::Bitrate << PlayableModel::Age << PlayableModel::Year << PlayableModel::Filesize;
    m_headerStyle[ Locker ]       << PlayableModel::Artist << PlayableModel::Track << PlayableModel::Composer << PlayableModel::Album << PlayableModel::Download << PlayableModel::AlbumPos << PlayableModel::Duration << PlayableModel::Bitrate << PlayableModel::Age << PlayableModel::Year << PlayableModel::Filesize;
}


Tomahawk::playlistinterface_ptr
PlayableProxyModel::playlistInterface() const
{
    return m_playlistInterface;
}


void
PlayableProxyModel::setPlaylistInterface( const Tomahawk::playlistinterface_ptr& playlistInterface )
{
    m_playlistInterface = playlistInterface;
}


QString
PlayableProxyModel::guid() const
{
    if ( m_model )
    {
        return m_model->guid();
    }
    else
        return QString();
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


QPersistentModelIndex
PlayableProxyModel::currentIndex() const
{
    if ( !m_model )
        return QPersistentModelIndex();

    return mapFromSource( m_model->currentItem() );
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
        disconnect( m_model, SIGNAL( loadingStarted() ), this, SIGNAL( loadingStarted() ) );
        disconnect( m_model, SIGNAL( loadingFinished() ), this, SIGNAL( loadingFinished() ) );
        disconnect( m_model, SIGNAL( itemCountChanged( unsigned int ) ), this, SIGNAL( itemCountChanged( unsigned int ) ) );
        disconnect( m_model, SIGNAL( indexPlayable( QModelIndex ) ), this, SLOT( onIndexPlayable( QModelIndex ) ) );
        disconnect( m_model, SIGNAL( indexResolved( QModelIndex ) ), this, SLOT( onIndexResolved( QModelIndex ) ) );
        disconnect( m_model, SIGNAL( currentIndexChanged( QModelIndex, QModelIndex ) ), this, SLOT( onCurrentIndexChanged( QModelIndex, QModelIndex ) ) );
        disconnect( m_model, SIGNAL( expandRequest( QPersistentModelIndex ) ), this, SLOT( expandRequested( QPersistentModelIndex ) ) );
        disconnect( m_model, SIGNAL( selectRequest( QPersistentModelIndex ) ), this, SLOT( selectRequested( QPersistentModelIndex ) ) );
    }

    m_model = sourceModel;
    if ( m_model )
    {
        connect( m_model, SIGNAL( loadingStarted() ), SIGNAL( loadingStarted() ) );
        connect( m_model, SIGNAL( loadingFinished() ), SIGNAL( loadingFinished() ) );
        connect( m_model, SIGNAL( itemCountChanged( unsigned int ) ), SIGNAL( itemCountChanged( unsigned int ) ) );
        connect( m_model, SIGNAL( indexPlayable( QModelIndex ) ), SLOT( onIndexPlayable( QModelIndex ) ) );
        connect( m_model, SIGNAL( indexResolved( QModelIndex ) ), SLOT( onIndexResolved( QModelIndex ) ) );
        connect( m_model, SIGNAL( currentIndexChanged( QModelIndex, QModelIndex ) ), SLOT( onCurrentIndexChanged( QModelIndex, QModelIndex ) ) );
        connect( m_model, SIGNAL( expandRequest( QPersistentModelIndex ) ), SLOT( expandRequested( QPersistentModelIndex ) ) );
        connect( m_model, SIGNAL( selectRequest( QPersistentModelIndex ) ), SLOT( selectRequested( QPersistentModelIndex ) ) );
    }

    QSortFilterProxyModel::setSourceModel( m_model );
}


bool
PlayableProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    PlayableProxyModelFilterMemo memo;

    PlayableItem* pi = itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    return filterAcceptsRowInternal( sourceRow, pi, sourceParent, memo );
}


bool
PlayableProxyModel::filterAcceptsRowInternal( int sourceRow, PlayableItem* pi, const QModelIndex& sourceParent, PlayableProxyModelFilterMemo& memo ) const
{
    if ( m_maxVisibleItems > 0 && !visibilityFilterAcceptsRow( sourceRow, sourceParent, memo ) )
        return false;
    if ( m_hideDupeItems && !dupeFilterAcceptsRow( sourceRow, pi, sourceParent, memo ) )
        return false;

    return nameFilterAcceptsRow( sourceRow, pi, sourceParent );
}


bool
PlayableProxyModel::dupeFilterAcceptsRow( int sourceRow, PlayableItem* pi, const QModelIndex& sourceParent, PlayableProxyModelFilterMemo& memo ) const
{
    if ( !m_hideDupeItems )
        return true;

    for ( int i = 0; i < sourceRow; i++ )
    {
        PlayableItem* di = itemFromIndex( sourceModel()->index( i, 0, sourceParent ) );
        if ( !di )
            continue;

        bool b = ( pi->query() && pi->query()->equals( di->query() ) ) ||
                 ( pi->album() && pi->album() == di->album() ) ||
                 ( pi->artist() && pi->artist()->name() == di->artist()->name() );

        if ( b && filterAcceptsRowInternal( i, di, sourceParent, memo ) )
            return false;
    }

    return true;
}


bool
PlayableProxyModel::visibilityFilterAcceptsRow( int sourceRow, const QModelIndex& sourceParent, PlayableProxyModelFilterMemo& memo ) const
{
    if ( m_maxVisibleItems <= 0 )
        return true;

    if ( static_cast<size_t>( sourceRow ) < memo.visibilty.size() )
    {
        // We have already memoized the return value.
        return memo.visibilty[sourceRow] < m_maxVisibleItems;
    }
    // else: We do not have memoized the value, so compute it.

    int items = memo.visibilty.back();
    for ( int i = memo.visibilty.size() - 1; ( i < sourceRow ) && ( items < m_maxVisibleItems ) ; i++ )
    {
        PlayableItem* pi = itemFromIndex( sourceModel()->index( i, 0, sourceParent ) );
        // We will not change memo in these calls as all values needed in them are already memoized.
        if ( pi && dupeFilterAcceptsRow( i, pi, sourceParent, memo ) && nameFilterAcceptsRow( i, pi, sourceParent ) )
        {
            items++;
            memo.visibilty.push_back( items ); // Sets memo.visibilty[i + 1] to items
        }
    }

    return ( items < m_maxVisibleItems );
}


bool
PlayableProxyModel::nameFilterAcceptsRow( int sourceRow, PlayableItem* pi, const QModelIndex& sourceParent ) const
{
    if ( m_hideEmptyParents && pi->source() )
    {
        if ( !sourceModel()->rowCount( sourceModel()->index( sourceRow, 0, sourceParent ) ) )
        {
            return false;
        }
    }

    const Tomahawk::query_ptr& query = pi->query();
    if ( query )
    {
        Tomahawk::result_ptr r;
        if ( query->numResults() )
            r = query->results().first();

        if ( !m_showOfflineResults && ( r.isNull() || !r->isOnline() ) )
            return false;

        const QRegExp regexp = filterRegExp();
        if ( regexp.isEmpty() )
            return true;

        QStringList sl = regexp.pattern().split( " ", QString::SkipEmptyParts );
        foreach( const QString& s, sl )
        {
            const Tomahawk::track_ptr& track = query->track();
            if ( !track->artist().contains( s, Qt::CaseInsensitive ) &&
                 !track->album().contains( s, Qt::CaseInsensitive ) &&
                 !track->track().contains( s, Qt::CaseInsensitive ) )
            {
                return false;
            }
        }
    }

    const Tomahawk::album_ptr& al = pi->album();
    if ( al )
    {
        QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );

        foreach( const QString& s, sl )
        {
            if ( !al->name().contains( s, Qt::CaseInsensitive ) &&
                 !al->artist()->name().contains( s, Qt::CaseInsensitive ) )
            {
                return false;
            }
        }

        return true;
    }

    const Tomahawk::artist_ptr& ar = pi->artist();
    if ( ar )
    {
        QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );

        foreach( const QString& s, sl )
        {
            if ( !ar->name().contains( s, Qt::CaseInsensitive ) )
            {
                return false;
            }
        }

        return true;
    }

    return true;
}


void
PlayableProxyModel::removeIndex( const QModelIndex& index )
{
    if ( !sourceModel() )
        return;
    if ( !index.isValid() )
        return;

    sourceModel()->removeIndex( mapToSource( index ) );
}


void
PlayableProxyModel::removeIndexes( const QModelIndexList& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach ( const QModelIndex& idx, indexes )
    {
        if ( idx.isValid() )
            pil << mapToSource( idx );
    }

    sourceModel()->removeIndexes( pil );
}


void
PlayableProxyModel::removeIndexes( const QList< QPersistentModelIndex >& indexes )
{
    if ( !sourceModel() )
        return;

    QList<QPersistentModelIndex> pil;
    foreach ( const QPersistentModelIndex& idx, indexes )
    {
        if ( idx.isValid() )
            pil << mapToSource( idx );
    }

    sourceModel()->removeIndexes( pil );
}


void
PlayableProxyModel::setShowOfflineResults( bool b )
{
    m_showOfflineResults = b;
    invalidateFilter();
}


void
PlayableProxyModel::setHideDupeItems( bool b )
{
    m_hideDupeItems = b;
    invalidateFilter();
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
    // Attention: This function may be called very often!
    // So be aware of its performance.
    const Tomahawk::track_ptr& t1 = q1->track();
    const Tomahawk::track_ptr& t2 = q2->track();
    const QString& artist1 = t1->artistSortname();
    const QString& artist2 = t2->artistSortname();
    const QString& album1 = t1->albumSortname();
    const QString& album2 = t2->albumSortname();
    const unsigned int albumpos1 = t1->albumpos();
    const unsigned int albumpos2 = t2->albumpos();
    const unsigned int discnumber1 = t1->discnumber();
    const unsigned int discnumber2 = t2->discnumber();
    const qint64 id1 = (qint64)&q1;
    const qint64 id2 = (qint64)&q2;

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

    // Sort by Composer
    const QString& composer1 = t1->composerSortname();
    const QString& composer2 = t2->composerSortname();
    if ( column == PlayableModel::Composer )
    {
        if ( composer1 == composer2 )
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

        return QString::localeAwareCompare( composer1, composer2 ) < 0;
    }

    // Sort by Album
    if ( column == PlayableModel::Album ) // sort by album
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

    // Lazy load these variables, they are not used before.
    unsigned int bitrate1 = 0, bitrate2 = 0;
    unsigned int mtime1 = 0, mtime2 = 0;
    unsigned int size1 = 0, size2 = 0;
    unsigned int year1 = 0, year2 = 0;
    float score1 = 0, score2 = 0;
    QString origin1;
    QString origin2;
    if ( !q1->results().isEmpty() )
    {
        Tomahawk::result_ptr r = q1->results().first();
        bitrate1 = r->bitrate();
        mtime1 = r->modificationTime();
        size1 = r->size();
        year1 = r->track()->year();
        score1 = q1->score();
        origin1 = r->friendlySource().toLower();
    }
    if ( !q2->results().isEmpty() )
    {
        Tomahawk::result_ptr r = q2->results().first();
        bitrate2 = r->bitrate();
        mtime2 = r->modificationTime();
        size2 = r->size();
        year2 = r->track()->year();
        score2 = q2->score();
        origin2 = r->friendlySource().toLower();
    }

    // Sort by bitrate
    if ( column == PlayableModel::Bitrate )
    {
        if ( bitrate1 == bitrate2 )
            return id1 < id2;

        return bitrate1 < bitrate2;
    }
    else if ( column == PlayableModel::Duration ) // sort by duration
    {
        unsigned int duration1 = t1->duration();
        unsigned int duration2 = t2->duration();

        if ( duration1 == duration2 )
            return id1 < id2;

        return duration1 < duration2;
    }
    else if ( column == PlayableModel::Age ) // sort by mtime
    {
        if ( mtime1 == mtime2 )
            return id1 < id2;

        return mtime1 < mtime2;
    }
    else if ( column == PlayableModel::Year ) // sort by release year
    {
        if ( year1 == year2 )
            return id1 < id2;

        return year1 < year2;
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
    else if ( column == PlayableModel::Origin ) // sort by file origin
    {
        if ( origin1 == origin2 )
            return id1 < id2;

        return origin1 < origin2;
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

    const QString& lefts = t1->track();
    const QString& rights = t2->track();
    if ( lefts == rights )
        return id1 < id2;

    return QString::localeAwareCompare( lefts, rights ) < 0;
}


bool
PlayableProxyModel::lessThan( const Tomahawk::album_ptr& album1, const Tomahawk::album_ptr& album2 ) const
{
    if ( album1->artist() == album2->artist() )
    {
        return QString::localeAwareCompare( album1->sortname(), album2->sortname() ) < 0;
    }

    return QString::localeAwareCompare( album1->artist()->sortname(), album2->artist()->sortname() ) < 0;
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
        if ( !m_headerStyle.contains( m_style ) || left.column() >= m_headerStyle[ m_style ].count() )
        {
            return lessThan( left.column(), p1->query(), p2->query() );
        }

        PlayableModel::Columns col = m_headerStyle[ m_style ].at( left.column() );
        return lessThan( col, p1->query(), p2->query() );
    }
    if ( p1->album() && p2->album() )
    {
        return lessThan( p1->album(), p2->album() );
    }

    return QString::localeAwareCompare( sourceModel()->data( left ).toString(), sourceModel()->data( right ).toString() ) < 0;
}


int
PlayableProxyModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    return m_headerStyle[ m_style ].length();
}


QVariant
PlayableProxyModel::data( const QModelIndex& index, int role ) const
{
    if ( role == StyleRole )
        return m_style;

    if ( !sourceModel() )
        return QVariant();
    if ( !m_headerStyle.contains( m_style ) )
        return QVariant();
    if ( index.column() < 0 || index.column() >= m_headerStyle[ m_style ].count() )
        return QVariant();

    PlayableModel::Columns col = m_headerStyle[ m_style ].at( index.column() );
    QModelIndex sourceIdx = mapToSource( index );
    QModelIndex idx = sourceModel()->index( sourceIdx.row(), (int)col, sourceIdx.parent() );

    return idx.data( role );
}


QVariant
PlayableProxyModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( !sourceModel() )
        return QVariant();
    if ( !m_headerStyle.contains( m_style ) )
        return QVariant();

    if ( section < m_headerStyle[ m_style ].count() )
    {
        PlayableModel::Columns col = m_headerStyle[ m_style ].at( section );
        return sourceModel()->headerData( (int)col, orientation, role );
    }
    else
        return sourceModel()->headerData( 255, orientation, role );
}


QList< double >
PlayableProxyModel::columnWeights() const
{
    QList< double > w;

    switch ( m_style )
    {
        case SingleColumn:
            w << 1.0;
            break;

        case Collection:
            w << 0.16 << 0.17 << 0.15 << 0.15 << 0.06 << 0.06 << 0.06 << 0.06 << 0.06; // << 0.07;
            break;

        case Locker:
            w << 0.14 << 0.15 << 0.12 << 0.12 << 0.12 << 0.06 << 0.06 << 0.06 << 0.06 << 0.06; // << 0.05;
            break;

        case Detailed:
        default:
            w << 0.15 << 0.16 << 0.13 << 0.13 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.08; // << 0.05;
            break;
    }

    return w;
}


void
PlayableProxyModel::updateDetailedInfo( const QModelIndex& index )
{
    PlayableItem* item = itemFromIndex( mapToSource( index ) );

    if ( item->album() )
    {
        item->album()->cover( QSize( 0, 0 ) );
    }
    else if ( item->artist() )
    {
        item->artist()->cover( QSize( 0, 0 ) );
    }
    else if ( item->query() )
    {
        item->query()->track()->cover( QSize( 0, 0 ) );

/*        if ( style() == PlayableProxyModel::Fancy )
        {
            item->query()->track()->loadSocialActions();
        }*/
    }
}


void
PlayableProxyModel::setFilter( const QString& pattern )
{
    if ( pattern != filterRegExp().pattern() )
    {
        setFilterRegExp( pattern );
        emit filterChanged( pattern );
    }
}


int
PlayableProxyModel::mapSourceColumnToColumn( PlayableModel::Columns column )
{
    return m_headerStyle[ m_style ].indexOf( column );
}


void
PlayableProxyModel::setCurrentIndex( const QModelIndex& index )
{
    tDebug() << Q_FUNC_INFO;
    m_model->setCurrentIndex( mapToSource( index ) );
}


void
PlayableProxyModel::onIndexPlayable( const QModelIndex& index )
{
    emit indexPlayable( mapFromSource( index ) );
}


void
PlayableProxyModel::onIndexResolved( const QModelIndex& index )
{
    emit indexResolved( mapFromSource( index ) );
}


void
PlayableProxyModel::expandRequested( const QPersistentModelIndex& idx )
{
    emit expandRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


void
PlayableProxyModel::selectRequested( const QPersistentModelIndex& idx )
{
    emit selectRequest( QPersistentModelIndex( mapFromSource( idx ) ) );
}


void
PlayableProxyModel::onCurrentIndexChanged( const QModelIndex& newIndex, const QModelIndex& oldIndex )
{
    emit currentIndexChanged( mapFromSource( newIndex ), mapFromSource( oldIndex ) );
}
