/*
 *   Copyright 2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ImageRegistry.h"

#include <QSvgRenderer>
#include <QPainter>
#include <qicon.h>

#include "utils/Logger.h"

static QHash< QString, QHash< int, QHash< qint64, QPixmap > > > s_cache;
ImageRegistry* ImageRegistry::s_instance = 0;


ImageRegistry*
ImageRegistry::instance()
{
    return s_instance;
}


ImageRegistry::ImageRegistry()
{
    s_instance = this;
}


QIcon
ImageRegistry::icon( const QString& image, TomahawkUtils::ImageMode mode )
{
    return pixmap( image, TomahawkUtils::defaultIconSize(), mode );
}


qint64
ImageRegistry::cacheKey( const QSize& size, float opacity, QColor tint )
{
    return size.width() * 100 + size.height() * 10 + ( opacity * 100.0 ) + tint.value();
}


QPixmap
ImageRegistry::pixmap( const QString& image, const QSize& size, TomahawkUtils::ImageMode mode, float opacity, QColor tint )
{
    if ( size.width() < 0 || size.height() < 0 )
    {
        Q_ASSERT( false );
        return QPixmap();
    }

    QHash< qint64, QPixmap > subsubcache;
    QHash< int, QHash< qint64, QPixmap > > subcache;

    if ( s_cache.contains( image ) )
    {
        subcache = s_cache.value( image );

        if ( subcache.contains( mode ) )
        {
            subsubcache = subcache.value( mode );

            const qint64 ck = cacheKey( size, opacity, tint );
            if ( subsubcache.contains( ck ) )
            {
                return subsubcache.value( ck );
            }
        }
    }

    // Image not found in cache. Let's load it.
    QPixmap pixmap;
    if ( image.toLower().endsWith( ".svg" ) )
    {
        QSvgRenderer svgRenderer( image );
        QPixmap p( size.isNull() || size.height() == 0 || size.width() == 0 ? svgRenderer.defaultSize() : size );
        p.fill( Qt::transparent );

        QPainter pixPainter( &p );
        pixPainter.setOpacity( opacity );
        svgRenderer.render( &pixPainter );
        pixPainter.end();

        if ( tint.alpha() > 0 )
            p = TomahawkUtils::tinted( p, tint );

        pixmap = p;
    }
    else
        pixmap = QPixmap( image );

    if ( !pixmap.isNull() )
    {
        switch ( mode )
        {
            case TomahawkUtils::RoundedCorners:
                pixmap = TomahawkUtils::createRoundedImage( pixmap, size );
                break;

            default:
                break;
        }

        if ( !size.isNull() && pixmap.size() != size )
        {
            if ( size.width() == 0 )
            {
                pixmap = pixmap.scaledToHeight( size.height(), Qt::SmoothTransformation );
            }
            else if ( size.height() == 0 )
            {
                pixmap = pixmap.scaledToWidth( size.width(), Qt::SmoothTransformation );
            }
            else
                pixmap = pixmap.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
        }

        putInCache( image, size, mode, opacity, pixmap, tint );
    }

    return pixmap;
}


void
ImageRegistry::putInCache( const QString& image, const QSize& size, TomahawkUtils::ImageMode mode, float opacity, const QPixmap& pixmap, QColor tint )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Adding to image cache:" << image << size << mode;

    QHash< qint64, QPixmap > subsubcache;
    QHash< int, QHash< qint64, QPixmap > > subcache;

    if ( s_cache.contains( image ) )
    {
        subcache = s_cache.value( image );

        if ( subcache.contains( mode ) )
        {
            subsubcache = subcache.value( mode );

/*            if ( subsubcache.contains( size.width() * size.height() ) )
            {
                Q_ASSERT( false );
            }*/
        }
    }

    subsubcache.insert( cacheKey( size, opacity, tint ), pixmap );
    subcache.insert( mode, subsubcache );
    s_cache.insert( image, subcache );
}
