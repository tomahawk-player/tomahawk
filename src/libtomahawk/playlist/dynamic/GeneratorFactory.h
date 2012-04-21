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

#ifndef GENERATOR_FACTORY_H
#define GENERATOR_FACTORY_H

#include <QHash>
#include <QString>

#include "playlist/dynamic/GeneratorInterface.h"
#include "Typedefs.h"

#include "DllMacro.h"

namespace Tomahawk
{

/**
  * Generators should subclass this and have it create the custom Generator
  */
class DLLEXPORT GeneratorFactoryInterface
{
public:
    GeneratorFactoryInterface() {}

    virtual ~GeneratorFactoryInterface() {}

    virtual GeneratorInterface* create() = 0;
    /**
     * Create a control for this generator, not tied to this generator itself. Used when loading dynamic
     *  playlists from a dbcmd.
     */
    virtual dyncontrol_ptr createControl( const QString& controlType = QString() ) = 0;

    virtual QStringList typeSelectors() const = 0;
};

/**
 * Simple factory that generates Generators from string type descriptors
 */
class DLLEXPORT GeneratorFactory
{
public:
    static geninterface_ptr create( const QString& type );
    // only used when loading from dbcmd
    static dyncontrol_ptr createControl( const QString& generatorType, const QString& controlType = QString() );

    static void registerFactory( const QString& type, GeneratorFactoryInterface* interface );
    static QStringList types();
    static QStringList typeSelectors( const QString& type );

private:
    static QHash<QString, GeneratorFactoryInterface*> s_factories;

};

};

#endif
