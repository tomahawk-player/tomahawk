/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <QtDebug>

#include "infosystemcache.h"

void
Tomahawk::InfoSystem::InfoSystemCache::getCachedInfoSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, Tomahawk::InfoSystem::InfoCustomData customData )
{
    qDebug() << Q_FUNC_INFO;
    emit notInCache( criteria, caller, type, input, customData );
}

void
Tomahawk::InfoSystem::InfoSystemCache::updateCacheSlot( Tomahawk::InfoSystem::InfoCacheCriteria criteria, Tomahawk::InfoSystem::InfoType type, QVariant output )
{
    qDebug() << Q_FUNC_INFO;
}
