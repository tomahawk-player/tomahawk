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

#ifndef DATABASECOMMANDLOGGABLE_H
#define DATABASECOMMANDLOGGABLE_H

#include "database/DatabaseCommand.h"
#include "DllMacro.h"

/// A Database Command that will be added to the oplog and sent over the network
/// so peers can sync up and changes to our collection in their cached copy.
class DLLEXPORT DatabaseCommandLoggable : public DatabaseCommand
{
Q_OBJECT
Q_PROPERTY(QString command READ commandname)

public:

    explicit DatabaseCommandLoggable( QObject* parent = 0 )
        : DatabaseCommand( parent )
    {}

    explicit DatabaseCommandLoggable( const Tomahawk::source_ptr& s, QObject* parent = 0 )
        : DatabaseCommand( s, parent )
    {}

    virtual bool loggable() const { return true; }
};

#endif // DATABASECOMMANDLOGGABLE_H
