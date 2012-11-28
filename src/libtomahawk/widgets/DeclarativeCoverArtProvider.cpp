#include "DeclarativeCoverArtProvider.h"
#include "playlist/PlayableItem.h"
#include "playlist/PlayableProxyModel.h"
#include "Query.h"
#include "Album.h"
#include "Artist.h"
#include "utils/TomahawkUtilsGui.h"

#include <QDeclarativeImageProvider>
#include <QModelIndex>
#include <QDebug>

namespace Tomahawk
{

DeclarativeCoverArtProvider::DeclarativeCoverArtProvider( PlayableProxyModel *model )
    : QDeclarativeImageProvider( QDeclarativeImageProvider::Pixmap )
    , m_model( model )
{

}

DeclarativeCoverArtProvider::~DeclarativeCoverArtProvider()
{
}

QPixmap DeclarativeCoverArtProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    // We always can generate it in the requested size
    int width = requestedSize.width() > 0 ? requestedSize.width() : 230;
    int height = requestedSize.height() > 0 ? requestedSize.height() : 230;

    if( size )
        *size = QSize( width, height );

//    PlayableItem *item = m_model->itemFromIndex( id.toInt() );
//    if( item ) {
//        qDebug() << "item:" << item;
//        qDebug() << "item2:" << item->artistName() << item->name();
//        if ( !item->query().isNull() ) {
//            return item->query()->displayQuery()->cover( *size );
//        }
//    }

    QPixmap cover;

    tDebug() << "Getting by id:" << id << requestedSize;
/*    query_ptr query = Query::getByCoverId( id );
    if ( !query.isNull() ) {
        return query->cover( requestedSize );
    }*/

    album_ptr album = Album::getByCoverId( id );
    if ( !album.isNull() )
    {
        tDebug() << "Returning album cover:" << album->cover( *size ).isNull();
        cover = album->cover( *size );
        if ( cover.isNull() )
        {
            tDebug() << Q_FUNC_INFO << "Returning default album image";
            cover = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::Original, *size );
        }
    }

    artist_ptr artist = Artist::getByCoverId( id );
    if ( !artist.isNull() )
    {
        tDebug() << "Returning artist cover:" << artist->cover( *size ).isNull();
        cover = artist->cover( *size );
        if ( cover.isNull() )
        {
            tDebug() << Q_FUNC_INFO << "Returning default artist image";
            cover = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Original, *size );
        }
    }

    if ( cover.isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Returning default track image";
        cover = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, *size );
    }
    return cover;
}

}
