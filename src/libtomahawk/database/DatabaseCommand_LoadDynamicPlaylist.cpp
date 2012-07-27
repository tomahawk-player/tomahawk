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

#include "DatabaseCommand_LoadDynamicPlaylist.h"

#include <QSqlQuery>

#include "DatabaseImpl.h"
#include "Source.h"
#include "dynamic/DynamicPlaylist.h"
#include "utils/Logger.h"
#include "Source.h"

using namespace Tomahawk;

Tomahawk::DatabaseCommand_LoadDynamicPlaylist::DatabaseCommand_LoadDynamicPlaylist( const source_ptr& s, const QString& guid, QObject* parent )
    : DatabaseCommand( s, parent )
    , m_plid( guid )
{

}


void
Tomahawk::DatabaseCommand_LoadDynamicPlaylist::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();

    query.exec( QString( "SELECT playlist.guid as guid, title, info, creator, createdOn, lastmodified, shared, currentrevision, dynamic_playlist.pltype, dynamic_playlist.plmode "
                         "FROM playlist, dynamic_playlist WHERE source %1 AND dynplaylist = 'true' AND playlist.guid = dynamic_playlist.guid AND playlist.guid = '%2'" )
    .arg( source()->isLocal() ? "IS NULL" : QString( "=%1" ).arg( source()->id() ) ).arg( m_plid ) );

    QList<dynplaylist_ptr> plists;
    if( query.next() )
    {
        dynplaylist_ptr p( new DynamicPlaylist( source(),
                                                query.value(7).toString(),  //current rev
                                                query.value(1).toString(),  //title
                                                query.value(2).toString(),  //info
                                                query.value(3).toString(),  //creator
                                                query.value(4).toUInt(),  //createdOn
                                                query.value(8).toString(),  // dynamic type
                                                static_cast<GeneratorMode>(query.value(9).toInt()),  // dynamic mode
                                                query.value(6).toBool(),    //shared
                                                query.value(5).toInt(),     //lastmod
                                                query.value(0).toString() ) );  //GUID

        p->setWeakSelf( p.toWeakRef() );
/*
        tLog() << "Loaded individual dynamic playlist:" <<      query.value(7).toString()  //current rev
        <<      query.value(1).toString()  //title
        <<      query.value(2).toString()  //info
        <<      query.value(3).toString()  //creator
        <<      query.value(4).toString()  //createdOn
        <<      query.value(8).toString()  // dynamic type
        <<      static_cast<GeneratorMode>(query.value(9).toInt())  // dynamic mode
        <<      query.value(6).toBool()    //shared
        <<      query.value(5).toInt()     //lastmod
        <<      query.value(0).toString();  //GUID */

        emit dynamicPlaylistLoaded( p );
    }

    emit done();
}

