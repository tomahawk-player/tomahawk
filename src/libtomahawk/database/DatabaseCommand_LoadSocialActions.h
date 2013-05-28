/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christopher Reichert <creichert07@gmail.com>
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef DATABASECOMMAND_LOADSOCIALACTIONS_H
#define DATABASECOMMAND_LOADSOCIALACTIONS_H


#include "database/DatabaseCommand.h"

// #include "SourceList.h"
// #include "Typedefs.h"
#include "track_ptr.h"
#include "trackdata_ptr.h"
// #include "Artist.h"
#include "SocialAction.h" // MetaType

// #include <QMap>
// #include <QDateTime>
// #include <QList>

#include "DllMacro.h"

/**
 * \class DatabaseCommand_LoadSocialActions
 * \brief Database command used to load social actions from the database.
 *
 * This Database command allows Tomahawk to load social actions from
 * the local database. The loaded social actions can be used to create
 * dynamic playlists, generate statistics and provide data to share with
 * friends on tomahawk.
 *
 * \see DatabaseCommand_SocialAction
 */
class DLLEXPORT DatabaseCommand_LoadSocialActions : public DatabaseCommand
{
Q_OBJECT

public:
    typedef QMap<Tomahawk::track_ptr, Tomahawk::SocialAction> TrackActions;

    /**
     * \brief Default constructor for DatabaseCommand_LoadSocialActions.
     *
     * Constructs an empty database command for loading social actions.
     */
    explicit DatabaseCommand_LoadSocialActions( QObject* parent = 0 )
        : DatabaseCommand( parent )
    {}

    /**
     * \brief Overloaded constructor for DatabaseCommand_LoadSocialAction.
     * \param result A Tomahawk Query object.
     * \param parent Parent class.
     *
     * Constructor which creates a new database command for loading all social actions.
     */
    explicit DatabaseCommand_LoadSocialActions( const Tomahawk::trackdata_ptr& track, QObject* parent = 0 );

    /**
     * Load all tracks with a specific social action
     */
    explicit DatabaseCommand_LoadSocialActions( const QString& action, const Tomahawk::source_ptr& source, QObject* parent = 0 );

    /**
     * \brief Returns the name of this database command.
     * \return QString containing the database command name 'loadsocialaction'.
     */
    virtual QString commandname() const { return "loadsocialactions"; }

    /**
     * \brief Executes the database command.
     * \param dbi Database instance.
     *
     * This method prepares an sql query to load the social actions
     * from the database into a list of all social actions.
     *
     * \see Result::setAllSocialActions()
     */
    virtual void exec( DatabaseImpl* );

    virtual bool doesMutates() const { return false; }

signals:
    /**
     * All loaded social actions for each track found, for queries that generate all tracks
     * with matching actions.
     */
    void done( DatabaseCommand_LoadSocialActions::TrackActions actionsForTracks );

private:
    Tomahawk::trackdata_ptr m_track;
    QString m_actionOnly;

};

Q_DECLARE_METATYPE( DatabaseCommand_LoadSocialActions::TrackActions )

#endif // DATABASECOMMAND_LOADSOCIALACTIONS_H
