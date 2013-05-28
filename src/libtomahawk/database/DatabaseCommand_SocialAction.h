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

#ifndef DATABASECOMMAND_SOCIALACTION_H
#define DATABASECOMMAND_SOCIALACTION_H

#include <QDateTime>
#include "database/DatabaseCommandLoggable.h"

#include "SourceList.h"
#include "Typedefs.h"
#include "Artist.h"
#include "Track.h"

#include "DllMacro.h"

/**
 * \class DatabaseCommand_SocialAction
 * \brief Database command used to write social actions to database.
 *
 * This Database command allows Tomahawk to write social actions to
 * the local database. These social actions can be interfaced with social
 * networking API's such as LastFm, Facebook, or Twitter to allow the user
 * to sync these actions with their accounts on these sites.
 *
 * \see DatabaseCommand_LoadSocialActions
 */
class DLLEXPORT DatabaseCommand_SocialAction : public DatabaseCommandLoggable
{
    Q_OBJECT
    Q_PROPERTY( QString action READ action WRITE setAction )
    Q_PROPERTY( QString comment READ comment WRITE setComment )
    Q_PROPERTY( int timestamp READ timestamp WRITE setTimestamp )
    Q_PROPERTY( QString artist READ artist WRITE setArtist )
    Q_PROPERTY( QString track READ track WRITE setTrack )

public:

    /**
     * \brief Default constructor for DatabaseCommand_SocialAction.
     *
     * Constructs an empty database command for a social action.
     */
    explicit DatabaseCommand_SocialAction( QObject* parent = 0 )
        : DatabaseCommandLoggable( parent )
    {}

    /**
     * \brief Overloaded constructor for DatabaseCommand_SocialAction.
     * \param track A Tomahawk Track object.
     * \param action Name of the social action to be written to the database.
     * \param comment Comment associated with this social action.
     * \param parent Parent class.
     *
     * Constructor which creates a new database command for the specified social action.
     */
    explicit DatabaseCommand_SocialAction( const Tomahawk::trackdata_ptr& track, QString action, QString comment = "", QObject* parent = 0 );

    /**
     * \brief Returns the name of this database command.
     * \return QString containing the database command name 'socialaction'.
     */
    virtual QString commandname() const { return "socialaction"; }

    /**
     * \brief Executes the database command.
     * \param dbi Database instance.
     *
     * This method prepares an sql query to write this social action
     * into the local database.
     */
    virtual void exec( DatabaseImpl* dbi );

    /**
     * \brief Triggers a Database Sync.
     */
    virtual void postCommitHook();

    /**
     * \brief Returns the artist associated with this database command.
     * \return Name of the artist.
     * \see setArtist()
     */
    virtual QString artist() const { return m_artist; }

    /**
     * \brief Sets the artist name for this database command.
     * \param s QString containing the artist name.
     * \see artist()
     */
    virtual void setArtist( const QString& s ) { m_artist = s; }

    /**
     * \brief Returns the track name associated with this social action.
     * \return QString containing the track name.
     * \see setTrack()
     */
    virtual QString track() const { return m_title; }

    /**
     * \brief Sets the track name associated with this database command.
     * \param track QString containing the track name.
     * \see track()
     */
    virtual void setTrack( const QString& title ) { m_title = title; }

    /**
     * \brief Returns the social action for this database command instance.
     * \return QString containing the action name.
     * \see setAction()
     */
    QString action() const { return m_action; }

    /**
     * \brief Sets the social actions
     * \param a QString containing action to be set in this class.
     * \see action()
     */
    void setAction( QString a ) { m_action = a; }

    /**
     * \brief Returns comment associated with this social action.
     * \return QString containing comment associated with this social action.
     * \see setComment()
     */
    virtual QString comment() const { return m_comment; }

    /**
     * \brief Sets the comment associated with this social action.
     * \param com Comment associated with this social action.
     * \see comment()
     */
    virtual void setComment( const QString& com ) { m_comment = com; }

    /**
     * \brief Returns the timestamp associated with this social action.
     * \return unsigned integer containing timestamp
     * \see setTimesetamp()
     */
    virtual int timestamp() const { return m_timestamp; }

    /**
     * \brief Sets the timestamp associated with this social action.
     * \param ts unsigned integer associated with this social action.
     * \see timestamp()
     */
    virtual void setTimestamp( const int ts ) { m_timestamp = ts; }

    virtual bool doesMutates() const { return true; }
    virtual bool groupable() const { return true; }

protected:
    Tomahawk::trackdata_ptr m_track;

private:
    QString m_artist;
    QString m_title;
    int m_timestamp;
    QString m_comment;
    QString m_action; //! currently used values: Love, Inbox
};

#endif // DATABASECOMMAND_SOCIALACTION_H
