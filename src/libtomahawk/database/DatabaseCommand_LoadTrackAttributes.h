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

#ifndef DATABASECOMMAND_LOADTRACKATTRIBUTES_H
#define DATABASECOMMAND_LOADTRACKATTRIBUTES_H

// #include <QDateTime>
// #include <QList>
#include "database/DatabaseCommand.h"

// #include "SourceList.h"
// #include "Typedefs.h"

#include "trackdata_ptr.h"

#include "DllMacro.h"

/**
 * \class DatabaseCommand_LoadTrackAttributes
 * \brief Database command used to load track attributes from the database.
 *
 * This Database command allows Tomahawk to load track attributes from
 * the local database.
 *
 */
class DLLEXPORT DatabaseCommand_LoadTrackAttributes : public DatabaseCommand
{
Q_OBJECT

public:
    /**
     * \brief Overloaded constructor for DatabaseCommand_LoadTrackAttributes.
     * \param result A Tomahawk Query object.
     * \param parent Parent class.
     *
     * Constructor which creates a new database command for loading all track attributes.
     */
    explicit DatabaseCommand_LoadTrackAttributes( const Tomahawk::trackdata_ptr& track, QObject* parent = 0 );

    /**
     * \brief Returns the name of this database command.
     * \return QString containing the database command name 'loadtrackattributes'.
     */
    virtual QString commandname() const { return "loadtrackattributes"; }

    /**
     * \brief Executes the database command.
     * \param dbi Database instance.
     *
     * This method executes an sql query to load the track attributes
     * from the database and sets it on a Track object.
     *
     * \see Track::setTrackAttributes()
     */
    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }

signals:
    void done();

private:
    Tomahawk::trackdata_ptr m_track;
};

#endif // DATABASECOMMAND_LOADTRACKATTRIBUTES_H
