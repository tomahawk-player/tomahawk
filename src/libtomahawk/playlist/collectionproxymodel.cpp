#include "collectionproxymodel.h"

#include <QDebug>
#include <QTreeView>

#include "album.h"
#include "query.h"
#include "collectionmodel.h"


CollectionProxyModel::CollectionProxyModel( QObject* parent )
    : TrackProxyModel( parent )
{
}


bool
CollectionProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    PlItem* p1 = itemFromIndex( left );
    PlItem* p2 = itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    const Tomahawk::query_ptr& q1 = p1->query();
    const Tomahawk::query_ptr& q2 = p2->query();

    if ( q1.isNull() || q2.isNull() )
    {
        return QString::localeAwareCompare( p1->caption, p2->caption ) < 0;
    }

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
    }

    if ( left.column() == 0 ) // sort by artist
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
    else if ( left.column() == 2 ) // sort by album
    {
        if ( album1 == album2 )
        {
            if ( albumpos1 == albumpos2 )
                return id1 < id2;

            return albumpos1 < albumpos2;
        }

        return QString::localeAwareCompare( album1, album2 ) < 0;
    }
    else if ( left.column() == 4 ) // sort by bitrate
    {
        if ( bitrate1 == bitrate2 )
            return id1 < id2;

        return bitrate1 < bitrate2;
    }
    else if ( left.column() == 5 ) // sort by mtime
    {
        if ( mtime1 == mtime2 )
            return id1 < id2;

        return mtime1 < mtime2;
    }

    return QString::localeAwareCompare( sourceModel()->data( left ).toString(),
                                        sourceModel()->data( right ).toString() ) < 0;
}
