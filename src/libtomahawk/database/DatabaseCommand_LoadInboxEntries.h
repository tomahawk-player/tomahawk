/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#ifndef DATABASECOMMAND_LOADINBOXENTRIES_H
#define DATABASECOMMAND_LOADINBOXENTRIES_H

#include "DatabaseCommand.h"

namespace Tomahawk
{

class DatabaseCommand_LoadInboxEntries : public DatabaseCommand
{
    Q_OBJECT
public:
    explicit DatabaseCommand_LoadInboxEntries( QObject* parent = 0 );

    virtual void exec( DatabaseImpl* );
    virtual bool doesMutates() const { return false; }
    virtual QString commandname() const { return "loadinboxentries"; }

signals:
    void tracks( QList< Tomahawk::query_ptr > );
};

}

#endif // DATABASECOMMAND_LOADINBOXENTRIES_H
