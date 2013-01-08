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

#ifndef TOMAHAWKUTILSGUI_H
#define TOMAHAWKUTILSGUI_H

#include "TomahawkUtils.h"
#include "DllMacro.h"

#include <QSize>
#include <QModelIndex>
#include <QColor>
#include <QRect>
#include <QTextOption>
#include <QImage>

// include contains typedefs for Qt4/Qt5 compatibility
#include <QStyleOption>

class PlayableItem;
class QPainter;
class QPixmap;
class QLayout;
class QPalette;
class QRect;
class QScrollBar;

namespace TomahawkUtils
{
    DLLEXPORT void drawQueryBackground( QPainter* p, const QRect& r );
    DLLEXPORT QWidget* tomahawkWindow();
    /// Platform-specific bringing tomahawk mainwindow to front, b/c qt's activate() and such don't seem to work well enough for us
    DLLEXPORT void bringToFront();

    DLLEXPORT void openUrl( const QUrl& url );

    DLLEXPORT QPixmap createRoundedImage( const QPixmap& avatar, const QSize& size, float frameWidthPct = 0.20 );

    DLLEXPORT QColor alphaBlend( const QColor& colorFrom, const QColor& colorTo, float opacity );
    DLLEXPORT QPixmap createDragPixmap( MediaType type, int itemCount = 1 );

    DLLEXPORT void drawShadowText( QPainter* p, const QRect& rect, const QString& text, const QTextOption& textOption );
    DLLEXPORT void drawBackgroundAndNumbers( QPainter* p, const QString& text, const QRect& rect );

    DLLEXPORT void unmarginLayout( QLayout* layout );

    DLLEXPORT void setDefaultFontSize( int points );
    DLLEXPORT int defaultFontSize();
    DLLEXPORT int defaultFontHeight();
    DLLEXPORT QSize defaultIconSize();

    DLLEXPORT void prepareStyleOption( QStyleOptionViewItemV4* option, const QModelIndex& index, PlayableItem* item );

    DLLEXPORT void drawRoundedButton( QPainter* painter, const QRect& btnRect, const QColor& color, const QColor &gradient1bottom = QColor(), const QColor& gradient2top = QColor(), const QColor& gradient2bottom = QColor() );
    DLLEXPORT void styleScrollBar( QScrollBar* scrollBar );

    DLLEXPORT QPixmap defaultPixmap( ImageType type, ImageMode mode = TomahawkUtils::Original, const QSize& size = QSize( 0, 0 ) );
    DLLEXPORT QPixmap createTiledPixmap( int width, int height, const QImage& src );
    DLLEXPORT QPixmap addDropShadow( const QPixmap& sourceImage, const QSize& targetSize );
    DLLEXPORT QPixmap squareCenterPixmap( const QPixmap& sourceImage );

    DLLEXPORT void drawCompositedPopup( QWidget* widget, const QPainterPath& outline, const QColor& lineColor, const QBrush& backgroundBrush, qreal opacity );

    namespace Colors
    {
        static const QColor BORDER_LINE = QColor( "#8c8c8c" );
        static const QColor POPUP_BACKGROUND = QColor( "#ffffff" );
        static const QColor GROUP_HEADER = QColor( "#637180" );
        static const QColor NOW_PLAYING_ITEM = QColor( "#962c26" );
        static const QColor NOW_PLAYING_ITEM_TEXT = QColor( "#ffffff" );
        static const QColor SELECTION_BACKGROUND = QColor( "#962c26" );
        static const QColor SELECTION_FOREGROUND = QColor( "#ffffff" );
    }

    static const int POPUP_ROUNDING_RADIUS = 6;
    static const float POPUP_OPACITY = 0.96;
}

#endif // TOMAHAWKUTILSGUI_H
