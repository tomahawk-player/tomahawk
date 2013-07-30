/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "DpiScaler.h"
#include "TomahawkUtilsGui.h"


namespace TomahawkUtils
{


DpiScaler::DpiScaler( const QPaintDevice* that )
    : that( that )
{
    m_ratioX = ratioX( that );
    m_ratioY = ratioY( that );
}


QSize
DpiScaler::scaled( int w, int h ) const
{
    return QSize( scaledX( w ), scaledY( h ) );
}


QSize
DpiScaler::scaled( const QSize& size ) const
{
    return scaled( size.width(), size.height() );
}


int
DpiScaler::scaledX( int x ) const
{
    return qRound( x * m_ratioX );
}


int
DpiScaler::scaledY( int y ) const
{
    return qRound( y * m_ratioY );
}

// static methods start here

QSize
DpiScaler::scaled( const QPaintDevice* pd, int w, int h )
{
    return QSize( scaledX( pd, w ), scaledY( pd, h ) );
}


QSize
DpiScaler::scaled( const QPaintDevice* pd, const QSize& size )
{
    return scaled( pd, size.width(), size.height() );
}


int
DpiScaler::scaledX( const QPaintDevice* pd, int x )
{
    return qRound( x * ratioX( pd ) );
}


int
DpiScaler::scaledY( const QPaintDevice* pd, int y )
{
    return qRound( y * ratioY( pd ) );
}


qreal
DpiScaler::ratioX( const QPaintDevice* pd )
{
    qreal ratioFromFH = ratioFromFontHeight();
    qreal ratioYFromDpi = pd->logicalDpiY() / s_baseDpi; //using Y because we compare with height

    //if the error is less than 1%, we trust that the logical DPI setting has the best value
    if ( qAbs( ratioFromFH / ratioYFromDpi - 1 ) < 0.01 )
        return pd->logicalDpiX() / s_baseDpi;
    else
        return ratioFromFH;
}


qreal
DpiScaler::ratioY( const QPaintDevice* pd )
{
    qreal ratioFromFH = ratioFromFontHeight();
    qreal ratioYFromDpi = pd->logicalDpiY() / s_baseDpi; //using Y because we compare with height

    //if the error is less than 1%, we trust that the logical DPI setting has the best value
    if ( qAbs( ratioFromFH / ratioYFromDpi - 1 ) < 0.01 )
        return ratioYFromDpi;
    else
        return ratioFromFH;
}


qreal
DpiScaler::ratioFromFontHeight()
{
    int fS = TomahawkUtils::defaultFontSize();
    QFont f;
    f.setPointSize( fS );
    int fH = QFontMetrics( f ).ascent() + 1; //a font's em-height should be ascent + 1px (baseline)

    qreal basePpp = s_baseDpi / 72.; //72*(1.333)=96 dpi

#ifdef Q_OS_MAC
    const int baseFontSize = 13;
#else
    const int baseFontSize = 7;
#endif

    qreal baseFontHeight = baseFontSize * basePpp; //we assume a minimum font size of 7pt

    qreal ratioFromFontHeights = qMax( fH / baseFontHeight, 1. );
    return ratioFromFontHeights;
}


}
