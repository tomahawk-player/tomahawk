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

#include "sourcelist.h"
#include "typedefs.h"
#include "artist.h"
#include "result.h"

#include "dllmacro.h"


class DLLEXPORT DatabaseCommand_LoadSocialActions : public DatabaseCommand
{
Q_OBJECT

public:
    
    explicit DatabaseCommand_LoadSocialActions( QObject* parent = 0 )
        : DatabaseCommand( parent )
    {}
    
    
    explicit DatabaseCommand_LoadSocialActions( Tomahawk::Result* result, QObject* parent = 0 )
        : DatabaseCommand( parent ), m_result( result )
    {
        setSource( SourceList::instance()->getLocal() );
        setArtist( result->artist()->name() );
        setTrack( result->track() );
    }
    
    virtual QString commandname() const { return "loadsocialactions"; }

    virtual void exec( DatabaseImpl* );
    
    QString artist() const { return m_artist; }
    void setArtist( const QString& s ) { m_artist = s; }

    QString track() const { return m_track; }
    void setTrack( const QString& s ) { m_track = s; }
    
signals:
    void done( QList< Tomahawk::SocialAction >& allSocialActions );
    
private:
    Tomahawk::Result* m_result;
    QString m_artist;
    QString m_track;
   
};

#endif // DATABASECOMMAND_LOADSOCIALACTIONS_H
