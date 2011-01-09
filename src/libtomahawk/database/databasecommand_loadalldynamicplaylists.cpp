/****************************************************************************************
 * Copyright (c) 2010 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
#include "databasecommand_loadalldynamicplaylists.h"

#include <QSqlQuery>

#include "dynamic/DynamicPlaylist.h"
#include "databaseimpl.h"

using namespace Tomahawk;


void DatabaseCommand_LoadAllDynamicPlaylists::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    
    query.exec( QString( "SELECT playlist.guid as guid, title, info, creator, lastmodified, shared, currentrevision, dynamic_playlist.pltype, dynamic_playlist.plmode "
                         "FROM playlist, dynamic_playlist WHERE source %1 AND dynplaylist = 'true' AND playlist.guid = dynamic_playlist.guid" )
    .arg( source()->isLocal() ? "IS NULL" :
    QString( "=%1" ).arg( source()->id() )
    ) );
    
    QList<dynplaylist_ptr> plists;
    while ( query.next() )
    {
        dynplaylist_ptr p( new DynamicPlaylist( source(),                  //src
                                      query.value(6).toString(), //current rev
                                      query.value(1).toString(), //title
                                      query.value(2).toString(), //info
                                      query.value(3).toString(), //creator
                                      query.value(7).toString(), // dynamic type
                                      static_cast<GeneratorMode>(query.value(8).toInt()), // dynamic mode
                                      query.value(5).toBool(),   //shared
                                      query.value(4).toInt(),    //lastmod
                                      query.value(0).toString()  //GUID
                                             
        ) );
        plists.append( p );
    }
    
    emit done( plists );
}

