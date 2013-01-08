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

#include <QDateTime>
#include <QList>
#include "database/DatabaseCommand.h"

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
    typedef QMap<Tomahawk::query_ptr,Tomahawk::SocialAction> TrackActions;
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
     * Load all tracks with a specific social action
     */
    explicit DatabaseCommand_LoadSocialActions( const QString& action, const Tomahawk::source_ptr& source, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_actionOnly( action )
    {
        setSource( source );
        qRegisterMetaType<TrackActions>( "DatabaseCommand_LoadSocialActions::TrackActions" );
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

    virtual bool doesMutates() const { return false; }
signals:
    /**
     * All loaded social actions for each track found, for queries that generate all tracks
     * with matching actions.
     */
    void done( DatabaseCommand_LoadSocialActions::TrackActions actionsForTracks );

private:
    Tomahawk::query_ptr m_query;
    QString m_artist;
    QString m_track;
    QString m_actionOnly;

};

//FIXME: Qt5: this fails with Qt5, is it needed at all? It compiles fine without in Qt4 as well
// Q_DECLARE_METATYPE( DatabaseCommand_LoadSocialActions::TrackActions )

#endif // DATABASECOMMAND_LOADSOCIALACTIONS_H
