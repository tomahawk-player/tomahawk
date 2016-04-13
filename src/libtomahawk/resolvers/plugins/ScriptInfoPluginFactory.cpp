/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Dominik Schmidt <domme@tomahawk-player.org>
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

#include "ScriptInfoPluginFactory.h"

#include "SourceList.h"
#include "../ScriptAccount.h"

using namespace Tomahawk;

void
ScriptInfoPluginFactory::addPlugin( const QSharedPointer< ScriptInfoPlugin >& infoPlugin ) const
{
    Tomahawk::InfoSystem::InfoSystem::instance()->addInfoPlugin( infoPlugin );
}


void
ScriptInfoPluginFactory::removePlugin( const QSharedPointer< ScriptInfoPlugin >& infoPlugin ) const
{
    Tomahawk::InfoSystem::InfoSystem::instance()->removeInfoPlugin( infoPlugin );
}


QSharedPointer< ScriptInfoPlugin >
ScriptInfoPluginFactory::createPlugin( const scriptobject_ptr& object, ScriptAccount* scriptAccount )
{
    // create infoplugin instance
    ScriptInfoPlugin* scriptInfoPlugin = new ScriptInfoPlugin( object, scriptAccount->name() );

    QSharedPointer< ScriptInfoPlugin > infoPlugin( scriptInfoPlugin );

    // move it to infosystem thread
    infoPlugin->moveToThread( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() );

    return infoPlugin;
}
