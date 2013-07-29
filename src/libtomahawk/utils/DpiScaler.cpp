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
    float ratioX = that->logicalDpiX() / 100.0;
    float ratioY = that->logicalDpiY() / 100.0;
    return QSize( qRound( w * ratioX ), qRound( h * ratioY ) );
}


QSize
DpiScaler::scaled( const QSize& size ) const
{
    return scaled( size.width(), size.height() );
}


int
DpiScaler::scaledX( int x ) const
{
    return scaled( x, 0 ).width();
}


int
DpiScaler::scaledY( int y ) const
{
    return scaled( 0, y ).height();
}


}
