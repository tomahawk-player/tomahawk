/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2016  Dominik Schmidt <domme@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
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
#include "ScriptLinkParserPluginFactory.h"

#include "../ScriptAccount.h"
#include "../../utils/LinkParser.h"
#include "../../utils/LinkParserPlugin.h"

using namespace Tomahawk;

void ScriptLinkParserPluginFactory::addPlugin( const QSharedPointer <ScriptLinkParserPlugin >& plugin ) const
{
    Tomahawk::Utils::LinkParser::instance()->addPlugin( plugin );
}

void ScriptLinkParserPluginFactory::removePlugin( const QSharedPointer< ScriptLinkParserPlugin >& plugin ) const
{
    Tomahawk::Utils::LinkParser::instance()->removePlugin( plugin );
}

QSharedPointer< ScriptLinkParserPlugin > ScriptLinkParserPluginFactory::createPlugin( const scriptobject_ptr& object, ScriptAccount* scriptAccount )
{
    return QSharedPointer< ScriptLinkParserPlugin >( new ScriptLinkParserPlugin( object, scriptAccount ) );
}
