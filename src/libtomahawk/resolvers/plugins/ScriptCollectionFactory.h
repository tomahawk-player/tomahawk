/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright (C) 2015  Dominik Schmidt <domme@tomahawk-player.org>
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

#pragma once
#ifndef TOMAHAWK_SCRIPTCOLLECTIONFACTORY_H
#define TOMAHAWK_SCRIPTCOLLECTIONFACTORY_H

#include "Typedefs.h"
#include "../ScriptPluginFactory.h"
#include "../ScriptCollection.h"

namespace Tomahawk
{

class ScriptAccount;
class ScriptCollection;


class DLLEXPORT ScriptCollectionFactory : public ScriptPluginFactory< ScriptCollection >
{
    QSharedPointer< ScriptCollection > createPlugin( const scriptobject_ptr&, ScriptAccount* ) override;
    void addPlugin( const QSharedPointer< ScriptCollection >& scriptPlugin ) const override;
    void removePlugin( const QSharedPointer< ScriptCollection >& scriptPlugin ) const override;
};

} // ns: Tomahawk

#endif // TOMAHAWK_SCRIPTCOLLECTIONFACTORY_H
