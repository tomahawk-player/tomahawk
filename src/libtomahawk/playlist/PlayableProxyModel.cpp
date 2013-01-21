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
    , m_hideDupeItems( false )
    , m_maxVisibleItems( -1 )
    , m_style( Detailed )
{
    m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::PlayableProxyModelPlaylistInterface( this ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourcePlayableModel( 0 );

    m_headerStyle[ Large ]      << PlayableModel::Name;
    m_headerStyle[ Detailed ]   << PlayableModel::Artist << PlayableModel::Track << PlayableModel::Composer << PlayableModel::Album << PlayableModel::AlbumPos << PlayableModel::Duration << PlayableModel::Bitrate << PlayableModel::Age << PlayableModel::Year << PlayableModel::Filesize << PlayableModel::Origin << PlayableModel::Score;
    m_headerStyle[ Collection ] << PlayableModel::Name << PlayableModel::Composer << PlayableModel::Duration << PlayableModel::Bitrate << PlayableModel::Age << PlayableModel::Year << PlayableModel::Filesize << PlayableModel::Origin;
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
        disconnect( m_model, SIGNAL( currentIndexChanged() ), this, SIGNAL( currentIndexChanged() ) );
    }

    m_model = sourceModel;

    if ( m_model )
    {
        connect( m_model, SIGNAL( loadingStarted() ), SIGNAL( loadingStarted() ) );
        connect( m_model, SIGNAL( loadingFinished() ), SIGNAL( loadingFinished() ) );
        connect( m_model, SIGNAL( itemCountChanged( unsigned int ) ), SIGNAL( itemCountChanged( unsigned int ) ) );
        connect( m_model, SIGNAL( indexPlayable( QModelIndex ) ), SLOT( onIndexPlayable( QModelIndex ) ) );
        connect( m_model, SIGNAL( indexResolved( QModelIndex ) ), SLOT( onIndexResolved( QModelIndex ) ) );
        connect( m_model, SIGNAL( currentIndexChanged() ), SIGNAL( currentIndexChanged() ) );
    }

    QSortFilterProxyModel::setSourceModel( m_model );
}


bool
PlayableProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    PlayableItem* pi = itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    if ( m_maxVisibleItems > 0 && sourceRow > m_maxVisibleItems - 1 )
        return false;

    if ( m_hideDupeItems )
    {
        for ( int i = 0; i < sourceRow; i++ )
        {
            PlayableItem* di = itemFromIndex( sourceModel()->index( i, 0, sourceParent ) );
            if ( !di )
                continue;

            bool b = ( pi->query() && pi->query()->equals( di->query() ) ) ||
                     ( pi->album() && pi->album() == di->album() ) ||
                     ( pi->artist() && pi->artist()->name() == di->artist()->name() );

            if ( b && filterAcceptsRow( i, sourceParent ) )
                return false;
        }
    }

    if ( pi->query() )
    {
        const Tomahawk::query_ptr& q = pi->query()->displayQuery();
        if ( q.isNull() ) // uh oh? filter out invalid queries i guess
            return false;

        Tomahawk::result_ptr r;
        if ( q->numResults() )
            r = q->results().first();

        if ( !m_showOfflineResults && ( r.isNull() || !r->isOnline() ) )
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
    const QString artist1 = q1->artistSortname();
    const QString artist2 = q2->artistSortname();
    const QString album1 = q1->albumSortname();
    const QString album2 = q2->albumSortname();
    const QString track1 = q1->trackSortname();
    const QString track2 = q2->trackSortname();
    const QString composer1 = q1->composerSortname();
    const QString composer2 = q2->composerSortname();
    const unsigned int albumpos1 = q1->albumpos();
    const unsigned int albumpos2 = q2->albumpos();
    const unsigned int discnumber1 = q1->discnumber();
    const unsigned int discnumber2 = q2->discnumber();
    unsigned int duration1 = q1->duration(), duration2 = q2->duration();
    unsigned int bitrate1 = 0, bitrate2 = 0;
    unsigned int mtime1 = 0, mtime2 = 0;
    unsigned int size1 = 0, size2 = 0;
    unsigned int year1 = 0, year2 = 0;
    float score1 = 0, score2 = 0;
    QString origin1;
    QString origin2;
    qint64 id1 = 0, id2 = 0;

    if ( q1->numResults() )
    {
        const Tomahawk::result_ptr& r = q1->results().at( 0 );
        bitrate1 = r->bitrate();
        duration1 = r->duration();
        mtime1 = r->modificationTime();
        size1 = r->size();
        year1 = r->year();
        score1 = r->score();
        origin1 = r->friendlySource().toLower();
        id1 = (qint64)&r;
    }
    if ( q2->numResults() )
    {
        const Tomahawk::result_ptr& r = q2->results().at( 0 );
        bitrate2 = r->bitrate();
        duration2 = r->duration();
        mtime2 = r->modificationTime();
        size2 = r->size();
        year2 = r->year();
        score2 = r->score();
        origin2 = r->friendlySource().toLower();
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
    else if ( column == PlayableModel::Composer ) // sort by composer
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
    else if ( column == PlayableModel::Duration ) // sort by duration
    {
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


int
PlayableProxyModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    switch ( m_style )
    {
        case Short:
        case ShortWithAvatars:
        case Large:
            return 1;
            break;

        case Collection:
            return 8;
            break;

        case Detailed:
        default:
            return 12;
            break;
    }
}


QVariant
PlayableProxyModel::data( const QModelIndex& index, int role ) const
{
    if ( role == StyleRole )
    {
        return m_style;
    }

    if ( !sourceModel() )
        return QVariant();
    if ( !m_headerStyle.contains( m_style ) )
        return QVariant();
    if ( index.column() < 0 )
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
        case Short:
        case ShortWithAvatars:
        case Large:
            w << 1.0;
            break;

        case Collection:
            w << 0.42 << 0.12 << 0.07 << 0.07 << 0.07 << 0.07 << 0.07; // << 0.11;
            break;

        case Detailed:
        default:
            w << 0.15 << 0.15 << 0.12 << 0.12 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.05 << 0.09; // << 0.03;
            break;
    }

    return w;
}


void
PlayableProxyModel::updateDetailedInfo( const QModelIndex& index )
{
    if ( style() != PlayableProxyModel::Short && style() != PlayableProxyModel::Large )
        return;

    PlayableItem* item = itemFromIndex( mapToSource( index ) );
    if ( item->query().isNull() )
        return;

    if ( style() == PlayableProxyModel::Short || style() == PlayableProxyModel::Large )
    {
        item->query()->displayQuery()->cover( QSize( 0, 0 ) );
    }

    if ( style() == PlayableProxyModel::Large )
    {
        item->query()->loadSocialActions();
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
