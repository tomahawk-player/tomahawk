/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "playlist/dynamic/GeneratorInterface.h"

#include "utils/Logger.h"
#include "Source.h"


Tomahawk::GeneratorInterface::GeneratorInterface( QObject* parent )
    : QObject( parent )
{
}


Tomahawk::GeneratorInterface::~GeneratorInterface()
{
}


QPixmap
Tomahawk::GeneratorInterface::logo()
{
    return QPixmap();
}


QVariantMap
Tomahawk::GeneratorInterface::controls() const
{
    return m_controls;
}


void
Tomahawk::GeneratorInterface::setControls( const QVariantMap& controls )
{
    m_controls = controls;
}
