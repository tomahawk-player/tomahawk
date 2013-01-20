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

#ifndef DATABASECOMMAND_SHARETRACK_H
#define DATABASECOMMAND_SHARETRACK_H

#include <QObject>
#include <QVariantMap>

#include "database/DatabaseCommandLoggable.h"
#include "SourceList.h"
#include "Typedefs.h"

#include "DllMacro.h"

class DatabaseCommand_ShareTrack : public DatabaseCommandLoggable
{
    Q_OBJECT
    Q_PROPERTY( QString artist      READ artist     WRITE setArtist )
    Q_PROPERTY( QString track       READ track      WRITE setTrack )
    Q_PROPERTY( QString recipient   READ recipient  WRITE setRecipient )

public:
    explicit DatabaseCommand_ShareTrack( QObject* parent = 0 );

    explicit DatabaseCommand_ShareTrack( const Tomahawk::query_ptr& query,
                                         const QString& recipientDbid,
                                         QObject* parent = 0 );

    explicit DatabaseCommand_ShareTrack( const Tomahawk::result_ptr& result,
                                         const QString& recipientDbid,
                                         QObject* parent = 0 );

    //TODO: construct from result instead?

    virtual QString commandname() const { return "sharetrack"; }

    virtual void exec( DatabaseImpl* );
    virtual void postCommitHook();

    virtual bool doesMutates() const { return false; }
    virtual bool singletonCmd() const { return false; }
    virtual bool localOnly() const { return false; }
    virtual bool groupable() const { return true; }

    QString artist() const { return m_artist; }
    void setArtist( const QString& s ) { m_artist = s; }

    QString track() const { return m_track; }
    void setTrack( const QString& s ) { m_track = s; }

    QString recipient() const { return m_recipient; }
    void setRecipient( const QString& s ) { m_recipient = s; }

private:
    Tomahawk::query_ptr m_query;
    Tomahawk::result_ptr m_result;

    QString m_artist;
    QString m_track;
    QString m_recipient;
};

#endif // DATABASECOMMAND_SHARETRACK_H
