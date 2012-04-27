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

#ifndef DATABASECOMMAND_LOADOPS_H
#define DATABASECOMMAND_LOADOPS_H

#include "Typedefs.h"
#include "DatabaseCommand.h"
#include "Op.h"

#include "DllMacro.h"

class DLLEXPORT DatabaseCommand_loadOps : public DatabaseCommand
{
Q_OBJECT
public:
    explicit DatabaseCommand_loadOps( const Tomahawk::source_ptr& src, QString since, QObject* parent = 0 )
        : DatabaseCommand( src ), m_since( since )
    {
        Q_UNUSED( parent );
    }

    virtual void exec( DatabaseImpl* db );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadops"; }

signals:
    void done( QString sinceguid, QString lastguid, QList< dbop_ptr > ops );

private:
    QString m_since; // guid to load from
};

#endif // DATABASECOMMAND_LOADOPS_H
