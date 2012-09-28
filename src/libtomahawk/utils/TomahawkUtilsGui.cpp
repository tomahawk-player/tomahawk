/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "TomahawkUtilsGui.h"

#include "config.h"
#include "Query.h"
#include "Result.h"
#include "Logger.h"
#include "PlayableItem.h"
#include "Source.h"

#include <QtGui/QLayout>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtGui/QPalette>
#include <QtGui/QApplication>
#include <QtGui/QScrollBar>
#include <QtGui/QWidget>
#include <QStyleOption>
#include <QDesktopServices>

#ifdef Q_WS_X11
    #include <QtGui/QX11Info>
    #include <libqnetwm/netwm.h>
#endif

#ifdef Q_WS_WIN
    #include <windows.h>
    #include <windowsx.h>
    #include <shellapi.h>
#endif

// Defined in qpixmapfilter.cpp, private but exported
extern void qt_blurImage( QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );

namespace TomahawkUtils
{
static int s_defaultFontSize = 0;
static int s_defaultFontHeight = 0;


QPixmap
createDragPixmap( MediaType type, int itemCount )
{
    // If more than one item is dragged, align the items inside a
    // rectangular grid. The maximum grid size is limited to 5 x 5 items.
    int xCount = 3;
    int size = 32;

    if ( itemCount > 16 )
    {
        xCount = 5;
        size = 16;
    }
    else if( itemCount > 9 )
    {
        xCount = 4;
        size = 22;
    }

    if ( itemCount < xCount )
    {
        xCount = itemCount;
    }

    int yCount = itemCount / xCount;
    if ( itemCount % xCount != 0 )
    {
        ++yCount;
    }
    if ( yCount > xCount )
    {
        yCount = xCount;
    }
    // Draw the selected items into the grid cells
    QPixmap dragPixmap( xCount * size + xCount - 1, yCount * size + yCount - 1 );
    dragPixmap.fill( Qt::transparent );

    QPainter painter( &dragPixmap );
    painter.setRenderHint( QPainter::Antialiasing );

    QPixmap pixmap;
    switch ( type )
    {
        case MediaTypeArtist:
            pixmap = QPixmap( ":/data/images/artist-icon.png" ).scaledToWidth( size, Qt::SmoothTransformation );
            break;
        case MediaTypeAlbum:
            pixmap = QPixmap( ":/data/images/album-icon.png" ).scaledToWidth( size, Qt::SmoothTransformation );
            break;
        case MediaTypeTrack:
            pixmap = QPixmap( QString( ":/data/images/track-icon-%2x%2.png" ).arg( size ) );
            break;
    }

    int x = 0;
    int y = 0;
    for ( int i = 0; i < itemCount; ++i )
    {

        painter.drawPixmap( x, y, pixmap );

        x += size + 1;
        if ( x >= dragPixmap.width() )
        {
            x = 0;
            y += size + 1;
        }
        if ( y >= dragPixmap.height() )
        {
            break;
        }
    }

    return dragPixmap;
}


void
drawShadowText( QPainter* painter, const QRect& rect, const QString& text, const QTextOption& textOption )
{
    painter->save();

    painter->drawText( rect, text, textOption );

/*    QFont font = painter->font();
    font.setPixelSize( font.pixelSize() + 2 );
    painter->setFont( font );

    painter->setPen( Qt::black );
    painter->drawText( rect, text, textOption );*/

    painter->restore();
}


void
drawBackgroundAndNumbers( QPainter* painter, const QString& text, const QRect& figRectIn )
{
    painter->save();

    QRect figRect = figRectIn;
    if ( text.length() == 1 )
        figRect.adjust( -painter->fontMetrics().averageCharWidth(), 0, 0, 0 );

    QPen origpen = painter->pen();
    QPen pen = origpen;
    pen.setWidth( 1.0 );
    painter->setPen( pen );
    painter->drawRect( figRect );

    // circles look bad. make it an oval. (thanks, apple)
    const int bulgeWidth = 8;
    const int offset = 0; // number of pixels to begin, counting inwards from figRect.x() and figRect.width(). 0 means start at each end, negative means start inside the rect.

    QPainterPath ppath;
    ppath.moveTo( QPoint( figRect.x() + offset, figRect.y() + figRect.height() / 2 ) );
    QRect leftArcRect( figRect.x() + offset - bulgeWidth, figRect.y(), 2*bulgeWidth, figRect.height() );
    ppath.arcTo( leftArcRect, 90, 180 );
    painter->drawPath( ppath );

    ppath = QPainterPath();
    ppath.moveTo( figRect.x() + figRect.width() - offset, figRect.y() + figRect.height() / 2 );
    leftArcRect = QRect( figRect.x() + figRect.width() - offset - bulgeWidth, figRect.y(), 2*bulgeWidth, figRect.height() );
    ppath.arcTo( leftArcRect, 270, 180 );
    painter->drawPath( ppath );

    figRect.adjust( -1, 0, 0, 0 );

    painter->setPen( origpen );
    painter->setPen( Qt::white );
    painter->drawText( figRect.adjusted( -5, 0, 6, 0 ), text, QTextOption( Qt::AlignCenter ) );

    painter->restore();
}


void
drawQueryBackground( QPainter* p, const QPalette& palette, const QRect& r, qreal lightnessFactor )
{
    p->setPen( palette.highlight().color().lighter( lightnessFactor * 100 ) );
    p->setBrush( palette.highlight().color().lighter( lightnessFactor * 100 ) );
    p->drawRoundedRect( r, 4.0, 4.0 );
}


void
unmarginLayout( QLayout* layout )
{
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setMargin( 0 );
    layout->setSpacing( 0 );

    for ( int i = 0; i < layout->count(); i++ )
    {
        QLayout* childLayout = layout->itemAt( i )->layout();
        if ( childLayout )
            unmarginLayout( childLayout );
    }
}


QWidget*
tomahawkWindow()
{
    QWidgetList widgetList = qApp->topLevelWidgets();
    int i = 0;
    while( i < widgetList.count() && widgetList.at( i )->objectName() != "TH_Main_Window" )
        i++;

    if ( i == widgetList.count() )
    {
        qDebug() << Q_FUNC_INFO << "could not find main Tomahawk mainwindow";
        Q_ASSERT( false );
        return 0;
    }

    QWidget *widget = widgetList.at( i );
    return widget;
}


#ifndef Q_WS_MAC
void
bringToFront()
{
#if defined(Q_WS_X11)
    {
        qDebug() << Q_FUNC_INFO;

        QWidget* widget = tomahawkWindow();
        if ( !widget )
            return;

        widget->show();
        widget->activateWindow();
        widget->raise();

        WId wid = widget->winId();
        NETWM::init();

        XEvent e;
        e.xclient.type = ClientMessage;
        e.xclient.message_type = NETWM::NET_ACTIVE_WINDOW;
        e.xclient.display = QX11Info::display();
        e.xclient.window = wid;
        e.xclient.format = 32;
        e.xclient.data.l[0] = 2;
        e.xclient.data.l[1] = QX11Info::appTime();
        e.xclient.data.l[2] = 0;
        e.xclient.data.l[3] = 0l;
        e.xclient.data.l[4] = 0l;

        XSendEvent( QX11Info::display(), RootWindow( QX11Info::display(), DefaultScreen( QX11Info::display() ) ), False, SubstructureRedirectMask | SubstructureNotifyMask, &e );
    }
#elif defined(Q_WS_WIN)
    {
        qDebug() << Q_FUNC_INFO;

        QWidget* widget = tomahawkWindow();
        if ( !widget )
            return;

        widget->show();
        widget->activateWindow();
        widget->raise();

        WId wid = widget->winId();

        HWND hwndActiveWin = GetForegroundWindow();
        int  idActive      = GetWindowThreadProcessId(hwndActiveWin, NULL);
        if ( AttachThreadInput(GetCurrentThreadId(), idActive, TRUE) )
        {
            SetForegroundWindow( wid );
            SetFocus( wid );
            AttachThreadInput(GetCurrentThreadId(), idActive, FALSE);
        }
    }
#endif
}
#endif


void
openUrl( const QUrl& url )
{
#ifdef Q_OS_WIN
    ShellExecuteW( 0, 0, (LPCWSTR)url.toString().utf16(), 0, 0, SW_SHOWNORMAL );
#else
    QDesktopServices::openUrl( url );
#endif
}


QPixmap
createAvatarFrame( const QPixmap &avatar )
{
    QPixmap frame( ":/data/images/avatar_frame.png" );
    QPixmap scaledAvatar = avatar.scaled( frame.height() * 75 / 100, frame.width() * 75 / 100, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    QPainter painter( &frame );
    painter.drawPixmap( ( frame.height() - scaledAvatar.height() ) / 2, ( frame.width() - scaledAvatar.width() ) / 2, scaledAvatar );

    return frame;
}


int
defaultFontSize()
{
    return s_defaultFontSize;
}


int
defaultFontHeight()
{
    if ( s_defaultFontHeight <= 0 )
    {
        QFont f;
        f.setPointSize( defaultFontSize() );
        s_defaultFontHeight = QFontMetrics( f ).height();
    }

    return s_defaultFontHeight;
}


void
setDefaultFontSize( int points )
{
    s_defaultFontSize = points;
}


QColor
alphaBlend( const QColor& colorFrom, const QColor& colorTo, float opacity )
{
    opacity = qMax( (float)0.3, opacity );
    int r = colorFrom.red(), g = colorFrom.green(), b = colorFrom.blue();
    r = opacity * r + ( 1 - opacity ) * colorTo.red();
    g = opacity * g + ( 1 - opacity ) * colorTo.green();
    b = opacity * b + ( 1 - opacity ) * colorTo.blue();

    return QColor( r, g, b );
}


QPixmap
defaultPixmap( ImageType type, ImageMode mode, const QSize& size )
{
    QPixmap pixmap;
    QHash< int, QPixmap > subsubcache;
    QHash< int, QHash< int, QPixmap > > subcache;
    static QHash< int, QHash< int, QHash< int, QPixmap > > > cache;

    if ( cache.contains( type ) )
    {
        subcache = cache.value( type );

        if ( subcache.contains( mode ) )
        {
            subsubcache = subcache.value( mode );

            if ( subsubcache.contains( size.width() ) )
                return subsubcache.value( size.width() );
        }
    }

    switch ( type )
    {
        case DefaultAlbumCover:
            if ( mode == CoverInCase )
                pixmap = QPixmap( RESPATH "images/no-album-art-placeholder.png" );
            else if ( mode == Grid )
                pixmap = QPixmap( RESPATH "images/album-placeholder-grid.png" );
            else
                pixmap = QPixmap( RESPATH "images/no-album-no-case.png" );
            break;

        case DefaultArtistImage:
            if ( mode == Grid )
                pixmap = QPixmap( RESPATH "images/artist-placeholder-grid.png" );
            else
                pixmap = QPixmap( RESPATH "images/no-artist-image-placeholder.png" );
            break;

        case DefaultTrackImage:
                pixmap = QPixmap( RESPATH "images/track-placeholder.png" );
            break;

        case DefaultSourceAvatar:
            if ( mode == AvatarInFrame )
                pixmap = TomahawkUtils::createAvatarFrame( QPixmap( RESPATH "images/user-avatar.png" ) );
            else
                pixmap = QPixmap( RESPATH "images/user-avatar.png" );
            break;

        case NowPlayingSpeaker:
            pixmap = QPixmap( RESPATH "images/now-playing-speaker.png" );
            break;

        case InfoIcon:
            pixmap = QPixmap( RESPATH "images/info.png" );

        default:
            break;
    }

    if ( pixmap.isNull() )
    {
        Q_ASSERT( false );
        return QPixmap();
    }

    if ( !size.isNull() )
        pixmap = pixmap.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    subsubcache.insert( size.width(), pixmap );
    subcache.insert( mode, subsubcache );
    cache.insert( type, subcache );

    return pixmap;
}


void
prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item )
{
    Q_UNUSED( index );

    if ( item->isPlaying() )
    {
        option->palette.setColor( QPalette::Highlight, option->palette.color( QPalette::Mid ) );

        option->backgroundBrush = option->palette.color( QPalette::Mid );
        option->palette.setColor( QPalette::Text, option->palette.color( QPalette::Text ) );

    }

    if ( option->state & QStyle::State_Selected && !item->isPlaying() )
    {
        option->palette.setColor( QPalette::Text, option->palette.color( QPalette::HighlightedText ) );
    }
    else
    {
        float opacity = 0.0;
        if ( !item->query()->results().isEmpty() )
            opacity = item->query()->results().first()->score();

        opacity = qMax( (float)0.3, opacity );
        QColor textColor = alphaBlend( option->palette.color( QPalette::Text ), option->palette.color( QPalette::BrightText ), opacity );

        option->palette.setColor( QPalette::Text, textColor );
    }
}


void
drawRoundedButton( QPainter* painter, const QRect& btnRect, const QColor& color, const QColor &gradient1bottom, const QColor& gradient2top, const QColor& gradient2bottom )
{
    QPainterPath btnPath;
    const int radius = 3;
    // draw top half gradient
    const int btnCenter = btnRect.bottom() - ( btnRect.height() / 2 );
    btnPath.moveTo( btnRect.left(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnRect.top() + radius );
    btnPath.quadTo( QPoint( btnRect.topLeft() ), QPoint( btnRect.left() + radius, btnRect.top() ) );
    btnPath.lineTo( btnRect.right() - radius, btnRect.top() );
    btnPath.quadTo( QPoint( btnRect.topRight() ), QPoint( btnRect.right(), btnRect.top() + radius ) );
    btnPath.lineTo( btnRect.right(),btnCenter );
    btnPath.lineTo( btnRect.left(), btnCenter );

    QLinearGradient g;
    if ( gradient1bottom.isValid() )
    {
        g.setColorAt( 0, color );
        g.setColorAt( 0.5, gradient1bottom );
        painter->fillPath( btnPath, g );
    }
    else
        painter->fillPath( btnPath, color );
    //painter->setPen( bg.darker() );

    //painter->drawPath( btnPath );

    btnPath = QPainterPath();
    btnPath.moveTo( btnRect.left(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnRect.bottom() - radius );
    btnPath.quadTo( QPoint( btnRect.bottomLeft() ), QPoint( btnRect.left() + radius, btnRect.bottom() ) );
    btnPath.lineTo( btnRect.right() - radius, btnRect.bottom() );
    btnPath.quadTo( QPoint( btnRect.bottomRight() ), QPoint( btnRect.right(), btnRect.bottom() - radius ) );
    btnPath.lineTo( btnRect.right(), btnCenter );
    btnPath.lineTo( btnRect.left(), btnCenter );

    if ( gradient2top.isValid() && gradient2bottom.isValid() )
    {
        g.setColorAt( 0, gradient2top );
        g.setColorAt( 0.5, gradient2bottom );
        painter->fillPath( btnPath, g );
    }
    else
        painter->fillPath( btnPath, color );

}


void
styleScrollBar( QScrollBar* scrollBar )
{
    scrollBar->setStyleSheet(
        "QScrollBar:horizontal { background-color: transparent; }"
        "QScrollBar::handle:horizontal { border-height: 9px; margin-bottom: 6px;"
            "border-image: url(" RESPATH "images/scrollbar-horizontal-handle.png) 3 3 3 3 stretch stretch;"
            "border-top: 3px transparent; border-bottom: 3px transparent; border-right: 3px transparent; border-left: 3px transparent; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0px; height: 0px; background: none; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; height: 0px; background: none; }"
        "QScrollBar:left-arrow:horizontal, QScrollBar::right-arrow:horizontal {"
            "border: 0px; width: 0px; height: 0px; background: none; background-color: transparent; }"

        "QScrollBar:vertical { background-color: transparent; }"
        "QScrollBar::handle:vertical { border-width: 9px; margin-right: 6px;"
            "border-image: url(" RESPATH "images/scrollbar-vertical-handle.png) 3 3 3 3 stretch stretch;"
            "border-top: 3px transparent; border-bottom: 3px transparent; border-right: 3px transparent; border-left: 3px transparent; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { width: 0px; height: 0px; background: none; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; height: 0px; background: none; }"
        "QScrollBar:up-arrow:vertical, QScrollBar::down-arrow:vertical {"
            "border: 0px; width: 0px; height: 0px; background: none; background-color: transparent; }" );
}


QPixmap
createTiledPixmap( int width, int height, const QImage& inputTile )
{
    if ( inputTile.isNull() )
        return QPixmap();


    QImage localTile = inputTile;

    if ( localTile.height() < height )
    {
        // image must be at least as tall as we are
        QImage taller( localTile.width(), height, QImage::Format_ARGB32_Premultiplied );
        QPainter p( &taller );
        int curY = 0;
        while ( curY < taller.height() )
        {
            const int thisHeight = (curY + localTile.height() > height) ? height - curY : localTile.height();
            p.drawImage( QRect( 0, curY, localTile.width(), thisHeight ), localTile, QRect( 0, 0, localTile.width(), thisHeight ) );
            curY += localTile.height();
        }
        localTile = taller;
    }


    QPixmap tiledImage = QPixmap( width, height );
    tiledImage.fill( Qt::transparent );

    int curWidth = 0;
    QPainter p( &tiledImage );
    while ( curWidth < width )
    {
        const int thisWidth = (curWidth + localTile.width() > width) ? width - curWidth : localTile.width();

        const QRect source( 0, 0, thisWidth, tiledImage.height() );
        const QRect dest( curWidth, 0, thisWidth, tiledImage.height() );
        p.drawImage( dest, localTile, source );
        curWidth += thisWidth;
    }

    return tiledImage;
}


//  addDropShadow is inspired by QPixmapDropShadowFilter::draw in
//  qt/src/gui/effects/qpixmapfilter.cpp
QPixmap
addDropShadow( const QPixmap& source, const QSize& targetSize )
{
    const QPoint offset( 2, 4 );
    const int radius = 4;
    const QColor shadowColor( 100, 100, 100, 100 );

    // If there is no targetSize, then return a larger pixmap with the shadow added on
    // otherwise, return a bounded pixmap and shrink the source
    const QSize sizeToUse = targetSize.isEmpty() ? QSize( source.width() + offset.x() + radius, source.height() + offset.y() + radius ) : targetSize;
    const QSize shrunkToFit( sizeToUse.width() - offset.x() - radius, sizeToUse.height() - offset.y() - radius );
    const QPixmap shrunk = source.scaled( shrunkToFit, Qt::KeepAspectRatio, Qt::SmoothTransformation );

    QImage tmp( sizeToUse, QImage::Format_ARGB32_Premultiplied );
    tmp.fill( 0 );

    QPainter tmpPainter( &tmp );
    tmpPainter.setCompositionMode( QPainter::CompositionMode_Source );
    tmpPainter.drawPixmap( offset, shrunk );
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred( sizeToUse, QImage::Format_ARGB32_Premultiplied );
    blurred.fill( 0 );
    QPainter blurPainter( &blurred );
    qt_blurImage( &blurPainter, tmp, radius, false, true );
    blurPainter.end();

    // blacken the image...
    QPainter blackenPainter( &blurred );
    blackenPainter.setCompositionMode( QPainter::CompositionMode_SourceIn );
    blackenPainter.fillRect( blurred.rect(), shadowColor );
    blackenPainter.end();

    const QRect resultRect( shrunk.rect().united( shrunk.rect().translated( offset ).adjusted( -radius, -radius, radius, radius ) ) );

    QPixmap result( resultRect.size() );
    result.fill( Qt::transparent );
    QPainter resultPainter( &result );

    // draw the blurred drop shadow...
    resultPainter.drawImage( 0, 0, blurred );

    // Draw the actual pixmap...
    resultPainter.drawPixmap( 0, 0, shrunk );

    return result;
}


} // ns
