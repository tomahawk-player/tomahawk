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
#include "database/databasecommand.h"

#include "sourcelist.h"
#include "typedefs.h"
#include "artist.h"

#include "dllmacro.h"


class DLLEXPORT DatabaseCommand_SocialAction : public DatabaseCommand
{
Q_OBJECT
Q_PROPERTY( QString action READ action WRITE setAction )
Q_PROPERTY( QString comment READ comment WRITE setComment )
Q_PROPERTY( int timestamp READ timestamp WRITE setTimestamp )

public:
    
    explicit DatabaseCommand_SocialAction( QObject* parent = 0 )
        : DatabaseCommand( parent )
    {}
    
    explicit DatabaseCommand_SocialAction( const Tomahawk::result_ptr& result, QString action, QString comment="", QObject* parent = 0 )
        : DatabaseCommand( parent ), m_result( result ), m_action( action )
    {
        setSource( SourceList::instance()->getLocal() );

        setArtist( result->artist()->name() );
        setTrack( result->track() );
        setComment( comment );
        setTimestamp( QDateTime::currentDateTime().toTime_t() );
    }

    virtual QString commandname() const { return "socialaction"; }

    virtual void exec( DatabaseImpl* );
    virtual void postCommitHook();

    QString artist() const { return m_artist; }
    void setArtist( const QString& s ) { m_artist = s; }

    QString track() const { return m_track; }
    void setTrack( const QString& s ) { m_track = s; }
    
    // key
    QString action() const { return m_action; }
    void setAction( QString a ) { m_action = a; }
 
    // value
    QString comment() const { return m_comment; }
    void setComment( const QString& com ) { m_comment = com; }
    
    int timestamp() const { return m_timestamp; }
    void setTimestamp( const int ts ) { m_timestamp = ts; }

private:
    Tomahawk::result_ptr m_result;

    QString m_artist;
    QString m_track;
    int m_timestamp;
    QString m_comment;
    QString m_action;
};

#endif // DATABASECOMMAND_SOCIALACTION_H
