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


namespace TomahawkUtils
{


DpiScaler::DpiScaler( const QPaintDevice* that )
    : that( that )
{
}


QSize
DpiScaler::scaled( int w, int h ) const
{
    return scaled( that, w, h );
}


QSize
DpiScaler::scaled( const QSize& size ) const
{
    return scaled( that, size );
}


int
DpiScaler::scaledX( int x ) const
{
    return scaledX( that, x );
}


int
DpiScaler::scaledY( int y ) const
{
    return scaledY( that, y );
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
    float ratioX = pd->logicalDpiX() / 100.0;
    return qRound( x * ratioX );
}


int
DpiScaler::scaledY( const QPaintDevice* pd, int y )
{
    float ratioY = pd->logicalDpiY() / 100.0;
    return qRound( y * ratioY );
}


}
