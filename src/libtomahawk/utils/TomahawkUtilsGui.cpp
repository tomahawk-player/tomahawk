/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#include "playlist/PlayableItem.h"
#include "config.h"
#include "Query.h"
#include "Result.h"
#include "Source.h"
#include "ImageRegistry.h"
#include "Logger.h"

#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QPalette>
#include <QApplication>
#include <QScrollBar>
#include <QWidget>
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

#ifdef QT_MAC_USE_COCOA
    #include "widgets/SourceTreePopupDialog_mac.h"
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
            pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Original, QSize( size, size ) );
            break;
        case MediaTypeAlbum:
            pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::Original, QSize( size, size ) );
            break;
        case MediaTypeTrack:
            pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, QSize( size, size ) );
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
    painter->drawText( figRect.adjusted( -5, 2, 6, 0 ), text, QTextOption( Qt::AlignCenter ) );

    painter->restore();
}


void
drawQueryBackground( QPainter* p, const QRect& r )
{
    p->save();

    p->setPen( Colors::SELECTION_BACKGROUND );
    p->setBrush( Colors::SELECTION_BACKGROUND );
    p->drawRoundedRect( r, 4.0, 4.0 );

    p->restore();
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
createRoundedImage( const QPixmap& pixmap, const QSize& size, float frameWidthPct )
{
    int height;
    int width;

    if ( !size.isEmpty() )
    {
        height = size.height();
        width = size.width();
    }
    else
    {
        height = pixmap.height();
        width = pixmap.width();
    }

    if ( !height || !width )
        return QPixmap();

    QPixmap scaledAvatar = pixmap.scaled( width, height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    if ( frameWidthPct == 0.00 )
        return scaledAvatar;

    QPixmap frame( width, height );
    frame.fill( Qt::transparent );

    QPainter painter( &frame );
    painter.setRenderHint( QPainter::Antialiasing );

    QRect outerRect( 0, 0, width, height );
    QBrush brush( scaledAvatar );
    QPen pen;
    pen.setColor( Qt::transparent );
    pen.setJoinStyle( Qt::RoundJoin );

    painter.setBrush( brush );
    painter.setPen( pen );
    painter.drawRoundedRect( outerRect, frameWidthPct * 100.0, frameWidthPct * 100.0, Qt::RelativeSize );

/*    painter.setBrush( Qt::transparent );
    painter.setPen( Qt::white );
    painter.drawRoundedRect( outerRect, frameWidthPct, frameWidthPct, Qt::RelativeSize ); */

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


QSize
defaultIconSize()
{
    const int w = defaultFontHeight() * 1.6;
    return QSize( w, w );
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

    switch ( type )
    {
        case DefaultAlbumCover:
            if ( mode == CoverInCase )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/no-album-art-placeholder.svg", size );
            else if ( mode == Grid )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/album-placeholder-grid.svg", size );
            else
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/album-icon.svg", size );
            break;

        case DefaultArtistImage:
            if ( mode == Grid )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/artist-placeholder-grid.svg", size );
            else
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/artist-icon.svg", size );
            break;

        case DefaultTrackImage:
            if ( mode == Grid )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/track-placeholder-grid.svg", size );
            else if ( mode == RoundedCorners )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/track-icon.svg", size, TomahawkUtils::RoundedCorners );
            else
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/track-icon.svg", size );
            break;

        case DefaultSourceAvatar:
            if ( mode == RoundedCorners )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/user-avatar.svg", size, TomahawkUtils::RoundedCorners );
            else
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/user-avatar.svg", size );
            break;

        case DefaultResolver:
            if ( mode == RoundedCorners )
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/resolver-default.svg", size, TomahawkUtils::RoundedCorners );
            else
                pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/resolver-default.svg", size );
            break;

        case DefaultCollection:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/collection.svg", size );
            break;

        case NowPlayingSpeaker:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/now-playing-speaker.svg", size );
            break;

        case NowPlayingSpeakerDark:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/now-playing-speaker-dark.svg", size );
            break;

        case InfoIcon:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/info.svg", size );
            break;

        case PlayButton:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/play-rest.svg", size );
            break;
        case PlayButtonPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/play-pressed.svg", size );
            break;

        case PauseButton:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/pause-rest.svg", size );
            break;
        case PauseButtonPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/pause-pressed.svg", size );
            break;

        case PrevButton:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/back-rest.svg", size );
            break;
        case PrevButtonPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/back-pressed.svg", size );
            break;

        case NextButton:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/skip-rest.svg", size );
            break;
        case NextButtonPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/skip-pressed.svg", size );
            break;

        case ShuffleOff:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/shuffle-off-rest.svg", size );
            break;
        case ShuffleOffPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/shuffle-off-pressed.svg", size );
            break;
        case ShuffleOn:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/shuffle-on-rest.svg", size );
            break;
        case ShuffleOnPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/shuffle-on-pressed.svg", size );
            break;

        case RepeatOne:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/repeat-1-on-rest.svg", size );
            break;
        case RepeatOnePressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/repeat-1-on-pressed.svg", size );
            break;
        case RepeatAll:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/repeat-all-on-rest.svg", size );
            break;
        case RepeatAllPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/repeat-all-on-pressed.svg", size );
            break;
        case RepeatOff:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/repeat-off-rest.svg", size );
            break;
        case RepeatOffPressed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/repeat-off-pressed.svg", size );
            break;

        case VolumeMuted:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/volume-icon-muted.svg", size );
            break;
        case VolumeFull:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/volume-icon-full.svg", size );
            break;

        case Share:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/share.svg", size );
            break;

        case NotLoved:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/not-loved.svg", size );
            break;
        case Loved:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/loved.svg", size );
            break;

        case Configure:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/configure.svg", size );
            break;

        case GreenDot:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/green-dot.svg", size );
            break;

        case AddContact:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/add-contact.svg", size );
            break;

        case SubscribeOn:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/subscribe-on.svg", size );
            break;
        case SubscribeOff:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/subscribe-off.svg", size );
            break;

        case JumpLink:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/jump-link.svg", size );
            break;

        case ProcessStop:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/process-stop.svg", size );
            break;

        case HeadphonesOn:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/headphones.svg", size );
            break;
        case HeadphonesOff:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/headphones-off.svg", size );
            break;

        case PadlockClosed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/closed-padlock.svg", size );
            break;
        case PadlockOpen:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/open-padlock.svg", size );
            break;

        case Downloading:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/downloading.svg", size );
            break;
        case Uploading:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/uploading.svg", size );
            break;

        case ViewRefresh:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/view-refresh.svg", size );
            break;

        case SuperCollection:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/supercollection.svg", size );
            break;
        case LovedPlaylist:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/loved_playlist.svg", size );
            break;
        case NewReleases:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/new-releases.svg", size );
            break;
        case NewAdditions:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/new-additions.svg", size );
            break;
        case RecentlyPlayed:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/recently-played.svg", size );
            break;
        case Charts:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/charts.svg", size );
            break;
        case AutomaticPlaylist:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/automatic-playlist.svg", size );
            break;
        case Station:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/station.svg", size );
            break;
        case Playlist:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/playlist-icon.svg", size );
            break;
        case Search:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/search-icon.svg", size );
            break;

        case Add:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/add.svg", size );
            break;
        case ListAdd:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/list-add.svg", size );
            break;
        case ListRemove:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/list-remove.svg", size );
            break;

        case AdvancedSettings:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/advanced-settings.svg", size );
            break;
        case AccountSettings:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/account-settings.svg", size );
            break;
        case MusicSettings:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/music-settings.svg", size );
            break;

        case DropSong:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-song.svg", size );
            break;
        case DropAlbum:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-album.svg", size );
            break;
        case DropAllSongs:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-all-songs.svg", size );
            break;
        case DropLocalSongs:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-local-songs.svg", size );
            break;
        case DropTopSongs:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-top-songs.svg", size );
            break;

        case Starred:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/starred.svg", size );
            break;
        case Unstarred:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/star-unstarred.svg", size );
            break;
        case StarHovered:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/star-hover.svg", size );
            break;

        case SipPluginOnline:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/sipplugin-online.svg", size );
            break;
        case SipPluginOffline:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/sipplugin-offline.svg", size );
            break;

        case AccountNone:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/account-none.svg", size );
            break;
        case LastfmIcon:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/lastfm-icon.svg", size );
            break;
        case SpotifyIcon:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/spotify-sourceicon.svg", size );
            break;
        case SoundcloudIcon:
            pixmap = ImageRegistry::instance()->pixmap( RESPATH "images/soundcloud.svg", size );
            break;

        default:
            break;
    }

    if ( pixmap.isNull() )
    {
        Q_ASSERT( false );
        return QPixmap();
    }

    return pixmap;
}


void
prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item )
{
    Q_UNUSED( index );

    if ( item->isPlaying() )
    {
        option->backgroundBrush = TomahawkUtils::Colors::NOW_PLAYING_ITEM;
        option->palette.setColor( QPalette::Highlight, TomahawkUtils::Colors::NOW_PLAYING_ITEM.lighter() );
        option->palette.setColor( QPalette::Text, TomahawkUtils::Colors::NOW_PLAYING_ITEM_TEXT );

    }
    else if ( option->state & QStyle::State_Selected )
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


QPixmap
squareCenterPixmap( const QPixmap& sourceImage )
{
    if ( sourceImage.width() != sourceImage.height() )
    {
        const int sqwidth = qMin( sourceImage.width(), sourceImage.height() );
        const int delta = abs( sourceImage.width() - sourceImage.height() );

        if ( sourceImage.width() > sourceImage.height() )
        {
            return sourceImage.copy( delta / 2, 0, sqwidth, sqwidth );
        }
        else
        {
            return sourceImage.copy( 0, delta / 2, sqwidth, sqwidth );
        }
    }

    return sourceImage;
}


void
drawCompositedPopup( QWidget* widget,
                     const QPainterPath& outline,
                     const QColor& lineColor,
                     const QBrush& backgroundBrush,
                     qreal opacity )
{
    bool compositingWorks = true;
#if defined(Q_WS_WIN)   //HACK: Windows refuses to perform compositing so we must fake it
    compositingWorks = false;
#elif defined(Q_WS_X11)
    if ( !QX11Info::isCompositingManagerRunning() )
        compositingWorks = false;
#endif

    QPainter p;
    QImage result;
    if ( compositingWorks )
    {
        p.begin( widget );
        p.setRenderHint( QPainter::Antialiasing );
        p.setBackgroundMode( Qt::TransparentMode );
    }
    else
    {
        result =  QImage( widget->rect().size(), QImage::Format_ARGB32_Premultiplied );
        p.begin( &result );
        p.setCompositionMode( QPainter::CompositionMode_Source );
        p.fillRect( result.rect(), Qt::transparent );
        p.setCompositionMode( QPainter::CompositionMode_SourceOver );
    }

    QPen pen( lineColor );
    pen.setWidth( 2 );
    p.setPen( pen );
    p.drawPath( outline );

    p.setOpacity( opacity );
    p.fillPath( outline, backgroundBrush );
    p.end();

    if ( !compositingWorks )
    {
        QPainter finalPainter( widget );
        finalPainter.setRenderHint( QPainter::Antialiasing );
        finalPainter.setBackgroundMode( Qt::TransparentMode );
        finalPainter.drawImage( 0, 0, result );
        widget->setMask( QPixmap::fromImage( result ).mask() );
    }

#ifdef QT_MAC_USE_COCOA
    // Work around bug in Qt/Mac Cocoa where opening subsequent popups
    // would incorrectly calculate the background due to it not being
    // invalidated.
    SourceTreePopupHelper::clearBackground( widget );
#endif
}


} // ns
