/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "DatabaseCommand_LoadAllAutoPlaylists.h"

#include "DatabaseImpl.h"
#include "Source.h"
#include "playlist/dynamic/DynamicPlaylist.h"
#include "utils/Logger.h"


#include <QSqlQuery>


using namespace Tomahawk;


void
DatabaseCommand_LoadAllAutoPlaylists::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QString orderToken, sourceToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case DatabaseCommand_LoadAllPlaylists::ModificationTime:
            orderToken = "playlist.createdOn";
    }

    if ( !source().isNull() )
        sourceToken = QString( "AND source %1 " ).arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );


    query.exec( QString( "SELECT playlist.guid as guid, title, info, creator, createdOn, lastmodified, shared, currentrevision, dynamic_playlist.pltype, dynamic_playlist.plmode "
                         "FROM playlist, dynamic_playlist WHERE dynplaylist = 'true' AND playlist.guid = dynamic_playlist.guid AND dynamic_playlist.plmode = %1 AND dynamic_playlist.autoload = 'true' "
                         "%2"
                         "%3 %4 %5"
                       )
                       .arg( Static )
                       .arg( sourceToken )
                       .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( orderToken ) : QString() )
                       .arg( m_sortDescending ? "DESC" : QString() )
                       .arg( m_limitAmount > 0 ? QString( "LIMIT 0, %1" ).arg( m_limitAmount ) : QString() ) );

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
            emit autoPlaylistLoaded( source(), data );
    }

    emit done();
}

