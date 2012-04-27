/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DatabaseCommand_ImportPlaylist.h"

#include <QSqlQuery>

#include "tomahawk/Query.h"
#include "tomahawk/Playlist.h"
#include "DatabaseImpl.h"
#include "utils/Logger.h"


void
DatabaseCommand_ImportPlaylist::exec( DatabaseImpl * dbi )
{
    /*
    qDebug() << "Importing playlist of" << m_playlist->length() << "tracks";
    TomahawkSqlQuery query = dbi->newquery();
    query.prepare("INSERT INTO playlist(title, info, creator, lastmodified) "
                  "VALUES(?,?,?,?)");
    query.addBindValue(m_playlist->title());
    query.addBindValue(m_playlist->info());
    query.addBindValue(m_playlist->creator());
    query.addBindValue(m_playlist->lastmodified());
    query.exec();
    int pid = query.lastInsertId().toInt();
    int pos = 0;
    query.prepare("INSERT INTO playlist_tracks( "
                  "playlist, position, trackname, albumname, artistname) "
                  "VALUES (?,?,?,?,?)");

    for(int k = 0; k < m_playlist->length(); k++)
    {
        pos++;
        query.addBindValue(pid);
        query.addBindValue(pos);
        query.addBindValue(m_playlist->at(k)->artist());
        query.addBindValue(m_playlist->at(k)->album());
        query.addBindValue(m_playlist->at(k)->track());
        query.exec();
    }
    emit done(pid);
    */
}

