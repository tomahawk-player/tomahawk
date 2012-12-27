#include "DeclarativeCoverArtProvider.h"
#include "playlist/PlayableItem.h"
#include "playlist/PlayableProxyModel.h"
#include "Query.h"
#include "Album.h"
#include "Artist.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QDeclarativeImageProvider>
#include <QModelIndex>
#include <QDebug>
#include <QPainter>

namespace Tomahawk
{

DeclarativeCoverArtProvider::DeclarativeCoverArtProvider(  )
    : QDeclarativeImageProvider( QDeclarativeImageProvider::Pixmap )
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

    QPixmap cover;

    tDebug() << "DeclarativeCoverArtprovider: Getting album art by id:" << id << requestedSize;

    bool mirrored = false;
    bool labeled = false;

    QString coverId = id;
    if(coverId.contains("-mirror")) {
        coverId.remove("-mirror");
        mirrored = true;
    }
    if(coverId.contains("-labels")) {
        coverId.remove("-labels");
        labeled = true;
    }

    artist_ptr artist = Artist::getByCoverId( coverId );
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
        album_ptr album = Album::getByCoverId( coverId );
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
    }

    if ( cover.isNull() )
    {
        tDebug() << Q_FUNC_INFO << "Returning default track image";
        cover = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, *size );
    }

    QImage image(*size, QImage::Format_ARGB32);

    if(labeled) {
        QImage coverImage(*size, QImage::Format_RGB32);
        QPainter bgPainter(&coverImage);
        bgPainter.drawPixmap(0, 0, size->width(), size->height(), cover);

        QColor c1;
        c1.setRgb( 0, 0, 0 );
        c1.setAlphaF( 0.00 );
        QColor c2;
        c2.setRgb( 0, 0, 0 );
        c2.setAlphaF( 0.88 );

        QLinearGradient gradient( QPointF( 0, 0 ), QPointF( 0, 1 ) );
        gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
        gradient.setColorAt( 0.0, c1 );
        gradient.setColorAt( 0.6, c2 );
        gradient.setColorAt( 1.0, c2 );

        bgPainter.setPen( Qt::transparent );
        bgPainter.setBrush(QBrush(gradient));
        bgPainter.drawRect(0, size->height() * 0.7, size->width(), size->height() * 0.3);
        cover = QPixmap::fromImage(coverImage);
    }

    QPainter painter(&image);
    if(!mirrored) {
        image.fill(Qt::white);
        painter.drawPixmap(0, 0, size->width(), size->height(), cover);
    } else {
        image.fill(QColor(0, 0, 0, 0));

        // Lets paint half of the image in a fragment per line
        int mirrorHeight = size->height() / 2;
        int fragmentCount = mirrorHeight;
        int fragmentHeight = mirrorHeight / fragmentCount;

        QPainter::PixmapFragment fragments[fragmentCount];

        qreal fragmentOpacity = 0;
        int fragmentStartY = size->height() - mirrorHeight;
        for(int i = 0; i < fragmentCount; ++i) {
            QPointF point = QPointF(size->width() / 2, fragmentStartY + (fragmentHeight / 2));
            QRectF sourceRect = QRectF(0, fragmentStartY, size->width(), fragmentHeight);
            fragments[i] = QPainter::PixmapFragment::create(point, sourceRect, 1, 1, 0, fragmentOpacity);
            fragmentOpacity += 0.5 / fragmentCount;
            fragmentStartY += fragmentHeight;
        }
        painter.drawPixmapFragments(fragments, fragmentCount, cover);
    }

    return QPixmap::fromImage(image);
}

}
