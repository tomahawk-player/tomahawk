/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef DATABASECOMMAND_FORCERESYNC_H
#define DATABASECOMMAND_FORCERESYNC_H

#include <QDateTime>

#include "database/DatabaseCommandLoggable.h"
#include "Typedefs.h"

#include "DllMacro.h"

namespace Tomahawk
{

/**
 * \class DatabaseCommand_ForceResync
 * \brief Database command used to force an entire resync of this collection
 */
class DLLEXPORT DatabaseCommand_ForceResync : public DatabaseCommandLoggable
{
    Q_OBJECT

public:

    /**
     * \brief Default constructor for DatabaseCommand_ForceResync.
     *
     * Constructs an empty database command for a forced resync
     */
    explicit DatabaseCommand_ForceResync( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    /**
     * \brief Returns the name of this database command.
     * \return QString containing the database command name 'forceresync'.
     */
    virtual QString commandname() const { return "forceresync"; }

    /**
     * \brief Executes the database command.
     * \param dbi Database instance.
     *
     * This method executes sql queries which delete an entire collection's
     * data from the local database.
     */
    virtual void exec( DatabaseImpl* dbi );

    /**
     * \brief Triggers a Database Sync.
     */
    virtual void postCommitHook();

    virtual bool doesMutates() const { return true; }
    virtual bool groupable() const { return false; }

protected:

private:
};

}

#endif // DATABASECOMMAND_FORCERESYNC_H
