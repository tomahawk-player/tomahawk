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

#include "playlist/dynamic/GeneratorFactory.h"
#include "playlist/dynamic/GeneratorInterface.h"

#include "utils/Logger.h"
#include "Source.h"

using namespace Tomahawk;


QHash< QString, GeneratorFactoryInterface* > GeneratorFactory::s_factories = QHash< QString, GeneratorFactoryInterface* >();

geninterface_ptr
GeneratorFactory::create ( const QString& type )
{
    if( type.isEmpty() && !s_factories.isEmpty() ) // default, return first
        return geninterface_ptr( s_factories.begin().value()->create() );

    if( !s_factories.contains( type ) )
        return geninterface_ptr();

    return geninterface_ptr( s_factories.value( type )->create() );
}


void
GeneratorFactory::registerFactory ( const QString& type, GeneratorFactoryInterface* interface )
{
    s_factories.insert( type, interface );
}


QStringList
GeneratorFactory::types()
{
    return s_factories.keys();
}

