#include "playlistproxymodel.h"

#include <QDebug>
#include <QTreeView>

#include "tomahawk/query.h"


PlaylistProxyModel::PlaylistProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_model( 0 )
    , m_repeatMode( PlaylistModelInterface::NoRepeat )
    , m_shuffled( false )
{
    qsrand( QTime( 0, 0, 0 ).secsTo( QTime::currentTime() ) );

    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceModel( 0 );
}


void
PlaylistProxyModel::setSourceModel( PlaylistModel* sourceModel )
{
    if ( m_model )
    {
        disconnect( m_model, SIGNAL( numSourcesChanged( unsigned int ) ),
                       this, SIGNAL( numSourcesChanged( unsigned int ) ) );

        disconnect( m_model, SIGNAL( numTracksChanged( unsigned int ) ),
                       this, SIGNAL( numTracksChanged( unsigned int ) ) );

        disconnect( m_model, SIGNAL( numArtistsChanged( unsigned int ) ),
                       this, SIGNAL( numArtistsChanged( unsigned int ) ) );

        disconnect( m_model, SIGNAL( numShownChanged( unsigned int ) ),
                       this, SIGNAL( numShownChanged( unsigned int ) ) );
    }

    ((QTreeView*)parent())->setSortingEnabled( false );
    ((QTreeView*)parent())->sortByColumn( -1 );

    m_model = sourceModel;
    QSortFilterProxyModel::setSourceModel( sourceModel );

    if ( m_model && !m_model->isPlaylistBacked() )
    {
        ((QTreeView*)parent())->setSortingEnabled( true );
        ((QTreeView*)parent())->sortByColumn( 0, Qt::AscendingOrder );
    }

    if ( m_model )
    {
        connect( m_model, SIGNAL( numSourcesChanged( unsigned int ) ),
                          SIGNAL( numSourcesChanged( unsigned int ) ) );

        connect( m_model, SIGNAL( numTracksChanged( unsigned int ) ),
                          SIGNAL( numTracksChanged( unsigned int ) ) );

        connect( m_model, SIGNAL( numArtistsChanged( unsigned int ) ),
                          SIGNAL( numArtistsChanged( unsigned int ) ) );

        connect( m_model, SIGNAL( numShownChanged( unsigned int ) ),
                          SIGNAL( numShownChanged( unsigned int ) ) );

        m_model->updateStats();
    }
}


void
PlaylistProxyModel::setFilterRegExp( const QString& pattern )
{
    qDebug() << Q_FUNC_INFO;
    QSortFilterProxyModel::setFilterRegExp( pattern );
    emit numShownChanged( rowCount() );
}


void
PlaylistProxyModel::setCurrentItem( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;
    return m_model->setCurrentItem( index );
}


PlaylistItem*
PlaylistProxyModel::previousItem()
{
    return siblingItem( -1 );
}


PlaylistItem*
PlaylistProxyModel::nextItem()
{
    return siblingItem( 1 );
}


PlaylistItem*
PlaylistProxyModel::siblingItem( int itemsAway )
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
        else if ( currentItem().isValid() )
        {
            idx = currentItem();

            // random mode is disabled
            if ( m_repeatMode == PlaylistModelInterface::RepeatOne )
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

    if ( !idx.isValid() && m_repeatMode == PlaylistModelInterface::RepeatAll )
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
        PlaylistItem* item = PlaylistModel::indexToPlaylistItem( idx );
        if ( item && item->query()->numResults() )
        {
            qDebug() << "Next PlaylistItem found: " << item->query()->toString() << item->query()->results().at( 0 )->url();
            setCurrentItem( item->index() );
            return item;
        }

        idx = index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0 );
    }
    while ( idx.isValid() );

    setCurrentItem( QModelIndex() );
    return 0;
}


bool
PlaylistProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    if ( filterRegExp().isEmpty() )
        return true;

    PlaylistItem* pi = PlaylistModel::indexToPlaylistItem( sourceModel()->index( sourceRow, 0, sourceParent ) );
    if ( !pi )
        return false;

    const Tomahawk::query_ptr& q = pi->query();
    Tomahawk::result_ptr r;
    if ( q->numResults() )
        r = q->results().at( 0 );

    QStringList sl = filterRegExp().pattern().split( " ", QString::SkipEmptyParts );
    bool found = true;

    foreach( const QString& s, sl )
    {
        if ( !r.isNull() )
        {
            if ( !r->artist().contains( s, Qt::CaseInsensitive ) &&
                 !r->album() .contains( s, Qt::CaseInsensitive ) &&
                 !r->track() .contains( s, Qt::CaseInsensitive ) )
            {
                found = false;
            }
        }
        else
        {
            if ( !q->artist().contains( s, Qt::CaseInsensitive ) &&
                 !q->album() .contains( s, Qt::CaseInsensitive ) &&
                 !q->track() .contains( s, Qt::CaseInsensitive ) )
            {
                found = false;
            }
        }
    }

    return found;
}


bool
PlaylistProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    PlaylistItem* p1 = PlaylistModel::indexToPlaylistItem( left );
    PlaylistItem* p2 = PlaylistModel::indexToPlaylistItem( right );

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

    if ( q1->numResults() )
    {
        const Tomahawk::result_ptr& r = q1->results().at( 0 );
        artist1 = r->artist();
        album1 = r->album();
        track1 = r->track();
        albumpos1 = r->albumpos();
        bitrate1 = r->bitrate();
    }
    if ( q2->numResults() )
    {
        const Tomahawk::result_ptr& r = q2->results().at( 0 );
        artist2 = r->artist();
        album2 = r->album();
        track2 = r->track();
        albumpos2 = r->albumpos();
        bitrate2 = r->bitrate();
    }

    if ( left.column() == 0 ) // sort by artist
    {
        if ( artist1 == artist2 )
        {
            if ( album1 == album2 )
            {
                if ( albumpos1 == albumpos2 )
                {
                    return QString::localeAwareCompare( track1, track2 ) < 0;
                }

                return albumpos1 < albumpos2;
            }

            return QString::localeAwareCompare( album1, album2 ) < 0;
        }

        return QString::localeAwareCompare( artist1, artist2 ) < 0;
    }
    else if ( left.column() == 2 ) // sort by album
    {
        if ( album1 == album2 )
        {
            return albumpos1 < albumpos2;
        }

        return QString::localeAwareCompare( album1, album2 ) < 0;
    }
    else if ( left.column() == 4 ) // sort by bitrate
    {
        return bitrate1 < bitrate2;
    }

    return QString::localeAwareCompare( sourceModel()->data( left ).toString(),
                                        sourceModel()->data( right ).toString() ) < 0;
}
