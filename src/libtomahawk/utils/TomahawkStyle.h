/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TOMAHAWKSTYLE_H
#define TOMAHAWKSTYLE_H

#include "TomahawkUtils.h"
#include "DllMacro.h"

#include <QColor>
#include <QPainter>
#include <QStyle>

namespace TomahawkStyle
{
    /**
     * Draws a header background on a painter with the specified rectangle
     */
    DLLEXPORT void horizontalHeader( QPainter* painter, const QRect& rect );

    /**
     * Draws a styled arrow that looks good on a Header (from qwindowstyle.cpp)
     * \copyright {  Licensed under the GPL v3+
     *               Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
     *               Contact: Nokia Corporation (qt-info@nokia.com) }
     */
    DLLEXPORT void drawArrow( QStyle::PrimitiveElement, QPainter* painter, const QStyleOption* opt );

    static const QColor BORDER_LINE = QColor( "#8c8c8c" );
    static const QColor POPUP_BACKGROUND = QColor( "#ffffff" );
    static const QColor GROUP_HEADER = QColor( "#637180" );
    static const QColor HOVER_GLOW = QColor( "#dddddd" );

    static const QColor NOW_PLAYING_ITEM = QColor( "#962c26" );
    static const QColor NOW_PLAYING_ITEM_TEXT = QColor( "#ffffff" );
    static const QColor SELECTION_BACKGROUND = QColor( "#962c26" );
    static const QColor SELECTION_FOREGROUND = QColor( "#ffffff" );

    static const QColor HEADER_UPPER = QColor( "#25292c" );
    static const QColor HEADER_LOWER = QColor( "#707070" );
    static const QColor HEADER_TEXT = QColor( "#eaeaea" );
    static const QColor HEADER_HIGHLIGHT = QColor( "#333" );

    static const int POPUP_ROUNDING_RADIUS = 6;
    static const float POPUP_OPACITY = 0.96;
}

#endif // TOMAHAWKSTYLE_H
