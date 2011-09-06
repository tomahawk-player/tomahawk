/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Casey Link <unnamedrambler@gmail.com>
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

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QColor>
#include <QStyle>
#include <QStyleOption>

class QPalette;
class QPainter;

/**
 * \class StyleHelper
 * \brief A static class that contains Tomahawk's styling details (colors, etc).
 */
class StyleHelper
{
public:

    /**
     * Our header BG is a two color gradient. This is the upper color.
     */
    static QColor headerUpperColor();
    /**
     * Our header BG is a two color gradient. This is the lower color.
     */
    static QColor headerLowerColor();
    /**
     * The color of text on a Header.
     */
    static QColor headerTextColor();
    /**
     * The widget highlight color for headers
     */
    static QColor headerHighlightColor();

    /**
     * Draws a header background on a painter with the specified rectangle
     */
    static void horizontalHeader( QPainter* painter, const QRect& rect );

    /**
     * Draws a styled arrow that looks good on a Header (from qwindowstyle.cpp)
     * \copyright {  Licensed under the GPL v3+
     *               Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
     *               Contact: Nokia Corporation (qt-info@nokia.com) }
     */
    static void drawArrow( QStyle::PrimitiveElement, QPainter* painter, const QStyleOption* opt );
};

#endif // STYLEHELPER_H
