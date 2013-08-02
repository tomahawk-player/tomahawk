/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,    Dominik Schmidt <domme@tomahawk-player.org>
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

#include "ViewPagePlugin.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/DpiScaler.h"
#include "utils/ImageRegistry.h"

Tomahawk::ViewPagePlugin::ViewPagePlugin(QObject* parent)
    : QObject( parent )
{

}

Tomahawk::ViewPagePlugin::~ViewPagePlugin()
{

}


int
Tomahawk::ViewPagePlugin::sortValue()
{
    return 0;
}


QPixmap
Tomahawk::ViewPagePlugin::pixmap() const
{
    // TODO: use DpiScaler here
    return ImageRegistry::instance()->pixmap( pixmapPath(), QSize( 256, 256 ) );
}


const QString
Tomahawk::ViewPagePlugin::pixmapPath() const
{
    return ( RESPATH "icons/tomahawk-icon-256x256.png" );
}
