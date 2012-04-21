/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christopher Reichert <creichert07@gmail.com>
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

#include <QDateTime>
#include <QList>
#include "database/databasecommand.h"

#include "SourceList.h"
#include "Typedefs.h"
#include "Artist.h"

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
    explicit DatabaseCommand_LoadSocialActions( const Tomahawk::query_ptr& query, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_query( query )
    {
        setSource( SourceList::instance()->getLocal() );
        setArtist( query->artist() );
        setTrack( query->track() );
    }

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

    /**
     * \brief Returns the artist associated with this database command.
     * \return Name of the artist.
     * \see setArtist()
     */
    QString artist() const { return m_artist; }

    /**
     * \brief Sets the artist name for this database command.
     * \param s QString containing the artist name.
     * \see artist()
     */
    void setArtist( const QString& s ) { m_artist = s; }

    /**
     * \brief Returns the track name associated with this social action.
     * \return QString containing the track name.
     * \see setTrack()
     */
    QString track() const { return m_track; }

    /**
     * \brief Sets the track name associated with this database command.
     * \param track QString containing the track name.
     * \see track()
     */
    void setTrack( const QString& s ) { m_track = s; }

signals:

    /**
     * \brief Emitted when the database command has finished the Query successfully
     *
     * \param QList of all social actions
     * \see QList
     */
    void done( QList< Tomahawk::SocialAction >& allSocialActions );

private:
    Tomahawk::query_ptr m_query;
    QString m_artist;
    QString m_track;

};

#endif // DATABASECOMMAND_LOADSOCIALACTIONS_H
