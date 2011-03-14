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

#include "databasecommand_loadalldynamicplaylists.h"

#include <QSqlQuery>

#include "dynamic/DynamicPlaylist.h"
#include "databaseimpl.h"

using namespace Tomahawk;


void DatabaseCommand_LoadAllDynamicPlaylists::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();

    query.exec( QString( "SELECT playlist.guid as guid, title, info, creator, createdOn, lastmodified, shared, currentrevision, dynamic_playlist.pltype, dynamic_playlist.plmode "
                         "FROM playlist, dynamic_playlist WHERE source %1 AND dynplaylist = 'true' AND playlist.guid = dynamic_playlist.guid" )
    .arg( source()->isLocal() ? "IS NULL" :
    QString( "=%1" ).arg( source()->id() )
    ) );

    QList<dynplaylist_ptr> plists;
    while ( query.next() )
    {
            QVariantList data = QVariantList()  <<      query.value(7).toString()  //current rev
                                                <<      query.value(1).toString()  //title
                                                <<      query.value(2).toString()  //info
                                                <<      query.value(3).toString()  //creator
                                                <<      query.value(4).toString()  //createdOn
                                                <<      query.value(8).toString()  // dynamic type
                                                <<      static_cast<GeneratorMode>(query.value(9).toInt())  // dynamic mode
                                                <<      query.value(6).toBool()    //shared
                                                <<      query.value(5).toInt()     //lastmod
                                                <<      query.value(0).toString();  //GUID
            if( static_cast<GeneratorMode>( query.value(8).toInt() ) == Static )
                emit autoPlaylistLoaded( source(), data );
            else
                emit stationLoaded( source(), data );
    }

    emit done();
}

