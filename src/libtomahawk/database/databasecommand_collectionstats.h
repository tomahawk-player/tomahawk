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

#ifndef DATABASECOMMAND_COLLECTIONSTATS_H
#define DATABASECOMMAND_COLLECTIONSTATS_H

#include <QVariantMap>

#include "DatabaseCommand.h"
#include "Typedefs.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_CollectionStats : public DatabaseCommand
{
Q_OBJECT

public:
    explicit DatabaseCommand_CollectionStats( const Tomahawk::source_ptr& source, QObject* parent = 0 );
    virtual void exec( DatabaseImpl* lib );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "collectionstats"; }

signals:
    void done( const QVariantMap& );
};

#endif // DATABASECOMMAND_COLLECTIONSTATS_H
